from pathlib import Path
import sys
root=Path(sys.argv[1]); header=(root/'src/VideoExporter.h').read_text();player=(root/'src/PlayingState.cpp').read_text();video=(root/'src/PlayingVideoExport.cpp').read_text()
assert 'bool Failed() const {return m_pipeError.load();}' in header
assert 'if(m_video.Active() && m_video.Failed()){' in player
assert 'StopVideoExport(false);' in player
assert 'void PlayingState::StopVideoExport(bool cancel)' in video
assert 'm_state.track_properties=m_video_previous_modes;' in video
assert 'm_video_status=result;' in video
assert 'm_video.Stop(result,cancel);' in video
print('OK: encoder failure triggers UI feedback and track-state restoration')
