#include "FluidEngine.h"
#include <iostream>
#include <thread>
#include <chrono>
int main(int argc,char**argv) {
 if(argc!=2)return 1;
 AdvancedSettings a;
 a.audioDriver="pulseaudio";
 a.audioDevice=argv[1];
 a.sampleRate=48000;
 a.bufferSize=512;
 a.gain=100;
 a.masterVolume=100;
 a.soundfont="/usr/share/sounds/sf2/FluidR3_GM.sf2";
 a.fantomUsbBoostDb=0;  // Direct PCM fixes the level; no booster needed.
 a.reverb=false;a.chorus=false;
 std::string error;
 if(!FluidEngine::Start(a,error)){
  std::cerr<<"FAILED_TO_START: "<<error<<"\n";return 2;
 }
 if(FluidEngine::Status().find("PCM float32 directo")==std::string::npos)
  return 3;
 std::this_thread::sleep_for(std::chrono::milliseconds(800));
 FluidEngine::Send(MidiEvent::Build(MidiEventSimple(0xB0,7,110)));
 FluidEngine::Send(MidiEvent::Build(MidiEventSimple(0xB0,11,127)));
 FluidEngine::Send(MidiEvent::Build(MidiEventSimple(0xC0,0,0)));
 FluidEngine::Send(MidiEvent::Build(MidiEventSimple(0x90,60,90)));
 std::this_thread::sleep_for(std::chrono::milliseconds(2000));
 FluidEngine::Send(MidiEvent::Build(MidiEventSimple(0x80,60,0)));
 std::this_thread::sleep_for(std::chrono::milliseconds(300));
 if(!FluidEngine::IsRunning())return 4;
 FluidEngine::Shutdown();
 if(FluidEngine::IsRunning())return 5;
 std::cout<<"OK: renderer float32, MIDI note and thread cleanup\n";
}
