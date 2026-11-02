#include "PianoKinematics.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace PianoKinematics;
static bool near(float a,float b,float eps=.002f){return std::fabs(a-b)<eps;}
int main(){
 const Vec3 shoulder={-.815f,-1.22f,1.80f};
 const Vec3 wrist={-.77f,-.614f,1.08f};
 const Vec3 elbow=Elbow(shoulder,wrist,{-.92f,-.86f,1.38f},.60f,.48f);
 assert(near(Norm(elbow-shoulder),.60f));
 assert(near(Norm(elbow-wrist),.48f));
 assert(std::isfinite(elbow.x)&&std::isfinite(elbow.y)&&std::isfinite(elbow.z));
 for(float pitchX=-1.0f;pitchX<-.20f;pitchX+=.08f){
   Vec3 candidate={pitchX,-.615f,1.08f};
   const Vec3 e=Elbow(shoulder,candidate,
                      {shoulder.x-.09f,-.88f,1.36f},.60f,.48f);
   assert(std::isfinite(e.x)&&std::isfinite(e.y)&&std::isfinite(e.z));
   if(Norm(candidate-shoulder)<1.07f){
    assert(near(Norm(e-shoulder),.60f));
    assert(near(Norm(e-candidate),.48f));
   }
 }
 const Vec3 root={-.70f,-.574f,1.067f};
 const Vec3 finger={-.682f,-.413f,.976f};
 const auto joint=Elbow(root,finger,{-0.687f,-.495f,1.105f},
                       .105f,.100f);
 assert(near(Norm(root-joint),.105f));
 assert(near(Norm(finger-joint),.100f));
 // Note contacts remain independently positioned on physically correct
 // white/black keys. This joint solver does not change MIDI pitch mapping.
 std::cout<<"OK: brazo y falanges de dos tramos, longitud constante, IK estable\n";
}
