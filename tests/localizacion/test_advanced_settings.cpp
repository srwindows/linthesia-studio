#include "AdvancedSettings.h"
#include <glib.h>
#include <cassert>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <unistd.h>
int main(){
 char scratch[]="/tmp/linthesia-config-test-XXXXXX";
 assert(mkdtemp(scratch));
 g_setenv("XDG_CONFIG_HOME",scratch,TRUE);
 AdvancedSettings d=AdvancedSettingsIO::Load();
 assert(d.sampleRate==48000);
 assert(d.multipleSelect);
 d.effectSpeed=70;d.effectCount=120;d.effectSpread=150;
 d.effectSize=65;d.effectPalette=6;d.effectBirth=1;
 d.scenePreset=6;d.sceneLightIntensity=80;d.sceneLightColor=3;
 d.sceneCameraMotion=60;d.scenePianistMotion=45;
 d.player3DNoteLength=70;
 d.practiceHands=2;d.notationVisible=false;d.fallingNotesVisible=false;d.loopRestartErrors=4;d.midiLightGuides=true;d.midiLightChannel=14;d.notationQuantize=2;d.notationExpanded=true;d.notationZoom=175;d.handSplitPoint=65;d.handEditEnabled=true;d.handEditorTool=1;d.fingerToAssign=5;
 d.fantomUsbBoostDb=9;
 d.playerStyle3D=2;d.fps=60;d.theme=2;d.uiScale=125;d.polyphony=512;d.audioDriver="alsa";d.audioDevice="default";
 d.libraryPath="/tmp/midi-datos";d.midiOutput="FluidSynth interno";
 d.dragHoverMs=1300;d.alphabeticOrder=true;d.midiChannel=9;d.midiThru=true;
 d.reverb=false;d.chorus=true;d.reverbSize=80;d.reverbDamp=40;
 d.reverbWidth=90;d.reverbLevel=30;
 d.chorusVoices=5;d.chorusLevel=70;d.chorusRate=12;d.chorusDepth=14;
 d.extraSoundfonts={"/mnt/externo/piano.sf2","/mnt/externo/strings.sf3"};
 std::string error;
 assert(AdvancedSettingsIO::Save(d,error));
 auto after=AdvancedSettingsIO::Load();
 assert(after.playerStyle3D==2);
 assert(after.practiceHands==2 && !after.notationVisible && after.notationQuantize==2);
 assert(!after.fallingNotesVisible);
 assert(after.midiLightGuides && after.midiLightChannel==14);
 assert(after.loopRestartErrors==4);
 assert(after.notationExpanded && after.notationZoom==175);
 assert(after.handSplitPoint==65 && after.handEditEnabled);
 assert(after.handEditorTool==1 && after.fingerToAssign==5);
 assert(after.effectSpeed==70 && after.effectCount==120 &&
        after.effectSpread==150 && after.effectSize==65 &&
        after.effectPalette==6 && after.effectBirth==1 && after.player3DNoteLength==70);
 assert(after.scenePreset==6 && after.sceneLightIntensity==80 &&
        after.sceneLightColor==3 && after.sceneCameraMotion==60 &&
        after.scenePianistMotion==45);
 assert(after.fantomUsbBoostDb==9);
 assert(after.fps==60 && after.theme==2 && after.uiScale==125 && after.polyphony==512 && after.audioDriver=="alsa");
 assert(after.libraryPath=="/tmp/midi-datos" && after.midiOutput=="FluidSynth interno");
 assert(after.dragHoverMs==1300 && after.alphabeticOrder);
 assert(after.midiChannel==9 && after.midiThru);
 assert(!after.reverb && after.chorus);
 assert(after.reverbSize==80 && after.reverbDamp==40);
 assert(after.reverbWidth==90 && after.reverbLevel==30);
 assert(after.chorusVoices==5 && after.chorusLevel==70);
 assert(after.chorusRate==12 && after.chorusDepth==14);
 assert(after.extraSoundfonts.size()==2);
 assert(after.extraSoundfonts[0]=="/mnt/externo/piano.sf2");
 assert(after.extraSoundfonts[1]=="/mnt/externo/strings.sf3");
 after.effectSpeed=-10;after.effectCount=900;
 after.effectSpread=999;after.effectSize=999;after.effectPalette=100;
 after.effectBirth=55;after.player3DNoteLength=1000;
 after.scenePreset=500;after.sceneLightIntensity=700;after.sceneLightColor=90;
 after.fantomUsbBoostDb=999;
 after.practiceHands=99;after.notationQuantize=99;after.loopRestartErrors=1000;after.midiLightChannel=200;after.notationZoom=900;
 after.handSplitPoint=999;after.handEditorTool=22;after.fingerToAssign=45;
 after.sceneCameraMotion=-40;after.scenePianistMotion=120;
 after.dragHoverMs=200000;after.polyphony=99999;after.midiChannel=99;after.uiScale=1000;after.theme=99;
 after.reverbSize=999;after.reverbDamp=-10;after.reverbLevel=999;
 after.chorusRate=0;after.chorusDepth=100;after.chorusVoices=100;
 assert(AdvancedSettingsIO::Save(after,error));
 auto bounded=AdvancedSettingsIO::Load();
 assert(bounded.effectSpeed==25 && bounded.effectCount==144 &&
        bounded.effectSpread==200 && bounded.effectSize==220 &&
        bounded.effectPalette==8 && bounded.effectBirth==1 && bounded.player3DNoteLength==200);
 assert(bounded.scenePreset==12 && bounded.sceneLightIntensity==100 &&
        bounded.sceneLightColor==6 && bounded.sceneCameraMotion==0 &&
        bounded.scenePianistMotion==100);
 assert(bounded.fantomUsbBoostDb==24);
 assert(bounded.dragHoverMs==1600 && bounded.polyphony==1024 && bounded.midiChannel==16);
 assert(bounded.uiScale==150 && bounded.theme==2);
 assert(bounded.practiceHands==2 && bounded.notationQuantize==2);
 assert(bounded.loopRestartErrors==20);
 assert(bounded.midiLightChannel==16);
 assert(bounded.notationZoom==200 && bounded.notationExpanded);
 assert(bounded.handSplitPoint==84);
 assert(bounded.handEditorTool==1 && bounded.fingerToAssign==5);
 assert(bounded.reverbSize==100 && bounded.reverbDamp==0);
 assert(bounded.reverbLevel==100 && bounded.chorusRate==1);
 assert(bounded.chorusDepth==21 && bounded.chorusVoices==16);
 assert(access(AdvancedSettingsIO::Filename().c_str(),R_OK)==0);
 puts("PASS: ajustes persistentes, recarga y validación de límites.");
 return 0;
}
