// SPDX-License-Identifier: GPL-2.0-or-later
#include "libmidi/Midi.h"
#include <sstream>
#include <cassert>
#include <iostream>
#include <string>
static void le32(std::string& s,unsigned v){
 for(int i=0;i<4;++i)s.push_back(char((v>>(8*i))&255));
}
int main(){
 const unsigned char smf[]={
 'M','T','h','d',0,0,0,6,0,0,0,1,1,0xE0,
 'M','T','r','k',0,0,0,13,
 0,0x90,60,90,0x83,0x60,0x80,60,0,0,0xff,0x2f,0};
 std::string plain(reinterpret_cast<const char*>(smf),sizeof(smf));
 std::string riff="RIFF";
 le32(riff,4+8+unsigned(plain.size()));
 riff+="RMID";riff+="data";le32(riff,unsigned(plain.size()));riff+=plain;
 std::istringstream standard(plain,std::ios::binary),wrapped(riff,std::ios::binary);
 Midi a=Midi::ReadFromStream(standard);
 Midi b=Midi::ReadFromStream(wrapped);
 a.Reset(0,0);b.Reset(0,0);
 assert(a.Notes().size()==1 && b.Notes().size()==1);
 assert(a.Notes().begin()->note_id==60);
 assert(b.Notes().begin()->note_id==60);
 // The first chunk can be arbitrary "JUNK" and must be skipped correctly.
 std::string withChunk="RIFF";
 le32(withChunk,4+8+3+1+8+unsigned(plain.size()));
 withChunk+="RMID";withChunk+="JUNK";le32(withChunk,3);
 withChunk+="abc";withChunk.push_back('\0');
 withChunk+="data";le32(withChunk,unsigned(plain.size()));withChunk+=plain;
 std::istringstream junk(withChunk,std::ios::binary);
 Midi c=Midi::ReadFromStream(junk);c.Reset(0,0);
 assert(c.Notes().size()==1);
 std::cout<<"OK: MIDI 0 & RIFF RMID 64-bit LE with odd padding"<<std::endl;
}
