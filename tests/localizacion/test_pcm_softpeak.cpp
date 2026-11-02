#include "AudioPeakGuard.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 for(float x : {0.f,0.02f,-0.2f,0.65f,-0.65f}){
  float l=x,r=-x*0.4f;
  AudioPeakGuard::StereoSoftLimit(l,r);
  assert(std::abs(l-x)<1e-6f && std::abs(r+x*0.4f)<1e-6f);
 }
 for(int n=0;n<=20000;++n){
  const float x=(n-10000)/1000.f;
  float l=x,r=x*0.31f;
  AudioPeakGuard::StereoSoftLimit(l,r);
  assert(std::isfinite(l) && std::isfinite(r));
  assert(std::abs(l)<0.8415f && std::abs(r)<0.8415f);
  if(std::abs(x)>0.00001f)
   assert(std::abs(r/l-0.31f)<0.00003f);
 }
 float nan=std::nanf(""),inf=INFINITY;
 AudioPeakGuard::StereoSoftLimit(nan,inf);
 assert(nan==0 && inf==0);
 float a=0.65000f,b=0.65001f,c=0;
 AudioPeakGuard::StereoSoftLimit(a,c);
 AudioPeakGuard::StereoSoftLimit(b,c);
 assert(a<b && b-a>0 && b-a<0.000011f);
 std::cout<<"OK: float32 stereo linked, soft knee, output -1.5 dBFS max; quiet notes unchanged, no clipping\n";
}
