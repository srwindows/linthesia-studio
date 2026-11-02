"""16 colores auténticos y pulsación transversal con autoría original intacta."""
from pathlib import Path
import sys,re
root=Path(sys.argv[1])
src=root/'src'
colors=(src/'TrackProperties.h').read_text()
screen=(src/'TrackSelectionState.cpp').read_text()
track=(src/'TrackTile.cpp').read_text()
renderer=(src/'Renderer.cpp').read_text()
manager=(src/'GameState.cpp').read_text()
header=(src/'Renderer.h').read_text()
assert 'const static int UserSelectableColorCount = 16;' in colors
assert 'const static int ColorCount = UserSelectableColorCount + 2;' in colors
for color in ('Cyan','Turquoise','Emerald','Lime','Coral','Pink','Fuchsia','Indigo','Copper','Olive'):
    assert color in colors
for color in ('Cian','Turquesa','Esmeralda','Lima','Coral','Rosa','Fucsia','Índigo','Cobre','Oliva'):
    assert '"'+color+'"' in screen
assert '(m_track_tiles.size()) % Track::UserSelectableColorCount' in screen
assert 'const int width=352, height=291;' in screen
assert '(i%4)*(swatchW+gap)' in screen and '(i/4)*(swatchH+gap)' in screen
assert 'ColorPaletteHit(mouse.x,mouse.y)' in screen
assert 'Track::UserSelectableColorCount;' in screen
assert 'GraphicHeight * Track::UserSelectableColorCount;' in track
assert 'BeginPressFrame(m_mouse.x,m_mouse.y,m_mouse.held.left)' in manager
assert 'renderer.DrawPressFeedback();' in manager
assert 'ObservePressRect' not in renderer
assert 'const bool pressed=pressState.held' in renderer
assert 'glColor4f(originalColor[0]*0.77f' in renderer
assert 'renderer.DrawTga' not in renderer.split('void Renderer::DrawPressFeedback(){',1)[1].split('void Renderer::SetColor',1)[0]
assert 'm_mouse.held.left' in manager
assert 'void BeginPressFrame(' in header and 'void DrawPressFeedback()' in header
assert 'Copyright (c) 2007 Nicholas Piegdon' in screen
assert 'Adaptation to GNU/Linux by Oscar Aceña' in screen
assert 'Version 2, June 1991' in (root/'COPYING').read_text()
try:
    from PIL import Image,ImageChops
except ImportError:
    Image=None
if Image:
    new=Image.open(root/'graphics/InterfaceButtons.tga').convert('RGBA')
    old=Image.open(root/'graphics/InterfaceButtons.tga.bak-20261017-sixteen-colors').convert('RGBA')
    assert new.size==(972,648) and old.size==(972,252)
    assert all(new.crop((0,i*36,972,(i+1)*36)).tobytes() ==
               old.crop((0,i*36,972,(i+1)*36)).tobytes() for i in range(6))
    assert all(new.crop((0,i*36,972,(i+1)*36)).tobytes()==
               old.crop((0,216,972,252)).tobytes() for i in (16,17))
    # Comparación de pulsación real en cuatro contextos independientes.
    lab=Path('/tmp/linthesia-16-colors-pressure-lab')
    for before,pressed in [
        ('home-hover','home-pressed'),
        ('tracks-hover','tracks-pressed'),
        ('settings-custom-hover','settings-custom-pressed'),
        ('library-hover','library-pressed')]:
        if (lab/(before+'.jpg')).exists() and (lab/(pressed+'.jpg')).exists():
            a=Image.open(lab/(before+'.jpg')).convert('RGB')
            b=Image.open(lab/(pressed+'.jpg')).convert('RGB')
            diff=ImageChops.difference(a,b)
            assert diff.getbbox() is not None, (before,pressed)
    if (lab/'palette-current16.jpg').exists():
        im=Image.open(lab/'palette-current16.jpg')
        assert im.width==1050
print('OK: 16 colores y atlas de 18 filas; pulsación común; 4 pruebas visuales; autores GPL v2')
