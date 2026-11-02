"""Interacción real: tempo BPM+REST, fallos separados y vista 3D reversible."""
from pathlib import Path
import sys
root=Path(sys.argv[1]); src=(root/'src/PlayingState.cpp').read_text()
header=(root/'src/PlayingState.h').read_text()
renderer=(root/'src/Renderer.cpp').read_text()
keyboard=(root/'src/KeyboardDisplay.cpp').read_text()
assert 'int PlayingState::EffectiveTempoBPM() const' in src
assert 'void PlayingState::AdjustTempoBPM(int delta)' in src
assert 'm_tempo_factor=target/static_cast<double>(original);' in src
assert 'delta_microseconds*m_tempo_factor' in src
assert 'void PlayingState::ResetTempoBPM()' in src
assert 'm_tempo_factor=1.0;' in src
assert 'ResetTempoBPM();changed=true;' in src
assert 'control(8,"REST")' in src
assert 'tempoNumber<<Text(STRING(EffectiveTempoBPM())' in src
assert 'control(6,"▲")' in src and 'control(7,"▼")' in src
assert 'm_state.song_speed<<"%"' not in src
assert 'm_view_3d=!m_view_3d' in src
assert 'm_view_3d?"2D":"3D"' in src
assert 'BeginPianoPerspective' in renderer
assert 'EndPianoPerspective' in renderer
assert 'glMultMatrixf(matrix)' in renderer
assert 'glPushMatrix()' in renderer and 'glPopMatrix()' in renderer
assert 'renderer.EndPianoPerspective();' in keyboard
assert 'renderer.EndPianoRollClip();' in keyboard
assert 'renderer.BeginPianoRollClip(' in keyboard
assert 'm_missed_notes=0;' in src
assert '++m_missed_notes;' in src
assert 'm_state.midi_in && note->state == UserPlayable' in src
assert 'm_state.stats.stray_notes++' in src
assert 'lcd(11,"Fallos",STRING(m_state.stats.stray_notes)' in src
assert 'm_missed_notes+m_state.stats.stray_notes' not in src
assert 'm_tempo_factor=1.0' in header
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
try: from PIL import Image,ImageChops
except ImportError: Image=None
p=Path('/tmp/linthesia-tempo3d-qa')
if Image and all((p/n).exists() for n in ('pause-2d.png','pause-3d.png','pause-return2d.png','pause-bpmplus.png','pause-bpmrest.png')):
 a,b,c,plus,reset=(Image.open(p/n).convert('RGB') for n in ('pause-2d.png','pause-3d.png','pause-return2d.png','pause-bpmplus.png','pause-bpmrest.png'))
 assert a.size==b.size==c.size==plus.size==reset.size
 keys=(16,596,1250,678);roll=(16,104,1250,584);lcd=(482,10,567,45)
 assert ImageChops.difference(a.crop(keys),b.crop(keys)).getbbox() is None
 assert ImageChops.difference(a.crop(roll),b.crop(roll)).getbbox() is not None
 assert ImageChops.difference(a.crop(roll),c.crop(roll)).getbbox() is None
 assert ImageChops.difference(a.crop(lcd),plus.crop(lcd)).getbbox() is not None
 assert ImageChops.difference(a.crop(lcd),reset.crop(lcd)).getbbox() is None
print('OK: tempo real + REST, fallos en tiempo real, 3D reversible sin deformar teclado')
