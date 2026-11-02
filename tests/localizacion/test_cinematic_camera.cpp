#include "CinematicCamera.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 auto off=CinematicCamera::Sample(26,29,450,0,180);
 assert(off.yaw==26&&off.pitch==29&&off.distance==450);
 auto p0=CinematicCamera::Sample(26,29,450,100,0);
 auto p1=CinematicCamera::Sample(26,29,450,100,3);
 auto p2=CinematicCamera::Sample(26,29,450,100,9);
 assert(std::abs(p0.yaw-p1.yaw)>1);
 assert(std::abs(p1.distance-p2.distance)>1);
 for(int amount:{0,5,50,100}){
  for(int t=0;t<300;t+=13){
   auto p=CinematicCamera::Sample(172,5,180,amount,float(t));
   assert(std::isfinite(p.yaw)&&std::isfinite(p.distance));
   assert(p.pitch>=5&&p.pitch<=78);
   assert(p.distance>=180&&p.distance<=1250);
  }
 }
 assert(CinematicCamera::Sample(26,29,450,100,4).yaw==
        CinematicCamera::Sample(26,29,450,100,4).yaw);
 std::cout<<"OK: cinematic 3D camera motion, stationary zero and safe orbit bounds\n";
}
