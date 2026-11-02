#include "PianoHands.h"
#include <cassert>
#include <array>
#include <cmath>
#include <iostream>
#include <set>
int main(){
 using namespace PianoHands;
 assert(ParseNoteName("A0")==21);
 assert(ParseNoteName("C4")==60);
 assert(ParseNoteName("C#4")==61);
 assert(ParseNoteName("C8")==108);
 assert(ParseNoteName("C-1")==0);
 assert(ParseNoteName("H5")==-1);
 assert(ParseNoteName("C4x")==-1);
 int whites=0,blacks=0;float old=-100;
 for(int n=21;n<=108;++n){
  if(White(n))whites++;else blacks++;
  assert(KeyX(n)>old);
  old=KeyX(n);
 }
 assert(whites==52&&blacks==36);
 assert(std::fabs(KeyX(21)-(-1.163f))<.001f);
 assert(KeyX(108)<.24f);
 Animator anim;
 std::array<bool,128> active{};
 auto advance=[&](int repeats){
   for(int i=0;i<repeats;++i)anim.Update(active,.016f,100);
 };
 advance(35);
 assert(anim.Current().liveNotes==0);
 assert(anim.Current().assignedNotes==0);
 active[48]=active[52]=active[55]=active[64]=active[67]=active[71]=true;
 advance(40);
 const auto& s=anim.Current();
 assert(s.liveNotes==6);
 assert(s.headNod>0.2f);
 assert(std::fabs(s.headYaw)<=9.01f);
 assert(s.shoulderRise>0.001f);
 assert(s.assignedNotes==6);
 std::set<int> assigned;
 for(const auto& hand:s.hands)
  for(const auto& finger:hand.finger)
   if(finger.note>=21){
    assert(active[finger.note]);
    assert(std::fabs(finger.x-KeyX(finger.note))<.003f);
    assert(finger.press>.85f);
    assigned.insert(finger.note);
   }
 assert(assigned.size()==6);
 for(int n=21;n<=108;++n)
  if(active[n])assert(s.keys[n]>.95f);
  else assert(s.keys[n]<.001f);
 // Sustained notes must remain in place, not repeat old frame-cycle poses.
 advance(60);
 bool sustainedNoteStayedOnItsKey=false;
 for(const auto& finger:anim.Current().hands[0].finger)
  if(finger.note==48 && std::fabs(finger.x-KeyX(48))<.003f)
   sustainedNoteStayedOnItsKey=true;
 assert(sustainedNoteStayedOnItsKey);
 // A new chord tone must not cause an already held note to jump fingers.
 std::array<int,128> prev{};prev.fill(-1);
 for(int hand=0;hand<2;++hand)
  for(int f=0;f<5;++f){
   const int pitch=anim.Current().hands[hand].finger[f].note;
   if(pitch>=21)prev[pitch]=hand*5+f;
  }
 active[73]=true;
 advance(22);
 for(int pitch:{48,52,55,64,67,71}){
  bool same=false;
  for(int hand=0;hand<2;++hand)
   for(int f=0;f<5;++f)
    if(anim.Current().hands[hand].finger[f].note==pitch &&
       prev[pitch]==hand*5+f)same=true;
  assert(same);
 }
 active[73]=false;
 advance(18);
 // On NoteOff, the key and finger release smoothly without residual hold.
 active[48]=active[52]=active[55]=active[64]=active[67]=active[71]=false;
 advance(42);
 assert(anim.Current().assignedNotes==0);
 for(int n:{48,52,55,64,67,71})assert(anim.Current().keys[n]<.001f);
 // Notes played in one octave divide between the hands, avoiding impossible
 // six-finger chords on one hand.
 for(int n=72;n<=77;++n)active[n]=true;
 advance(40);
 assert(anim.Current().assignedNotes==6);
 int leftAssigned=0,rightAssigned=0;
 for(const auto& finger:anim.Current().hands[0].finger)
  if(finger.note>=72)++leftAssigned;
 for(const auto& finger:anim.Current().hands[1].finger)
  if(finger.note>=72)++rightAssigned;
 assert(leftAssigned==3 && rightAssigned==3);
 // Animation control at zero stops gesture without muting any key.
 Animator stopped;
 stopped.Update(active,.016f,0);
 for(int i=0;i<35;++i)stopped.Update(active,.016f,0);
 assert(stopped.Current().assignedNotes==0);
 assert(stopped.Current().keys[72]>.98f);
 std::cout<<"OK: 88 keys, independent MIDI fingers, 6-note chords, key travel, release, sustain\n";
}
