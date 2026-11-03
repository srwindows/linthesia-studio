from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
# Upgrade painterly materials and clothes while retaining MakeHuman CC0 head/UV.
from pathlib import Path
import bpy,random,math
p=Path(f'{_LINTHESIA_ROOT}/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend')
bpy.ops.wm.open_mainfile(filepath=str(p))
sc=bpy.data.scenes['Pianista animado'];bpy.context.window.scene=sc
def mat(name,color,rough=.83):
 m=bpy.data.materials.get(name)
 assert m,name
 m.diffuse_color=(*color,1)
 m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF')
 if bs:
  bs.inputs['Base Color'].default_value=(*color,1)
  bs.inputs['Roughness'].default_value=rough
  bs.inputs['Metallic'].default_value=.015
 return m
suit=mat('Traje azul grafito',(.085,.097,.12),.91)
mat('Traje azul oscuro de paño',(.072,.079,.099),.91)
mat('Pantalón oscuro',(.053,.059,.07),.87)
mat('Camisa gris perla',(.70,.73,.74),.92)
# Subtle jacket seam geometry to communicate tailoring and cloth thickness.
for ob in list(sc.objects):
 if ob.name.startswith('ACABADO HUMANO |'):
  bpy.data.objects.remove(ob,do_unlink=True)
seam=bpy.data.materials.get('Costura sastre azul')
if not seam:seam=suit
def seamline(name,pts,r=.0025,matref=seam):
 curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D'
 curve.bevel_depth=r;curve.bevel_resolution=2;curve.resolution_u=12
 spline=curve.splines.new('BEZIER')
 spline.bezier_points.add(len(pts)-1)
 for bp,co in zip(spline.bezier_points,pts):
  bp.co=co;bp.handle_left_type='AUTO';bp.handle_right_type='AUTO'
 ob=bpy.data.objects.new('ACABADO HUMANO | '+name,curve)
 sc.collection.objects.link(ob)
 curve.materials.append(matref)
 return ob
seamline('costura espalda centro',
         [(-.48,-1.479,1.80),(-.48,-1.45,1.63),
          (-.48,-1.385,1.34),(-.48,-1.34,1.16)],.0018)
# Remove the previous fixed shirt spikes if any (redundant sleeve cuffs remain procedural).
for ob in list(sc.objects):
 if ob.name.startswith('REALISMO | solapa del traje'):
  bpy.data.objects.remove(ob,do_unlink=True)
# Suit hem seams follow the torso front lower centre, not floating circular nodes.
for side in (-1,1):
 seamline('solapa entallada %s'%side,
          [(-.48+side*.045,-.967,1.82),
           (-.48+side*.136,-.919,1.67),
           (-.48+side*.08,-.920,1.50)],.0023)
# A few matte dark buttons; real button diameter is around 15 mm.
button=bpy.data.materials.get('Botones traje oscuros') or suit
for i,z in enumerate((1.42,1.34,1.26)):
 bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=.0072,depth=.004,
      location=(-.48,-.908,z),rotation=(math.pi/2,0,0))
 ob=bpy.context.object;ob.name='ACABADO HUMANO | botón %d'%i
 ob.data.materials.append(button)
 for poly in ob.data.polygons:poly.use_smooth=True
# Metal microphone-like artifacts must not be introduced into pianist's props.
bpy.ops.wm.save_as_mainfile(filepath=str(p))
print('ACABADO_MATERIALES_SASTRERIA',len([o for o in sc.objects if o.name.startswith('ACABADO HUMANO |')]))
