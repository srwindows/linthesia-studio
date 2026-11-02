from pathlib import Path
import sys
r=Path(sys.argv[1])
hand=(r/'src/PianoHands.h').read_text()
kbd=(r/'src/KeyboardDisplay.cpp').read_text()
play=(r/'src/PlayingState.cpp').read_text()
mesh=(r/'src/LiveStage3D.cpp').read_text()
assert 'inline int SlotForFinger(int hand,int finger)' in hand
assert 'if(guided && used>0)' in hand
assert 'assigned[slot]=note;chosen[note]=true;' in hand
assert 'const Guidance& guidance=Guidance()' in hand
assert 'handGuidance.hands[key]=m_hand_classifier.ForNote(note)' in play
assert 'handGuidance.fingers[key]=finger' in play
assert 'const int finger=m_hand_classifier.Finger(note.track_id,key,note.start)' in play
assert 'const PianoHands::Guidance& handGuidance' in kbd
assert 'm_live_hand_animator.Update(held,handDelta,m_visual.scenePianistMotion,' in kbd
assert 'handGuidance);' in kbd
assert 'stage==6?&m_live_hand_animator.Current():nullptr' in kbd
assert 'DrawRiggedHand(frame,hand,wrist)' in mesh
print('OK: score hand and digit -> frame held MIDI -> 16 bone Blender hand mesh')
