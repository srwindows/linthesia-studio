// Linthesia GPL-2.0-or-later: deterministic one-shot top-origin effect.
#ifndef LINTHESIA_TOP_BIRTH_EMITTER_H
#define LINTHESIA_TOP_BIRTH_EMITTER_H
#include "NoteEmitter.h"
namespace TopBirthEmitter {
struct Particle {float x=0,y=0,size=0,alpha=0;};
constexpr float Duration=1.05f;
inline float Reveal(float scaledAge){
  if(!std::isfinite(scaledAge) || scaledAge<0)return 1.0f;
  // Explosion, particles and the first pixels of the note all begin NOW.
  return NoteEmitter::Smooth(0.0f,.48f,scaledAge);
}

// Positive y moves DOWN from the upper end of a falling note.
// Every particle bursts outward, reverses gently and falls in a zigzag.
inline Particle Sample(float age,unsigned index,unsigned seed,float keyWidth,
                       float dispersion=1.0f,float scale=1.0f,
                       unsigned count=36){
  Particle p;
  if(!std::isfinite(age)||age<0||age>Duration||
     !std::isfinite(keyWidth)||keyWidth<=0||count==0||index>=count)return p;
  dispersion=std::max(0.0f,std::min(2.0f,dispersion));
  scale=std::max(.25f,std::min(2.2f,scale));
  const unsigned n=index*41u;
  // Launch particles together with the explosion and forming note.
  const float delay=.005f+.042f*(index+.25f*NoteEmitter::Random(seed,n+1u))/count;
  const float t=age-delay;
  if(t<=0)return p;
  const float lifetime=.52f+.43f*NoteEmitter::Random(seed,n+2u);
  if(t>=lifetime)return p;
  const float u=t/lifetime;
  const float onset=NoteEmitter::Smooth(0.0f,.055f,t);
  const float fade=1.0f-NoteEmitter::Smooth(.30f,1.0f,u);
  const float theta=6.283185307f*NoteEmitter::Random(seed,n+3u);
  const float initialSide=std::cos(theta)*(.4f+.6f*NoteEmitter::Random(seed,n+4u));
  const float burst=keyWidth*(.33f+.22f*NoteEmitter::Random(seed,n+5u));
  const float wiggle=dispersion*(1.6f+3.5f*NoteEmitter::Random(seed,n+6u));
  p.x=(initialSide*burst*NoteEmitter::Smooth(0,.21f,u)+
       std::sin(u*11.0f+theta)*wiggle*NoteEmitter::Smooth(0,.15f,u))*dispersion;
  p.y=2.0f+(11.0f+12.0f*NoteEmitter::Random(seed,n+7u))*u+
       (32.0f+39.0f*NoteEmitter::Random(seed,n+8u))*u*u;
  p.size=(.8f+1.75f*NoteEmitter::Random(seed,n+9u))*(1.0f-.55f*u)*scale;
  p.alpha=(.35f+.60f*NoteEmitter::Random(seed,n+10u))*onset*fade;
  return p;
}
} // namespace TopBirthEmitter
#endif
