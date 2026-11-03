from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
# SPDX-License-Identifier: GPL-2.0-or-later
# Seven editable scene cameras and high-resolution preview assets.
import bpy,math
from pathlib import Path
from mathutils import Vector
root=Path(f'{_LINTHESIA_ROOT}')
asset=root/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'
out=root/'graphics/scenarios'
preview=root/'tests/capturas_7_escenarios_blender'
preview.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(asset))
names=['Pianista animado','Guitarrista','Baterista','Cantante',
       'Cantante femenina','Dúo vocal','Banda completa']
assert len(bpy.data.scenes)==7,set(bpy.data.scenes.keys())
for nr,name in enumerate(names,6):
 sc=bpy.data.scenes[name]
 bpy.context.window.scene=sc
 sc.frame_set(1)
 for obj in list(sc.objects):
  if obj.type=='CAMERA' and obj.name.startswith('LINTHESIA | Cámara'):
   bpy.data.objects.remove(obj,do_unlink=True)
 if nr==6:
  location=(2.1,-3.45,2.9);target=(-.48,.24,1.20)
 elif nr==12:
  location=(-.48,-9.3,3.15);target=(-.48,.35,1.35)
 elif nr==11:
  location=(-.48,-5.6,2.6);target=(-.48,.34,1.35)
 else:
  location=(-.48,-5.15,2.68);target=(-.48,.31,1.36)
 bpy.ops.object.camera_add(location=location)
 cam=bpy.context.object
 cam.name=f'LINTHESIA | Cámara escenario {nr-5}'
 cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
 cam.data.type='ORTHO'
 cam.data.ortho_scale=9.5 if nr==12 else (5.7 if nr==11 else (4.7 if nr==6 else 4.4))
 sc.camera=cam
 sc.render.engine='BLENDER_WORKBENCH'
 sc.display.shading.light='STUDIO'
 sc.display.shading.color_type='MATERIAL'
 sc.display.shading.show_cavity=True
 sc.display.shading.show_shadows=True
 sc.render.resolution_x=1280
 sc.render.resolution_y=720
 sc.render.resolution_percentage=100
 sc.render.image_settings.file_format='PNG'
 sc.render.filepath=str(preview/f'escena_{nr:02d}.png')
 sc.render.film_transparent=False
 bpy.ops.render.render(write_still=True)
 print('RENDERED_BLENDER',name,sc.render.filepath,flush=True)
bpy.context.window.scene=bpy.data.scenes['Pianista animado']
bpy.ops.wm.save_as_mainfile(filepath=str(asset))
print('BLEND_7_SCENES_WITH_CAMERAS_SAVED',flush=True)
