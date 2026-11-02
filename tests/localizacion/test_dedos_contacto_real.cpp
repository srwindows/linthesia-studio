#include "PianoHands.h"
#include <cassert>
#include <cmath>
#include <array>
#include <iostream>
// Regression: fingertips must make physical contact with depressed white
// and black key tops; no floating hand, no penetration below keyboard.
static float TipHeight(const PianoHands::Frame& frame,int note){
 for(int hand=0;hand<2;++hand)
  for(const auto& finger:frame.hands[hand].finger)
   if(finger.note==note && finger.press>.9f)return finger.z;
 return -1.f;
}
static float SurfaceAtFinger(const PianoHands::Frame& frame,int note){
 for(int hand=0;hand<2;++hand)
  for(const auto& finger:frame.hands[hand].finger)
   if(finger.note==note)
    return PianoHands::KeySurfaceZ(note,finger.y,frame.keys[note]);
 return -1.f;
}
static float PadRadius(const PianoHands::Frame& frame,int note){
 for(int hand=0;hand<2;++hand)
  for(int slot=0;slot<5;++slot)
   if(frame.hands[hand].finger[slot].note==note)
    return PianoHands::FingerWidth(hand,slot)*.66f;
 return -.1f;
}
static void settle(PianoHands::Animator& a,
                   const std::array<bool,128>& held,int frames){
 for(int i=0;i<frames;++i)a.Update(held,.016f,100);
}
int main(){
 for(int note:{48,52,55,60,61,63,64,67,69,72,73,76,77}){
  PianoHands::Animator animator;
  std::array<bool,128> held{};
  settle(animator,held,40);
  held[note]=true;
  settle(animator,held,110);
  const auto& state=animator.Current();
  const bool white=PianoHands::White(note);
  const float keySurface=SurfaceAtFinger(state,note);
  const float fingertip=TipHeight(state,note);
  assert(fingertip>0.f);
  // Fingertip is measured at centre; ~10 mm lower radius meets key top.
  const float radius=PadRadius(state,note);
  assert(radius>.0045f && radius<.0090f);
  assert(std::fabs((fingertip-radius)-keySurface)<.0030f);
  const float targetWrist=-.603f+(white?0.f:.055f);
  const int hand=note<60?0:1;
  // Single pitches are split between hands in the visualizer, so find
  // whichever hand owns this note rather than assuming a fixed division.
  for(const auto& h:state.hands)
   for(const auto& f:h.finger)
    if(f.note==note)assert(std::fabs(h.wristY-targetWrist)<.009f);
  (void)hand;
  held[note]=false;
  settle(animator,held,65);
  assert(state.assignedNotes==0);
 }
 std::cout<<"OK: 13 pressed white/black notes contact correctly, hands move to raised keys\n";
}
