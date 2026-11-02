// SPDX-License-Identifier: GPL-2.0-or-later
#include "HandPractice.h"
#include "HandCorrectionStore.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
int main(){
 char dir[]="/tmp/linthesia-hand-corrections-XXXXXX";
 assert(mkdtemp(dir));
 setenv("XDG_CONFIG_HOME",dir,1);
 const std::string song=std::string(dir)+"/duet.mid";
 {std::ofstream file(song);file<<"MThd";} // identity of this test song
 TranslatedNoteSet notes;
 for(int k=0;k<12;++k){
  TranslatedNote l{},r{};
  l.note_id=48+k%4;l.start=k*1000000;l.end=l.start+400000;
  l.track_id=0;l.channel=0;l.state=AutoPlayed;l.retry_state=AutoPlayed;
  r=l;r.note_id=60+k%4;
  notes.insert(l);notes.insert(r);
 }
 HandPractice::Classifier c;c.Build(notes,60);
 assert(c.ForEvent(0,60,0)==HandPractice::Right);
 assert(c.ForEvent(0,60,100000)==HandPractice::Right);
 assert(c.SetCorrection(0,60,0,HandPractice::Left));
 assert(c.ForEvent(0,60,0)==HandPractice::Left);
 assert(c.ForEvent(0,60,100000)==HandPractice::Left);
 assert(c.IsCorrected(0,60,0));
 assert(c.Corrections().size()==1);
 assert(HandCorrectionStore::Save(song,c.Corrections()));
 auto stored=HandCorrectionStore::Load(song);
 assert(stored.size()==1);
 HandPractice::Classifier reloaded;reloaded.Build(notes,60);
 for(const auto& correction:stored)
  assert(reloaded.SetCorrection(correction.track,correction.pitch,
         correction.when,correction.hand));
 assert(reloaded.ForEvent(0,60,0)==HandPractice::Left);
 assert(reloaded.ClearCorrection(0,60,0));
 assert(reloaded.ForEvent(0,60,0)==HandPractice::Right);
 // The automatic split can be tuned to accommodate overlapping hands.
 reloaded.Build(notes,63);
 assert(reloaded.ForEvent(0,60,0)==HandPractice::Left);
 // Invalid annotations can't be applied to a note that doesn't exist.
 assert(!reloaded.SetCorrection(40,127,0,HandPractice::Left));
 // The corrections file is outside the MIDI and hasn't modified it.
 std::ifstream infile(song);std::string check;infile>>check;
 assert(check=="MThd");
 std::cout<<"OK: manual hand corrections are per-song, persisted, reversible and non-destructive\n";
}
