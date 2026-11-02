from pathlib import Path
import sys
r=Path(sys.argv[1])
s=(r/'src/PlayingState.cpp').read_text()
h=(r/'src/PlayingState.h').read_text()
for k in ('IsKeyPressed(KeyF6)',
          'm_state.track_properties=m_saved_practice_track_modes',
          'FullSongPlayback::EnableAll(m_state.track_properties',
          'ToggleFullSongAudio();',
          'SeekTo(target);','FullSongPlayback::Count(',
          'RestorePracticeTrackModes();'):
 assert k in s,k
assert 'ShouldOfferCompleteMix(' not in s
assert 'm_full_song_audio=true;' in s
# All-tracks is opt-in via F6, never triggered on constructor entry.
constructor=s.split('PlayingState::PlayingState(',1)[1].split('PlayingState::~PlayingState(',1)[0]
assert 'FullSongPlayback::EnableAll' not in constructor
assert 'm_full_song_audio=true' not in constructor
assert s.count('FullSongPlayback::EnableAll(m_state.track_properties')==1
assert s.count('ToggleFullSongAudio();')==1
assert 'if(!m_settings_open && IsKeyPressed(KeyF6))ToggleFullSongAudio();' in s

assert 'bool m_full_song_audio=false' in h
assert 'm_saved_practice_track_modes' in h
assert s.count('RestorePracticeTrackModes();')>=4
assert 'case Track::ModeNotPlayed:           draw = false;  play = false;' in s
print('OK: UI explicitly switches from muted practice stems to complete accompaniment, preserving user settings')
