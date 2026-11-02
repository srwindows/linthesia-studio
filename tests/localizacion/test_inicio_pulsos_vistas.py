"""Regresión de Inicio, contador de pulsos y 4 modos de representación."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
s=(root/'src/PlayingState.cpp').read_text()
k=(root/'src/KeyboardDisplay.cpp').read_text()
r=(root/'src/Renderer.cpp').read_text()
cfg=(root/'src/AdvancedSettings.cpp').read_text()
assert 'control(17,"Inicio")' in s
assert 'if(hit(17)){' in s
assert 'GetSongInitialPositionInMicroseconds()' in s
assert 'SeekTo(first)' in s
assert 'const RibbonBox meterBox=Ribbon(9,width)' in s
assert 'const unsigned int currentBeat=' in s
assert 'std::to_string(i+1)' in s
assert 'case 401: set(p.playerBeatMode,0,1);break;' in s
assert 'case 400: p.playerBeatIndicator=flag;break;' in s
assert 'meterNumerator' in s and 'meterDenominator' in s
assert 'case 200: set(p.playerStyle2D,0,1);break;' in s
assert 'case 300: Apply3DProfile(p,clampInt(requested,0,2));break;' in s
assert 'm_midi_preferences.playerStyle2D' in s
assert 'm_midi_preferences.playerStyle3D' in s
assert 'neon=m_guide_style==1' in k
assert 'perspectiveStyle==1?1.16f:0.86f' in r
assert 'k=std::min(k,0.42f/height);' in r
assert 'I("Reproductor","style-2d",s.playerStyle2D)' in cfg
assert 'I("Reproductor","style-3d",s.playerStyle3D)' in cfg
assert 'I("Reproductor","beat-mode",s.playerBeatMode)' in cfg
assert 'B("Reproductor","beat-visible",s.playerBeatIndicator)' in cfg
assert 'PracticeError::WrongPress' in s
assert 'm_state.stats.stray_notes++' in s
assert 'm_missed_notes+m_state.stats.stray_notes' not in s
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
try:
 from PIL import Image,ImageChops
except ImportError:
 Image=None
lab=Path('/tmp/linthesia-inicio-vistas')
modes=['2d-clasico','2d-neon','3d-estudio','3d-profundo']
if Image and all((lab/m/'player.png').is_file() for m in modes):
 shots={m:Image.open(lab/m/'player.png').convert('RGB') for m in modes}
 assert all(im.size==(1280,720) for im in shots.values())
 roll=(45,104,1100,570)
 def distinct(a,b):
  return ImageChops.difference(shots[a].crop(roll),shots[b].crop(roll)).getbbox() is not None
 assert distinct('2d-clasico','2d-neon')
 assert distinct('2d-clasico','3d-estudio')
 assert distinct('3d-estudio','3d-profundo')
 start=Image.open(lab/'2d-clasico'/'inicio.png').convert('RGB')
 assert ImageChops.difference(shots['2d-clasico'].crop((590,12,655,43)),
                              start.crop((590,12,655,43))).getbbox()
 print('OK: 4 modos visualmente distintos, pulsos, Inicio y preferencias')
else:
 print('OK: fuentes, sin capturas disponibles')
