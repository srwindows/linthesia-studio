// SPDX-License-Identifier: GPL-2.0-or-later
#include "NoteReleaseFade.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 using namespace NoteReleaseFade;
 const std::int64_t off=25200000;
 assert(DurationUs==420000);
 assert(Gain(off-1,off)==1.f);
 assert(Gain(off,off)==1.f);
 assert(Visible(off,off));
 assert(Gain(off+DurationUs,off)==0.f);
 assert(!Visible(off+DurationUs,off));
 assert(Gain(off+DurationUs+1,off)==0.f);
 // A smoothstep envelope has symmetrical mid point and no jumps at boundaries.
 assert(std::abs(Gain(off+DurationUs/2,off)-0.5f)<.0001f);
 assert(Gain(off+1,off)>.99999f);
 assert(Gain(off+DurationUs-1,off)<.00001f);
 float previous=Gain(off,off);
 for(int millis=1;millis<=420;++millis){
   const float gain=Gain(off+millis*1000,off);
   assert(gain>=0.f && gain<=previous);
   previous=gain;
 }
 // No MIDI note can gain an extended sounding lifetime from visual release.
 assert(!Visible(off-1,off));
 puts("OK: release 420ms 1->0 monotonic smooth; NoteOff remains exact");
}
