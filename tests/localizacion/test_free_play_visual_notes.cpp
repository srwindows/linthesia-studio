#include "FreePlayVisualNotes.h"
#include <cassert>
#include <iostream>
using namespace FreePlay;
int main(){
 VisualNotes visual;
 visual.Event(0x90,60,110,1000);
 assert(visual.ActiveCount()==1);
 assert(visual.notes[0].velocity==110);
 assert(visual.Alpha(visual.notes[0],250000)==232);
 visual.Event(0x90,64,70,10000);
 assert(visual.ActiveCount()==2);
 visual.Event(0x80,60,0,501000);
 assert(visual.ActiveCount()==1);
 assert(visual.notes[0].release==501000);
 assert(visual.Alpha(visual.notes[0],501001)>=230);
 const int mid=visual.Alpha(visual.notes[0],501000+VisualNotes::FadeUs/2);
 assert(mid>90&&mid<140);
 assert(visual.Alpha(visual.notes[0],501000+VisualNotes::FadeUs-140000)>0);
 assert(visual.Alpha(visual.notes[0],501000+VisualNotes::FadeUs)==0);
 visual.Update(501000+VisualNotes::FadeUs);
 assert(visual.notes.size()==1 && visual.notes[0].pitch==64);
 visual.Event(0x90,64,90,900000);
 assert(visual.ActiveCount()==1 && visual.notes.size()==2);
 visual.Event(0x90,65,100,1000000);
 visual.Event(0x90,66,100,1100000);
 visual.ReleaseAll(1200000);
 assert(visual.ActiveCount()==0);
 assert(visual.notes[0].release==900000);
 visual.Update(1200000+VisualNotes::FadeUs);
 assert(visual.notes.empty());
 visual.Event(0xF0,60,100,3000000);
 assert(visual.notes.empty());
 visual.Event(0x91,72,110,3000000);
 visual.Event(0x81,72,0,3200000);
 assert(visual.notes.size()==1);
 assert(visual.notes[0].channel==1);
 assert(visual.ActiveCount()==0);
 std::cout<<"OK live note-on/mouse/MIDI note-off/retrigger/velocity/channel/smooth-fade/no-stuck-note\n";
}
