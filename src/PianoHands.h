// SPDX-License-Identifier: GPL-2.0-or-later
// Performance-driven hand and grand-key dynamics, independent from render FPS.
#ifndef LINTHESIA_PIANO_HANDS_H
#define LINTHESIA_PIANO_HANDS_H
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
namespace PianoHands {
inline float Clamp(float x,float a,float b){return std::max(a,std::min(b,x));}
inline bool White(int midi){
 const int c=midi%12;
 return c==0||c==2||c==4||c==5||c==7||c==9||c==11;
}
inline int WhiteBefore(int midi){
 int n=0;for(int k=21;k<midi&&k<109;++k)if(White(k))++n;
 return n;
}
inline float KeyX(int note){
 if(note<21||note>108)return -.48f;
 const int w=WhiteBefore(note);
 return -1.163f+(White(note)?float(w):float(w)-.46f)*.02645f;
}
inline float KeyTop(int note){return White(note)?.970f:1.0205f;}
inline float KeyContactY(int note){return White(note)?-.475f:-.382f;}
// Match the real OpenGL hinge, not a uniform vertical shift. The rear
// remains at rest height and the front sinks towards the keybed.
inline float KeySurfaceZ(int note,float y,float press){
 if(note<21||note>108)return .970f;
 const bool white=White(note);
 const float length=white?.200f:.130f;
 const float originY=white?-.420f:-.345f;
 const float originZ=white?.950f:.988f;
 const float depth=white?.040f:.065f;
 const float rearY=originY+length*.5f;
 const float travel=white?.014f:.010f;
 const float angle=std::atan2(travel,length)*Clamp(press,0.f,1.f);
 return originZ+(depth*.5f)*std::cos(angle)+(y-rearY)*std::sin(angle);
}
// Five anatomical digits, ordered thumb/index/middle/ring/little.
// MIDI slots are ordered left-to-right along the keyboard.  In the left
// hand the thumb is on the RIGHT; in the right hand it is on the LEFT.
// The reference photograph shows this unmistakably, unlike the old rig
// which treated the left little finger as a thumb.
inline int DigitForSlot(int hand,int slot){
 return hand==0?4-slot:slot;
}
// User finger numbers are anatomical: thumb=1 ... little=5 in BOTH hands.
// Slot order follows key pitch, so left-hand digits are mirrored.
inline int SlotForFinger(int hand,int finger){
 if(hand<0||hand>1||finger<1||finger>5)return -1;
 return hand==0?5-finger:finger-1;
}
struct Guidance {
 std::array<int,128> hands;   // -1 automatic, 0 left, 1 right
 std::array<int,128> fingers; //  0 automatic, 1..5 thumb to little
 Guidance(){hands.fill(-1);fingers.fill(0);}
};
inline bool IsThumb(int hand,int slot){
 return DigitForSlot(hand,slot)==0;
}
inline float BaseOffsetX(int hand,int slot){
 const float positions[5]={-.048f,-.024f,0.f,.024f,.050f};
 const int digit=DigitForSlot(hand,slot);
 // The thumb's root is tucked closer to the index web, while its tip
 // abducts inward across the key. The earlier root was too far outward
 // and made the thumb appear parallel to the other four fingers.
 // Thumb tip rests abducted beyond the index fingertip. The thumb
 // root remains at the first metacarpal (below, in HandRigKinematics).
 // Keeping both tips 1cm apart made two fingers look missing.
 if(digit==0)return hand==0?.070f:-.070f;
 return positions[slot];
}
inline float BaseOffsetY(int hand,int slot,bool blackKey){
 const float restY[5]={.067f,.104f,.111f,.105f,.092f};
 const int digit=DigitForSlot(hand,slot);
 return restY[digit]+(blackKey && digit!=0?.009f:0.f);
}
inline float FingerContactY(int note,int hand,int slot){
 if(note<21||note>108){
  const float idle[5]={-.505f,-.476f,-.458f,-.468f,-.493f};
  return idle[DigitForSlot(hand,slot)];
 }
 const int digit=DigitForSlot(hand,slot);
 // The thumb stays at the FRONT edge of the key and falls more flatly,
 // while the middle digit reaches farther into the keyboard.  All of these
 // targets are within the actual longitudinal extents of the keys.
 // White-key fingertips extend INTO the keyboard as in an actual
 // pianist's posture: longer index/middle fingers almost horizontal.
 // Previously they barely moved beyond the metacarpal joints, forcing
 // an exaggerated downward claw of 40+ degrees.
 // Targets respect the 7-10cm visible phalanx length in the photo.
 const float whiteY[5]={-.505f,-.476f,-.458f,-.468f,-.493f};
 const float blackY[5]={-.406f,-.391f,-.381f,-.389f,-.404f};
 return White(note)?whiteY[digit]:blackY[digit];
}
inline float FingerWidth(int hand,int slot){
 // Anatomical near-MCP *radii*; old digits were 20-26mm thick.
 const float mm[5]={.0116f,.0103f,.0107f,.0103f,.0088f};
 return mm[DigitForSlot(hand,slot)];
}
inline float FingerArch(int hand,int slot){
 const float arch[5]={.004f,.012f,.014f,.012f,.009f};
 return arch[DigitForSlot(hand,slot)];
}
inline int ParseNoteName(const std::string& name){
 // E.g., C4=60, A0=21, C#5=73. Handle C-1 without unsigned wrap.
 if(name.size()<2)return -1;
 const std::string names[]={"C","C#","D","D#","E","F","F#",
                            "G","G#","A","A#","B"};
 int prefix=-1;size_t len=0;
 for(int k=0;k<12;++k){
  if(name.compare(0,names[k].size(),names[k])==0 &&
     names[k].size()>len){prefix=k;len=names[k].size();}
 }
 if(prefix<0||len>=name.size())return -1;
 int octave=0;bool neg=false;
 size_t i=len;
 if(name[i]=='-'){neg=true;++i;}
 if(i>=name.size())return -1;
 for(;i<name.size();++i){
  if(name[i]<'0'||name[i]>'9')return -1;
  octave=octave*10+name[i]-'0';
  if(octave>10)return -1;
 }
 return prefix+12*((neg?-octave:octave)+1);
}
struct Finger {
 int note=-1;
 float x=0,y=0,z=1.05f,press=0;
};
struct Hand {
 float wristX=0;
 float wristY=-.615f;  // forward motion when reaching raised black keys
 std::array<Finger,5> finger;
};
struct Frame {
 std::array<float,128> keys{};
 std::array<Hand,2> hands;
 int liveNotes=0;
 int assignedNotes=0;
 float headYaw=0.0f;
 float headNod=0.0f;
 float shoulderRise=0.0f;
};
class Animator {
 Frame m_frame;
 bool m_started=false;
 static float Ease(float now,float target,float dt,float speed){
  return now+(target-now)*(1.f-std::exp(-Clamp(dt,0.f,.08f)*speed));
 }
public:
 const Frame& Current()const{return m_frame;}
 void Reset(){m_frame=Frame();m_started=false;}
 void Update(const std::array<bool,128>& active,float dt,int motion,
             const Guidance& guidance=Guidance()){
  if(!m_started){
   m_frame.hands[0].wristX=-.77f;
   m_frame.hands[1].wristX=-.20f;
   for(int h=0;h<2;++h){
    for(int f=0;f<5;++f){
     auto& finger=m_frame.hands[h].finger[f];
     finger.x=m_frame.hands[h].wristX+BaseOffsetX(h,f);
     finger.y=FingerContactY(60,h,f);
     finger.z=1.005f;
    }
   }
   m_started=true;
  }
  m_frame.liveNotes=0;m_frame.assignedNotes=0;
  const float speed=.40f+.60f*Clamp(float(motion),0.f,100.f)/100.f;
  std::vector<int> all;
  for(int note=21;note<=108;++note){
   const float target=active[note]?1.f:0.f;
   m_frame.keys[note]=Ease(m_frame.keys[note],target,dt,target>0?.08f+24.f:14.f);
   if(active[note]){all.push_back(note);++m_frame.liveNotes;}
  }
  std::vector<int> pitches[2];
  bool guided=false;
  for(int note:all)
    if(guidance.hands[note]>=0 || guidance.fingers[note]>0)
      guided=true;
  for(int note:all){
    const int preferred=guidance.hands[note];
    pitches[preferred==0||preferred==1?preferred:(note<60?0:1)]
      .push_back(note);
  }
  // Keep the existing adaptive split for unannotated songs. When the
  // score explicitly specifies a hand, never silently send its note
  // to the opposite hand merely to balance a dense chord.
  if(!guided && all.size()>=2&&(pitches[0].empty()||pitches[1].empty())){
   const int cut=static_cast<int>(all.size())/2;
   pitches[0].assign(all.begin(),all.begin()+cut);
   pitches[1].assign(all.begin()+cut,all.end());
  }
  // Choose finger slots by minimising travel and strongly rewarding
  // continuity: a held MIDI note NEVER swaps fingers because another
  // note enters or leaves the chord. This avoids robotic hand flickering.
  // Exhaustive combinations are tiny: at most C(5,3)=10 candidates.
  const int slots[6][5]={{-1,-1,-1,-1,-1},{2,-1,-1,-1,-1},
     {1,3,-1,-1,-1},{1,2,3,-1,-1},{0,1,2,3,-1},{0,1,2,3,4}};
  // Determine both wrists together. A close chord must not stack the
  // two palms at the same X coordinate (the former 3+3 notes in a single
  // octave caused two hands to intersect). The key pitches remain exact.
  float wristTargets[2]={-.77f,-.20f};
  int playing[2]={0,0};
  for(int h=0;h<2;++h){
   const auto& notes=pitches[h];
   playing[h]=motion<=0?0:std::min(5,static_cast<int>(notes.size()));
   if(playing[h]){
    float sum=0;
    for(int i=0;i<playing[h];++i)sum+=KeyX(notes[i]);
    wristTargets[h]=sum/playing[h];
   }
   wristTargets[h]=Clamp(wristTargets[h],-1.10f,.12f);
  }
  if(playing[0] && playing[1]){
   const float minimumPalmClearance=.165f;
   const float distance=wristTargets[1]-wristTargets[0];
   if(distance<minimumPalmClearance){
    const float centre=(wristTargets[0]+wristTargets[1])*.5f;
    wristTargets[0]=Clamp(centre-minimumPalmClearance*.5f,-1.10f,.12f);
    wristTargets[1]=Clamp(centre+minimumPalmClearance*.5f,-1.10f,.12f);
   }
  }
  for(int h=0;h<2;++h){
   const auto& notes=pitches[h];
   auto& hand=m_frame.hands[h];
   const int used=playing[h];
   const float wristTarget=wristTargets[h];
   hand.wristX=Ease(hand.wristX,wristTarget,dt,8.f*speed);
   // Realistic advance of the whole hand towards black keys (not just
   // stretching the fingers while the wrist stays at the white-key edge).
   const int blackCount=static_cast<int>(std::count_if(
      notes.begin(),notes.end(),[](int n){return !White(n);}));
   const float blackRatio=notes.empty()?0.f:
      static_cast<float>(blackCount)/notes.size();
   hand.wristY=Ease(hand.wristY,-.603f+.055f*blackRatio,dt,8.f*speed);
   std::array<int,5> assigned{};
   assigned.fill(-1);
   float bestCost=1e15f;
   if(guided && used>0){
     // Reserve the *actual* anatomical finger (1..5) first. The original
     // pitch-ordered minimizer is allowed only on as-yet unclaimed slots.
     // This drives the SAME skinned 16-bone mesh, not a separate effect.
     std::array<bool,128> chosen{};
     for(int note:notes){
       const int slot=SlotForFinger(h,guidance.fingers[note]);
       if(slot<0 || assigned[slot]>=21)continue;
       assigned[slot]=note;chosen[note]=true;
       if(std::count_if(assigned.begin(),assigned.end(),
            [](int n){return n>=21;})>=used)break;
     }
     for(int note:notes){
       if(chosen[note])continue;
       int occupied=std::count_if(assigned.begin(),assigned.end(),
                        [](int n){return n>=21;});
       if(occupied>=used)break;
       int winner=-1;
       float score=1e15f;
       for(int slot=0;slot<5;++slot){
         if(assigned[slot]>=21)continue;
         const auto& old=hand.finger[slot];
         const float distance=std::fabs(KeyX(note)-old.x);
         const int position=int(std::lower_bound(notes.begin(),notes.end(),
                                                 note)-notes.begin());
         const int ideal=slots[used][std::min(used-1,position)];
         float cost=distance*46.f+std::fabs(float(slot-ideal))*5.f+
                        (old.note>=21?18.f:0.f);
         if(old.note==note)cost-=200.f;
         // Avoid crossing already-reserved neighboring fingers when a
         // consistent non-crossing solution exists.
         for(int other=0;other<5;++other)
           if(assigned[other]>=21 &&
             ((other<slot && assigned[other]>note)||
              (other>slot && assigned[other]<note)))cost+=30.f;
         if(cost<score){score=cost;winner=slot;}
       }
       if(winner>=0){assigned[winner]=note;chosen[note]=true;}
     }
   }else for(int mask=0;mask<32;++mask){
    int bits=0;
    for(int f=0;f<5;++f)bits+=(mask>>f)&1;
    if(bits!=used)continue;
    std::array<int,5> candidate{};
    candidate.fill(-1);
    float cost=0.f;
    int pos=0;
    for(int f=0;f<5;++f){
     if((mask&(1<<f))==0)continue;
     const int note=notes[pos];
     const auto& old=hand.finger[f];
     const float target=KeyX(note);
     cost+=std::fabs(target-old.x)*46.f;
     cost+=std::fabs(float(f-slots[used][pos]))*5.f;
     if(old.note==note)cost-=200.f; // pitch still held by same finger
     else if(old.note>=21)cost+=18.f;
     candidate[f]=note;
     ++pos;
    }
    if(cost<bestCost){bestCost=cost;assigned=candidate;}
   }
   for(int f=0;f<5;++f){
    const int targetNote=assigned[f];
    auto& finger=hand.finger[f];
    const float idleX=hand.wristX+BaseOffsetX(h,f);
    const float targetX=targetNote>=21?KeyX(targetNote):idleX;
    finger.x=Ease(finger.x,targetX,dt,15.f*speed);
    const float goalY=FingerContactY(targetNote,h,f);
    finger.y=Ease(finger.y,goalY,dt,14.f*speed);
    const float convergence=std::fabs(targetX-finger.x);
    const float down=(targetNote>=21&&convergence<.038f)?
                       m_frame.keys[targetNote]:0.f;
    finger.press=Ease(finger.press,down,dt,(down>0?21.f:11.f)*speed);
    // Finger-pad contact follows the DEPRESSED top of the actual key.
    // White key: 14mm travel, black key: 10mm. The tip's ~10mm radius
    // is included, preventing the visually obvious hovering/clipping.
    const float fingerPad=FingerWidth(h,f)*.66f;
    const float contact=KeySurfaceZ(targetNote,finger.y,
        targetNote>=21?m_frame.keys[targetNote]:0.f)+fingerPad;
    const float lift=targetNote>=21?.018f*(1.f-finger.press):.025f;
    finger.z=Ease(finger.z,contact+lift,dt,22.f*speed);
    finger.note=targetNote;
    if(targetNote>=21)++m_frame.assignedNotes;
   }
  }
  // Gaze tracks hand register, with slow neck transitions. Small shoulder
  // motion follows playing phrases; deliberately avoid a metronomic
  // oscillation that looks like a puppet nodding continuously.
  const float centre=.5f*(m_frame.hands[0].wristX+m_frame.hands[1].wristX);
  m_frame.headYaw=Ease(m_frame.headYaw,
       motion?Clamp((centre+.48f)*17.f,-9.f,9.f):0.f,dt,2.0f*speed);
  m_frame.headNod=Ease(m_frame.headNod,
       motion?(m_frame.liveNotes?4.4f:1.0f):0.f,dt,2.0f*speed);
  m_frame.shoulderRise=Ease(m_frame.shoulderRise,
       motion&&m_frame.liveNotes?.010f:0.f,dt,1.7f*speed);
  // Motion=0 freezes the visual performer in resting position, while the
  // eighty-eight physical piano keys still track the MIDI NoteOn/NoteOff.
  // Other values regulate arm/finger travel and articulation speed.
 }
};
}
#endif
