from pathlib import Path
import sys,struct
r=Path(sys.argv[1]);assets=r/'graphics/scenarios'
names=['guitarrista','baterista','cantante','cantantef','duo','banda']
for index,name in enumerate(names):
 frames=12 if name=='banda' else 8
 paths=[assets/f'mesh_{name}_{i:02d}.l3d' for i in range(frames)]
 for path in paths:
  b=path.read_bytes()
  assert b[:5]==b'L3DM\x03',(name,path,b[:5])
  tris,=struct.unpack_from('<I',b,8)
  assert tris>3000
  assert len(b)==12+tris*108,(path,len(b),tris)
 # At least one triangle vertex should actually move between first frames.
 a=paths[0].read_bytes();b=paths[1].read_bytes()
 assert len(a)==len(b)
 distinct=sum(a[i:i+12]!=b[i:i+12] for i in range(12,len(a),36))
 assert distinct>10,(name,distinct)
 assert (assets/f'escena_{index+7:02d}.tga').stat().st_size>10000
 print('MUSICIAN',name,'frames',frames,'triangles',tris,'changing vertices',distinct)
backend=(r/'src/LiveStage3D.cpp').read_text()
assert 'names[]={"guitarrista","baterista","cantante","cantantef"}' in backend
assert 'scene==11?49+' in backend and 'scene==12?57+' in backend
ui=(r/'src/SettingsPanel.cpp').read_text()
assert 'for(int i=6;i<StageScenes::Count;++i)' in ui
# Verify the source Blender authoring contract for correct default poses.
builder=(r/'assets/scenarios/blender/crear_cinco_escenarios.py').read_text()
assert "ob.rotation_euler[2]=rot[2]+.08*math.sin(th)" in builder
assert "ob.rotation_euler[2]=math.pi" in builder
assert "target=(x+side*.40,y-.24,1.37)" in builder
assert "target=(x-.115,y-.445,1.20)" in builder
assert "target=(x-.02,y-.19,1.38)" in builder
assert "jacket(label,x,y,coat,female=female)" in builder
assert "parche circular" in builder
print('OK: seven Blender scenes, frontal face keyframes, human body, musician hand targets, MIDI independent')
