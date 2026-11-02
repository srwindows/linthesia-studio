#include "HandAutoDetector.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
static void add(TranslatedNoteSet& notes,int pitch,size_t track,unsigned long start){
 TranslatedNote n{};
 n.note_id=pitch;n.track_id=track;n.channel=0;
 n.start=start;n.end=start+100000;n.velocity=90;
 notes.insert(n);
}
int main(){
 TranslatedNoteSet hands;
 unsigned long t=0;
 for(int i=0;i<36;++i){
  add(hands,45+i%5,0,t++*125000);
  add(hands,69+i%6,0,t++*125000);
 }
 for(int i=0;i<24;++i)add(hands,79+i%4,1,t++*125000);
 auto none=HandAutoDetector::Detect(hands,{false,false},72);
 assert(!none.success && none.notes==0 && none.splitPoint==72);
 auto piano=HandAutoDetector::Detect(hands,{true,false},72,true);
 assert(piano.success && piano.notes==72);
 assert(piano.left>20 && piano.right>20);
 assert(piano.left+piano.right==piano.notes);
 assert(piano.pianoSelected && piano.splitPoint>=48 && piano.splitPoint<=84);
 auto manual=HandAutoDetector::Detect(hands,{true,false},68,true,false);
 assert(manual.success && manual.notes==72);
 assert(manual.splitPoint==68);
 assert(manual.left+manual.right==72);
 assert(hands.size()==96);

 // When the selected part is only the high right-hand melody, do not
 // invent a lower hand just to force a 50/50 split.
 auto right=HandAutoDetector::Detect(hands,{false,true},60);
 assert(right.success && right.notes==24);
 assert(right.left==0 && right.right==24);
 assert(right.splitPoint==60);
 // Percussion must never contribute to left/right estimates.
 TranslatedNote drum{};drum.note_id=40;drum.track_id=2;
 drum.channel=9;drum.start=t*125000;drum.end=drum.start+40000;
 hands.insert(drum);
 auto all=HandAutoDetector::Detect(hands,{true,true,true},60);
 assert(all.success && all.notes==96);
 assert(all.left+all.right==96);
 // The source note set must remain unchanged by any detection action.
 assert(hands.size()==97);
 std::cout<<"OK: detector selected hands, no-track fallback, solo RH, drums excluded, notes intact\n";
}
