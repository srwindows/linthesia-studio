from pathlib import Path
import sys
r=Path(sys.argv[1])
p=(r/'src/PlayingState.cpp').read_text()
h=(r/'src/HandPractice.h').read_text()
store=(r/'src/HandCorrectionStore.h').read_text()
for text in ('Tramo A-B: IZQ','Tramo A-B: DER','Deshacer tramo',
             'm_hand_classifier.AssignRange(m_loop_start,m_loop_end,hand)',
             'm_bulk_undo=previous','m_hand_classifier.RestoreCorrections(m_bulk_undo)',
             'm_midi_preferences.handEditorTool==1',
             'm_hand_classifier.SetFinger(chosen->track_id',
             'm_hand_classifier.ClearFinger(chosen->track_id',
             'm_hand_classifier.Finger(n.track_id,n.note_id,n.start)',
             'SaveHandAnnotations()', 'HandCorrectionStore::LoadAll(m_state.song_path)',
             'HandCorrectionStore::SaveAll(m_state.song_path',
             'if(!m_loop_defined||m_loop_start>=m_loop_end)'):
 assert text in p,text
assert 'Fingerings() const' in h
assert 'size_t AssignRange(' in h
assert 'LINTHESIA_HAND_ASSIGNMENTS_2' in store
assert 'LINTHESIA_HAND_ASSIGNMENTS_1' in store
print('OK: range buttons, undo, finger 1..5 and separate per-song storage integrated')
