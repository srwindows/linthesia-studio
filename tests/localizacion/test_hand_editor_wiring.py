"""Public behavior integration: per-note left/right choice, persisted outside MIDI."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
p=(root/"src/PlayingState.cpp").read_text()
a=(root/"src/AdvancedSettings.cpp").read_text()
h=(root/"src/HandPractice.h").read_text()
assert "void PlayingState::RebuildHandClassification()" in p
assert "bool PlayingState::HandleStaffHandEditor(" in p
assert "m_hand_classifier.SetCorrection(chosen->track_id" in p
assert "HandCorrectionStore::SaveAll(" in p
assert "HandCorrectionStore::LoadAll(m_state.song_path)" in p
assert "if(HandleStaffHandEditor(Mouse()))return;" in p
assert "m_required_notes.clear();" in p
assert "Corregir manos con clic en pentagrama" in p
assert "División automática en nota MIDI" in p
assert "m_midi_preferences.handSplitPoint" in p
assert "m_midi_preferences.handEditEnabled" in p
assert 'I("Reproductor","hand-split-point",s.handSplitPoint);' in a
assert 'B("Reproductor","hand-edit-enabled",s.handEditEnabled);' in a
assert 'std::map<std::tuple<size_t,int,microseconds_t>,Hand> manual_' in h
print("OK: score note hand correction via left/right click, global split and per-song persistence")
assert '(!mouse.released.left&&!mouse.released.right)' in p
assert 'mouse.released.right?HandPractice::Right:' in p
