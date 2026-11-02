#include "LoopErrorPolicy.h"
#include <cassert>
#include <iostream>
using LoopErrorPolicy::ShouldRestart;
int main(){
 assert(!ShouldRestart(0,0,100,true,true,false));
 assert(!ShouldRestart(2,2,4,false,true,false));
 assert(!ShouldRestart(2,2,4,true,false,false));
 assert(!ShouldRestart(2,2,4,true,true,true));
 assert(!ShouldRestart(2,5,6,true,true,false));
 assert(ShouldRestart(2,5,7,true,true,false));
 assert(ShouldRestart(1,2,3,true,true,false));
 assert(!ShouldRestart(21,2,999,true,true,false));
 assert(!ShouldRestart(3,10,9,true,true,false));
 // After a trip the baseline moves; the same wrong key cannot retrigger.
 assert(!ShouldRestart(2,7,7,true,true,false));
 std::cout<<"OK: disabled, active loop, paused, baseline, threshold, restart reset\n";
}
