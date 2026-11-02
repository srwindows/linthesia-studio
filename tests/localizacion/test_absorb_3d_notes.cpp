#include "ShortNoteVisual.h"
#include <cassert>
#include <iostream>
int main(){
  using namespace ShortNoteVisual;
  const int strike=580,body=72;
  const auto arriving=Absorb(strike,body,0.0f);
  const auto entering=Absorb(strike,body,0.10f);
  const auto sinking=Absorb(strike,body,0.20f);
  const auto swallowed=Absorb(strike,body,0.40f);
  assert(arriving.top+arriving.height==strike);
  assert(entering.top+entering.height==strike);
  assert(sinking.top+sinking.height==strike);
  assert(arriving.height==72);
  assert(entering.height<arriving.height);
  assert(sinking.height<entering.height);
  assert(swallowed.height==0);
  assert(Absorb(strike,body,5.f).height==0);
  // Normal incoming notes still carry their intact MIDI note length.
  const auto incoming=Fit(400,510,body);
  assert(incoming.top+incoming.height==510);
  assert(incoming.height==body);
  std::cout<<"OK: note mesh is swallowed, remains flush to piano, MIDI untouched\n";
}
