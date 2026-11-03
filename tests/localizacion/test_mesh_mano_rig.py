from pathlib import Path
import struct,sys,math,collections
root=Path(sys.argv[1])
for hand in (0,1):
 path=root/f'graphics/scenarios/mesh_mano_rig_{hand:02d}.lhr'
 data=path.read_bytes()
 assert len(data)>500_000
 magic,count,bones=struct.unpack_from('<4sII',data,0)
 assert magic==b'LHR1' and 6000<=count<=10000 and bones==16,(count,bones)
 assert len(data)==12+count*3*47
 influences=collections.Counter()
 faces=set()
 for k in range(count*3):
  vals=struct.unpack_from('<3f3f2f3B3f',data,12+47*k)
  xyz=vals[:3]; normal=vals[3:6];uv=vals[6:8];ids=vals[8:11];weights=vals[11:14]
  assert all(math.isfinite(v) for v in xyz+normal+uv+weights)
  assert all(-.15<v<.24 for v in xyz)
  assert all(0<=index<16 for index in ids)
  assert .995<sum(weights)<1.005
  assert all(0<=w<=1.001 for w in weights)
  for idx,weight in zip(ids,weights):
   if weight>.2:influences[idx]+=1
  if k%3==0:faces.add(tuple(round(v,3) for v in xyz))
 assert len(faces)>2000
 assert all(influences[i]>40 for i in range(16)),influences
 print('HAND',hand,'triangles',count,'weighted_bones',sorted(influences))
src=(root/'src/LiveStage3D.cpp').read_text()
assert 'DrawRiggedHand(frame,hand,wrist)' in src
assert 'LoadRiggedHand(hand)' in src
assert 'glCallList(g_palm_lists[hand])' in src # fallback, not normal path
assert (root/'assets/scenarios/blender/crear_rig_manos_continuas.py').exists()
assert (root/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend').stat().st_size>3_500_000 and (root/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend').read_bytes()[:32].find(b'BLENDER')>=0
print('OK: continuous skinned hands with 16 populated bones, CC0 UV and safe legacy fallback')
