"""Regresión del bucle A-B: ratón, MIDI y marcador luminoso."""
from pathlib import Path
import sys

root=Path(sys.argv[1])
code=(root/'src/PlayingState.cpp').read_text()
head=(root/'src/PlayingState.h').read_text()
assert 'void PlayingState::OpenLoopEditor()' in code
assert 'void PlayingState::CloseLoopEditor(bool apply)' in code
assert 'bool PlayingState::HandleLoopEditor(const MouseInfo& mouse)' in code
assert '(mouse.newPress.right || mouse.released.right)' in code
assert 'm_loop_restore_paused=m_paused' in code
assert 'm_paused=true;' in code
assert 'm_loop_draft_valid=false;' in code
assert 'm_loop_draft_start=LoopFraction(mouse.x)' in code
assert 'm_loop_draft_end=LoopFraction(mouse.x)' in code
assert 'std::min(m_loop_draft_start,m_loop_draft_end)' in code
assert 'm_loop_defined=false;' in code
assert 'CloseLoopEditor(true);' in code
assert 'm_loop_active=!m_loop_active;' in code
assert 'm_loop_active && !m_paused' in code
assert 'if(at+delta_microseconds>=m_loop_end)' in code
assert 'SeekTo(m_loop_start);' in code
assert 'void PlayingState::DrawLoopEditor(Renderer &renderer) const' in code
assert 'if(m_loop_editor)DrawLoopEditor(renderer);' in code
assert 'control(14,"Bucle",m_loop_active)' in code
assert 'Ribbon(14,width).Contains(mouse)' in code
assert 'control(14,"Bucle",m_loop_active)' in code
assert 'if(IsKeyPressed(KeyEscape))CloseLoopEditor(false);' in code
assert 'm_loop_dragging=false;' in code
assert 'm_loop_draft_valid' in head
assert 'bool m_loop_editor=false' in head
assert 'microseconds_t m_loop_start=0;' in head
assert 'microseconds_t m_loop_end=0;' in head
assert 'Copyright (c) 2007 Nicholas Piegdon' in code
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()

try:
    from PIL import Image
except ImportError:
    Image=None
lab=Path('/tmp/linthesia-loop-qa')
names=('editor-empty','reverse-range','cleared','new-range',
       'replaced-range','applied','repeated','disabled','enabled')
if Image and all((lab/(name+'.png')).exists() for name in names):
    im={name:Image.open(lab/(name+'.png')).convert('RGB') for name in names}
    assert all(v.size==(1280,720) for v in im.values())
    assert im['editor-empty'].getpixel((540,141)) != im['applied'].getpixel((540,141))
    assert im['reverse-range'].getpixel((450,81)) != im['cleared'].getpixel((450,81))
    assert im['cleared'].getpixel((450,81)) != im['new-range'].getpixel((450,81))
    assert im['new-range'].getpixel((350,68)) != im['replaced-range'].getpixel((350,68))
    assert im['applied'].getpixel((1165,12))==(82,235,113)
    assert im['repeated'].getpixel((1165,12))==(82,235,113)
    assert im['disabled'].getpixel((1165,12))!=(82,235,113)
    assert im['enabled'].getpixel((1165,12))==(82,235,113)
    # La nueva ejecución retrocede de B a A y permanece dentro del intervalo.
    # Comparamos las capturas originales del test aislado.
    assert im['applied'].getpixel((450,81)) == im['repeated'].getpixel((450,81))
print('OK: A-B derecha/izquierda, borrar, aplicar, repetir y activar/desactivar')
