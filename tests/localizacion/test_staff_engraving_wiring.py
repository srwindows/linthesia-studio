from pathlib import Path
import sys
r=Path(sys.argv[1])
p=(r/'src/PlayingState.cpp').read_text()
h=(r/'src/PlayingState.h').read_text()
alg=(r/'src/StaffEngraving.h').read_text()
for needle in (
    'StaffRenderData PlayingState::PrepareStaffRender(',
    'staff=PrepareStaffRender(scoreStart,horizon)',
    'StaffEngraving::Build(scoreNotes,std::move(bars)',
    'glyph.note.source',
    'glyph.start,bpm,m_midi_preferences.notationQuantize',
    'if(accidentalMark && !glyph.tieIn)',
    'if(!glyph.beamPrev&&!glyph.beamNext)',
    'for(const auto& rest:staff.layout.rests)',
    'if(!a.tieOut||!geometry[i].visible)continue;',
    'if(!a.beamNext || !geometry[i].visible)continue;'
):
 assert needle in p,needle
assert 'struct StaffRenderData' in h
assert 'StaffEngraving::Layout' in h
assert 'result.rests.push_back' in alg
assert 'g.tieIn=cursor>n.start' in alg
assert 'g.tieOut=until<n.end' in alg
print('OK: editor and renderer share MIDI engraving layout with voices, rests, ties and beams')
