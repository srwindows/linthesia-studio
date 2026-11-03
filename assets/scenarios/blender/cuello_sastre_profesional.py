from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy
from pathlib import Path
from mathutils import Vector
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(p))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
old=sc.objects.get('HUMANO CC0 | cuello anatómico')
if old:bpy.data.objects.remove(old,do_unlink=True)
skin=bpy.data.materials.get('HUMANO | piel cálida CC0')
# Thin neck collar is an anatomical transition, not a grey sphere.
verts=[];faces=[]
sides=24
import math
for z,y,r in [(1.93,-1.180,.075),(1.985,-1.178,.072),(2.048,-1.166,.064),(2.092,-1.159,.060)]:
 for i in range(sides):
  a=math.tau*i/sides
  verts.append((-.48+r*math.cos(a),y+r*math.sin(a),z))
for ring in range(3):
 for j in range(sides):
  a=ring*sides+j;b=ring*sides+(j+1)%sides
  faces.append((a,b,b+sides,a+sides))
me=bpy.data.meshes.new('Cuello sastre humano');
me.from_pydata(verts,[],faces);me.update()
ob=bpy.data.objects.new('TEATRO PROFESIONAL | cuello humano anatómico',me)
sc.collection.objects.link(ob)
ob.data.materials.append(skin)
for polygon in ob.data.polygons:polygon.use_smooth=True
# Add the conventional subtle dress-shirt collar collar (white linen),
# never a massive white ring sticking out of the neck.
cloth=bpy.data.materials.get('Camisa gris perla')
for side in (-1,1):
 me=bpy.data.meshes.new('Solapa camisa sin volumen irreal')
 x=-.48
 pts=[(x+side*.064,-1.180,1.97),(x+side*.102,-1.174,1.91),
      (x+side*.020,-1.123,1.885),(x+side*.035,-1.132,1.943)]
 me.from_pydata(pts,[],[(0,1,2,3)]);me.update()
 ob=bpy.data.objects.new('TEATRO PROFESIONAL | cuello camisa '+str(side),me)
 sc.collection.objects.link(ob)
 if cloth:me.materials.append(cloth)
bpy.ops.wm.save_as_mainfile(filepath=str(p))
print('CUELLO_ESFERA_ELIMINADA',len(me.vertices),flush=True)
