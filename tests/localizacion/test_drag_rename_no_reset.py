"""Regresiones: no entrar al mantener una carpeta; conservar orden al renombrar."""
from pathlib import Path
import sys
root = Path(sys.argv[1])
s = (root/'src/SongLibState.cpp').read_text()
refresh=s.split('void SongLibState::UpdateSongTiles() {',1)[1].split('void SongLibState::Resize() {',1)[0]
assert 'm_personal.ReadOrder(m_current_path);' in refresh
assert 'if(!saved.empty())' in refresh, 'Custom positions must override alphabetical preferences'
assert 'if(!saved.empty() && !m_preferences.alphabeticOrder)' not in refresh
hover=s.split('void SongLibState::UpdateFolderDragHover(int x,int y){',1)[1].split('void SongLibState::ExecuteFolderBatch()',1)[0]
assert 'TileAt(x,y)' not in hover, 'Hover over a sibling folder may never open it'
assert 'm_path_up_button' in hover, 'Keep explicit navigation to parent while holding'
assert 'NavigateDuringFolderDrag(target);' in hover
assert 'SDL_GetTicks()-m_folder_hover_started' in hover
complete=s.split('void SongLibState::CompleteDrag(int x,int y){',1)[1].split('void SongLibState::Update() {',1)[0]
assert 'm_drop_choice=true;' in complete, 'Drop choice must wait until button release'
assert 'm_drop_target=target;' in complete
rename=s.split('void SongLibState::FinishCreateFolder(bool confirm){',1)[1].split('void SongLibState::',1)[0]
assert 'RenameOrderEntry(m_current_path,shortName(renameSource),m_folder_name)' in rename
assert 'UpdateSongTiles();' in rename
assert 'm_current_page=static_cast<int>(i)' in rename
print('OK: conservative drag-hover, drop decision, and order persistence through rename')
