from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,math,sys
from pathlib import Path
from mathutils import Vector
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
root=p.parent
# Execute torso replacement in this process without saving.
saveRequested='--save' in sys.argv
sys.argv=['blender','--python',str(root/'humano_cc0/torso_anatomico.py')]
exec(compile((root/'humano_cc0/torso_anatomico.py').read_text(),str(root/'humano_cc0/torso_anatomico.py'),'exec'))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
for ob in list(sc.objects):
 if ob.name.startswith('Pantalón pierna superior'):
  bpy.data.objects.remove(ob,do_unlink=True)
cloth=bpy.data.materials['Traje azul grafito']
pants=bpy.data.materials['Pantalón oscuro']
def shape(name,pos,radii,mat):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=28,ring_count=18,location=pos)
 o=bpy.context.object;o.name='HUMANO CC0 | '+name
 o.scale=radii;o.data.materials.append(mat)
 for f in o.data.polygons:f.use_smooth=True
 return o
def segment(name,a,b,rad,mat):
 a=Vector(a);b=Vector(b);axis=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=20,radius=rad,depth=axis.length,location=(a+b)*.5)
 ob=bpy.context.object;ob.name='HUMANO CC0 | '+name
 ob.rotation_euler=axis.to_track_quat('Z','Y').to_euler()
 ob.data.materials.append(mat)
 for f in ob.data.polygons:f.use_smooth=True
 return ob
ob=bpy.data.objects.get('HUMANO CC0 | torso anatómico vestido')
if ob:
 sub=ob.modifiers.new('Costuras suavizadas','SUBSURF')
 sub.levels=1;sub.render_levels=1
# Covered shoulder seams around separated C++ driven sleeves.
for side in (-1,1):
 shape('hombrera anatómica '+str(side),
       (-.48+side*.275,-1.21,1.75),(.079,.105,.081),cloth)
 segment('muslo sentado '+str(side),
       (-.48+side*.185,-1.54,.99),
       (-.48+side*.205,-1.12,.52),.106,pants)
# Close the neckline with tailored fabric instead of the old white collar.
shape('solapa trasera',(-.48,-1.304,1.925),(.20,.067,.043),cloth)
if saveRequested:
 bpy.ops.wm.save_as_mainfile(filepath=str(p))
 print('ANATOMICAL_SUIT_FINAL_SAVED')
sc.frame_set(1);sc.render.engine='CYCLES';sc.cycles.samples=15
sc.render.resolution_x=960;sc.render.resolution_y=540
sc.render.filepath=str(root/'pianista_torso_final_prueba.png')
bpy.ops.render.render(write_still=True,scene=sc.name)
print('GARMENT_PREVIEW_FINISHED')
