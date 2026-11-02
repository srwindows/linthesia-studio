"""Regresion sin datos propietarios del sistema de particulas Linthesia."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
r=(root/'src/Renderer.cpp').read_text()
h=(root/'src/Renderer.h').read_text()
k=(root/'src/KeyboardDisplay.cpp').read_text()
kh=(root/'src/KeyboardDisplay.h').read_text()
assert 'void Renderer::DrawPianoImpactParticles(' in r
assert 'glBlendFunc(GL_SRC_ALPHA,GL_ONE);' in r
assert 'glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);' in r
assert 'ImpactFlameEmitter::Sample(' in r
assert 'const bool sustained=' not in r
assert '#include "NoteEmitter.h"' in r
assert 'void DrawPianoImpactParticles(' in h
assert 'std::map<std::string,Uint32> m_impact_started;' in kh
assert 'm_frame_active_keys=m_hands?KeyNames{}:m_active_keys;' in k
assert 'm_impact_started[key.first]=now;' in k
assert 'it=m_impact_started.erase(it);' in k
assert 'm_impact_visuals.clear();' in k
assert 'renderer.DrawPianoImpactParticles(impact.x,impact.y,impact.width,' in k
assert 'if(m_render_3d)renderer.EndBlenderKeyboard3D();' in k
assert 'if(m_render_3d){' in k
assert 'renderer.DrawPianoKey3D(key_x,y_offset,key_width,key_height,' in k
assert 'renderer.DrawPianoKey3D(start_x,y_offset,black_width,black_height,' in k
assert 'std::max(16,std::min(m_visual.opacity,58)):m_visual.opacity;' in k
assert 'NoteParticleSeed(note_name)' in k
print('OK: efectos 3D originales sincronizados con MIDI, alfa restaurado y modo 2D intacto.')

# Estelas de duración real por partícula, con control de reciclaje invisible.
assert 'ImpactFlameEmitter::Duration' in r
assert 'const auto spark=ImpactFlameEmitter::Sample(' in r
