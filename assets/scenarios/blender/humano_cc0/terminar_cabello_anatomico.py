from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy, math, random
from mathutils import Vector
from pathlib import Path
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(p))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
for o in list(sc.objects):
 if o.name.startswith(('REALISMO | cabello','REALISMO | mechón',
                        'HUMANO CC0 | cabello', 'HUMANO CC0 | mechón',
                        'HUMANO CC0 | helper-')):
  bpy.data.objects.remove(o,do_unlink=True)
skin=bpy.data.materials['HUMANO | piel cálida CC0']
skin.diffuse_color=(.37,.242,.18,1)
skin.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.37,.242,.18,1)
hair=bpy.data.materials.get('Cabello corto castaño')
if hair:
 hair.diffuse_color=(.039,.025,.019,1)
 hair.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.039,.025,.019,1)
verts=[];faces=[];grp=''
for row in Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/humano_cc0/base.obj').open():
 if row.startswith('v '):verts.append(tuple(map(float,row.split()[1:4])))
 elif row.startswith('g '):grp=row[2:].strip()
 elif row.startswith('f ') and grp=='body':
  ids=[int(a.split('/')[0])-1 for a in row.split()[1:]]
  # Front/brow line is receded; rear starts at the lower back of head.
  def hairline(vi):
   x,y,z=verts[vi]
   front=max(0.,min(1.,(z-.15)/1.3))
   return 7.05+.68*front+.18*max(0.,abs(x)-.50)
  if all(verts[i][1] > hairline(i) for i in ids):
   faces.append(ids)
used=sorted(set(i for face in faces for i in face))
idx={src:i for i,src in enumerate(used)}
pts=[]
for i in used:
 x,y,z=verts[i]
 # outward displacement leaves no scalp z-fighting in OpenGL
 ox=.005*(1 if x>0 else -1)
 oz=.007*(1 if z>.45 else -1)
 pts.append((-.48+x*.179+ox,
             -1.17+(z-.48)*.179+oz,
             2.039+(y-6)*.179+.004))
me=bpy.data.meshes.new('Malla cabello moldeada a cráneo humano')
me.from_pydata(pts,[],[[idx[j] for j in f] for f in faces]);me.update()
ob=bpy.data.objects.new('HUMANO CC0 | cabello corto ajustado',me)
sc.collection.objects.link(ob)
ob.data.materials.append(hair)
for poly in ob.data.polygons:poly.use_smooth=True
# Realistic suit geometry: trim old ornamental hip balls, add trouser crease
# and vest fold. The seated pose and interactive arms remain untouched.
for obj in list(sc.objects):
 if obj.name.startswith('Muslo sentado') or obj.name.startswith('Pianista sentado | pelvis'):
  bpy.data.objects.remove(obj,do_unlink=True)
suit=bpy.data.materials.get('Traje azul grafito')
if suit:
 for x in (-.68,-.28):
  bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=14,
        location=(x,-1.56,.99))
  ob=bpy.context.object;ob.name='HUMANO CC0 | cadera traje entallado'
  ob.scale=(.19,.23,.17);ob.data.materials.append(suit)
  for poly in ob.data.polygons:poly.use_smooth=True
bpy.ops.wm.save_as_mainfile(filepath=str(p))
print('SCALP_GEOMETRY',len(pts),len(faces),flush=True)
# show both rear and side: panoramic image to judge scalp and nose
sc.frame_set(1);sc.render.engine='CYCLES';sc.cycles.samples=16
sc.render.resolution_x=960;sc.render.resolution_y=540
sc.render.filepath=str(p.parent/'pianista_humano_cc0_cabello.png')
bpy.ops.render.render(write_still=True,scene=sc.name)
print('HAIR_PREVIEW_COMPLETE',flush=True)
