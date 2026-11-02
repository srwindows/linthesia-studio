from pathlib import Path
import sys
r=Path(sys.argv[1])
source=(r/'src/TrackSelectionState.cpp').read_text()
header=(r/'src/TrackSelectionState.h').read_text()
auto=(r/'src/HandAutoDetector.h').read_text()
checks=[
 'void TrackSelectionState::OpenHandsDetector(){',
 'void TrackSelectionState::HandleDetectorModalClick(int id)',
 'void TrackSelectionState::ApplyHandsDetection(){',
 'void TrackSelectionState::DetectHandsFromSelectedTracks(){',
 'void TrackSelectionState::Draw(Renderer &renderer) const',
 'm_detector_open=true;',
 'm_detector_preview_ready=false;',
 'm_detector_automatic=true;',
 'm_detector_source_piano=true;',
 'm_detector_split=AdvancedSettingsIO::Load().handSplitPoint;',
 'm_detector_feedback="Pulsa Analizar primero; puedes cancelar sin cambios.";',
 'if(!m_detector_preview_ready)',
 'updated.handSplitPoint=m_detector_preview_split;',
 'if(!AdvancedSettingsIO::Save(updated,error))',
 'm_detector_open=false;',
 'm_modal_buttons[id].Update(mouse);',
 'if(m_modal_buttons[id].hit)',
 'HandleDetectorModalClick(id);',
 'if(IsKeyPressed(KeyEscape)){m_detector_open=false;return;}',
 'OpenHandsDetector();',
 'DetectHandsFromSelectedTracks();',
 'ApplyHandsDetection();',
 '"Configura la detección antes de aplicarla; el MIDI no cambia."',
 '"Piano seleccionado","Todas activas"',
 '"Automática","Manual"',
 '"Analizar","Aplicar","Cancelar"',
 'HandCorrectionStore::LoadAll(m_state.song_path)',
 'TextWriter cut(',
 'renderer.SetColor(255,255,255);',
]
for t in checks: assert t in source,t
for t in ('bool m_detector_open=false;', 'bool m_detector_preview_ready=false;',
          'bool m_detector_automatic=true;', 'int m_detector_split=60;',
          'void ApplyHandsDetection();'):
 assert t in header,t
assert 'bool automatic=true' in auto
assert 'if(automatic)out.splitPoint=SuggestSplit' in auto
opener=source.split('void TrackSelectionState::OpenHandsDetector(){',1)[1].split('void TrackSelectionState::HandleDetectorModalClick(',1)[0]
preview=source.split('void TrackSelectionState::DetectHandsFromSelectedTracks(){',1)[1].split('void TrackSelectionState::ApplyHandsDetection(){',1)[0]
assert 'AdvancedSettingsIO::Save(' not in opener
assert 'AdvancedSettingsIO::Save(' not in preview
assert source.count('AdvancedSettingsIO::Save(updated,error)')==1
assert 'OpenHandsDetector();' in source and 'DetectHandsFromSelectedTracks();' in source
print('OK: opening or analyzing never saves, Apply only after preview, Cancel/Esc non-destructive')
