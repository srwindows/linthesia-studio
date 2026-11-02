from pathlib import Path
import sys
root=Path(sys.argv[1]);s=(root/'src/PlayingState.cpp').read_text()
assert 'm_settings_dragging=true;' in s
assert 'mouse.x-m_settings_drag_dx' in s
assert 'HandlePlayerSettings(mouse))' in s
assert 'm_settings_open && IsKeyPressed(KeyEscape)' in s
assert 'return;\n  }\n  if(m_show_help' not in s
start=s.index('void PlayingState::OpenPlayerSettings()')
end=s.index('void PlayingState::SavePlayerSettings()',start)
assert 'm_paused=true' not in s[start:end]
assert 'm_midi_out->Reset()' not in s[start:end]
start=s.index('void PlayingState::DrawPlayerSettings(')
end=s.index('void PlayingState::DrawLoopEditor(',start)
assert 'renderer.DrawQuad(0,95' not in s[start:end]
assert 'm_settings_feedback' in s[start:end]
assert 'Float' in s[start:end]
print('OK: movable, live-preview, non-pausing settings with Save/Cancel')
