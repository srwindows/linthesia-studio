"""Contrato de protección de las carpetas Añadidos y Recientes."""
from pathlib import Path
import sys
r=Path(sys.argv[1]);s=(r/'src/SongLibState.cpp').read_text()
assert 'return "Añadidos";' in s
assert 'return "Recientes";' in s
assert 'path==RecentFolderPath() ||' in s
assert 'm_current_path==RecentFolderPath()' in s
assert 'm_current_path==RecentFolderPath())return false' in s
assert 'm_recent_options_open=true;' in s
assert 'Configurar Recientes' in s
assert 'm_context_open && m_context_path==RecentFolderPath()' in s
assert 'm_context_open && m_context_path!=RecentFolderPath()' in s
assert 'if(target==RecentFolderPath())' in s
assert 'if(nextPath==RecentFolderPath())return;' in s
assert 'if(child==RecentFolderPath())continue;' in s
assert 'RecentSongs::List(m_preferences.recentLimit' in s
assert 'if(m_current_path==RecentFolderPath())UpdateSongTiles();' in s
assert 'm_state.song_path=path;' in s
picker=(r/'src/midi_import_picker.py').read_text()
assert 'Importar selección a Añadidos' in picker
assert '"1"' in picker # La ruta fisica sigue siendo 1.
playing=(r/'src/PlayingState.cpp').read_text()
assert 'RecentSongs::RecordPlay(m_state.song_path)' in playing
assert 'RecentSongs::RecordPoints(m_state.song_path' in playing
licence=(r/'COPYING').read_text()
assert 'GNU GENERAL PUBLIC LICENSE' in licence and 'Version 2, June 1991' in licence
print('OK: Añadidos fijo, Recientes protegido, reproducción, preferencias y GPL v2')
