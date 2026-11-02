// SPDX-License-Identifier: GPL-2.0-or-later
#include "HandCorrectionStore.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <string>
int main(){
 char path[]="/tmp/linthesia-hand-range-finger-XXXXXX";
 assert(mkdtemp(path));
 setenv("XDG_CONFIG_HOME",path,1);
 const std::string song=std::string(path)+"/study.mid";
 {std::ofstream midi(song,std::ios::binary);midi<<"MThd";}
 TranslatedNoteSet score;
 for(int n=0;n<8;++n){
  TranslatedNote note{};
  note.track_id=0;note.channel=0;note.note_id=60+n%3;
  note.start=n*500000;note.end=note.start+250000;
  note.state=AutoPlayed;note.retry_state=AutoPlayed;
  score.insert(note);
 }
 // GM drums are NOT reassigned by applying a hand over a time region.
 TranslatedNote drum{};
 drum.track_id=1;drum.channel=9;drum.note_id=36;
 drum.start=1000000;drum.end=1100000;
 score.insert(drum);
 HandPractice::Classifier c;c.Build(score);
 const auto before=c.Corrections();
 const size_t count=c.AssignRange(1000000,2500000,HandPractice::Left);
 assert(count==3);
 for(const auto& n:score)if(n.channel!=9){
  const bool selected=n.start>=1000000 && n.start<2500000;
  if(selected)assert(c.ForNote(n)==HandPractice::Left);
  else assert(!c.IsCorrected(n.track_id,n.note_id,n.start));
 }
 assert(c.AssignRange(2500000,1000000,HandPractice::Right)==0);
 assert(c.AssignRange(3000000,3000000,HandPractice::Left)==0);
 // Right-hand assignment is equally constrained by the chosen interval.
 assert(c.AssignRange(1000000,2500000,HandPractice::Right)==3);
 for(const auto& n:score)
   if(n.channel!=9 && n.start>=1000000 && n.start<2500000)
     assert(c.ForNote(n)==HandPractice::Right);
 // Continue with the left-hand annotations for persistence and undo tests.
 assert(c.AssignRange(1000000,2500000,HandPractice::Left)==3);
 assert(c.SetFinger(0,60,0,1));
 assert(c.SetFinger(0,61,500000,3));
 assert(!c.SetFinger(0,60,0,0));
 assert(!c.SetFinger(0,60,0,6));
 assert(!c.SetFinger(1,36,1000000,2));
 assert(c.Finger(0,60,0)==1);
 assert(c.Finger(0,61,500000)==3);
 assert(HandCorrectionStore::SaveAll(song,c.Corrections(),c.Fingerings()));
 const auto data=HandCorrectionStore::LoadAll(song);
 assert(data.hands.size()==3);
 assert(data.fingers.size()==2);
 assert(HandCorrectionStore::Load(song).size()==3);
 assert(HandCorrectionStore::LoadFingers(song).size()==2);
 {
  std::ifstream input(HandCorrectionStore::File(song));
  std::string magic;
  std::getline(input,magic);
  assert(magic=="LINTHESIA_HAND_ASSIGNMENTS_2");
 }
 HandPractice::Classifier again;again.Build(score);
 for(const auto& hand:data.hands)
  assert(again.SetCorrection(hand.track,hand.pitch,hand.when,hand.hand));
 for(const auto& finger:data.fingers)
  assert(again.SetFinger(finger.track,finger.pitch,finger.when,finger.finger));
 assert(again.Finger(0,61,500000)==3);
 assert(again.ClearFinger(0,61,500000));
 assert(again.Finger(0,61,500000)==0);
 again.RestoreCorrections(before); // Undo bulk assignment
 assert(again.Corrections().empty());
 assert(HandCorrectionStore::SaveAll(song,again.Corrections(),again.Fingerings()));
 assert(HandCorrectionStore::Load(song).empty());
 assert(HandCorrectionStore::LoadFingers(song).size()==1);
 // Legacy v1 files are still supported without losing earlier corrections.
 assert(again.SetCorrection(0,60,0,HandPractice::Left));
 assert(HandCorrectionStore::Save(song,again.Corrections()));
 const auto legacy=HandCorrectionStore::LoadAll(song);
 assert(legacy.hands.size()==1 && legacy.fingers.empty());
 std::ifstream original(song,std::ios::binary);
 std::string signature;original>>signature;
 assert(signature=="MThd"); // No changes to the original MIDI
 std::cout<<"OK: A-B inclusive/exclusive, undo, finger 1..5, versioned persistence and unchanged MIDI\n";
}
