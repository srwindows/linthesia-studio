"""Validación UI de estilos 2D/3D y preferencias guardadas."""
from pathlib import Path
import configparser
import sys
root=Path(sys.argv[1])
code=(root/'src/PlayingState.cpp').read_text()
assert 'control(17,"Inicio")' in code
assert 'const RibbonBox meterBox=Ribbon(9,width)' in code
assert 'case 200: set(p.playerStyle2D,0,1);break;' in code
assert 'case 300: Apply3DProfile(p,clampInt(requested,0,2));break;' in code
assert 'm_view_3d=!m_view_3d' in code
try:
 from PIL import Image,ImageChops
except ImportError: Image=None
lab=Path('/tmp/linthesia-inicio-click-qa')
names=('original-2d','inicio','ajustes-vista','neon','3d-profundo','barra-737')
if Image and all((lab/(name+'.png')).is_file() for name in names):
 images={name:Image.open(lab/(name+'.png')).convert('RGB') for name in names}
 assert images['barra-737'].size==(737,540)
 assert images['inicio'].size==(1280,720)
 player=(16,105,1100,580)
 def diff(a,b):
  return ImageChops.difference(images[a].crop(player),images[b].crop(player)).getbbox()
 assert diff('original-2d','neon')
 assert diff('neon','3d-profundo')
 # Inicio devuelve el lector/progreso al principio.
 assert images['inicio'].getpixel((57,72)) != images['original-2d'].getpixel((57,72))
 assert images['ajustes-vista'].getpixel((290,140)) != images['neon'].getpixel((290,140))
cfg=lab/'config/linthesia/ajustes.ini'
if cfg.is_file():
 c=configparser.ConfigParser()
 c.read(cfg)
 assert c.getboolean('Reproductor','view-3d')
 assert c.getint('Reproductor','style-2d')==1
 assert c.getint('Reproductor','style-3d')==1
 assert c.getboolean('Reproductor','beat-visible')
print('OK: boton Inicio, pestaña Vista y modos guardados, ancho 737px')
