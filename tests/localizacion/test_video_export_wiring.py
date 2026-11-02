"""Video exporter: public controls, actual frames, no side-effect on track selection."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
player=(root/'src/PlayingState.cpp').read_text()
ui=(root/'src/PlayingVideoExport.cpp').read_text()
engine=(root/'src/VideoExporter.cpp').read_text()
header=(root/'src/VideoExporter.h').read_text()
meson=(root/'src/meson.build').read_text()
for name in ('VideoExporter.cpp','PlayingVideoExport.cpp'):
    assert f"'{name}'" in meson
for token in (
    'if(m_video.Active())CaptureVideoFrame();',
    'if(m_video_dialog)DrawVideoDialog(renderer);',
    'OpenVideoDialog();',
    'StopVideoExport(false);',
    'StopVideoExport(true);',
    'if(m_video.Active() && m_state.midi->IsSongOver())',
    'if(m_video.Active() && IsKeyPressed(KeyEscape))',
    'if(m_video_prompt_on_enter)OpenVideoDialog();',
    '"Finalizar vídeo"',
):
    assert token in player,token
assert '"Exportar vídeo del escenario"' in ui
assert '"Exportar vídeo del escenario"' in (root/'src/TrackSelectionState.cpp').read_text()
assert player.index('if(m_video.Active())CaptureVideoFrame();') < player.index('if(m_midi_preferences.notationVisible)DrawGrandStaff(renderer);')
for token in (
    'void PlayingState::DrawVideoDialog(Renderer& renderer) const',
    'void PlayingState::StartVideoExport()',
    'void PlayingState::StopVideoExport(bool cancel)',
    'void PlayingState::CaptureVideoFrame() const',
    'm_video_previous_modes=m_state.track_properties;',
    'm_state.track_properties=m_video_previous_modes;',
    'm_video_previous_position=m_state.midi->GetSongPositionInMicroseconds();',
    'SeekTo(m_video_previous_position);',
    'if(prop.mode!=Track::ModeNotPlayed',
    'm_video_path=DefaultPath(m_state.song_title)',
    'kdialog',
    'm_video_sinks=VideoExporter::AvailableMonitors();',
    '"Generar vídeo"',
    '"Elegir carpeta"',
):
    assert token in ui,token
for fmt in ('mp4','mkv','mov','webm','gif'):
    assert f'"{fmt}"' in ui
for resolution in ('1280,720','1920,1080','2560,1440','3840,2160'):
    assert resolution in ui
for token in ('"libx264"','"libvpx-vp9"','"gif"',
    '"pulse"','"libopus"','"aac"','glReadPixels(',
    'GL_RGB','GL_UNSIGNED_BYTE','-shortest','"-vf",scale',
    'm_writer=std::thread(', 'm_queue.size()>9',
    'if(cancel)kill(m_pid,SIGTERM);',
    'if(cancel)unlink(m_options.path.c_str());'):
    assert token in engine,token
assert 'std::atomic<bool> m_pipeError' in header
print('OK: real OpenGL video export, FFmpeg formats, audio sink, cancellation and state restoration')
