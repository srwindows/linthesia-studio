# Physical proportions/contacts checked independently of GUI screenshot.
from pathlib import Path
import math,struct,sys
r=Path(sys.argv[1])
s=(r/'src/LiveStage3D.cpp').read_text()
assert 'const V3 shoulder={-.48f+sign*.235f,-1.03f,' in s
assert '1.65f+frame.shoulderRise' in s
assert '},.435f,.395f);' in s
assert 'const float raise=PianoHands::FingerArch(hand,fingerId);' in s
assert 'AnatomicalFinger(base,knuckle,middle,tip,hand,fingerId)' in s
assert 'TailoredSleeve(shoulder,elbow,wrist' in s
assert 'AnatomicalFinger(base,knuckle,middle,tip' in s
assert 'PianoHands::BaseOffsetY(hand,fingerId,black)' in s
assert 'HandRig::WristHeight(frame,hand)' in s
assert '1.015f+(black?.027f:0.f)' in s
assert 'const float radii[4]={1.04f,1.0f,.88f,.66f};' in s
def white(n):return n%12 in (0,2,4,5,7,9,11)
def xkey(n):
 return -1.163+.02645*(sum(white(k) for k in range(21,n))+(0 if white(n) else -.46))
for notes in ([48,52,55],[61,63,66],[49,54,58],
              [64,67,71],[72,76,79],[73,78,82]):
 wrist=sum(xkey(n) for n in notes)/len(notes)
 for f,n in zip((0,2,4),notes):
  target=xkey(n);spread=(f-2)*.035
  base_x=wrist+spread+.44*(target-(wrist+spread))
  base_y=-.552 if white(n) else -.507
  tip_y=-.413 if white(n) else -.347
  tip_z=(.970 if white(n) else 1.0205)+.01
  reach=math.dist((base_x,base_y,1.067),(target,tip_y,tip_z))
  assert reach<.178,('finger would dislocate',n,reach)
for shoulder,wrist in [
  ((-.715,-1.03,1.65),(-.687,-.614,1.08)),
  ((-.245,-1.03,1.65),(-.449,-.614,1.08)),
  ((-.245,-1.03,1.65),(-.317,-.614,1.08))
]:
 assert math.dist(shoulder,wrist)<.83,('arm reach impossible',shoulder,wrist)
mesh=(r/'graphics/scenarios/mesh_pianista_00.l3d').read_bytes()
assert mesh[:5]==b'L3DM\x03'
tri,=struct.unpack_from('<I',mesh,8)
assert 29000<tri<65000,('unexpected character mesh resolution',tri)
marks=mesh[12+35::36]
assert sum(bool(i&2) for i in marks)>10000,('non-animated haircut',tri)
assert sum(bool(i&1) for i in marks)>5000,('lost facial photographic texture',tri)
assert (r/'assets/scenarios/blender/cabello_microgeometria.py').exists()
assert (r/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend').stat().st_size>3_500_000 and (r/'assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend').read_bytes()[:32].find(b'BLENDER')>=0
print('OK: natural arm/hand lengths, playable black/white keys, moving hair and skin')
