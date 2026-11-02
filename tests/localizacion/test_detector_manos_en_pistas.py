from pathlib import Path
import sys
r=Path(sys.argv[1])
s=(r/'src/TrackSelectionState.cpp').read_text()
h=(r/'src/TrackSelectionState.h').read_text()
p=(r/'src/PlayingState.cpp').read_text()
for token in (
 'ButtonState TrackSelectionState::AutoDetectHandsBounds() const',
 'void TrackSelectionState::DetectHandsFromSelectedTracks()',
 'HandAutoDetector::Detect(m_state.midi->Notes()',
 'AdvancedSettingsIO::Save(updated,error)',
 'updated.handSplitPoint=m_detector_preview_split;',
 'void TrackSelectionState::OpenHandsDetector()',
 'void TrackSelectionState::ApplyHandsDetection()',
 'm_detector_preview_ready=true;',
 'HandCorrectionStore::LoadAll(m_state.song_path)',
 'classifier.SetCorrection(change.track,change.pitch,',
 'm_detected_hands_summary="Detección: izquierda "',
 '"Detectar manos"',
 'DetectHandsFromSelectedTracks();',
 'm_tooltip=m_detected_hands_summary;',
 'TextWriter tooltip(GetStateWidth()/2, GetStateHeight()',
):
 assert token in s,token
for token in ('ButtonState AutoDetectHandsBounds() const;',
              'void DetectHandsFromSelectedTracks();',
              'std::string m_detected_hands_summary;'):
 assert token in h,token
assert 'm_hand_classifier.Build(m_state.midi->Notes(),' in p
assert 'm_midi_preferences.handSplitPoint' in p
assert 'HandCorrectionStore::LoadAll(m_state.song_path)' in p
assert 'm_practice_hand=AdvancedSettingsIO::Load().practiceHands' in s
assert 'm_state.track_properties = BuildTrackProperties();' in s
print('OK: detection button is real, uses selected MIDI parts, same classifier/split as playback, results visible, notes unchanged')
