from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,math
from pathlib import Path
from mathutils import Vector
P=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(P))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
def setbone(ob,a,b,frame,r):
 v=Vector(b)-Vector(a);ob.location=a;ob.rotation_quaternion=v.to_track_quat('Z','Y').to_euler().to_quaternion()
 ob.scale=(r,r,v.length)
 for attr in ('location','rotation_quaternion','scale'):ob.keyframe_insert(attr,frame=frame)
for index,frame in enumerate([1,7,13,19,25,31,37,43,49,55,61]):
 for side in [-1,1]:
  suffix=str(side)
  hand=sc.objects['CONTROL MIDI | '+('izquierda' if side<0 else 'derecha')]
  upper=sc.objects['BRAZO SUPERIOR | '+suffix];lower=sc.objects['ANTEBRAZO MÓVIL | '+suffix]
  elbow=sc.objects['Codo articulado | '+suffix];cuff=sc.objects['Puño camisa | '+suffix]
  cx=-.48
  lat=.065*math.sin(index*.79+(1 if side>0 else 0))
  press=.042 if (index+int(side>0))%3==1 else .009
  shoulder=(cx+side*.335,-1.22,1.80)
  # Hands clearly separated at playable key registers.
  wrist=(cx+(-.72 if side<0 else .72)+lat,-.485,1.047-press)
  eb=(cx+(-.52 if side<0 else .52)+lat*.5,-.87,1.37-press*.3)
  hand.location=wrist;hand.keyframe_insert('location',frame=frame)
  elbow.location=eb;elbow.keyframe_insert('location',frame=frame)
  cuff.location=(wrist[0],wrist[1]-.06,wrist[2]+.015);cuff.keyframe_insert('location',frame=frame)
  setbone(upper,shoulder,eb,frame,.123)
  setbone(lower,eb,wrist,frame,.10)
cam=sc.camera;cam.animation_data_clear();cam.data.lens=43
for frame in [1,7,13,19,25,31,37,43,49,55,61]:
 t=(frame-1)/60
 cam.location=(.35+.055*math.sin(t*math.tau),-6.55+.07*math.cos(t*math.tau),4.08)
 look=Vector((-.48,-.36,.91))
 cam.rotation_euler=(look-cam.location).to_track_quat('-Z','Y').to_euler()
 cam.keyframe_insert('location',frame=frame)
 cam.keyframe_insert('rotation_euler',frame=frame)
bpy.ops.wm.save_as_mainfile(filepath=str(P))
sc.frame_set(25);sc.render.filepath=str(P.parent/'PREV_two_hands_high.png')
bpy.ops.render.render(write_still=True,scene=sc.name)
print('TWO_HANDS_HIGH_READY')
