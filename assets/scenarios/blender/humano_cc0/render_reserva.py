from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy
from pathlib import Path
r=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender')
bpy.ops.wm.open_mainfile(filepath=str(r/'Linthesia_Pack_7_Escenarios.blend'))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
sc.render.engine='CYCLES';sc.cycles.samples=14
sc.render.resolution_x=960;sc.render.resolution_y=540
sc.render.resolution_percentage=100
frames=(1,7,13,19,25,31,37,43,49,55,61)
for i,frame in enumerate(frames):
 sc.frame_set(frame)
 sc.render.filepath=str(r/('escenario_06.png' if i==0 else 'pianista_%02d.png'%frame))
 bpy.ops.render.render(write_still=True,scene=sc.name)
 print('RESERVA_FRAME',i,frame,flush=True)
print('RESERVA_FINAL_11',flush=True)
