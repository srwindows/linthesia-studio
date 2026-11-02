"""Mantener las teclas negras físicas negras tras ampliar los colores MIDI."""
from pathlib import Path
import sys
root=Path(sys.argv[1]);src=(root/'src'/'KeyboardDisplay.cpp').read_text()
head=(root/'src'/'KeyboardDisplay.h').read_text()
assert 'constexpr int BlackKeySpriteIndex = 7;' in src
assert 'black_width,black_height,BlackKeySpriteIndex);' in src
assert 'Track::TrackColor c = Track::MissedNote;' not in src
assert 'renderer.SetColor(c);' in src  # Las teclas blancas mantienen su renderizado.
assert 'sprite_index * d.tex_width' in src
assert 'int sprite_index) const;' in head
assert 'Copyright (c) 2007 Nicholas Piegdon' in src
assert 'Adaptation to GNU/Linux by Oscar Aceña' in src
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
lab=Path('/tmp/linthesia-black-keys-verification')
if (lab/'before.png').exists() and (lab/'after.png').exists():
    from PIL import Image
    a=Image.open(lab/'before.png').convert('RGB')
    b=Image.open(lab/'after.png').convert('RGB')
    assert a.size==b.size==(1280,720)
    for x in (40,87,112,157):
        assert a.getpixel((x,485))==(86,157,17)
        assert b.getpixel((x,485))==(30,30,30)
    for xy in ((53,484),(51,555),(16,480)):
        assert a.getpixel(xy)==b.getpixel(xy)==(255,255,255)
print('OK: teclado negro tradicional (sprite 7), blancos sin alterar, GPL v2')
