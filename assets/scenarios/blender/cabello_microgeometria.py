from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy, math,random
from pathlib import Path
from mathutils import Vector
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(p))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
for ob in list(sc.objects):
 if ob.name.startswith('HUMANO CC0 | cabello fibra'):
  bpy.data.objects.remove(ob,do_unlink=True)
cap=sc.objects['HUMANO CC0 | cabello corto ajustado']
cap.data.calc_loop_triangles()
rng=random.Random(2301)
materials=[]
for name,c in [('Cabello micro | grafito',(.033,.023,.018)),('Cabello micro | reflejos',(.070,.042,.029))]:
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.diffuse_color=(*c,1);m.use_nodes=True
 m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*c,1)
 m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.94
 materials.append(m)
# 800 fine flat hair fibres sculpted over the existing scalp surface.
# Their winding produces genuine geometry in the rotating 3D scene,
# unlike painted noise. Keep the geometry modest for 60 fps.
tris=cap.data.loop_triangles
verts=[];faces=[];matid=[]
for i in range(850):
 tri=tris[rng.randrange(len(tris))]
 v0,v1,v2=[cap.matrix_world@cap.data.vertices[x].co for x in tri.vertices]
 u=rng.random();v=rng.random()
 if u+v>1:u=1-u;v=1-v
 root=v0*(1-u-v)+v1*u+v2*v
 normal=(v1-v0).cross(v2-v0).normalized()
 if normal.length<.01:continue
 # Project the hairstyle direction onto the local tangent plane.
 flow=Vector((rng.uniform(-.18,.18),-.55,-.45 if normal.y<.3 else -.15))
 flow=flow-normal*flow.dot(normal)
 if flow.length<.0001:continue
 flow.normalize()
 side=normal.cross(flow).normalized()
 length=rng.uniform(.012,.035)
 width=rng.uniform(.0012,.0027)
 off=normal*rng.uniform(.0015,.0027)
 p0=root+off
 p1=root+flow*length*.62+off+normal*.001
 p2=root+flow*length+off+normal*.002
 j=len(verts)
 verts.extend((p0-side*width,p0+side*width,p1-side*width*.65,
               p1+side*width*.65,p2-side*width*.12,p2+side*width*.12))
 faces.append((j,j+1,j+3,j+2))
 faces.append((j+2,j+3,j+5,j+4))
 shade=1 if i%7==0 else 0
 matid.extend((shade,shade))
me=bpy.data.meshes.new('Geometría fibra capilar estilizada')
me.from_pydata(verts,[],faces);me.update()
o=bpy.data.objects.new('HUMANO CC0 | cabello fibra microscópica',me)
sc.collection.objects.link(o)
for m in materials:o.data.materials.append(m)
for face,idx in zip(me.polygons,matid):
 face.material_index=idx
 face.use_smooth=True
bpy.ops.wm.save_as_mainfile(filepath=str(p))
print('FIBRAS_CAPILARES_REAL_3D',len(me.polygons),len(me.vertices),flush=True)
