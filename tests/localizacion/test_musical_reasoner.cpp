#include "MusicalReasoner.h"
#include "MusicalReviewStore.h"
#include "StaffNotation.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
using namespace MusicalReasoner;
static TranslatedNote note(int p,long long onset,int vel=90,long long dur=100000){
  TranslatedNote n{};
  n.note_id=p;n.track_id=0;n.channel=0;n.velocity=vel;
  n.start=onset;n.end=onset+dur;
  return n;
}
int main(){
  assert(InKey(60,0));assert(!InKey(61,0));
  std::vector<bool> chosen={true};
  TranslatedNoteSet octave;
  octave.insert(note(60,0));octave.insert(note(72,500000));
  octave.insert(note(60,1000000));
  auto results=Analyze(octave,chosen,0);
  assert(results.size()==1);
  assert(results.front().original==72 && results.front().replacement==60);
  assert(results.front().confidence>=78);
  TranslatedNoteSet chromatic;
  chromatic.insert(note(60,0));chromatic.insert(note(61,500000,60));
  chromatic.insert(note(60,1000000));
  auto semitone=Analyze(chromatic,chosen,0);
  assert(semitone.size()==1 && semitone[0].replacement==60);
  TranslatedNoteSet ghost;
  ghost.insert(note(60,0));ghost.insert(note(84,500000,15,20000));
  ghost.insert(note(60,1000000));
  auto phantom=Analyze(ghost,chosen,0);
  assert(phantom.size()==1 && phantom[0].replacement==-1);
  // Normal passing notes and intentional chromaticism must survive review.
  TranslatedNoteSet passage;
  passage.insert(note(64,0));passage.insert(note(66,500000));
  passage.insert(note(67,1000000));
  assert(Analyze(passage,chosen,0).empty());
  chosen[0]=false;
  assert(Analyze(octave,chosen,0).empty());
  // In C major F# is a deliberate sharp, not a reason to shift its pitch.
  const auto fsharp=StaffNotation::Spell(66,HandPractice::Right,0);
  assert(fsharp.symbol=='#');
  const auto fnatural=StaffNotation::Spell(65,HandPractice::Right,1);
  assert(fnatural.symbol=='n');
  const char* tmp=getenv("TEST_SONG");
  if(tmp && *tmp){
    std::FILE* fd=std::fopen(tmp,"wb");assert(fd);std::fputs("MIDI",fd);std::fclose(fd);
    assert(MusicalReviewStore::Save(tmp,results));
    const auto saved=MusicalReviewStore::Load(tmp);
    assert(saved.size()==1 && saved.front().replacement==60);
    std::remove(tmp);
  }
  std::cout << "OK: contextual corrections, tonal spelling, optional persistence, no false chromatic fix\n";
}
