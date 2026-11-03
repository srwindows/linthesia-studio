from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
# SPDX-License-Identifier: GPL-2.0-or-later
# Adult head/body ratio and tailoring of the MIDI-aware pianist.
import bpy
from mathutils import Vector
from pathlib import Path
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(p))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
pivot=Vector((-.48,-1.17,2.245))
scale=Vector((.82,.82,.77))
parts=('Pianista sentado | cabeza','Rostro perfil |',
       'REALISMO | mandíbula','REALISMO | mentón',
       'REALISMO | esclerótica','REALISMO | iris',
       'REALISMO | ceja','REALISMO | pómulo',
       'REALISMO | patilla','REALISMO | nariz',
       'REALISMO | labio','REALISMO | cabello',
       'REALISMO | mechón')
count=0
for obj in sc.objects:
 if not obj.name.startswith(parts):continue
 if obj.name.startswith('REALISMO | cabello entallado 3D'):
  # Haircap mesh uses original absolute positions as local coordinates.
  for v in obj.data.vertices:
   v.co.x=pivot.x+(v.co.x-pivot.x)*scale.x
   v.co.y=pivot.y+(v.co.y-pivot.y)*scale.y
   v.co.z=pivot.z+(v.co.z-pivot.z)*scale.z
 else:
  loc=obj.location.copy()
  obj.location=Vector((pivot.x+(loc.x-pivot.x)*scale.x,
                       pivot.y+(loc.y-pivot.y)*scale.y,
                       pivot.z+(loc.z-pivot.z)*scale.z))
  obj.scale.x*=scale.x;obj.scale.y*=scale.y;obj.scale.z*=scale.z
 count+=1
# The old neck was almost white under the spotlights. Keep consistent warm
# skin, seated with head/shoulder proportions of an adult pianist.
skin=bpy.data.materials.get('Piel humana | tono medio')
for obj in sc.objects:
 if obj.name.startswith(('Pianista sentado | cuello','Rostro perfil | oreja')):
  if skin:
   obj.data.materials.clear()
   obj.data.materials.append(skin)
# Slim the protruding thigh cylinders; they previously looked like two
# rounded ornaments resting on the piano stool.
for obj in sc.objects:
 if obj.name.startswith('Pantalón pierna superior'):
  obj.scale.x*=.79;obj.scale.y*=.79
# Widen tailcoat shoulders slightly while leaving hip and back resting on seat.
# It gives a clear upper-body silhouette viewed behind the instrument.
co=sc.objects.get('Chaqueta entallada | postura sentado')
if co:
 for v in co.data.vertices:
  if v.co.z>1.60:
   v.co.x=-.48+(v.co.x+.48)*1.10
# A better haircut than the previous flat disk: textured wisps along the nape.
hair=bpy.data.materials.get('Cabello corto castaño')
thread=bpy.data.materials.get('Costura sastre azul')
def line(label,a,b,width,mat):
 a=Vector(a);b=Vector(b);dv=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=10,radius=width,depth=dv.length,location=(a+b)*.5)
 ob=bpy.context.object;ob.name='PROPORCION | '+label
 ob.rotation_euler=dv.to_track_quat('Z','Y').to_euler()
 ob.data.materials.append(mat)
 for f in ob.data.polygons:f.use_smooth=True
for ob in list(sc.objects):
 if ob.name.startswith('PROPORCION |'):bpy.data.objects.remove(ob,do_unlink=True)
if thread:
 for side in (-1,1):
  line('costura sastre lado '+str(side),
       (-.48+side*.19,-1.64,1.72),
       (-.48+side*.12,-1.67,1.34),.0035,thread)
if hair:
 for j in range(-7,8):
  dx=j*.015
  line('pelo nuca %d'%j,(-.48+dx,-1.329,2.18),
       (-.48+dx*.89,-1.299,2.105),.0038,hair)
bpy.ops.wm.save_as_mainfile(filepath=str(p))
print('ADULT_PROPORtIONS_REFINED',count,flush=True)
sc.frame_set(1);sc.render.engine='CYCLES';sc.cycles.samples=18
sc.render.resolution_x=960;sc.render.resolution_y=540
sc.render.filepath=str(p.parent/'pianista_proporciones_adultas.png')
bpy.ops.render.render(write_still=True,scene=sc.name)
print('PREVIEW_ADULT_READY',flush=True)
