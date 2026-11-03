from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,math
from mathutils import Vector
from pathlib import Path
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(p))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
coat=bpy.data.materials['Traje azul grafito']
trouser=bpy.data.materials['Pantalón oscuro']
for name in ('Pianista sentado | espalda','Pianista sentado | pecho','Pianista sentado | pelvis'):
 o=sc.objects.get(name)
 if o:bpy.data.objects.remove(o,do_unlink=True)
def tailored_jacket():
 # Four cross-section rings: waist, chest and neck; bent forwards toward keys.
 rings=[(1.12,.26,.24,-1.55),(1.39,.33,.22,-1.46),
        (1.69,.37,.225,-1.33),(1.89,.33,.21,-1.26),(1.98,.17,.17,-1.25)]
 vertices=[];faces=[]
 for z,rx,ry,y in rings:
  for i in range(24):
   ang=2*math.pi*i/24
   vertices.append((-.48+rx*math.cos(ang),y+ry*math.sin(ang),z))
 for i in range(4):
  for j in range(24):
   a=i*24+j;b=i*24+(j+1)%24
   faces.append((a,b,b+24,a+24))
 faces.append(tuple(reversed(list(range(24)))))
 faces.append(tuple(96+j for j in range(24)))
 mesh=bpy.data.meshes.new('Confección chaqueta');mesh.from_pydata(vertices,[],faces);mesh.update()
 ob=bpy.data.objects.new('Chaqueta entallada | postura sentado',mesh);sc.collection.objects.link(ob)
 ob.data.materials.append(coat)
 for poly in ob.data.polygons:poly.use_smooth=True
 # Subdivision smoothing without ballooning; custom shape is preserved.
 sub=ob.modifiers.new('Superficie textil','SUBSURF');sub.levels=1;sub.render_levels=1
 return ob
tailored_jacket()
for o in list(sc.objects):
 if o.name.startswith('Muslo sentado'):
  bpy.data.objects.remove(o,do_unlink=True)
def segment(name,a,b,radius,material):
 v=Vector(b)-Vector(a)
 bpy.ops.mesh.primitive_cylinder_add(vertices=18,radius=radius,depth=v.length,location=(Vector(a)+Vector(b))/2)
 ob=bpy.context.object;ob.name=name;ob.rotation_euler=v.to_track_quat('Z','Y').to_euler()
 ob.data.materials.append(material)
 for poly in ob.data.polygons:poly.use_smooth=True
for side in [-1,1]:
 x=-.48+side*.20
 segment('Pantalón pierna superior', (x,-1.59,1.00),(x+side*.08,-1.13,.47),.145,trouser)
# Camera diagonally from behind and above: both forearms and contact points visible.
cam=sc.camera;cam.animation_data_clear()
cam.data.lens=41
for f in [1,7,13,19,25,31,37,43,49,55,61]:
 t=(f-1)/60
 cam.location=(1.70+.08*math.sin(t*math.tau),-6.95+.06*math.cos(t*math.tau),3.50)
 target=Vector((-.45,-.41,1.04))
 cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
 cam.keyframe_insert(data_path='location',frame=f)
 cam.keyframe_insert(data_path='rotation_euler',frame=f)
bpy.ops.wm.save_as_mainfile(filepath=str(p))
sc.frame_set(19)
sc.render.filepath=f'{_LINTHESIA_ROOT}/assets/scenarios/blender/PREV_both_hands.png'
bpy.ops.render.render(write_still=True,scene=sc.name)
print('BOTH_HANDS_RENDERED')
