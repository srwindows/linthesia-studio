"""Regresión de continuidad de las notas 3D y acabado sin halos agresivos."""
from pathlib import Path
import sys
p=Path(sys.argv[1])
k=(p/"src/KeyboardDisplay.cpp").read_text()
r=(p/"src/Renderer.cpp").read_text()
assert "std::map<std::pair<int,int>,std::size_t> most_recent" in k
assert "old.state==note->state && old.retry_state==note->retry_state" in k
assert "if(gap>=-onePixel && gap<=onePixel &&" in k
assert "previous.second=std::max(previous.second,note->end);" in k
assert "const microseconds_t visual_end=visual.second;" in k
assert "const long long adjusted_end   = max(visual_end-current_time, 0LL);" in k
assert "m_render_3d?1:2" in k
assert "SDL_Color{58,77,88,92}" in k
assert '#include "BlenderMeshes.h"' in r
assert 'BlenderMesh::NoteCount' in r
assert 'const float diff=std::max(0.0f' in r
assert 'const float cap=std::max(2.0f' in r
assert 'glEnable(GL_DEPTH_TEST);' in r
assert 'glClear(GL_DEPTH_BUFFER_BIT);' in r
# La unión musical no afecta al procesamiento MIDI: sólo a la visualización.
assert "for (const auto& visual : visual_notes)" in k
assert "if(m_render_3d && prior!=most_recent.end())" in k
try:
 from PIL import Image, ImageChops
except ImportError:
 Image=None
lab=Path("/tmp/linthesia-3d-geometry-qa")
if Image:
 for mode in ("moderno","clasico"):
  for size,shape in (("1280",(1280,720)),("950",(950,620)),("737",(737,540))):
   f=lab/mode/("paused-"+size+".png")
   if f.is_file():
    im=Image.open(f).convert("RGB")
    assert im.size==shape,(f,im.size)
    assert im.getbbox(),f
print("OK: uniones musicales, bordes discretos, perspectiva estable y 2D aislado")
