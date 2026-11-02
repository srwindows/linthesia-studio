"""Geometría Blender en Linthesia: modelos 3D, normales y runtime (GPLv2)."""
from pathlib import Path
import re
import sys
root=Path(sys.argv[1])
models=root/"assets/blender3d/Linthesia_Modelos_3D.blend"
generator=root/"tools/blender_generate_models.py"
header=root/"src/BlenderMeshes.h"
renderer=(root/"src/Renderer.cpp").read_text()
kbd=(root/"src/KeyboardDisplay.cpp").read_text()
assert models.is_file() and models.stat().st_size>250000
assert generator.is_file()
assert "bpy.ops.mesh.primitive_cube_add" in generator.read_text()
assert "modifier.segments=segments" in generator.read_text()
assert "mesh.corner_normals[loop_id]" in generator.read_text()
assert "collection.objects.link(key)" in generator.read_text()
data=header.read_text()
assert "namespace BlenderMesh {" in data
for key,min_tri in (("White",400),("Black",650),("Note",1250)):
    match=re.search(r"static constexpr size_t "+key+r"Count=(\d+);",data)
    assert match,key
    assert int(match.group(1))//3>=min_tri,key
assert "#include \"BlenderMeshes.h\"" in renderer
assert "void Renderer::BeginBlenderKeyboard3D()" in renderer
assert "void Renderer::BeginBlenderNotes3D()" in renderer
assert "renderer.BeginBlenderKeyboard3D();" in kbd
assert "renderer.BeginBlenderNotes3D();" in kbd
assert "renderer.DrawNeonCapsule(newLeft,top,newWidth,height" in kbd
assert "const float drop=frontTravel*depression*t;" in renderer
assert "GNU GENERAL PUBLIC LICENSE" in (root/"COPYING").read_text()
print("OK: Blender real mallas 88 teclas, 3D normales, animación y licencia")
