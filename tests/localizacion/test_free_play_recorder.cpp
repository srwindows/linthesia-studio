#include "FreePlayRecorder.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace FreePlay;
int main(){
 char path[]="/tmp/linthesia-freeplay-test-XXXXXX";
 assert(mkdtemp(path));
 std::string prefix=std::string(path)+"/";
 Recorder r;
 assert(!r.Save(prefix+"empty.mid"));
 assert(r.Start());
 assert(r.recording && r.activeTrack==0);
 assert(!r.Start());
 r.Push(0,0x90,60,100);
 r.Push(120000,0xB0,64,127); // sustain pedal
 r.Push(220000,0xE0,0,80);   // pitch bend
 r.Push(380000,0xD0,40,0);   // channel pressure
 r.Push(490000,0xA0,60,70);  // poly aftertouch
 r.Push(500000,0x90,60,0);
 r.Stop(500000);
 assert(r.tracks.size()==1 && r.tracks[0].events.size()==6);
 assert(!r.recording);
 assert(r.Start());
 r.Push(0,0x90,67,80);  // unfinished note must be closed on Stop
 r.Stop(250000);
 assert(r.tracks[1].events.size()==2);
 assert((r.tracks[1].events[1].status&0xf0)==0x80);
 assert(r.SetProgram(1,22));
 assert(!r.SetProgram(1,128));
 assert(!r.Delete(10));
 assert(r.Save(prefix+"song.mid"));
 auto bytes=r.ToMidi();
 assert(bytes.size()>72);
 assert(std::string(bytes.begin(),bytes.begin()+4)=="MThd");
 assert(bytes[8]==0 && bytes[9]==1);
 assert(bytes[10]==0 && bytes[11]==3); // tempo + two tracks
 assert(bytes[12]==3 && bytes[13]==192); // 960 PPQN
 assert(r.SaveSession(prefix+"session"));
 Recorder loaded;
 assert(loaded.Restore(prefix+"session"));
 assert(loaded.tracks.size()==2);
 assert(loaded.tracks[0].events.size()==6);
 assert(loaded.tracks[1].events.size()==2);
 assert(loaded.tracks[1].program==22);
 assert(loaded.bpm==120);
 for(int i=0;i<4;++i){
   assert(loaded.Start());
   loaded.Push(0,0x90,uint8_t(69+i),60);
   loaded.Stop(300000);
 }
 assert(loaded.tracks.size()==6);
 assert(!loaded.Start());
 assert(!loaded.recording);
 assert(loaded.Delete(2));
 assert(loaded.tracks.size()==5);
 assert(loaded.Start());
 loaded.Stop(2000); // empty track discarded
 assert(loaded.tracks.size()==5);
 loaded.quantize=true;loaded.bpm=90;
 assert(loaded.SaveSession(prefix+"session-new"));
 Recorder again;
 assert(again.Restore(prefix+"session-new"));
 assert(again.tracks.size()==5 && again.quantize && again.bpm==90);
 assert(again.ToMidi().size()>128);
 assert(!again.Restore(prefix+"missing"));
 std::ofstream(prefix+"bad")<<"not a session";
 assert(!again.Restore(prefix+"bad"));
 assert(again.tracks.size()==5); // reject corrupt file without losing data
 assert(!Recorder::ValidEvent(0xf0,1,2));
 assert(!Recorder::ValidEvent(0x90,128,10));
 std::cout<<"OK: 6 independent MIDI tracks, sustain, pitch, pressure, note-close, SMF, quantize, session recovery and data isolation\n";
}
