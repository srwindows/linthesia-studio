// SPDX-License-Identifier: GPL-2.0-or-later
// Non-destructive piano hand estimation for MIDI 0/1. Not an audio/MIDI edit.
#ifndef LINTHESIA_HAND_PRACTICE_H
#define LINTHESIA_HAND_PRACTICE_H
#include <cstddef>
#include "libmidi/Note.h"
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>
#include <tuple>
namespace HandPractice {
enum Hand { Left=0, Right=1 };
enum Practice { Both=0, RightOnly=1, LeftOnly=2 };
inline const char* PracticeName(int m){
 return m==RightOnly?"Derecha":(m==LeftOnly?"Izquierda":"Ambas");
}
// C major example: C4=60, C5=72. Median/track analysis handles single
// melody tracks and mixed piano tracks without altering the original notes.
struct Correction {
  size_t track=0;
  int pitch=60;
  microseconds_t when=0;
  Hand hand=Right;
};
struct Fingering {
  size_t track=0;
  int pitch=60;
  microseconds_t when=0;
  int finger=1; // 1 pulgar ... 5 meñique, independiente por mano
};
class Classifier {
  std::map<std::tuple<size_t,int,microseconds_t>,Hand> manual_;
  std::map<std::tuple<size_t,int,microseconds_t>,int> fingers_;
  std::map<std::pair<size_t,int>,std::vector<std::pair<microseconds_t,Hand>>> events_;
  std::map<size_t,int> threshold_;
  std::map<size_t,Hand> monophonic_;
public:
  void Build(const TranslatedNoteSet &notes,int split=60){
    events_.clear(); threshold_.clear();monophonic_.clear();
    manual_.clear();fingers_.clear();
    split=std::max(48,std::min(84,split));
    std::map<size_t,std::vector<int>> pitches;
    for(const auto& n:notes)
      if(n.note_id<128 && n.channel!=9)
        pitches[n.track_id].push_back(int(n.note_id));
    for(auto& part:pitches){
      auto &v=part.second;
      std::sort(v.begin(),v.end());
      const int median=v[v.size()/2],range=v.back()-v.front();
      // Independent voices: narrow low/high range is a single hand.
      if(range<13 && median<=55)monophonic_[part.first]=Left;
      else if(range<13 && median>=65)monophonic_[part.first]=Right;
      threshold_[part.first]=split;
    }
    // When a MIDI already provides two distinct melodic tracks, treat
    // those tracks as hands before trying the pitch split within a track.
    // E.g. a RH voice containing C4 must not get mislabelled left merely
    // because its own median happens to be D4.
    if(pitches.size()==2){
      auto lo=pitches.begin(),hi=std::next(lo);
      const int medianLo=lo->second[lo->second.size()/2];
      const int medianHi=hi->second[hi->second.size()/2];
      // A single mixed piano staff can span both registers even when
      // another melodic track exists. Only force a whole-track hand for
      // two sufficiently narrow, genuinely separated parts.
      const int spanLo=lo->second.back()-lo->second.front();
      const int spanHi=hi->second.back()-hi->second.front();
      if(std::abs(medianLo-medianHi)>=7 &&
         spanLo<17 && spanHi<17){
        monophonic_[lo->first]=medianLo<medianHi?Left:Right;
        monophonic_[hi->first]=medianLo<medianHi?Right:Left;
      }
    }
    for(const auto& n:notes){
      if(n.note_id>=128 || n.channel==9)continue; // GM drums not piano
      const auto mono=monophonic_.find(n.track_id);
      const int cut=threshold_[n.track_id];
      const Hand hand=mono!=monophonic_.end()?mono->second:
            (int(n.note_id)<cut?Left:Right);
      events_[{n.track_id,int(n.note_id)}].push_back({n.start,hand});
    }
    for(auto &entry:events_)
      std::sort(entry.second.begin(),entry.second.end());
  }
  bool SetCorrection(size_t track,int pitch,microseconds_t when,Hand hand){
    if(pitch<0 || pitch>127)return false;
    const auto it=events_.find({track,pitch});
    if(it==events_.end())return false;
    for(const auto &entry:it->second)
      if(entry.first==when){
        manual_[std::make_tuple(track,pitch,when)]=hand;
        return true;
      }
    return false;
  }
  // Apply a chosen hand over [start,end) across piano notes (not drums).
  // Existing annotations outside the selected time interval remain unchanged.
  size_t AssignRange(microseconds_t start,microseconds_t end,Hand hand){
    if(start>=end)return 0;
    size_t count=0;
    for(const auto &group:events_)
      for(const auto &note:group.second)
        if(note.first>=start && note.first<end){
          manual_[std::make_tuple(group.first.first,group.first.second,
                                  note.first)]=hand;
          ++count;
        }
    return count;
  }
  void RestoreCorrections(const std::vector<Correction>& entries){
    manual_.clear();
    for(const auto& c:entries)SetCorrection(c.track,c.pitch,c.when,c.hand);
  }
  bool SetFinger(size_t track,int pitch,microseconds_t when,int finger){
    if(finger<1||finger>5||pitch<0||pitch>127)return false;
    const auto it=events_.find({track,pitch});
    if(it==events_.end())return false;
    for(const auto& event:it->second)
      if(event.first==when){
        fingers_[std::make_tuple(track,pitch,when)]=finger;
        return true;
      }
    return false;
  }
  bool ClearFinger(size_t track,int pitch,microseconds_t when){
    return fingers_.erase(std::make_tuple(track,pitch,when))!=0;
  }
  int Finger(size_t track,int pitch,microseconds_t when) const {
    const auto it=fingers_.find(std::make_tuple(track,pitch,when));
    return it==fingers_.end()?0:it->second;
  }
  std::vector<Fingering> Fingerings() const{
    std::vector<Fingering> result;
    for(const auto& entry:fingers_)
      result.push_back({std::get<0>(entry.first),std::get<1>(entry.first),
                        std::get<2>(entry.first),entry.second});
    return result;
  }
  bool IsCorrected(size_t track,int pitch,microseconds_t when) const{
    return manual_.count(std::make_tuple(track,pitch,when))!=0;
  }
  bool ClearCorrection(size_t track,int pitch,microseconds_t when){
    return manual_.erase(std::make_tuple(track,pitch,when))!=0;
  }
  std::vector<Correction> Corrections() const{
    std::vector<Correction> result;
    for(const auto &e:manual_)
      result.push_back({std::get<0>(e.first),std::get<1>(e.first),
                        std::get<2>(e.first),e.second});
    return result;
  }
  Hand ForNote(const TranslatedNote &note) const {
    return ForEvent(note.track_id,int(note.note_id),note.start);
  }
  Hand ForEvent(size_t track,int pitch,microseconds_t when) const {
    const auto itManual=manual_.find(std::make_tuple(track,pitch,when));
    if(itManual!=manual_.end())return itManual->second;
    const auto it=events_.find({track,pitch});
    if(it!=events_.end()&&!it->second.empty()){
      const auto& seq=it->second;
      auto above=std::lower_bound(seq.begin(),seq.end(),when,
          [](const std::pair<microseconds_t,Hand>& e,microseconds_t t){
            return e.first<t;
          });
      if(above==seq.end()){
        auto override=manual_.find(std::make_tuple(track,pitch,seq.back().first));
        return override==manual_.end()?seq.back().second:override->second;
      }
      const auto closest=above==seq.begin()?above:
          ((when-(above-1)->first<=above->first-when)?above-1:above);
      auto override=manual_.find(std::make_tuple(track,pitch,closest->first));
      return override==manual_.end()?closest->second:override->second;
    }
    const auto mono=monophonic_.find(track);
    if(mono!=monophonic_.end())return mono->second;
    const auto cut=threshold_.find(track);
    return pitch<(cut!=threshold_.end()?cut->second:60)?Left:Right;
  }
  bool Selected(const TranslatedNote &n,int practiceMode) const {
    return practiceMode==Both ||
      (n.channel!=9 &&
       ((practiceMode==RightOnly && ForNote(n)==Right) ||
        (practiceMode==LeftOnly && ForNote(n)==Left)));
  }
  bool SelectedEvent(size_t track,int pitch,microseconds_t when,int mode) const {
    return mode==Both || (mode==RightOnly)==(ForEvent(track,pitch,when)==Right);
  }
};
inline int StepFromC(int midi){
 static constexpr int letters[12]={0,0,1,1,2,3,3,4,4,5,5,6};
 return (midi/12-1)*7+letters[midi%12];
}
// Line positions: treble bottom E4 and bass bottom G2.
inline int DiatonicOffset(int pitch,Hand hand){
 return StepFromC(pitch)-(hand==Right?StepFromC(64):StepFromC(43));
}
inline microseconds_t VisualQuantize(microseconds_t start,int bpm,int subdivision){
 if(subdivision==0 || bpm<=0)return start;
 const microseconds_t beat=60000000LL/bpm;
 const microseconds_t step=std::max<microseconds_t>(1000,beat/(subdivision==1?2:4));
 // Quantization only affects the display, never note events or evaluation.
 return (start>=0?(start+step/2)/step:(start-step/2)/step)*step;
}
}
#endif
