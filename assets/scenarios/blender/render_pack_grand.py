from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy
from pathlib import Path
ROOT=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender')
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Linthesia_Pack_7_Escenarios.blend'))
names=['Minimalista','Concierto oscuro','Neón futurista','Auditorio clásico',
       'Galaxia','Fuego cinematográfico','Pianista animado']
for index,name in enumerate(names):
 sc=bpy.data.scenes[name];bpy.context.window.scene=sc
 sc.render.engine='CYCLES';sc.cycles.samples=12
 sc.render.resolution_x=960;sc.render.resolution_y=540
 sc.render.resolution_percentage=100
 sc.render.image_settings.file_format='PNG'
 sc.frame_set(1)
 sc.render.filepath=str(ROOT/('escenario_%02d.png'%index))
 bpy.ops.render.render(write_still=True,scene=sc.name)
 print('COLA_FONDO_COMPLETADO',index,name,flush=True)
sc=bpy.data.scenes[names[-1]];bpy.context.window.scene=sc
for k,frame in enumerate((7,13,19,25,31,37,43,49,55,61),1):
 sc.frame_set(frame)
 sc.render.filepath=str(ROOT/('pianista_%02d.png'%frame))
 bpy.ops.render.render(write_still=True,scene=sc.name)
 print('COLA_ANIMACION_COMPLETADA',k,frame,flush=True)
print('BLENDER_18_FOTOGRAMAS_TERMINADOS',flush=True)
