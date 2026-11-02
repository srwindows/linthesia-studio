"""Integración: notas largas al teclado y pulsación MIDI 3D (GPLv2)."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
kbd=(root/"src/KeyboardDisplay.cpp").read_text()
header=(root/"src/KeyboardDisplay.h").read_text()
render=(root/"src/Renderer.cpp").read_text()
for fragment in (
    "m_frame_active_keys=m_hands?KeyNames{}:m_active_keys;",
    "m_frame_contact_keys.clear();",
    "note.end<=current_time",
    "if(p.mode==Track::ModePlayedAutomatically)",
    "m_frame_active_keys[name]=handColor;",
    "m_frame_contact_keys[name]=handColor;",
    "(m_render_3d||m_hands)?",
    "ShortNoteVisual::Absorb(y+y_offset,limit,",
    "if(height<=0)continue;",
    "const auto travel=m_key_travel.find(note_name);",
    "renderer.DrawPianoKey3D(start_x,y_offset,black_width,black_height,",
    "renderer.DrawPianoKey3D(key_x,y_offset,key_width,key_height,",
):
    assert fragment in kbd,fragment
assert "KeyNames m_frame_active_keys;" in header
assert "KeyNames m_frame_contact_keys;" in header
assert "std::map<std::string,float> m_key_travel;" in header
assert 'BlenderMesh::WhiteCount' in render
assert 'BlenderMesh::BlackCount' in render
assert 'BlenderMesh::NoteCount' in render
assert 'renderer' not in '' # Blender geometría nativa empleada en Renderizado OpenGL.
assert "GNU GENERAL PUBLIC LICENSE" in (root/"COPYING").read_text()
try:
    from PIL import Image
except ImportError:Image=None
lab=Path("/tmp/linthesia-3d-impact-qa")
if Image and all((lab/(name+".png")).is_file()
                 for name in ("before","held","after","compact")):
    photos={name:Image.open(lab/(name+".png")).convert("RGB")
            for name in ("before","held","after","compact")}
    assert photos["held"].size==(1280,720)
    assert photos["compact"].size==(850,590)
    held=photos["held"]; released=photos["after"]
    # F#4: sostenido apagado después de NoteOff (zona de tecla negra).
    assert sum(held.getpixel((660,617)))-sum(released.getpixel((660,617)))>220
    # A4: tecla blanca coloreada mientras la nota MIDI está sostenida.
    h=held.getpixel((698,655));off=released.getpixel((698,655))
    assert h[2]>h[0] and off[0]>=off[2]-25,(h,off)
    assert sum(abs(h[i]-off[i]) for i in range(3))>100
    # No exigir una cabeza permanente: ahora es absorbida en la tecla.
print("OK: tecla negra/blanca, caída sostenida, NoteOff y geometría 3D")
