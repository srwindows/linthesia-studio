// Linthesia — comprobación de firmas MIDI, posiciones musicales y tempo.
#include "libmidi/Midi.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
static void raw(std::vector<unsigned char>& o,unsigned long n){
  unsigned char b[5];int i=0;b[i++]=n&0x7f;
  while((n>>=7)!=0)b[i++]=0x80|(n&0x7f);
  while(i)o.push_back(b[--i]);
}
static void event(std::vector<unsigned char>& s,unsigned long delay,
                  std::initializer_list<unsigned char> e){
 raw(s,delay);s.insert(s.end(),e.begin(),e.end());
}
static void big(std::vector<unsigned char>& s,unsigned int value,int bytes){
 for(int n=bytes-1;n>=0;--n)s.push_back((value>>(8*n))&255);
}
int main(){
 std::vector<unsigned char> song;
 song.insert(song.end(),{'M','T','h','d'});big(song,6,4);
 big(song,0,2);big(song,1,2);big(song,480,2);
 std::vector<unsigned char> tr;
 event(tr,0,{0xff,0x58,4,3,2,24,8});   // 3/4
 event(tr,0,{0xff,0x51,3,7,0xa1,0x20}); // 120 BPM
 event(tr,0,{0x90,60,99});
 event(tr,1440,{0x80,60,0});
 event(tr,0,{0x90,64,80});
 event(tr,1440,{0xff,0x58,4,5,3,24,8});  // 5/8
 event(tr,0,{0xff,0x51,3,10,44,43});   // 90 BPM
 event(tr,1200,{0xff,0x58,4,2,1,24,8}); // 2/2
 event(tr,0,{0xff,0x51,3,6,26,128});  // approx 145 BPM
 event(tr,3840,{0x80,64,0});
 event(tr,0,{0xff,0x2f,0});
 song.insert(song.end(),{'M','T','r','k'});big(song,tr.size(),4);
 song.insert(song.end(),tr.begin(),tr.end());
 const char* file="/tmp/linthesia-professional-meters.mid";
 std::ofstream output(file,std::ios::binary);
 output.write(reinterpret_cast<const char*>(song.data()),song.size());
 output.close();
 Midi m=Midi::ReadFromFile(file);
 m.Reset(0,0);
 const auto& bars=m.GetMusicalBars();
 assert(bars.size()>4);
 assert(bars[0].pulse==0 && bars[0].number==1 && bars[0].numerator==3
        && bars[0].denominator==4);
 assert(bars[1].pulse==1440 && bars[1].numerator==3);
 assert(bars[2].pulse==2880 && bars[2].numerator==5 && bars[2].denominator==8);
 assert(bars[3].pulse==4080 && bars[3].numerator==2 && bars[3].denominator==2);
 assert(m.GetMusicalPosition(bars[2].time)=="3:1");
 assert(m.GetMusicalPosition(bars[3].time)=="4:1");
 m.GoTo(400000);
 assert(m.GetCurrentTimeSignature()=="3/4" && m.GetCurrentBPM()==120);
 m.GoTo(bars[2].time);
 assert(m.GetCurrentTimeSignature()=="5/8" && m.GetCurrentBPM()==90);
 m.GoTo(bars[3].time);
 assert(m.GetCurrentTimeSignature()=="2/2");
 std::cout<<"OK compases MIDI 3/4,5/8,2/2; 120→90 BPM; escala y posición reales\n";
}
