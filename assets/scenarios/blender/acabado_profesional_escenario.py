from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,math
from pathlib import Path
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(p))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
def finish_mat(name,rgb,roughness=.65,metal=0):
 m=bpy.data.materials.get(name)
 if not m:return
 m.diffuse_color=(*rgb,1)
 if m.use_nodes and m.node_tree:
  bs=m.node_tree.nodes.get('Principled BSDF')
  if bs:
   bs.inputs['Base Color'].default_value=(*rgb,1)
   bs.inputs['Roughness'].default_value=roughness
   bs.inputs['Metallic'].default_value=metal
def new_mat(name,rgb,rough=.8):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.use_nodes=True;m.diffuse_color=(*rgb,1)
 bs=m.node_tree.nodes.get('Principled BSDF')
 bs.inputs['Base Color'].default_value=(*rgb,1)
 bs.inputs['Roughness'].default_value=rough
 return m
finish_mat('Pianista animado suelo',(.065,.070,.079),.68,.07)
finish_mat('Pianista animado pared',(.042,.045,.061),.87,.0)
finish_mat('Traje azul grafito',(.039,.044,.052),.81,.01)
finish_mat('Traje azul oscuro de paño',(.045,.049,.063),.83,.0)
finish_mat('Pantalón oscuro',(.030,.033,.041),.77,.0)
finish_mat('Cola | Marco de hierro fundido',(.40,.265,.10),.31,.53)
finish_mat('Cola | Cuerdas de acero',(.39,.345,.24),.29,.72)
finish_mat('Cola | Negro piano lacado',(.018,.021,.027),.16,.40)
# Remove static spheres at the shoulders: dynamic continuous sleeves cover
# these articulations and should not look like robotic ball-joint shoulders.
for obj in list(sc.objects):
 if obj.name.startswith(('HUMANO CC0 | hombrera anatómica',
                         'HUMANO CC0 | solapa trasera')):
  bpy.data.objects.remove(obj,do_unlink=True)
# Classic theatre backdrop with geometry, not a fixed camera-facing card.
curtain='TEATRO PROFESIONAL | cortina azul noche'
obj=sc.objects.get(curtain)
if obj:bpy.data.objects.remove(obj,do_unlink=True)
mat=new_mat('Terciopelo acústico | azul profundo',(.025,.028,.043),.97)
verts=[];faces=[]
segments=100
for i in range(segments+1):
 x=-8.5+17.*i/segments
 y=4.86+.11*math.sin(i*math.pi*.34)
 for z in (.05,5.7):
  verts.append((x,y,z))
for i in range(segments):
 j=2*i
 faces.append((j,j+2,j+3,j+1))
mesh=bpy.data.meshes.new(curtain)
mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new(curtain,mesh);sc.collection.objects.link(obj)
obj.data.materials.append(mat)
for poly in obj.data.polygons:poly.use_smooth=True
# Satin black, properly sized floor riser framing the piano, replacing the
# oversized hot pink platform look of the previous material.
stage_mat=new_mat('Tarima | grafito satinado',(.060,.064,.070),.41)
bpy.ops.mesh.primitive_cube_add(size=1,location=(-.45,.20,-.065))
riser=bpy.context.object
riser.name='TEATRO PROFESIONAL | tarima de concierto'
riser.dimensions=(3.5,4.55,.12)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
riser.data.materials.append(stage_mat)
bevel=riser.modifiers.new('Bordes suaves tarima','BEVEL');bevel.width=.03;bevel.segments=3
riser.modifiers.new('Normales','WEIGHTED_NORMAL')
# The seated singer pianist is an adult, not an inflatable toy.
# Remove any inert, old Cartesian spheres that duplicate the live hands.
for obj in list(sc.objects):
 if obj.name.startswith('PROPORCION |'):
  bpy.data.objects.remove(obj,do_unlink=True)
bpy.ops.wm.save_as_mainfile(filepath=str(p))
print('CONCERT_STAGE_MATERIALS_AND_GEOMETRY_DONE',len(sc.objects),flush=True)
