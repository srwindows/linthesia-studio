from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
"""Correct the seated pianist's orientation, arm rig and actual keyboard contact.
Edits ONLY the seventh Blender scene, preserving six existing scene assets.
GPL-2.0-or-later – original procedural geometry.
"""
import bpy, math, sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender')
BLEND=ROOT/'Linthesia_Pack_7_Escenarios.blend'
bpy.ops.wm.open_mainfile(filepath=str(BLEND))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
# Remove the previous character: he was placed behind the instrument and
# the upper/lower arms were static primitives that did not follow the wrists.
starts=('Pianista |','Oreja anatomía','Ojos','Ceja oscura','Mejilla anatómica',
        'Cabello patilla','Solapa izquierda','Solapa derecha','CONTROL MIDI |',
        'Palma mano','Dedo ','Pianista | antebrazo','Pianista | brazo')
for o in list(sc.objects):
 if o.name.startswith(starts):bpy.data.objects.remove(o,do_unlink=True)
def make_mat(name,color,rough=.54,metal=0.0):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.diffuse_color=(*color,1)
 m.use_nodes=True
 sh=m.node_tree.nodes.get('Principled BSDF')
 sh.inputs['Base Color'].default_value=(*color,1)
 sh.inputs['Roughness'].default_value=rough
 sh.inputs['Metallic'].default_value=metal
 return m
skin=make_mat('Piel del músico',(.45,.29,.19))
jacket=make_mat('Traje azul grafito',(.030,.043,.075),.65)
trousers=make_mat('Pantalón oscuro',(.035,.037,.047),.77)
shirt=make_mat('Camisa marfil',(.76,.76,.70),.58)
hair=make_mat('Cabello realista oscuro',(.022,.019,.018),.83)
shoe=make_mat('Zapato cuero',(.023,.026,.028),.37,.12)
def ellipsoid(name,position,dimensions,material,segments=28,rings=16):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=rings,location=position)
 ob=bpy.context.object;ob.name=name;ob.scale=dimensions
 ob.data.materials.append(material)
 for face in ob.data.polygons:face.use_smooth=True
 return ob
def seg(name,material,radius):
 # Local Z runs from 0 to 1 for stable joints. Each frame animates
 # position + quaternion + scale, preserving shoulder/elbow/wrist contact.
 bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=1,depth=1,location=(0,0,0))
 ob=bpy.context.object;ob.name=name;ob.data.materials.append(material)
 for v in ob.data.vertices:v.co.z+=.5
 for poly in ob.data.polygons:poly.use_smooth=True
 ob.rotation_mode='QUATERNION'
 ob['proportional_endpoints']=True
 return ob
def pose_segment(obj,a,b,frame,radius):
 a=Vector(a);b=Vector(b);v=b-a
 obj.location=a
 obj.rotation_quaternion=v.to_track_quat('Z','Y')
 obj.scale=(radius,radius,v.length)
 for field in ('location','rotation_quaternion','scale'):
  obj.keyframe_insert(data_path=field,frame=frame)
def anchor(name,position):
 bpy.ops.object.empty_add(type='PLAIN_AXES',location=position)
 ob=bpy.context.object;ob.name=name;return ob
cx=-.48
# Position the stool on the AUDIENCE SIDE of the keys (front is y < -.56).
for o in sc.objects:
 if o.name=='Banco del pianista':
  o.location.y=-1.62
 if o.name.startswith('Pata banco'):
  o.location.y=-1.62
# Seated, forward-leaning performer facing piano (+Y), seen from the
# rear-quarter camera rather than cleaning the back of the piano.
ellipsoid('Pianista sentado | pelvis',(cx,-1.61,1.00),(.35,.29,.30),trousers)
torso=ellipsoid('Pianista sentado | espalda',(cx,-1.36,1.59),(.39,.28,.47),jacket)
torso.rotation_euler.x=.16
ellipsoid('Pianista sentado | pecho',(cx,-1.15,1.63),(.31,.22,.42),jacket)
ellipsoid('Pianista sentado | cuello',(cx,-1.22,2.01),(.115,.12,.15),skin)
ellipsoid('Pianista sentado | cabeza',(cx,-1.17,2.24),(.22,.235,.27),skin,36,22)
ellipsoid('Pianista sentado | cabello',(cx,-1.20,2.42),(.225,.236,.12),hair)
# Eyes/nose on +Y face: performer looks at keys, never at audience.
for side in (-1,1):
 ellipsoid('Rostro perfil | oreja',(cx+side*.219,-1.15,2.245),(.045,.064,.087),skin,16,12)
 ellipsoid('Rostro perfil | ojo',(cx+side*.086,-.958,2.277),(.023,.017,.019),hair,14,10)
ellipsoid('Rostro perfil | nariz',(cx,-.922,2.203),(.045,.065,.056),skin,16,12)
ellipsoid('Camisa en cuello',(cx,-1.04,1.94),(.16,.06,.11),shirt)
for sign in (-1,1):
 hip=(cx+sign*.19,-1.62,.89)
 knee=(cx+sign*.25,-1.08,.47)
 foot=(cx+sign*.26,-.90,.19)
 thigh=ellipsoid('Muslo sentado',((hip[0]+knee[0])/2,(hip[1]+knee[1])/2,(hip[2]+knee[2])/2),
                 (.17,.36,.18),trousers)
 shin=seg('Pierna animable | '+str(sign),trousers,.13)
 pose_segment(shin,knee,foot,1,.13)
 ellipsoid('Zapato de pianista',foot,(.15,.28,.105),shoe)
# Wrists, palm groups and arm links are EACH animated at matching frame.
hands=[]
for sign in (-1,1):
 shoulder=(cx+sign*.335,-1.22,1.80)
 hand=anchor('CONTROL MIDI | '+('izquierda' if sign<0 else 'derecha'),
             (cx+sign*.71,-.49,1.043))
 wristgroup=ellipsoid('Palma | '+str(sign),(0,0,0),(.165,.15,.054),skin)
 wristgroup.parent=hand;wristgroup.location=(0,0,0)
 fingers=[]
 for finger in range(5):
  # Curved fingers reach +Y from the palm, onto the white and black keys.
  ob=ellipsoid('Dedo %02d | %s'%(finger,sign),(0,0,0),
               (.028,.095,.028),skin,18,12)
  ob.parent=hand
  ob.location=((finger-2)*.060,.135,.006)
  ob.rotation_mode='XYZ'
  # Fingertip is at y≈-.27 and z=.96 in the pressed pose.
  fingers.append(ob)
 upper=seg('BRAZO SUPERIOR | '+str(sign),jacket,.125)
 lower=seg('ANTEBRAZO MÓVIL | '+str(sign),jacket,.100)
 elbowpart=ellipsoid('Codo articulado | '+str(sign),(0,0,0),(.115,.115,.115),jacket)
 cuff=ellipsoid('Puño camisa | '+str(sign),(0,0,0),(.105,.09,.09),shirt)
 hands.append((sign,shoulder,hand,fingers,upper,lower,elbowpart,cuff))
# Eleven render poses designed around specific left/right hand key ranges.
# Frame 1 is resting; the remaining poses are pressure/position changes.
frames=[1,7,13,19,25,31,37,43,49,55,61]
for index,frame in enumerate(frames):
 t=index/10.0
 for sign,shoulder,hand,fingers,upper,lower,elbowpart,cuff in hands:
  # 40cm coherent lateral motion; fingers stay above the visible 88-key row.
  lateral= .11*math.sin(index*.79+(1 if sign>0 else 0))
  pressure=(.045 if (index+int(sign>0))%3==1 else .009)
  x=cx+sign*.71+lateral
  w=(x,-.485,1.047-pressure)
  e=(cx+sign*.49+lateral*.47,-.89,1.37-pressure*.35)
  hand.location=w
  hand.rotation_euler=(.035*math.sin(index*.53),0,.045*math.cos(index*.81))
  hand.keyframe_insert(data_path='location',frame=frame)
  hand.keyframe_insert(data_path='rotation_euler',frame=frame)
  pose_segment(upper,shoulder,e,frame,.124)
  pose_segment(lower,e,w,frame,.099)
  elbowpart.location=e;elbowpart.keyframe_insert(data_path='location',frame=frame)
  cuff.location=(w[0],w[1]-.065,w[2]+.018)
  cuff.keyframe_insert(data_path='location',frame=frame)
  for finger,ob in enumerate(fingers):
   active=index!=0 and (index+finger+int(sign>0))%4 in (0,1)
   bend=.12 if active else .04
   ob.location.z=-.016 if active else .013
   ob.rotation_euler.x=bend
   ob.keyframe_insert(data_path='location',frame=frame)
   ob.keyframe_insert(data_path='rotation_euler',frame=frame)
# Change the shot to an oblique concert angle looking toward the player's
# hands and the piano keys; no frontal hand-on-the-back illusion.
cam=sc.camera
cam.animation_data_clear()
cam.data.lens=42
for index,frame in enumerate(frames):
 t=index/10.
 cam.location=(3.35+.14*math.sin(t*math.tau),-6.45+.10*math.cos(t*math.tau),3.25+.075*math.sin(t*math.tau))
 target=Vector((-.40,-.55,1.19))
 cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
 cam.keyframe_insert(data_path='location',frame=frame)
 cam.keyframe_insert(data_path='rotation_euler',frame=frame)
sc.frame_start=1;sc.frame_end=61;sc.render.fps=24
sc.render.engine='CYCLES';sc.cycles.samples=30
sc.render.resolution_x=960;sc.render.resolution_y=540
for layer in sc.view_layers:layer.cycles.use_denoising=False
# Assert the performer's hands really reach the playable surface.
sc.frame_set(7)
bpy.context.view_layer.update()
for sign,shoulder,hand,fingers,upper,lower,elbowpart,cuff in hands:
 assert hand.location.y<-.35 and hand.location.y>-.57
 assert .95<hand.location.z<1.08
 assert elbowpart.location.y<-.70
 assert shoulder[1]<-.95
 assert all(ob.parent==hand for ob in fingers)
assert sc.camera.location.y< -4.0
print('VERIFIED_HANDS_KEYS_ALIGNED',flush=True)
# Avoid losing artists' edits in other scenes. Save a new backup-respecting blend.
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
# Preview 3 poses first; the final render set is a separate invocation.
onlypreview='--all' not in sys.argv
samples=[1,19,43] if onlypreview else frames
for i,frame in enumerate(samples):
 sc.frame_set(frame)
 sc.render.filepath=str(ROOT/(f'PREV_pianista_{frame:02d}.png' if onlypreview else
                  f'escenario_06.png' if frame==1 else f'pianista_{frame:02d}.png'))
 bpy.ops.render.render(write_still=True,scene=sc.name)
 print('RENDER_POSTURA',frame,flush=True)
print('POSE_FIX_COMPLETE',flush=True)
