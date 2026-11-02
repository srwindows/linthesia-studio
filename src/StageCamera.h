// GPL-2.0-or-later Linthesia orbital camera constraints and view presets.
#ifndef LINTHESIA_STAGE_CAMERA_H
#define LINTHESIA_STAGE_CAMERA_H
#include <algorithm>
namespace StageCamera {
inline int Clamp(int x,int lower,int upper){
 return std::max(lower,std::min(upper,x));
}
inline int WrapYaw(int yaw){
 while(yaw>180)yaw-=360;
 while(yaw< -180)yaw+=360;
 return yaw;
}
inline const char* ViewName(int id){
 static const char* names[]={
  "Libre","Frontal","Izquierda","Derecha","Detras","Cenital","Diagonal",
  "Primer plano","Manos cenital"
 };
 return names[Clamp(id,0,8)];
}
struct Angles{int yaw,pitch,zoom;};
inline Angles Preset(int id){
 switch(Clamp(id,0,8)){
  case 8:return {178,65,200};
  case 7:return {117,20,235};
  case 1:return {0,18,380};
  case 2:return {-73,25,470};
  case 3:return {73,25,470};
  case 4:return {177,28,490};
  case 5:return {20,75,600};
  case 6:return {92,26,405};
  default:return {0,18,380};
 }
}
}
#endif
