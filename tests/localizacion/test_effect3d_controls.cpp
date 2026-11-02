#include "NoteEmitter.h"
#include "Effect3DPalette.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
  const auto initial=NoteEmitter::Sample(0.7f,5,128,23.0f);
  const auto small=NoteEmitter::Sample(0.7f,5,128,23.0f,1.0f,0.5f);
  const auto large=NoteEmitter::Sample(0.7f,5,128,23.0f,1.0f,2.0f);
  assert(initial.size>0 && std::abs(small.size/initial.size-0.5f)<0.01f);
  assert(std::abs(large.size/initial.size-2.0f)<0.01f);
  const auto close=NoteEmitter::Sample(0.7f,5,128,23.0f,0.0f,1.0f);
  const auto wide=NoteEmitter::Sample(0.7f,5,128,23.0f,2.0f,1.0f);
  assert(std::abs(wide.x)>std::abs(close.x)+1.0f);
  assert(NoteEmitter::Count>=144);
  auto e=NoteEmitter::Sample(0.5f,143,41,24.0f,1.0f,1.0f,144);
  assert(std::isfinite(e.x));
  assert(NoteEmitter::Sample(0.5f,12,41,24.0f,1.0f,1.0f,12).alpha==0);
  SDL_Color a{114,159,207,255};
  for(int palette=0;palette<Effect3DPalette::Count;++palette){
    assert(Effect3DPalette::Name(palette)[0]);
    for(int track=0;track<16;++track){
      const auto col=Effect3DPalette::Color(a,palette,track);
      assert(col.a==255);
      if(palette==0)assert(col.r==a.r && col.g==a.g && col.b==a.b);
    }
  }
  const auto fire=Effect3DPalette::Color(a,4,0);
  const auto ocean=Effect3DPalette::Color(a,5,0);
  assert(fire.r>fire.b && ocean.b>ocean.r);
  // A palette must no longer replace the user-selected track hue.
  const SDL_Color chosenBlue{114,159,207,255};
  const SDL_Color chosenGreen{138,226,52,255};
  const SDL_Color chosenPurple{147,130,244,255};
  for(int preset=0;preset<Effect3DPalette::Count;++preset){
    const auto blue=Effect3DPalette::TrackTint(chosenBlue,preset,1);
    const auto green=Effect3DPalette::TrackTint(chosenGreen,preset,1);
    const auto purple=Effect3DPalette::TrackTint(chosenPurple,preset,1);
    assert(blue.b>blue.r && blue.b>blue.g);
    assert(green.g>green.r && green.g>green.b);
    assert(purple.b>purple.r && purple.b>purple.g);
    assert(blue.r!=green.r && blue.g!=green.g && green.b!=purple.b);
    if(preset==0){ assert(blue.b==207 && green.g==226 && purple.r==147); }
  }
  puts("OK: size, spread, count, palette swatches and per-track colors preserved");
}
