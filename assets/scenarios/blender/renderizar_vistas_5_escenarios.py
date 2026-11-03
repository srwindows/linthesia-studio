from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,math
from pathlib import Path
from mathutils import Vector
r=Path(f'{_LINTHESIA_ROOT}')
bpy.ops.wm.open_mainfile(filepath=str(r/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'))
pairs=[('Guitarrista',7),('Baterista',8),('Cantante',9),('Cantante femenina',10),('Banda completa',11)]
for name,idx in pairs:
 sc=bpy.data.scenes[name]
 bpy.context.window.scene=sc
 sc.frame_set(1)
 bpy.ops.object.camera_add(location=(-.4,-9.0 if idx==11 else -5.3,3.6 if idx==11 else 2.7))
 cam=bpy.context.object;cam.name='ESC | Cámara miniatura '+name
 look=Vector((-.48,.32,1.27))-cam.location
 cam.rotation_euler=look.to_track_quat('-Z','Y').to_euler()
 cam.data.type='ORTHO'
 cam.data.ortho_scale=8.8 if idx==11 else 4.6
 sc.camera=cam
 sc.render.engine='BLENDER_WORKBENCH'
 sc.display.shading.light='STUDIO'
 sc.display.shading.color_type='MATERIAL'
 sc.display.shading.show_cavity=True
 sc.render.resolution_x=640;sc.render.resolution_y=360;sc.render.resolution_percentage=100
 sc.render.image_settings.file_format='PNG'
 sc.render.filepath=f'/tmp/linthesia-stage-preview-{idx:02d}.png'
 sc.world=bpy.data.worlds.new('Fondo oscuro '+name)
 sc.world.color=(.03,.045,.065)
 bpy.ops.render.render(write_still=True)
 print('PREVIEW_READY',name,sc.render.filepath,flush=True)
print('PREVIEWS_COMPLETE',flush=True)
