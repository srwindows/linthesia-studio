from pathlib import Path
import struct,sys,math
root=Path(sys.argv[1])
f=root/'graphics/scenarios/mesh_pianista_00.l3d'
b=f.read_bytes()
assert b[:5]==b'L3DM\x03'
tri,=struct.unpack_from('<I',b,8)
assert len(b)==12+tri*3*36
tex=head=other=0
uvsamples=set()
for i in range(tri*3):
 offset=12+i*36
 x,y,z=struct.unpack_from('<fff',b,offset)
 u,v,flag=struct.unpack_from('<ffB',b,offset+27)
 assert all(math.isfinite(vv) for vv in (x,y,z,u,v))
 assert (flag&~3)==0,flag
 if flag&1:
  tex+=1
  uvsamples.add((round(u,3),round(v,3)))
  assert flag&2
  assert 0<=u<=1 and 0<=v<=1
 if flag&2:head+=1
 else:other+=1
assert tex>5000 and head>tex and other>tex,(tex,head,other)
assert len(uvsamples)>1000,len(uvsamples)
texture=root/'graphics/scenarios/piel_masculina_cc0.tga'
assert texture.stat().st_size>=12*1024*1024
m=(root/'src/PianoHands.h').read_text()
renderer=(root/'src/LiveStage3D.cpp').read_text()
for keyword in ('headNod','headYaw','shoulderRise'):
 assert keyword in m and keyword in renderer
assert 'g_head_lists' in renderer
assert 'cameraFocus' in renderer
assert 'camara-enfoque' in (root/'src/AdvancedSettings.cpp').read_text()
assert 'glCallList(g_head_lists[index])' in renderer
assert 'scenarios/piel_masculina_cc0' in renderer
print('OK: piel fotográfica 2048, UV MakeHuman y cabeza animada en malla OpenGL')
