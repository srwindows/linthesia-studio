from pathlib import Path
import sys
root=Path(sys.argv[1])
r=(root/'src/Renderer.cpp').read_text()
k=(root/'src/KeyboardDisplay.cpp').read_text()
h=(root/'src/Renderer.h').read_text()
assert 'float hitAge=-1.0f' in h
assert 'const float remaining=1.0f;' in r
assert 'sounding?static_cast<float>((current_time-i->start)*.000001):-1.0f' in k
assert 'for(int band=0;band<7;++band)' in r
assert 'for(int i=0;i<12;++i)' in r
assert 'Extra brief ignition synchronized with each NoteOn' in r
assert 'const bool sustained=' not in r
assert 'ImpactFlameEmitter::Sample(' in r
assert 'const float sx=cx+spark.x+m_xoffset;' in r
assert 'i*2.39996323f' not in r
assert 'glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);' in r
assert 'gl_FragColor=vec4(1.0,0.08,vUv.y,0.95);' not in r
assert 'Linthesia GLSL flame: shader activo' in r
assert 'vec2 uv=vec2((gl_FragCoord.x-uOrigin.x)/uExtent.x,' in r
assert 'float pixelY=uv.y*uExtent.y;' in r
assert 'for(int i=0;i<blobs;++i)' in r
assert 'if(cinematic && h>=18 && w>=5 && glow>0 && !filmDrawn)' in r
assert 'const float remaining=1.0f;' in r
print('OK: luz dentro de cada nota, envoltura, liberación única y sin órbitas')
