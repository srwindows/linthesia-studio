from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
# Anatomical MakeHuman CC0 hands -> Linthesia 16-bone skinned source meshes
# Derived only from bundled base.obj, with its original mesh topology and UVs.
# Usage: python3 exportar_manos_humanas_nativas.py
from pathlib import Path
import numpy as np,struct,math
R=Path(f'{_LINTHESIA_ROOT}')
src=R/'assets/scenarios/blender/humano_cc0/base.obj'
out=Path('/tmp/linthesia-native-hands');out.mkdir(exist_ok=True)
v=[];uv=[];groups={};body=[];active=''
for line in src.open():
 if line.startswith('v '):v.append(tuple(map(float,line.split()[1:4])))
 elif line.startswith('vt '):uv.append(tuple(map(float,line.split()[1:3])))
 elif line.startswith('g '):active=line[2:].strip()
 elif line.startswith('f '):
  parts=[(int(t.split('/')[0])-1,int(t.split('/')[1])-1) for t in line[2:].split()]
  if active=='body':body.extend([[parts[0],parts[i],parts[i+1]] for i in range(1,len(parts)-1)])
  elif active.startswith('joint-'):groups.setdefault(active,[]).extend([a for a,u in parts])
v=np.array(v);uv=np.array(uv)
rest=np.array([
 [[-.032,.054,0],[-.053,.075,-.003],[-.069,.090,-.005],[-.078,.101,-.006]],
 [[-.022,.076,.005],[-.024,.111,.006],[-.024,.139,.003],[-.022,.160,0]],
 [[0,.083,.006],[-.001,.123,.007],[0,.155,.003],[0,.180,-.001]],
 [[.021,.077,.004],[.025,.113,.005],[.027,.140,.002],[.026,.160,-.001]],
 [[.040,.069,.002],[.046,.094,.003],[.048,.119,0],[.048,.139,-.002]]])
def distance(q,a,b):
 vec=b-a;v=q-a
 t=np.clip(np.einsum('ij,ij->i',v,vec)/np.einsum('ij,ij->i',vec,vec),0,1)
 return np.linalg.norm(a+vec*t[:,None]-q,axis=1)
for hand,side in ((0,'l'),(1,'r')):
 srcj=np.array([v[groups[f'joint-{side}-finger-{digit+1}-{j+1}']].mean(axis=0) for digit in range(5) for j in range(4)])
 dstj=rest.copy();dstj[:,:,0]*=(-1 if hand==0 else 1)
 dstj=dstj.reshape(-1,3)
 A=np.column_stack((srcj,np.ones(len(srcj))))
 mat=np.linalg.lstsq(A,dstj,rcond=None)[0]
 bone_a=srcj.reshape(5,4,3)[:,:-1,:].reshape(-1,3)
 bone_b=srcj.reshape(5,4,3)[:,1:,:].reshape(-1,3)
 used=[]
 for f in body:
  centroid=v[[index for index,uvix in f]].mean(axis=0)
  if ((centroid[0]>3.65) if hand==0 else (centroid[0]<-3.65)) and min(distance(centroid,bone_a,bone_b))<.36:
   used.append(f)
 points={index for f in used for index,u in f}
 normalized={index:(np.append(v[index],1)@mat) for index in points}
 # Identity per-vertex normals from actual anatomical surface (not flat tubes)
 norm={index:np.zeros(3) for index in points}
 for f in used:
  ai,bi,ci=[idx for idx,u in f]
  aa,bb,cc=[normalized[i] for i in (ai,bi,ci)]
  n=np.cross(bb-aa,cc-aa)
  for i in (ai,bi,ci):norm[i]+=n
 for i in points:norm[i]/=max(np.linalg.norm(norm[i]),1e-9)
 def skinweights(vi):
  co=normalized[vi]
  # Robust root weights: palm stays fused; only fingers articulate
  digit_dists=np.array([distance(v[vi],bone_a[3*d:3*d+3],bone_b[3*d:3*d+3]) for d in range(5)])
  nearest_digit=int(np.argmin(digit_dists.min(axis=1)))
  seg=int(np.argmin(digit_dists[nearest_digit]))
  root=1 if co[1]<.050 else (0 if co[1]>.087 else (.087-co[1])/.037)
  ids=[0,1+3*nearest_digit+seg,0]
  weights=[root,1-root,0.]
  return ids,weights
 count=0;buffer=bytearray()
 for f in used:
  tri=[(idx,u) for idx,u in f]
  for idx,uvindex in tri:
   ids,weights=skinweights(idx)
   # Linthesia blending: all bone weights sum exactly to unity
   buffer+=struct.pack('<3f3f2f3B3f',*map(float,normalized[idx]),*map(float,norm[idx]),*map(float,uv[uvindex]),*ids,*weights)
  count+=1
 path=out/f'mesh_mano_rig_{hand:02d}.lhr'
 path.write_bytes(struct.pack('<4sII',b'LHR1',count,16)+buffer)
 errors=np.linalg.norm(A@mat-dstj,axis=1)
 print('NATIVE_HAND',hand,'triangles',count,'vertices',len(points),'native joint mean error',round(float(errors.mean()),4),'max',round(float(errors.max()),4),'determinant',round(float(np.linalg.det(mat[:3])),6),'file',path,flush=True)

# Source is deliberately CC0-native rather than synthetic cylinders.
# Tessellate each original anatomical triangle into two to improve
# interpolation of skin weights/UVs across joint creases in motion.
for hand in (0,1):
 path=out/f'mesh_mano_rig_{hand:02d}.lhr'
 raw=path.read_bytes()
 count=struct.unpack_from('<I',raw,4)[0]
 def vertex(i):
  return struct.unpack_from('<3f3f2f3B3f',raw,12+47*i)
 def midpoint(a,b):
  v=[(a[i]+b[i])*.5 for i in range(8)]
  n=np.array(v[3:6]);n/=max(np.linalg.norm(n),1e-10);v[3:6]=list(map(float,n))
  weights={}
  for old in (a,b):
   for bone,w in zip(old[8:11],old[11:14]):
    weights[bone]=weights.get(bone,0)+w*.5
  top=sorted(weights.items(),key=lambda item:-item[1])[:3]
  ids=[int(k) for k,w in top]+[0]*(3-len(top))
  ww=[float(w) for k,w in top]+[0.]*(3-len(top))
  norm=sum(ww);ww=[w/norm for w in ww]
  return tuple(v)+tuple(ids)+tuple(ww)
 data=bytearray()
 for tri in range(count):
  a,b,c=[vertex(tri*3+i) for i in range(3)]
  mid=midpoint(a,b)
  for vv in (a,mid,c,mid,b,c):
   data+=struct.pack('<3f3f2f3B3f',*vv)
 path.write_bytes(struct.pack('<4sII',b'LHR1',count*2,16)+data)
 print('SUBDIVIDED_NATIVE_HAND',hand,'triangles',count*2,'bytes',path.stat().st_size,flush=True)
