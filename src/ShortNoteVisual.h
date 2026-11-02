// Linthesia GPLv2+: visual-only note length, independent of MIDI duration.
#ifndef LINTHESIA_SHORT_NOTE_VISUAL_H
#define LINTHESIA_SHORT_NOTE_VISUAL_H
#include <algorithm>
#include <cmath>
namespace ShortNoteVisual {
struct Body {int top=0,height=0;};
// A 3D note is a compact moving object, not a bar proportional to sustain.
// A 100% length is approx. 3.4 white keys tall. Never alter NoteOn/NoteOff.
inline int Maximum(int whiteKeyWidth,int percent){
  const int scaled=static_cast<int>(std::lround(
      std::max(1,whiteKeyWidth)*3.4f*
      std::max(40,std::min(200,percent))/100.0f));
  return std::max(18,std::min(140,scaled));
}
// A note's front enters the piano at NoteOn. The illuminated body follows
// the key surface inward until completely absorbed. Audio sustain belongs
// to MIDI and is deliberately independent of this visual lifespan.
inline Body Absorb(int strike,int limit,float elapsedSeconds){
  if(limit<=0 || elapsedSeconds<0.f)return {strike,0};
  // Linear travel creates an observable sinking motion, not a fixed luminous
  // cap. No division by FPS or BPM: the elapsed MIDI clock owns this effect.
  const int original=std::max(3,limit);
  const int consumed=static_cast<int>(std::floor(elapsedSeconds*230.f));
  const int visible=std::max(0,original-consumed);
  return {strike-visible,visible};
}

inline Body Fit(int originalTop,int originalBottom,int limit){
  if(limit<=0||originalBottom<=originalTop)return {originalTop,0};
  // Incoming 3D notes keep their leading edge. After NoteOn use Absorb()
  // instead: a permanently held luminous head is not part of this renderer.
  const int length=std::max(3,std::min(limit,originalBottom-originalTop));
  return {originalBottom-length,length};
}
}
#endif
