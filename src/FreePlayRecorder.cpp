// SPDX-License-Identifier: GPL-2.0-or-later
#include "FreePlayRecorder.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <algorithm>
#include <array>
#include <limits>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>
namespace {
void put16(std::vector<uint8_t>& bytes,uint32_t n){
 bytes.push_back((n>>8)&255);bytes.push_back(n&255);
}
void put32(std::vector<uint8_t>& bytes,uint32_t n){
 bytes.push_back((n>>24)&255);bytes.push_back((n>>16)&255);
 bytes.push_back((n>>8)&255);bytes.push_back(n&255);
}
void vlq(std::vector<uint8_t>& bytes,uint32_t value){
 uint8_t b[5];int i=0;b[i++]=value&127;
 while((value>>=7)&&i<5)b[i++]=uint8_t((value&127)|128);
 while(i)bytes.push_back(b[--i]);
}
uint32_t noteTick(uint64_t time,int bpm,bool quant){
 const uint64_t tick=(time*uint64_t(bpm)*960+30000000)/60000000;
 uint64_t t=tick;
 if(quant)t=((t+120)/240)*240; // nearest sixteenth, 960 PPQN
 return uint32_t(std::min<uint64_t>(t,0x0ffffff0));
}
void chunk(std::vector<uint8_t>& out,std::vector<FreePlay::Event> events,
           int bpm,bool quant,int channel,int program){
 std::vector<std::pair<uint32_t,std::vector<uint8_t>>> rows;
 rows.emplace_back(0,std::vector<uint8_t>{uint8_t(0xC0|channel),uint8_t(program)});
 for(auto& e:events){
  const int hi=e.status&0xF0;
  auto b=std::vector<uint8_t>{
     uint8_t(hi|channel),e.data1};
  if(hi!=0xC0 && hi!=0xD0)b.push_back(e.data2);
  rows.emplace_back(noteTick(e.microseconds,bpm,quant),std::move(b));
 }
 std::stable_sort(rows.begin(),rows.end(),[](const auto&a,const auto&b){
   if(a.first!=b.first)return a.first<b.first;
   const auto pa=a.second[0]&0xF0,pb=b.second[0]&0xF0;
   if(pa==0x80 && pb!=0x80)return true;
   if(pb==0x80 && pa!=0x80)return false;
   return false;
 });
 std::vector<uint8_t> data;
 uint32_t previous=0;
 for(const auto& row:rows){
  vlq(data,row.first-previous);
  data.insert(data.end(),row.second.begin(),row.second.end());
  previous=row.first;
 }
 data.insert(data.end(),{0,0xff,0x2f,0});
 out.insert(out.end(),{'M','T','r','k'});
 put32(out,static_cast<uint32_t>(data.size()));
 out.insert(out.end(),data.begin(),data.end());
}
bool writeAtomic(const std::string& path,const std::vector<uint8_t>& bytes){
 if(bytes.empty()||bytes.size()>64*1024*1024)return false;
 GError* error=nullptr;
 const bool ok=g_file_set_contents(path.c_str(),
          reinterpret_cast<const char*>(bytes.data()),bytes.size(),&error);
 if(error){g_warning("Linthesia: grabador: %s",error->message);g_error_free(error);}
 return ok;
}
}
namespace FreePlay {
bool Recorder::ValidEvent(uint8_t status,uint8_t d1,uint8_t d2){
 const int hi=status&0xF0;
 return (status>=0x80 && status<=0xEF) &&
        d1<128 && d2<128 &&
        (hi==0x80||hi==0x90||hi==0xA0||hi==0xB0||
         hi==0xC0||hi==0xD0||hi==0xE0);
}
bool Recorder::Start(){
 if(recording||tracks.size()>=MaxTracks)return false;
 recording=true;
 activeTrack=static_cast<int>(tracks.size());
 tracks.emplace_back();
 return true;
}
void Recorder::Push(uint64_t time,uint8_t status,uint8_t d1,uint8_t d2){
 if(!recording||activeTrack<0||size_t(activeTrack)>=tracks.size())return;
 if(!ValidEvent(status,d1,d2))return;
 auto& v=tracks[activeTrack].events;
 if(v.size()>=100000)return;
 v.push_back({std::min<uint64_t>(time,24ULL*3600*1000000),status,d1,d2});
}
void Recorder::Stop(uint64_t time){
 if(!recording)return;
 auto& t=tracks.at(size_t(activeTrack));
 std::array<std::array<int,128>,16> held{};
 for(auto& e:t.events){
  const int ch=e.status&0xf,kind=e.status&0xf0;
  if(kind==0x90&&e.data2)++held[ch][e.data1];
  else if(kind==0x80||(kind==0x90&&!e.data2))
    held[ch][e.data1]=std::max(0,held[ch][e.data1]-1);
 }
 for(int ch=0;ch<16;++ch)for(int n=0;n<128;++n)
  if(held[ch][n])t.events.push_back({time,uint8_t(0x80|ch),uint8_t(n),0});
 if(t.events.empty())tracks.pop_back();
 activeTrack=-1;recording=false;
}
bool Recorder::Delete(size_t index){
 if(recording||index>=tracks.size())return false;
 tracks.erase(tracks.begin()+index);return true;
}
bool Recorder::SetProgram(size_t index,int program){
 if(recording||index>=tracks.size()||program<0||program>127)return false;
 tracks[index].program=uint8_t(program);return true;
}
std::vector<uint8_t> Recorder::ToMidi() const{
 std::vector<uint8_t> out{'M','T','h','d'};
 put32(out,6);put16(out,1);put16(out,uint16_t(tracks.size()+1));put16(out,960);
 std::vector<uint8_t> tempo{'M','T','r','k'};
 std::vector<uint8_t> content{0,0xff,0x51,3};
 const int usec=60000000/std::max(20,std::min(400,bpm));
 content.push_back((usec>>16)&255);
 content.push_back((usec>>8)&255);
 content.push_back(usec&255);
 content.insert(content.end(),{0,0xff,0x58,4,4,2,24,8,0,0xff,0x2f,0});
 put32(tempo,uint32_t(content.size()));
 tempo.insert(tempo.end(),content.begin(),content.end());
 out.insert(out.end(),tempo.begin(),tempo.end());
 for(size_t i=0;i<tracks.size()&&i<MaxTracks;++i)
  chunk(out,tracks[i].events,bpm,quantize,int(i),tracks[i].program);
 return out;
}
bool Recorder::Save(const std::string& path)const{
 if(tracks.empty())return false;
 return writeAtomic(path,ToMidi());
}
bool Recorder::SaveSession(const std::string& path)const{
 if(tracks.size()>MaxTracks)return false;
 std::vector<uint8_t> data{'L','I','N','T','H','F','R','1'};
 put32(data,uint32_t(std::max(20,std::min(400,bpm))));
 put32(data,quantize?1:0);
 put32(data,uint32_t(tracks.size()));
 for(const auto& track:tracks){
  if(track.events.size()>100000)return false;
  put32(data,track.program);put32(data,track.muted?1:0);
  put32(data,uint32_t(track.events.size()));
  for(auto& event:track.events){
   uint64_t t=event.microseconds;
   for(int i=7;i>=0;--i)data.push_back(uint8_t(t>>(8*i)));
   data.insert(data.end(),{event.status,event.data1,event.data2});
  }
 }
 return writeAtomic(path,data);
}
bool Recorder::Restore(const std::string& path){
 struct stat file{};
 if(g_stat(path.c_str(),&file)!=0 || !S_ISREG(file.st_mode) ||
    file.st_size<20 || file.st_size>12000000)return false;
 gchar* raw=nullptr;gsize length=0;GError* error=nullptr;
 if(!g_file_get_contents(path.c_str(),&raw,&length,&error)){
  if(error)g_error_free(error);
  return false;
 }
 std::vector<uint8_t> data(raw,raw+length);
 g_free(raw);
 if(length<20||length>12000000||!std::equal(data.begin(),data.begin()+8,"LINTHFR1"))
  return false;
 size_t pos=8;
 const auto take32=[&]() -> uint32_t {
  if(pos+4>data.size())throw 1;
  uint32_t v=0;
  for(int i=0;i<4;++i)v=(v<<8)|data[pos++];
  return v;
 };
 try{
  Recorder fresh;
  fresh.bpm=int(take32());
  fresh.quantize=take32()!=0;
  const auto n=take32();
  if(n>MaxTracks||fresh.bpm<20||fresh.bpm>400)return false;
  for(uint32_t i=0;i<n;++i){
   Track track;
   const auto program=take32(),mute=take32(),count=take32();
   if(program>127||mute>1||count>100000||pos+uint64_t(count)*11>data.size())
     return false;
   track.program=uint8_t(program);
   track.muted=mute!=0;
   for(uint32_t k=0;k<count;++k){
    Event ev;
    for(int b=0;b<8;++b)ev.microseconds=(ev.microseconds<<8)|data[pos++];
    ev.status=data[pos++];ev.data1=data[pos++];ev.data2=data[pos++];
    if(!ValidEvent(ev.status,ev.data1,ev.data2)||
       ev.microseconds>24ULL*3600*1000000)return false;
    track.events.push_back(ev);
   }
   fresh.tracks.push_back(std::move(track));
  }
  if(pos!=data.size())return false;
  *this=std::move(fresh);
  return true;
 }catch(...){return false;}
}
}
