from pathlib import Path
import sys
root=Path(sys.argv[1]); p=(root/'src/PlayingState.cpp').read_text()
selection=(root/'src/TrackSelectionState.cpp').read_text()
ui=(root/'src/PlayingVideoExport.cpp').read_text()
transport=p.split('void PlayingState::DrawTransport(',1)[1].split('void PlayingState::',1)[0]
handler=p.split('bool PlayingState::HandleTransport(',1)[1].split('void PlayingState::',1)[0]
settings=p.split('void PlayingState::DrawPlayerSettings(',1)[1].split('void PlayingState::',1)[0]
assert 'const char* hands[3]' not in transport and 'Escuchar canción completa' not in transport and 'Mostrar partitura' not in transport
assert 'Azul: izquierda' not in transport
assert 'Exportar vídeo' not in transport
assert 'Detener vídeo' not in transport
assert 'mouse.x>=17 && mouse.x<334' not in handler
assert 'videoX=' not in handler and 'mouse.y>=110 && mouse.y<145' not in handler
assert 'm_record_video_button.Update' in selection
assert 'm_record_video_button.hit' in selection
assert '"Exportar vídeo del escenario"' in selection
assert 'ChangeState(new PlayingState(m_state,true));' in selection
assert 'if(m_video_prompt_on_enter)OpenVideoDialog();' in p
assert 'm_video_prompt_on_enter=false;' in ui
assert 'ChangeState(new TrackSelectionState(m_state));' in ui
assert 'exporting?"Finalizar vídeo":"Exportar vídeo"' in settings
assert 'OpenVideoDialog();' in p
assert 'IsKeyPressed(KeyF6)' in p and 'IsKeyPressed(KeyF2)' in p
assert 'case 2: // Evaluación' in p and 'Manos a practicar' in p
print('OK: no extra video/hand controls in scene; recorder on track-selection page')
