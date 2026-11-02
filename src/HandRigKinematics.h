// SPDX-License-Identifier: GPL-2.0-or-later
// Real-time 16-bone skinning for the contiguous Blender hand mesh.
// Anatomical rest pose (metres), 1 wrist + 5 digits x 3 joints.
#pragma once
#include "PianoHands.h"
#include "NativeHandRest.h"
#include <array>
#include <algorithm>
#include <cmath>
namespace HandRig {
struct V {
 float x=0,y=0,z=0;
 V operator+(V b)const{return {x+b.x,y+b.y,z+b.z};}
 V operator-(V b)const{return {x-b.x,y-b.y,z-b.z};}
 V operator*(float f)const{return {x*f,y*f,z*f};}
};
inline float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float Norm(V a){return std::sqrt(Dot(a,a));}
inline V Unit(V a){const float n=Norm(a);return n<1e-8f?V{0,1,0}:a*(1.f/n);}
struct Segment {
 V restA,restB,poseA,poseB;
 V Rotate(V p)const{
  const V oldAxis=Unit(restB-restA),newAxis=Unit(poseB-poseA);
  const float c=std::max(-1.f,std::min(1.f,Dot(oldAxis,newAxis)));
  if(c>.99999f)return p;
  if(c<-.99999f){
   const V ref=std::fabs(oldAxis.z)<.85f?V{0,0,1}:V{0,1,0};
   const V axis=Unit(Cross(oldAxis,ref));
   return p*(-1.f)+axis*(2.f*Dot(axis,p));
  }
  const V w=Cross(oldAxis,newAxis);
  return p+Cross(w,p)+Cross(w,Cross(w,p))*(1.f/(1.f+c));
 }
 V Map(V restPos)const{
  const V axis=Unit(restB-restA);
  const V rel=restPos-restA;
  const float along=Dot(rel,axis);
  const V perp=rel-axis*along;
  const float scale=std::max(.55f,std::min(1.45f,
                         Norm(poseB-poseA)/std::max(.001f,Norm(restB-restA))));
  return poseA+Unit(poseB-poseA)*(along*scale)+Rotate(perp);
 }
};
using Pose=std::array<Segment,16>;
// A live wrist should hover about 3cm above depressed white keys. The
// former 1.027m wrist and 1.067m MCP joint made fingers dive steeply
// into the keyboard and hide behind their own palms in overhead views.
inline float WristHeight(const PianoHands::Frame& frame,int hand){
 int active=0,raised=0;
 for(const auto& f:frame.hands[hand].finger)
  if(f.note>=21){++active;raised+=!PianoHands::White(f.note);}
 const float blackRatio=active?float(raised)/active:0.f;
 return .997f+.028f*blackRatio;
}
inline V RestJoint(int digit,int joint,int hand){
 const float* xyz=NativeHandRest::joints[hand][digit][joint];
 return {xyz[0],xyz[1],xyz[2]};
}
inline Pose Sample(const PianoHands::Frame& frame,int hand,V wrist){
 Pose pose{};
 pose[0]={{0,-.024f,0},{0,.064f,0},
          {0,-.024f,0},{0,.064f,0}};
 for(int slot=0;slot<5;++slot){
  const int digit=PianoHands::DigitForSlot(hand,slot);
  const auto& finger=frame.hands[hand].finger[slot];
  const bool black=finger.note>=21&&!PianoHands::White(finger.note);
  const float spread=PianoHands::IsThumb(hand,slot)?
                         (hand==0?.036f:-.036f):
                         PianoHands::BaseOffsetX(hand,slot);
  // A real metacarpal cannot remain stationary while the fingertip
  // reaches a distant key. Follow lateral travel at the MCP joint;
  // otherwise the old rig stretches its phalanges into spider legs.
  const float lateral=(finger.x-wrist.x)-spread;
  const float follow=std::max(-.085f,std::min(.085f,lateral));
  V base={spread+.58f*follow,
          PianoHands::BaseOffsetY(hand,slot,black),
          .005f+(black?.009f:0.f)};
  const V tip={finger.x-wrist.x,
               finger.y-wrist.y,
               finger.z-wrist.z};
  // The MCP follows the note if a finger would have to stretch beyond
  // its actual phalanx lengths. Moving the palm joint is less deforming
  // than extending three bones like rubber fingers.
  float naturalReach=0.f;
  for(int j=0;j<3;++j)
   naturalReach+=Norm(RestJoint(digit,j+1,hand)-
                      RestJoint(digit,j,hand));
  const V toTip=tip-base;
  const float distance=Norm(toTip);
  if(distance>naturalReach*1.06f)
   base=base+Unit(toTip)*
        std::min(.055f,distance-naturalReach*1.06f);
  const float arch=PianoHands::FingerArch(hand,slot);
  // Joint placement MUST respect each phalanx's actual rest length.
  // The old constant .31/.65 fractions stretched the short distal
  // segment by as much as 46%, producing the notorious claw shape.
  const float l0=Norm(RestJoint(digit,1,hand)-RestJoint(digit,0,hand));
  const float l1=Norm(RestJoint(digit,2,hand)-RestJoint(digit,1,hand));
  const float t0=l0/naturalReach;
  const float t1=(l0+l1)/naturalReach;
  const V chord=tip-base;
  const float span=Norm(chord);
  // A shallow 3D dorsal bend preserves phalanx lengths without a
  // sharp "spider leg" knuckle. The bend is perpendicular to reach.
  const V tangent=Unit(chord);
  const V dorsal=Unit(V{0.f,0.f,1.f}-tangent*tangent.z);
  const float reachArc=.52f*std::sqrt(std::max(0.f,
                       naturalReach*naturalReach-span*span));
  const float bend=std::min(.027f,std::max(arch*.35f,reachArc));
  const float pi=3.14159265f;
  V moved[4]={base,
               base*(1.f-t0)+tip*t0+
                 dorsal*(bend*std::sin(pi*t0)),
               base*(1.f-t1)+tip*t1+
                 dorsal*(bend*.14f),
               tip};
  for(int segment=0;segment<3;++segment){
   auto& bone=pose[1+digit*3+segment];
   bone={RestJoint(digit,segment,hand),
         RestJoint(digit,segment+1,hand),
         moved[segment],moved[segment+1]};
  }
 }
 return pose;
}
inline bool Valid(const Pose& pose){
 for(const auto& p:pose)for(V v:{p.restA,p.restB,p.poseA,p.poseB})
  if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))
   return false;
 return true;
}
} // namespace HandRig
