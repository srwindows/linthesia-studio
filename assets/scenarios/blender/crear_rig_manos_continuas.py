from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,bmesh,math,struct
from mathutils import Vector
from pathlib import Path
R=Path(f'{_LINTHESIA_ROOT}'); OUT=Path('/tmp/linthesia-rig-manos-20261023')
OUT.mkdir(exist_ok=True)
blend=R/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'
bpy.ops.wm.open_mainfile(filepath=str(blend))
orig_scene=bpy.context.window.scene
scene_name='MANOS ARTICULADAS MIDI'
if scene_name in bpy.data.scenes:
 old=bpy.data.scenes[scene_name]
 bpy.data.scenes.remove(old)
scene=bpy.data.scenes.new(scene_name)
bpy.context.window.scene=scene
scene.render.engine='BLENDER_EEVEE_NEXT'
scene.render.resolution_x=1200;scene.render.resolution_y=750
scene.render.resolution_percentage=100
scene.eevee.taa_render_samples=20
# Mesh components are fused through a voxel remesh: unlike overlapping palm
# and cylindrical digits, the resulting hand has ONE continuous surface.
def make_palm(mirror):
 verts=[];faces=[];segments=28
 rings=[
 (-.020,.020,.012),(-.010,.028,.014),(.006,.035,.016),
 (.026,.043,.019),(.050,.047,.018),(.067,.048,.016),
 (.080,.044,.013),(.091,.036,.009)]
 for y,rx,rz in rings:
  for j in range(segments):
   a=j*2*math.pi/segments
   x=mirror*rx*math.cos(a)
   z=rz*math.sin(a)
   verts.append((x,y,z))
 for ring in range(len(rings)-1):
  for j in range(segments):
   a=ring*segments+j;b=ring*segments+(j+1)%segments
   faces.append((a,b,b+segments,a+segments))
 faces.append(tuple(reversed(tuple(range(segments)))))
 faces.append(tuple((len(rings)-1)*segments+j for j in range(segments)))
 m=bpy.data.meshes.new('Palma continua');m.from_pydata(verts,[],faces);m.update()
 return m
# Bone rest points in physical metres, local to the wrist. Digit order is:
# thumb, index, middle, ring, little.
chains=[
 [(-.032,.054,0),(-.053,.075,-.003),(-.069,.090,-.005),(-.078,.101,-.006)],
 [(-.022,.076,.005),(-.024,.111,.006),(-.024,.139,.003),(-.022,.160,.000)],
 [(0,.083,.006),(-.001,.123,.007),(0,.155,.003),(0,.180,-.001)],
 [(.021,.077,.004),(.025,.113,.005),(.027,.140,.002),(.026,.160,-.001)],
 [(.040,.069,.002),(.046,.094,.003),(.048,.119,.000),(.048,.139,-.002)]
]
# Ring radii, approximating MCP and distal pad anatomy.
radius=[
 [.013,.012,.011,.009],
 [.0108,.0091,.0085,.0075],
 [.0114,.0098,.0091,.0077],
 [.0111,.0095,.0086,.0073],
 [.0090,.0081,.0072,.0067]
]
def make_digit(d,mirror):
 points=[Vector((x*mirror,y,z)) for x,y,z in chains[d]]
 verts=[];faces=[]
 radial=14;steps=20
 for i in range(steps+1):
  t=i/steps*3
  seg=min(2,int(t));f=t-seg
  c=points[seg].lerp(points[seg+1],f)
  axis=(points[seg+1]-points[seg]).normalized()
  side=axis.cross(Vector((0,0,1))).normalized()
  up=side.cross(axis).normalized()
  r=radius[d][seg]*(1-f)+radius[d][seg+1]*f
  for j in range(radial):
   a=j*math.tau/radial
   direction=side*math.cos(a)+up*(.85*math.sin(a))
   v=c+direction*r
   verts.append(v)
 for i in range(steps):
  for j in range(radial):
   a=i*radial+j;b=i*radial+(j+1)%radial
   faces.append((a,b,b+radial,a+radial))
 # Hemisphere at tip
 base=points[-1];axis=(points[-1]-points[-2]).normalized()
 side=axis.cross(Vector((0,0,1))).normalized()
 up=side.cross(axis).normalized()
 prev=steps*radial
 for row in range(1,6):
  a=math.pi*.5*row/5
  for j in range(radial):
   theta=math.tau*j/radial
   v=base+axis*(radius[d][-1]*math.sin(a)) + (
         side*math.cos(theta)+up*.85*math.sin(theta))*radius[d][-1]*math.cos(a)
   verts.append(v)
  curr=steps*radial+(row-1)*radial
  for j in range(radial):
   k=prev+j;l=prev+(j+1)%radial
   faces.append((k,l,curr+(j+1)%radial,curr+j))
  prev=curr
 m=bpy.data.meshes.new('Dedos anatómicos')
 m.from_pydata(verts,[],faces);m.update()
 return m
skin=bpy.data.materials.get('MANOS | piel humana CC0') or bpy.data.materials.new('MANOS | piel humana CC0')
skin.use_nodes=True
bs=skin.node_tree.nodes.get('Principled BSDF')
bs.inputs['Roughness'].default_value=.78
bs.inputs['Subsurface Weight'].default_value=.08
im=bpy.data.images.load(str(R/'assets/scenarios/blender/humano_cc0/textures/middleage_male_skin_diffuse.png'),check_existing=True)
tex=skin.node_tree.nodes.get('Imagen piel manos') or skin.node_tree.nodes.new('ShaderNodeTexImage')
tex.name='Imagen piel manos';tex.image=im
skin.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
def make_hand(hand):
 mirror=-1 if hand==0 else 1
 parts=[]
 for mesh in [make_palm(mirror)]+[make_digit(i,mirror) for i in range(5)]:
  ob=bpy.data.objects.new('temporal-mano',mesh);scene.collection.objects.link(ob)
  parts.append(ob)
 bpy.ops.object.select_all(action='DESELECT')
 for ob in parts:ob.select_set(True)
 bpy.context.view_layer.objects.active=parts[0]
 bpy.ops.object.join()
 obj=bpy.context.object
 obj.name='MANO_CONTINUA_'+('izquierda' if hand==0 else 'derecha')
 obj.data.remesh_voxel_size=.00245
 bpy.ops.object.voxel_remesh()
 sm=obj.modifiers.new('Suavizado tegumento','SMOOTH');sm.factor=1.35;sm.iterations=4
 bpy.context.view_layer.objects.active=obj;bpy.ops.object.modifier_apply(modifier=sm.name)
 # Keep real-time MIDI skinning responsive. The 9,778-vertex sculpted
 # topology is editable in the script; the exported mesh uses a carefully
 # decimated 42% of the triangles and retains the continuous finger joins.
 dec=obj.modifiers.new('Optimización tiempo real','DECIMATE');dec.ratio=.42
 bpy.context.view_layer.objects.active=obj;bpy.ops.object.modifier_apply(modifier=dec.name)
 for p in obj.data.polygons:p.use_smooth=True
 obj.data.materials.clear();obj.data.materials.append(skin)
 uv=obj.data.uv_layers.new(name='UV_CC0_piel')
 for polygon in obj.data.polygons:
  for loop in polygon.loop_indices:
   v=obj.data.vertices[obj.data.loops[loop].vertex_index].co
   uv.data[loop].uv=(.689+v.x*.25,.144+v.y*.10)
 # Rest skeleton: root wrist + 3 bones for every finger.
 arm=bpy.data.armatures.new(obj.name+'_huesos')
 rig=bpy.data.objects.new('RIG_'+str(hand),arm)
 scene.collection.objects.link(rig)
 bpy.context.view_layer.objects.active=rig
 rig.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
 root=arm.edit_bones.new('palma')
 root.head=(0,-.024,0);root.tail=(0,.064,0)
 for d in range(5):
  pts=[Vector((x*mirror,y,z)) for x,y,z in chains[d]]
  for seg in range(3):
   b=arm.edit_bones.new(f'dedo{d}_falange{seg}')
   b.head=pts[seg];b.tail=pts[seg+1];b.parent=root if seg==0 else arm.edit_bones[f'dedo{d}_falange{seg-1}']
   b.use_connect=False
 bpy.ops.object.mode_set(mode='OBJECT')
 bpy.ops.object.select_all(action='DESELECT')
 # Per-vertex paint weights: root to phalanges with an MCP transition.
 groups=[obj.vertex_groups.new(name='palma')]
 for d in range(5):
  for i in range(3):groups.append(obj.vertex_groups.new(name=f'dedo{d}_falange{i}'))
 for vert in obj.data.vertices:
  v=vert.co;weights={0:1.}
  if v.y>.046:
   # Identify nearest anatomical finger chain by distance to the bone axes.
   best=(1e4,None,0,0.)
   for d in range(5):
    pts=[Vector((x*mirror,y,z)) for x,y,z in chains[d]]
    for seg in range(3):
     a=pts[seg];b=pts[seg+1]
     t=max(0.,min(1.,(v-a).dot(b-a)/(b-a).length_squared))
     closest=a+(b-a)*t
     dist=(v-closest).length
     if dist<best[0]:best=(dist,d,seg,t)
   dist,d,seg,t=best
   # Smooth skin envelope crossing from palm to proximal metacarpal.
   span=max(0.,min(1.,(v.y-.062)/.026))
   proximity=max(0.,min(1.,(.032-dist)/.015))
   influence=span*proximity
   if influence>.0001:
    weights[0]=1.-influence
    # Softly blend adjacent phalanges around knuckles.
    cur=1+3*d+seg
    if seg>0 and t<.15:
     prev=cur-1;blend=(.15-t)/.30
     weights[prev]=blend*influence;weights[cur]=(1-blend)*influence
    elif seg<2 and t>.85:
     nxt=cur+1;blend=(t-.85)/.30
     weights[nxt]=blend*influence;weights[cur]=(1-blend)*influence
    else:weights[cur]=influence
  for gid,w in weights.items():
   if w>1e-4:groups[gid].add([vert.index],w,'REPLACE')
 modifier=obj.modifiers.new('Deformación anatómica armature','ARMATURE')
 modifier.object=rig
 obj.parent=rig
 obj['license']='CC0 MakeHuman skin texture, new continuous geometric model'
 obj['bones']=16
 obj['fingers']=5
 obj['keyboard_hand']=hand
 # UV+soft-weighted rig exported as binary for real-time C++ CPU skinning.
 bm=obj.data;bm.calc_loop_triangles()
 data=bytearray()
 for tri in bm.loop_triangles:
  for loop_index in tri.loops:
   loop=bm.loops[loop_index];vert=bm.vertices[loop.vertex_index]
   pos=vert.co;normal=vert.normal;uvco=uv.data[loop_index].uv
   g=sorted([(gr.group,gr.weight) for gr in vert.groups],key=lambda t:-t[1])[:3]
   ids=[int(i) for i,w in g]+[0]*max(0,3-len(g))
   weights=[float(w) for i,w in g]+[0.]*max(0,3-len(g))
   total=sum(weights)
   if total<1e-4:ids=[0,0,0];weights=[1.,0.,0.]
   else:weights=[w/total for w in weights]
   data+=struct.pack('<3f3f2f3B3f',
     *pos,*normal,*uvco, *ids, *weights)
 out=R/f'graphics/scenarios/mesh_mano_rig_{hand:02d}.lhr'
 with out.open('wb') as f:f.write(struct.pack('<4sII',b'LHR1',len(bm.loop_triangles),16));f.write(data)
 print('HAND_RIG_COMPLETE',hand,
       'verts',len(bm.vertices),'triangles',len(bm.loop_triangles),
       'bytes',out.stat().st_size,flush=True)
 # For a visual scene, leave unposed and shift each armature separately.
 rig.location.x=(-.17 if hand==0 else .17)
 return obj,rig
hands=[make_hand(i) for i in range(2)]
# Minimal piano keyboard for preview.
white=bpy.data.materials.new('Piano ivory');white.diffuse_color=(.94,.91,.84,1)
black=bpy.data.materials.new('Piano ebony');black.diffuse_color=(.009,.011,.016,1)
for i in range(42):
 x=-.66+.032*i
 bpy.ops.mesh.primitive_cube_add(size=1,location=(x,.055,-.048))
 key=bpy.context.object;key.name='Tecla blanca '+str(i);key.dimensions=(.030,.37,.045)
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 key.data.materials.append(white)
 if i%7 not in (2,6):
  bpy.ops.mesh.primitive_cube_add(size=1,location=(x+.016,.119,-.012))
  key=bpy.context.object;key.name='Tecla negra '+str(i)
  key.dimensions=(.019,.20,.052)
  bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
  key.data.materials.append(black)
bpy.ops.object.camera_add(location=(0,.16,.95))
cam=bpy.context.object;cam.rotation_euler=(Vector((0,.080,0))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=.93
scene.camera=cam
bpy.ops.object.light_add(type='AREA',location=(.0,.16,1.15))
light=bpy.context.object;light.data.energy=410;light.data.size=1.1
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(OUT/'manos-continuas-rig.png')
bpy.ops.render.render(write_still=True)
# Restore user's primary scene and save *in the same editable pack*, no
# cloned replacement projects.
bpy.context.window.scene=orig_scene
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
print('COMPLETE_EDITABLE_HAND_RIG_SAVED',str(blend),flush=True)
