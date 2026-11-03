# -*- coding: utf-8 -*-
"""Modelos originales Blender para teclado y notas Linthesia (GPL-2.0-or-later).
Malla real con biseles, normales por vértice y fuente .blend reutilizable.
Licencia del proyecto de Nicholas Piegdon y adaptación Oscar Aceña preservadas.
"""
import bpy, math
from pathlib import Path
from mathutils import Vector
source=Path("/home/javi/linthesia")
output=source/"assets"/"blender3d"
output.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
models=[]
def make(name,dimensions,location,bev,segments,material,roughness):
    bpy.ops.mesh.primitive_cube_add(size=1,location=location)
    ob=bpy.context.object;ob.name=name
    ob.dimensions=dimensions
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    modifier=ob.modifiers.new("Radio ergonómico | bisel real", "BEVEL")
    modifier.width=bev;modifier.segments=segments
    modifier.affect="EDGES"
    bpy.context.view_layer.objects.active=ob
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    # smooth curved edges while preserving planar corners via angle normals
    for polygon in ob.data.polygons:polygon.use_smooth=True
    mat=bpy.data.materials.new(material)
    mat.diffuse_color=(*roughness[:3],1)
    mat.use_nodes=True
    bsdf=mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value=(*roughness[:3],1)
        bsdf.inputs["Roughness"].default_value=roughness[3]
        bsdf.inputs["Metallic"].default_value=0.04
        bsdf.inputs["Coat Weight"].default_value=0.42
        bsdf.inputs["Coat Roughness"].default_value=0.14
    ob.data.materials.append(mat)
    models.append(ob)
    return ob
white=make("Tecla blanca 3D | cuerpo marfil",(.92,.98,.14),(0,.49,.07),.022,10,"Esmalte marfil",(.94,.95,.96,.22))
black=make("Tecla negra 3D | grafito pulido",(.62,.64,.23),(0,.32,.19),.032,12,"Grafito satinado",(.045,.055,.07,.27))
note=make("Nota 3D | cápsula LED",(.76,.97,.14),(0,.50,.12),.069,18,"Polímero translúcido | RGB",(.08,.75,.85,.16))
# One actual piano model in the .blend: 52 whites, 36 blacks
collection=bpy.data.collections.new("Piano completo 88 teclas | referencia")
bpy.context.scene.collection.children.link(collection)
layout=['A','A#','B','C','C#','D','D#','E','F','F#','G','G#']
whiteIndex=-1
for midi in range(21,109):
    name=layout[(midi-21)%12]
    black_key="#" in name
    if not black_key:whiteIndex+=1
    template=black if black_key else white
    # Piano real: 52 blancas adyacentes y 36 negras entre ellas.
    # Instancias enlazadas de los modelos originales para facilitar edición.
    key=template.copy(); key.data=template.data
    key.name=f"Piano-88 | MIDI {midi:03d} | {name}"
    unit=(whiteIndex+(0.5 if black_key else 0.0)-25.5)*1.03
    key.location=(unit,0,0)
    collection.objects.link(key)
# Hide collection in Blender's viewport by default; templates visible and editable.
# Mostrar el teclado completo al abrir Blender, no solamente las plantillas.
collection.hide_viewport=False
for template in (white,black,note):
    template.hide_render=True
    template.hide_set(True)
# Dos teclas con animación de pulsación para comprobar el recorrido mecánico.
for midi in (60,64):
    obj=next(o for o in collection.objects if f"MIDI {midi:03d}" in o.name)
    resting=obj.location.z
    for frame,offset in ((1,0.0),(5,-0.055),(11,-0.055),(17,0.0)):
        obj.location.z=resting+offset
        obj.keyframe_insert(data_path="location",frame=frame,index=2)
    obj.location.z=resting
bpy.context.scene.frame_set(1)
# keep coherent camera and lights for inspection in native Blender
bpy.ops.object.camera_add(location=(22,-31,32))
cam=bpy.context.object;cam.name="Cámara inspección materiales"
direction=Vector((0,.5,.1))-cam.location
cam.rotation_euler=direction.to_track_quat('-Z','Y').to_euler()
cam.data.type="ORTHO";cam.data.ortho_scale=64
bpy.context.scene.camera=cam
bpy.ops.object.light_add(type="AREA",location=(0,-5,17))
light=bpy.context.object;light.name="Luz suave | 800 W";light.data.energy=3500;light.data.shape="DISK";light.data.size=30
bpy.context.scene.render.engine="CYCLES"
bpy.context.scene.cycles.samples=24
bpy.context.scene.render.resolution_x=1200
bpy.context.scene.render.resolution_y=740
bpy.context.scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(output/"Linthesia_Modelos_3D.blend"))
# Export each evaluated mesh from Blender (not procedural approximation at runtime).
header=source/"src"/"BlenderMeshes.h"
with header.open("w") as out:
    out.write('// Modelos exportados desde assets/blender3d/Linthesia_Modelos_3D.blend\n')
    out.write('// Autoría Linthesia y licencia GPLv2 conservadas. Generar con Blender 4.3.\n')
    out.write('#pragma once\n#include <cstddef>\nnamespace BlenderMesh {\n')
    out.write('struct Vertex { float x,y,z,nx,ny,nz; };\n')
    for key,obj in (("White",white),("Black",black),("Note",note)):
        mesh=obj.data
        mesh.calc_loop_triangles()
        out.write('static const Vertex '+key+'[] = {\n')
        rows=0
        for tri in mesh.loop_triangles:
            for loop_id in tri.loops:
                loop=mesh.loops[loop_id]
                v=mesh.vertices[loop.vertex_index]
                normal=mesh.corner_normals[loop_id].vector
                xyz=v.co+obj.location
                out.write('  {%0.6ff,%0.6ff,%0.6ff,%0.6ff,%0.6ff,%0.6ff},\n'%(
                    xyz.x,xyz.y,xyz.z,normal.x,normal.y,normal.z))
                rows+=1
        out.write('};\nstatic constexpr size_t '+key+'Count='+str(rows)+';\n')
        print(key,rows,'tri-vertices',rows//3,'triangles')
    out.write('}\n')
print('BLENDER_3D_ASSETS_SAVED',output)
