from pathlib import Path
import sys,struct
p=Path(sys.argv[1])
src=(p/'src')
renderer=(src/'LiveStage3D.cpp').read_text()
ui=(src/'PlayingState.cpp').read_text()
mouse=(src/'main.cpp').read_text()
renderloop=(src/'KeyboardDisplay.cpp').read_text()
prefs=(src/'AdvancedSettings.cpp').read_text()
assert 'glFrustum(' in renderer and 'glCallList(' in renderer
assert 'glPushMatrix()' in renderer and 'glViewport(' in renderer
assert 'glClear(GL_DEPTH_BUFFER_BIT)' in renderer
assert 'glEnable(GL_LIGHTING)' in renderer
assert 'glLightfv(GL_LIGHT0,GL_DIFFUSE' in renderer
assert 'DrawBlenderScene3D' in renderloop
assert 'SDL_MOUSEWHEEL' in mouse and 'MouseWheel' in mouse
assert 'm_orbit_dragging' in ui and 'mouse.held.right' in ui
assert 'm_midi_preferences.sceneYaw+dx/2' in ui
assert 'm_midi_preferences.sceneZoom-mouse.wheelY*35' in ui
assert 'StageCamera::Preset(p.sceneView)' in ui
for key in ['camara-giro','camara-elevacion','camara-distancia','camara-vista']:
 assert key in prefs
root=p/'graphics/scenarios'
paths=[root/('mesh_pianista_%02d.l3d'%i) for i in range(11)]
assert not list(root.glob('mesh_escena_*.l3d'))
for path in paths:
 data=path.read_bytes()
 assert data[:4]==b'L3DM' and data[4] in (1,2,3),path
 tri,=struct.unpack_from('<I',data,8)
 vertex_size={1:15,2:27,3:36}[data[4]]
 # Other scenes no longer bake 88 immobile keys (previously ~9k
 # triangles), so expecting >7000 would reject the correct dynamic model.
 assert tri>2000 and len(data)==12+tri*vertex_size*3,(path,tri,len(data))
 assert b'\x00'*45!=data[12:57]
assert (p/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend').is_file()
model=(p/'assets/scenarios/blender/grand_3d_export.py').read_text()
assert 'white==52 and black==36' in model
assert 'range(21,109)' in model
assert 'me.corner_normals' in model
assert 'glNormal3f(smoothNormal.x' in renderer
print('OK: sole pianist stage, 11 animation meshes, orbit and zoom, camera settings')
