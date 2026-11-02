#include "NoteColumnSizing.h"
#include <iostream>
#include <cstdlib>
static void ensure(bool ok,const char *msg){
  if(!ok){std::cerr<<msg<<"\n";std::exit(1);}
}
int main(){
  for(int white: {12,17,23,31,45}){
    for(bool black: {false,true}){
      const int keyWidth=black?int(white*.5454545):white;
      for(int setting: {25,50,65,100,108,125,200}){
        const auto n=NoteColumnSizing::Fit3D(300,keyWidth,black,setting);
        ensure(n.width>=3 && n.width<=keyWidth-2,"oversized note or invalid width");
        ensure(n.left>=300,"left side escapes key");
        ensure(n.left+n.width<=300+keyWidth,"right side escapes key");
        const auto scaled=NoteColumnSizing::Fit3D(300,keyWidth,black,setting);
        ensure(scaled.left==n.left && scaled.width==n.width,"unstable column");
        if(setting==108){
          ensure(n.width<keyWidth,"108% setting still exceeds key");
          ensure(n.width<=int(keyWidth*.80)+1,"column insufficiently slender");
        }
      }
    }
  }
  const auto small=NoteColumnSizing::Fit3D(10,3,true,108);
  ensure(small.width==3 && small.left==10,"tiny key handling");
  std::cout<<"OK: 3D columns stay centered and narrower than every black/white key";
}
