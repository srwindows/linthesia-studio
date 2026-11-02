// Linthesia GPL-2.0-or-later. Blender-authored selectable stages.
#ifndef LINTHESIA_STAGE_SCENES_H
#define LINTHESIA_STAGE_SCENES_H
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace StageScenes {
constexpr int Count=13; // siete escenarios visibles: internos 6..12
constexpr int AnimationFrames=11;
inline const char* Name(int index){
 // Stable internal ids 6..12 preserve users' original piano-stage settings.
 // Only the UI numbers are one through seven, independent of stored ids.
 static constexpr const char* names[]={
  "1 · Pianista","2 · Guitarrista","3 · Baterista",
  "4 · Cantante masculino","5 · Cantante femenina",
  "6 · Dúo vocal","7 · Banda completa"};
 return index<0?"Sin escenario":
        index>=6&&index<Count?names[index-6]:"Escenario no disponible";
}
inline const char* ColorName(int index){
 static constexpr const char* names[]={
  "Blanco cálido","Azul","Cian","Violeta","Ámbar","Rojo","Verde"};
 return names[std::max(0,std::min(6,index))];
}
struct RGB{int r,g,b;};
inline RGB LightColor(int n){
 const RGB colors[7]={{255,225,195},{91,158,255},{68,235,250},
  {179,120,255},{255,181,79},{255,89,57},{99,255,169}};
 return colors[std::max(0,std::min(6,n))];
}
inline int ClampPreset(int n){return n<0?-1:std::max(6,std::min(12,n));}
inline int FrameCount(int stage){return stage==6?11:(stage==12?12:8);}
inline int PerformancePose(double t,int active,int motion,int stage){
 if(stage<7||stage>12||motion<=0||active<=0)return 0;
 const double rate=.9+2.0*motion/100.0;
 return int(std::floor(std::max(0.,t)*rate))%FrameCount(stage);
}
inline int ClampPercent(int n){return std::max(0,std::min(100,n));}
inline int PianistPose(double seconds,int activeKeys,uint32_t pitchHash,int movement){
 // Keep the hands resting when there is no active MIDI note. Choose a
 // key-contact pose from the notes, with only gradual movement while held.
 // The MIDI pose set approximates two-handed playing, not per-finger IK.
 if(movement==0||activeKeys==0)return 0;
 const int gesture=static_cast<int>(pitchHash%10u);
 const double slowRate=.03+movement*.0025; // avoid sweeping arms constantly
 const int articulation=int(std::floor(std::max(0.,seconds)*slowRate));
 return 1+(gesture+articulation)%10;
}
inline double CameraOffset(double seconds,int movement){
 if(movement==0)return 0.0;
 return std::sin(seconds*.21*(.5+movement/90.0))*movement/100.0;
}
}
#endif
