from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
# SPDX-License-Identifier: GPL-2.0-or-later
# Modest anatomical refinements to the editable Blender character.
# Preserve keyboard, stage, pose, audio, lighting and all other scenes.
import bpy, math, random
from mathutils import Vector
from pathlib import Path
path=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(path))
sc=bpy.data.scenes['Pianista animado']
bpy.context.window.scene=sc
rng=random.Random(3179)
for o in list(sc.objects):
 if o.name.startswith('REALISMO |') or o.name in ('Pianista sentado | cabello','Camisa en cuello'):
  bpy.data.objects.remove(o,do_unlink=True)
def material(name,col,roughness=.6):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.diffuse_color=(*col,1)
 m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF')
 bs.inputs['Base Color'].default_value=(*col,1)
 bs.inputs['Roughness'].default_value=roughness
 return m
skin=material('Piel humana | tono medio',(.45,.283,.195),.81)
warm=material('Piel humana | labios',(.29,.13,.112),.83)
hair=material('Cabello corto castaño',(.065,.043,.031),.89)
hairL=material('Cabello matiz cálido',(.09,.055,.037),.82)
brow=material('Pelo cejas',(.04,.028,.023),.85)
eyeWhite=material('Esclerótica natural',(.57,.52,.45),.7)
iris=material('Iris castaño',(.11,.067,.049),.75)
beard=material('Barba corta natural',(.18,.14,.11),.93)
suit=material('Traje azul oscuro de paño',(.035,.049,.079),.79)
seam=material('Costura sastre azul',(.065,.087,.115),.79)
shirt=material('Camisa gris perla',(.69,.70,.68),.85)
button=material('Botones traje oscuros',(.055,.06,.065),.65)
def smooth(obj):
 for polygon in obj.data.polygons:polygon.use_smooth=True
def ball(name,loc,scaling,mat,seg=24,rings=16):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=seg,ring_count=rings,location=loc)
 o=bpy.context.object;o.name='REALISMO | '+name
 o.scale=scaling;o.data.materials.append(mat);smooth(o)
 return o
def bone(name,p0,p1,r,mat):
 p0=Vector(p0);p1=Vector(p1);v=p1-p0
 bpy.ops.mesh.primitive_cylinder_add(vertices=10,radius=r,depth=v.length,location=(p0+p1)*.5)
 o=bpy.context.object;o.name='REALISMO | '+name
 o.rotation_euler=v.to_track_quat('Z','Y').to_euler()
 o.data.materials.append(mat);smooth(o);return o
head=sc.objects['Pianista sentado | cabeza']
head.scale=(.205,.217,.244)
head.location.z=2.245
head.data.materials.clear()
head.data.materials.append(skin)
smooth(head)
# Resculpt the bottom half of the head with a tapered human jaw.
# The mesh's vertex positions are normalized; scale is in object space.
for vertex in head.data.vertices:
 if vertex.co.z<-.30:
  factor=1.0+.10*vertex.co.z
  vertex.co.x*=factor
  vertex.co.y*=factor
ball('mandíbula estrecha',(-.48,-1.09,2.055),(.122,.127,.073),skin)
ball('mentón',(-.48,-1.035,2.037),(.091,.066,.038),skin)
for old in list(sc.objects):
 if old.name.startswith('Rostro perfil | ojo'):
  bpy.data.objects.remove(old,do_unlink=True)
for side in (-1,1):
 x=-.48+side*.079
 ball('esclerótica '+str(side),(x,-.978,2.281),(.027,.018,.013),eyeWhite,18,10)
 ball('iris '+str(side),(x,-.961,2.281),(.012,.008,.011),iris,18,10)
 bone('ceja '+str(side),
      (x-side*.030,-.986,2.322),(x+side*.030,-.986,2.318),.009,brow)
 ball('pómulo '+str(side),(-.48+side*.123,-1.005,2.184),
      (.067,.042,.050),skin,18,10)
 ball('patilla '+str(side),(-.48+side*.191,-1.158,2.324),
      (.022,.029,.083),hair,18,12)
ball('nariz puente',(-.48,-.977,2.232),(.031,.050,.059),skin)
ball('labio superior',(-.48,-.955,2.132),(.060,.013,.014),warm)
ball('labio inferior',(-.48,-.955,2.112),(.056,.011,.012),skin)
# UV-sculpted irregular hairstyle: follows the skull curvature and a
# raised forehead hairline; avoids the old flat, round "helmet/hat".
center=Vector((-.48,-1.17,2.245))
rx,ry,rz=.209,.226,.253
verts=[];faces=[];rings=13;segments=48
for ri in range(rings+1):
 fraction=ri/float(rings)
 for j in range(segments):
  az=math.tau*j/segments
  forward=max(0.,math.sin(az)) # positive Y is where the face looks
  polarMax=1.67-.60*forward
  pol=max(.0003,polarMax*fraction)
  texture=.006*math.sin(az*7+pol*24)+.003*math.sin(az*14+pol*8)
  point=(center.x+rx*math.sin(pol)*math.cos(az)*(1+texture),
         center.y+ry*math.sin(pol)*math.sin(az)*(1+texture),
         center.z+rz*math.cos(pol)+.0025)
  verts.append(point)
for ri in range(rings):
 for j in range(segments):
  idx=ri*segments+j;nxt=ri*segments+(j+1)%segments
  faces.append((idx,nxt,nxt+segments,idx+segments))
mesh=bpy.data.meshes.new('Hairline geometry');mesh.from_pydata(verts,[],faces);mesh.update()
ob=bpy.data.objects.new('REALISMO | cabello entallado 3D',mesh)
sc.collection.objects.link(ob);ob.data.materials.append(hair)
smooth(ob)
# Fine hair strands create tactile variation in the light at orbital angles.
for k in range(120):
 az=rng.uniform(0,math.tau)
 front=max(0,math.sin(az))
 polMax=1.67-.60*front
 t=rng.uniform(.14,.94)
 p0=polMax*t
 p1=min(polMax,p0+rng.uniform(.04,.105))
 side=(.003 if k%4 else -.003)
 def loc(pol):
  return (center.x+(rx+side)*math.sin(pol)*math.cos(az),
          center.y+(ry+side)*math.sin(pol)*math.sin(az),
          center.z+(rz+.004)*math.cos(pol))
 bone('mechón natural %03d'%k,loc(p0),loc(p1),
      rng.uniform(.0023,.0040),hairL if k%5==0 else hair)
# Only shirt edges visible from the FRONT of torso (+Y): no white collar
# disk visible over the performer's neck from a rear camera view.
for side in (-1,1):
 bone('solapa del traje '+str(side),
      (-.48+side*.16,-1.062,1.87),
      (-.48+side*.052,-1.015,1.63),.021,suit)
 bone('borde camisa '+str(side),
      (-.48+side*.095,-1.025,1.835),
      (-.48+side*.015,-1.000,1.664),.015,shirt)
for z in [1.45,1.30]:
 ball('botón chaqueta',(-.48,-1.09,z),(.012,.014,.012),button,12,8)
# Minimal cloth details, kept near torso, no fantasy or stylized anatomy.
bone('costura posterior traje',(-.48,-1.686,1.64),
     (-.48,-1.675,1.26),.0035,seam)
bpy.ops.wm.save_as_mainfile(filepath=str(path))
print('HUMAN_MODEL_IMPROVEMENT_SAVED',len([o for o in sc.objects if o.name.startswith('REALISMO |')]),flush=True)
sc.frame_set(1)
sc.render.engine='CYCLES';sc.cycles.samples=22
sc.render.resolution_x=960;sc.render.resolution_y=540
sc.render.filepath=str(path.parent/'pianista_human_model_preview.png')
bpy.ops.render.render(write_still=True,scene=sc.name)
print('HUMAN_MODEL_PREVIEW_RENDERED',flush=True)
