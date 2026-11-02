"""Recientes: engranaje, desplegables independientes y estilo original."""
from pathlib import Path
import sys

root=Path(sys.argv[1])
source=(root/'src'/'SongLibState.cpp').read_text()
head=(root/'src'/'SongLibState.h').read_text()
tile=(root/'src'/'SongTile.cpp').read_text()

assert 'bool SongLibState::RecentGearHit(int x,int y) const{' in source
assert 'm_current_path!=m_base_path' in source.split('bool SongLibState::RecentGearHit',1)[1].split('}',1)[0]
assert 'tile.GetPath()!=RecentFolderPath()' in source
assert 'const int side=36;' in source
assert 'm_recent_gear_pressed' in head
assert 'if(mouse.released.left && RecentGearHit(mouse.x,mouse.y)' in source
assert 'OpenRecentOptions();' in source
assert 'm_last_clicked_folder.clear();' in source
assert 'if(iconHover){' in source
assert 'renderer.DrawQuad(tile.GetX(),tile.GetY(),area.w,2);' in source
assert 'renderer.DrawQuad(gx,gy,iconSize,iconSize);' in source
assert 'm_recent_dropdown=1;' in source
assert 'm_recent_dropdown=2;' in source
assert 'm_recent_limit_draft=(index+1)*10;' in source
assert 'const int count=dropdown==1?20:4;' in source
assert 'm_recent_limit_draft=std::max(1,m_recent_limit_draft-1)' in source
assert 'm_recent_limit_draft=std::min(500,m_recent_limit_draft+1)' in source
assert 'm_recent_sort_draft=cycle(m_recent_sort_draft,-1,4)' in source
assert 'm_recent_sort_draft=cycle(m_recent_sort_draft,+1,4)' in source
assert 'if(m_recent_dropdown)m_recent_dropdown=0;' in source
assert 'if(mouse.released.left && m_recent_dropdown)' in source
assert 'm_recent_dropdown=0;' in source
assert 'AdvancedSettingsIO::Save(actual,error)' in source
assert 'SongTileWidth = 510' in (root/'src'/'SongTile.h').read_text()
assert 'const int SongTileHeight = 80' in (root/'src'/'SongTile.h').read_text()
assert 'renderer.SetColor(whole_tile.hovering ? hover : no_hover)' in tile
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
assert 'Copyright (c) 2007 Nicholas Piegdon' in (root/'src'/'PlayingState.cpp').read_text()
print('OK: engranaje, clic independiente, hover doble, selector 10-200 y ±1, y licencia')
