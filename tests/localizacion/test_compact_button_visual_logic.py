"""The compact controls use real Linthesia menu chrome and ButtonState."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
s=(root/'src/TrackSelectionState.cpp').read_text()
h=(root/'src/TrackSelectionState.h').read_text()
button=(root/'src/MenuLayout.h').read_text()
original=(root/'src/MenuLayout.cpp').read_text()
start=original.index('void DrawCompactButton(')
style=original[start:original.index('\n  }',start)]
assert 'void Update(const MouseInfo &mouse)' in button
assert 'depressed = hovering && mouse.held.left;' in button
assert 'hit = hovering && mouse.released.left;' in button
assert 'button.depressed ?' in original
assert 'button.y+(button.depressed?1:0)' in original
assert 'void DrawCompactButton(' in button
assert 'DrawCompactMenuButton' not in s
for token in (
    'renderer.DrawStretchedTga(chrome',
    'button.w-2*edge',
    '150,0,3,46',
    '162,0,edge,46',
    'button.depressed ? 173',
    'button.hovering ? 255 : 224',
    'y+button.h-6',
    '"FreeSerif.ttf"',
    'bool selected', 'bool enabled',
):
    assert token in style,token
for token in (
    'Layout::DrawCompactButton(renderer,m_detect_button,GetTexture(ButtonSongs)',
    'Layout::DrawCompactButton(renderer,m_hand_buttons[index],GetTexture(ButtonSongs)',
    'Layout::DrawCompactButton(renderer,m_modal_buttons[id],GetTexture(ButtonSongs)',
):
    assert token in s,token
for token in ('ButtonState m_hand_buttons[3];','ButtonState m_detect_button;',
              'ButtonState m_modal_buttons[11];'):
    assert token in h,token
for token in ('m_detect_button.Update(handMouse);',
              'm_hand_buttons[option].Update(handMouse);',
              'm_modal_buttons[id].Update(mouse);',
              'if(m_modal_buttons[id].hit)',
              'if(m_detect_button.hit)',
              'if(m_hand_buttons[option].hit)'):
    assert token in s,token
assert 'if((id==5||id==6)&&m_detector_automatic)return;' in s
assert 'if(id==8&&!m_detector_preview_ready)return;' in s
assert 'm_detector_open=false;return;' in s
print("OK: 14 compact controls use the same textured chrome, serif label, and press/hover/release mechanics as other menu controls")
