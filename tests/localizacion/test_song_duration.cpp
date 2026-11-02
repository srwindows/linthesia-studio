#include "libmidi/Midi.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

static void require(bool ok,const char* message){
  if(!ok){std::cerr<<message<<"\n";std::exit(1);}
}
static void vlq(std::string& bytes,unsigned value){
  unsigned char encoded[4];unsigned n=0;
  encoded[n++]=static_cast<unsigned char>(value&127u);
  while((value>>=7)!=0)encoded[n++]=static_cast<unsigned char>((value&127u)|128u);
  while(n)bytes.push_back(static_cast<char>(encoded[--n]));
}
static void event(std::string& track,unsigned delta,unsigned status,unsigned note){
  vlq(track,delta);
  track.push_back(static_cast<char>(status));
  track.push_back(static_cast<char>(note));
  track.push_back(static_cast<char>(status==0x90?100:0));
}
static Midi readTrack(std::string track){
  track.append("\0\xff\x2f\0",4);
  const unsigned char header[]={0x4d,0x54,0x68,0x64,0,0,0,6,0,0,0,1,1,0xe0,
                                0x4d,0x54,0x72,0x6b};
  std::string data(reinterpret_cast<const char*>(header),sizeof header);
  const unsigned size=static_cast<unsigned>(track.size());
  for(int shift=24;shift>=0;shift-=8)data.push_back(static_cast<char>((size>>shift)&255u));
  data+=track;
  std::istringstream stream(data,std::ios::in|std::ios::binary);
  return Midi::ReadFromStream(stream);
}
int main(){
  // 480 PPQ at 120 BPM = 960 ticks/second. The first note lasts 60s,
  // while a later note starts at 26s and ends at 27.75s.
  std::string track;
  event(track,0,0x90,50);
  event(track,24960,0x90,62);
  event(track,1680,0x80,62);
  event(track,30960,0x80,50);
  Midi song=readTrack(track);
  song.Reset(0,0);
  require(song.Notes().size()==2,"fixture did not parse its two notes");
  require(song.Notes().rbegin()->end==27750000,"fixture lacks staggered note endings");
  const auto start=song.GetDeadAirStartOffsetMicroseconds();
  const auto length=song.GetSongLengthInMicroseconds();
  require(start+length==60000000,"song duration ignored an earlier-starting long note");
  require(length>=60000000&&length<=60000001,"incorrect 60-second playable duration");
  song.GoTo(27750000);
  require(!song.IsSongOver(),"later short NoteOff stopped the still-held long note");
  song.GoTo(30000000);
  require(song.GetSongPercentageComplete()>.4999&&song.GetSongPercentageComplete()<.5001,
          "progress used the short note instead of the final NoteOff");
  song.GoTo(59999999);
  require(!song.IsSongOver(),"song ended before the long NoteOff");
  song.GoTo(60000000);
  require(song.IsSongOver(),"song did not end at the final NoteOff");
  // A valid metadata-only track has no note from which to take rbegin().
  Midi silent=readTrack(std::string());
  silent.Reset(0,0);
  require(silent.Notes().empty(),"metadata-only track acquired a note");
  require(silent.GetSongLengthInMicroseconds()==0,"empty song has a spurious duration");
  require(silent.IsSongOver(),"empty song cannot complete");
  std::cout<<"OK: latest NoteOff defines song duration; 60s hold survives later short notes; empty MIDI safe\n";
}
