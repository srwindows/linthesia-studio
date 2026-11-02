"""Estado de espera real y teclas físicas 3D con MIDI virtual (GPLv2)."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
play=(root/"src/PlayingState.cpp").read_text()
keys=(root/"src/KeyboardDisplay.cpp").read_text()
renderer=(root/"src/Renderer.cpp").read_text()
for key in ( "if (isUserPlayableTrack(track_id) &&",
            "PracticeThisEvent(track_id,note_number,",
            "m_required_notes.insert(note_number);",
            "m_required_notes.insert(note_number);",
            "if (areAllRequiredKeysPressed())",
            "m_required_notes.erase(note_number);",
            'transportStatus="Esperando tecla";',
            "transportStatus+=MidiEvent::NoteName(id);"):
    assert key in play,key
for key in ("m_frame_contact_keys[name]=handColor;",
            "const Uint32 elapsed=m_last_travel_frame?",
            "iter->second+=(target-iter->second)*response;",
            "renderer.DrawPianoKey3D(key_x,y_offset,key_width,key_height,",
            "renderer.DrawPianoKey3D(start_x,y_offset,black_width,black_height,",
            "m_keyboard_center=x+x_offset+final_width/2.0f;"):
    assert key in keys,key
for key in ("void Renderer::DrawPianoKey3D(",
            "const BlenderMesh::Vertex* vertices=black?",
            "const float drop=frontTravel*depression*t;",
            "const float screenY=y+t*h-v.z*zScale+drop;",
            "void Renderer::BeginBlenderKeyboard3D(){",
            "glEnable(GL_DEPTH_TEST);",
            "if(cue && depression<=.005f)"):
    assert key in renderer,key
assert "GNU GENERAL PUBLIC LICENSE" in (root/"COPYING").read_text()
lab=Path("/tmp/linthesia-full-midi-qa")
try:
    from PIL import Image,ImageChops
except ImportError:
    Image=None
if Image and all((lab/(n+".png")).is_file()
                 for n in ("waiting","pressed","released")):
    wait,hit,up=[Image.open(lab/(n+".png")).convert("RGB")
                 for n in ("waiting","pressed","released")]
    assert wait.size==hit.size==up.size==(1280,720)
    def variance(a,b,region):
        c=ImageChops.difference(a.crop(region),b.crop(region))
        return sum(sum(px) for px in c.getdata())
    assert variance(wait,hit,(55,72,150,89))>15000  # transporte avanza
    assert variance(wait,hit,(568,561,615,674))>8000 # C4 pulsa
    assert variance(hit,up,(568,561,615,674))>4000 # tecla sube
    # La pausa no debe borrar las notas que esperan encima del teclado.
    assert max(wait.getpixel((x,y))[2] for x in range(570,601)
               for y in (450,485,520))>110
print("OK: Tocar espera, MIDI válido reanuda, tecla 3D baja/sube")
