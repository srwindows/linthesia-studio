#include "ImpactFlameEmitter.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
  using namespace ImpactFlameEmitter;
  assert(Duration>=.25f && Duration<=.33f);
  for(unsigned seed:{1u,123u,98173u}){
    float energy=.0f;
    unsigned moving=0;
    for(unsigned i=0;i<Count;++i){
      const auto a=Sample(.095f,i,seed,18.f);
      const auto b=Sample(.135f,i,seed,18.f);
      const auto c=Sample(.270f,i,seed,18.f);
      const auto d=Sample(.350f,i,seed,18.f);
      assert(d.alpha==0.f);
      assert(Sample(2.f,i,seed,18.f).alpha==0.f);
      assert(std::isfinite(a.x)&&std::isfinite(a.y));
      assert(a.y<=0.f && b.y<=0.f && c.y<=0.f);
      assert(a.alpha>=0.f && a.alpha<=1.f);
      energy+=a.alpha+b.alpha;
      if(a.alpha>.035f && b.alpha>.035f){
        assert(b.y<a.y); // spark rises with time
        moving++;
      }
    }
    assert(energy>5.0f);
    assert(moving>=6);
  }
  // No dependence on sampling FPS or history.
  for(unsigned i=0;i<Count;++i){
    const auto a=Sample(.125f,i,881,22.f);
    Sample(.050f,i,881,22.f);
    const auto b=Sample(.125f,i,881,22.f);
    assert(a.x==b.x && a.y==b.y && a.alpha==b.alpha);
  }
  puts("OK: short cinematic rising sparks, deterministic motion, no permanent white flame");
}
