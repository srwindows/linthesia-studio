#include "StageCamera.h"
#include "StageScenes.h"
#include <cassert>
#include <iostream>
#include <string>
int main(){
 assert(StageCamera::WrapYaw(205)==-155);
 assert(StageCamera::WrapYaw(-205)==155);
 assert(StageCamera::Clamp(280,300,1250)==300);
 assert(StageCamera::Clamp(2000,300,1250)==1250);
 for(int i=1;i<=8;++i){
  const auto c=StageCamera::Preset(i);
  assert(c.yaw>=-180&&c.yaw<=180);
  assert(c.pitch>=5&&c.pitch<=78);
  assert(c.zoom>=180&&c.zoom<=1250);
  assert(std::string(StageCamera::ViewName(i)).length()>3);
 }
 const auto top=StageCamera::Preset(5);
 const auto back=StageCamera::Preset(4);
 assert(top.pitch>65 && back.yaw>150);
 const auto closeup=StageCamera::Preset(7);
 assert(closeup.yaw>90 && closeup.zoom<=260);
 assert(std::string(StageCamera::ViewName(7))=="Primer plano");
 const auto hands=StageCamera::Preset(8);
 assert(hands.yaw>150 && hands.pitch>=60 && hands.zoom<=220);
 assert(std::string(StageCamera::ViewName(8))=="Manos cenital");
 assert(StageScenes::Count==13);
 std::cout<<"OK: orbit camera mouse constraints, six views and Blender scene selection\n";
}
