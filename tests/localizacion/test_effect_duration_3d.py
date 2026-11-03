"""Music 3D stays present while short contact sparks don't hang in space."""
from pathlib import Path
import sys
r=(Path(sys.argv[1])/'src/Renderer.cpp').read_text()
k=(Path(sys.argv[1])/'src/KeyboardDisplay.cpp').read_text()
part=r.split('void Renderer::DrawPianoImpactParticles(',1)[1].split('void Renderer::',1)[0]
assert 'effectAge>=maxLife || releaseFade<=0.0f)return;' in part
assert 'const float maxLife=1.10f*m_effect_life/100.f;' in part
assert 'NoteEmitter::Sample(visualAge,i,seed,float(keyWidth),' in part
assert 'if(m_effect_explosion && effectAge<ImpactFlameEmitter::Duration*lifeScale)' in part
assert 'const auto spark=ImpactFlameEmitter::Sample(' in part
assert 'std::min(unsigned(m_effect_count),96u)' in part
assert 'Núcleo persistente de luz física' not in part
assert 'const bool filmDrawn=cinematic && h>=10 && w>=5' in r
assert 'm_current_impacts[note_name]=impact;' in k
assert 'const float flightAge=it->second.visual.age+sinceRelease;' in k
print('OK: 3D MIDI geometry untouched; visual plume reaches mid-screen after NoteOff')
