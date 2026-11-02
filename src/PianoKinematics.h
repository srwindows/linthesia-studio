// SPDX-License-Identifier: GPL-2.0-or-later
// Geometric two-link arm/elbow solver used by the OpenGL concert pianist.
// Preserves upper-arm and forearm lengths while the hand follows MIDI notes.
#ifndef LINTHESIA_PIANO_KINEMATICS_H
#define LINTHESIA_PIANO_KINEMATICS_H
#include <algorithm>
#include <cmath>
namespace PianoKinematics {
struct Vec3{
 float x,y,z;
 Vec3 operator+(const Vec3& o)const{return{x+o.x,y+o.y,z+o.z};}
 Vec3 operator-(const Vec3& o)const{return{x-o.x,y-o.y,z-o.z};}
 Vec3 operator*(float k)const{return{x*k,y*k,z*k};}
};
inline float Dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float Norm(Vec3 a){return std::sqrt(Dot(a,a));}
inline Vec3 Normalized(Vec3 a){
 const float l=Norm(a);return l<1e-7f?Vec3{0,1,0}:a*(1.f/l);
}
inline Vec3 Elbow(Vec3 shoulder,Vec3 wrist,Vec3 preferredBend,
                 float upperLength,float lowerLength){
 const Vec3 target=wrist-shoulder;
 const float d=std::max(.00001f,Norm(target));
 const Vec3 u=Normalized(target);
 const float lo=std::fabs(upperLength-lowerLength)+.0001f;
 const float hi=upperLength+lowerLength-.0001f;
 const float reach=std::max(lo,std::min(hi,d));
 const float along=(upperLength*upperLength-
                    lowerLength*lowerLength+
                    reach*reach)/(2.f*reach);
 const float height=std::sqrt(std::max(0.f,upperLength*upperLength-
                                           along*along));
 Vec3 bend=preferredBend-shoulder;
 bend=bend-u*Dot(bend,u);
 if(Norm(bend)<.0001f){
  const Vec3 alternate=std::fabs(u.z)<.9f?Vec3{0,0,1}:Vec3{0,1,0};
  bend=alternate-u*Dot(alternate,u);
 }
 return shoulder+u*along+Normalized(bend)*height;
}
}
#endif
