// GPL-2.0-or-later. Contextual, conservative arranger assistance.
// No pretrained ML model: interpretable voice-leading/tonality/onset inference.
#ifndef LINTHESIA_MUSICAL_REASONER_H
#define LINTHESIA_MUSICAL_REASONER_H
#include <cstddef>
#include "libmidi/Note.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <string>
#include <vector>
namespace MusicalReasoner {
struct Suggestion {
  size_t track=0;
  int original=0, replacement=0; // -1: discard a likely spurious MIDI event
  microseconds_t when=0;
  int confidence=0;
  std::string reason;
};
inline bool InKey(int pitch,int fifths){
  const int tonic=((7*fifths)%12+12)%12;
  const int degree=((pitch-tonic)%12+12)%12;
  return degree==0||degree==2||degree==4||degree==5||
         degree==7||degree==9||degree==11;
}
inline std::vector<Suggestion> Analyze(const TranslatedNoteSet& all,
                  const std::vector<bool>& chosen,int fifths){
  std::map<size_t,std::vector<const TranslatedNote*>> voices;
  for(const auto& n:all)
    if(n.note_id<128&&n.channel!=9&&n.track_id<chosen.size()&&chosen[n.track_id])
      voices[n.track_id].push_back(&n);
  std::vector<Suggestion> out;
  for(auto& voice:voices){
    auto& ns=voice.second;
    std::sort(ns.begin(),ns.end(),[](const TranslatedNote* a,const TranslatedNote* b){
      if(a->start!=b->start)return a->start<b->start;
      return a->note_id<b->note_id;
    });
    for(size_t i=1;i+1<ns.size();++i){
      const auto& prev=*ns[i-1],&note=*ns[i],&next=*ns[i+1];
      // Only reason about a genuine melodic line. Polyphonic chords, repeated
      // onssets, ornaments and tied/overlapping voices are NOT errors.
      if(prev.start==note.start||next.start==note.start||
         prev.end>note.start+15000||note.end>next.start+15000)continue;
      if(next.start-note.start>1700000||note.start-prev.start>1700000)continue;
      const int old=int(note.note_id);
      const int l=int(prev.note_id),r=int(next.note_id);
      const int oldCost=std::abs(old-l)+std::abs(old-r);
      int best=old,bestGain=0;
      for(int shift:{-12,12,-1,1}){
        const int target=old+shift;
        if(target<21||target>108)continue;
        const int now=std::abs(target-l)+std::abs(target-r);
        const int gain=oldCost-now;
        if(gain>bestGain){bestGain=gain;best=target;}
      }
      // A displaced octave that leaps away and returns is strong evidence,
      // whereas semitone chromaticism is legitimate in most real music.
      if(note.end-note.start<55000 && note.velocity<=25 &&
         std::abs(old-l)>=7 && std::abs(old-r)>=7){
        out.push_back({voice.first,old,-1,note.start,79,
          "Nota muy breve y débil fuera de la línea melódica: posible fantasma"});
        continue;
      }
      if(std::abs(best-old)==12 && bestGain>=18 &&
         std::abs(l-r)<=7 && oldCost>=24){
        out.push_back({voice.first,old,best,note.start,
             std::min(98,78+bestGain/3),"Salto de octava aislado; la voz vuelve al mismo registro"});
        continue;
      }
      if(std::abs(best-old)==1 && bestGain>=2 &&
         !InKey(old,fifths)&&InKey(best,fifths) &&
         std::abs(l-r)<=2 && std::abs(old-l)>=1 &&
         note.velocity<75){
        out.push_back({voice.first,old,best,note.start,64,
          "Semitono fuera de tonalidad entre notas cercanas: revisar sostenido/bemol"});
        continue;
      }
    }
  }
  // Stable order: highest-confidence candidates first; each remains
  // opt-in and can be individually rejected by the musician.
  std::stable_sort(out.begin(),out.end(),[](const Suggestion& a,const Suggestion& b){
    return a.confidence>b.confidence;
  });
  if(out.size()>64)out.resize(64);
  return out;
}
}
#endif
