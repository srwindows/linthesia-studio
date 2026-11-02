#include "PianoHands.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 PianoHands::Animator animator;
 std::array<bool,128> notes{};
 int white=0,black=0;
 for(int pitch=21;pitch<=108;++pitch){
  notes.fill(false);
  notes[pitch]=true;
  for(int frame=0;frame<40;++frame)animator.Update(notes,.016f,100);
  auto& state=animator.Current();
  assert(state.keys[pitch]>.99f);
  assert(state.liveNotes==1);
  bool attached=false;
  for(const auto& hand:state.hands)
   for(const auto& finger:hand.finger)
    if(finger.note==pitch && finger.press>.60f)attached=true;
  assert(attached);
  const float height=PianoHands::KeyTop(pitch)-
                     (PianoHands::White(pitch)?.014f:.010f)*state.keys[pitch];
  assert(std::isfinite(height));
  assert(height>.94f&&height<1.03f);
  white+=PianoHands::White(pitch);black+=!PianoHands::White(pitch);
  notes[pitch]=false;
  for(int frame=0;frame<65;++frame)animator.Update(notes,.016f,100);
  assert(animator.Current().keys[pitch]<.005f);
 }
 assert(white==52&&black==36);
 // The rear pivots almost in place. Front goes down 14/10 mm,
 // and the skin-contact calculation agrees with the OpenGL hinge.
 for(int note=21;note<=108;++note){
  const bool ivory=PianoHands::White(note);
  const float centre=ivory?-.420f:-.345f;
  const float length=ivory?.200f:.130f;
  const float rear=centre+length*.5f,front=centre-length*.5f;
  const float rearDrop=PianoHands::KeySurfaceZ(note,rear,0.f)-
                       PianoHands::KeySurfaceZ(note,rear,1.f);
  const float frontDrop=PianoHands::KeySurfaceZ(note,front,0.f)-
                        PianoHands::KeySurfaceZ(note,front,1.f);
  assert(std::fabs(rearDrop)<.0002f);
  assert(std::fabs(frontDrop-(ivory?.014f:.010f))<.0002f);
  assert(PianoHands::KeySurfaceZ(note,front,0.5f)>
         PianoHands::KeySurfaceZ(note,front,1.f));
 }
 std::cout<<"OK: 88 MIDI keys pressed/released, 52 white, 36 black, consistent hand state"<<std::endl;
}
