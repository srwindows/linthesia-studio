from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
import bpy,math,random
from pathlib import Path
from mathutils import Vector
random.seed(2026)
root=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender')
root.mkdir(parents=True,exist_ok=True)
names=['Minimalista','Concierto oscuro','Neón futurista','Auditorio clásico','Galaxia','Fuego cinematográfico','Pianista animado']
accents=[(.6,.7,.78),(.22,.4,.8),(.15,.8,1),(.85,.54,.28),(.4,.3,.9),(1,.28,.09),(.34,.6,1)]
def material(name,col,metal=0,em=0):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.diffuse_color=(*col,1)
 m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF')
 p.inputs['Base Color'].default_value=(*col,1)
 p.inputs['Roughness'].default_value=.3 if metal else .59
 p.inputs['Metallic'].default_value=metal
 p.inputs['Emission Color'].default_value=(*col,1)
 p.inputs['Emission Strength'].default_value=em
 return m
def cube(name,loc,dims,mat):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
 o=bpy.context.object;o.name=name;o.dimensions=dims
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.data.materials.append(mat)
 b=o.modifiers.new('Bordes suavizados','BEVEL');b.width=min(.05,min(dims)*.2);b.segments=2
 o.modifiers.new('Normales','WEIGHTED_NORMAL')
 return o
def sphere(name,loc,scale,mat):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,location=loc)
 o=bpy.context.object;o.name=name;o.scale=scale;o.data.materials.append(mat)
 for p in o.data.polygons:p.use_smooth=True
 return o
def rod(name,a,b,radius,mat):
 vec=Vector(b)-Vector(a);loc=(Vector(a)+Vector(b))*.5
 bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=radius,depth=vec.length,location=loc)
 o=bpy.context.object;o.name=name
 o.rotation_euler=vec.to_track_quat('Z','Y').to_euler()
 o.data.materials.append(mat)
 return o
def lamp(name,loc,col,power,target=(0,0,1.5)):
 bpy.ops.object.light_add(type='AREA',location=loc)
 o=bpy.context.object;o.name=name;o.data.shape='DISK';o.data.size=4.2;o.data.energy=power;o.data.color=col
 o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
 return o
def stage(idx):
 sc=bpy.data.scenes.new(names[idx]);bpy.context.window.scene=sc
 sc.render.engine='CYCLES';sc.cycles.samples=20;sc.cycles.use_denoising=False
 sc.render.resolution_x=960;sc.render.resolution_y=540;sc.render.resolution_percentage=100
 sc.render.image_settings.file_format='PNG';sc.view_settings.view_transform='Standard'
 for layer in sc.view_layers:layer.cycles.use_denoising=False
 sc.render.film_transparent=False
 sc.world=bpy.data.worlds.new('Mundo '+names[idx]);sc.world.color=(.018,.019,.027)
 col=accents[idx]
 floor=material(names[idx]+' suelo',(.09+.04*idx,.10,.13),.22)
 back=material(names[idx]+' pared',(.03+.023*idx,.04,.065))
 black=material('Piano de cola negro',(.013,.016,.025),.45)
 white=material('Teclas marfil',(.86,.87,.89))
 dark=material('Teclas grafito',(.014,.015,.020))
 metal=material('Cerchas y soportes',(.19,.20,.24),.68)
 cube('Escenario real | suelo',(0,1,-.20),(18,11,.20),floor)
 cube('Escenario real | pared',(0,5.2,3.1),(18,.20,7.0),back)
 cube('Tarima',(0,.50,0),(8.0,4.5,.18),metal)
 # 88-key grand, modeled in Blender as separate editable components
 cube('Piano de cola | caja',(0,.6,.78),(4.85,1.9,.28),black)
 cube('Piano de cola | frontal',(0,-.2,.80),(4.82,.18,.26),black)
 for i in range(52):
  cube('Tecla blanca %02d'%i,((i-25.5)*.091,-.33,.925),(.087,.46,.072),white)
 for i in range(36):
  cube('Tecla negra %02d'%i,((i-17.5)*.129,-.23,.977),(.053,.28,.095),dark)
 for x in [-1.94,1.94]:
  cube('Pata piano',(x,1.1,.33),(.17,.19,.67),black)
 cube('Banco del pianista',(0,-.90,.50),(1.02,.46,.10),black)
 for x in (-.39,.39):cube('Pata banco',(x,-.90,.23),(.07,.07,.46),metal)
 if idx in (0,3):
  wood=material('Paneles salón',(.38,.40,.40) if idx==0 else (.42,.21,.075))
  for i in range(-5,6):
   cube('Panel arquitectónico',(i*1.26,5.05,2.7),(.13,.14,5.4),wood)
 if idx in (1,6):
  rod('Estructura de luces',(-6,3.9,5.9),(6,3.9,5.9),.06,metal)
  for x in (-6,-3,0,3,6):
   rod('Cercha lateral',(x,3.9,.8),(x,3.9,5.9),.05,metal)
 if idx==2:
  lightmat=material('Tubos neón',(.02,.8,1),.06,5)
  purple=material('Neón violeta',(.52,.15,1),.10,3)
  for x in (-6,-4,-2,2,4,6):
   rod('Barras LED',(x,5,.3),(x,5,5.7),.055,lightmat if x%4==0 else purple)
 if idx==4:
  star=material('Estrellas profundas',(.55,.65,1),0,5)
  for i in range(115):
   sphere('Estrella', (random.uniform(-7,7),4.95,random.uniform(.1,6)),(.015,.015,.015),star)
 if idx==5:
  ember=material('Ascua incandescente',(1,.24,.018),0,3)
  for i in range(38):
   sphere('Partícula ambiental de fuego',(random.uniform(-7,7),4.8,random.uniform(.2,5)),(.025,.025,.18),ember)
 lamp('LUZ EDITABLE | Principal',(-3,-4,5),(1,.88,.75),1300)
 lamp('LUZ EDITABLE | Contraluz',(4,1.5,6),col,1800)
 lamp('LUZ EDITABLE | Relleno',(1,-4,2),(.42,.55,1),600)
 bpy.ops.object.camera_add(location=(.0,-10.8,4.05))
 cam=bpy.context.object;cam.name='CAMARA EDITABLE | Cine'
 cam.rotation_euler=(Vector((0,.5,1.45))-cam.location).to_track_quat('-Z','Y').to_euler()
 cam.data.lens=42;sc.camera=cam
 sc.frame_start=1;sc.frame_end=64;sc.render.fps=24
 return sc
def pianist(sc):
 skin=material('Piel del músico',(.46,.295,.225))
 lips=material('Labios discretos',(.27,.115,.12))
 coat=material('Traje elegante grafito',(.036,.042,.064))
 hair=material('Cabello natural',(.018,.019,.022))
 shirt=material('Camisa clara',(.77,.79,.78))
 sphere('Pianista | chaqueta 3D',(0,1.6,1.70),(.5,.36,.72),coat)
 sphere('Pianista | cuello camisa',(0,1.4,2.3),(.17,.15,.25),shirt)
 sphere('Pianista | cabeza anatómica',(0,1.36,2.60),(.26,.25,.32),skin)
 sphere('Pianista | cabello',(0,1.45,2.8),(.27,.25,.15),hair)
 sphere('Pianista | nariz anatómica',(0,1.102,2.57),(.056,.073,.075),skin)
 sphere('Pianista | labios',(0,1.118,2.47),(.076,.014,.017),lips)
 sphere('Pianista | mandíbula',(0,1.42,2.414),(.19,.154,.083),skin)
 for sx in (-1,1):
  rod('Ceja oscura',(sx*.055,1.125,2.73),(sx*.15,1.134,2.72),.012,hair)
  sphere('Mejilla anatómica',(sx*.154,1.27,2.55),(.105,.10,.10),skin)
  sphere('Cabello patilla',(sx*.22,1.38,2.73),(.05,.076,.147),hair)
 cube('Solapa izquierda',(-.22,1.31,2.06),(.21,.06,.46),shirt)
 cube('Solapa derecha',(.22,1.31,2.06),(.21,.06,.46),shirt)
 for s in (-1,1):
  sphere('Oreja anatomía',(s*.251,1.35,2.60),(.06,.075,.092),skin)
  sphere('Ojos',(s*.104,1.126,2.65),(.023,.016,.023),hair)
  rod('Pianista | antebrazo superior',(s*.44,1.62,2.08),(s*.64,1.10,1.66),.14,coat)
  # Controlador de muñeca y dedos realmente animados en Blender
  bpy.ops.object.empty_add(type='PLAIN_AXES',location=(s*.80,.22,1.13))
  hand=bpy.context.object;hand.name='CONTROL MIDI | '+('mano izquierda' if s<0 else 'mano derecha')
  palm=sphere('Palma mano',hand.location,(.20,.14,.08),skin)
  palm.parent=hand;palm.location=(0,0,0)
  for j in range(5):
   obj=sphere('Dedo %d | mano %s'%(j,s),(0,0,0),(.035,.118,.041),skin)
   obj.parent=hand;obj.location=((j-2)*.077,-.15,0)
  rod('Pianista | antebrazo',(s*.64,1.10,1.66),(s*.8,.22,1.13),.118,coat)
  for frame in range(1,65,4):
   t=(frame-1)/63
   hand.location=(s*(.77+.17*math.sin(math.tau*t*1.3+s)),.20+.04*math.cos(t*math.tau*2),1.12+.035*abs(math.sin(t*math.tau*4+s)))
   hand.keyframe_insert(data_path='location',frame=frame)
   for child in hand.children:
    if child.name.startswith('Dedo'):
     child.rotation_euler.x=.1+.26*max(0,math.sin(math.tau*4*t+float(child.name.split()[1])*.7+s))
     child.keyframe_insert('rotation_euler',frame=frame)
 cam=sc.camera
 for frame in range(1,65,8):
  t=(frame-1)/63.0
  cam.location=(.42*math.sin(t*math.tau),-10.7+.27*math.cos(t*math.tau),4.06+.2*math.sin(t*math.tau+.4))
  cam.rotation_euler=(Vector((0,.45,1.55))-cam.location).to_track_quat('-Z','Y').to_euler()
  cam.keyframe_insert('location',frame=frame);cam.keyframe_insert('rotation_euler',frame=frame)
 for ob in sc.objects:
  if ob.type=='LIGHT' and 'EDITABLE' in ob.name:
   for frame in [1,16,32,48,64]:
    ob.data.energy=630*(1+.21*math.sin(frame*.11+len(ob.name)))
    ob.data.keyframe_insert('energy',frame=frame)
bpy.ops.wm.read_factory_settings(use_empty=True)
for i in range(7):
 sc=stage(i)
 if i==6:pianist(sc)
 print('ESCENA_LISTA',i,sc.name,len(sc.objects),flush=True)
bpy.context.window.scene=bpy.data.scenes[names[6]]
bpy.ops.wm.save_as_mainfile(filepath=str(root/'Linthesia_Pack_7_Escenarios.blend'))
print('BLEND_SAVED',flush=True)
for i in range(7):
 sc=bpy.data.scenes[names[i]]
 bpy.context.window.scene=sc;sc.frame_set(1)
 sc.render.filepath=str(root/('escenario_%02d.png'%i))
 bpy.ops.render.render(write_still=True,scene=sc.name)
 print('RENDER_ESCENA',i,flush=True)
sc=bpy.data.scenes[names[6]]
for f in [7,13,19,25,31,37,43,49,55,61]:
 bpy.context.window.scene=sc;sc.frame_set(f)
 sc.render.filepath=str(root/('pianista_%02d.png'%f))
 bpy.ops.render.render(write_still=True,scene=sc.name)
 print('RENDER_PIANISTA',f,flush=True)
print('RENDERS_COMPLETOS',flush=True)
