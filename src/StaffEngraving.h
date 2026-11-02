// SPDX-License-Identifier: GPL-2.0-or-later
// Deterministic display-only engraving. Works from actual MIDI event and
// measure times without modifying note-on, note-off, playback or MIDI files.
#ifndef LINTHESIA_STAFF_ENGRAVING_H
#define LINTHESIA_STAFF_ENGRAVING_H
#include "StaffNotation.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
namespace StaffEngraving {
using Time=std::int64_t;
struct Note {
  int pitch=60;
  int hand=0; // 0 treble / right, 1 bass / left
  Time start=0,end=0;
  size_t source=0; // index in caller's immutable source vector
};
struct Bar {
  Time start=0;
  int number=1;
};
struct Glyph {
  Note note;
  int voice=0; // 0 upper (up-stem), 1 lower (down-stem)
  Time start=0,end=0;
  int bar=1;
  bool tieIn=false,tieOut=false;
  bool beamPrev=false,beamNext=false;
  int tuplet=0;
};
struct Rest {
  int hand=0,voice=0,bar=1;
  Time start=0,end=0;
  StaffNotation::NoteValue value=StaffNotation::NoteValue::Quarter;
};
struct Layout {
  std::vector<Glyph> notes;
  std::vector<Rest> rests;
};
inline std::vector<Bar> NormalizeBars(std::vector<Bar> bars) {
  if(bars.empty()||bars.front().start>0)bars.push_back({0,1});
  std::sort(bars.begin(),bars.end(),[](const Bar&a,const Bar&b){
    return a.start<b.start;
  });
  bars.erase(std::unique(bars.begin(),bars.end(),[](const Bar&a,const Bar&b){
    return a.start==b.start;
  }),bars.end());
  return bars;
}
inline size_t AtBar(const std::vector<Bar>& bars,Time t) {
  auto it=std::upper_bound(bars.begin(),bars.end(),t,[](Time time,const Bar& b){
    return time<b.start;
  });
  return it==bars.begin()?0:static_cast<size_t>((it-bars.begin())-1);
}
inline Layout Build(const std::vector<Note>& input,std::vector<Bar> bars,
                    Time viewportStart,Time viewportEnd,int bpm) {
  Layout result;
  if(viewportEnd<=viewportStart||input.empty()||bpm<=0)return result;
  bars=NormalizeBars(std::move(bars));
  std::vector<Note> ordered;
  ordered.reserve(input.size());
  for(const auto &n:input)
    if(n.hand>=0&&n.hand<=1&&n.pitch>=0&&n.pitch<=127&&n.end>n.start)
      ordered.push_back(n);
  std::stable_sort(ordered.begin(),ordered.end(),[](const Note&a,const Note&b){
    if(a.start!=b.start)return a.start<b.start;
    if(a.hand!=b.hand)return a.hand<b.hand;
    if(a.end!=b.end)return a.end>b.end; // long voices first
    return a.pitch<b.pitch;
  });
  // Running voice allocation with chords (same start/end) sharing a stem.
  Time busy[2][2]={{0,0},{0,0}};
  Time lastStart[2][2]={{std::numeric_limits<Time>::min(),
                        std::numeric_limits<Time>::min()},
                       {std::numeric_limits<Time>::min(),
                        std::numeric_limits<Time>::min()}};
  Time lastEnd[2][2]={{0,0},{0,0}};
  int lastPitch[2][2]={{0,0},{0,0}};
  const Time quarter=60000000LL/bpm;
  for(const auto &n:ordered){
    int voice=0;
    const int h=n.hand;
    if(n.start==lastStart[h][0]&&n.end==lastEnd[h][0])voice=0;
    else if(n.start==lastStart[h][1]&&n.end==lastEnd[h][1])voice=1;
    else if(busy[h][0]<=n.start)voice=0;
    else if(busy[h][1]<=n.start)voice=1;
    else {
      // Both voices are sounding: favour the nearest pitch/register and
      // retain the old performance note independently of notation.
      voice=std::abs(n.pitch-lastPitch[h][0])<=
            std::abs(n.pitch-lastPitch[h][1])?0:1;
    }
    busy[h][voice]=std::max(busy[h][voice],n.end);
    lastStart[h][voice]=n.start;
    lastEnd[h][voice]=n.end;
    lastPitch[h][voice]=n.pitch;
    Time cursor=n.start;
    for(int segment=0;segment<128 && cursor<n.end;++segment){
      const size_t barIndex=AtBar(bars,cursor);
      const Time nextBar=barIndex+1<bars.size()?
                       bars[barIndex+1].start:n.end;
      const Time until=std::min(n.end,nextBar);
      if(until<=cursor)break;
      if(until>viewportStart && cursor<viewportEnd){
        Glyph g{};
        g.note=n;g.start=cursor;g.end=until;
        g.voice=voice;g.bar=bars[barIndex].number;
        g.tieIn=cursor>n.start;
        g.tieOut=until<n.end;
        g.tuplet=StaffNotation::RhythmFromDuration(until-cursor,bpm).tuplet;
        result.notes.push_back(g);
      }
      cursor=until;
    }
  }
  std::stable_sort(result.notes.begin(),result.notes.end(),[](const Glyph&a,const Glyph&b){
    if(a.start!=b.start)return a.start<b.start;
    if(a.note.hand!=b.note.hand)return a.note.hand<b.note.hand;
    return a.voice<b.voice;
  });
  // Beams only join contiguous notes in the same voice and measure,
  // without ties, and never force two independent voices into one beam.
  for(size_t i=0;i<result.notes.size();++i) {
    auto &a=result.notes[i];
    if(a.tieIn||a.tieOut)continue;
    const auto kindA=StaffNotation::RhythmFromDuration(a.end-a.start,bpm);
    if(StaffNotation::Flags(kindA.value)==0)continue;
    const size_t barIndex=AtBar(bars,a.start);
    const Time groupWindow=kindA.tuplet?
                      (kindA.value==StaffNotation::NoteValue::Eighth?quarter:
                       quarter/2) :
                     (StaffNotation::Flags(kindA.value)==2?quarter:2*quarter);
    for(size_t j=i+1;j<result.notes.size();++j){
      auto &b=result.notes[j];
      if(b.start>a.end+quarter/16)break;
      if(a.note.hand!=b.note.hand||a.voice!=b.voice||a.bar!=b.bar ||
         b.tieIn||b.tieOut||b.start<=a.start)continue;
      const auto kindB=StaffNotation::RhythmFromDuration(b.end-b.start,bpm);
      if(kindB.value!=kindA.value||kindB.tuplet!=kindA.tuplet)
        continue;
      if(b.start-a.end>quarter/16)continue;
      if((b.start-bars[barIndex].start)/groupWindow!=
         (a.start-bars[barIndex].start)/groupWindow)continue;
      if(b.beamPrev)continue;
      a.beamNext=true;b.beamPrev=true;
      break;
    }
  }
  // A displayed rest is valid only BETWEEN two notes belonging to the
  // same independent voice and bar. No imaginary lead-in, after-song or
  // chord rests are manufactured, and overlapping notes create no rests.
  for(int h=0;h<2;++h)for(int v=0;v<2;++v){
    bool previous=false;
    Time prevEnd=0;
    int prevBar=-1;
    for(const auto& g:result.notes){
      if(g.note.hand!=h||g.voice!=v)continue;
      if(previous && g.bar==prevBar && g.start>prevEnd+quarter/8){
        const Time gap=g.start-prevEnd;
        const auto val=StaffNotation::ValueFromDuration(gap,bpm);
        if(gap>=quarter/4 && result.rests.size()<80)
          result.rests.push_back({h,v,g.bar,prevEnd,g.start,val});
      }
      if(!previous||g.start>=prevEnd){
        prevEnd=g.end;prevBar=g.bar;
      }else prevEnd=std::max(prevEnd,g.end);
      previous=true;
    }
  }
  return result;
}
}
#endif
