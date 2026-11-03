from pathlib import Path
import struct,sys
p=Path(sys.argv[1])
r=(p/'src/LiveStage3D.cpp').read_text()
b=(p/'src/KeyboardDisplay.cpp').read_text()
assert 'TailoredSleeve(shoulder,elbow,wrist' in r
assert 'AnatomicalFinger(base,knuckle,middle,tip' in r
f=r.split('void DrawLiveFingers(',1)[1].split('bool LoadMeshFile(',1)[0]
assert 'Ball3D(elbow,' not in f
assert 'Bone3D(shoulder,elbow,' not in f
assert 'glEnable(GL_LIGHT1)' in r and 'fillDiffuse' in r
assert 'if(stage<6)renderer.DrawStageLights' in b
assert 'm_visual.sceneCameraFocus' in b
assert 'm_visual.sceneCameraMotion' in b
blend=p/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend'
assert blend.stat().st_size>3_500_000 and blend.read_bytes()[:32].find(b'BLENDER')>=0
src=(p/'assets/scenarios/blender/cuello_sastre_profesional.py').read_text()
assert 'HUMANO CC0 | cuello anatómico' in src
stage=(p/'assets/scenarios/blender/acabado_profesional_escenario.py').read_text()
assert 'Terciopelo acústico' in stage
mesh=(p/'graphics/scenarios/mesh_pianista_00.l3d').read_bytes()
assert mesh[:5]==b'L3DM\x03'
tris,=struct.unpack_from('<I',mesh,8)
assert tris>26000
print('OK: traje continuo, articulaciones sin esferas, cine 3D iluminado y cuello anatómico')
