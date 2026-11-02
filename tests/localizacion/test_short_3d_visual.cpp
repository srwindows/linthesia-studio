#include "ShortNoteVisual.h"
#include "TopBirthEmitter.h"
#include <cstdlib>
#include <iostream>
#include <cmath>
static void req(bool ok,const char* why){
 if(!ok){std::cerr<<"ERROR "<<why<<"\n";std::exit(1);}
}
int main(){
 for(int white: {12,18,24,38,52}){
  for(int setting:{40,70,100,150,200}){
    const int max=ShortNoteVisual::Maximum(white,setting);
    req(max>=18&&max<=140,"length outside design constraints");
    for(int actual:{8,40,80,200,530,1300}){
      const auto falling=ShortNoteVisual::Fit(100,100+actual,max);
      req(falling.height<=max,"visual bar exceeds limit");
      req(falling.height<=actual,"cannot elongate note");
      req(falling.top+falling.height==100+actual,"falling leading edge moved");
      const auto absorbed=ShortNoteVisual::Absorb(600,max,.75f);
      req(absorbed.height==0,"head must be fully absorbed, not held on key");
    }
  }
 }
 req(ShortNoteVisual::Maximum(18,100)<=65,"default note body too long");
 req(TopBirthEmitter::Reveal(0)==0,"explosion starts without body");
 req(TopBirthEmitter::Reveal(.04f)>.001f,"visible note does not emerge immediately");
 req(TopBirthEmitter::Reveal(.50f)==1.0f,"too much initial birth delay");
 req(TopBirthEmitter::Sample(.07f,0,120,20).alpha>.01f,
     "falling particles delayed behind note");
 std::cout<<"OK: compact note follows leading edge, MIDI sustain unchanged, synchronous ignition\n";
}
