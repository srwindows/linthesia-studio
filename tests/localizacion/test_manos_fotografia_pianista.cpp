#include "PianoHands.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
using namespace PianoHands;
int main(){
 // In an overhead real pianist reference, left thumb faces inward (right)
 // and the right thumb faces inward (left). Former rig had both reversed.
 assert(IsThumb(0,4) && IsThumb(1,0));
 for(int slot=0;slot<4;++slot)assert(!IsThumb(0,slot));
 for(int slot=1;slot<5;++slot)assert(!IsThumb(1,slot));
 assert(DigitForSlot(0,0)==4);
 assert(DigitForSlot(1,4)==4);
 // Anthropometric taper: middle longest, little shortest, thumbs wider.
 assert(FingerWidth(1,0)>FingerWidth(1,4));
 assert(FingerWidth(0,4)>FingerWidth(0,0));
 assert(FingerArch(1,0)<FingerArch(1,2));
 assert(FingerArch(0,4)<FingerArch(0,2));
 // Thumb originates behind the four main knuckles, inside each palm.
 assert(BaseOffsetY(0,4,false)<BaseOffsetY(0,2,false));
 assert(BaseOffsetY(1,0,false)<BaseOffsetY(1,2,false));
 assert(BaseOffsetX(0,4)>BaseOffsetX(0,3));
 assert(BaseOffsetX(1,0)<BaseOffsetX(1,1));
 assert(BaseOffsetX(0,4)>.060f && BaseOffsetX(0,4)<.080f);
 assert(BaseOffsetX(1,0)<-.060f && BaseOffsetX(1,0)>-.080f);
 assert(BaseOffsetX(0,4)-BaseOffsetX(0,3)>.030f);
 assert(BaseOffsetX(1,1)-BaseOffsetX(1,0)>.030f);
 // Finger tips follow different depths within their own key, exactly as
 // visible in the photograph: thumb plays nearer front, middle deeper.
 for(int hand=0;hand<2;++hand){
  const int thumb=hand==0?4:0, middle=2;
  assert(FingerContactY(60,hand,thumb)<FingerContactY(60,hand,middle));
  assert(FingerContactY(61,hand,thumb)<FingerContactY(61,hand,middle));
  for(int slot=0;slot<5;++slot){
   const float white=FingerContactY(60,hand,slot);
   const float black=FingerContactY(61,hand,slot);
   assert(white>-.578f&&white<-.212f);
   assert(black>-.4465f&&black<-.2395f);
  }
 }
 Animator player;
 std::array<bool,128> held{};
 held[48]=held[52]=held[55]=held[64]=held[67]=held[71]=true;
 for(int i=0;i<130;++i)player.Update(held,.016f,100);
 for(int hand=0;hand<2;++hand){
  const auto& state=player.Current().hands[hand];
  for(int slot=0;slot<5;++slot){
   const auto& f=state.finger[slot];
   if(f.note<0)continue;
   assert(std::fabs(f.y-FingerContactY(f.note,hand,slot))<.004f);
   assert(f.press>.98f);
  }
 }
 std::cout<<"OK: mano izquierda/derecha, pulgares opuestos, cinco dedos anatomicos, acordes MIDI"<<std::endl;
}
