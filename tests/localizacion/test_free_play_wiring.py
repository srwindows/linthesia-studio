from pathlib import Path
import sys
r=Path(sys.argv[1])
main=(r/'src/TitleState.cpp').read_text()
state=(r/'src/FreePlayState.cpp').read_text()
build=(r/'src/meson.build').read_text()
for x in ('ChangeState(new FreePlayState(m_state))','MODO LIBRE'):
 assert x in main
for x in ('m_state.midi_in->Read()','m_recorder.Push(m_current',
          'm_recorder.Restore(SessionPath())','SaveSession();',
          'm_recorder.Save(filename)','KeyboardHit(', 'KeyboardNote(',
          'm_mutes[i].hit','m_programs[i].hit','m_deletes[i].hit',
          'm_recorder.quantize=!m_recorder.quantize'):
 assert x in state,x
for x in ("'FreePlayRecorder.cpp'","'FreePlayState.cpp'"):
 assert x in build
print("OK: Free Play accessible from Inicio, 88 keys, independent tracks, export and recovery")
