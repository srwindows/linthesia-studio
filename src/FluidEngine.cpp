#include "FluidEngine.h"
#include "AudioPeakGuard.h"
#include <fluidsynth.h>
#include <fluidsynth/ladspa.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdio>
#include <sys/stat.h>
#include <algorithm>
#include <string>
namespace {
fluid_settings_t* fluidSettings=nullptr;
fluid_synth_t* synth=nullptr;
fluid_audio_driver_t* audio=nullptr;
// FluidSynth's built-in PulseAudio driver outputs approximately 36 dB
// below fluid_synth_write_float on this Debian/PipeWire host. Render the
// actual FluidSynth floating-point PCM and submit it unscaled via Pulse
// Simple API. The native ALSA driver remains unchanged.
pa_simple* directPulse=nullptr;
std::thread directThread;
std::atomic<bool> directRunning{false};
std::atomic<bool> directFault{false};
std::mutex renderMutex;
void directRender(int frames){
 std::vector<float> left(frames),right(frames),interleaved(frames*2);
 while(directRunning.load()){
  {
   std::lock_guard<std::mutex> guard(renderMutex);
   if(!synth || fluid_synth_write_float(synth,frames,left.data(),0,1,
                                      right.data(),0,1)!=FLUID_OK){
    directFault.store(true);break;
   }
  }
  for(int i=0;i<frames;++i){
   // A full MIDI arrangement can exceed 0 dBFS even when the LADSPA
   // effect is enabled; protect the FINAL PCM float stream instead of
   // relying on the synth mixer's internal peaks. Do not alter quieter
   // notes or the left/right balance.
   AudioPeakGuard::StereoSoftLimit(left[i],right[i]);
   interleaved[2*i]=left[i];
   interleaved[2*i+1]=right[i];
  }
  int err=PA_OK;
  if(pa_simple_write(directPulse,interleaved.data(),
           interleaved.size()*sizeof(float),&err)<0){
   directFault.store(true);
   fprintf(stderr,"[Linthesia] Salida PCM PipeWire/PulseAudio: %s\n",
           pa_strerror(err));
   break;
  }
 }
 directRunning.store(false);
}
std::string currentStatus="FluidSynth detenido";
bool validFile(const std::string& path){
 struct stat st{};
 return stat(path.c_str(),&st)==0 && S_ISREG(st.st_mode) &&
       (path.size()>4 && (path.substr(path.size()-4)==".sf2" ||
                          path.substr(path.size()-4)==".sf3"));
}
bool validLadspa(const std::string& path){
 struct stat st{};
 return stat(path.c_str(),&st)==0 && S_ISREG(st.st_mode);
}
void closeEngine(){
 directRunning.store(false);
 // Never free the synth or pa_simple until the PCM writer has stopped.
 if(directThread.joinable())directThread.join();
 if(directPulse){pa_simple_free(directPulse);directPulse=nullptr;}
 directFault.store(false);
 if(audio){delete_fluid_audio_driver(audio);audio=nullptr;}
 if(synth){delete_fluid_synth(synth);synth=nullptr;}
 if(fluidSettings){delete_fluid_settings(fluidSettings);fluidSettings=nullptr;}
}
}
namespace FluidEngine {
bool Start(const AdvancedSettings& a,std::string& error){
 // First create the new engine without disturbing an existing playing synth.
 if(!validFile(a.soundfont)){
  error="SoundFont SF2/SF3 no disponible: "+a.soundfont;return false;
 }
 fluid_settings_t* settings=new_fluid_settings();
 if(!settings){error="No se pueden crear los ajustes FluidSynth";return false;}
 fluid_settings_setstr(settings,"audio.driver",a.audioDriver.c_str());
 if(a.audioDevice!="default" && !a.audioDevice.empty()){
  if(a.audioDriver=="alsa")fluid_settings_setstr(settings,"audio.alsa.device",a.audioDevice.c_str());
  else if(a.audioDriver=="pulseaudio")fluid_settings_setstr(settings,"audio.pulseaudio.device",a.audioDevice.c_str());
 }
 // The booster requires a real limiter. The system FluidSynth 2.4.4 does
 // not include its optional built-in limiter, so load a verified LADSPA
 // fast-lookahead limiter rather than increasing synth.gain unprotected.
 // Calibrated on FluidR3 GM: Do4 at velocity 90 is -32.35 dBFS RMS
 // without boost; +18 dB reaches -14.38 dBFS with a -1.5 dBFS limiter.
 // Keep the user-selectable boost OFF for other destinations.
 const int boost=std::max(0,std::min(24,a.fantomUsbBoostDb));
 const bool fantomOutput=a.audioDevice.find("Roland_FANTOM")!=
                          std::string::npos;
 const bool useBoost=boost>0 && fantomOutput;
 // The direct PCM path removes a 36 dB attenuation. A full MIDI
 // arrangement can now peak above 0 dBFS, so the FANTOM PCM output needs
 // its peak limiter even when the optional boost is set to 0 dB.
 const bool useLimiter=useBoost ||
     (fantomOutput && a.audioDriver=="pulseaudio");
 const char* limiterPath="/usr/lib/ladspa/fast_lookahead_limiter_1913.so";
 if(useLimiter && !validLadspa(limiterPath)){
   delete_fluid_settings(settings);
   error="No disponible el limitador de seguridad LADSPA. Refuerzo no aplicado.";
   return false;
 }
 if(useLimiter &&
    fluid_settings_setint(settings,"synth.ladspa.active",1)!=FLUID_OK){
   delete_fluid_settings(settings);
   error="El motor no admite el limitador LADSPA; no se aumenta ganancia.";
   return false;
 }
 fluid_settings_setnum(settings,"synth.sample-rate",a.sampleRate);
 fluid_settings_setint(settings,"synth.polyphony",a.polyphony);
 fluid_settings_setnum(settings,"synth.gain",
     (a.gain/100.0)*(a.masterVolume/100.0)*0.5);
 fluid_settings_setint(settings,"audio.period-size",a.bufferSize);
 fluid_settings_setint(settings,"audio.periods",2);
 fluid_synth_t* next=new_fluid_synth(settings);
 if(!next){delete_fluid_settings(settings);error="No se puede iniciar sintetizador";return false;}
 if(useLimiter){
   fluid_ladspa_fx_t* fx=fluid_synth_get_ladspa_fx(next);
   const char* name="limiter";
   const bool ok=fx &&
       fluid_ladspa_add_effect(fx,name,limiterPath,"fastLookaheadLimiter")==FLUID_OK &&
       fluid_ladspa_effect_link(fx,name,"Input 1","Main:L")==FLUID_OK &&
       fluid_ladspa_effect_link(fx,name,"Input 2","Main:R")==FLUID_OK &&
       fluid_ladspa_effect_link(fx,name,"Output 1","Main:L")==FLUID_OK &&
       fluid_ladspa_effect_link(fx,name,"Output 2","Main:R")==FLUID_OK &&
       fluid_ladspa_effect_set_control(fx,name,"Input gain",
                                       static_cast<float>(boost))==FLUID_OK &&
       fluid_ladspa_effect_set_control(fx,name,"Limit",-1.5f)==FLUID_OK &&
       fluid_ladspa_effect_set_control(fx,name,"Release",0.12f)==FLUID_OK &&
       fluid_ladspa_activate(fx)==FLUID_OK;
   if(!ok){
     delete_fluid_synth(next);delete_fluid_settings(settings);
     error="No se pudo activar el limitador. Se conserva la salida anterior.";
     return false;
   }
 }
 if(fluid_synth_sfload(next,a.soundfont.c_str(),1)==FLUID_FAILED){
  delete_fluid_synth(next);delete_fluid_settings(settings);
  error="FluidSynth no pudo cargar el SoundFont";return false;
 }
 // Apply the actual FluidSynth DSP values before activating the driver.
 // Group 0 exists in the standard stereo synth (not a decorative UI).
 fluid_synth_set_reverb_group_roomsize(next,0,a.reverbSize/100.0);
 fluid_synth_set_reverb_group_damp(next,0,a.reverbDamp/100.0);
 fluid_synth_set_reverb_group_width(next,0,a.reverbWidth);
 fluid_synth_set_reverb_group_level(next,0,a.reverbLevel/100.0);
 fluid_synth_set_chorus_group_nr(next,0,a.chorusVoices);
 fluid_synth_set_chorus_group_level(next,0,a.chorusLevel/10.0);
 fluid_synth_set_chorus_group_speed(next,0,a.chorusRate/10.0);
 fluid_synth_set_chorus_group_depth(next,0,a.chorusDepth);
 fluid_synth_reverb_on(next,-1,a.reverb?1:0);
 fluid_synth_chorus_on(next,-1,a.chorus?1:0);
 fluid_audio_driver_t* output=nullptr;
 pa_simple* pulse=nullptr;
 if(a.audioDriver=="pulseaudio"){
  // Use exactly the target sink selected in Advanced Settings (no
  // system-default rerouting). Unscaled float32 interleaved stereo.
  const pa_sample_spec spec={PA_SAMPLE_FLOAT32NE,
      static_cast<uint32_t>(a.sampleRate),2};
  const unsigned int frames=static_cast<unsigned int>(
      std::max(128,std::min(4096,a.bufferSize)));
  pa_buffer_attr attr={static_cast<uint32_t>(-1),
                       static_cast<uint32_t>(frames*2*sizeof(float)*4),
                       static_cast<uint32_t>(-1),
                       static_cast<uint32_t>(-1),
                       static_cast<uint32_t>(-1)};
  const char* destination=(a.audioDevice.empty() ||
       a.audioDevice=="default")?nullptr:a.audioDevice.c_str();
  int err=PA_OK;
  pulse=pa_simple_new(nullptr,"Linthesia",PA_STREAM_PLAYBACK,
                      destination,"FluidSynth PCM directo",
                      &spec,nullptr,&attr,&err);
  if(!pulse){
   delete_fluid_synth(next);delete_fluid_settings(settings);
   error="No se puede abrir la salida PCM "+a.audioDevice+
         ": "+pa_strerror(err);
   return false;
  }
 }else{
  // Keep the original native driver for ALSA/JACK and other engines.
  output=new_fluid_audio_driver(settings,next);
  if(!output){
   delete_fluid_synth(next);delete_fluid_settings(settings);
   error="No hay salida de audio FluidSynth: "+a.audioDriver;
   return false;
  }
 }
 closeEngine();
 fluidSettings=settings;synth=next;audio=output;directPulse=pulse;
 if(directPulse){
  directFault.store(false);
  directRunning.store(true);
  const int frames=std::max(128,std::min(4096,a.bufferSize));
  directThread=std::thread(directRender,frames);
 }
 currentStatus=std::string("FluidSynth activo · ")+a.audioDriver+
       (directPulse?" · PCM float32 directo":"")+
       (useLimiter?" · limitador -1,5 dBFS":"")+
       (useBoost?" · FANTOM USB +"+std::to_string(boost)+" dB":"");
 error.clear();return true;
}
void Shutdown(){closeEngine();currentStatus="FluidSynth detenido";}
bool IsRunning(){return synth!=nullptr &&
    (audio!=nullptr || (directPulse!=nullptr && !directFault.load()));}
bool ReadEffects(EffectReadback& out){
 if(!IsRunning())return false;
 return fluid_synth_get_reverb_group_roomsize(synth,0,&out.roomSize)==FLUID_OK &&
        fluid_synth_get_reverb_group_damp(synth,0,&out.damping)==FLUID_OK &&
        fluid_synth_get_reverb_group_width(synth,0,&out.width)==FLUID_OK &&
        fluid_synth_get_reverb_group_level(synth,0,&out.reverbLevel)==FLUID_OK &&
        fluid_synth_get_chorus_group_nr(synth,0,&out.voices)==FLUID_OK &&
        fluid_synth_get_chorus_group_level(synth,0,&out.chorusLevel)==FLUID_OK &&
        fluid_synth_get_chorus_group_speed(synth,0,&out.chorusRate)==FLUID_OK &&
        fluid_synth_get_chorus_group_depth(synth,0,&out.chorusDepth)==FLUID_OK;
}
std::string Status(){return directFault.load()?
  "Error de salida PCM PulseAudio/PipeWire; revisa el dispositivo":currentStatus;}
std::vector<Preset> Presets(){
 std::lock_guard<std::mutex> guard(renderMutex);
 std::vector<Preset> result;
 if(!synth)return result;
 for(unsigned font=0;;++font){
   fluid_sfont_t* sf=fluid_synth_get_sfont(synth,font);
   if(!sf)break;
   fluid_sfont_iteration_start(sf);
   while(fluid_preset_t* preset=fluid_sfont_iteration_next(sf)){
     Preset p;
     p.bank=fluid_preset_get_banknum(preset);
     p.program=fluid_preset_get_num(preset);
     const char* label=fluid_preset_get_name(preset);
     p.name=label?label:"Sin nombre";
     if(p.bank>=0&&p.bank<=16383&&p.program>=0&&p.program<128)
       result.push_back(std::move(p));
   }
 }
 std::sort(result.begin(),result.end(),[](const Preset& a,const Preset& b){
   if(a.bank!=b.bank)return a.bank<b.bank;
   return a.program<b.program;
 });
 return result;
}
bool SelectPreset(int channel,int bank,int program){
  std::lock_guard<std::mutex> guard(renderMutex);
  if(!synth||channel<0||channel>15||bank<0||bank>16383||
     program<0||program>127)return false;
  fluid_sfont_t* sf=fluid_synth_get_sfont(synth,0);
  if(!sf || !fluid_sfont_get_preset(sf,bank,program))return false;
  return fluid_synth_program_select(synth,channel,
           fluid_sfont_get_id(sf),bank,program)==FLUID_OK;
}
void AllSoundsOff(){
 std::lock_guard<std::mutex> guard(renderMutex);
 if(!synth)return;
 for(int channel=0;channel<16;++channel)fluid_synth_all_sounds_off(synth,channel);
}
void Send(const MidiEvent& event){
 std::lock_guard<std::mutex> guard(renderMutex);
 if(!synth)return;
 const int ch=event.Channel();
 switch(event.Type()){
  case MidiEventType_NoteOn:
   if(event.NoteVelocity()>0)fluid_synth_noteon(synth,ch,event.NoteNumber(),event.NoteVelocity());
   else fluid_synth_noteoff(synth,ch,event.NoteNumber());
   break;
  case MidiEventType_NoteOff:
   fluid_synth_noteoff(synth,ch,event.NoteNumber());break;
  case MidiEventType_ProgramChange:
   fluid_synth_program_change(synth,ch,event.ProgramNumber());break;
  default:{
   MidiEventSimple data;
   if(!event.GetSimpleEvent(&data))break;
   switch(data.status&0xF0){
    case 0xB0:
     fluid_synth_cc(synth,ch,data.byte1,data.byte2);break;
    case 0xE0:
     fluid_synth_pitch_bend(synth,ch,
                 int(data.byte1)+(int(data.byte2)<<7));break;
    case 0xD0:
     fluid_synth_channel_pressure(synth,ch,data.byte1);break;
    case 0xA0:
     fluid_synth_key_pressure(synth,ch,data.byte1,data.byte2);break;
    default:break;
   }
   break;
  }
 }
}
void NoteOn(){
 std::lock_guard<std::mutex> guard(renderMutex);
 if(synth)fluid_synth_noteon(synth,0,60,90);
}
void NoteOff(){
 std::lock_guard<std::mutex> guard(renderMutex);
 if(synth)fluid_synth_noteoff(synth,0,60);
}
}
