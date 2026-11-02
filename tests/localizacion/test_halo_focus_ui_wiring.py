from pathlib import Path
import sys
r=Path(sys.argv[1])
p=(r/'src/PlayingState.cpp').read_text()
h=(r/'src/PlayingState.h').read_text()
kb=(r/'src/KeyboardDisplay.cpp').read_text()
for token in (
 'm_score_tracks=PianoScoreTracks::Choose(tracks)',
 'm_score_tracks.count(n.track_id)==0',
 'm_score_tracks.count(n.track_id) &&',
 'm_keyboard->Draw(renderer, key_tex, note_tex',
 'visibleNotes, m_show_duration',
 'if(e.id==124||e.id==127)',
 'm_required_notes.clear();',
 'SetupNoteState();',
 'case 124: set(p.practiceHands,0,2);break;',
 'RebuildHandClassification();',
 'g.step=7;',
 'g.bottom[1]=g.top+199;',
 'renderer.SetColor(9,25,40,view.expanded?250:243);',
):
 assert token in p,token
assert 'std::set<size_t> m_score_tracks' in h
assert 'm_visual.glow, // restore actual fluorescence slider' in kb
print('OK Halo: piano-only grand staff, visually focused 3D notes, direct hand selector, readable notation')
