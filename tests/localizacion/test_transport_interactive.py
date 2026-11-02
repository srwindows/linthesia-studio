"""Barra interactiva: arrastre, reloj MIDI y ausencia de notas colgadas."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
s=(root/'src/PlayingState.cpp').read_text()
h=(root/'src/PlayingState.h').read_text()
m=(root/'src/libmidi/Midi.cpp').read_text()
e=(root/'src/libmidi/MidiEvent.cpp').read_text()
assert 'void PlayingState::SeekTo(microseconds_t target)' in s
assert 'm_state.midi_out->Reset()' in s
assert 'm_state.midi_in->Reset()' in s
assert 'm_active_notes.clear()' in s
assert 'm_pressed_notes.clear()' in s
assert 'm_required_notes.clear()' in s
assert 'm_keyboard->ResetActiveKeys()' in s
assert 'm_state.midi->GoTo(target)' in s
assert 'eraseUntilTime(target)' in s
assert 'MidiEventType_ProgramChange' in s and 'MidiEventType_Controller' in s
assert 'm_scrubbing=true;' in s and 'm_scrubbing=false;' in s
assert 'if(m_paused || m_scrubbing || transport_changed)' in s or 'if (m_paused || m_scrubbing || transport_changed)' in s
assert 'const int barX=58,barW=std::max(90,width-116)' in s
assert 'HandleTransport(Mouse(),transport_changed)' in s
assert 'DrawTransport(renderer)' in s
assert 'if(mouse.newPress.left && onBar)' in s
assert 'if(mouse.released.left)' in s
assert 'm_scrub_fraction' in h
assert 'GetCurrentBPM' in m and 'GetCurrentTimeSignature' in m and 'GetCurrentKeySignature' in m
assert 'case MidiMetaEvent_KeySignature:' in e
assert 'case MidiMetaEvent_TimeSignature:' in e
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
lab=Path('/tmp/linthesia-compact-redesign-lab')
try:
 from PIL import Image
except ImportError: Image=None
if Image and all((lab/(n+'.jpg')).is_file() for n in ('compact-initial','compact-drag','compact-seek','compact-paused','compact-resumed')):
 before=Image.open(lab/'compact-initial.jpg').convert('RGB')
 drag=Image.open(lab/'compact-drag.jpg').convert('RGB')
 seek=Image.open(lab/'compact-seek.jpg').convert('RGB')
 assert before.size==drag.size==seek.size
 assert drag.getpixel((500,29))!=before.getpixel((500,29)), 'Scrub preview did not move progress'
 assert seek.getpixel((500,29))==drag.getpixel((500,29)), 'Seek release did not update progress'
print('OK: scrub, seek MIDI sincronizado, tempo/metrica/tonalidad, licencia GPLv2')
