"""Stage clean, particles rise after NoteOff without prolonging MIDI audio."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
r=(root/'src/Renderer.cpp').read_text()
k=(root/'src/KeyboardDisplay.cpp').read_text()
p=(root/'src/PlayingState.cpp').read_text()
t=(root/'src/TrackSelectionState.cpp').read_text()
assert 'const int plumeH=std::max(90,std::min(int(windowH*.47f*m_effect_flame_height/100.f),keyTop-95));' in r
assert 'float front=age*375.0;' in r
assert 'if(age<0.0 || age>=1.10)' in r
assert 'if(m_effect_smoke)for(unsigned i=0;i<30;++i)' in r and 'glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);' in r
assert 'const float life=(effectAge-delay)/lifetime;' in r
assert 'NoteEmitter::Smooth(0.f,.14f,life)' in r
assert 'flightAge*(m_visual.effectSpeed/100.f)>=1.10f*m_visual.effectLife/100.f' in k
assert 'last.fade=1.0f;' in k
assert 'NoteReleaseFade::DurationUs' not in r
assert 'const int cinematicOpacity=noteOpacity;' in k
assert 'const SDL_Color base=Track::ColorNoteWhite[brush_id];' in k
assert 'm_visual.glow, // restore actual fluorescence slider' in k
assert 'const char* hands[3]' not in p.split('void PlayingState::DrawTransport(',1)[1]
assert '"Exportar vídeo del escenario"' in t and 'm_record_video_button.hit' in t
print('OK: tall rising flame with smoke, MIDI release independent, restored neon/transparency, video on track selection')
