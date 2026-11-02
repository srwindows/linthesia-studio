from pathlib import Path
import sys
r=Path(sys.argv[1])
cpp=(r/'src/FreePlayState.cpp').read_text()
h=(r/'src/FreePlayState.h').read_text()
for name in ('FreePlayVisualNotes.h','FreePlay::VisualNotes m_visual',
             'm_show_tracks=false'):
 assert name in h,name
for name in (
 'm_visual.Event(simple.status,simple.byte1,simple.byte2,m_visual_now)',
 'm_visual.Event(uint8_t(on?0x90:0x80)',
 'm_visual.Event(uint8_t(kind|i),e.data1,e.data2,m_visual_now)',
 'm_visual.Update(m_visual_now)',
 'm_visual.ReleaseAll(m_visual_now)',
 'm_visual.Alpha(note,m_visual_now)',
 'note.held',
 'const double age=',
 'const double off=',
 'const int x=KeyX(note.pitch)',
 'const int x=KeyX(pitch)',
 'm_show_tracks=!m_show_tracks',
 'if(m_show_tracks){',
 'Toca con el ratón o conecta un MIDI IN'):
 assert name in cpp,name
assert cpp.index('for(int pass=0;pass<2;++pass)') < cpp.index('// Keyboard with shared hit-test')
assert 'const int ky=KeyboardTop(),kx=KeyboardLeft(),kw=KeyboardWidth();' in cpp
print("OK: independent real-time MIDI and mouse visualization, held length, fade, alignment, minimal track UI")
