from pathlib import Path
import sys
root=Path(sys.argv[1])
source=(root/'src/TrackSelectionState.cpp').read_text()
header=(root/'src/TrackSelectionState.h').read_text()
player=(root/'src/PlayingState.cpp').read_text()
for token in (
    'm_practice_hand=AdvancedSettingsIO::Load().practiceHands;',
    'void TrackSelectionState::ChoosePracticeHand(int choice)',
    'const int modes[3]={HandPractice::LeftOnly,HandPractice::Both,',
    'preferences.practiceHands=requested;',
    'AdvancedSettingsIO::Save(preferences,error)',
    'm_practice_hand=requested;',
    'ButtonState TrackSelectionState::HandChoiceBounds(int button) const',
    'const char* hands[3]={"Izquierda","Ambas","Derecha"};',
    'm_detect_button.Update(handMouse);',
    'm_hand_buttons[option].Update(handMouse);',
    'if(m_hand_buttons[option].hit)',
    'ChoosePracticeHand(option);',
    'ChangeState(new PlayingState(m_state));',
    'PersistCurrent();',
    'Layout::DrawCompactButton(renderer,m_hand_buttons[index],GetTexture(ButtonSongs),',
    'm_hand_status_until=SDL_GetTicks()+1800;',
):
    assert token in source, token
assert 'int m_practice_hand=0' in header
assert 'm_midi_preferences=AdvancedSettingsIO::Load();' in player
assert 'case 124: set(p.practiceHands,0,2);break;' in player and 'if(e.id==124||e.id==127)' in player
assert 'AdvancedSettingsIO::Save(m_midi_preferences,error)' in player and 'SavePlayerSettings()' in player
assert 'SetupNoteState();' in player
assert 'src/FluidEngine.cpp' not in source
print("OK: track selection Left/Both/Right saves the same preferences the player reads; no tracks or MIDI/audio paths changed")
