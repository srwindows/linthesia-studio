#include "HandPractice.h"
#include <cassert>
#include <cstdio>
int main(){
 TranslatedNoteSet notes;
 auto add=[&](int pitch,int track,long long start,int ch=0){
  TranslatedNote n{};
  n.note_id=pitch;n.track_id=track;n.start=start;
  n.end=start+250000;n.channel=ch;n.velocity=88;
  n.state=AutoPlayed;n.retry_state=AutoPlayed;notes.insert(n);
 };
 for(int i=0;i<20;++i){
  add(43+i%3,0,i*500000);add(67+i%5,0,i*500000);
  add(76+i%3,1,i*500000);add(40+i%3,2,i*500000);
 }
 add(38,3,2000000,9);
 HandPractice::Classifier h;h.Build(notes);
 for(const auto& n:notes){
  if(n.channel==9)continue;
  if(n.track_id==1)assert(h.ForNote(n)==HandPractice::Right);
  if(n.track_id==2)assert(h.ForNote(n)==HandPractice::Left);
  if(n.track_id==0){
    assert(h.ForNote(n)==(n.note_id<60?HandPractice::Left:HandPractice::Right));
    assert(h.Selected(n,HandPractice::Both));
    assert(h.Selected(n,HandPractice::RightOnly)!=h.Selected(n,HandPractice::LeftOnly));
  }
 }
 TranslatedNoteSet separate;
 for(int i=0;i<12;++i){
   for(const auto part:std::vector<std::pair<int,int>>{{0,48+i%5},{1,60+i%6}}){
     TranslatedNote n{};
     n.start=i*500000;n.end=n.start+300000;
     n.note_id=part.second;n.track_id=part.first;n.channel=0;
     separate.insert(n);
   }
 }
 h.Build(separate);
 for(const auto& n:separate)
   assert(h.ForNote(n)==(n.track_id==0?HandPractice::Left:HandPractice::Right));
 assert(HandPractice::DiatonicOffset(64,HandPractice::Right)==0);
 assert(HandPractice::DiatonicOffset(43,HandPractice::Left)==0);
 assert(HandPractice::DiatonicOffset(60,HandPractice::Right)==-2);
 assert(HandPractice::VisualQuantize(121000,120,1)==0);
 assert(HandPractice::VisualQuantize(129000,120,1)==250000);
 assert(HandPractice::VisualQuantize(129000,120,0)==129000);
 puts("OK: two hands, mixed and separate tracks, staff, quantization non-destructive");
}
