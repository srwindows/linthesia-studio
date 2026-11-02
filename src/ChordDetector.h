// Linthesia — GPL v2 (COPYING). Preserva autoría del proyecto original.
// Analizador determinista: coincidencia exacta por clases de altura MIDI.
// No adivina acordes con menos de tres notas distintas.
#ifndef LINTHESIA_CHORD_DETECTOR_H
#define LINTHESIA_CHORD_DETECTOR_H

#include <algorithm>
#include <set>
#include <string>
#include <vector>
namespace ChordDetector {
struct Pattern {
  const char* suffix;
  std::vector<int> intervals;
};
inline std::string Name(int pc) {
  static const char* names[]={"C","C#","D","Eb","E","F",
                               "F#","G","Ab","A","Bb","B"};
  return names[(pc%12+12)%12];
}
inline std::string Detect(const std::vector<int>& midiNotes) {
  if(midiNotes.empty())return "—";
  std::set<int> pcs;
  int bass=127;
  for(int n:midiNotes){
    if(n<0||n>127)continue;
    bass=std::min(bass,n);
    pcs.insert(n%12);
  }
  if(pcs.size()<3 || pcs.size()>6)return "—";
  const Pattern patterns[]={
    {"", {0,4,7}}, {"m",{0,3,7}},
    {"dim",{0,3,6}}, {"+",{0,4,8}},
    {"sus2",{0,2,7}}, {"sus4",{0,5,7}},
    {"6",{0,4,7,9}}, {"m6",{0,3,7,9}},
    {"7",{0,4,7,10}}, {"maj7",{0,4,7,11}},
    {"m7",{0,3,7,10}}, {"m(maj7)",{0,3,7,11}},
    {"m7b5",{0,3,6,10}}, {"dim7",{0,3,6,9}},
    {"7sus4",{0,5,7,10}}, {"add9",{0,2,4,7}},
    {"madd9",{0,2,3,7}}, {"9",{0,2,4,7,10}},
    {"maj9",{0,2,4,7,11}}, {"m9",{0,2,3,7,10}},
    {"7b9",{0,1,4,7,10}}, {"7#9",{0,3,4,7,10}},
    {"11",{0,2,4,5,7,10}}, {"m11",{0,2,3,5,7,10}}
  };
  // Resolver raíces armónicas, no confundir inversión con raíz.
  // Priorizar la raíz en el bajo para acordes ambiguos.
  std::string best="—";
  int bestScore=-10000;
  const int bassPc=bass%12;
  for(int root=0;root<12;++root){
    for(const Pattern& pattern:patterns){
      if(pattern.intervals.size()!=pcs.size())continue;
      bool match=true;
      for(int interval:pattern.intervals)
        if(pcs.count((root+interval)%12)==0){match=false;break;}
      if(!match)continue;
      const int score=(root==bassPc?40:0)
          +static_cast<int>(pattern.intervals.size())*2;
      if(score<=bestScore)continue;
      bestScore=score;
      best=Name(root)+pattern.suffix;
      if(root!=bassPc)best+="/"+Name(bassPc);
    }
  }
  return best;
}
} // namespace ChordDetector
#endif
