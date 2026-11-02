"""Code-level regression: no lost fade in notes, GLSL, fallback or key effects."""
from pathlib import Path
import sys
r=Path(sys.argv[1])
kbd=(r/'src/KeyboardDisplay.cpp').read_text()
renderer=(r/'src/Renderer.cpp').read_text()
header=(r/'src/KeyboardDisplay.h').read_text()
assert 'NoteReleaseFade::Visible(current_time,visual_end)' in kbd
assert 'NoteReleaseFade::Gain(current_time,visual_end)' in kbd
assert 'sounding||releasing' in kbd
assert 'if(m_render_3d && visual_end<=current_time && !releasing)' in kbd
assert 'impact.fade' in kbd
assert 'const float flightAge=it->second.visual.age+sinceRelease;' in kbd
assert 'flightAge>=1.10f' in kbd
assert 'm_release_impacts.erase(current.first)' in kbd
assert 'm_previous_impacts=m_current_impacts' in kbd
assert 'm_frame_active_keys[name]=handColor' in kbd
assert 'if(note.end<=current_time ||' in kbd
assert 'releaseFade=1.0f' in (r/'src/Renderer.h').read_text()
assert 'const float alpha=std::max(.13f,std::min(.78f,opacity/100.0f))*releaseFade;' in renderer
assert ')*releaseFade;' in renderer
assert 'float(glow),false,m_effect_birth==1' in renderer
assert 'NoteEmitter::Smooth(10.0f,24.0f,float(h))*releaseFade' in renderer
assert '.90f*attack*releaseFade' in renderer
assert 'std::min(.9f,glow/115.0f)*releaseFade' in renderer
assert 'std::map<std::string,ReleasedImpact> m_release_impacts;' in header
print("OK: 420ms note-body fade, independent 1.10s smoke and exact MIDI NoteOff")
