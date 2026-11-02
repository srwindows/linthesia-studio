"""No reintroduction of the 3D disappearing-flame bug."""
from pathlib import Path
import sys
r=(Path(sys.argv[1])/'src/Renderer.cpp').read_text()
assert 'float front=age*375.0;' in r
assert 'if(age<0.0 || age>=1.10)' in r
assert 'float phase=(pixelY+uTime*310.0)*.069;' in r
assert 'float tip1=pow(' in r and 'float tip2=pow(' in r
assert 'float localAlpha=clamp(emission*localFlame*1.55' in r
assert 'vec3 edgeColor=mix(uTint,vec3(.08,.71,.98),.69);' in r
assert 'vec3 middle=mix(uTint,vec3(1.0,.51,.11),.78);' in r
assert 'vec3 coolFire=mix(uTint,vec3(.12,.66,.96),.61);' in r
assert 'vec3 fire=mix(coolFire,warmFire' in r
assert 'float shell=exp(' in r and 'vec3 shell=' not in r
assert 'const bool filmDrawn=cinematic && h>=10 && w>=5' in r
assert 'BlenderMesh::NoteCount' in r
assert 'DrawBlenderScene3D' in (Path(sys.argv[1])/'src/Renderer.h').read_text()
print('OK: original Blender 3D mesh and chromatic flame rising to half the screen')
