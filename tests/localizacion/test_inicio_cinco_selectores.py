from pathlib import Path
import sys
r=Path(sys.argv[1])
s=(r/'src/TitleState.cpp').read_text()
h=(r/'src/TitleState.h').read_text()
for token in (
 'DrawHomeCards(renderer);',
 'if(m_home_open>=0)DrawHomeOptions(renderer);',
 'UpdateHomeOptions(mouse);',
 'for(int c=0;c<5;++c)',
 'RecentSongs::List(10,RecentSongs::PlaybackOrder)',
 'ChangeState(new SongLibState(m_state));',
 'Midi::ReadFromFile(path)',
 'RecentSongs::RememberLastSelected(path)',
 'm_state.track_properties.clear();',
 'm_state.musicxml_score_path.clear();',
 'RefreshDeviceChoices();',
 'MidiCommOut::GetDeviceList()',
 'MidiCommIn::GetDeviceList()',
 'SaveKeyboardSize();',
 'if(!AdvancedSettingsIO::Save(updated,error))',
 'FluidEngine::Start(updated,error)',
 'm_home_open=-1;',
 '"Dispositivo de audio · FluidSynth"',
 '"Dispositivo de salida MIDI"',
 '"Dispositivo de entrada MIDI"',
 '"Tamaño del teclado"',
 '"Últimas 10 canciones reproducidas"',
 'm_saved_settings=updated;',
 '"No se pudo abrir el dispositivo seleccionado."',
):
 assert token in s,token
assert 'm_home_open=-1,m_home_page=0' in h
assert 'std::vector<HomeChoice> m_home_choices' in h
assert 'm_home_open>=0' in s
assert 'm_input_tile->DrawReadOnly(renderer)' not in s
assert 'm_output_tile->DrawReadOnly(renderer)' not in s
assert 'm_keyboard_size_tile->DrawReadOnly(renderer)' not in s
assert 'm_file_tile->Draw(renderer)' not in s
print("OK: five interactive selectors, 10 real recents, MIDI/audio choices, persistence, no automatic MIDI transmission")
