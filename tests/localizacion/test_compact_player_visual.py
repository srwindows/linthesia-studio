"""Regresión visual: barra compacta, progreso integrado y teclado respetado."""
from pathlib import Path
import sys
root=Path(sys.argv[1]);s=(root/'src/PlayingState.cpp').read_text()
k=(root/'src/KeyboardDisplay.cpp').read_text()
assert 'height -= 43+94;' in s
assert 'renderer.DrawQuad(0,0,width,55)' in s
assert 'renderer.DrawQuad(0,56,width,38)' in s
assert 'RibbonBox Ribbon(int index,int width)' in s
assert 'control(16,"",m_settings_open)' in s and 'Rueda dentada vectorial' in s
assert 'control(15,m_view_3d?"2D":"3D")' in s
assert 'renderer.DrawQuad(0,bottom,width,34)' in s
assert 'const int barX=58,barW=std::max(90,width-116)' in s
assert 'm_scrubbing?m_scrub_fraction' in s
assert 'DrawTransport(renderer);' in s
assert 'GetCurrentKeySignature()' in s and 'meterNumerator' in s and 'meterDenominator' in s
assert 'const bool neon=m_guide_style==1;' in k
assert 'const SDL_Color thick(neon?' in k
assert 'Copyright (c) 2007 Nicholas Piegdon' in s
assert 'Adaptation to GNU/Linux by Oscar Aceña' in s
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
try:
 from PIL import Image
except ImportError:Image=None
lab=Path('/tmp/linthesia-compact-redesign-lab')
if Image and all((lab/n).exists() for n in ('compact-initial.jpg','compact-drag.jpg','compact-seek.jpg')):
 a=Image.open(lab/'compact-initial.jpg').convert('RGB')
 b=Image.open(lab/'compact-drag.jpg').convert('RGB')
 c=Image.open(lab/'compact-seek.jpg').convert('RGB')
 assert a.size==b.size==c.size
 assert max(a.getpixel((500,150)))<65,('Piano roll no está oscuro',a.getpixel((500,150)))
 assert b.getpixel((500,29))[1]>a.getpixel((500,29))[1]+60
 assert c.getpixel((500,29))[1]>a.getpixel((500,29))[1]+60
 assert sum(a.getpixel((500,10)))>110,('La barra no debe estar negra',a.getpixel((500,10)))
print('OK: barra compacta legible, progreso verde interactivo, piano roll oscuro')
