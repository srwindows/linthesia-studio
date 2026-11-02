from pathlib import Path
import sys
r=Path(sys.argv[1])
p=(r/'src/PlayingState.cpp').read_text()
h=(r/'src/PlayingState.h').read_text()
cfg=(r/'src/AdvancedSettings.cpp').read_text()
for x in ('ClearLightGuides();','UpdateLightGuides();',
          'm_midi_preferences.midiLightGuides',
          'm_state.midi_in && m_state.midi_out',
          'm_sent_light_notes=std::move(upcoming)',
          'm_state.midi_out->GetDeviceDescription().client>=0',
          'name.find("FANTOM")==std::string::npos',
          'uint8_t(pitch),uint8_t(1)',
          'Luces MIDI estándar (requiere teclado compatible)',
          'Canal de luces MIDI (1-16)'):
 assert x in p,x
assert 'm_sent_light_notes' in h
assert 'key-lights' in cfg and 'key-lights-channel' in cfg
assert 'getbool(f,"MIDI","key-lights",false)' in cfg
print('OK standard MIDI key light guidance is opt-in; off for FluidSynth and FANTOM; note-offs on exit/seek')
