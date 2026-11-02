#include "PianoHands.h"
#include "HandRigKinematics.h"
#include "HandPractice.h"
#include <array>
#include <cmath>
#include <cassert>
#include <iostream>
using namespace PianoHands;
static bool HasKey(const Frame& frame,int hand,int finger,int note){
 const int slot=SlotForFinger(hand,finger);
 return slot>=0 && frame.hands[hand].finger[slot].note==note;
}
int main(){
 for(int hand=0;hand<2;++hand)
  for(int finger=1;finger<=5;++finger){
   const int slot=SlotForFinger(hand,finger);
   assert(slot>=0 && DigitForSlot(hand,slot)==finger-1);
  }
 assert(SlotForFinger(0,0)==-1);
 assert(SlotForFinger(1,6)==-1);
 std::array<bool,128> active{};
 Animator anim;
 Guidance score;
 // The same note's physical finger depends on the saved hand AND finger,
 // even if its pitch would normally be assigned to the other hand.
 active[48]=true;score.hands[48]=0;score.fingers[48]=5; // left little
 active[60]=true;score.hands[60]=1;score.fingers[60]=1; // right thumb
 active[72]=true;score.hands[72]=1;score.fingers[72]=5; // right little
 for(int i=0;i<110;++i)anim.Update(active,.016f,100,score);
 const Frame& first=anim.Current();
 assert(HasKey(first,0,5,48));
 assert(HasKey(first,1,1,60));
 assert(HasKey(first,1,5,72));
 assert(first.assignedNotes==3);
 for(int note:{48,60,72})assert(first.keys[note]>.95f);
 for(int hand=0;hand<2;++hand){
  const auto& state=first.hands[hand];
  const HandRig::V wrist={state.wristX,state.wristY,
                           HandRig::WristHeight(first,hand)};
  const auto pose=HandRig::Sample(first,hand,wrist);
  assert(HandRig::Valid(pose));
  for(int finger=1;finger<=5;++finger){
   const auto slot=SlotForFinger(hand,finger);
   const auto& f=state.finger[slot];
   if(f.note<21)continue;
   const int digit=finger-1;
   const HandRig::V tip=pose[1+digit*3+2].poseB;
   assert(std::fabs(tip.x-(f.x-wrist.x))<.0001f);
   assert(std::fabs(tip.y-(f.y-wrist.y))<.0001f);
   assert(std::fabs(tip.z-(f.z-wrist.z))<.0001f);
   assert(std::fabs(f.x-KeyX(f.note))<.005f);
  }
 }
 // Hold continuity when another note enters; annotated notes keep digits.
 active[67]=true;
 for(int i=0;i<70;++i)anim.Update(active,.016f,100,score);
 assert(HasKey(anim.Current(),0,5,48));
 assert(HasKey(anim.Current(),1,1,60));
 assert(HasKey(anim.Current(),1,5,72));
 assert(anim.Current().assignedNotes==4);
 // Move the right hand's high note to a low left-hand pitch explicitly.
 active.fill(false);score=Guidance();
 active[80]=true;score.hands[80]=0;score.fingers[80]=2;
 for(int i=0;i<100;++i)anim.Update(active,.016f,100,score);
 assert(HasKey(anim.Current(),0,2,80));
 assert(anim.Current().hands[1].finger[0].note<21);
 assert(anim.Current().assignedNotes==1);
 // No source MIDI NoteOn means no fingers, no matter what the score says.
 active.fill(false);
 for(int i=0;i<100;++i)anim.Update(active,.016f,100,score);
 assert(anim.Current().assignedNotes==0);
 assert(anim.Current().keys[80]<.001f);
 // Motion=0: key depresses but animated hands stay at rest.
 Animator frozen;
 active[60]=true;score=Guidance();score.hands[60]=1;score.fingers[60]=3;
 for(int i=0;i<100;++i)frozen.Update(active,.016f,0,score);
 assert(frozen.Current().keys[60]>.95f);
 assert(frozen.Current().assignedNotes==0);
 // Old songs without annotations retain the original automatic split.
 Animator automatic;
 Guidance none;
 active.fill(false);
 for(int n=72;n<=77;++n)active[n]=true;
 for(int i=0;i<85;++i)automatic.Update(active,.016f,100,none);
 assert(automatic.Current().assignedNotes==6);
 int left=0,right=0;
 for(const auto& f:automatic.Current().hands[0].finger)if(f.note>=21)++left;
 for(const auto& f:automatic.Current().hands[1].finger)if(f.note>=21)++right;
 assert(left==3&&right==3);
 std::cout<<"OK: thumb..little exact MIDI digit, both hands mirrored, full 16-bone rig, NoteOff & idle"<<std::endl;
}
