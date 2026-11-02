"""Composición por nota: halo tenue y núcleo cromático sincronizados con MIDI."""
from pathlib import Path
import sys
root=Path(sys.argv[1]); r=(root/'src/Renderer.cpp').read_text(); k=(root/'src/KeyboardDisplay.cpp').read_text()
assert 'uniform float uBloom;' in r
assert 'if(uBloom>.5){' in r
assert 'bloomLocation=getUniformLocation(program,"uBloom");' in r
assert 'uniform1f(bloomLocation,pass==0?1.0f:0.0f);' in r
assert 'glBlendFunc(GL_SRC_ALPHA,pass==0?GL_ONE:GL_ONE_MINUS_SRC_ALPHA);' in r
assert 'float glowSoft=exp(-pow(distance/max(.01,radius),2.0));' in r
assert 'const int plumeW=std::max(40,int(keyWidth*3.0f));' in r
assert 'const int plumeH=std::max(90,std::min(int(windowH*.47f),keyTop-95));' in r
assert 'ImpactFlameEmitter::Sample(' in r
assert 'const float remaining=1.0f;' in r
assert 'const float fuel=1.0f;' in r
assert 'if(note.end<=current_time ||' in k
assert 'if(m_frame_active_keys.count(it->first)==0)' in k
assert 'renderer.DrawPianoImpactParticles(impact.x,impact.y,impact.width,' in k
print('OK: bloom, transparent neon notes, 47%-height impact smoke and real NoteOff')
