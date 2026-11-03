from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
"""Keep the CC0 anatomical palm geometry editable in the Blender project.
Scene 'Palmas CC0 (edición)' is NOT included in 3D backdrop exports.
"""
import bpy,struct
from pathlib import Path
root=Path(f'{_LINTHESIA_ROOT}')
src=root/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'
bpy.ops.wm.open_mainfile(filepath=str(src))
active=bpy.context.window.scene
name='Palmas CC0 (edición)'
sc=bpy.data.scenes.get(name)
if sc is None:sc=bpy.data.scenes.new(name)
for old in list(sc.objects):
 bpy.data.objects.remove(old,do_unlink=True)
skin=bpy.data.materials.get('HUMANO | piel cálida CC0')
assert skin and skin.use_nodes
for hand in range(2):
 path=root/f'graphics/scenarios/mesh_palma_cc0_{hand:02d}.l3d'
 data=path.read_bytes()
 assert data[:5]==b'L3DM\x03'
 tris,=struct.unpack_from('<I',data,8)
 vertices=[];texcoords=[];faces=[]
 for tri in range(tris):
  first=len(vertices)
  for j in range(3):
   off=12+(3*tri+j)*36
   x,y,z,r,g,b,nx,ny,nz,u,v,flag=struct.unpack_from('<fffBBBfffffB',data,off)
   vertices.append((x+(.23 if hand else -.23),y,z))
   texcoords.append((u,v))
  faces.append((first,first+1,first+2))
 mesh=bpy.data.meshes.new(f'Palma MakeHuman {hand}');mesh.from_pydata(vertices,[],faces)
 mesh.update()
 uvmap=mesh.uv_layers.new(name='UV piel MakeHuman')
 for loop in mesh.loops:uvmap.data[loop.index].uv=texcoords[loop.vertex_index]
 mesh.materials.append(skin)
 for polygon in mesh.polygons:polygon.use_smooth=True
 obj=bpy.data.objects.new('Palma humana CC0 | '+('izquierda' if hand==0 else 'derecha'),mesh)
 sc.collection.objects.link(obj)
 obj['licencia']='MakeHuman CC0 1.0'
 obj['motor']='Malla anatómica fija para palma; dedos cinemáticos MIDI en C++'
 print('BLENDER_PALM_EDITABLE',hand,len(mesh.polygons),flush=True)
bpy.context.window.scene=active
bpy.ops.wm.save_as_mainfile(filepath=str(src))
print('BLENDER_HAND_GEOMETRY_SAVED',flush=True)
