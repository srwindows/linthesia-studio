from pathlib import Path
import math,sys,struct
r=Path(sys.argv[1]);assets=r/'graphics/scenarios'
# Validate only retained stage 6 and its 11 mesh poses:
# static 88 keys must NOT be duplicated inside the scene geometry.
assert not list(assets.glob("mesh_escena_*.l3d"))
for scene in range(11):
 path=assets/(f"mesh_pianista_{scene:02d}.l3d")
 b=path.read_bytes();assert b[:5]==b'L3DM\x03'
 triangles,=struct.unpack_from('<I',b,8)
 assert triangles>1000,(path,triangles)
 static_white=static_black=0
 for k in range(triangles*3):
  x,y,z,r_,g,bl,nx,ny,nz,u,v,flags=struct.unpack_from(
            '<fffBBBfffffB',b,12+k*36)
  assert all(math.isfinite(q) for q in (x,y,z))
  # Test only upper surface of the EXACT historic baked key boxes.
  if -.60<y<-.18 and .920<z<1.05:
   if 180<=r_<=245 and 180<=g<=235 and 175<=bl<=235:
    static_white+=1
   if r_<30 and g<30 and bl<45:static_black+=1
 assert static_white<250,(scene,'embedded static white keys',static_white)
 # Black body material is not an indication of duplicate ebony keys.
 print('STAGE',scene,'tris',triangles,'white key vertices',static_white)
# Regression: scenery is a BACKGROUND, never the interactive piano-roll.
# All six remaining scenes (6..11) use the same real lower keyboard.
display=(r/'src/KeyboardDisplay.cpp').read_text()
backend=(r/'src/LiveStage3D.cpp').read_text()
header=(r/'src/Renderer.h').read_text()
assert 'm_live_hand_animator.Update(held,handDelta' in display
assert 'stage==6?&m_live_hand_animator.Current():nullptr,' in display
assert 'DrawLiveKeys(*liveHands);' in backend
assert 'DrawLiveFingers(*liveHands);' in backend
assert 'glDisable(GL_CULL_FACE);' in backend
assert 'DrawStageFallingNotes' not in backend
assert 'StageFallingNote' not in header
assert 'fallingNotes' not in backend
assert 'worldNotes' not in display
assert 'pianistStageNotesRendered' not in display
assert 'const int stage=StageScenes::ClampPreset(m_visual.scenePreset);' in display
assert 'renderer.DrawBlenderScene3D(' in display
assert display.count('  DrawNotePass(renderer,')==1
assert display.count('DrawNotePass(renderer,')==2
assert 'if(m_visual.shadows)DrawNotePass(' in display
assert 'if(perspective3d)renderer.BeginBlenderNotes3D();' in display
assert 'if(perspective3d)renderer.EndBlenderNotes3D();' in display
assert 'if(m_render_3d){\n    if(m_cinematic_3d)\n      renderer.DrawPianoContactLightBar(' in display
assert 'renderer.DrawPianoImpactParticles(impact.x,impact.y,impact.width,' in display
assert 'renderer.DrawPianoKey3D(key_x,y_offset,key_width,key_height,' in display
assert 'renderer.DrawPianoKey3D(start_x,y_offset,black_width,black_height,' in display
assert 'm_frame_active_keys[name]=handColor;' in display
# The note's strike pixel must be computed from the same bottom keyboard Y.
assert 'ShortNoteVisual::Absorb(y+y_offset,limit,' in display
assert 'if(height<=0)continue;' in display
assert 'const int y_end   = y - static_cast<int>(adjusted_start * scaling_factor) + y_offset;' in display
assert 'const int start_x = (octave_offset + inner_octave_offset + keyboard_type_offset)' in display
assert 'const int key_x = i * (key_width + key_space) + x_offset;' in display
print('OK: six scenes independent, interactive piano only below, notes use bottom-keyboard X/Y')

# Seven-visible-stage regression: 1..7 in the UI; stable source 6..12.
scenes=(r/'src/StageScenes.h').read_text()
ui=(r/'src/SettingsPanel.cpp').read_text()
assert 'constexpr int Count=13;' in scenes
assert '6 · Dúo vocal' in scenes and '7 · Banda completa' in scenes
assert 'for(int i=6;i<StageScenes::Count;++i)' in ui
assert 'const int next=cycle(current,StageScenes::Count-5,direction);' in ui
assert 'scene==12?57+' in backend and 'scene==11?49+' in backend
assert 'glDisable(GL_CULL_FACE);' in backend
print('Seven user-visible scenes 1..7, stable binary routing and keyboard separation')
