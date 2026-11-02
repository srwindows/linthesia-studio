from pathlib import Path
import sys
r=Path(sys.argv[1])
s=(r/'src/PlayingState.cpp').read_text()
h=(r/'src/PlayingState.h').read_text()
t=(r/'src/StatsState.cpp').read_text()
lib=(r/'src/PracticeHistory.cpp').read_text()
for x in ('SavePracticeAttempt(false);','SavePracticeAttempt(true);',
          'PracticeHistory::Append(m_state.song_path,a)','m_practice_saved=true'):
 assert x in s,x
assert s.count('SavePracticeAttempt(false);')==2
assert s.count('SavePracticeAttempt(true);')==1
assert 'm_practice_saved=false' in h
for x in ('PracticeHistory::Load(m_state.song_path)','HISTORIAL DE PRÁCTICA',
          'Variación reciente:','a.Accuracy()','a.complete'):
 assert x in t,x
assert 'g_get_user_data_dir()' in lib
assert 'g_file_set_contents' in lib
assert 'if(items.size()>50)' in lib
print('OK: completed and abandoned performances persisted and displayed in statistics')
