from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,math,sys
from pathlib import Path
from mathutils import Vector
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
source=p.parent/'humano_cc0/base.obj'
bpy.ops.wm.open_mainfile(filepath=str(p))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
for o in list(sc.objects):
 if o.name.startswith(('Chaqueta entallada |','HUMANO CC0 | cadera traje',
                        'HUMANO CC0 | torso anatómico',
                        'HUMANO CC0 | chaleco')):bpy.data.objects.remove(o,do_unlink=True)
vertices=[];faces=[];grp=''
for line in source.open():
 if line.startswith('v '):vertices.append(tuple(map(float,line.split()[1:4])))
 elif line.startswith('g '):grp=line[2:].strip()
 elif line.startswith('f ') and grp=='body':
  ids=[int(a.split('/')[0])-1 for a in line.split()[1:]]
  if all(.10<=vertices[i][1]<=5.50 and abs(vertices[i][0])<1.93 for i in ids):
   faces.append(ids)
used=sorted(set(j for face in faces for j in face))
index={id:i for i,id in enumerate(used)}
def remap(v):
 x,y,z=v
 shift=.032*(y-1.5)
 return (-.48+x*.179,
         -1.17+(z-.48)*.179+shift,
         2.039+(y-6)*.179)
verts=[remap(vertices[k]) for k in used]
me=bpy.data.meshes.new('Torso anatómico vestido CC0')
me.from_pydata(verts,[],[[index[i] for i in face] for face in faces])
me.update()
ob=bpy.data.objects.new('HUMANO CC0 | torso anatómico vestido',me)
sc.collection.objects.link(ob)
suit=bpy.data.materials.get('Traje azul grafito')
if suit:ob.data.materials.append(suit)
for face in ob.data.polygons:face.use_smooth=True
# No spherical hip ornaments: tapered seated upper legs already exist.
# Neck opening stays behind the separate anatomically scaled neck.
print('ANATOMICAL_SUIT',len(verts),len(faces))
if '--save' in sys.argv:
 bpy.ops.wm.save_as_mainfile(filepath=str(p))
 print('SAVED_ANATOMICAL_TORSO')
sc.frame_set(1);sc.render.engine='CYCLES';sc.cycles.samples=16
sc.render.resolution_x=960;sc.render.resolution_y=540
sc.render.filepath=str(p.parent/'pianista_torso_anatomico_preview.png')
bpy.ops.render.render(write_still=True,scene=sc.name)
print('ANATOMICAL_TORSO_PREVIEW_RENDERED')
