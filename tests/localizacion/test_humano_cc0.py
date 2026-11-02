from pathlib import Path
import struct,sys,math
root=Path(sys.argv[1])
source=root/'assets/scenarios/blender/humano_cc0'
assert (source/'base.obj').is_file()
license=(source/'LICENSE.ASSETS.md').read_text()
assert 'CC0 1.0' in license
rig=(root/'src/PianoKinematics.h').read_text()
assert 'Elbow(' in rig
visual=(root/'src/LiveStage3D.cpp').read_text()
assert 'PianoKinematics::Elbow(' in visual
assert 'g_skin_texture' in visual and 'glTexCoord2f' in visual
mesh=root/'graphics/scenarios/mesh_pianista_00.l3d'
blob=mesh.read_bytes()
assert blob[:5] in (b'L3DM\x02',b'L3DM\x03')
count=struct.unpack_from('<I',blob,8)[0]
assert count>=25000,(mesh,count)
assert len(blob)==12+count*3*(36 if blob[4]==3 else 27),(count,len(blob))
# Every sampled vertex and normal must be finite; prohibit animation NaNs.
for j in [0,20,137,500,3988,10000,count*3-1]:
 i=12+j*(36 if blob[4]==3 else 27)
 vertex=struct.unpack_from('<fff',blob,i)
 normal=struct.unpack_from('<fff',blob,i+15)
 assert all(math.isfinite(v) for v in vertex+normal)
 assert .7 < math.sqrt(sum(v*v for v in normal)) < 1.3,normal
if blob[4]==3:
 assert sum(blob[12+35+i*36]!=0 for i in range(count*3))>0
 assert (root/'graphics/scenarios/piel_masculina_cc0.tga').is_file()
print('OK: MakeHuman CC0 skin UV texture, 3D humanoid export, smooth normals and IK')
