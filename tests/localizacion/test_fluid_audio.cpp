#include <fluidsynth.h>
#include <sys/stat.h>
#include <vector>
#include <cmath>
#include <cstdio>
int main(){
 const char* sf="/usr/share/sounds/sf2/default-GM.sf2";
 struct stat statbuf{};
 if(stat(sf,&statbuf)!=0){puts("SKIP: SoundFont GM no instalado");return 77;}
 fluid_settings_t* settings=new_fluid_settings();
 if(!settings)return 1;
 fluid_settings_setnum(settings,"synth.sample-rate",48000);
 fluid_settings_setint(settings,"synth.polyphony",256);
 fluid_synth_t* synth=new_fluid_synth(settings);
 if(!synth){delete_fluid_settings(settings);return 2;}
 if(fluid_synth_sfload(synth,sf,1)==FLUID_FAILED){
  delete_fluid_synth(synth);delete_fluid_settings(settings);return 3;
 }
 fluid_synth_program_change(synth,0,0);
 fluid_synth_noteon(synth,0,60,105);
 std::vector<float> left(8192),right(8192);
 fluid_synth_write_float(synth,left.size(),left.data(),0,1,right.data(),0,1);
 float peak=0;
 for(size_t i=0;i<left.size();i++)
  peak=std::max(peak,std::max(std::abs(left[i]),std::abs(right[i])));
 fluid_synth_all_sounds_off(synth,0);
 delete_fluid_synth(synth);delete_fluid_settings(settings);
 if(peak<0.00001f){puts("FAIL: audio vacío");return 4;}
 printf("PASS: sintetizador SF2 produce audio real a 48kHz; peak=%f\n",peak);
 return 0;
}
