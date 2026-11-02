#include "HandRigKinematics.h"
#include <array>
#include <cmath>
#include <cassert>
#include <iostream>
using namespace HandRig;
int main(){
 for(int hand=0;hand<2;++hand){
  for(int digit=0;digit<5;++digit){
   for(int j=0;j<3;++j){
    const V a=RestJoint(digit,j,hand),b=RestJoint(digit,j+1,hand);
    assert(Norm(b-a)>.007f && Norm(b-a)<.08f);
    assert(b.y>a.y);
   }
  }
 }
 assert(RestJoint(0,3,0).x>RestJoint(1,3,0).x);
 assert(RestJoint(0,3,1).x<RestJoint(1,3,1).x);
 std::array<bool,128> active{};
 PianoHands::Animator anim;
 for(int t=0;t<80;++t)anim.Update(active,.016f,100);
 const auto before=Sample(anim.Current(),0,
             {anim.Current().hands[0].wristX,
              anim.Current().hands[0].wristY,WristHeight(anim.Current(),0)});
 active[48]=active[52]=active[55]=active[64]=active[67]=active[71]=true;
 for(int t=0;t<90;++t)anim.Update(active,.016f,100);
 for(int hand=0;hand<2;++hand){
  const auto& state=anim.Current().hands[hand];
  const V wrist={state.wristX,state.wristY,WristHeight(anim.Current(),hand)};
  const Pose pose=Sample(anim.Current(),hand,wrist);
  assert(Valid(pose));
  for(const auto& bone:pose){
   assert(Norm(bone.restB-bone.restA)>.001f);
   assert(Norm(bone.poseB-bone.poseA)>.001f);
   for(V p:{bone.restA,bone.restB}){
    const auto out=bone.Map(p);
    assert(std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z));
    assert(Norm(out)<.65f);
   }
  }
  for(int slot=0;slot<5;++slot){
   const int d=PianoHands::DigitForSlot(hand,slot);
   const auto& f=state.finger[slot];
   const V tip=pose[1+d*3+2].poseB;
   assert(std::fabs(tip.x-(f.x-wrist.x))<1e-4f);
   assert(std::fabs(tip.y-(f.y-wrist.y))<1e-4f);
   assert(std::fabs(tip.z-(f.z-wrist.z))<1e-4f);
   const V base=pose[1+d*3].poseA;
   assert(base.z-tip.z<.048f);
  }
 }
 const auto after=Sample(anim.Current(),0,
             {anim.Current().hands[0].wristX,
              anim.Current().hands[0].wristY,WristHeight(anim.Current(),0)});
 assert(Norm(before[5].poseB-after[5].poseB)>.008f);
 // Blending across the phalange joint must not teleport a vertex.
 for(int d=0;d<5;++d){
  for(int bone=0;bone<2;++bone){
   const auto& a=after[1+d*3+bone];
   const auto& b=after[2+d*3+bone];
   assert(Norm(a.poseB-b.poseA)<1e-4f);
  }
 }
 // Six pitches in the same octave previously piled both wrists at the
 // same location. Both palms must remain visibly separated while all
 // six keys keep their own MIDI NoteOn state.
 active.fill(false);
 for(int n:{60,61,64,66,67,71})active[n]=true;
 for(int t=0;t<180;++t)anim.Update(active,.016f,100);
 const auto& dense=anim.Current();
 assert(dense.hands[1].wristX-dense.hands[0].wristX>.155f);
 for(int n:{60,61,64,66,67,71})assert(dense.keys[n]>.98f);
 std::cout<<"OK: full 16-bone rig, both hands, dynamic chord skinning and continuous phalange joints\n";
}
