#ifndef LINTHESIA_ADVANCED_SETTINGS_H
#define LINTHESIA_ADVANCED_SETTINGS_H
#include <string>
#include <vector>
struct AdvancedSettings {
 int fps=300;
 int windowWidth=1280,windowHeight=720;
 bool fullscreen=false, showFps=false;
 int uiScale=100;
 int theme=0; // 0=Clásico,1=Azul metálico,2=Alto contraste
 int masterVolume=70, sampleRate=48000, bufferSize=512;
 // FANTOM USB playback boost; limited with stereo look-ahead LADSPA.
 // It is never applied to non-FANTOM outputs or external MIDI devices.
 int fantomUsbBoostDb=0;
 std::string audioDevice="default";
 std::string libraryPath;
 std::string audioDriver="pulseaudio"; // PipeWire compatibility (pipewire-pulse)
 std::string soundfont="/usr/share/sounds/sf2/default-GM.sf2";
 bool reverb=true, chorus=true;
 // FluidSynth effect parameters (normalized percentages except stated units).
 int reverbSize=61, reverbDamp=23, reverbWidth=76, reverbLevel=57;
 int chorusVoices=3,chorusLevel=20,chorusRate=3,chorusDepth=8; // Rate in 0.1 Hz
 std::vector<std::string> extraSoundfonts; // Registered paths; no file copies or deletion.
 int polyphony=256, gain=70;
 std::string midiInput, midiOutput;
 bool midiThru=false;
 int midiChannel=0; // 0=Omni; 1..16 fixed
 bool rememberFolder=true, alphabeticOrder=false;
 int recentLimit=20, recentSort=1; // 0=alfabético,1=últimas,2=reproducciones,3=puntos
 bool multipleSelect=true;
 int dragHoverMs=800, doubleClickMs=400;
 bool diagnostics=false;
 // Preferencias reales del reproductor (persisten entre sesiones).
 int playerPianoPct=16, playerTheme=0,playerNoteDurationMs=3500;
 // Estilos: 2D 0 clásico / 1 neón; 3D 0 clásico / 1 moderno.
 int playerStyle2D=0,playerStyle3D=0,playerBeatMode=0;
 // Ajustes de representación independientes y utilizables por el motor.
 int player2DHeight=16,player2DWidth=90,player2DRadius=3;
 int player2DOpacity=100,player2DGlow=20,player2DBrightness=100;
 int player2DGuideSize=1,player2DGuideOpacity=85,player2DKeyOpacity=100;
 int player2DBackgroundOpacity=100;
 bool player2DShadows=true;
 int player3DHeight=15,player3DWidth=108,player3DRadius=4;
 int player3DNoteLength=100; // 3D body only; MIDI sustain remains unchanged.
 int player3DOpacity=100,player3DGlow=18,player3DBrightness=108;
 int player3DGuideSize=1,player3DGuideOpacity=28,player3DKeyOpacity=100;
 int player3DDepth=38,player3DAngle=20,player3DHorizon=10;
 int player3DStageWidth=116,player3DBackgroundOpacity=92;
 bool player3DShadows=true;
 // Independent cinematic effect controls: speed, count, spread, particle size,
 // and palette. No MIDI events, piano notes, or song tempo are affected.
 int effectSpeed=100,effectCount=72,effectSpread=100,effectSize=100;
 int effectPalette=0; // 0 = original channel colors; 1-8 = built-in palettes.
 int effectBirth=1; // 0 = clásico, 1 = explosión superior descendente.
 // SeeMusic visual references: our own independent and persistent GPU effects.
 int effectWind=100;
 int effectLife=100;
 int effectTurbulence=85;
 int effectInitialVelocity=100;
 int effectBloom=115;
 int effectOpacity=70;
 int effectBlend=0;
 int effectRays=1;
 int effectSmoke=1;
 int effectTrails=1;
 int effectExplosion=130;
 int effectFlameHeight=100;
 int scenePreset=-1; // -1 without stage; internal 6..12 -> user-visible 1..7.
 int sceneLightIntensity=55,sceneLightColor=0;
 int sceneCameraMotion=40,scenePianistMotion=55;
 int sceneYaw=92,scenePitch=26,sceneZoom=405,sceneView=6;
 int sceneCameraFocus=0; // 0: escena, 1: pianista, 2: manos/teclado cenital
 bool playerBeatIndicator=true;
 bool player3D=false;
 bool playerNames=true,playerGuides=true,playerFooter=true;
 bool playerMeter=true,playerReader=true,playerErrors=true;
 bool playerChord=true,playerKey=true,playerBarNumbers=true;
 // Piano learning only: heuristic splitting and visual notation, not MIDI edits.
 int practiceHands=0; // 0 both, 1 right, 2 left
 bool notationVisible=true;
 bool fallingNotesVisible=true; // F5: hide falling notes, retain 88-key piano
 bool midiLightGuides=false; // Explicit opt-in only; never auto-enable
 int midiLightChannel=16; // 1..16, channel configured by supported keyboard
 int loopRestartErrors=0; // 0 disabled; 1..20 errors in one A-B pass
 bool notationExpanded=false; // expanded score overlay, reversibly hides stage
 int notationZoom=100;        // 75..200 visual zoom, no MIDI changes
 int notationQuantize=0; // 0 original, 1 eighth, 2 sixteenth (visual only)
 int handSplitPoint=60; // global starting split, adjustable C3..C6
 bool handEditEnabled=false; // manual corrections in score (saved per song)
 int handEditorTool=0; // 0: asignar mano, 1: digitación
 int fingerToAssign=1; // 1 pulgar, ... 5 meñique
};
namespace AdvancedSettingsIO {
 AdvancedSettings Load();
 bool Save(const AdvancedSettings &value,std::string &error);
 std::string Filename();
}
#endif
