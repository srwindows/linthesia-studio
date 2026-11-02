// Original GPL-2.0-or-later Linthesia extension.
// True OpenGL perspective renderer for Blender-authored 3D meshes.
#include "Renderer.h"
#include "StageScenes.h"
#include "PianoKinematics.h"
#include "HandRigKinematics.h"
#include "CinematicCamera.h"
#include "UiAppearance.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct V3 {
 float x,y,z;
 V3 operator-(V3 o)const{return {x-o.x,y-o.y,z-o.z};}
 V3 operator+(V3 o)const{return {x+o.x,y+o.y,z+o.z};}
 V3 operator*(float f)const{return {x*f,y*f,z*f};}
};
V3 cross(V3 a,V3 b){
 return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
float dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V3 unit(V3 v){
 const float n=std::sqrt(dot(v,v));
 if(n<.00001f)return {0,0,1};
 return v*(1.f/n);
}
// Renderer is re-created each frame. The compiled GPU geometry must persist.
GLuint g_stage_lists[80]={};
GLuint g_head_lists[80]={};
GLuint g_palm_lists[2]={};
bool g_palm_failed[2]={};
// CPU-skinned vertices are kept once per GL context; generated from the
// armature-authored continuous human hand mesh, not independent finger tubes.
struct RigVertex {
 HandRig::V pos,normal;
 float u=0,v=0;
 unsigned char bones[3]={0,0,0};
 float weight[3]={1,0,0};
};
std::vector<RigVertex> g_hand_mesh[2];
bool g_hand_failed[2]={};
bool g_hand_ready[2]={};
bool g_stage_failed[80]={};
Tga* g_skin_texture=nullptr;
bool g_skin_failed=false;
struct Vertex {
 V3 p;
 unsigned char r,g,b;
 V3 n;
 float u=0.f,v=0.f;
 unsigned char textured=0;
};
using Triangle=std::array<Vertex,3>;
// The following rig is drawn in exactly the same model/view/projection and
// z-buffer as the piano. Each articulation responds to a specific MIDI pitch,
// without additional baked images or pseudo-animation.
void SetMat(unsigned char r,unsigned char g,unsigned char b){
 glColor3ub(r,g,b);
}
void Box3D(V3 c,V3 size,unsigned char r,unsigned char g,unsigned char b, unsigned char alpha=255){
 const float x=c.x,y=c.y,z=c.z;
 const float a=size.x*.5f,d=size.y*.5f,e=size.z*.5f;
 const V3 p[8]={
  {x-a,y-d,z-e},{x+a,y-d,z-e},{x+a,y+d,z-e},{x-a,y+d,z-e},
  {x-a,y-d,z+e},{x+a,y-d,z+e},{x+a,y+d,z+e},{x-a,y+d,z+e}
 };
 const int f[6][4]={{0,3,2,1},{4,5,6,7},{0,1,5,4},
                    {1,2,6,5},{2,3,7,6},{3,0,4,7}};
 const V3 n[6]={{0,0,-1},{0,0,1},{0,-1,0},
                 {1,0,0},{0,1,0},{-1,0,0}};
 glColor4ub(r,g,b,alpha);
 glBegin(GL_QUADS);
 for(int face=0;face<6;++face){
  glNormal3f(n[face].x,n[face].y,n[face].z);
  for(int v=0;v<4;++v){
   const auto pt=p[f[face][v]];
   glVertex3f(pt.x,pt.y,pt.z);
  }
 }
 glEnd();
}
// A bevelled three-dimensional keycap, with distinct upper lacquer,
// rounded edge band and darker vertical walls.  Unlike the former flat
// cuboids, its edges and side walls reveal the physical key travel.
void KeyCap3D(V3 c,V3 size,bool ivory){
 const float a=size.x*.5f,b=size.y*.5f,h=size.z*.5f;
 const float edge=std::min(.0022f,size.x*.10f);
 const float xs[4]={-a,a,a,-a},ys[4]={-b,-b,b,b};
 const float xi[4]={-a+edge,a-edge,a-edge,-a+edge};
 const float yi[4]={-b+edge,-b+edge,b-edge,b-edge};
 const V3 normals[4]={{0,-1,0},{1,0,0},{0,1,0},{-1,0,0}};
 SetMat(ivory?228:25,ivory?224:27,ivory?211:32);
 glBegin(GL_QUADS);
 glNormal3f(0,0,1);
 for(int i=0;i<4;++i)glVertex3f(c.x+xi[i],c.y+yi[i],c.z+h);
 glEnd();
 for(int i=0;i<4;++i){
  const int j=(i+1)%4;
  const V3 side=normals[i];
  SetMat(ivory?209:16,ivory?205:18,ivory?194:22);
  glBegin(GL_QUADS);
  glNormal3f(side.x,side.y,side.z);
  glVertex3f(c.x+xs[i],c.y+ys[i],c.z-h);
  glVertex3f(c.x+xs[j],c.y+ys[j],c.z-h);
  glVertex3f(c.x+xs[j],c.y+ys[j],c.z+h-edge);
  glVertex3f(c.x+xs[i],c.y+ys[i],c.z+h-edge);
  glEnd();
  SetMat(ivory?223:23,ivory?219:25,ivory?206:29);
  glBegin(GL_QUADS);
  const V3 n=unit(V3{side.x,side.y,1.f});
  glNormal3f(n.x,n.y,n.z);
  glVertex3f(c.x+xs[i],c.y+ys[i],c.z+h-edge);
  glVertex3f(c.x+xs[j],c.y+ys[j],c.z+h-edge);
  glVertex3f(c.x+xi[j],c.y+yi[j],c.z+h);
  glVertex3f(c.x+xi[i],c.y+yi[i],c.z+h);
  glEnd();
 }
}
void Bone3D(V3 a,V3 b,float r0,float r1,
            unsigned char red,unsigned char green,unsigned char blue){
 const V3 axis=unit(b-a);
 const V3 other=std::fabs(axis.z)<.91f?V3{0,0,1}:V3{0,1,0};
 const V3 tangent=unit(cross(axis,other));
 const V3 bitangent=unit(cross(axis,tangent));
 const int n=12;
 const float pi=3.14159265f;
 SetMat(red,green,blue);
 glBegin(GL_QUADS);
 for(int i=0;i<n;++i){
  const float t0=2.f*pi*i/n,t1=2.f*pi*(i+1)/n;
  const V3 d0=tangent*std::cos(t0)+bitangent*std::sin(t0);
  const V3 d1=tangent*std::cos(t1)+bitangent*std::sin(t1);
  const V3 p0=a+d0*r0,p1=a+d1*r0,q0=b+d0*r1,q1=b+d1*r1;
  glNormal3f(d0.x,d0.y,d0.z);glVertex3f(p0.x,p0.y,p0.z);
  glNormal3f(d1.x,d1.y,d1.z);glVertex3f(p1.x,p1.y,p1.z);
  glNormal3f(d1.x,d1.y,d1.z);glVertex3f(q1.x,q1.y,q1.z);
  glNormal3f(d0.x,d0.y,d0.z);glVertex3f(q0.x,q0.y,q0.z);
 }
 glEnd();
}
void Ball3D(V3 centre,V3 radii,unsigned char r,unsigned char g,unsigned char b){
 const int rows=9,cols=12;
 const float pi=3.14159265f;
 SetMat(r,g,b);
 for(int row=0;row<rows;++row){
  const float p0=pi*row/rows,p1=pi*(row+1)/rows;
  glBegin(GL_QUAD_STRIP);
  for(int col=0;col<=cols;++col){
   const float az=2.f*pi*col/cols;
   for(float p : {p0,p1}){
    const V3 normal={std::sin(p)*std::cos(az),
                     std::sin(p)*std::sin(az),std::cos(p)};
    V3 pt={centre.x+normal.x*radii.x,
           centre.y+normal.y*radii.y,
           centre.z+normal.z*radii.z};
    glNormal3f(normal.x,normal.y,normal.z);
    glVertex3f(pt.x,pt.y,pt.z);
   }
  }
  glEnd();
 }
}
void DrawLiveKeys(const PianoHands::Frame& frame){
 for(int n=21;n<=108;++n){
  const bool white=PianoHands::White(n);
  const float press=frame.keys[n];
  const float x=PianoHands::KeyX(n);
  const V3 size=white?V3{.024f,.200f,.040f}:V3{.014f,.130f,.065f};
  const V3 origin=white?V3{x,-.420f,.950f}:V3{x,-.345f,.988f};
  // Real piano action: pivot each key about its rear end.  The front
  // travels down 14 mm (white) or 10 mm (black) for MIDI NoteOn.
  // The ivory/ebony material never changes colour when the key moves.
  const float rearY=origin.y+size.y*.5f;
  const float frontTravel=white?.014f:.010f;
  const float angle=std::atan2(frontTravel,size.y)*180.f/3.14159265f;
  glPushMatrix();
  glTranslatef(0.f,rearY,origin.z);
  glRotatef(angle*press,1.f,0.f,0.f);
  glTranslatef(0.f,-rearY,-origin.z);
  KeyCap3D(origin,size,white);
  glPopMatrix();
 }
}

// The 3D stage is visual accompaniment. It never draws the MIDI
// waterfall: that is owned exclusively by the lower playable keyboard.

// Smooth, continuously skinned sleeve / finger cross sections replace the
// exposed cylindrical "robot" bones and ball-like elbow joints.  The curve
// passes through shoulder, elbow and wrist, with a tangent-continuous elbow.
V3 SmoothChain(V3 a,V3 joint,V3 b,float t){
 const bool first=t<=.5f;
 const float u=first?t*2.f:(t-.5f)*2.f;
 const V3 p0=first?a:joint,p1=first?joint:b;
 const V3 tangent=(b-a)*.43f;
 const V3 m0=first?(joint-a)*.92f:tangent;
 const V3 m1=first?tangent:(b-joint)*.92f;
 const float u2=u*u,u3=u2*u;
 return p0*(2*u3-3*u2+1) + m0*(u3-2*u2+u)
      + p1*(-2*u3+3*u2) + m1*(u3-u2);
}
void TailoredSleeve(V3 shoulder,V3 elbow,V3 wrist,bool leftSide){
 constexpr float pi=3.14159265f;
 constexpr int sides=18,rings=24;
 // Anatomical taper: shoulder, bent elbow, forearm, then narrow shirt cuff.
 const float sign=leftSide?-1.f:1.f;
 auto ring=[&](float t){
  const float u=t<=.5f?t*2.f:(t-.5f)*2.f;
  float r=t<.5f?(.102f-.022f*u):(.080f-.025f*u);
  // Low-frequency textile folds, not luminous polygons/metal joints.
  r*=1.f+.016f*std::sin(24.f*t+sign)+.009f*std::sin(55.f*t);
  return r;
 };
 auto section=[&](float t, int j){
  const float nearT=std::max(0.f,t-.004f);
  const float farT=std::min(1.f,t+.004f);
  const V3 centre=SmoothChain(shoulder,elbow,wrist,t);
  const V3 tangent=unit(SmoothChain(shoulder,elbow,wrist,farT)
                       -SmoothChain(shoulder,elbow,wrist,nearT));
  const V3 guide={0,1,0};
  const V3 side=unit(cross(tangent,guide));
  const V3 front=unit(cross(side,tangent));
  const float radius=ring(t);
  // At each ring normals point OUT from the garment; surfaces interpolate
  // smoothly over the elbow while retaining a recognisable human silhouette.
  {
   const float phi=(j% sides)*2*pi/sides;
   const float c=std::cos(phi),ss=std::sin(phi);
   const float oval=.84f;
   V3 radial=unit(side*c+front*(ss/oval));
   V3 pos=centre+side*(c*radius)+front*(ss*radius*oval);
   glNormal3f(radial.x,radial.y,radial.z);
   glVertex3f(pos.x,pos.y,pos.z);
  }
 };
 // Draw connected tube with smooth shading, not individual bone cylinders.
 glColor3ub(40,45,56);
 for(int i=0;i<rings;++i){
  glBegin(GL_QUAD_STRIP);
  for(int j=0;j<=sides;++j){
   section(float(i)/rings,j);
   section(float(i+1)/rings,j);
  }
  glEnd();
 }
 // Linen cuff tapers naturally into the top of the palm.
 glColor3ub(202,207,204);
 const V3 cuffStart=SmoothChain(shoulder,elbow,wrist,.958f);
 Bone3D(cuffStart,wrist,.056f,.051f,202,207,204);
}
// Anatomical digit: ONE continuous smooth surface instead of three tubes
// stitched at sharp angles. This stops the "claw / spider" silhouette.
V3 DigitSpline(const V3 (&points)[4],float t){
 const float position=std::max(0.f,std::min(1.f,t))*3.f;
 const int section=std::min(2,static_cast<int>(position));
 const float u=position-section,u2=u*u,u3=u2*u;
 const V3 a=points[section];
 const V3 b=points[section+1];
 const V3 before=points[std::max(0,section-1)];
 const V3 after=points[std::min(3,section+2)];
 const V3 v0=(b-before)*.5f;
 const V3 v1=(after-a)*.5f;
 return a*(2*u3-3*u2+1)+v0*(u3-2*u2+u)
      +b*(-2*u3+3*u2)+v1*(u3-u2);
}
void AnatomicalFinger(V3 base,V3 knuckle,V3 middle,V3 tip,
                      int hand,int slot){
 const bool thumb=PianoHands::IsThumb(hand,slot);
 const float thickness=PianoHands::FingerWidth(hand,slot);
 // The yema is rounded, not a tapered claw. Retain almost 80% of
 // the radius at the fingertip and use a slightly flattened ellipse.
 const float radii[4]={1.04f,1.0f,.88f,.66f};
 const V3 points[4]={base,knuckle,middle,tip};
 constexpr int around=14,along=24;
 constexpr float pi=3.14159265359f;
 const bool useSkin=g_skin_texture!=nullptr;
 if(useSkin){
  glBindTexture(GL_TEXTURE_2D,g_skin_texture->GetId());
  glEnable(GL_TEXTURE_2D);
  glColor3ub(244,244,244);
 }else glColor3ub(188,131,107);
 for(int i=0;i<along;++i){
  glBegin(GL_QUAD_STRIP);
  for(int j=0;j<=around;++j){
   const float angle=2.f*pi*(j%around)/around;
   for(int k=0;k<2;++k){
    const float t=float(i+k)/along;
    const float pos=t*3.f;
    const int segment=std::min(2,static_cast<int>(pos));
    const float alpha=pos-segment;
    // Very subtle knuckle knurls without joint spheres or sharp seams.
    const float rad=thickness*
      (radii[segment]*(1.f-alpha)+radii[segment+1]*alpha);
    const V3 centre=DigitSpline(points,t);
    const V3 before=DigitSpline(points,std::max(0.f,t-.004f));
    const V3 after=DigitSpline(points,std::min(1.f,t+.004f));
    const V3 axis=unit(after-before);
    const V3 side=unit(cross(axis,V3{0.f,0.f,1.f}));
    const V3 upward=unit(cross(side,axis));
    const float co=std::cos(angle),si=std::sin(angle);
    const V3 normal=unit(side*co+upward*(si/.85f));
    const V3 vertex=centre+side*(co*rad)+upward*(si*rad*.85f);
    // Sample an unobstructed CC0 skin region in the same MakeHuman
    // atlas as the palm, avoiding flat plastic-looking finger colours.
    if(useSkin)glTexCoord2f(.674f+.061f*t,
                             .137f+.022f*std::cos(angle));
    glNormal3f(normal.x,normal.y,normal.z);
    glVertex3f(vertex.x,vertex.y,vertex.z);
   }
  }
  glEnd();
 }
 // Anatomical rounded finger pad: close the distal surface with a
 // continuous hemisphere instead of an exposed square-ended tube.
 // Normals and UV coordinates match the finger's final ring exactly.
 const V3 distal=unit(tip-middle);
 const V3 side=unit(cross(distal,V3{0.f,0.f,1.f}));
 const V3 top=unit(cross(side,distal));
 constexpr int capRings=6;
 const float padRadius=thickness*radii[3];
 for(int ring=0;ring<capRings;++ring){
  glBegin(GL_QUAD_STRIP);
  for(int seg=0;seg<=around;++seg){
   const float phi=2.f*pi*(seg%around)/around;
   for(int row:{ring,ring+1}){
    const float a=.5f*pi*float(row)/capRings;
    const float c=std::cos(a),sn=std::sin(a);
    const float co=std::cos(phi),si=std::sin(phi);
    const V3 radial=side*co+top*(si*.85f);
    const V3 normal=unit(radial*c+distal*sn);
    const V3 pos=tip+distal*(padRadius*sn)+
                  radial*(padRadius*c);
    if(useSkin)glTexCoord2f(.735f+.015f*sn,
                            .137f+.022f*co);
    glNormal3f(normal.x,normal.y,normal.z);
    glVertex3f(pos.x,pos.y,pos.z);
   }
  }
  glEnd();
 }
 if(useSkin)glDisable(GL_TEXTURE_2D);
}

bool LoadRiggedHand(int hand){
 if(hand<0 || hand>1)return false;
 if(g_hand_ready[hand])return true;
 if(g_hand_failed[hand])return false;
 const std::string path=std::string(GRAPHDIR)+"/scenarios/"+
       (hand==0?"mesh_mano_rig_00.lhr":"mesh_mano_rig_01.lhr");
 std::ifstream f(path,std::ios::binary);
 if(!f){g_hand_failed[hand]=true;return false;}
 char magic[4]={};
 std::uint32_t triangles=0,bones=0;
 f.read(magic,4);
 f.read(reinterpret_cast<char*>(&triangles),4);
 f.read(reinterpret_cast<char*>(&bones),4);
 if(!f || std::memcmp(magic,"LHR1",4)!=0 ||
    triangles<100 || triangles>40000 || bones!=16){
   g_hand_failed[hand]=true;return false;
 }
 auto& vertices=g_hand_mesh[hand];
 vertices.resize(static_cast<std::size_t>(triangles)*3);
 for(RigVertex& v:vertices){
  f.read(reinterpret_cast<char*>(&v.pos.x),4);
  f.read(reinterpret_cast<char*>(&v.pos.y),4);
  f.read(reinterpret_cast<char*>(&v.pos.z),4);
  f.read(reinterpret_cast<char*>(&v.normal.x),4);
  f.read(reinterpret_cast<char*>(&v.normal.y),4);
  f.read(reinterpret_cast<char*>(&v.normal.z),4);
  f.read(reinterpret_cast<char*>(&v.u),4);
  f.read(reinterpret_cast<char*>(&v.v),4);
  f.read(reinterpret_cast<char*>(v.bones),3);
  for(int i=0;i<3;++i)
   f.read(reinterpret_cast<char*>(&v.weight[i]),4);
  if(!f){g_hand_failed[hand]=true;vertices.clear();return false;}
  float sum=0;
  for(int i=0;i<3;++i){
   if(v.bones[i]>=16 || !std::isfinite(v.weight[i]) ||
      v.weight[i]<0.f || v.weight[i]>1.001f){
    g_hand_failed[hand]=true;vertices.clear();return false;
   }
   sum+=v.weight[i];
  }
  if(sum<.99f || sum>1.01f){
   g_hand_failed[hand]=true;vertices.clear();return false;
  }
 }
 g_hand_ready[hand]=true;
 std::cerr<<"Linthesia: mano articulada "<<hand<<", "<<triangles
          <<" triángulos, rig de "<<bones<<" huesos"<<std::endl;
 return true;
}
bool DrawRiggedHand(const PianoHands::Frame& frame,int hand,V3 wrist){
 if(!g_skin_texture || !LoadRiggedHand(hand))return false;
 const auto pose=HandRig::Sample(frame,hand,
                                  {wrist.x,wrist.y,wrist.z});
 if(!HandRig::Valid(pose))return false;
 glEnable(GL_TEXTURE_2D);
 glBindTexture(GL_TEXTURE_2D,g_skin_texture->GetId());
 glColor3ub(242,242,242);
 glPushMatrix();
 glTranslatef(wrist.x,wrist.y,wrist.z);
 glBegin(GL_TRIANGLES);
 for(const auto& v:g_hand_mesh[hand]){
  HandRig::V pos{},normal{};
  for(int k=0;k<3;++k){
   const float w=v.weight[k];
   if(w<=.00001f)continue;
   const auto& bone=pose[v.bones[k]];
   pos=pos+bone.Map(v.pos)*w;
   normal=normal+bone.Rotate(v.normal)*w;
  }
  normal=HandRig::Unit(normal);
  glNormal3f(normal.x,normal.y,normal.z);
  glTexCoord2f(v.u,v.v);
  glVertex3f(pos.x,pos.y,pos.z);
 }
 glEnd();
 glPopMatrix();
 glDisable(GL_TEXTURE_2D);
 return true;
}
void DrawLiveFingers(const PianoHands::Frame& frame){
 const unsigned char coatR=34,coatG=42,coatB=64;
 const unsigned char skinR=172,skinG=112,skinB=78;
 for(int hand=0;hand<2;++hand){
  const int sign=hand==0?-1:1;
  // Shoulder anchors measured directly on the MakeHuman CC0 seated torso.
  // Previous -1.22/1.80 anchors required impossible one-metre arms.
  const V3 shoulder={-.48f+sign*.235f,-1.03f,
                     1.65f+frame.shoulderRise};
  const float wx=frame.hands[hand].wristX;
  // Wrist pitches gently upward to reach the higher black keys;
  // MCP joints stay close to white keys instead of towering above them.
  const V3 wrist={wx,frame.hands[hand].wristY,
                  HandRig::WristHeight(frame,hand)};
  const auto bend=PianoKinematics::Elbow(
      {shoulder.x,shoulder.y,shoulder.z},
      {wrist.x,wrist.y,wrist.z},
      {shoulder.x+sign*.075f,-.86f,1.38f},.435f,.395f);
  const V3 elbow={bend.x,bend.y,bend.z};
  // Use real MakeHuman CC0 palmar anatomy and photographic skin UV.
  // The old ellipsoid remains only as fallback for missing palm assets.
  TailoredSleeve(shoulder,elbow,wrist,hand==0);
  // The Blender armature and single connected anatomical skin take
  // precedence. Old MakeHuman palm + separate tubes remain safe fallback.
  if(DrawRiggedHand(frame,hand,wrist))continue;
  if(g_palm_lists[hand] && g_skin_texture){
   glBindTexture(GL_TEXTURE_2D,g_skin_texture->GetId());
   glPushMatrix();
   // Align the MakeHuman palm's distal edge with the MCP joints.
   glTranslatef(wrist.x,wrist.y+.006f,wrist.z);
   glCallList(g_palm_lists[hand]);
   glPopMatrix();
  }else{
   Ball3D(wrist+V3{0,.047f,-.009f},{.077f,.053f,.023f},
          184,128,102);
  }
  for(int fingerId=0;fingerId<5;++fingerId){
   const auto& finger=frame.hands[hand].finger[fingerId];
   const float spread=PianoHands::BaseOffsetX(hand,fingerId);
   // Move the palm forward when attacking a raised black key: fingers
   // bend at knuckles, instead of stretching beyond bone lengths.
   const bool black=finger.note>=21 &&
                    !PianoHands::White(finger.note);
   const float fingerBaseY=wrist.y+
      PianoHands::BaseOffsetY(hand,fingerId,black);
   const float thumbPivot=PianoHands::IsThumb(hand,fingerId)?.17f:.26f;
   const V3 base={wx+spread+thumbPivot*(finger.x-(wx+spread)),
                  fingerBaseY,1.015f+(black?.027f:0.f)};
   const V3 tip={finger.x,finger.y,finger.z};
   // Real inverse geometry: a fixed-length two-link chain solves the
   // intermediary metacarpophalangeal joint instead of arbitrary linear
   // interpolation that visibly stretches fingers as MIDI pitch changes.
   // Piano fingering needs a LOW arch, not the high elbow-like IK bend
   // that made the fingers look like spider legs. Short, smooth dorsal
   // curve between palmar knuckles and the note-specific key contact.
   const float raise=PianoHands::FingerArch(hand,fingerId);
   // Anatomical thumb opposition: the thumb's MCP moves outward from
   // the index web, but the other four digits follow a low dorsal arc.
   const float thumbSide=PianoHands::IsThumb(hand,fingerId)?
                            (hand==0?.006f:-.006f):0.f;
   const V3 knuckle=base*.65f+tip*.35f+
                   V3{thumbSide,0.f,raise};
   const V3 middle=base*.32f+tip*.68f+
                   V3{thumbSide*.40f,0.f,raise*.60f};
   AnatomicalFinger(base,knuckle,middle,tip,hand,fingerId);
  }
 }
}
bool LoadMeshPath(const std::string& path,
                  std::vector<Triangle>& tris){
 std::ifstream f(path,std::ios::binary);
 if(!f)return false;
 char header[8]={};
 std::uint32_t count=0;
 f.read(header,8);f.read(reinterpret_cast<char*>(&count),4);
 if(!f || std::memcmp(header,"L3DM",4)!=0 ||
    (header[4]!=1&&header[4]!=2&&header[4]!=3) ||
    count>250000u)return false;
 tris.reserve(count);
 for(std::uint32_t i=0;i<count;++i){
  Triangle t;
  for(Vertex& v:t){
   f.read(reinterpret_cast<char*>(&v.p.x),4);
   f.read(reinterpret_cast<char*>(&v.p.y),4);
   f.read(reinterpret_cast<char*>(&v.p.z),4);
   f.read(reinterpret_cast<char*>(&v.r),1);
   f.read(reinterpret_cast<char*>(&v.g),1);
   f.read(reinterpret_cast<char*>(&v.b),1);
   if(header[4]>=2){
    f.read(reinterpret_cast<char*>(&v.n.x),4);
    f.read(reinterpret_cast<char*>(&v.n.y),4);
    f.read(reinterpret_cast<char*>(&v.n.z),4);
   }else v.n={0,0,0};
   if(header[4]==3){
    f.read(reinterpret_cast<char*>(&v.u),4);
    f.read(reinterpret_cast<char*>(&v.v),4);
    f.read(reinterpret_cast<char*>(&v.textured),1);
   }
  }
  if(!f)return false;
  if(header[4]==1){
   const V3 n=unit(cross(t[1].p-t[0].p,t[2].p-t[0].p));
   for(auto& vertex:t)vertex.n=n;
  }
  tris.push_back(t);
 }
 return true;
}
bool LoadMeshFile(int index,std::vector<Triangle>& tris){
 if(index<6||index>68)return false;
 std::string prefix;int frame=0;
 if(index<=16){prefix="pianista";frame=index-6;}
 else if(index<=48){
   const char* names[]={"guitarrista","baterista","cantante","cantantef"};
   const int id=(index-17)/8;
   prefix=names[id];frame=(index-17)%8;
 }else if(index<=56){prefix="duo";frame=index-49;}
 else{prefix="banda";frame=index-57;}
 const std::string name="mesh_"+prefix+"_"+
            (frame<10?std::string("0"):std::string(""))+
            std::to_string(frame)+".l3d";
 return LoadMeshPath(std::string(GRAPHDIR)+"/scenarios/"+name,tris);
}
bool LoadPalmMesh(int hand,std::vector<Triangle>& tris){
 if(hand<0||hand>1)return false;
 const std::string name=hand==0?"mesh_palma_cc0_00.l3d":
                                  "mesh_palma_cc0_01.l3d";
 return LoadMeshPath(std::string(GRAPHDIR)+"/scenarios/"+name,tris);
}
void InitPalmMeshes(){
 if(!g_skin_texture)return;
 for(int hand=0;hand<2;++hand){
  if(g_palm_lists[hand]||g_palm_failed[hand])continue;
  std::vector<Triangle> triangles;
  if(!LoadPalmMesh(hand,triangles) || triangles.empty()){
   g_palm_failed[hand]=true;continue;
  }
  const GLuint list=glGenLists(1);
  if(!list){g_palm_failed[hand]=true;continue;}
  glNewList(list,GL_COMPILE);
  glEnable(GL_TEXTURE_2D);
  glColor3ub(244,244,244);
  glBegin(GL_TRIANGLES);
  for(const auto& tri:triangles){
   for(const auto& v:tri){
    const V3 n=unit(v.n);
    glNormal3f(n.x,n.y,n.z);
    glTexCoord2f(v.u,v.v);
    glVertex3f(v.p.x,v.p.y,v.p.z);
   }
  }
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glEndList();
  g_palm_lists[hand]=list;
 }
}
} // namespace

bool Renderer::DrawBlenderScene3D(int scene,int pose,
                 int x,int y,int width,int height,
                 int yaw,int pitch,int zoom,int lightLevel,int lightTint,
                 const PianoHands::Frame* liveHands,
                 int cameraFocus,int cameraMotion,float elapsedSeconds){
 if(scene<6 || scene>=StageScenes::Count || width<2 || height<2)return false;
 const int index=scene==6?6+std::max(0,std::min(10,pose)):
                 scene==12?57+std::max(0,std::min(11,pose)):
                 scene==11?49+std::max(0,std::min(7,pose)):
                 17+(scene-7)*8+std::max(0,std::min(7,pose));
 if(index<6 || index>=69)return false;
 if(scene>=6 && !g_skin_texture && !g_skin_failed){
  try{
   g_skin_texture=Tga::Load("scenarios/piel_masculina_cc0");
   if(g_skin_texture)g_skin_texture->SetSmooth(true);
  }catch(const std::exception& ex){
   g_skin_failed=true;
   std::cerr<<"Linthesia: textura de piel no disponible: "
            <<ex.what()<<std::endl;
  }
 }
 if(scene==6)InitPalmMeshes();
 if(!g_stage_lists[index] && !g_stage_failed[index]){
  std::vector<Triangle> data;
  if(!LoadMeshFile(index,data) || data.empty()){
   g_stage_failed[index]=true;
   std::cerr<<"Linthesia: no se encontró la malla Blender 3D "
            <<index<<"; usando fondo original"<<std::endl;
  }else{
   const GLuint list=glGenLists(scene==6?2:1);
   if(list){
    // Split head+scalp geometry into a separate OpenGL display list,
    // making anatomically anchored real-time neck movement possible.
    // All other objects remain fixed in the piano's original Blender space.
    for(int segment=0;segment<(scene==6?2:1);++segment){
     glNewList(list+segment,GL_COMPILE);
     int currentState=-1;
     const V3 sun=unit({-.30f,-.55f,1.30f});
     for(const Triangle& tri:data){
     const bool movingHead=(tri[0].textured&2)!=0;
     if((segment==1)!=movingHead)continue;
     const int nextState=int((tri[0].textured&1)!=0 && g_skin_texture!=nullptr);
     if(currentState!=nextState){
      if(currentState>=0)glEnd();
      if(nextState)glEnable(GL_TEXTURE_2D);
      else glDisable(GL_TEXTURE_2D);
      glBegin(GL_TRIANGLES);
      currentState=nextState;
     }
     const V3 n=unit(cross(tri[1].p-tri[0].p,tri[2].p-tri[0].p));
     glNormal3f(n.x,n.y,n.z);
     const float diffuse=std::max(0.f,dot(n,sun));
     const float shade=.94f+.25f*diffuse;
     for(const auto& v:tri){
      // Gentle highlight is essential for readability of black piano lacquer.
      const float spec=std::max(0.f,diffuse-.67f)*
                      (nextState?0.f:12.f);
      const auto channel=[&](unsigned char a)->unsigned char{
       return static_cast<unsigned char>(std::max(0.f,
              std::min(255.f,a*shade+spec)));
      };
      glColor3ub(channel(v.r),channel(v.g),channel(v.b));
      if(nextState)glTexCoord2f(v.u,v.v);
      const V3 smoothNormal=unit(v.n);
      glNormal3f(smoothNormal.x,smoothNormal.y,smoothNormal.z);
      glVertex3f(v.p.x,v.p.y,v.p.z);
     }
    }
     if(currentState>=0)glEnd();
     glDisable(GL_TEXTURE_2D);
     glEndList();
    }
    g_stage_lists[index]=list;
    if(scene==6)g_head_lists[index]=list+1;
   }else g_stage_failed[index]=true;
  }
 }
 if(!g_stage_lists[index])return false;
 int drawableW=0,drawableH=0;
 SDL_GL_GetDrawableSize(m_sdl_window,&drawableW,&drawableH);
 if(drawableW<1 || drawableH<1)return false;
 const GLint logicalW=std::max(1,UiAppearance::LogicalWidth(drawableW,drawableH));
 const GLint logicalH=std::max(1,UiAppearance::LogicalHeight(drawableW,drawableH));
 const float sx=float(drawableW)/logicalW,sy=float(drawableH)/logicalH;
 const GLint vx=std::max(0,int((x+m_xoffset)*sx));
 const GLint vw=std::min(drawableW-vx,int(width*sx));
 const GLint vh=std::min(drawableH,int(height*sy));
 const GLint vy=std::max(0,drawableH-int((y+height+m_yoffset)*sy));
 if(vw<=2||vh<=2)return false;
 GLint originalViewport[4]={};
 glGetIntegerv(GL_VIEWPORT,originalViewport);
 glPushAttrib(GL_ALL_ATTRIB_BITS);
 glViewport(vx,vy,vw,vh);
 glEnable(GL_SCISSOR_TEST);
 glScissor(vx,vy,vw,vh);
 glEnable(GL_DEPTH_TEST);
 glDepthFunc(GL_LEQUAL);
 glDepthMask(GL_TRUE);
 glClear(GL_DEPTH_BUFFER_BIT);
 glDisable(GL_TEXTURE_2D);
 glDisable(GL_BLEND);
 glDisable(GL_LIGHTING);
 glDisable(GL_CULL_FACE);
 glMatrixMode(GL_PROJECTION);
 glPushMatrix();glLoadIdentity();
 const float nearPlane=.20f,farPlane=115.f;
 const float aspect=float(vw)/vh;
 const float top=nearPlane*
                       ((scene==6 && cameraFocus==2)?.28f:.44f);
 glFrustum(-top*aspect,top*aspect,-top,top,nearPlane,farPlane);
 glMatrixMode(GL_MODELVIEW);
 glPushMatrix();glLoadIdentity();
 // Camera animation is rendered in true 3D, not just motion of the 2D
 // spotlight overlay. Manual orbit and presets define the centre; the
 // slider adds a subtle, non-accumulating cinematographic camera dolly.
 const auto camera=CinematicCamera::Sample(
      float(yaw),float(pitch),float(zoom),
      cameraMotion,elapsedSeconds);
 const float az=camera.yaw*3.14159265f/180.f;
 const float elev=camera.pitch*3.14159265f/180.f;
 const float range=camera.distance/100.f;
 const V3 target=scene==6 && cameraFocus==2?
      V3{-.48f,-.47f,1.07f}:
      (scene==6 && cameraFocus==1?
         V3{-.48f,-1.10f,1.79f}:V3{-.48f,.53f,1.30f});
 const V3 eye={target.x+range*std::cos(elev)*std::sin(az),
               target.y-range*std::cos(elev)*std::cos(az),
               target.z+range*std::sin(elev)};
 const V3 forward=unit(target-eye);
 const V3 right=unit(cross(forward,{0,0,1}));
 const V3 up=cross(right,forward);
 const GLfloat matrix[]={
  right.x,up.x,-forward.x,0,
  right.y,up.y,-forward.y,0,
  right.z,up.z,-forward.z,0,
  0,0,0,1
 };
 glMultMatrixf(matrix);
 glTranslatef(-eye.x,-eye.y,-eye.z);
 // Real fixed-function 3D lighting responds to the scene light controls.
 // Triangles carry their own face normal and material colour from Blender.
 const auto tint=StageScenes::LightColor(lightTint);
 const float power=.36f+.90f*std::max(0,std::min(100,lightLevel))/100.f;
 const GLfloat ambient[4]={.52f,.53f,.57f,1.f};
 const GLfloat directional[4]={-2.5f,-3.5f,7.0f,0.f};
 const GLfloat diffuse[4]={(.48f+.52f*tint.r/255.f)*power,
                           (.48f+.52f*tint.g/255.f)*power,
                           (.48f+.52f*tint.b/255.f)*power,1.f};
 const GLfloat specular[4]={.16f,.16f,.16f,1.f};
 glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);glEnable(GL_LIGHT1);
 const GLfloat fillPos[4]={2.1f,-1.9f,3.7f,0.f};
 const GLfloat fillDiffuse[4]={.25f*power,.29f*power,.39f*power,1.f};
 glLightfv(GL_LIGHT1,GL_POSITION,fillPos);
 glLightfv(GL_LIGHT1,GL_DIFFUSE,fillDiffuse);
 // Soft rear rim gives the suit and face a concert-stage silhouette, with
 // brightness controlled by the existing editable light-intensity slider.
 const GLfloat rimPos[4]={-2.2f,3.5f,5.5f,0.f};
 const GLfloat rimDiffuse[4]={.17f*power,.21f*power,.29f*power,1.f};
 glEnable(GL_LIGHT2);
 glLightfv(GL_LIGHT2,GL_POSITION,rimPos);
 glLightfv(GL_LIGHT2,GL_DIFFUSE,rimDiffuse);
 glEnable(GL_NORMALIZE);glEnable(GL_COLOR_MATERIAL);
 glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
 glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
 glLightfv(GL_LIGHT0,GL_POSITION,directional);
 glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);
 glLightfv(GL_LIGHT0,GL_SPECULAR,specular);
 glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
 glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular);
 glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,35.f);
 if(g_skin_texture){
  glBindTexture(GL_TEXTURE_2D,g_skin_texture->GetId());
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
 }
 glCallList(g_stage_lists[index]);
 // The head follows the hand register and the phrase with a slow, small
 // nod. These are actual 3D transformations, not prerendered frames.
 if(scene==6 && g_head_lists[index]){
  const GLfloat yawHead=liveHands?liveHands->headYaw:0.f;
  const GLfloat nodHead=liveHands?liveHands->headNod:0.f;
  glPushMatrix();
  glTranslatef(-.48f,-1.17f,2.035f); // base of neck (Blender metres)
  glRotatef(yawHead,0.f,0.f,1.f);
  glRotatef(-10.0f-nodHead,1.f,0.f,0.f);
  glTranslatef(.48f,1.17f,-2.035f);
  glCallList(g_head_lists[index]);
  glPopMatrix();
 }
 // The dynamic hands & keys have untextured materials.
 glDisable(GL_TEXTURE_2D);
 // Every 3D stage renders exactly the SAME set of 88 MIDI-driven
 // keys. Only the pianist stage needs the animated body and hands.
 if(scene==6 && liveHands){
  DrawLiveKeys(*liveHands);
  DrawLiveFingers(*liveHands);
 }
 glPopMatrix();
 glMatrixMode(GL_PROJECTION);glPopMatrix();
 glMatrixMode(GL_MODELVIEW);
 glPopAttrib();
 glViewport(originalViewport[0],originalViewport[1],
            originalViewport[2],originalViewport[3]);
 SetColor(255,255,255);
 // 3D shading and animated 2D light beams are separate controls.
 return true;
}
