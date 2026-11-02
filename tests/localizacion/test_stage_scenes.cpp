#include "StageScenes.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
int main(int argc,char** argv){
 assert(StageScenes::Count==13);
 assert(StageScenes::AnimationFrames==11);
 const char* performers[]={"1 · Pianista","2 · Guitarrista",
    "3 · Baterista","4 · Cantante masculino","5 · Cantante femenina",
    "6 · Dúo vocal","7 · Banda completa"};
 for(int i=6;i<13;++i){
  assert(std::string(StageScenes::Name(i))==performers[i-6]);
  assert(StageScenes::FrameCount(i)==(i==6?11:(i==12?12:8)));
  assert(StageScenes::PerformancePose(15.0,4,75,i)<
         StageScenes::FrameCount(i));
 }
 assert(std::string(StageScenes::Name(6))=="1 · Pianista");
 assert(std::string(StageScenes::Name(-1))=="Sin escenario");
 for(int i=0;i<7;++i){
   assert(std::string(StageScenes::Name(i)).size()>3);
   const auto c=StageScenes::LightColor(i);
   assert(c.r>=0 && c.g>=0 && c.b>=0);
 }
 assert(StageScenes::ClampPreset(-100)==-1);
 assert(StageScenes::ClampPreset(100)==12);
 assert(StageScenes::PianistPose(0.,0,0u,0)==0);
 assert(StageScenes::PianistPose(2.,6,5u,100)<11);
 assert(std::abs(StageScenes::CameraOffset(3.,0))<.0001);
 assert(std::abs(StageScenes::CameraOffset(3.,100))<=1.0);
 if(argc>1){
  const std::string p=argv[1];
  std::ifstream src(p+"/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend",
                   std::ios::binary);
  assert(src.good());
  // Obsolete stages 0-5 were archived; only pianist stage 6 is active.
  for(int i=0;i<13;++i){
   char buffer[120];
   snprintf(buffer,sizeof(buffer),"/graphics/scenarios/escena_%02d.tga",i);
   std::ifstream f(p+buffer,std::ios::binary);
   assert(f.good()==(i>=6));
  }
  for(int i=0;i<StageScenes::AnimationFrames;++i){
   char buffer[120];
   snprintf(buffer,sizeof(buffer),"/graphics/scenarios/pianista_%02d.tga",i);
   std::ifstream f(p+buffer,std::ios::binary);
   assert(f.good());
  }
 }
 puts("OK: Blender scenes, real stage resources, performance frame mapping and light/camera bounds");
}
