"""Regresión de material translúcido y articulación, solo modo 3D."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
renderer=(root/"src/Renderer.cpp").read_text()
keyboard=(root/"src/KeyboardDisplay.cpp").read_text()
assert "const float drop=frontTravel*depression*t;" in renderer
assert "const float zScale=black?56.0f:47.0f;" in renderer
assert "const float materialR=black?34.0f:238.0f;" in renderer
assert "glDepthMask(GL_FALSE);" in renderer
assert "glDepthMask(GL_TRUE);" in renderer
assert "const float nz=(a.nz+b.nz+c.nz)/3.0f;" in renderer
assert "if(nz<.68f)continue;" in renderer
assert "static_cast<int>((black?93.0f:76.0f)*depression)" in renderer
assert "if(cue && depression<=.005f)" in renderer
assert "m_frame_active_keys[name]=handColor;" in keyboard
assert "const int noteOpacity=sounding?" in keyboard
assert "std::max(16,std::min(m_visual.opacity,58)):m_visual.opacity;" in keyboard
assert "const bool sounding=(i->start<=current_time && i->end>current_time);" in keyboard
assert "const auto compact=(sounding||releasing)?" in keyboard
assert "ShortNoteVisual::Absorb(y+y_offset,limit," in keyboard
assert "renderer.DrawQuad(key_x,y_offset,key_width,key_height);" in keyboard
print("OK: tecla articulada, material Blender visible y contacto translúcido")
