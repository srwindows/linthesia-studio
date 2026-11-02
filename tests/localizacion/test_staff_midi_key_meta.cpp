// SPDX-License-Identifier: GPL-2.0-or-later
#include "libmidi/Midi.h"
#include <sstream>
#include <cassert>
#include <iostream>
#include <string>
int main(){
 // Type-0 file, D major +2 sharps, then F major -1 flat at tick 480.
 // Tempo 120 BPM and an 8-quarter-note sustained C4.
 const unsigned char midi[]={
  'M','T','h','d',0,0,0,6,0,0,0,1,1,0xE0,
  'M','T','r','k',0,0,0,0x24,
  0,0xff,0x59,2,2,0, // D major
  0,0x90,60,100,
  0x83,0x60,0xff,0x59,2,0xff,0, // F major (tick 480)
  0x9e,0,0x80,60,0,
  0,0xff,0x2f,0
 };
 // Use computed track length rather than accidentally hand-counting.
 std::string input(reinterpret_cast<const char*>(midi),sizeof(midi));
 const unsigned trackBytes=unsigned(input.size()-22);
 input[18]=char((trackBytes>>24)&255);
 input[19]=char((trackBytes>>16)&255);
 input[20]=char((trackBytes>>8)&255);
 input[21]=char(trackBytes&255);
 std::istringstream ss(input,std::ios::binary);
 Midi song=Midi::ReadFromStream(ss);
 song.Reset(0,0);
 assert(song.GetCurrentKeyAccidentals()==2);
 assert(song.GetCurrentKeySignature()=="Re mayor");
 song.GoTo(700000);
 assert(song.GetCurrentKeyAccidentals()==-1);
 assert(song.GetCurrentKeySignature()=="Fa mayor");
 std::cout<<"OK: MIDI key signature meta events, signed sharps and flats, temporal changes\n";
}
