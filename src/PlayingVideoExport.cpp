#include "PlayingState.h"
#include "TrackSelectionState.h"
#include "MenuLayout.h"
#include "Renderer.h"
#include "TextWriter.h"
#include <SDL.h>
#include <glib.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <unistd.h>

namespace {
const char* const kFormats[]={"mp4","mkv","mov","webm","gif"};
const int kResolutions[][2]={{1280,720},{1920,1080},{2560,1440},{3840,2160}};
const int kRates[]={24,30,60};
const char* const kResNames[]={"HD 720p","Full HD 1080p","QHD 1440p","4K UHD"};
struct VideoRect {int x,y,w,h;};
VideoRect Panel(int w,int h){return {(w-650)/2,(h-518)/2,650,518};}
bool Contains(VideoRect r,const MouseInfo& mouse){
  return mouse.x>=r.x && mouse.x<r.x+r.w &&
         mouse.y>=r.y && mouse.y<r.y+r.h;
}
VideoRect Rel(VideoRect p,int x,int y,int w,int h){
  return {p.x+x,p.y+y,w,h};
}
VideoRect Dropdown(VideoRect p,int id){
  if(id==1)return Rel(p,24,134,180,5*29);
  if(id==2)return Rel(p,24,207,217,4*29);
  return Rel(p,261,207,178,3*29);
}
std::string WithFormat(std::string path,int fmt){
  if(path.empty())return {};
  const size_t pos=path.find_last_of('.');
  const size_t sep=path.find_last_of('/');
  if(pos!=std::string::npos && (sep==std::string::npos || pos>sep)){
    for(const auto* ext:kFormats)
      if(path.substr(pos+1)==ext){path.erase(pos);break;}
  }
  return path+"."+kFormats[fmt];
}
// A per-user preference, not tied to the song or its MIDI files. Only the
// directory is remembered: new songs still get their own generated name.
std::string ExportPrefsFile(){
  return std::string(g_get_user_config_dir())+"/linthesia/video-export.ini";
}
std::string LastExportFolder(){
  GKeyFile* prefs=g_key_file_new();
  const auto path=ExportPrefsFile();
  std::string dir;
  if(g_key_file_load_from_file(prefs,path.c_str(),G_KEY_FILE_NONE,nullptr)){
    gchar* saved=g_key_file_get_string(prefs,"Exportacion","carpeta",nullptr);
    if(saved){dir=saved;g_free(saved);}
  }
  g_key_file_free(prefs);
  return !dir.empty()&&g_path_is_absolute(dir.c_str())&&
         g_file_test(dir.c_str(),G_FILE_TEST_IS_DIR)?dir:std::string();
}
void RememberExportFolder(const std::string& fullFile){
  gchar* dir=g_path_get_dirname(fullFile.c_str());
  if(!dir)return;
  const std::string base=g_get_user_config_dir();
  const std::string folder=base+"/linthesia";
  if(g_mkdir_with_parents(folder.c_str(),0700)==0){
    GKeyFile* prefs=g_key_file_new();
    g_key_file_set_string(prefs,"Exportacion","carpeta",dir);
    gsize n=0;
    gchar* bytes=g_key_file_to_data(prefs,&n,nullptr);
    if(bytes)g_file_set_contents(ExportPrefsFile().c_str(),bytes,n,nullptr);
    g_free(bytes);g_key_file_free(prefs);
  }
  g_free(dir);
}
std::string Dirname(const std::string& file){
  gchar* dir=g_path_get_dirname(file.c_str());
  const std::string out=dir?dir:"";
  g_free(dir);return out;
}
std::string Basename(const std::string& file){
  gchar* name=g_path_get_basename(file.c_str());
  const std::string out=name?name:"";
  g_free(name);return out;
}
std::string ShortDisplay(const std::string& path,size_t limit){
  if(path.size()<=limit)return path;
  return "…"+path.substr(path.size()-limit+1);
}
std::string DefaultPath(const std::string& title){
  const char* videos=g_get_user_special_dir(G_USER_DIRECTORY_VIDEOS);
  const char* home=g_get_home_dir();
  const std::string last=LastExportFolder();
  std::string dir=!last.empty()?last:
       (videos&&g_file_test(videos,G_FILE_TEST_IS_DIR)?videos:
           std::string(home?home:"/tmp"));
  std::string basename=title.empty()?"escenario":title;
  for(auto& c:basename)
    if(!std::isalnum(static_cast<unsigned char>(c)) && c!='-' && c!='_')c='_';
  if(basename.size()>80)basename.resize(80);
  return dir+"/Linthesia_"+basename;
}
}

void PlayingState::OpenVideoDialog(){
  m_video_restore_paused=m_paused;
  m_paused=true;
  m_video_dialog=true;
  m_video_dropdown=0;
  m_video_status.clear();
  if(m_video_path.empty())m_video_path=DefaultPath(m_state.song_title);
  m_video_sinks=VideoExporter::AvailableMonitors();
  const auto defaultSink=VideoExporter::DefaultMonitor();
  m_video_sink_index=0;
  for(size_t i=0;i<m_video_sinks.size();++i)
    if(m_video_sinks[i]==defaultSink)m_video_sink_index=int(i);
  m_video_audio=!m_video_sinks.empty();
  if(m_state.midi_out)m_state.midi_out->Reset();
}
void PlayingState::CloseVideoDialog(){
  m_video_dialog=false;
  m_video_dropdown=0;
  m_paused=m_video_restore_paused;
  // Cancel from preflight returns to the screen where recording was chosen.
  if(m_video_prompt_on_enter){
    m_video_prompt_on_enter=false;
    if(m_state.midi_out)m_state.midi_out->Reset();
    ChangeState(new TrackSelectionState(m_state));
  }
}
void PlayingState::ChooseVideoFolder(){
  const std::string current=Dirname(WithFormat(m_video_path,m_video_format));
  gchar* output=nullptr;gchar* errout=nullptr;
  gint status=0;GError* error=nullptr;
  gchar* argv[]={const_cast<gchar*>("kdialog"),
      const_cast<gchar*>("--getexistingdirectory"),
      const_cast<gchar*>(current.c_str()),
      const_cast<gchar*>("--title"),
      const_cast<gchar*>("Elegir carpeta para guardar el vídeo"),nullptr};
  const gboolean ran=g_spawn_sync(nullptr,argv,nullptr,G_SPAWN_SEARCH_PATH,
                          nullptr,nullptr,&output,&errout,&status,&error);
  if(ran&&status==0&&output&&*output){
    std::string dir(output);
    dir.erase(dir.find_last_not_of("\r\n ")+1);
    if(g_path_is_absolute(dir.c_str())&&
       g_file_test(dir.c_str(),G_FILE_TEST_IS_DIR)){
      m_video_path=dir+"/"+Basename(m_video_path);
      RememberExportFolder(m_video_path);
      m_video_status="Carpeta seleccionada: "+dir;
    }else m_video_status="La carpeta elegida no existe.";
  }else if(!ran){
    m_video_status="No se pudo abrir el explorador de carpetas.";
  }
  if(error)g_error_free(error);
  g_free(output);g_free(errout);
}

void PlayingState::ChooseVideoFile(){
  const std::string suggested=WithFormat(m_video_path,m_video_format);
  gchar* output=nullptr;gchar* errout=nullptr;
  gint status=0;GError* error=nullptr;
  gchar* argv[]={const_cast<gchar*>("kdialog"),
     const_cast<gchar*>("--getsavefilename"),
     const_cast<gchar*>(suggested.c_str()),
     const_cast<gchar*>("Vídeos (*.mp4 *.mkv *.mov *.webm *.gif)"),
     const_cast<gchar*>("--title"),
     const_cast<gchar*>("Guardar vídeo de Linthesia"),nullptr};
  const gboolean ran=g_spawn_sync(nullptr,argv,nullptr,G_SPAWN_SEARCH_PATH,
                 nullptr,nullptr,&output,&errout,&status,&error);
  if(ran && status==0 && output && *output){
    std::string selected(output);
    selected.erase(selected.find_last_not_of("\r\n")+1);
    if(!selected.empty()&&g_path_is_absolute(selected.c_str())){
      m_video_path=selected;
      RememberExportFolder(m_video_path);
      m_video_status="Archivo de destino: "+WithFormat(m_video_path,m_video_format);
    }
  }else if(!ran){
    m_video_status=std::string("Explorador no disponible: ")+
       (error?error->message:"desconocido");
  }
  if(error)g_error_free(error);
  g_free(output);g_free(errout);
}

bool PlayingState::HandleVideoDialog(const MouseInfo& mouse){
  if(!m_video_dialog)return false;
  if(!mouse.released.left)return true;
  const auto p=Panel(GetStateWidth(),GetStateHeight());
  if(m_video_dropdown){
    const int count=m_video_dropdown==1?5:(m_video_dropdown==2?4:3);
    const auto dropdown=Dropdown(p,m_video_dropdown);
    if(Contains(dropdown,mouse)){
      const int index=(mouse.y-dropdown.y)/29;
      if(index>=0&&index<count){
        if(m_video_dropdown==1)m_video_format=index;
        if(m_video_dropdown==2)m_video_resolution=index;
        if(m_video_dropdown==3)m_video_fps=index;
      }
    }
    m_video_dropdown=0;
    return true;
  }
  if(Contains(Rel(p,607,15,31,30),mouse)||
     Contains(Rel(p,436,430,178,38),mouse)){
    CloseVideoDialog();return true;
  }
  if(Contains(Rel(p,24,100,180,34),mouse))
    m_video_dropdown=1;
  else if(Contains(Rel(p,24,173,217,34),mouse))
    m_video_dropdown=2;
  else if(Contains(Rel(p,261,173,178,34),mouse) && m_video_format!=4)
    m_video_dropdown=3;
  else if(Contains(Rel(p,24,246,233,34),mouse))
    m_video_audio=!m_video_audio;
  else if(Contains(Rel(p,268,246,345,34),mouse) && !m_video_sinks.empty())
    m_video_sink_index=(m_video_sink_index+1)%int(m_video_sinks.size());
  else if(Contains(Rel(p,455,306,158,34),mouse))
    ChooseVideoFolder();
  else if(Contains(Rel(p,455,367,158,34),mouse))
    ChooseVideoFile();
  else if(Contains(Rel(p,248,430,178,38),mouse))
    StartVideoExport();
  return true;
}

void PlayingState::StartVideoExport(){
  if(m_video.Active())return;
  VideoExporter::Options opts;
  opts.format=kFormats[m_video_format];
  opts.width=kResolutions[m_video_resolution][0];
  opts.height=kResolutions[m_video_resolution][1];
  opts.fps=m_video_format==4?15:kRates[m_video_fps];
  opts.path=WithFormat(m_video_path,m_video_format);
  if(!g_path_is_absolute(opts.path.c_str()) ||
     !g_file_test(Dirname(opts.path).c_str(),G_FILE_TEST_IS_DIR)){
    m_video_status="Selecciona una carpeta válida antes de generar el vídeo.";
    return;
  }
  if(access(Dirname(opts.path).c_str(),W_OK)!=0){
    m_video_status="No tienes permiso de escritura en la carpeta elegida.";
    return;
  }
  opts.audioMonitor=m_video_audio && !m_video_sinks.empty() ?
        m_video_sinks[std::min<int>(m_video_sink_index,m_video_sinks.size()-1)]:"";
  if(m_video_audio&&opts.audioMonitor.empty()){
    m_video_status="No se encontró salida de audio. Desactiva Audio para continuar.";
    return;
  }
  if(access(opts.path.c_str(),F_OK)==0){
    m_video_status="El archivo ya existe. Elige otro nombre con «Destino».";
    return;
  }
  std::string error;
  if(!m_video.Start(std::max(64,GetStateWidth()-32),
                    std::max(64,CalcKeyboardHeight()),opts,error)){
    m_video_status=error;return;
  }
  RememberExportFolder(opts.path);
  m_video_previous_modes=m_state.track_properties;
  m_video_previous_position=m_state.midi->GetSongPositionInMicroseconds();
  m_video_previous_paused=m_video_restore_paused;
  m_video_previous_loop=m_loop_active;
  m_loop_active=false;
  // Durante el render se tocan automáticamente SOLO las pistas elegidas.
  // No se reactiva ninguna pista marcada «No reproducir».
  for(auto& prop:m_state.track_properties)
    if(prop.mode!=Track::ModeNotPlayed &&
       prop.mode!=Track::ModePlayedButHidden)
      prop.mode=Track::ModePlayedAutomatically;
  m_required_notes.clear();
  SeekTo(m_state.midi->GetSongInitialPositionInMicroseconds());
  m_paused=false;
  m_video_dialog=false;
  m_video_prompt_on_enter=false;
  m_video_status.clear();
}
void PlayingState::StopVideoExport(bool cancel){
  if(!m_video.Active())return;
  std::string result;
  m_video.Stop(result,cancel);
  m_video_status=result;
  m_state.track_properties=m_video_previous_modes;
  m_video_previous_modes.clear();
  m_loop_active=m_video_previous_loop;
  SeekTo(m_video_previous_position);
  m_paused=m_video_previous_paused;
  if(m_state.midi_out)m_state.midi_out->Reset();
}
void PlayingState::CaptureVideoFrame() const{
  const int h=CalcKeyboardHeight();
  const int bottomY=GetStateHeight()-(94+h);
  m_video.Capture(16,std::max(0,bottomY));
}
void PlayingState::DrawVideoDialog(Renderer& renderer) const{
  const auto p=Panel(GetStateWidth(),GetStateHeight());
  const MouseInfo mouse=Mouse();
  const GLboolean previousBlend=glIsEnabled(GL_BLEND);
  GLint src=GL_ONE,dst=GL_ZERO;
  glGetIntegerv(GL_BLEND_SRC,&src);
  glGetIntegerv(GL_BLEND_DST,&dst);
  glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  renderer.SetColor(5,17,27,200);
  renderer.DrawQuad(0,0,GetStateWidth(),GetStateHeight());
  glBlendFunc(src,dst);
  if(!previousBlend)glDisable(GL_BLEND);
  renderer.SetColor(18,35,47);
  renderer.DrawQuad(p.x,p.y,p.w,p.h);
  renderer.SetColor(67,155,175);
  renderer.DrawQuad(p.x,p.y,p.w,4);
  renderer.SetColor(253,253,253);
  TextWriter title(p.x+24,p.y+20,renderer,false,22);
  title<<Text("Exportar vídeo del escenario",Renderer::ToColor(236,250,252));
  auto label=[&](int x,int y,const std::string& value){
    renderer.SetColor(255,255,255);
    TextWriter t(p.x+x,p.y+y,renderer,false,13);
    t<<Text(value,Renderer::ToColor(199,225,233));
  };
  auto button=[&](int x,int y,int w,int h,const std::string& caption,bool active=false){
    const auto rect=Rel(p,x,y,w,h);
    ButtonState state(rect.x,rect.y,rect.w,rect.h);
    state.Update(mouse);
    Layout::DrawCompactButton(renderer,state,GetTexture(ButtonSongs),caption,active,true,14);
  };
  button(606,13,31,28,"X");
  label(24,70,"Formato de vídeo");
  button(24,100,180,34,std::string(kFormats[m_video_format])+"   ▾");
  label(24,145,"Resolución");
  label(261,145,"Fotogramas por segundo");
  button(24,173,217,34,std::string(kResNames[m_video_resolution])+"   ▾");
  button(261,173,178,34,m_video_format==4?
         "15 FPS (GIF)":std::to_string(kRates[m_video_fps])+" FPS   ▾");
  label(24,221,"Sonido de la salida del sistema");
  button(24,246,233,34,m_video_audio?"Audio activado":"Sin audio",m_video_audio);
  if(m_video_format==4)label(269,257,"GIF sin pista de audio");
  else if(m_video_sinks.empty())label(269,257,"Sin salidas de audio detectadas");
  else {
    std::string sink=m_video_sinks[m_video_sink_index];
    if(sink.size()>44)sink="…"+sink.substr(sink.size()-42);
    button(268,246,345,34,sink+"   ▾");
  }
  // Show BOTH directory and filename clearly, with separate real native
  // file-picker actions. Users can pick any writable mounted drive.
  const std::string path=WithFormat(m_video_path,m_video_format);
  const std::string folder=Dirname(path);
  label(24,286,"CARPETA DONDE SE GUARDARÁ");
  renderer.SetColor(5,25,39);
  renderer.DrawQuad(p.x+24,p.y+306,420,34);
  renderer.SetColor(249,249,249);
  TextWriter folderText(p.x+31,p.y+315,renderer,false,12);
  folderText<<Text(ShortDisplay(folder,52),Renderer::ToColor(211,240,243));
  button(455,306,158,34,"Elegir carpeta");
  label(24,349,"NOMBRE DEL ARCHIVO");
  renderer.SetColor(5,25,39);
  renderer.DrawQuad(p.x+24,p.y+367,420,34);
  renderer.SetColor(249,249,249);
  TextWriter fileText(p.x+31,p.y+376,renderer,false,12);
  fileText<<Text(ShortDisplay(Basename(path),53),Renderer::ToColor(211,240,243));
  button(455,367,158,34,"Guardar como...");
  label(24,410,"Destino completo: "+ShortDisplay(path,72));
  button(248,430,178,38,"Generar vídeo",true);
  button(436,430,178,38,"Cancelar");
  if(!m_video_status.empty()){
    renderer.SetColor(255,219,165);
    TextWriter warning(p.x+24,p.y+479,renderer,false,11);
    warning<<Text(ShortDisplay(m_video_status,87),Renderer::ToColor(255,218,169));
  }
  // Un desplegable real, no controles que obliguen a recorrer valores
  // a ciegas uno por uno. Las opciones se eligen por clic directo.
  if(m_video_dropdown){
    const auto menu=Dropdown(p,m_video_dropdown);
    const int count=m_video_dropdown==1?5:(m_video_dropdown==2?4:3);
    renderer.SetColor(7,29,41);
    renderer.DrawQuad(menu.x-2,menu.y-2,menu.w+4,menu.h+4);
    for(int i=0;i<count;++i){
      const bool selected=m_video_dropdown==1?i==m_video_format:
         (m_video_dropdown==2?i==m_video_resolution:i==m_video_fps);
      const auto r=Rel(menu,0,29*i,menu.w,28);
      ButtonState state(r.x,r.y,r.w,r.h);
      state.Update(mouse);
      const std::string caption=m_video_dropdown==1?kFormats[i]:
         (m_video_dropdown==2?kResNames[i]:
           std::to_string(kRates[i])+" FPS");
      Layout::DrawCompactButton(renderer,state,GetTexture(ButtonSongs),
                                caption,selected,true,13);
    }
  }
}
