#include "NoteEmitter.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

static void require(bool ok,const char* message){
  if(!ok){std::cerr<<message<<"\n";std::exit(1);}
}
static float energy(float time,unsigned seed){
  float sum=0.0f;
  for(unsigned i=0;i<NoteEmitter::Count;++i)
    sum+=NoteEmitter::Sample(time,i,seed,24.0f).alpha;
  return sum;
}
static void checkFinite(const NoteEmitter::Particle& p){
  require(std::isfinite(p.x)&&std::isfinite(p.y)&&
          std::isfinite(p.size)&&std::isfinite(p.alpha),"non-finite particle");
  require(p.alpha>=0.0f&&p.alpha<=1.0f,"opacity outside normalized range");
  if(p.alpha>0.0f){
    require(std::abs(p.x)<50.0f&&p.y<=-4.0f&&p.y>-240.0f,
            "particle escaped the bounded upward plume");
    require(p.size>.20f&&p.size<=2.44f,"unexpected particle size");
  }
}
int main(){
  using NoteEmitter::Sample;
  const unsigned seeds[]={0u,1u,12345u,0xffffffffu};
  require(Sample(-.01f,0,1,20).alpha==0,"negative time emitted a particle");
  require(Sample(1,NoteEmitter::Count,1,20).alpha==0,"invalid slot emitted");
  require(Sample(1,0,1,-1).alpha==0,"negative key width emitted");
  require(Sample(std::numeric_limits<float>::infinity(),0,1,20).alpha==0,
          "infinite time emitted");
  require(Sample(std::numeric_limits<float>::quiet_NaN(),0,1,20).alpha==0,
          "NaN time emitted");
  require(energy(0.0f,1)==0.0f,"emitter must start transparently");

  for(unsigned seed:seeds){
    // No attack/sustain switch, even at the previous 1.35 second boundary.
    for(unsigned i=0;i<NoteEmitter::Count;++i){
      const auto a=Sample(1.35f-.0001f,i,seed,24);
      const auto b=Sample(1.35f+.0001f,i,seed,24);
      require(std::abs(a.alpha-b.alpha)<.006f,"opacity jumps at sustain");
      if(a.alpha>.001f&&b.alpha>.001f){
        require(std::abs(a.x-b.x)<.05f&&std::abs(a.y-b.y)<.05f,
                "trajectory jumps at sustain");
      }
    }
    // Every recycle crosses an invisible interval; visible motion always rises.
    for(unsigned i=0;i<NoteEmitter::Count;++i){
      auto previous=Sample(0,i,seed,24);
      for(int step=1;step<=24000;++step){
        const float t=step*.0005f;
        const auto current=Sample(t,i,seed,24);
        checkFinite(current);
        require(std::abs(current.alpha-previous.alpha)<.016f,
                "birth/death opacity is discontinuous");
        if(current.alpha>.0001f&&previous.alpha>.0001f){
          require(current.y<=previous.y+.0001f,"visible particle fell or teleported");
          require(previous.y-current.y<.09f,"vertical velocity jumped");
          require(std::abs(current.x-previous.x)<.105f,"lateral path jumped");
        }
        previous=current;
      }
    }
    // Comprobar zigzag verdadero: movimientos laterales en ambos sentidos
    // mientras las partículas mantienen el ascenso, sin saltos de lugar.
    unsigned vivid=0;
    for(unsigned i=0;i<NoteEmitter::Count;++i){
      int turns=0,lastDirection=0;
      float excursionLow=1e6f,excursionHigh=-1e6f;
      for(int sample=1;sample<125;++sample){
        const float now=sample*.012f;
        const auto a=Sample(now-.012f,i,seed,24.0f);
        const auto b=Sample(now,i,seed,24.0f);
        if(a.alpha<.055f||b.alpha<.055f)continue;
        const float dx=b.x-a.x;
        const int direction=dx>.075f?1:(dx<-.075f?-1:0);
        if(direction!=0){
          if(lastDirection && direction!=lastDirection)++turns;
          lastDirection=direction;
        }
        excursionLow=std::min(excursionLow,b.x);
        excursionHigh=std::max(excursionHigh,b.x);
      }
      if(turns>=2 && excursionHigh-excursionLow>=4.5f)++vivid;
    }
    require(vivid>=NoteEmitter::Count/2,
            "too few particles with visible left/right zigzag");
    // Long NoteOn cannot expire or acquire a different trajectory rule.
    for(float t:{2.0f,10.0f,60.0f,600.0f}){
      require(energy(t,seed)>2.0f,"sustained note lost its emitter");
      for(unsigned i=0;i<NoteEmitter::Count;++i)checkFinite(Sample(t,i,seed,24));
    }
  }
  // Sampling depends only on time, not previous calls or display frame rate.
  std::vector<NoteEmitter::Particle> expected;
  for(unsigned i=0;i<NoteEmitter::Count;++i)expected.push_back(Sample(60,i,421,24));
  for(int fps:{30,60,144}){
    for(int frame=0;frame<fps*3;++frame)(void)Sample(float(frame)/fps,frame%72,421,24);
    for(unsigned i=0;i<NoteEmitter::Count;++i){
      const auto p=Sample(60,i,421,24),q=expected[i];
      require(p.x==q.x&&p.y==q.y&&p.size==q.size&&p.alpha==q.alpha,
              "particle changed with call history or frame rate");
    }
  }
  // Smooth envelopes have zero slope at both endpoints.
  const float epsilon=.0001f;
  require(NoteEmitter::Smooth(0,1,epsilon)/epsilon<.001f,
          "birth envelope has nonzero endpoint slope");
  require((1-NoteEmitter::Smooth(0,1,1-epsilon))/epsilon<.001f,
          "death envelope has nonzero endpoint slope");
  std::cout<<"OK: continuous zigzag particles, invisible recycling, upward paths, long notes and frame independence\n";
}
