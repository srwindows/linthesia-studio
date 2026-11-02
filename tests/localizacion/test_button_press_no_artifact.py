"""No superponer rectángulos al pulsar botones con esquinas redondeadas."""
from pathlib import Path
import sys
root=Path(sys.argv[1]); r=(root/'src/Renderer.cpp').read_text()
buttons=(root/'src/MenuLayout.cpp').read_text()
assert 'ObservePressRect' not in r
assert 'pressState.candidate' not in r
assert 'pressState.lastSet' not in r
assert 'DrawQuad(active.x' not in r
assert 'No-op deliberado: no dibujar rectángulos adicionales' in r
assert 'const bool pressed=pressState.held' in r
assert 'w>=34 && w<=245' in r
assert 'glColor4fv(originalColor)' in r
assert 'button.depressed ?' in buttons
assert 'button.y+(button.depressed?1:0)' in buttons
assert 'renderer.DrawQuad' not in buttons.split('void DrawButton(',1)[1].split('void DrawCompactButton(',1)[0]
assert 'renderer.DrawQuad(button.x+edge, y+button.h-6,button.w-2*edge,2);' in buttons
try:
 from PIL import Image,ImageChops
except ImportError: Image=None
p=Path('/tmp/linthesia-16-colors-pressure-lab')
if Image and all((p/n).exists() for n in ('create-hover.jpg','create-pressed.jpg')):
 a=Image.open(p/'create-hover.jpg').convert('RGB')
 b=Image.open(p/'create-pressed.jpg').convert('RGB')
 diff=ImageChops.difference(a,b)
 bounds=diff.getbbox()
 assert bounds is not None
 assert 90 <= bounds[2]-bounds[0] <= 175, bounds
 assert 28 <= bounds[3]-bounds[1] <= 56, bounds
print('OK: pulsación sin rectángulos superpuestos ni invasión del entorno')
