// Deterministic visual emitter: no MIDI or rendering state is mutated.
#ifndef LINTHESIA_NOTE_EMITTER_H
#define LINTHESIA_NOTE_EMITTER_H

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace NoteEmitter {
constexpr unsigned Count=144;

inline float Smooth(float lo,float hi,float x){
  if(hi<=lo)return x<lo?0.0f:1.0f;
  const float t=std::max(0.0f,std::min(1.0f,(x-lo)/(hi-lo)));
  return t*t*(3.0f-2.0f*t);
}

struct Particle {
  float x=0.0f;
  float y=0.0f;
  float size=0.0f;
  float alpha=0.0f;
};

inline float Random(unsigned seed,unsigned n){
  std::uint32_t x=std::uint32_t(seed)^(std::uint32_t(n)*0x9e3779b9u);
  x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;
  return float(x&65535u)/65535.0f;
}

// Each slot keeps its lifetime and path through attack and sustain.
// Modulo recycling happens only at zero opacity, including zero derivative.
inline Particle Sample(float elapsed,unsigned index,unsigned seed,float keyWidth,
                       float dispersion=1.0f,float particleSize=1.0f,
                       unsigned activeCount=Count){
  Particle p;
  if(!std::isfinite(elapsed) || !std::isfinite(keyWidth) ||
     elapsed<0.0f || keyWidth<=0.0f || index>=activeCount ||
     activeCount==0 || activeCount>Count)return p;
  dispersion=std::max(0.0f,std::min(2.0f,dispersion));
  particleSize=std::max(.25f,std::min(2.2f,particleSize));
  const unsigned n=index*43u;
  const float delay=.62f*(float(index)+.5f*Random(seed,n+1u))/float(activeCount);
  // Partículas de desplazamiento pausado: mayor tiempo de vida, zigzag
  // visible sin sacudidas y sin afectar jamás al reloj MIDI.
  const float life=1.30f+.82f*Random(seed,n+2u);
  const float period=life+.16f+.24f*Random(seed,n+3u);
  const double local=double(elapsed)-double(delay);
  if(local<0.0)return p;
  const double generation=std::floor(local/double(period));
  const float age=float(local-generation*double(period));
  if(age>=life)return p;
  const float t=age/life;
  const float birth=Smooth(0.0f,.18f,age);
  const float death=1.0f-Smooth(.49f*life,life,age);
  const double born=double(delay)+generation*double(period);
  // The attack changes emission brightness, never the flight of live particles.
  const float attack=1.0f+.16f*float(std::exp(-born/.22));
  const float shimmer=.88f+.12f*std::sin(age*5.2f+
      6.28318530718f*Random(seed,n+15u));
  p.alpha=std::min(1.0f,(.43f+.53f*Random(seed,n+4u))*
      birth*death*attack*shimmer);
  const float side=Random(seed,n+5u)<.5f?-1.0f:1.0f;
  const float opening=(5.0f+17.0f*Random(seed,n+6u))*dispersion;
  const float phase=6.28318530718f*Random(seed,n+7u);
  const float drift=(1.2f+2.5f*Random(seed,n+8u))*dispersion;
  // Zigzag ascendente: cada partícula serpentea 2-3 veces durante su
  // recorrido, con amplitud, ritmo y fase independientes por semilla MIDI.
  // Se redondean las inversiones, sin quiebros geométricos ni órbitas.
  const float zigzagCycles=1.65f+.73f*Random(seed,n+12u);
  const float zigzagAmplitude=(5.3f+3.4f*Random(seed,n+13u))*dispersion;
  const float bendPhase=phase+6.28318530718f*zigzagCycles*t;
  const float zigzag=std::sin(bendPhase)+
      .13f*std::sin(bendPhase*2.0f+1.9f*Random(seed,n+14u));
  const float soften=Smooth(0.0f,.15f,t)*
      (1.0f-.22f*Smooth(.78f,1.0f,t));
  p.x=side*(keyWidth*.14f*dispersion+opening*Smooth(0.0f,.92f,t))+
      zigzagAmplitude*zigzag*soften+
      drift*(std::sin(phase+age*1.1f)-std::sin(phase));
  const float speed=43.0f+42.0f*Random(seed,n+9u);
  const float acceleration=1.0f+3.5f*Random(seed,n+10u);
  p.y=-4.0f-speed*age-.5f*acceleration*age*age;
  p.size=(.75f+1.68f*Random(seed,n+11u))*(1.0f-.38f*t)*particleSize;
  return p;
}
} // namespace NoteEmitter
#endif
