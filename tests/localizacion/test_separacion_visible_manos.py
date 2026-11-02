from pathlib import Path
import sys
r=Path(sys.argv[1]); c=(r/'src/PlayingState.cpp').read_text()
kbd=(r/'src/KeyboardDisplay.cpp').read_text()
h=(r/'src/KeyboardDisplay.h').read_text()
for token in (
 'm_hand_classifier.Selected(n,p.practiceHands)',
 'm_score_tracks.count(n.track_id)',
 'm_keyboard->SetHandVisualization(&m_hand_classifier,',
 'visibleNotes, m_show_duration',
 'm_hand_classifier.Selected(n,m_midi_preferences.practiceHands)',
 'case 124: set(p.practiceHands,0,2);break;',
 'm_midi_preferences.notationVisible=!m_midi_preferences.notationVisible',
 'm_hand_classifier.Selected(n,m_midi_preferences.practiceHands)',
):
 assert token in c,token
for token in (
 'const auto handColor=NoteColor(note,p.color);',
 'm_frame_active_keys[name]=handColor;',
 'm_frame_contact_keys[name]=handColor;',
 'const Track::TrackColor color = NoteColor(*i,',
):
 assert token in kbd,token
assert 'm_hands->ForNote(n)' in h
assert 'SetManualKeyboardNotes' in h
assert 'm_frame_active_keys=m_hands?KeyNames{}:m_active_keys;' in kbd
assert 'HandPractice::Left?Track::TangoSkyBlue' in h
assert 'Track::TangoOrange' in h
print('OK Izquierda/derecha now filter actual visible notes and assign contrasting key/note colors, Sol/Fa toggle')
