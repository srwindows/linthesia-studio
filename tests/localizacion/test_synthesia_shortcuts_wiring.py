from pathlib import Path
import sys
r=Path(sys.argv[1])
main=(r/'src/main.cpp').read_text()
keyboard=(r/'src/KeyboardDisplay.cpp').read_text()
player=(r/'src/PlayingState.cpp').read_text()
settings=(r/'src/AdvancedSettings.cpp').read_text()
header=(r/'src/GameState.h').read_text()
for key in ('F2','F3','F5','BACKSPACE'):
 assert f'SDLK_{key}' in main,key
for key in ('KeyF2','KeyF3','KeyF5','KeyBackspace'):
 assert key in header and key in player,key
assert 'if(showFallingNotes){' in keyboard
assert '} // showFallingNotes' in keyboard
# Distinguish MIDI playback from visualization: transport and keyboard
# remain live, only the falling-note passes are gated.
assert keyboard.index('if(showFallingNotes){') < keyboard.index('DrawNotePass(')
assert keyboard.index('} // showFallingNotes') < keyboard.index('const int ActualKeyboardWidth')
assert 'm_midi_preferences.fallingNotesVisible' in player
assert 'm_midi_preferences.notationVisible=!m_midi_preferences.notationVisible' in player
assert 'm_midi_preferences.notationZoom=' in player
assert 'SeekTo(m_loop_start)' in player
assert 'falling-notes-visible' in settings
print('OK: Synthesia-like F2/F3/F5/Backspace mapped to independent Linthesia view controls')
