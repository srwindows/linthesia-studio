// SPDX-License-Identifier: GPL-2.0-or-later
// Slow virtual dolly/orbit independent from music speed and user presets.
// Zero intensity is exactly motionless; no accumulated camera drift.
#pragma once
#include <algorithm>
#include <cmath>
namespace CinematicCamera {
struct Pose {float yaw,pitch,distance;};
inline Pose Sample(float yaw,float pitch,float distance,int amount,float seconds){
 const float intensity=std::max(0,std::min(amount,100))/100.f;
 if(intensity==0.f)return {yaw,pitch,distance};
 const float phase=std::max(0.f,seconds);
 // Different, slow incommensurate periods avoid mechanical looping.
 const float turning=std::sin(phase*.173f);
 const float dolly=std::sin(phase*.226f-.83f);
 const float rise=std::sin(phase*.139f+.45f);
 return {
  yaw+intensity*8.f*turning,
  std::max(5.f,std::min(pitch+intensity*2.5f*rise,78.f)),
  std::max(180.f,std::min(distance*(1.f+intensity*.042f*dolly),1250.f))
 };
}
}
