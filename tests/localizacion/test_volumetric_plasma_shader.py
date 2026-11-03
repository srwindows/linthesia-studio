from pathlib import Path
import sys
root=Path(sys.argv[1])
r=(root/'src/Renderer.cpp').read_text()
s=r[r.index('struct NoteFlameFilm'):r.index('void Renderer::DrawNeonCapsule')]
p=r[r.index('void Renderer::DrawPianoImpactParticles'):r.index('void Renderer::DrawStageLights')]
assert 'uniform float uTurbulence;' in s
assert 'uniform float uWind;' in s
assert 'turbulenceLocation=getUniformLocation(program,"uTurbulence");' in s
assert 'windLocation=getUniformLocation(program,"uWind");' in s
assert 'uniform1f(turbulenceLocation,turbulence);' in s
assert 'uniform1f(windLocation,wind);' in s
assert 'float occupancy=smoothstep(.43,.77,density);' in s
assert 'float energy=body*texturedCore*localFlicker*travel*lower*' in s
assert 'float front=age*375.0;' in s
assert 'float halo=exp(-pow(distance/max(.01,radius*1.8),2.0));' in s
assert 'float luminance=(.031*halo+.046*glowSoft+.051*core)*' in s
assert 'float travel=exp(-pow((rise-front+15.0)/54.0,2.0));' not in s
assert 'GLboolean depthWriteWasEnabled=GL_TRUE;' in s
assert s.count('glDepthMask(depthWriteWasEnabled);')==1
assert p.count('glDepthMask(depthWriteWasEnabled);')==1
assert 'turbulence,wind);' in p
assert 'm_effect_turbulence/100.f,m_effect_wind/100.f);' in r
assert 'sin(flow*11.8-t*(1.0+.8*uTurbulence))' in s
print('OK: GPU plasma segmented, multiple soft-bloom lobes, live wind/turbulence, no fixed column, GL state restored')
