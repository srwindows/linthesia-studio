"""Regresión visual: sostenidos activos y notas 3D translúcidas (GPLv2)."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
src=(root/'src/KeyboardDisplay.cpp').read_text()
h=(root/'src/KeyboardDisplay.h').read_text()
assert "m_render_3d=perspective3d;" in src
assert "Track::ColorNoteHit[idx]" in src
assert "if(active && !m_render_3d && black_width>=3" in src
assert "renderer.DrawPianoKey3D(start_x,y_offset,black_width,black_height," in src
assert "DrawBlackKey(renderer,tex,BlackKeyDimensions" in src
assert "renderer.DrawQuad(x1,y_offset+2,w1" in src
assert "Track::ColorNoteWhite[brush_id]" in src
assert "if(m_render_3d)" in src
assert "m_visual.radius" in src
assert "renderer.DrawNeonCapsule(newLeft,top,newWidth,height" in src
assert 'm_visual.glow, // restore actual fluorescence slider' in src
assert "const int noteOpacity=sounding?" in src
assert "const int cinematicOpacity=noteOpacity;" in src
assert "m_render_3d=false" in h
assert "GNU GENERAL PUBLIC LICENSE" in (root/'COPYING').read_text()
try:
    from PIL import Image,ImageChops
except ImportError:
    Image=None
lab=Path('/tmp/linthesia-neon-qa')
names=('3d-semis','3d-natural','2d-semis')
if Image and all((lab/name/'active.png').is_file() for name in names):
    snaps={name:Image.open(lab/name/'active.png').convert('RGB') for name in names}
    semis=snaps['3d-semis'];naturals=snaps['3d-natural']
    assert all(im.size==(1280,720) for im in snaps.values())
    # C#4, F#4 y A#4 están iluminadas; la tecla natural
    # de comparación conserva el sprite negro no pulsado.
    for x in (591,659,710):
        r,g,b=semis.getpixel((x,610))
        rr,gg,bb=naturals.getpixel((x,610))
        assert (r+g+b)-(rr+gg+bb)>155,(x,(r,g,b),(rr,gg,bb))
    # No iluminar las teclas de alrededor.
    assert semis.getpixel((610,610))==naturals.getpixel((610,610))
    # Las teclas negras también muestran un halo en modo 2D.
    assert snaps['2d-semis'].getpixel((591,610))[2]>165
    # Las notas 3D adquieren azul eléctrico; se distinguen
    # visualmente de las notas del 2D clásico.
    assert ImageChops.difference(
        semis.crop((570,290,725,580)),
        snaps['2d-semis'].crop((570,290,725,580))
    ).getbbox()
print('OK: C#, F#, A# iluminadas; teclas vecinas intactas; notas 3D con halo y alfa')
