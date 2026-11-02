// Linthesia GPL v2. Evaluación de teclas MIDI, no de silencios.
#ifndef LINTHESIA_PRACTICE_ERROR_H
#define LINTHESIA_PRACTICE_ERROR_H
#include <algorithm>
#include <vector>
namespace PracticeError {
inline bool WrongPress(int pressed, bool held, bool evaluating,
                       const std::vector<int>& expected){
  // Solo cuenta una pulsación nueva que no sea ninguna tecla exigida.
  return evaluating && !held && !expected.empty() &&
         std::find(expected.begin(),expected.end(),pressed)==expected.end();
}
}
#endif
