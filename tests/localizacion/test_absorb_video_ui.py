from pathlib import Path
import sys
src=Path(sys.argv[1])/'src'
k=(src/'KeyboardDisplay.cpp').read_text()
p=(src/'PlayingState.cpp').read_text()
s=(src/'TrackSelectionState.cpp').read_text()
h=(src/'ShortNoteVisual.h').read_text()
assert 'ShortNoteVisual::Absorb(y+y_offset,limit,' in k
assert 'if(height<=0)continue;' in k
assert 'const auto compact=(sounding||releasing)?' in k
assert 'pertenece MIDI' not in k
assert 'const int consumed=static_cast<int>(std::floor(elapsedSeconds*230.f));' in h
assert '"Exportar vídeo del escenario"' in s
assert 'm_record_video_button.hit' in s
assert 'mouse.released.left && videoButton.contains(mouse.x,mouse.y)' in p
assert 'ClosePlayerSettings();' in p and 'OpenVideoDialog();' in p
assert 'exporting?"Finalizar vídeo":"Exportar vídeo"' in p
assert 'm_video_prompt_on_enter' in p
print('OK: notes absorbed into keys, export accessible in both track selection and Ajustes')
