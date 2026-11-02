// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_PIANO_SCORE_TRACKS_H
#define LINTHESIA_PIANO_SCORE_TRACKS_H
#include <cstddef>
#include <string>
#include <vector>
#include <set>
namespace PianoScoreTracks {
struct Candidate {
 size_t id=0;
 bool visible=true,userPlayable=false,isPiano=false,isPercussion=false;
 size_t noteCount=0;
};
// Respect deliberate track selection; otherwise prefer genuine piano
// instruments over a complete orchestral arrangement. Nothing is muted.
inline std::set<size_t> Choose(const std::vector<Candidate>& tracks){
 std::set<size_t> chosen;
 std::vector<Candidate> playable;
 for(const auto& t:tracks){
  if(!t.visible||t.isPercussion||!t.noteCount)continue;
  if(t.userPlayable)playable.push_back(t);
 }
 const auto& pool=playable.empty()?tracks:playable;
 size_t preferred=size_t(-1),largest=0;
 for(const auto& t:pool)if(t.visible&&!t.isPercussion&&
                              t.isPiano&&t.noteCount>largest){
  preferred=t.id;largest=t.noteCount;
 }
 if(preferred!=size_t(-1)){
  chosen.insert(preferred);
 }else if(!playable.empty()){
  for(const auto& t:playable)chosen.insert(t.id);
 }else{
  largest=0;
  for(const auto& t:tracks)if(t.visible&&!t.isPercussion&&
                            t.noteCount>largest){
   preferred=t.id;largest=t.noteCount;
  }
  if(preferred!=size_t(-1))chosen.insert(preferred);
 }
 return chosen;
}
}
#endif
