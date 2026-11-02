#include "FluidEngine.h"
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cmath>
int main(){
 // SFZ is not a supported native format: reject clearly, do not pretend it loaded.
 const std::string sfz="/tmp/linthesia-regression-unsupported.sfz";
 {std::ofstream file(sfz);file<<"<group>\nsample=piano.wav\n";}
 AdvancedSettings rejected;
 rejected.soundfont=sfz;
 std::string rejectReason;
 if(FluidEngine::Start(rejected,rejectReason)){
  unlink(sfz.c_str());
  fprintf(stderr,"FAIL: SFZ was accepted without an SFZ engine\n");
  FluidEngine::Shutdown();return 7;
 }
 unlink(sfz.c_str());
 if(rejectReason.find("SF2/SF3")==std::string::npos){
  fprintf(stderr,"FAIL: SFZ not explained: %s\n",rejectReason.c_str());
  return 8;
 }
 AdvancedSettings setting;
 setting.audioDriver="alsa";
 setting.audioDevice="null";
 setting.soundfont="/usr/share/sounds/sf2/default-GM.sf2";
 setting.sampleRate=48000;
 setting.reverbSize=80;setting.reverbDamp=40;
 setting.reverbWidth=60;setting.reverbLevel=30;
 setting.chorusVoices=5;setting.chorusLevel=50;
 setting.chorusRate=15;setting.chorusDepth=14;
 std::string error;
 if(!FluidEngine::Start(setting,error)){
  fprintf(stderr,"FAIL: %s\n",error.c_str());
  return 2;
 }
 if(!FluidEngine::IsRunning())return 3;
 FluidEngine::EffectReadback effects;
 if(!FluidEngine::ReadEffects(effects)){
  fprintf(stderr,"FAIL: cannot read actual DSP values\n");
  FluidEngine::Shutdown();return 9;
 }
 auto near=[](double x,double target){return std::fabs(x-target)<0.02;};
 if(!near(effects.roomSize,0.8)||!near(effects.damping,0.4)||
    !near(effects.width,60.0)||!near(effects.reverbLevel,0.3)||
    effects.voices!=5||!near(effects.chorusLevel,5.0)||
    !near(effects.chorusRate,1.5)||!near(effects.chorusDepth,14)){
  fprintf(stderr,"FAIL: DSP parameters differ: size=%f damp=%f width=%f "
    "level=%f chorusVoices=%d level=%f rate=%f depth=%f\n",
    effects.roomSize,effects.damping,effects.width,effects.reverbLevel,
    effects.voices,effects.chorusLevel,effects.chorusRate,effects.chorusDepth);
  FluidEngine::Shutdown();return 10;
 }
 puts("PASS: reverb and chorus live parameters read back from FluidSynth");
 FluidEngine::NoteOn();
 std::this_thread::sleep_for(std::chrono::milliseconds(150));
 FluidEngine::NoteOff();
 FluidEngine::Shutdown();
 if(FluidEngine::IsRunning())return 4;
 puts("PASS: FluidSynth reproducido por ALSA null sin hardware real");
 return 0;
}
