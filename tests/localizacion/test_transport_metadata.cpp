// Linthesia GPL v2: comprobar tonalidad, compás y tempo al mover el transporte.
#include "libmidi/Midi.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
int main(){
  const unsigned char midi[]={0x4d,0x54,0x68,0x64,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x01,0x01,0xe0,0x4d,0x54,0x72,0x6b,0x00,0x00,0x00,0x2a,0x00,0xff,0x59,0x02,0x02,0x00,0x00,0xff,0x58,0x04,0x03,0x02,0x18,0x08,0x00,0xff,0x51,0x03,0x07,0xa1,0x20,0x00,0x90,0x3c,0x64,0x87,0x40,0xff,0x51,0x03,0x0a,0x2c,0x2b,0x87,0x40,0x80,0x3c,0x00,0x00,0xff,0x2f,0x00};
  char file[]="/tmp/linthesia-meta-test-XXXXXX";
  int fd=mkstemp(file);assert(fd>=0);
  assert(write(fd,midi,sizeof midi)==static_cast<ssize_t>(sizeof midi));
  close(fd);
  Midi m=Midi::ReadFromFile(file);
  unlink(file);
  m.Reset(0,0);
  assert(m.GetCurrentKeySignature()=="Re mayor");
  assert(m.GetCurrentTimeSignature()=="3/4");
  assert(m.GetCurrentBPM()==120);
  const microseconds_t start=m.GetDeadAirStartOffsetMicroseconds();
  const microseconds_t length=m.GetSongLengthInMicroseconds();
  m.GoTo(start+length/4);
  assert(m.GetSongPercentageComplete()>0.24);
  assert(m.GetSongPercentageComplete()<0.26);
  m.GoTo(1400000);
  assert(m.GetCurrentBPM()==90);
  m.GoTo(start+length/2);
  assert(m.GetSongPercentageComplete()>0.49 && m.GetSongPercentageComplete()<0.51);
  std::cout<<"OK: progreso real, tonalidad Re mayor, métrica 3/4 y BPM 120→90\n";
}
