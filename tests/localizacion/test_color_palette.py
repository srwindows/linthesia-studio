"""Regresiones del selector de color de pistas y pruebas reales del ratón."""
from pathlib import Path
import sys

source=Path(sys.argv[1])
tracks=(source/'src'/'TrackTile.cpp').read_text()
tile_h=(source/'src'/'TrackTile.h').read_text()
screen=(source/'src'/'TrackSelectionState.cpp').read_text()
screen_h=(source/'src'/'TrackSelectionState.h').read_text()
properties=(source/'src'/'TrackProperties.h').read_text()

assert 'int color = static_cast<int>(m_color) + 1;' not in tracks
assert 'void SetColor(Track::TrackColor color)' in tile_h
assert 'Track::UserSelectableColorCount' in tile_h
assert 'const static int UserSelectableColorCount = 16;' in properties
assert 'const static int ColorCount = UserSelectableColorCount + 2;' in properties
assert 'm_color_palette_tile=-1' in screen_h
assert 'if(t.ButtonColor().hit)' in screen
assert 'm_color_palette_tile=static_cast<int>(i);' in screen
assert 'void TrackSelectionState::Update()' in screen
assert 'if(IsKeyPressed(KeyEscape))' in screen
assert 'const int chosen=ColorPaletteHit(mouse.x,mouse.y);' in screen
assert 'm_track_tiles[m_color_palette_tile].SetColor(' in screen
assert 'm_color_palette_tile=-1; // Fuera del panel' in screen
assert 'for(int i=0;i<Track::UserSelectableColorCount;++i)' in screen
for color in ('Azul','Verde','Naranja','Amarillo','Violeta','Rojo','Cian',
              'Turquesa','Esmeralda','Lima','Coral','Rosa','Fucsia','Índigo',
              'Cobre','Oliva'):
    assert '"'+color+'"' in screen
assert 'm_track_tiles[m_color_palette_tile].GetColor()' in screen
assert 'Copyright (c) 2007 Nicholas Piegdon' in tracks
assert 'Adaptation to GNU/Linux by Oscar Aceña' in screen
assert 'GNU GENERAL PUBLIC LICENSE' in (source/'COPYING').read_text()

# Las capturas se verifican cuando están presentes en el laboratorio local.
# El test básico sigue siendo portable a otras máquinas sin Xvfb ni Pillow.
lab=Path('/tmp/linthesia_palette_lab_20261017')
if (lab/'before.jpg').is_file():
    from PIL import Image
    def pixel(name,pos):
        return Image.open(lab/(name+'.jpg')).convert('RGB').getpixel(pos)
    blue=pixel('before',(399,165))
    red=pixel('selected-red',(399,165))
    red_after=pixel('after-return',(399,165))
    assert blue[2]>blue[0],('Initial blue',blue)
    assert red[0]>red[2]*2,('Selected red',red)
    assert abs(red_after[0]-red[0])<5 and abs(red_after[1]-red[1])<5
    assert pixel('open',(505,275))!=pixel('cancel-esc',(505,275))
    assert pixel('active-red',(505,275))!=pixel('cancel-outside',(505,275))
    assert pixel('cancel-outside',(399,165))==red
    assert pixel('palette-after-return',(505,275))!=pixel('after-return',(505,275))
print('OK: 6 colores, clic, ESC, clic fuera, resaltado actual y persistencia de color')
