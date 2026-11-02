#include "HandRigKinematics.h"
#include <fstream>
#include <array>
#include <vector>
#include <iostream>
#include <cmath>
struct Vertex{
 HandRig::V p;
 HandRig::V n;
 float u,v;unsigned char bones[3];float w[3];
};
int main(int argc,char** argv){
 if(argc!=2)return 2;
 PianoHands::Animator a;
 std::array<bool,128> active{};for(int test=21;test<=108;++test){active.fill(false);active[test]=true;
 for(int t=0;t<130;++t)a.Update(active,.016f,100);
 for(int hand=0;hand<2;++hand){
  std::ifstream f(std::string(argv[1])+"/graphics/scenarios/mesh_mano_rig_0"+std::to_string(hand)+".lhr",std::ios::binary);
  if(!f)return 3;
  char magic[4];unsigned count,bones;f.read(magic,4);f.read((char*)&count,4);f.read((char*)&bones,4);
  const auto& hs=a.Current().hands[hand];HandRig::V wrist{hs.wristX,hs.wristY,HandRig::WristHeight(a.Current(),hand)};
  auto pose=HandRig::Sample(a.Current(),hand,wrist);
  int collide=0,total=0,skinFloor=0;float worst=-1,wx=0,wy=0,wz=0;int blackPitch=0;
  for(int i=0;i<count*3;++i){
   Vertex v;f.read((char*)&v.p.x,4);f.read((char*)&v.p.y,4);f.read((char*)&v.p.z,4);
   f.read((char*)&v.n.x,4);f.read((char*)&v.n.y,4);f.read((char*)&v.n.z,4);
   f.read((char*)&v.u,4);f.read((char*)&v.v,4);f.read((char*)v.bones,3);
   for(auto& w:v.w)f.read((char*)&w,4);
   HandRig::V o{};
   for(int k=0;k<3;++k)o=o+pose[v.bones[k]].Map(v.p)*v.w[k];
   o=o+wrist;total++;
   if(o.z<.960f)skinFloor++;
   for(int note=21;note<=108;++note){
    if(PianoHands::White(note))continue;
    const float x=PianoHands::KeyX(note);
    if(std::abs(o.x-x)<.0065f && o.y>-.4100f&&o.y<-.2800f &&
       o.z>.9555f && o.z<1.0205f){
     collide++;float depth=1.0205f-o.z;
     if(depth>worst){worst=depth;wx=o.x;wy=o.y;wz=o.z;blackPitch=note;}
     break;
    }
   }
  }
  // The skin pad can compress a few millimetres into its own
  // depressed black key, but must never penetrate the keybed.
  if(worst>.0035f || skinFloor>0)return 1;
  if(collide>0)std::cout<<"TEST "<<test<<" HAND "<<hand<<" collision "<<collide<<"/"<<total<<" blackPitch "<<blackPitch<<" max penetration "<<worst<<" at "<<wx<<","<<wy<<","<<wz<<" below keyboard "<<skinFloor<<"\n";
 }
}
}
