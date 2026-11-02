#include "FramePacer.h"
#include <cstdlib>
#include <iostream>

static void require(bool ok,const char* message){
  if(!ok){std::cerr<<message<<"\n";std::exit(1);}
}
static FramePacer::Clock::duration seconds(long double value){
  return std::chrono::duration_cast<FramePacer::Clock::duration>(
      std::chrono::duration<long double>(value));
}
int main(){
  using Clock=FramePacer::Clock;
  using TimePoint=FramePacer::TimePoint;
  const TimePoint origin(seconds(123.0L));
  for(int fps:{30,60,120,300}){
    FramePacer pacer(fps,origin);
    TimePoint now=origin;
    // Render cost varies but the nth deadline stays on the absolute grid.
    // Four hundred seconds at 300 Hz also exposes rounded-period drift.
    for(int frame=1;frame<=120000;++frame){
      const TimePoint target=pacer.NextDeadline(now);
      const TimePoint expected=origin+seconds(static_cast<long double>(frame)/fps);
      const auto error=target>expected?target-expected:expected-target;
      require(error<=Clock::duration(1),"absolute deadline accumulated drift");
      require(target>now,"pacer returned a past deadline");
      now=target+seconds((frame%2?.17L:.63L)/fps);
    }
    // A slow frame gets no additional sleep; the next frame has a fresh budget.
    FramePacer slow(fps,origin);
    const auto finished=origin+seconds(2.4L/fps);
    const auto afterSlow=slow.NextDeadline(finished);
    require(afterSlow==finished,"slow rendering received an extra sleep interval");
    const auto next=slow.NextDeadline(afterSlow+seconds(.15L/fps));
    require(next==afterSlow+seconds(1.0L/fps),"slow frame left catch-up debt");
    // Finishing exactly on deadline does not reset the absolute schedule.
    FramePacer exact(fps,origin);
    const auto onTime=origin+seconds(1.0L/fps);
    require(exact.NextDeadline(onTime)==onTime,"exact deadline was unnecessarily delayed");
    require(exact.NextDeadline(onTime+seconds(.1L/fps))==
                origin+seconds(2.0L/fps),"exact deadline broke the absolute schedule");
    // Long inactivity clears the backlog in constant time.
    FramePacer idle(fps,origin);
    now=origin+seconds(600.0001L);
    require(idle.NextDeadline(now)==now,"inactivity added another frame of waiting");
    require(idle.NextDeadline(now+seconds(.1L/fps))==now+seconds(1.0L/fps),
            "long inactivity left a backlog");
  }
  // A 17ms renderer on a 60Hz budget should run at its natural rate,
  // rather than being pushed to 30Hz by waiting for a second fixed slot.
  FramePacer overloaded(60,origin);
  TimePoint busy=origin;
  for(int frame=0;frame<1000;++frame){
    busy+=seconds(.017L);
    require(overloaded.NextDeadline(busy)==busy,"17ms renderer was throttled to 30Hz");
  }
  FramePacer settings(60,origin);
  const auto first=settings.NextDeadline(origin);
  settings.SetRate(60,first);
  require(settings.NextDeadline(first)==origin+seconds(2.0L/60),
          "unchanged frame rate reset the frame clock");
  const TimePoint changed=origin+seconds(.09L);
  settings.SetRate(120,changed);
  require(settings.Rate()==120&&settings.NextDeadline(changed)==changed+seconds(1.0L/120),
          "frame-rate change did not reset the interval");
  settings.Reset(changed);
  require(settings.NextDeadline(changed)==changed+seconds(1.0L/120),
          "reset did not restart from a clean deadline");
  require(FramePacer(0,origin).Rate()==60&&FramePacer(-1,origin).Rate()==60,
          "invalid rates did not fall back safely");
  std::cout<<"OK: absolute deadlines, no drift or backlog, 30/60/120/300 Hz and rate changes\n";
}
