#include "TopBirthEmitter.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
static void check(bool b,const char* msg){if(!b){fprintf(stderr,"FAIL %s\n",msg);exit(1);}}
int main(){
  const unsigned seed=34994u;
  check(TopBirthEmitter::Sample(-1,1,seed,20).alpha==0,"negative birth time");
  check(TopBirthEmitter::Sample(2,1,seed,20).alpha==0,"burst never stops");
  check(TopBirthEmitter::Sample(.25f,1,seed,20,1,1,0).alpha==0,"zero count");
  check(TopBirthEmitter::Sample(.25f,99,seed,20,1,1,24).alpha==0,"slot outside count");
  int falling=0,alive=0;
  for(unsigned i=0;i<36;++i){
    const auto initial=TopBirthEmitter::Sample(.44f,i,seed,20,1,1,36);
    const auto later=TopBirthEmitter::Sample(.54f,i,seed,20,1,1,36);
    if(initial.alpha>.05f && later.alpha>.05f){
      ++alive;
      check(std::isfinite(later.y) && std::isfinite(later.x),"nonfinite position");
      if(later.y>initial.y)++falling;
      const auto tiny=TopBirthEmitter::Sample(.44f,i,seed,20,1,.5f,36);
      check(std::abs(tiny.size-initial.size*.5f)<.01f,"size not configurable");
    }
  }
  check(alive>=15 && falling==alive,"particles should be falling downward");
  // Explosion, particles and the growing note start together.
  check(TopBirthEmitter::Sample(.08f,0,seed,20).alpha>0,
        "particles must appear with explosion");
  check(TopBirthEmitter::Reveal(.0f)==0.0f,
        "note should start from its first pixels");
  check(TopBirthEmitter::Reveal(.08f)>0.0f,
        "note formation should begin with particles");
  check(TopBirthEmitter::Reveal(.25f)>.05f &&
        TopBirthEmitter::Reveal(.25f)<.95f,
        "column should emerge gradually");
  check(TopBirthEmitter::Reveal(.52f)==1.0f,
        "note body must be complete in under a second");
  const auto a=TopBirthEmitter::Sample(.3f,3,seed,20,1.0f,1,36);
  const auto b=TopBirthEmitter::Sample(.3f,3,seed,20,1.0f,1,36);
  check(a.x==b.x && a.y==b.y && a.alpha==b.alpha,"non deterministic top effect");
  check(TopBirthEmitter::Sample(TopBirthEmitter::Duration,0,seed,20).alpha==0,
        "one-shot burst must end");
  puts("OK: deterministic one-shot top burst with falling particles and configurable size");
}
