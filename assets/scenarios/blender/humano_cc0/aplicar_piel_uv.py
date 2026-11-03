from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
# SPDX-License-Identifier: GPL-2.0-or-later
# Restore native UV coordinates from CC0 MakeHuman OBJ to the anatomical head.
# This preserves original photographic texture without baking a 2D face image.
from pathlib import Path
import bpy
from mathutils import Vector
R=Path(f'{_LINTHESIA_ROOT}')
P=R/'assets/scenarios/blender/humano_cc0'
BLEND=R/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'
bpy.ops.wm.open_mainfile(filepath=str(BLEND))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
head=sc.objects.get('HUMANO CC0 | cabeza anatómica')
assert head and head.type=='MESH'
pos=[];uv=[];faces=[];group=''
for row in (P/'base.obj').open():
 if row.startswith('v '):
  pos.append(tuple(map(float,row.split()[1:4])))
 elif row.startswith('vt '):
  uv.append(tuple(map(float,row.split()[1:3])))
 elif row.startswith('g '):group=row[2:].strip()
 elif row.startswith('f ') and group=='body':
  loops=[tuple(int(i)-1 for i in part.split('/')[:2]) for part in row.split()[1:]]
  if all(pos[i][1]>=5.95 for i,j in loops):
   faces.append(loops)
assert len(faces)==len(head.data.polygons), (len(faces),len(head.data.polygons))
layer=head.data.uv_layers.get('UV MakeHuman original') or head.data.uv_layers.new(name='UV MakeHuman original')
for polygon, face in zip(head.data.polygons,faces):
 assert len(polygon.loop_indices)==len(face)
 for loop_id, (vertex_id, uv_id) in zip(polygon.loop_indices,face):
  layer.data[loop_id].uv=uv[uv_id]
mat=head.data.materials[0]
img=bpy.data.images.load(str(P/'textures/middleage_male_skin_diffuse.png'),check_existing=True)
img.colorspace_settings.name='sRGB'
mat.use_nodes=True
nodes=mat.node_tree.nodes;links=mat.node_tree.links
base=nodes.get('Principled BSDF')
for node in list(nodes):
 if node.type=='TEX_IMAGE' and node.name.startswith('SKIN_MAP'):nodes.remove(node)
node=nodes.new('ShaderNodeTexImage');node.name='SKIN_MAP_PHOTOGRAPHIC_CC0';node.image=img
links.new(node.outputs['Color'],base.inputs['Base Color'])
base.inputs['Roughness'].default_value=.74
base.inputs['Subsurface Weight'].default_value=.11
base.inputs['Subsurface Radius'].default_value=(.68,.31,.19)
mat.diffuse_color=(1,1,1,1)
head['textura_piel']='middleage_lightskinned_male_diffuse.png; MakeHuman CC0'
head['uv_coordinate']='MakeHuman CC0 base.obj vt indices'
assert len(head.data.uv_layers)>0
sc.render.engine='CYCLES';sc.cycles.samples=24
sc.render.resolution_x=960;sc.render.resolution_y=540
# Side/front portrait to check UV orientation, not writing camera transform
camera=sc.camera
old_location=camera.location.copy()
old_rotation=camera.rotation_euler.copy()
camera.location=(2.35,-.78,2.72)
camera.rotation_euler=(Vector((-.48,-1.13,2.12))-camera.location).to_track_quat('-Z','Y').to_euler()
sc.frame_set(1)
sc.render.filepath=str(P/'retrato-piel-uv-prueba.png')
bpy.ops.render.render(write_still=True,scene=sc.name)
camera.location=old_location;camera.rotation_euler=old_rotation
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
print('UV_HEAD_SAVED',len(faces),len(uv),img.size[:],flush=True)
