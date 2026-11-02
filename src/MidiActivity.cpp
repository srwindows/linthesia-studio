#include "MidiActivity.h"
#include "libmidi/MidiEvent.h"
#include <SDL2/SDL.h>
#include <algorithm>
namespace {
 uint64_t eventCount=0;
 Uint32 timestamps[16]={};
 int intensities[16]={};
 int lastCh=0,lastNote=-1,lastVelocity=0;
 std::string message="Esperando eventos MIDI";
}
namespace MidiActivity {
void Observe(const MidiEvent& ev){
 const MidiEventType type=ev.Type();
 const int ch=std::max(0,std::min(15,static_cast<int>(ev.Channel())));
 MidiEventSimple simple;
 if(type==MidiEventType_NoteOn){
  if(ev.NoteVelocity()>0){
   lastNote=ev.NoteNumber();lastVelocity=ev.NoteVelocity();
   message="Nota ON "+std::to_string(lastNote)+" · canal "+std::to_string(ch+1);
   intensities[ch]=std::max(intensities[ch],ev.NoteVelocity());
  }else{
   message="Nota OFF "+std::to_string(ev.NoteNumber());
   lastVelocity=0;
  }
 }else if(type==MidiEventType_NoteOff){
  lastNote=ev.NoteNumber();lastVelocity=0;
  message="Nota OFF "+std::to_string(lastNote)+" · canal "+std::to_string(ch+1);
 }else if(type==MidiEventType_Controller && ev.GetSimpleEvent(&simple)){
  message="CC "+std::to_string(simple.byte1)+
          " = "+std::to_string(simple.byte2);
  intensities[ch]=std::max(intensities[ch],static_cast<int>(simple.byte2)/2);
 }else if(type==MidiEventType_ProgramChange){
  message="Programa "+std::to_string(ev.ProgramNumber());
 }else return;
 ++eventCount;lastCh=ch+1;timestamps[ch]=SDL_GetTicks();
}
Snapshot Now(){
 Snapshot s;
 s.events=eventCount;s.channel=lastCh;s.note=lastNote;
 s.velocity=lastVelocity;s.last=message;
 const Uint32 now=SDL_GetTicks();
 for(int i=0;i<16;i++){
  const Uint32 elapsed=now-timestamps[i];
  s.bars[i]=intensities[i]==0||elapsed>=1250?0:
       (intensities[i]*(1250-elapsed)/1250);
 }
 return s;
}
void Reset(){
 eventCount=0;lastCh=0;lastNote=-1;lastVelocity=0;
 message="Esperando eventos MIDI";
 for(int i=0;i<16;i++){intensities[i]=0;timestamps[i]=0;}
}
}
