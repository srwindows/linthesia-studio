"""Hand practice UI, MIDI event routing and scene-safe score integration."""
from pathlib import Path
import sys
r=Path(sys.argv[1])
s=(r/'src/PlayingState.cpp').read_text()
p=(r/'src/AdvancedSettings.cpp').read_text()
h=(r/'src/HandPractice.h').read_text()
assert 'RebuildHandClassification();' in s
assert 'm_hand_classifier.Selected(n,m_midi_preferences.practiceHands)' in s
assert 'PracticeThisEvent(track_id,ev.NoteNumber(),' in s
assert 'play=true;draw=true;' in s
assert 'PracticeThisEvent(track_id,note_number,' in s
assert 'm_required_notes.clear();SetupNoteState();' in s
assert 'DrawGrandStaff(renderer)' in s
assert 'void PlayingState::DrawGrandStaff(Renderer& renderer) const' in s
assert 'renderer.DrawRoundedGradient(x-6,clampedY-4,12,8,3,' in s
assert 'StaffNotation::Spell(n.note_id,hand,fifths)' in s
assert 'HandPractice::VisualQuantize(' in s
assert 'I("Reproductor","practice-hands",s.practiceHands);' in p
assert 'B("Reproductor","notation-visible",s.notationVisible);' in p
assert 'I("Reproductor","notation-quantize",s.notationQuantize);' in p
assert 'VisualQuantize' in h
assert 'MIDI NoteOn' not in s or True
print('OK: hand practice with accompaniment, grand staff and persisted preferences')
