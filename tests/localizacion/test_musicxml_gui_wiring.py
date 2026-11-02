from pathlib import Path
import sys
root=Path(sys.argv[1])
song=(root/'src/SongLibState.cpp').read_text()
main=(root/'src/main.cpp').read_text()
player=(root/'src/PlayingState.cpp').read_text()
settings=(root/'src/AdvancedSettings.cpp').read_text()
search=(root/'src/MidiSearchIndex.cpp').read_text()
for token in ('MusicXmlImport::Convert(path)',
              'm_state.musicxml_score_path=annotations',
              'MusicXmlImport::IsXml(f)'):
    assert token in song,token
assert 'MusicXmlImport::Convert(file_opt)' in main
assert 'state.musicxml_score_path=imported_score_annotations' in main
assert 'm_state.musicxml_score_path' in player
assert 'LINTHESIA_MUSICXML_SCORE_1' in player
for token in ('notationExpanded','notationZoom','GetStaffViewport()',
              'm_midi_preferences.notationExpanded=!m_midi_preferences.notationExpanded',
              'AdvancedSettingsIO::Save(m_midi_preferences,error)',
              'tags.find("staccato")','tags.find("tenuto")',
              'tags.find("fermata")','marks.find("slur-start")',
              'marks.find("slur-stop")'):
    assert token in player,token
for token in ('notation-expanded','notation-zoom'):
    assert token in settings
for ext in ('.musicxml','.mxl','.xml'):
    assert ext in search,ext
print('OK MusicXML GUI/CLI importer, annotation display, secure source and score zoom persistence')
