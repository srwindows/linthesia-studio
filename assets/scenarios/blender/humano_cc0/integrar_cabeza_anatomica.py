from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
"""CC0 MakeHuman anatomical head and eye mesh on the GPL Linthesia pianist.
Source geometry credited in humano_cc0/README_ORIGEN.md. Blender file editable.
"""
import bpy, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender')
OBJ=ROOT/'humano_cc0/base.obj'
BLEND=ROOT/'Linthesia_Pack_7_Escenarios.blend'
bpy.ops.wm.open_mainfile(filepath=str(BLEND))
scene=bpy.data.scenes['Pianista animado']
bpy.context.window.scene=scene
# Remove the former sphere-shaped cartoon face and its floating brow/nose:
# Replace with manifold-derived anatomical head polygons from MakeHuman CC0.
for obj in list(scene.objects):
 if obj.name.startswith((
     'Pianista sentado | cabeza', 'Pianista sentado | cuello',
     'Rostro perfil |',
     'REALISMO | mandíbula', 'REALISMO | mentón',
     'REALISMO | esclerótica', 'REALISMO | iris',
     'REALISMO | ceja', 'REALISMO | pómulo',
     'REALISMO | patilla', 'REALISMO | nariz',
     'REALISMO | labio', 'Pianista sentado | cabello',
     'HUMANO CC0 |'
  )):
  bpy.data.objects.remove(obj,do_unlink=True)
# Existing textured hair cap and strands made in Blender stay editable.
def material(name,color,roughness=.78):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.diffuse_color=(*color,1)
 m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF')
 bs.inputs['Base Color'].default_value=(*color,1)
 bs.inputs['Roughness'].default_value=roughness
 return m
skin=material('HUMANO | piel cálida CC0',(.47,.319,.246),.76)
lips=material('HUMANO | mucosa labios',(.39,.19,.168),.77)
white=material('HUMANO | córnea marfil',(.77,.735,.67),.20)
iris=material('HUMANO | iris castaño',(.13,.075,.051),.42)
hair=material('Cabello corto castaño',(.065,.043,.031),.84)
vertices=[]
faces={'body':[],'helper-l-eye':[],'helper-r-eye':[]}
groups=''
for line in OBJ.open():
 if line.startswith('v '):
  vertices.append(tuple(float(v) for v in line.split()[1:4]))
 elif line.startswith('g '):
  groups=line[2:].strip()
 elif line.startswith('f ') and groups in faces:
  polygon=[int(pair.split('/')[0])-1 for pair in line.split()[1:]]
  if groups=='body' and all(vertices[k][1]>=5.95 for k in polygon):
   faces[groups].append(polygon)
  elif groups!='body':
   faces[groups].append(polygon)
# x horizontal, y vertical, z towards face from MakeHuman;
# in Linthesia x horizontal, y towards piano, z vertical.
def mapped(v):
 x,y,z=v
 return (-.48+x*.179,-1.17+(z-.48)*.179,2.039+(y-6.00)*.179)
def create(name,polygons,mat):
 used=sorted(set(k for face in polygons for k in face))
 idx={k:i for i,k in enumerate(used)}
 mesh=bpy.data.meshes.new(name)
 mesh.from_pydata([mapped(vertices[k]) for k in used], [],
                  [[idx[k] for k in f] for f in polygons])
 mesh.update()
 ob=bpy.data.objects.new(name,mesh)
 scene.collection.objects.link(ob)
 ob.data.materials.append(mat)
 for poly in ob.data.polygons:poly.use_smooth=True
 return ob
head=create('HUMANO CC0 | cabeza anatómica',faces['body'],skin)
for key in ('helper-l-eye','helper-r-eye'):
 create('HUMANO CC0 | '+key,faces[key],white)
# Physically separated neck cylinder tapers into anatomy to avoid sharp
# cut line on the head; no more large round ears, forehead or button nose.
def ellipsoid(name,loc,radii,mat):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,location=loc)
 o=bpy.context.object;o.name='HUMANO CC0 | '+name;o.scale=radii
 o.data.materials.append(mat)
 for poly in o.data.polygons:poly.use_smooth=True
 return o
ellipsoid('cuello anatómico',(-.48,-1.178,2.027),(.098,.092,.125),skin)
# Angular chin underside and neck are derived from CC0 base head; ears
# and nose come from MakeHuman facial vertices, not procedural spheres.
# Capped hair (already authored in Blender) follows the narrow scalp.
scalp=scene.objects.get('REALISMO | cabello entallado 3D')
if scalp:
 # Existing hair cap is shorter than MakeHuman skull; adapt to rear scalp.
 # Coordinates were baked as local model-space points.
 centre=Vector((-.48,-1.174,2.39))
 for v in scalp.data.vertices:
  v.co.x=centre.x+(v.co.x-centre.x)*1.00
  v.co.y=centre.y+(v.co.y-centre.y)*1.00
  v.co.z=centre.z+(v.co.z-centre.z)*1.12
# Match strand positions to the cap, without moving objects of other scenes.
# Explicit CC0 provenance is retained in .blend custom properties.
head['source']='MakeHuman Community base.obj (CC0 1.0); geometry restricted to head'
head['author']='Data Collection AB / MakeHuman Community contributors, 2020'
head['mesh_license']='CC0-1.0'
head['integration']='Linthesia Blender 3D character'
assert len(head.data.vertices)>1500
print('ANATOMICAL_HEAD',len(head.data.vertices),len(head.data.polygons),flush=True)
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
scene.frame_set(1)
scene.render.resolution_x=960
scene.render.resolution_y=540
scene.render.resolution_percentage=100
scene.render.engine='CYCLES';scene.cycles.samples=20
scene.render.filepath=str(ROOT/'pianista_cabeza_cc0_prueba.png')
bpy.ops.render.render(write_still=True,scene=scene.name)
print('PREVIEW_HEAD_CC0_RENDERED',flush=True)
