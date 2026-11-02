from pathlib import Path
import sys
r=Path(sys.argv[1])
s=(r/'src/SongLibState.cpp').read_text()
h=(r/'src/SongLibState.h').read_text()
m=(r/'src/main.cpp').read_text()
search=(r/'src/MidiSearchIndex.cpp').read_text()
for x in ('OpenMetaEditor()', 'CloseMetaEditor(true)',
          'SongMetadata::Save(m_meta_path,m_meta_entry)',
          'SongMetadata::Load(path)', 'm_meta_field=0',
          'METADATOS DE LA CANCIÓN','m_state.song_title=songInfo.title'):
 assert x in s,x
assert 'm_meta_editing' in h
assert 'SDLK_F10' in m
assert 'meta.composer' in search and 'meta.tags' in search
print('OK: F10 metadata editor, persistent titles/composers/tags, library search and presentation')
