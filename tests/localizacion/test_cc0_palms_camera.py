from pathlib import Path
import math,struct,sys
root=Path(sys.argv[1])
assets=root/'graphics/scenarios'
for hand in (0,1):
 blob=(assets/f'mesh_palma_cc0_{hand:02d}.l3d').read_bytes()
 assert blob[:5]==b'L3DM\x03'
 tris,=struct.unpack_from('<I',blob,8)
 assert 600<tris<1800
 assert len(blob)==12+tris*108
 uvs=set()
 bounds=[[1e4,-1e4] for _ in range(3)]
 for n in range(tris*3):
  x,y,z,r,g,b,nx,ny,nz,u,v,flags=struct.unpack_from(
         '<fffBBBfffffB',blob,12+36*n)
  assert all(math.isfinite(f) for f in (x,y,z,nx,ny,nz,u,v))
  assert (flags&1)==1 and 0<=u<=1 and 0<=v<=1
  assert -0.15<x<0.15 and -.01<y<0.15 and -.05<z<.05
  for axis,value in enumerate((x,y,z)):
   bounds[axis][0]=min(bounds[axis][0],value)
   bounds[axis][1]=max(bounds[axis][1],value)
  uvs.add((round(u,3),round(v,3)))
 assert len(uvs)>70,len(uvs)
 widths=[hi-lo for lo,hi in bounds]
 assert .09<widths[0]<.11,('palm too wide',widths)
 assert .10<widths[1]<.12,('palm length incorrect',widths)
 assert .025<widths[2]<.042,('palm too thick',widths)
assert (root/'assets/scenarios/blender/humano_cc0/LICENSE.ASSETS.md').is_file()
blend=root/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'
assert blend.stat().st_size>15_000_000
src=(root/'src/LiveStage3D.cpp').read_text()
assert 'InitPalmMeshes()' in src
assert 'LoadPalmMesh(hand,triangles)' in src
assert 'glCallList(g_palm_lists[hand])' in src
assert 'CinematicCamera::Sample(' in src
hands=(root/'src/PianoHands.h').read_text()
assert 'cost-=200.f' in hands
assert 'if(old.note==note)' in hands
print('OK: UV human palms, dynamic MIDI fingers, real 3D camera, CC0 source')
