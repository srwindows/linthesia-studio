#include "AdvancedSettings.h"
#include "StageScenes.h"
#include <glib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <string>

namespace {
int bounded(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
int getint(GKeyFile* f,const char* group,const char* key,int otherwise){
 GError* e=nullptr;
 int r=g_key_file_get_integer(f,group,key,&e);
 if(e){g_error_free(e);return otherwise;}
 return r;
}
bool getbool(GKeyFile* f,const char* group,const char* key,bool otherwise){
 GError* e=nullptr;
 bool r=g_key_file_get_boolean(f,group,key,&e);
 if(e){g_error_free(e);return otherwise;}
 return r;
}
std::string getstr(GKeyFile* f,const char* group,const char* key,const std::string& otherwise){
 GError* e=nullptr;
 gchar* val=g_key_file_get_string(f,group,key,&e);
 if(e){g_error_free(e);return otherwise;}
 std::string result=val?val:"";g_free(val);
 return result;
}
}
namespace AdvancedSettingsIO {
std::string Filename(){
 return std::string(g_get_user_config_dir())+"/linthesia/ajustes.ini";
}
AdvancedSettings Load(){
 AdvancedSettings a;
 GKeyFile* f=g_key_file_new();
 const std::string path=Filename();
 g_key_file_load_from_file(f,path.c_str(),G_KEY_FILE_NONE,nullptr);
 a.fps=getint(f,"Pantalla","fps",a.fps);
 if(a.fps!=30 && a.fps!=60 && a.fps!=120 && a.fps!=300)a.fps=300;
 a.windowWidth=bounded(getint(f,"Pantalla","width",a.windowWidth),720,3840);
 a.windowHeight=bounded(getint(f,"Pantalla","height",a.windowHeight),540,2160);
 a.fullscreen=getbool(f,"Pantalla","fullscreen",a.fullscreen);
 a.showFps=getbool(f,"Pantalla","show-fps",a.showFps);
 a.uiScale=bounded(getint(f,"Pantalla","scale",a.uiScale),75,150);
 a.theme=bounded(getint(f,"Pantalla","theme",a.theme),0,2);
 a.masterVolume=bounded(getint(f,"Sonido","master-volume",a.masterVolume),0,100);
 a.fantomUsbBoostDb=bounded(getint(f,"Sonido","fantom-usb-boost-db",0),0,24);
 a.sampleRate=getint(f,"Sonido","sample-rate",a.sampleRate);
 if(a.sampleRate!=44100 && a.sampleRate!=48000 && a.sampleRate!=96000)a.sampleRate=48000;
 a.bufferSize=getint(f,"Sonido","buffer",a.bufferSize);
 if(a.bufferSize!=256 && a.bufferSize!=512 && a.bufferSize!=1024 && a.bufferSize!=2048)a.bufferSize=512;
 a.audioDriver=getstr(f,"Sonido","driver",a.audioDriver);
 a.audioDevice=getstr(f,"Sonido","device",a.audioDevice);
 if(a.audioDriver!="pulseaudio" && a.audioDriver!="alsa" && a.audioDriver!="jack")
  a.audioDriver="pulseaudio";
 a.soundfont=getstr(f,"FluidSynth","soundfont",a.soundfont);
 a.reverb=getbool(f,"FluidSynth","reverb",a.reverb);
 a.chorus=getbool(f,"FluidSynth","chorus",a.chorus);
 a.reverbSize=bounded(getint(f,"FluidSynth","reverb-size",a.reverbSize),0,100);
 a.reverbDamp=bounded(getint(f,"FluidSynth","reverb-damp",a.reverbDamp),0,100);
 a.reverbWidth=bounded(getint(f,"FluidSynth","reverb-width",a.reverbWidth),0,100);
 a.reverbLevel=bounded(getint(f,"FluidSynth","reverb-level",a.reverbLevel),0,100);
 a.chorusVoices=bounded(getint(f,"FluidSynth","chorus-voices",a.chorusVoices),1,16);
 a.chorusLevel=bounded(getint(f,"FluidSynth","chorus-level",a.chorusLevel),0,100);
 a.chorusRate=bounded(getint(f,"FluidSynth","chorus-rate",a.chorusRate),1,50);
 a.chorusDepth=bounded(getint(f,"FluidSynth","chorus-depth",a.chorusDepth),0,21);
 gsize fontCount=0;GError* fontsError=nullptr;
 gchar** fonts=g_key_file_get_string_list(f,"FluidSynth","extra-soundfonts",
                                           &fontCount,&fontsError);
 if(fontsError)g_error_free(fontsError);
 if(fonts){
  for(gsize i=0;i<fontCount;++i)
    if(fonts[i]&&*fonts[i])a.extraSoundfonts.emplace_back(fonts[i]);
  g_strfreev(fonts);
 }
 a.polyphony=bounded(getint(f,"FluidSynth","polyphony",a.polyphony),32,1024);
 a.gain=bounded(getint(f,"FluidSynth","gain",a.gain),0,100);
 a.midiInput=getstr(f,"MIDI","input",a.midiInput);
 a.midiOutput=getstr(f,"MIDI","output",a.midiOutput);
 a.midiThru=getbool(f,"MIDI","thru",a.midiThru);
 a.midiChannel=bounded(getint(f,"MIDI","channel",0),0,16);
 a.libraryPath=getstr(f,"Biblioteca","path",a.libraryPath);
 a.rememberFolder=getbool(f,"Biblioteca","remember-last",a.rememberFolder);
 a.alphabeticOrder=getbool(f,"Biblioteca","alphabetic",a.alphabeticOrder);
 a.multipleSelect=getbool(f,"Controles","multiple",a.multipleSelect);
 a.dragHoverMs=bounded(getint(f,"Controles","hover-ms",a.dragHoverMs),400,1600);
 a.doubleClickMs=bounded(getint(f,"Controles","double-ms",a.doubleClickMs),250,700);
 a.recentLimit=bounded(getint(f,"Biblioteca","recent-limit",a.recentLimit),1,500);
 a.recentSort=bounded(getint(f,"Biblioteca","recent-sort",a.recentSort),0,3);
 a.diagnostics=getbool(f,"Avanzado","logs",a.diagnostics);
 a.playerPianoPct=bounded(getint(f,"Reproductor","piano-percent",16),10,25);
 a.playerTheme=bounded(getint(f,"Reproductor","color-theme",0),0,2);
 a.playerNoteDurationMs=bounded(getint(f,"Reproductor","note-ms",3500),500,10000);
 a.player3D=getbool(f,"Reproductor","view-3d",false);
 a.playerStyle2D=bounded(getint(f,"Reproductor","style-2d",0),0,1);
 a.playerStyle3D=bounded(getint(f,"Reproductor","style-3d",0),0,2);
 a.playerBeatMode=bounded(getint(f,"Reproductor","beat-mode",0),0,1);
 a.playerBeatIndicator=getbool(f,"Reproductor","beat-visible",true);
 #define PINT(member,key,lo,hi) a.member=bounded(getint(f,"Reproductor",key,a.member),lo,hi)
 #define PBOOL(member,key) a.member=getbool(f,"Reproductor",key,a.member)
 PINT(player2DHeight,"2d-height",10,30);PINT(player2DWidth,"2d-width",50,110);
 PINT(player2DRadius,"2d-radius",0,16);PINT(player2DOpacity,"2d-opacity",10,100);
 PINT(player2DGlow,"2d-glow",0,100);PINT(player2DBrightness,"2d-brightness",20,150);
 PINT(player2DGuideSize,"2d-guide-size",1,4);PINT(player2DGuideOpacity,"2d-guide-opacity",0,100);
 PINT(player2DKeyOpacity,"2d-key-opacity",20,100);
 PINT(player2DBackgroundOpacity,"2d-background-opacity",10,100);
 PBOOL(player2DShadows,"2d-shadows");
 PINT(player3DHeight,"3d-height",10,30);PINT(player3DWidth,"3d-width",50,110);
 PINT(player3DNoteLength,"3d-note-length",40,200);
 PINT(player3DRadius,"3d-radius",0,16);PINT(player3DOpacity,"3d-opacity",10,100);
 PINT(player3DGlow,"3d-glow",0,100);PINT(player3DBrightness,"3d-brightness",20,150);
 PINT(player3DGuideSize,"3d-guide-size",1,4);PINT(player3DGuideOpacity,"3d-guide-opacity",0,100);
 PINT(player3DKeyOpacity,"3d-key-opacity",20,100);PBOOL(player3DShadows,"3d-shadows");
 PINT(player3DDepth,"3d-depth",0,100);PINT(player3DAngle,"3d-angle",0,65);
 PINT(player3DHorizon,"3d-horizon",10,60);PINT(player3DStageWidth,"3d-stage-width",40,120);
 PINT(player3DBackgroundOpacity,"3d-background-opacity",10,100);
 #undef PINT
 #undef PBOOL
 a.effectSpeed=bounded(getint(f,"Efectos3D","velocidad",a.effectSpeed),25,200);
 a.effectCount=bounded(getint(f,"Efectos3D","cantidad",a.effectCount),0,144);
 a.effectSpread=bounded(getint(f,"Efectos3D","dispersion",a.effectSpread),0,200);
 a.effectSize=bounded(getint(f,"Efectos3D","tamano",a.effectSize),25,220);
 a.effectPalette=bounded(getint(f,"Efectos3D","paleta",a.effectPalette),0,8);
 a.effectBirth=bounded(getint(f,"Efectos3D","nacimiento",a.effectBirth),0,1);
 a.effectWind=bounded(getint(f,"Efectos3D","viento",a.effectWind),0,250);
 a.effectLife=bounded(getint(f,"Efectos3D","vida",a.effectLife),25,250);
 a.effectTurbulence=bounded(getint(f,"Efectos3D","turbulencia",a.effectTurbulence),0,200);
 a.effectInitialVelocity=bounded(getint(f,"Efectos3D","velocidad-inicial",a.effectInitialVelocity),0,250);
 a.effectBloom=bounded(getint(f,"Efectos3D","bloom",a.effectBloom),0,200);
 a.effectOpacity=bounded(getint(f,"Efectos3D","opacidad",a.effectOpacity),0,100);
 a.effectBlend=bounded(getint(f,"Efectos3D","mezcla",a.effectBlend),0,1);
 a.effectRays=bounded(getint(f,"Efectos3D","rayos",a.effectRays),0,1);
 a.effectSmoke=bounded(getint(f,"Efectos3D","humo",a.effectSmoke),0,1);
 a.effectTrails=bounded(getint(f,"Efectos3D","estelas",a.effectTrails),0,1);
 a.effectExplosion=bounded(getint(f,"Efectos3D","explosion",a.effectExplosion),0,250);
 a.effectFlameHeight=bounded(getint(f,"Efectos3D","altura-llama",a.effectFlameHeight),20,130);
 a.scenePreset=StageScenes::ClampPreset(getint(f,"Escenarios3D","escenario",a.scenePreset));
 a.sceneLightIntensity=bounded(getint(f,"Escenarios3D","intensidad-luz",a.sceneLightIntensity),0,100);
 a.sceneLightColor=bounded(getint(f,"Escenarios3D","color-luz",a.sceneLightColor),0,6);
 a.sceneCameraMotion=bounded(getint(f,"Escenarios3D","movimiento-camara",a.sceneCameraMotion),0,100);
 a.scenePianistMotion=bounded(getint(f,"Escenarios3D","movimiento-pianista",a.scenePianistMotion),0,100);
 a.sceneYaw=bounded(getint(f,"Escenarios3D","camara-giro",a.sceneYaw),-180,180);
 a.scenePitch=bounded(getint(f,"Escenarios3D","camara-elevacion",a.scenePitch),5,78);
 a.sceneZoom=bounded(getint(f,"Escenarios3D","camara-distancia",a.sceneZoom),180,1250);
 a.sceneView=bounded(getint(f,"Escenarios3D","camara-vista",a.sceneView),0,8);
 a.sceneCameraFocus=bounded(getint(f,"Escenarios3D","camara-enfoque",a.sceneCameraFocus),0,2);
 a.playerNames=getbool(f,"Reproductor","names",true);
 a.playerGuides=getbool(f,"Reproductor","guides",true);
 a.playerFooter=getbool(f,"Reproductor","footer",true);
 a.playerMeter=getbool(f,"Reproductor","meter",true);
 a.playerReader=getbool(f,"Reproductor","reader",true);
 a.playerErrors=getbool(f,"Reproductor","errors",true);
 a.playerChord=getbool(f,"Reproductor","chord",true);
 a.playerKey=getbool(f,"Reproductor","key",true);
 a.playerBarNumbers=getbool(f,"Reproductor","bar-numbers",true);
 a.practiceHands=bounded(getint(f,"Reproductor","practice-hands",0),0,2);
 a.notationVisible=getbool(f,"Reproductor","notation-visible",true);
 a.fallingNotesVisible=getbool(f,"Reproductor","falling-notes-visible",true);
 a.midiLightGuides=getbool(f,"MIDI","key-lights",false);
 a.midiLightChannel=bounded(getint(f,"MIDI","key-lights-channel",16),1,16);
 a.loopRestartErrors=bounded(getint(f,"Reproductor","loop-restart-errors",0),0,20);
 a.notationExpanded=getbool(f,"Reproductor","notation-expanded",false);
 a.notationZoom=bounded(getint(f,"Reproductor","notation-zoom",100),75,200);
 a.notationQuantize=bounded(getint(f,"Reproductor","notation-quantize",0),0,2);
 a.handSplitPoint=bounded(getint(f,"Reproductor","hand-split-point",60),48,84);
 a.handEditEnabled=getbool(f,"Reproductor","hand-edit-enabled",false);
 a.handEditorTool=bounded(getint(f,"Reproductor","hand-editor-tool",0),0,1);
 a.fingerToAssign=bounded(getint(f,"Reproductor","finger-to-assign",1),1,5);
 g_key_file_free(f);
 return a;
}
bool Save(const AdvancedSettings& s,std::string& error){
 GKeyFile* f=g_key_file_new();
 auto I=[&](const char*g,const char*k,int v){g_key_file_set_integer(f,g,k,v);};
 auto B=[&](const char*g,const char*k,bool v){g_key_file_set_boolean(f,g,k,v);};
 auto S=[&](const char*g,const char*k,const std::string& v){g_key_file_set_string(f,g,k,v.c_str());};
 I("Pantalla","fps",s.fps);I("Pantalla","width",s.windowWidth);I("Pantalla","height",s.windowHeight);
 B("Pantalla","fullscreen",s.fullscreen);B("Pantalla","show-fps",s.showFps);I("Pantalla","scale",s.uiScale);I("Pantalla","theme",s.theme);
 I("Sonido","master-volume",s.masterVolume);I("Sonido","sample-rate",s.sampleRate);I("Sonido","buffer",s.bufferSize);
 I("Sonido","fantom-usb-boost-db",bounded(s.fantomUsbBoostDb,0,24));
 S("Sonido","driver",s.audioDriver);S("Sonido","device",s.audioDevice);
 S("FluidSynth","soundfont",s.soundfont);I("FluidSynth","gain",s.gain);I("FluidSynth","polyphony",s.polyphony);
 B("FluidSynth","reverb",s.reverb);B("FluidSynth","chorus",s.chorus);
 I("FluidSynth","reverb-size",s.reverbSize);
 I("FluidSynth","reverb-damp",s.reverbDamp);
 I("FluidSynth","reverb-width",s.reverbWidth);
 I("FluidSynth","reverb-level",s.reverbLevel);
 I("FluidSynth","chorus-voices",s.chorusVoices);
 I("FluidSynth","chorus-level",s.chorusLevel);
 I("FluidSynth","chorus-rate",s.chorusRate);
 I("FluidSynth","chorus-depth",s.chorusDepth);
 std::vector<const gchar*> extras;
 for(const auto& path:s.extraSoundfonts)extras.push_back(path.c_str());
 g_key_file_set_string_list(f,"FluidSynth","extra-soundfonts",
    extras.empty()?nullptr:extras.data(),extras.size());
 S("MIDI","input",s.midiInput);S("MIDI","output",s.midiOutput);B("MIDI","thru",s.midiThru);I("MIDI","channel",s.midiChannel);
 S("Biblioteca","path",s.libraryPath);
 B("Biblioteca","remember-last",s.rememberFolder);B("Biblioteca","alphabetic",s.alphabeticOrder);
 I("Biblioteca","recent-limit",s.recentLimit);I("Biblioteca","recent-sort",s.recentSort);
 I("Reproductor","piano-percent",s.playerPianoPct);
 I("Reproductor","color-theme",s.playerTheme);
 I("Reproductor","note-ms",s.playerNoteDurationMs);
 B("Reproductor","view-3d",s.player3D);
 I("Reproductor","style-2d",s.playerStyle2D);
 I("Reproductor","style-3d",s.playerStyle3D);
 I("Reproductor","beat-mode",s.playerBeatMode);
 B("Reproductor","beat-visible",s.playerBeatIndicator);
 #define PINT(member,key) I("Reproductor",key,s.member)
 #define PBOOL(member,key) B("Reproductor",key,s.member)
 PINT(player2DHeight,"2d-height");PINT(player2DWidth,"2d-width");
 PINT(player2DRadius,"2d-radius");PINT(player2DOpacity,"2d-opacity");
 PINT(player2DGlow,"2d-glow");PINT(player2DBrightness,"2d-brightness");
 PINT(player2DGuideSize,"2d-guide-size");PINT(player2DGuideOpacity,"2d-guide-opacity");
 PINT(player2DKeyOpacity,"2d-key-opacity");
 PINT(player2DBackgroundOpacity,"2d-background-opacity");PBOOL(player2DShadows,"2d-shadows");
 PINT(player3DHeight,"3d-height");PINT(player3DWidth,"3d-width");
 PINT(player3DNoteLength,"3d-note-length");
 PINT(player3DRadius,"3d-radius");PINT(player3DOpacity,"3d-opacity");
 PINT(player3DGlow,"3d-glow");PINT(player3DBrightness,"3d-brightness");
 PINT(player3DGuideSize,"3d-guide-size");PINT(player3DGuideOpacity,"3d-guide-opacity");
 PINT(player3DKeyOpacity,"3d-key-opacity");PBOOL(player3DShadows,"3d-shadows");
 PINT(player3DDepth,"3d-depth");PINT(player3DAngle,"3d-angle");
 PINT(player3DHorizon,"3d-horizon");PINT(player3DStageWidth,"3d-stage-width");
 PINT(player3DBackgroundOpacity,"3d-background-opacity");
 #undef PINT
 #undef PBOOL
 I("Efectos3D","velocidad",s.effectSpeed);
 I("Efectos3D","cantidad",s.effectCount);
 I("Efectos3D","dispersion",s.effectSpread);
 I("Efectos3D","tamano",s.effectSize);
 I("Efectos3D","paleta",s.effectPalette);
 I("Efectos3D","nacimiento",s.effectBirth);
 I("Efectos3D","viento",s.effectWind);
 I("Efectos3D","vida",s.effectLife);
 I("Efectos3D","turbulencia",s.effectTurbulence);
 I("Efectos3D","velocidad-inicial",s.effectInitialVelocity);
 I("Efectos3D","bloom",s.effectBloom);
 I("Efectos3D","opacidad",s.effectOpacity);
 I("Efectos3D","mezcla",s.effectBlend);
 I("Efectos3D","rayos",s.effectRays);
 I("Efectos3D","humo",s.effectSmoke);
 I("Efectos3D","estelas",s.effectTrails);
 I("Efectos3D","explosion",s.effectExplosion);
 I("Efectos3D","altura-llama",s.effectFlameHeight);
 I("Escenarios3D","escenario",s.scenePreset);
 I("Escenarios3D","intensidad-luz",s.sceneLightIntensity);
 I("Escenarios3D","color-luz",s.sceneLightColor);
 I("Escenarios3D","movimiento-camara",s.sceneCameraMotion);
 I("Escenarios3D","movimiento-pianista",s.scenePianistMotion);
 I("Escenarios3D","camara-giro",s.sceneYaw);
 I("Escenarios3D","camara-elevacion",s.scenePitch);
 I("Escenarios3D","camara-distancia",s.sceneZoom);
 I("Escenarios3D","camara-vista",s.sceneView);
 I("Escenarios3D","camara-enfoque",s.sceneCameraFocus);
 B("Reproductor","names",s.playerNames);B("Reproductor","guides",s.playerGuides);
 B("Reproductor","footer",s.playerFooter);B("Reproductor","meter",s.playerMeter);
 B("Reproductor","reader",s.playerReader);B("Reproductor","errors",s.playerErrors);
 B("Reproductor","chord",s.playerChord);B("Reproductor","key",s.playerKey);
 B("Reproductor","bar-numbers",s.playerBarNumbers);
 I("Reproductor","practice-hands",s.practiceHands);
 B("Reproductor","notation-visible",s.notationVisible);
 B("Reproductor","falling-notes-visible",s.fallingNotesVisible);
 B("MIDI","key-lights",s.midiLightGuides);
 I("MIDI","key-lights-channel",s.midiLightChannel);
 I("Reproductor","loop-restart-errors",s.loopRestartErrors);
 B("Reproductor","notation-expanded",s.notationExpanded);
 I("Reproductor","notation-zoom",s.notationZoom);
 I("Reproductor","notation-quantize",s.notationQuantize);
 I("Reproductor","hand-split-point",s.handSplitPoint);
 B("Reproductor","hand-edit-enabled",s.handEditEnabled);
 I("Reproductor","hand-editor-tool",s.handEditorTool);
 I("Reproductor","finger-to-assign",s.fingerToAssign);
 B("Controles","multiple",s.multipleSelect);I("Controles","hover-ms",s.dragHoverMs);
 I("Controles","double-ms",s.doubleClickMs);B("Avanzado","logs",s.diagnostics);
 const std::string filename=Filename();
 const std::string dir=std::string(g_get_user_config_dir())+"/linthesia";
 if(g_mkdir_with_parents(dir.c_str(),0700)!=0){
  error="No se puede crear "+dir;g_key_file_free(f);return false;
 }
 gsize length=0;
 gchar* bytes=g_key_file_to_data(f,&length,nullptr);
 const std::string temp=filename+".tmp";
 GError* e=nullptr;
 const bool ok=g_file_set_contents(temp.c_str(),bytes,length,&e);
 g_free(bytes);g_key_file_free(f);
 if(!ok){
  error=e?e->message:"Error al guardar ajustes";
  if(e)g_error_free(e);
  return false;
 }
 chmod(temp.c_str(),0600);
 if(rename(temp.c_str(),filename.c_str())!=0){
  error="No se pudo reemplazar el archivo de ajustes";
  unlink(temp.c_str());return false;
 }
 return true;
}
}
