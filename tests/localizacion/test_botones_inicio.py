"""Regresión del pie de inicio, ajustes persistentes y cursor SDL."""
from pathlib import Path
import io
import re
import subprocess
import sys
from PIL import Image
root=Path(sys.argv[1])
button_names=('title_Canciones','title_Configuracion')
pristine=Image.open(io.BytesIO(subprocess.check_output(
 ['git','-C',str(root),'show','origin/main:graphics/title_Exit.tga'])))
for item in button_names:
 im=Image.open(root/'graphics'/f'{item}.tga')
 assert im.size==pristine.size==(256,64)
 assert im.mode=='RGBA'
 assert im.getchannel('A').getbbox()==pristine.getchannel('A').getbbox()
 print('FOOTER_SPRITE_OK',item,im.size)
layout=(root/'src/TitleState.cpp').read_text()
for required in [
 'm_songs_button.Update(mouse)',
 'ChangeState(new SongLibState(m_state))',
 'm_settings_button.Update(mouse)',
 'OpenAdvancedSettings()',
 'CloseAdvancedSettings()',
 'UserSetting::Set(KEYBOARD_SIZE_KEY,m_keyboard_size.GetValue())',
 'Layout::DrawButton(renderer, m_songs_button, GetTexture(ButtonSongs))',
 'Layout::DrawButton(renderer, m_settings_button, GetTexture(ButtonSettings))',
 'const bool narrow = GetStateWidth() < 800',
 'if (IsKeyPressed(KeyEnter) || m_continue_button.hit)'
]:
 assert required in layout,required
settings=(root/'src/SettingsPanel.cpp').read_text()
for required in [
 'UserSetting::Set(REFRESH_RATE_KEY,std::to_string(a.fps))',
 'AdvancedSettingsIO::Save(a,error)',
 'DrawAdvancedSettings(renderer)',
 'm_settings_dirty=false',
 'ResetSettingsSection()',
 'FluidEngine::Start(a,error)',
]:
 assert required in (settings+layout),required
compatible=(root/'src/CompatibleSystem.cpp').read_text()
hide=compatible.split('void HideMouseCursor()',1)[1].split('void ShowMouseCursor()',1)[0]
assert 'SDL_ShowCursor(SDL_ENABLE)' in hide
assert 'SDL_DISABLE' not in hide
assert 'SDL_SetRelativeMouseMode' not in compatible
print('FOOTER_ACTIONS_AND_VISIBLE_CURSOR_PASS')
