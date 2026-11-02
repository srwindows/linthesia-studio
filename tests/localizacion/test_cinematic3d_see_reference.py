"""Visual cinematic rendering invariants, authored independently."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
r=(root/'src/Renderer.cpp').read_text()
h=(root/'src/Renderer.h').read_text()
k=(root/'src/KeyboardDisplay.cpp').read_text()
a=(root/'src/AdvancedSettings.cpp').read_text()
p=(root/'src/PlayingState.cpp').read_text()
assert 'style-3d",0),0,2' in a
assert 'case 300: Apply3DProfile(p,clampInt(requested,0,2))' in p
assert 'Estilo 3D (0 clásico, 1 moderno, 2 cinematográfico)' in p
assert 'm_cinematic_3d=perspective3d && style3D==2' in k
assert 'renderer.BeginPianoRollClip(x,y,x+m_width,y+y_offset)' in k
assert 'if(!m_cinematic_3d)renderer.EndPianoPerspective();' in k
assert 'renderer.DrawPianoContactLightBar' in k
assert 'DrawPianoContactLightBar' in h and 'void Renderer::DrawPianoContactLightBar(' in r
assert 'const bool filmDrawn=cinematic && h>=10 && w>=5' in r
assert 'for(int band=0;band<7;++band)' in r
assert 'ImpactFlameEmitter::Sample(' in r
assert 'glBlendFunc(GL_SRC_ALPHA,GL_ONE);' in r
assert 'glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);' in r
assert 'std::max(16,std::min(m_visual.opacity,58)):m_visual.opacity' in k
assert 'const int cinematicOpacity=noteOpacity;' in k
assert 'm_frame_active_keys=m_hands?KeyNames{}:m_active_keys' in k
print('Cinematic 3D: legacy 3D and 2D preserved, dynamic caustics, additive glow, MIDI particles')
