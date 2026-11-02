"""Retrocompatibilidad de portada: conserva logo, pie y Ajustes, con cinco tarjetas editables.

La especificación nueva del 30-10-2026 sustituye la antigua portada sólo lectura.
"""
from pathlib import Path
import sys
r=Path(sys.argv[1])
s=(r/'src/TitleState.cpp').read_text()
draw=s.split('void TitleState::Draw(Renderer &renderer) const {',1)[1]
up=s.split('void TitleState::Update() {',1)[1].split('void TitleState::PlayDevicePreview(',1)[0]
for k in ('GetTexture(TitleLogo)','GetTexture(GameMusicThemes)',
          'Layout::DrawButton(renderer, m_songs_button',
          'Layout::DrawButton(renderer, m_settings_button',
          'Layout::DrawButton(renderer, m_continue_button',
          'DrawHomeCards(renderer);','if(m_home_open>=0)DrawHomeOptions(renderer);'):
    assert k in draw,k
for legacy in ('m_file_tile->Draw(renderer);',
               'm_output_tile->DrawReadOnly(renderer);',
               'm_input_tile->DrawReadOnly(renderer);',
               'm_keyboard_size_tile->DrawReadOnly(renderer);'):
    assert legacy not in draw,legacy
assert 'UpdateHomeOptions(mouse);' in up
assert 'OpenHomeOptions(c);' in up
assert 'ChangeState(new SongLibState(m_state))' in s
assert 'OpenAdvancedSettings()' in up
print('OK: portada, botones de navegación y configuración preservados; cinco tarjetas ahora editables')
