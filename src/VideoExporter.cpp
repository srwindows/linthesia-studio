#include "VideoExporter.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

VideoExporter::~VideoExporter(){
  if(Active()){std::string ignored;Stop(ignored,true);}
}

std::string VideoExporter::DefaultMonitor(){
  FILE* p=popen("pactl get-default-sink 2>/dev/null","r");
  if(!p)return {};
  char buffer[256]={};
  const bool ok=fgets(buffer,sizeof(buffer),p)!=nullptr;
  pclose(p);
  if(!ok)return {};
  std::string sink(buffer);
  sink.erase(sink.find_last_not_of("\r\n ")+1);
  if(sink.empty() || sink=="@DEFAULT_SINK@")return {};
  return sink+".monitor";
}

std::vector<std::string> VideoExporter::AvailableMonitors(){
  std::vector<std::string> names;
  FILE* p=popen("pactl list short sinks 2>/dev/null","r");
  if(!p)return names;
  char line[1024];
  while(fgets(line,sizeof(line),p)){
    char* save=nullptr;
    char* idx=strtok_r(line,"\t",&save);
    char* name=strtok_r(nullptr,"\t",&save);
    if(!idx||!name||!*name)continue;
    names.emplace_back(std::string(name)+".monitor");
  }
  pclose(p);
  return names;
}

bool VideoExporter::Start(int sourceWidth,int sourceHeight,
                          const Options& options,std::string& error){
  if(Active()){error="Ya hay una exportación activa";return false;}
  if(sourceWidth<64||sourceHeight<64||options.path.empty() ||
     options.fps<1 || options.fps>60 || options.width<160 || options.height<90){
    error="Parámetros de vídeo no válidos";return false;
  }
  const bool gif=options.format=="gif";
  const bool webm=options.format=="webm";
  const bool mov=options.format=="mov";
  const bool mp4=options.format=="mp4";
  const bool mkv=options.format=="mkv";
  if(!gif&&!webm&&!mov&&!mp4&&!mkv){error="Formato desconocido";return false;}
  int fds[2];
  if(pipe(fds)!=0){error=std::string("Pipe: ")+strerror(errno);return false;}
  // Prevent a failed encoder from killing Linthesia with SIGPIPE.
  signal(SIGPIPE,SIG_IGN);
  const pid_t child=fork();
  if(child<0){close(fds[0]);close(fds[1]);error="No se pudo abrir FFmpeg";return false;}
  if(child==0){
    close(fds[1]);dup2(fds[0],STDIN_FILENO);close(fds[0]);
    const int log=open((options.path+".ffmpeg.log").c_str(),
                      O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(log>=0){dup2(log,STDERR_FILENO);close(log);}
    const std::string inputSize=std::to_string(sourceWidth)+"x"+std::to_string(sourceHeight);
    const std::string fps=std::to_string(options.fps);
    const std::string scale="vflip,scale="+std::to_string(options.width)+":"+
                  std::to_string(options.height)+":flags=lanczos:force_original_aspect_ratio=decrease,"+
                  "pad="+std::to_string(options.width)+":"+std::to_string(options.height)+":(ow-iw)/2:(oh-ih)/2,setsar=1"+
                  (gif?",fps=15":",format=yuv420p");
    std::vector<std::string> args={"ffmpeg","-hide_banner","-loglevel","error",
       "-nostdin","-y","-thread_queue_size","1024", "-f","rawvideo",
       "-pixel_format","rgb24","-video_size",inputSize,
       "-framerate",fps,"-i","pipe:0"};
    const bool audio=!gif&&!options.audioMonitor.empty();
    if(audio){args.insert(args.end(),{"-thread_queue_size","1024","-f","pulse",
           "-i",options.audioMonitor});}
    args.insert(args.end(),{"-map","0:v:0"});
    if(audio)args.insert(args.end(),{"-map","1:a:0"});
    args.insert(args.end(),{"-vf",scale});
    if(gif)args.insert(args.end(),{"-c:v","gif"});
    else if(webm)args.insert(args.end(),{"-c:v","libvpx-vp9","-deadline","realtime",
                    "-cpu-used","5","-b:v","0","-crf","30"});
    else args.insert(args.end(),{"-c:v","libx264","-preset","veryfast",
                     "-crf","20"});
    if(audio){
      if(webm)args.insert(args.end(),{"-c:a","libopus","-b:a","160k"});
      else args.insert(args.end(),{"-c:a","aac","-b:a","192k"});
      args.insert(args.end(),{"-af","aresample=async=1:first_pts=0"});
      args.insert(args.end(),{"-shortest"});
    }
    if(mp4||mov)args.insert(args.end(),{"-movflags","+faststart"});
    args.push_back(options.path);
    std::vector<char*> argv;
    for(auto& a:args)argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    execvp("ffmpeg",argv.data());
    _exit(127);
  }
  close(fds[0]);
  m_pid=child;m_pipe=fds[1];m_srcWidth=sourceWidth;m_srcHeight=sourceHeight;
  m_options=options;m_startTick=SDL_GetTicks();m_frames=0;
  m_finish=false;m_pipeError=false;
  m_queue.clear();
  m_writer=std::thread(&VideoExporter::RunWriter,this);
  return true;
}

void VideoExporter::RunWriter(){
  for(;;){
    std::vector<unsigned char> frame;
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_ready.wait(lock,[this]{return m_finish||!m_queue.empty();});
      if(m_queue.empty())break;
      frame=std::move(m_queue.front());m_queue.pop_front();
    }
    size_t sent=0;
    while(sent<frame.size()){
      const ssize_t n=write(m_pipe,frame.data()+sent,frame.size()-sent);
      if(n>0)sent+=size_t(n);
      else if(n<0&&errno==EINTR)continue;
      else {m_pipeError=true;break;}
    }
    if(m_pipeError)break;
  }
  close(m_pipe);m_pipe=-1;
}

void VideoExporter::Capture(int x,int bottomY){
  if(!Active()||m_pipeError)return;
  const uint32_t elapsed=SDL_GetTicks()-m_startTick;
  const size_t expected=1+(static_cast<uint64_t>(elapsed)*m_options.fps/1000);
  if(m_frames>=expected)return;
  // Bound memory under slow encoders. Never block or freeze the 3D animator.
  {std::lock_guard<std::mutex> lock(m_mutex);
   if(m_queue.size()>9)return;}
  std::vector<unsigned char> frame(size_t(m_srcWidth)*m_srcHeight*3);
  GLint pack=4,read=GL_BACK;
  glGetIntegerv(GL_PACK_ALIGNMENT,&pack);
  glGetIntegerv(GL_READ_BUFFER,&read);
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  glReadBuffer(GL_BACK);
  glReadPixels(x,bottomY,m_srcWidth,m_srcHeight,GL_RGB,GL_UNSIGNED_BYTE,frame.data());
  glReadBuffer(read);glPixelStorei(GL_PACK_ALIGNMENT,pack);
  if(glGetError()!=GL_NO_ERROR){m_pipeError=true;return;}
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(frame);
    ++m_frames;
    // Duplicate frames when the GPU rendered slower than nominal FPS,
    // preserving real-time audio/video duration without speeding up music.
    const size_t missing=std::min<size_t>(3,expected-m_frames);
    for(size_t i=0;i<missing&&m_queue.size()<10;i++){
      m_queue.push_back(frame);++m_frames;
    }
  }
  m_ready.notify_one();
}

bool VideoExporter::Stop(std::string& result,bool cancel){
  if(!Active()){result="No había exportación activa";return false;}
  {std::lock_guard<std::mutex> lock(m_mutex);
   if(cancel)m_queue.clear();
   m_finish=true;}
  m_ready.notify_one();
  // Un encodificador bloqueado puede mantener la escritura al pipe
  // esperando; terminarlo ANTES del join permite cancelar sin colgar UI.
  if(cancel)kill(m_pid,SIGTERM);
  if(m_writer.joinable())m_writer.join();
  int status=0;
  while(waitpid(m_pid,&status,0)<0 && errno==EINTR){}
  const bool success=!cancel&&!m_pipeError&&WIFEXITED(status)&&WEXITSTATUS(status)==0;
  if(cancel)unlink(m_options.path.c_str());
  if(success){unlink((m_options.path+".ffmpeg.log").c_str());
    result="Vídeo guardado: "+m_options.path;}
  else if(cancel)result="Exportación cancelada; vídeo parcial eliminado";
  else result="Error al codificar. Consultar "+m_options.path+".ffmpeg.log";
  m_pid=-1;m_frames=0;m_finish=false;m_pipeError=false;
  return success;
}
