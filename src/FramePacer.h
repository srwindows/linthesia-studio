#ifndef LINTHESIA_FRAME_PACER_H
#define LINTHESIA_FRAME_PACER_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

// Absolute steady-clock deadlines keep rendering time inside each frame.
// Missed frames are skipped, so a slow frame cannot trigger a catch-up burst.
class FramePacer {
public:
  using Clock=std::chrono::steady_clock;
  using TimePoint=Clock::time_point;

  explicit FramePacer(int fps,TimePoint now=Clock::now()):
      m_rate(ValidRate(fps)),m_origin(now),m_frame(1){}

  int Rate() const{return m_rate;}
  void Reset(TimePoint now){m_origin=now;m_frame=1;}
  void SetRate(int fps,TimePoint now){
    const int rate=ValidRate(fps);
    if(rate!=m_rate){m_rate=rate;Reset(now);}
  }

  TimePoint NextDeadline(TimePoint now){
    TimePoint deadline=At(m_frame);
    if(now>deadline){
      // Rendering already consumed this interval. Do not add another
      // interval of sleep or keep missed deadlines as catch-up debt.
      Reset(now);
      return now;
    }
    ++m_frame;
    return deadline;
  }

private:
  static int ValidRate(int fps){return fps>0?std::min(fps,1000):60;}
  TimePoint At(std::uint64_t frame) const{
    // Recompute from the origin; rounding a fractional period once per
    // deadline avoids accumulating truncation error at 60/120/300 Hz.
    return m_origin+std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<long double>(
            static_cast<long double>(frame)/m_rate));
  }
  int m_rate;
  TimePoint m_origin;
  std::uint64_t m_frame;
};
#endif
