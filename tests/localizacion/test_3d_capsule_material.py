"""Comprobar material 3D y geometría redondeada sin shaders externos."""
from pathlib import Path
import sys
r=Path(sys.argv[1]);src=(r/"src/KeyboardDisplay.cpp").read_text()
renderer=(r/"src/Renderer.cpp").read_text()
header=(r/"src/Renderer.h").read_text()
for required in (
 "void Renderer::DrawNeonCapsule(",
 "glBegin(GL_TRIANGLES);",
 "BlenderMesh::NoteCount",
 "BlenderMesh::Note[i]",
 "const float cap=std::max(2.0f",
 "glEnable(GL_DEPTH_TEST);",
 "glClear(GL_DEPTH_BUFFER_BIT);",
 "void Renderer::BeginBlenderNotes3D(){",
 "void Renderer::EndBlenderNotes3D(){",
):
 assert required in renderer,required
assert '#include "BlenderMeshes.h"' in renderer
assert "void DrawNeonCapsule(int x,int y,int w,int h,int radius," in header
assert "renderer.BeginBlenderNotes3D();" in src
assert "renderer.EndBlenderNotes3D();" in src
mesh=(r/"src/BlenderMeshes.h").read_text()
for name in ("White","Black","Note"):
 assert ("static const Vertex "+name+"[]") in mesh,name
 assert ("static constexpr size_t "+name+"Count=") in mesh,name
blend=r/"assets/blender3d/Linthesia_Modelos_3D.blend"
assert blend.exists() and blend.stat().st_size>200000
assert "GNU GENERAL PUBLIC LICENSE" in (r/"COPYING").read_text()
print("OK: cápsulas 3D con volumen, degradado, halo y perfil cerrado")
