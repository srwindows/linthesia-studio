from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,sys
from pathlib import Path
from mathutils import Vector
root=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender')
bpy.ops.wm.open_mainfile(filepath=str(root/'Linthesia_Pack_7_Escenarios.blend'))
scene=bpy.data.scenes['Pianista animado']
bpy.context.window.scene=scene
timeline=[1,7,13,19,25,31,37,43,49,55,61]
for k,f in enumerate(timeline):
 scene.frame_set(f)
 wrist_left=scene.objects['CONTROL MIDI | izquierda'].location
 wrist_right=scene.objects['CONTROL MIDI | derecha'].location
 assert wrist_left.x<wrist_right.x and -2.3<wrist_left.x<-.6
 assert -.5<wrist_right.x<1.0
 assert -.56<wrist_left.y<-.35 and -.56<wrist_right.y<-.35
 assert .97 <wrist_left.z<1.08 and .97<wrist_right.z<1.08
 assert all(scene.objects.get('ANTEBRAZO MÓVIL | '+str(sign)) is not None for sign in (-1,1))
 scene.render.filepath=str(root/('escenario_06.png' if k==0 else 'pianista_%02d.png'%f))
 bpy.ops.render.render(write_still=True,scene=scene.name)
 print('FOTOGRAMA_VERIFICADO',k,f,round(wrist_left.z,3),round(wrist_right.z,3),flush=True)
print('11_POSES_BLENDER_TERMINADAS',flush=True)
