#ifndef LINTHESIA_MIDI_ACTIVITY_H
#define LINTHESIA_MIDI_ACTIVITY_H
#include <cstdint>
#include <string>
class MidiEvent;
namespace MidiActivity {
 struct Snapshot {
  uint64_t events=0;
  int channel=0,note=-1,velocity=0;
  std::string last="Esperando eventos MIDI";
  int bars[16]={};
 };
 void Observe(const MidiEvent& ev);
 Snapshot Now();
 void Reset();
}
#endif
