from pathlib import Path
import sys
r=Path(sys.argv[1])
p=(r/'src/PlayingState.cpp').read_text()
s=(r/'src/AdvancedSettings.cpp').read_text()
for needle in (
 'm_midi_preferences.loopRestartErrors>0',
 'LoopErrorPolicy::ShouldRestart(',
 'm_loop_error_baseline=m_state.stats.stray_notes',
 'SeekTo(m_loop_start);',
 'Reiniciar A-B tras N errores',
 'saved.loopRestartErrors!=m_midi_preferences.loopRestartErrors'):
 assert needle in p,needle
assert 'loop-restart-errors' in s
assert 'bounded(getint(f,"Reproductor","loop-restart-errors",0),0,20)' in s
print('OK: Synthesia-style adjustable error threshold wired to A-B after MIDI input')
