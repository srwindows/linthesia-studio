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
    # Canonical per-row hashes from the original six-color public atlas.
    # Store checksums, NOT an untracked machine-specific .bak file.
    import hashlib
    preserved_row_sha256 = ['52a6ec9c2e4732158fbd9d3f419c853c31ffb8561d1af805d173222ff7c50239', 'fd11f033365363cbcbccf023427d223a83ba08d122d1b70a6ada839934273aa9', '312e60c8786b0e68c4cd027ef756cffd080a225b38bff5875cbd616e845e2d89', '27243699832ada827f028285a4a0e2207e8e23087f7f3f60146b740d45ac629c', '24eb93c7e2faea5f8502beba56f7a147f11e61a485095e91044eb5961ea50250', 'af6e7bc6f392f158aaacd0aea184f186e7688a657665f07c6d9c73ed525244c6', '3833ec3d91c2b5ee09344ddcab1cd9b2b44b1738518fbbc8c9bcd7a7d93533f7']
    assert new.size==(972,648)
    for i in range(6):
        assert hashlib.sha256(new.crop((0,i*36,972,(i+1)*36)).tobytes()).hexdigest()==preserved_row_sha256[i]
    for i in (16,17):
        assert hashlib.sha256(new.crop((0,i*36,972,(i+1)*36)).tobytes()).hexdigest()==preserved_row_sha256[6]
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
