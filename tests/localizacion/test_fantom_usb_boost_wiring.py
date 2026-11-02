from pathlib import Path
import sys
r=Path(sys.argv[1])
config=(r/'src/AdvancedSettings.cpp').read_text()
engine=(r/'src/FluidEngine.cpp').read_text()
screen=(r/'src/SettingsPanel.cpp').read_text()
checks={
 'FANTOM-specific detection':'a.audioDevice.find("Roland_FANTOM")',
 '0 dB headroom protection':'const bool useLimiter=useBoost ||',
 'PCM when no boost':'(fantomOutput && a.audioDriver=="pulseaudio")',
 'booster opt-in':'const bool useBoost=boost>0 && fantomOutput;',
 'LADSPA activation':'fluid_settings_setint(settings,"synth.ladspa.active",1)',
 'look-ahead limiter':'fast_lookahead_limiter_1913.so',
 'gain control':'"Input gain"',
 'output peak protection':'"Limit",-1.5f',
 'stereo ports': '"Output 2","Main:R"',
 'check activation':'fluid_ladspa_activate(fx)==FLUID_OK',
 'error fallback': 'Se conserva la salida anterior.',
 'no permanent MIDI alteration':'fluid_synth_noteon(synth,ch,event.NoteNumber(),event.NoteVelocity());'
}
for name,token in checks.items():
 assert token in engine,name
assert 'fantom-usb-boost-db' in config
assert '"Refuerzo USB FANTOM con limitador"' in screen
assert '"Desactivado","+6 dB","+12 dB","+18 dB","+24 dB"' in screen
assert 'a.fantomUsbBoostDb!=m_saved_settings.fantomUsbBoostDb' in screen
print('OK: USB FANTOM only, LADSPA stereo limiter, fail closed, persistent GUI control')
