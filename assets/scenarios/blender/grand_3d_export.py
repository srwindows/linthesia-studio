from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy, math, struct
from pathlib import Path
from mathutils import Vector
ROOT=Path(f'{_LINTHESIA_ROOT}')
ASSET=ROOT/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'
OUT=ROOT/'graphics/scenarios'
bpy.ops.wm.open_mainfile(filepath=str(ASSET))
def mat(name,col,metal=0,rough=.3):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.diffuse_color=(*col,1);m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF')
 p.inputs['Base Color'].default_value=(*col,1)
 p.inputs['Metallic'].default_value=metal
 p.inputs['Roughness'].default_value=rough
 return m
ebony=mat('Cola | Negro piano lacado',(.018,.021,.027),.40,.16)
red=mat('Cola | Madera interior caoba',(.28,.095,.043),.09,.44)
gold=mat('Cola | Marco de hierro fundido',(.40,.265,.10),.53,.31)
string=mat('Cola | Cuerdas de acero',(.39,.345,.24),.72,.29)
ivory=mat('Cola | Marfil mate',(.84,.82,.77),.02,.32)
dark=mat('Cola | Ébano mate',(.012,.015,.022),.1,.31)
felt=mat('Cola | Fieltro',(.10,.025,.024))
def mesh(name,points,faces,material):
 me=bpy.data.meshes.new(name);me.from_pydata(points,[],faces);me.update()
 ob=bpy.data.objects.new(name,me);bpy.context.scene.collection.objects.link(ob)
 me.materials.append(material)
 bevel=ob.modifiers.new('Aristas redondeadas','BEVEL');bevel.width=.023;bevel.segments=2
 ob.modifiers.new('Normales ponderadas','WEIGHTED_NORMAL')
 return ob
def block(name,pos,dims,m):
 bpy.ops.mesh.primitive_cube_add(size=1,location=pos)
 o=bpy.context.object;o.name=name;o.dimensions=dims
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.data.materials.append(m)
 bev=o.modifiers.new('Bordes','BEVEL');bev.width=min(.012,min(dims)*.15);bev.segments=2
 o.modifiers.new('Normales','WEIGHTED_NORMAL')
 return o
def rod(name,a,b,r,material):
 a=Vector(a);b=Vector(b);v=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=10,radius=r,depth=v.length,location=(a+b)*.5)
 o=bpy.context.object;o.name=name;o.rotation_euler=v.to_track_quat('Z','Y').to_euler()
 o.data.materials.append(material)
 return o
def oval(name,center,radius,material):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=10,location=center)
 o=bpy.context.object;o.name=name;o.scale=radius;o.data.materials.append(material)
 for f in o.data.polygons:f.use_smooth=True
 return o
# Realistic grand footprint: full width 1.48 m, depth 2.15 m; the previous
# oversized rectangular case measured 4.85m wide and only 1.9m deep.
outline=[(-1.26,.09),(.30,.09),(.38,.25),(.42,.55),(.39,.90),(.32,1.20),
         (.18,1.55),(-.08,1.86),(-.43,2.14),(-.76,2.18),(-1.11,2.07),
         (-1.24,1.75),(-1.26,1.15)]
def grand():
 n=len(outline)
 bottom=[(x,y,.76) for x,y in outline];top=[(x,y,.99) for x,y in outline]
 faces=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]
 faces+= [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
 body=mesh('COLA REAL | aro perfil curvo',bottom+top,faces,ebony)
 block('COLA | teclado integrado | bastidor',(-.48,-.27,.83),(1.55,.59,.17),ebony)
 block('COLA | traviesa del teclado',(-.48,-.56,.84),(1.57,.065,.23),ebony)
 block('COLA | puente continuo teclado y caja',(-.48,.036,.880),(1.55,.34,.185),ebony)
 block('COLA | bisagra y tapa de teclas',(-.48,-.242,.957),(1.45,.130,.038),ebony)
 block('COLA | liston tecla posterior',(-.48,-.310,.969),(1.43,.022,.012),felt)
 # Exact A0–C8 chromatic layout: 52 white + 36 black = 88 physical keys.
 white=0;black=0
 white_classes={0,2,4,5,7,9,11}
 for midi in range(21,109):
  if midi%12 in white_classes:
   x=-1.163+white*.02645
   block('COLA | tecla blanca MIDI%03d'%midi,(x,-.420,.950),(.024,.200,.040),ivory)
   white+=1
  else:
   x=-1.163+(white-.46)*.02645
   block('COLA | tecla negra MIDI%03d'%midi,
         (x,-.345,.988),(.014,.130,.065),dark)
   black+=1
 assert white==52 and black==36,(white,black)
 # Rear felt and gold interior harp.
 block('COLA | fieltro clavijas',(-.48,-.154,.984),(1.44,.11,.013),felt)
 # Soundboard gold harp and 48 actual strings, under raised grand lid.
 interior=[(-1.16,.25),(.30,.25),(.31,.73),(.22,1.33),(-.43,2.02),(-1.11,1.94)]
 verts=[(x,y,1.005) for x,y in interior]
 mesh('COLA | arpa dorada',verts,[tuple(range(len(verts)))],gold)
 for idx in range(30):
  x=-1.10+idx*.044
  back=1.85-.85*max(0,x+.45)
  rod('COLA | cuerda acústica %02d'%idx,(x,.33,1.014),
      (x*.7-.20,max(.64,back),1.014),.0032,string)
 # Raised lid hinges along bass side, raised toward curved high side.
 lidpts=[(x,y,1.06+.37*max(0,(x+1.26)/1.68)) for x,y in outline]
 mesh('COLA REAL | tapa abierta',lidpts,[tuple(range(n))],ebony)
 rod('COLA | soporte tapa',(.27,.70,1.02),(.27,.70,1.36),.016,gold)
 rod('COLA | bisagra tapa',(-1.255,.30,1.03),(-1.255,1.86,1.03),.016,gold)
 # Three legs, satin black, connected to rim (with foot plates).
 for xx,yy in [(-1.11,.34),(.25,.46),(-.57,1.99)]:
  block('COLA | pata perfilada',(xx,yy,.408),(.14,.14,.75),ebony)
  oval('COLA | zapata',(xx,yy,.13),(.13,.13,.06),ebony)
 block('COLA | lira pedales',(-.48,.22,.37),(.20,.12,.53),ebony)
 for x in [-.56,-.48,-.40]:
  rod('COLA | pedal',(x,.20,.15),(x,-.04,.13),.015,gold)
 # Bench rebalanced to piano width and performer height.
 block('COLA | banqueta cuero',(-.48,-1.55,.79),(.72,.39,.11),ebony)
 for x in (-.75,-.21):
  for y in (-1.73,-1.38):
   block('COLA | banqueta pata',(x,y,.39),(.052,.06,.75),ebony)
for index,sc in enumerate(list(bpy.data.scenes)):
 if sc.name!='Pianista animado':continue
 bpy.context.window.scene=sc
 for ob in list(sc.objects):
  if ob.name.startswith(('Piano de cola |','Tecla blanca ','Tecla negra ',
                          'Pata piano','Banco del pianista','Pata banco',
                          'COLA |','COLA REAL |')):
   bpy.data.objects.remove(ob,do_unlink=True)
 grand()
 print('NUEVO_GRAND',index,sc.name,flush=True)
# The previously corrected pianist already sits in front of the keys;
# adapt wrists to the real keyboard width and bench height.
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
for frame in (1,7,13,19,25,31,37,43,49,55,61):
 sc.frame_set(frame)
 for side in (-1,1):
  name='izquierda' if side<0 else 'derecha'
  hand=sc.objects.get('CONTROL MIDI | '+name)
  upper=sc.objects.get('BRAZO SUPERIOR | '+str(side))
  lower=sc.objects.get('ANTEBRAZO MÓVIL | '+str(side))
  elbow=sc.objects.get('Codo articulado | '+str(side))
  cuff=sc.objects.get('Puño camisa | '+str(side))
  if not all((hand,upper,lower,elbow,cuff)):continue
  phase=(frame-1)/6
  cx=-.48;lat=.032*math.sin(phase*.75+side)
  pressed=.026 if (frame+side)%3==0 else .007
  shoulder=Vector((cx+side*.335,-1.22,1.80))
  joint=Vector((cx+side*.43+lat*.6,-.91,1.30))
  wrist=Vector((cx+side*.48+lat,-.49,.999-pressed))
  hand.location=wrist;hand.keyframe_insert('location',frame=frame)
  elbow.location=joint;elbow.keyframe_insert('location',frame=frame)
  cuff.location=wrist+Vector((0,-.04,.012));cuff.keyframe_insert('location',frame=frame)
  for bone,a,b,r in ((upper,shoulder,joint,.115),(lower,joint,wrist,.094)):
   v=b-a;bone.location=a
   bone.rotation_mode='QUATERNION';bone.rotation_quaternion=v.to_track_quat('Z','Y')
   bone.scale=(r,r,v.length)
   for prop in ('location','rotation_quaternion','scale'):
    bone.keyframe_insert(prop,frame=frame)
 sc.frame_set(frame)
 # A second explicit camera animation remains, but live C++ orbit overrides it.
# Keep only the pianist stage. The original 7-scene .blend is saved
# separately under backups before this reduction.
retired={'Minimalista','Concierto oscuro','Neón futurista',
         'Auditorio clásico','Galaxia','Fuego cinematográfico'}
for old in list(bpy.data.scenes):
 if old.name in retired:bpy.data.scenes.remove(old)
# Preserve Guitarrista, Baterista, Cantante, Cantante femenina and Banda completa.
# Each frame of the original native project stays editable.
bpy.context.window.scene=sc
bpy.ops.wm.save_as_mainfile(filepath=str(ASSET))
print('BLEND_GRAND_SAVED',flush=True)
# Store compact binary mesh frames, positions & precomputed diffuse material
# colours. This is REAL perspective geometry, not a background panorama.
def export(sc,frame,dest):
 bpy.context.window.scene=sc
 sc.frame_set(frame)
 dg=bpy.context.evaluated_depsgraph_get()
 triangles=0
 with open(dest,'wb') as f:
  # Version 3: Blender per-corner normals, native UV and texture mask.
  # Format v1/v2 readers remain supported by the OpenGL client.
  f.write(b'L3DM\x03\x00\x00\x00')
  f.write(struct.pack('<I',0))
  for obj in sc.objects:
   if obj.type!='MESH' or obj.hide_render:continue
   # Every selectable 3D scene shares the SAME 88 animated key meshes.
   # Do not bake fixed grand piano keys into scenery: those static copies
   # visually hide the depressed live keys and cause z-fighting.
   if obj.name.startswith(('COLA | tecla blanca MIDI',
                           'COLA | tecla negra MIDI')):
    continue
   # In the animated-pianist scene the body and the hands are driven by
   # the live C++ MIDI rig rather than obsolete animation frames.
   if sc.name=='Pianista animado' and obj.name.startswith((
       'CONTROL MIDI |','BRAZO SUPERIOR |','ANTEBRAZO MÓVIL |',
       'Codo articulado |','Puño camisa |','Palma |','Dedo ')):
    continue
   evaluated=obj.evaluated_get(dg)
   me=evaluated.to_mesh()
   try:
    me.calc_loop_triangles()
    mm=evaluated.matrix_world
    normal_matrix=mm.to_3x3().inverted_safe().transposed()
    mats=[slot.material for slot in evaluated.material_slots]
    movingHead=(sc.name=='Pianista animado' and
                obj.name.startswith(('HUMANO CC0 | cabeza anatómica',
                                     'HUMANO CC0 | cabello')))
    textured=(sc.name=='Pianista animado' and
              obj.name.startswith('HUMANO CC0 | cabeza anatómica') and
              bool(me.uv_layers.active))
    uvmap=me.uv_layers.active.data if textured else None
    for tri in me.loop_triangles:
     matidx=me.polygons[tri.polygon_index].material_index
     m=mats[matidx] if matidx<len(mats) else None
     if m:
      c=m.diffuse_color
      rgb=[int(max(0,min(255,255*float(t)))) for t in c[:3]]
     else:rgb=[160,160,160]
     for vertex_id,loop_id in zip(tri.vertices,tri.loops):
      v=mm@me.vertices[vertex_id].co
      polygon=me.polygons[tri.polygon_index]
      if polygon.use_smooth:
       normal=me.corner_normals[loop_id].vector
      else:
       normal=polygon.normal
      n=(normal_matrix@normal)
      if n.length<1e-5:
       n=(normal_matrix@polygon.normal)
      if n.length<1e-5:
       n=Vector((0,0,1))
      n.normalize()
      # Photographic skin atlas is already correctly color-corrected; do
      # not multiply it again by the skin material's brown base tint.
      if textured:
       col=(244,244,244)
       uv=uvmap[loop_id].uv
       u,vtex=float(uv[0]),float(uv[1])
      else:
       col=rgb;u=0.;vtex=0.
      f.write(struct.pack('<fffBBBfffffB',
                   v.x,v.y,v.z,*col,n.x,n.y,n.z,u,vtex,
                   int(textured)|(2 if movingHead else 0)))
     triangles+=1
   finally:evaluated.to_mesh_clear()
  f.seek(8);f.write(struct.pack('<I',triangles))
 print('MALLA_3D',dest.name,'triangulos',triangles,'bytes',dest.stat().st_size,flush=True)
# Obsolete scenes are deliberately no longer exported.
for i,frame in enumerate((1,7,13,19,25,31,37,43,49,55,61)):
 export(sc,frame,OUT/('mesh_pianista_%02d.l3d'%i))
print('BLENDER_LIVE_3D_EXPORT_OK',flush=True)
