#include "MidiActivity.h"
#include "libmidi/MidiEvent.h"
#include <cassert>
#include <SDL2/SDL.h>
#include <cstdio>
int main(){
 SDL_Init(SDL_INIT_TIMER);
 MidiActivity::Reset();
 auto base=MidiActivity::Now();
 assert(base.events==0 && base.note==-1);
 const MidiEvent e=MidiEvent::Build(MidiEventSimple(0x93,60,100));
 MidiActivity::Observe(e);
 auto snap=MidiActivity::Now();
 assert(snap.events==1);
 assert(snap.channel==4);
 assert(snap.note==60);
 assert(snap.velocity==100);
 assert(snap.bars[3]>0);
 for(int i=0;i<16;i++)if(i!=3)assert(snap.bars[i]==0);
 MidiActivity::Observe(MidiEvent::Build(MidiEventSimple(0x83,60,0)));
 snap=MidiActivity::Now();
 assert(snap.events==2 && snap.velocity==0);
 MidiActivity::Reset();
 assert(MidiActivity::Now().events==0);
 puts("PASS: monitor 16 canales, actividad, note-on/off, reset.");
}
