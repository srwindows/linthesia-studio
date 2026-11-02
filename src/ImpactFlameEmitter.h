// SPDX-License-Identifier: GPL-2.0-or-later
// Flame at a piano key: ONE short-lived upward burst per MIDI NoteOn.
// Deterministic from onset elapsed time and note seed; no frame-dependent
// allocation, no long-lived column and no jump when FPS drops.
#ifndef LINTHESIA_IMPACT_FLAME_EMITTER_H
#define LINTHESIA_IMPACT_FLAME_EMITTER_H
#include "NoteEmitter.h"
#include <algorithm>
#include <cmath>
namespace ImpactFlameEmitter {
constexpr unsigned Count=44;
constexpr float Duration=0.31f;
struct Spark {
 float x=0,y=0,radius=0,alpha=0,core=0;
};
inline Spark Sample(float age,unsigned i,unsigned seed,float width,
                    float spread=1.f,float particleSize=1.f){
 Spark p;
 if(!std::isfinite(age)||!std::isfinite(width)||age<0.f||
    i>=Count||width<=0.f||age>Duration)return p;
 spread=std::max(.1f,std::min(2.0f,spread));
 particleSize=std::max(.25f,std::min(2.f,particleSize));
 const unsigned salt=i*59u;
 // A narrow, staggered launch; 135-225 ms visible travel per spark.
 const float delay=.065f*NoteEmitter::Random(seed,salt+3u);
 const float life=.135f+.075f*NoteEmitter::Random(seed,salt+5u);
 const float t=(age-delay)/life;
 if(t<=0.f||t>=1.f)return p;
 const float curved=t*t*(3.f-2.f*t);
 const float side=2.f*NoteEmitter::Random(seed,salt+7u)-1.f;
 const float phase=6.2831853f*NoteEmitter::Random(seed,salt+11u);
 const float rise=25.f+48.f*NoteEmitter::Random(seed,salt+13u);
 const float curl=std::sin(phase+6.8f*t)+.24f*std::sin(phase*2.f+12.f*t);
 p.x=side*(width*.17f+2.4f*curved)*spread+
     (1.3f+3.3f*curved)*curl*spread;
 p.y=-3.f-rise*curved; // always upward
 // Smooth birth and fast fade; none remains as a stationary white blob.
 const float fadeIn=NoteEmitter::Smooth(0.f,.10f,t);
 const float fadeOut=1.f-NoteEmitter::Smooth(.34f,1.f,t);
 p.alpha=(.38f+.54f*NoteEmitter::Random(seed,salt+17u))*fadeIn*fadeOut;
 p.radius=(.55f+1.25f*NoteEmitter::Random(seed,salt+19u))*
          (.94f-.60f*t)*particleSize;
 p.core=NoteEmitter::Random(seed,salt+23u);
 return p;
}
}
#endif
