from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
"""Export MakeHuman CC0 hand skin into compact, UV-mapped piano palms.
The hands remain anatomically sculpted, while fingers articulate in C++ MIDI.
Source license: assets/scenarios/blender/humano_cc0/LICENSE.ASSETS.md (CC0).
"""
from pathlib import Path
from collections import defaultdict
import math,struct
r=Path(f'{_LINTHESIA_ROOT}')
source=r/'assets/scenarios/blender/humano_cc0/base.obj'
out=r/'graphics/scenarios'
P=[];UV=[];polygons=[];group=''
for line in source.open():
 if line.startswith('v '):P.append(tuple(map(float,line.split()[1:4])))
 elif line.startswith('vt '):UV.append(tuple(map(float,line.split()[1:3])))
 elif line.startswith('g '):group=line[2:].strip()
 elif line.startswith('f ') and group=='body':
  q=[tuple(int(v)-1 for v in tok.split('/')[:2]) for tok in line[2:].split()]
  if all(3.96<P[i][0]<4.56 and 1.60<P[i][1]<3.00
         and .8<P[i][2]<3.40 for i,uv in q):
   polygons.append(q)
assert len(polygons)>350,(len(polygons))
def norm(v):
 l=math.sqrt(sum(k*k for k in v))
 return tuple(k/max(l,1e-10) for k in v)
def cross(a,b):
 return (a[1]*b[2]-a[2]*b[1],
         a[2]*b[0]-a[0]*b[2],
         a[0]*b[1]-a[1]*b[0])
def minus(a,b):return tuple(x-y for x,y in zip(a,b))
def convert(p,hand):
 x,y,z=p
 # Anatomical adult palm: ~9.5cm wide, ~11cm long and ~3.5cm
 # thick. The legacy conversion yielded a grotesque 18cm paddle.
 return ((z-2.18)*(.041 if hand==0 else -.041),
         (x-3.98)*.186,
         (y-2.13)*.026)
for hand in range(2):
 faces=[]
 for polygon in polygons:
  for i in range(1,len(polygon)-1):
   triangle=[polygon[0],polygon[i],polygon[i+1]]
   if hand==1:triangle[1],triangle[2]=triangle[2],triangle[1]
   faces.append(triangle)
 accum=defaultdict(lambda:[0.,0.,0.])
 for tri in faces:
  p0,p1,p2=[convert(P[k],hand) for k,u in tri]
  n=cross(minus(p1,p0),minus(p2,p0))
  for vid,uv in tri:
   for i in range(3):accum[vid][i]+=n[i]
 result=out/('mesh_palma_cc0_%02d.l3d'%hand)
 with result.open('wb') as f:
  f.write(b'L3DM\x03\x00\x00\x00')
  f.write(struct.pack('<I',len(faces)))
  for tri in faces:
   for vid,uv_id in tri:
    pos=convert(P[vid],hand)
    normal=norm(accum[vid])
    u,v=UV[uv_id]
    f.write(struct.pack('<fffBBBfffffB',
              *pos,244,244,244,*normal,u,v,1))
 assert result.stat().st_size==12+len(faces)*3*36
 print('CC0_PALM_MESH',hand,'triangles',len(faces),'size',result.stat().st_size,flush=True)
