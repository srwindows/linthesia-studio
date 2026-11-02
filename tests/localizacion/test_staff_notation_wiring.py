from pathlib import Path
import sys
root=Path(sys.argv[1])
p=(root/'src/PlayingState.cpp').read_text()
m=(root/'src/libmidi/Midi.cpp').read_text()
for needle in ('StaffNotation::Spell(n.note_id,hand,fifths)',
               'StaffNotation::Spell(note.note_id,hand,fifths)',
               'StaffNotation::RhythmFromDuration(',
               'StaffNotation::Stem(value)',
               'StaffNotation::Hollow(value)',
               'StaffNotation::Flags(value)',
               'm_state.midi->GetMusicalBars()',
               'm_state.midi->GetCurrentKeySignature()',
               'm_state.midi->GetCurrentKeyAccidentals()'):
 assert needle in p,needle
assert 'int Midi::GetCurrentKeyAccidentals() const' in m
assert 'GetCurrentTimeSignature()' in p
print('OK: accidentals, key signature, time signature, note value, measure bars, editor alignment')
