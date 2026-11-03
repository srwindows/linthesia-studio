from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
# SPDX-License-Identifier: GPL-2.0-or-later
# Procedural, editable Blender concert performers for Linthesia.
# Seven interactive choices: pianist + four solo performers + vocal duo + band.
import bpy, math, struct
from pathlib import Path
from mathutils import Vector
ROOT=Path(f'{_LINTHESIA_ROOT}')
BLEND=ROOT/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'
OUT=ROOT/'graphics/scenarios'
bpy.ops.wm.open_mainfile(filepath=str(BLEND))
# Idempotent rebuild: remove the five previously generated scenes only.
# The licensed CC0 anatomical head is reused for male musicians.
_native_src=bpy.data.scenes['Pianista animado'].objects['HUMANO CC0 | cabeza anatómica']
_native_head_center=Vector((-.480,-1.14055,2.25764))
def anatomical_head(label,x,y,female=False):
 mesh=_native_src.data.copy()
 for vert in mesh.vertices:vert.co-=_native_head_center
 ob=bpy.data.objects.new('PERSONA CC0 | cabeza UV '+label,mesh)
 bpy.context.scene.collection.objects.link(ob)
 ob.location=(x,y,1.775)
 # The native CC0 head originally looked towards the pianist's keyboard
 # (+Y), leaving the singer and guitarist facing AWAY from their audience.
 # Rotate just the head toward the audience (-Y); instruments stay playable.
 ob.rotation_euler[2]=math.pi
 ob.scale=(.82 if female else .88,.82 if female else .88,
           .82 if female else .86)
 for poly in ob.data.polygons:poly.use_smooth=True
 return ob

for generated_name in ('Guitarrista','Baterista','Cantante',
                       'Cantante femenina','Dúo vocal','Banda completa'):
 old=bpy.data.scenes.get(generated_name)
 if old:bpy.data.scenes.remove(old)


def mat(name,rgb,metal=0,rough=.47):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.diffuse_color=(*rgb,1);m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF')
 bs.inputs['Base Color'].default_value=(*rgb,1)
 bs.inputs['Metallic'].default_value=metal
 bs.inputs['Roughness'].default_value=rough
 return m
skin=mat('ESC | Piel cálida',(.60,.365,.26))
darkskin=mat('ESC | Piel oscura',(.37,.21,.15))
skinlight=mat('ESC | Piel clara',(.81,.57,.44))
hairdark=mat('ESC | Pelo castaño',(.078,.046,.034))
hairgold=mat('ESC | Pelo rubio',(.47,.29,.105))
hairblack=mat('ESC | Pelo negro',(.016,.019,.025))
white=mat('ESC | Blanco mate',(.72,.75,.80))
red=mat('ESC | Granate',(.27,.032,.062))
blue=mat('ESC | Azul eléctrico',(.05,.14,.34))
teal=mat('ESC | Teal',(.024,.26,.26))
black=mat('ESC | Negro semimate',(.025,.028,.04),.14)
steel=mat('ESC | Acero satinado',(.34,.39,.44),.65)
chrome=mat('ESC | Plata cromada',(.56,.62,.69),.86)
gold=mat('ESC | Latón',(.49,.31,.082),.53)
rosewood=mat('ESC | Caoba',(.27,.058,.042),.14)
sunburst=mat('ESC | Guitarra ámbar',(.72,.36,.068),.21)
drumred=mat('ESC | Batería rojo vino',(.34,.035,.064),.28)
stagefloor=mat('ESC | Tarima',(.085,.093,.107))
stageback=mat('ESC | Fondo concert hall',(.040,.052,.074))
ledblue=mat('ESC | Luz azul',(.065,.31,.78),.13)
ledorange=mat('ESC | Luz ámbar',(.83,.43,.10),.11)

def cube(name,p,sz,material,bevel=0.0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p)
 ob=bpy.context.object;ob.name=name;ob.dimensions=sz
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 ob.data.materials.append(material)
 if bevel:
  mod=ob.modifiers.new('Bordes suavizados','BEVEL')
  mod.width=min(bevel, min(sz)*.25);mod.segments=2
  ob.modifiers.new('Normales','WEIGHTED_NORMAL')
 return ob
def ball(name,p,sz,material,smooth=True):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=16,location=p)
 ob=bpy.context.object;ob.name=name;ob.scale=sz
 ob.data.materials.append(material)
 if smooth:
  for face in ob.data.polygons:face.use_smooth=True
 return ob
def link(name,a,b,r,material,segments=16):
 a,b=Vector(a),Vector(b);mid=(a+b)*.5;axis=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=max(.001,axis.length),location=mid)
 ob=bpy.context.object;ob.name=name
 if axis.length>.001:ob.rotation_euler=axis.to_track_quat('Z','Y').to_euler()
 ob.data.materials.append(material)
 return ob
def drum(name,p,rad,depth,skinmat):
 bpy.ops.mesh.primitive_cylinder_add(vertices=32,radius=rad,depth=depth,location=p)
 ob=bpy.context.object;ob.name=name;ob.data.materials.append(skinmat)
 # Genuine ROUND drumheads. The previous square white cube covers looked
 # like white cardboard from the cymbal cameras.
 bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=rad*.955,
           depth=.008,location=(p[0],p[1],p[2]+depth*.5+.004))
 head=bpy.context.object;head.name=name+' | parche circular'
 head.data.materials.append(white)
 ring=ball(name+' | montura central',(p[0],p[1],p[2]+depth*.5+.015),
           (.025,.025,.010),chrome)
 for k in range(8):
  angle=k*math.tau/8
  a=(p[0]+rad*math.cos(angle),p[1]+rad*math.sin(angle),p[2]-depth*.43)
  b=(a[0],a[1],p[2]+depth*.43)
  link(name+' | tensor',a,b,.006,chrome)
 return ob
def cymbal(name,p,r):
 bpy.ops.mesh.primitive_cone_add(vertices=32,radius1=r,radius2=.026,depth=.022,location=p)
 ob=bpy.context.object;ob.name=name;ob.data.materials.append(gold)
 link(name+' | pie', (p[0],p[1],.08), (p[0],p[1],p[2]-.04),.013,chrome)
 return ob

animated=[]
def jacket(label,x,y,material,female=False):
 # Human-shaped continuous fitted coat, replacing the inflated egg torso.
 # 9 cross sections, 24-sided polygonal fabric with weighted normals.
 profile=(
  (.90,.175,.124),(.98,.220,.150),(1.09,.215,.146),
  (1.18,.195,.133),(1.25,.218,.145),(1.35,.262 if not female else .226,.158),
  (1.42,.267 if not female else .229,.152),
  (1.50,.208,.118),(1.56,.082,.066)
 )
 vertices=[];faces=[];n=24
 for z,rx,ry in profile:
  for j in range(n):
   ang=math.tau*j/n
   vertices.append((x+rx*math.cos(ang),y+ry*math.sin(ang),z))
 for k in range(len(profile)-1):
  for j in range(n):
   a=k*n+j;b=k*n+(j+1)%n
   faces.append((a,b,b+n,a+n))
 faces.append(tuple(range(n-1,-1,-1)))
 faces.append(tuple((len(profile)-1)*n+j for j in range(n)))
 me=bpy.data.meshes.new(label+' | chaqueta anatómica')
 me.from_pydata(vertices,[],faces);me.update()
 ob=bpy.data.objects.new(label+' | chaqueta sastrería',me)
 bpy.context.scene.collection.objects.link(ob);ob.data.materials.append(material)
 for poly in me.polygons:poly.use_smooth=True
 # Fabric lapels: four cloth polygons attached directly to jacket.
 for side in (-1,1):
  xx=x+side*.084
  link(label+' | solapa '+str(side),
       (x+side*.061,y-.166,1.51),(xx+side*.095,y-.154,1.22),
       .013,black)
 return ob

def anim(ob,typ,amp=.14):
 animated.append((ob,typ,tuple(ob.location),tuple(ob.rotation_euler),amp))
def performer(label,x,y,female=False,sk=skin,shirt=black,coat=blue,style='sing'):
 z=0
 sc='MUJER' if female else 'HOMBRE'
 # Dense enough for facial features while preserving a readable silhouette
 cube(label+' | botines izq',(x-.13,y-.04,.105),(.19,.30,.13),black,.035)
 cube(label+' | botines der',(x+.13,y-.04,.105),(.19,.30,.13),black,.035)
 for side in (-1,1):
  xx=x+side*.125
  link(label+' | pierna', (xx,y,.2),(xx+side*.012,y,.87),.086,coat)
  ball(label+' | rodilla',(xx,y,.50),(.079,.079,.085),coat)
 ball(label+' | pelvis',(x,y,.89),(.185,.128,.12),coat)
 jacket(label,x,y,coat,female=female)
 cube(label+' | camisa',(x,y-.146,1.38),(.080,.018,.225),shirt,.018)
 link(label+' | cuello',(x,y,1.52),(x,y,1.67),.066,sk)
 head=anatomical_head(label,x,y,female=female)
 anim(head,'head',.028)
 # Eyes and eyebrows on the front (camera is at negative Y)
 # Real UV facial anatomy replaces the geometric eyes, nose and lips;
 # their former overlaid spheres made the vocalists look like cartoons.
 cap=ball(label+' | pelo',(x,y+.014,1.887),(.149,.146,.102),hairgold if female else hairblack)
 if female:
  for side in (-1,1):
   for k in range(3):
    dx=side*(.112+.015*k)
    link(label+' | mechón', (x+dx,y+.002,1.916), (x+dx+side*.014,y+.055,1.38-.06*k), .031,hairgold,10)
  # Clean garment silhouette in place of swollen shoulder/bust spheres.
  # The skirt follows the jacket's lower body; avoid a swollen sphere.
  jacket(label+' vestido',x,y,shirt,female=True)
  ball(label+' | joya',(x,y-.176,1.47),(.018,.014,.018),gold)
 elif style=='sing':
  link(label+' | corbata',(x,y-.190,1.52),(x,y-.194,1.30),.018,red)
 for side in (-1,1):
  sx=x+side*(.25 if female else .285)
  ball(label+' | hombro'+str(side),(sx,y,1.468),(.097,.095,.095),coat)
  # Each forearm animated independently; arm bone remains an editable mesh
  if style=='drums':
   # Palms hold the actual drumstick base, not the empty air below the kit.
   target=(x+side*.40,y-.24,1.37)
  elif style=='guitar':
   # Left hand articulates near strings, right hand supports guitar neck.
   # The original exaggerated spread left both hands floating beside it.
   target=(x-.115,y-.445,1.20) if side<0 else (x+.355,y-.374,1.265)
  else:
   # One hand grips microphone stem; other arm rests alongside the torso.
   target=(x-.02,y-.19,1.38) if side<0 else (x+.31,y-.17,1.10)
  arm=link(label+' | antebrazo '+str(side),(sx,y,1.47),target,.069,coat)
  hand=ball(label+' | mano '+str(side),target,(.055,.049,.066),sk)
  anim(hand,'hands',.06)
  if style=='sing':anim(arm,'singer_arm',.10)
  elif style=='drums':anim(arm,'drum_arm',.22)
  elif style=='guitar':anim(arm,'strum_arm',.17)
 return head

def microphone(label,x,y,height=1.56):
 link(label+' | pie de micrófono',(x,y,.05),(x,y,height-.1),.021,steel)
 ball(label+' | contrapeso',(x,y,.052),(.18,.17,.040),black)
 link(label+' | micrófono',(x,y,height-.05),(x,y-.21,height+.06),.024,black)
 ball(label+' | cápsula',(x,y-.25,height+.08),(.040,.060,.042),chrome)
 for i in range(3):
  link(label+' | aro '+str(i),(x-.041,y-.265,height+.045+i*.025),
       (x+.041,y-.265,height+.045+i*.025),.0035,steel)

def guitar(label,x,y,bass=False):
 # Offset instrument outward and forward: natural playable diagonal
 bx=x+.065;by=y-.31;bz=1.145
 # Fender-like asymmetric cutaway silhouette, solid body with thickness
 # and rounded lacquered edges instead of the old oval 'banjo' shape.
 contour=[(-.18,-.29),(-.28,-.23),(-.32,-.12),(-.26,.02),
          (-.18,.095),(-.24,.17),(-.20,.29),(-.07,.31),
          (.01,.17),(.13,.18),(.20,.29),(.30,.27),
          (.33,.16),(.24,.045),(.24,-.13),(.19,-.26),(.07,-.32)]
 count=len(contour)
 vertices=[(bx+dx,by+dy,bz+dz) for dy in (.047,-.047)
             for dx,dz in contour]
 faces=[tuple(range(count)),tuple(range(count,2*count))]
 faces.extend((i,(i+1)%count,(i+1)%count+count,i+count)
              for i in range(count))
 mesh=bpy.data.meshes.new(label+' | cuerpo perfil de guitarra')
 mesh.from_pydata(vertices,[],faces);mesh.update()
 ob=bpy.data.objects.new(label+' | guitarra sólida',mesh)
 bpy.context.scene.collection.objects.link(ob)
 mesh.materials.append(sunburst if not bass else red)
 bevel=ob.modifiers.new('Perfil lacado redondeado','BEVEL')
 bevel.width=.028;bevel.segments=3
 ob.modifiers.new('Normales','WEIGHTED_NORMAL')
 ball(label+' | golpeador',(bx-.035,by-.102,bz+.035),(.125,.012,.165),white)
 a=(bx+.10,by,bz+.16);b=(bx+.69,by,bz+.37)
 link(label+' | mástil',a,b,.055,rosewood)
 link(label+' | pala',b,(b[0]+.14,b[1],b[2]+.05),.069,rosewood)
 for k in range(10):
  t=k/11;xk=a[0]+(b[0]-a[0])*t;zk=a[2]+(b[2]-a[2])*t
  link(label+' | traste '+str(k),(xk,by-.060,zk-.044),(xk,by-.060,zk+.044),.004,chrome)
 for k in range(4 if bass else 6):
  yy=by-.10+(.017*k if not bass else .024*k)
  link(label+' | cuerda '+str(k),(bx-.15,yy,bz-.10),(b[0]+.09,yy,b[2]+.05),.0025,steel,6)
  ball(label+' | clavija '+str(k),(b[0]+.04+k*.018,by-.025,b[2]+.07),(.016,.027,.015),chrome)
 for k in range(2 if bass else 3):
  cube(label+' | pastilla '+str(k),(bx-.12+k*.095,by-.113,bz+.015),(.040,.012,.13),black,.002)
 # Strumming hand movement visible against the pick guard
 strum=ball(label+' | mano rasgueo',(bx-.15,by-.15,bz+.09),(.065,.044,.07),skinlight)
 anim(strum,'strum',.095)
 link(label+' | correa',(x-.25,y+.01,1.45),(bx+.12,by+.06,bz-.17),.027,black)

def drums(label,x,y):
 drum(label+' | bombo',(x,y-.40,.38),.37,.48,drumred)
 drum(label+' | caja',(x-.61,y-.51,.86),.20,.19,drumred)
 for i,(dx,dy,dz,rad) in enumerate([( -.21,-.04,1.12,.17),(.22,-.04,1.12,.18),(-.45,.24,.77,.21),(.42,.30,.76,.23)]):
  drum(label+' | tom '+str(i),(x+dx,y+dy,dz),rad,.21,drumred)
 cymbal(label+' | crash izq',(x-.85,y-.04,1.44),.34)
 cymbal(label+' | crash der',(x+.90,y+.18,1.46),.36)
 cymbal(label+' | hi-hat',(x-.90,y-.60,1.05),.27)
 cymbal(label+' | ride',(x+.83,y-.52,1.14),.30)
 cube(label+' | asiento',(x,y+.35,.64),(.43,.35,.12),black,.04)
 for k in (-1,1):
  xx=x+k*.40;stick=link(label+' | BAQUETA '+str(k),(xx,y-.24,1.37),
                 (xx+k*.20,y-.50,1.03),.013,rosewood)
  anim(stick,'stickleft' if k<0 else 'stickright',.34)
 # Kick pedal
 cube(label+' | pedal bombo',(x,y-.86,.07),(.11,.23,.035),steel,.012)

def stage(name,full=False):
 sc=bpy.data.scenes.new(name)
 bpy.context.window.scene=sc
 size=(8.2,5.4,.23) if full else (4.7,4.0,.23)
 cube(name+' | Tarima base',(-.48,.28,.015),size,stagefloor,.03)
 cube(name+' | Fondo escenográfico',(-.48,2.45,1.75),(size[0],.10,3.5),stageback,.03)
 # Architectural details remain subtle so the falling MIDI lanes are readable.
 # Rounded metallic frames and acoustic panels replace featureless black boxes.
 for k in range(5 if full else 3):
  xx=-.48+(k-(2 if full else 1))*(1.40 if full else 1.23)
  cube(name+' | panel acústico '+str(k),(xx,2.38,1.77),
       (.93 if full else .83,.055,2.49),black,.055)
  for side in (-1,1):
   cube(name+' | perfil marco '+str(k)+str(side),
        (xx+side*(.47 if full else .42),2.32,1.77),
        (.023,.057,2.52),chrome,.008)
  cube(name+' | luz rasante panel '+str(k),(xx,2.30,.54),
       (.61,.026,.030),ledblue if k%2 else gold,.008)
 for edge in (-1,1):
  cube(name+' | tarima canto '+str(edge),
       (-.48+edge*(size[0]*.47),-.87,.16),
       (.045,.048,.095),steel,.008)
 # Separate front footlights create visible modelling under stage lighting.
 for n in range(5 if full else 3):
  xx=-.48+(n-(2 if full else 1))*(1.58 if full else 1.27)
  ball(name+' | luz suelo '+str(n),(xx,-1.08,.20),
       (.09,.095,.075),ledblue if n%2 else ledorange)
 for side in (-1,1):
  xx=-.48+side*(size[0]*.45)
  cube(name+' | rack laterales '+str(side),(xx,1.83,1.5),(.20,.28,2.75),black,.02)
  for k in range(3):
   ball(name+' | cabeza móvil '+str(side)+str(k),(xx,1.54,1.15+k*.65),
        (.09,.10,.09),ledblue if k%2 else ledorange)
 for k in range(6 if full else 4):
  xx=-.48+(k-(2.5 if full else 1.5))*(1.13 if full else 1.02)
  link(name+' | barra de luces '+str(k),(xx,1.5,2.8),(xx,2.10,3.18),.032,chrome)
  ball(name+' | PAR led '+str(k),(xx,1.45,2.92),(.11,.12,.16),ledblue if k%2 else ledorange)
 # Side audio and foldback monitors
 for dx in (-.45,.45) if not full else (-2.4,-1.3,.8,2.25):
  cube(name+' | Monitor suelo',(dx,-.95,.21),(.47,.38,.31),black,.03)
 return sc

# Individual stages. Distinct editable collections and instruments.
stages=[
 ('Guitarrista','guitarrista',8),
 ('Baterista','baterista',8),
 ('Cantante','cantante',8),
 ('Cantante femenina','cantantef',8),
 ('Dúo vocal','duo',8),
 ('Banda completa','banda',12),
]
for name,slug,count in stages:
 animated.clear()
 sc=stage(name,name=='Banda completa')
 if slug=='guitarrista':
  performer('Guitarrista principal',-.52,.45,shirt=red,coat=black,style='guitar')
  guitar('Guitarra eléctrica',-.52,.45)
  cube('Amplificador vintage',(1.09,1.15,.56),(.64,.38,.87),black,.035)
  cube('Rejilla altavoz',(1.09,.92,.56),(.54,.02,.70),steel,.01)
 elif slug=='baterista':
  performer('Baterista',-.48,1.18,sk=darkskin,shirt=black,coat=teal,style='drums')
  drums('Batería de concierto',-.48,.87)
 elif slug in ('cantante','cantantef'):
  female=slug=='cantantef'
  performer('Voz femenina' if female else 'Voz masculina',-.50,.15,
            female=female,sk=skinlight if female else darkskin,
            shirt=red if female else white,coat=red if female else blue,style='sing')
  microphone('Voz principal',-.52,-.15)
 elif slug=='duo':
  performer('Voz dúo masculina',-1.25,.22,sk=darkskin,
            shirt=white,coat=blue,style='sing')
  performer('Voz dúo femenina',.35,.22,female=True,sk=skinlight,
            shirt=red,coat=black,style='sing')
  microphone('Micrófono dúo masculino',-1.25,-.24)
  microphone('Micrófono dúo femenino',.35,-.24)
 elif slug=='banda':
  performer('Vocalista masculino',-2.15,-.20,sk=darkskin,shirt=white,coat=blue,style='sing')
  microphone('Micrófono vocal masculino',-2.12,-.47)
  performer('Vocalista femenina',-1.15,-.18,female=True,sk=skinlight,shirt=red,coat=black,style='sing')
  microphone('Micrófono vocal femenino',-1.11,-.45)
  performer('Guitarrista banda',.45,-.14,shirt=white,coat=red,style='guitar')
  guitar('Guitarra banda',.45,-.14)
  performer('Bajista',1.71,.18,sk=darkskin,coat=teal,style='guitar')
  guitar('Bajo eléctrico',1.71,.18,bass=True)
  performer('Baterista banda',.50,1.55,sk=skinlight,coat=black,style='drums')
  drums('Batería banda',.50,1.19)
  # Keyboardist is the sixth performer
  performer('Teclista banda',-2.28,1.36,sk=skin,coat=black,style='guitar')
  cube('Sintetizador 61 teclas',(-2.30,.87,1.08),(1.03,.39,.17),black,.016)
  for k in range(36):
   cube('Sintetizador blanca '+str(k),(-2.76+k*.0265,.66,1.174),
        (.023,.094,.017),white,.002)
  for xx in (-2.66,-1.95):
   link('Sintetizador soporte', (xx,.76,.12),(xx,.88,.91),.035,steel)
 # Animations are real Blender keyframes, not screen-space sprites.
 for n in range(count):
  frame=1+n*6
  th=math.tau*n/count
  sc.frame_set(frame)
  for ob,typ,base,rot,amp in animated:
   ob.location=base
   ob.rotation_euler=rot
   if typ=='head':
    ob.location.z=base[2]+amp*math.sin(th)
    ob.rotation_euler[2]=rot[2]+.08*math.sin(th) # preserve base frontal rotation
   elif typ=='hands':
    ob.location.z=base[2]+amp*.45*math.sin(th)
   elif typ in ('singer_arm','strum_arm'):
    ob.rotation_euler[0]=rot[0]+amp*math.sin(th)
    ob.location.y=base[1]-.025*math.cos(th)
   elif typ=='drum_arm':
    ob.rotation_euler[0]=rot[0]+amp*math.sin(th*2+(0 if 'izq' in ob.name else 1.4))
   elif typ=='strum':
    ob.location.z=base[2]+amp*math.sin(th*2)
    ob.location.x=base[0]+.018*math.cos(th*2)
   elif typ.startswith('stick'):
    ob.rotation_euler[0]=rot[0]+amp*math.sin(th*2+(0 if typ=='stickleft' else math.pi))
    ob.location.z=base[2]+.09*math.sin(th*2+(0 if typ=='stickleft' else math.pi))
   ob.keyframe_insert('location',frame=frame)
   ob.keyframe_insert('rotation_euler',frame=frame)
 sc.frame_set(1)
 print('NEW_SCENE',sc.name,'OBJECTS',len(sc.objects),'FRAMES',count,flush=True)

# Keep source Blender scenes separately editable by artist.
bpy.context.window.scene=bpy.data.scenes['Pianista animado']
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
print('SAVED_BLEND',BLEND,'SCENES',len(bpy.data.scenes),flush=True)

# Export precisely the binary v3 format consumed by OpenGL.
def export(sc,frame,path):
 bpy.context.window.scene=sc;sc.frame_set(frame)
 dg=bpy.context.evaluated_depsgraph_get()
 triangles=0
 with path.open('wb') as f:
  f.write(b'L3DM\x03\x00\x00\x00')
  f.write(struct.pack('<I',0))
  for ob in sc.objects:
   if ob.type!='MESH' or ob.hide_render:continue
   evalob=ob.evaluated_get(dg);mesh=evalob.to_mesh()
   try:
    mesh.calc_loop_triangles()
    matrix=evalob.matrix_world
    nm=matrix.to_3x3().inverted_safe().transposed()
    mats=[slot.material for slot in evalob.material_slots]
    uvdata=mesh.uv_layers.active.data if ob.name.startswith('PERSONA CC0 | cabeza UV') and mesh.uv_layers.active else None
    for tr in mesh.loop_triangles:
     face=mesh.polygons[tr.polygon_index]
     material=mats[face.material_index] if face.material_index<len(mats) else None
     color=material.diffuse_color[:3] if material else (.5,.5,.5)
     rgb=[int(max(0,min(255,round(v*255)))) for v in color]
     for vertex_index,loop_index in zip(tr.vertices,tr.loops):
      v=matrix@mesh.vertices[vertex_index].co
      normal=mesh.corner_normals[loop_index].vector if face.use_smooth else face.normal
      norm=nm@normal
      if norm.length<1e-9:norm=Vector((0,0,1))
      norm.normalize()
      if uvdata:
       uv=uvdata[loop_index].uv
       rgbout=(244,244,244)
       uu,vv=float(uv.x),float(uv.y);flags=1
      else:
       rgbout=rgb;uu=0.;vv=0.;flags=0
      f.write(struct.pack('<fffBBBfffffB',
       v.x,v.y,v.z,*rgbout,norm.x,norm.y,norm.z,uu,vv,flags))
     triangles+=1
   finally:evalob.to_mesh_clear()
  f.seek(8);f.write(struct.pack('<I',triangles))
 print('EXPORTED',path.name,'TRIANGLES',triangles,'BYTES',path.stat().st_size,flush=True)

for name,slug,count in stages:
 sc=bpy.data.scenes[name]
 for i in range(count):
  export(sc,1+i*6,OUT/f'mesh_{slug}_{i:02d}.l3d')
print('COMPLETE_SIX_MUSICIAN_SCENES',flush=True)
