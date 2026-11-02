"""Regresión: piano pequeño, escala real, HUD y reconocimiento tras seek."""
from pathlib import Path
import sys
p=Path(sys.argv[1]); cpp=(p/'src/PlayingState.cpp').read_text()
head=(p/'src/PlayingState.h').read_text()
keyboard=(p/'src/KeyboardDisplay.cpp').read_text()
mid=(p/'src/libmidi/Midi.cpp').read_text()
assert 'MaxKeyboardRatio = std::max(10,std::min(25,pianoPercent))/100.0' in keyboard
assert 'height -= 43+94;' in cpp
assert 'Layout::ScreenMarginX, 94, visibleNotes' in cpp
assert 'const auto& visibleNotes=' in cpp
assert 'tempoNumber<<Text(STRING(EffectiveTempoBPM())' in cpp
assert 'const RibbonBox meterBox=Ribbon(9,width);' in cpp
assert 'std::to_string(i+1)' in cpp
assert 'const std::string musicalPosition=midi.GetMusicalPosition(preview);' in cpp
assert 'lcd(12,"Acorde",m_current_chord,m_midi_preferences.playerChord)' in cpp
assert 'lcd(10,"Lector",STRING(currentBar<<"/"<<totalBars)' in cpp
assert 'midi.GetMusicalBars()' in cpp
assert 'RefreshChord(true)' in cpp
assert 'RefreshChord();' in cpp
assert 'if(m_scrubbing)' in cpp
assert 'm_scrubbing=true;' in cpp
assert 'm_chord_pending_since' in head
assert 'GetMusicalPosition' in mid and 'm_musical_bars.push_back' in mid
assert 'GNU GENERAL PUBLIC LICENSE' in (p/'COPYING').read_text()
try:
 from PIL import Image,ImageChops
except ImportError: Image=None
lab=Path('/tmp/linthesia-prof-player-visual')
if Image and all((lab/(n+'.jpg')).exists() for n in ('initial','dragging','seek','paused')):
 image=lambda n: Image.open(lab/(n+'.jpg')).convert('RGB')
 a=image('initial');b=image('dragging');c=image('seek');d=image('paused')
 assert a.size==b.size==c.size==d.size
 assert max(a.getpixel((200,250)))<65,'roll oscuro sin bloques extra'
 assert a.getpixel((500,70))[1] < b.getpixel((500,70))[1], 'barra verde responde al arrastre'
 assert b.getpixel((500,70))[1]==c.getpixel((500,70))[1]
 assert ImageChops.difference(c,d).getbbox() is not None
lab=Path('/tmp/linthesia-prof-chord-visual')
if Image and all((lab/(n+'.jpg')).exists() for n in ('chord-c','chord-after-seek')):
 a=Image.open(lab/'chord-c.jpg').convert('RGB')
 b=Image.open(lab/'chord-after-seek.jpg').convert('RGB')
 assert ImageChops.difference(a,b).getbbox() is not None
print('OK: piano reducido, compases reales, LCD y acorde tras arrastrar')
