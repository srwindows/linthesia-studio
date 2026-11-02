#ifndef LINTHESIA_FLUID_ENGINE_H
#define LINTHESIA_FLUID_ENGINE_H
#include <string>
#include <vector>
#include "AdvancedSettings.h"
#include "libmidi/MidiEvent.h"
namespace FluidEngine {
 struct Preset {int bank=0,program=0;std::string name;};
 std::vector<Preset> Presets();
 bool SelectPreset(int channel,int bank,int program);
 // Values read back from the running FluidSynth DSP, not merely UI state.
 struct EffectReadback {
    double roomSize=0,damping=0,width=0,reverbLevel=0;
    int voices=0;
    double chorusLevel=0,chorusRate=0,chorusDepth=0;
 };
 bool ReadEffects(EffectReadback& out);
 bool Start(const AdvancedSettings& settings,std::string& error);
 void Shutdown();
 bool IsRunning();
 std::string Status();
 void Send(const MidiEvent& event);
 void AllSoundsOff();
 void NoteOn();
 void NoteOff();
}
#endif
