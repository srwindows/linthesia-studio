// SPDX-License-Identifier: GPL-2.0-or-later
#include "StaffEngraving.h"
#include <cassert>
#include <iostream>
using namespace StaffEngraving;
int main() {
 const std::vector<Bar> bars={{0,1},{1500000,2},{3000000,3},{4500000,4}};
 // Sixteenth and eighth beaming are controlled by independent onset and
 // durations; no false beams on quarter notes or across a measure boundary.
 {
  const std::vector<StaffEngraving::Note> n={{60,0,0,125000,0},{62,0,125000,250000,1},
                             {64,0,250000,375000,2},{65,0,375000,500000,3}};
  auto result=Build(n,bars,0,1400000,120);
  assert(result.notes.size()==4);
  assert(result.notes[0].beamNext);
  assert(result.notes[1].beamPrev);
  assert(result.notes[1].beamNext);
  assert(result.notes[3].beamPrev);
  for(auto &g:result.notes)assert(g.voice==0 && g.bar==1);
  assert(result.rests.empty());
 }
 // Rest only inside a real monophonic gap: not before, not after.
 {
  const std::vector<StaffEngraving::Note> n={{60,0,0,500000,0},{62,0,1000000,1250000,1}};
  auto result=Build(n,bars,0,1400000,120);
  assert(result.rests.size()==1);
  assert(result.rests[0].start==500000 &&
         result.rests[0].end==1000000);
  assert(result.rests[0].value==StaffNotation::NoteValue::Quarter);
  assert(!result.notes[0].beamNext);
 }
 // A note held across the real MIDI barline is split visually with a tie,
 // but its source identity and original timing are preserved.
 {
  const std::vector<StaffEngraving::Note> n={{60,0,1000000,2100000,10}};
  auto result=Build(n,bars,900000,2400000,120);
  assert(result.notes.size()==2);
  assert(result.notes[0].start==1000000 &&
         result.notes[0].end==1500000 &&
         result.notes[0].tieOut&&!result.notes[0].tieIn);
  assert(result.notes[1].start==1500000 &&
         result.notes[1].end==2100000 &&
         result.notes[1].tieIn&&!result.notes[1].tieOut);
  assert(result.notes[0].note.source==10 &&
         result.notes[1].note.source==10);
  assert(result.notes[1].bar==2);
  assert(result.rests.empty());
 }
 // Chord of three notes with same start/end belongs to one voice;
 // an independent moving line overlapping it is the other voice.
 {
  const std::vector<StaffEngraving::Note> n={{48,1,0,1500000,0},
                             {55,1,0,1500000,1},
                             {60,1,0,1500000,2},
                             {67,1,500000,750000,3},
                             {69,1,750000,1000000,4}};
  auto l=Build(n,bars,0,1300000,120);
  for(const auto &g:l.notes)
    if(g.note.source<=2)assert(g.voice==0);
  for(const auto &g:l.notes)
    if(g.note.source>=3)assert(g.voice==1);
  assert(l.rests.empty());
  assert(l.notes.size()==5);
 }
 // Mid-measure silence in lower voice never forces a rest in upper voice.
 {
  const std::vector<StaffEngraving::Note> n={
    {60,0,0,1500000,0},{72,0,0,250000,1},{74,0,750000,1000000,2}};
  const auto l=Build(n,bars,0,1400000,120);
  assert(l.rests.size()==1);
  assert(l.rests[0].voice==1 && l.rests[0].hand==0);
  assert(l.rests[0].start==250000 && l.rests[0].end==750000);
 }
 // Three eighth triplets span exactly one quarter-note beat.
 {
  const std::vector<StaffEngraving::Note> n={
    {60,0,0,166667,0},{62,0,166667,333334,1},
    {64,0,333334,500001,2}};
  auto l=Build(n,bars,0,600000,120);
  assert(l.notes.size()==3);
  assert(l.notes[0].tuplet==3 && l.notes[1].tuplet==3);
  assert(l.notes[0].beamNext && l.notes[1].beamPrev &&
         l.notes[1].beamNext && l.notes[2].beamPrev);
 }
 // The end of a viewport must not generate a fictitious rest or prolong a
 // true MIDI NoteOff.
 {
  const std::vector<StaffEngraving::Note> n={{60,0,0,500000,0}};
  auto l=Build(n,bars,250000,600000,120);
  assert(l.notes.size()==1 && l.notes[0].end==500000);
  assert(l.rests.empty());
 }
 // Two quick notes split by the time signature's barline are never beamed.
 {
  const std::vector<StaffEngraving::Note> n={{64,0,1375000,1500000,0},
                             {65,0,1500000,1625000,1}};
  auto l=Build(n,bars,1000000,1900000,120);
  assert(l.notes.size()==2);
  assert(!l.notes[0].beamNext&&!l.notes[1].beamPrev);
 }
 // Whole-note render set and MIDI source remain immutable.
 {
  const std::vector<StaffEngraving::Note> empty={{60,0,0,250000,0}};
  auto l=Build(empty,bars,900000,1100000,120);
  assert(l.notes.empty()&&l.rests.empty());
  assert(empty[0].end==250000);
 }
 std::cout<<"OK: independent polyphony, chords, sixteenth beams, pauses, tied bar splits & MIDI immutability\n";
}
