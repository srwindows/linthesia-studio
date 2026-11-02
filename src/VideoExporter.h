// Linthesia: exportación del framebuffer OpenGL mediante FFmpeg.
#ifndef LINTHESIA_VIDEO_EXPORTER_H
#define LINTHESIA_VIDEO_EXPORTER_H
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <sys/types.h>

class VideoExporter {
public:
  struct Options {
    std::string path;
    std::string format="mp4";
    std::string audioMonitor;
    int width=1920, height=1080, fps=30;
  };
  VideoExporter()=default;
  ~VideoExporter();
  bool Start(int sourceWidth,int sourceHeight,const Options& options,
             std::string& error);
  // called once per OpenGL display frame, top-level scene only
  void Capture(int x,int bottomY);
  bool Stop(std::string& result,bool cancel=false);
  bool Active() const {return m_pid>0;}
  // A failed FFmpeg pipe must not leave the UI saying "grabando" forever.
  bool Failed() const {return m_pipeError.load();}
  size_t EncodedFrames() const {return m_frames;}
  const std::string& Path() const {return m_options.path;}
  static std::string DefaultMonitor();
  static std::vector<std::string> AvailableMonitors();
private:
  void RunWriter();
  Options m_options;
  pid_t m_pid=-1;
  int m_pipe=-1,m_srcWidth=0,m_srcHeight=0;
  uint32_t m_startTick=0;
  size_t m_frames=0;
  std::mutex m_mutex;
  std::condition_variable m_ready;
  std::deque<std::vector<unsigned char>> m_queue;
  std::thread m_writer;
  bool m_finish=false;
  std::atomic<bool> m_pipeError{false};
};
#endif
