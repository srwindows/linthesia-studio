// Linthesia GPL v2. Evaluación MIDI aislada.
#include "PracticeError.h"
#include <cassert>
#include <vector>
#include <iostream>
int main(){
  const std::vector<int> chord={60,64,67};
  const std::vector<int> silence={};
  using PracticeError::WrongPress;
  assert(!WrongPress(60,false,true,chord));  // Do correcto
  assert(!WrongPress(64,false,true,chord));  // Mi correcto
  assert(!WrongPress(67,false,true,chord));  // Sol correcto
  assert(WrongPress(61,false,true,chord));   // tecla ajena a acorde
  assert(!WrongPress(61,true,true,chord));   // repetición sostenida
  assert(!WrongPress(61,false,false,chord)); // pausa, ajustes, reproducción automática
  assert(!WrongPress(61,false,true,silence));// no penalizar silencios/omisiones
  assert(WrongPress(61,false,true,{60}));    // melodía mono
  std::cout<<"OK: errores solo pulsaciones equivocadas durante teclas exigidas\n";
}
