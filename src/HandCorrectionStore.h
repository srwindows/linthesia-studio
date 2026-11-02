// SPDX-License-Identifier: GPL-2.0-or-later
// Linthesia original hand annotations; no Synthesia file formats or code.
// Save only chosen hand assignments, never rewrite a musical MIDI file.
#ifndef LINTHESIA_HAND_CORRECTION_STORE_H
#define LINTHESIA_HAND_CORRECTION_STORE_H
#include "HandPractice.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
namespace HandCorrectionStore {
inline std::string File(const std::string& midiPath){
 if(midiPath.empty())return {};
 const char* env=std::getenv("XDG_CONFIG_HOME");
 const char* home=std::getenv("HOME");
 const std::string base=env&&*env?env:
      (home&&*home?std::string(home)+"/.config":std::string());
 if(base.empty())return {};
 // Include size/mtime to prevent annotations belonging to an overwritten
 // song from silently affecting a different song at the same path.
 struct stat st{};
 const bool exists=stat(midiPath.c_str(),&st)==0;
 const std::string identity=midiPath+"|"+
  (exists?std::to_string(static_cast<long long>(st.st_size)):"unavailable")+"|"+
  (exists?std::to_string(static_cast<long long>(st.st_mtime)):"unavailable");
 unsigned long long hash=14695981039346656037ULL;
 for(const auto byte:identity){
  hash^=static_cast<unsigned char>(byte);
  hash*=1099511628211ULL;
 }
 const std::string folder=base+"/linthesia";
 const std::string annotations=folder+"/hand-assignments";
 mkdir(base.c_str(),0700);
 mkdir(folder.c_str(),0700);
 mkdir(annotations.c_str(),0700);
 std::ostringstream out;
 out<<annotations<<"/"<<std::hex<<std::setw(16)<<std::setfill('0')<<hash<<".txt";
 return out.str();
}

// Extended annotation format: H denotes hand correction, F denotes finger.
// Existing LINTHESIA_HAND_ASSIGNMENTS_1 files are read without migration.
// Neither format ever alters the song, channel, program or note velocity.
struct Data {
  std::vector<HandPractice::Correction> hands;
  std::vector<HandPractice::Fingering> fingers;
};
inline Data LoadAll(const std::string& song){
  Data result;
  const auto file=File(song);
  if(file.empty())return result;
  std::ifstream in(file);
  std::string header;
  if(!std::getline(in,header))return result;
  const bool v1=header=="LINTHESIA_HAND_ASSIGNMENTS_1";
  const bool v2=header=="LINTHESIA_HAND_ASSIGNMENTS_2";
  if(!v1&&!v2)return result;
  std::string line;
  for(size_t count=0;count<100000 && std::getline(in,line);++count){
    if(line.size()>128)continue;
    std::istringstream row(line);
    char type='H';
    if(v2 && !(row>>type))continue;
    unsigned long long track;
    int pitch,value;
    long long when;
    if(!(row>>track>>pitch>>when>>value))continue;
    std::string excess;
    if(row>>excess)continue;
    if(track>1024 || pitch<0 || pitch>127 || when<0)continue;
    if(type=='H' && (value==0||value==1))
      result.hands.push_back({static_cast<size_t>(track),pitch,
         static_cast<microseconds_t>(when),static_cast<HandPractice::Hand>(value)});
    else if(type=='F' && value>=1 && value<=5)
      result.fingers.push_back({static_cast<size_t>(track),pitch,
         static_cast<microseconds_t>(when),value});
  }
  return result;
}
inline bool SaveAll(const std::string& song,
                    const std::vector<HandPractice::Correction>& hands,
                    const std::vector<HandPractice::Fingering>& fingers){
  const auto file=File(song);
  if(file.empty()||hands.size()+fingers.size()>100000)return false;
  const auto tmp=file+".tmp."+std::to_string(getpid());
  {
    std::ofstream out(tmp,std::ios::trunc);
    if(!out)return false;
    out<<"LINTHESIA_HAND_ASSIGNMENTS_2\n";
    for(const auto& c:hands)
      out<<"H "<<c.track<<" "<<c.pitch<<" "<<c.when<<" "<<int(c.hand)<<"\n";
    for(const auto& f:fingers)
      out<<"F "<<f.track<<" "<<f.pitch<<" "<<f.when<<" "<<f.finger<<"\n";
    out.flush();
    if(!out){std::remove(tmp.c_str());return false;}
  }
  if(std::rename(tmp.c_str(),file.c_str())!=0){
    std::remove(tmp.c_str());
    return false;
  }
  return true;
}

inline std::vector<HandPractice::Correction> Load(const std::string& song){
  return LoadAll(song).hands;
}
inline std::vector<HandPractice::Fingering> LoadFingers(const std::string& song){
  return LoadAll(song).fingers;
}
inline bool Save(const std::string& song,
                 const std::vector<HandPractice::Correction>& corrections){
 const auto file=File(song);if(file.empty()||corrections.size()>100000)
   return false;
 const auto tmp=file+".tmp."+std::to_string(getpid());
 {
  std::ofstream out(tmp,std::ios::trunc);
  if(!out)return false;
  out<<"LINTHESIA_HAND_ASSIGNMENTS_1\n";
  for(const auto &c:corrections)
   out<<c.track<<" "<<c.pitch<<" "<<c.when<<" "<<int(c.hand)<<"\n";
  out.flush();
  if(!out){std::remove(tmp.c_str());return false;}
 }
 if(std::rename(tmp.c_str(),file.c_str())!=0){
  std::remove(tmp.c_str());return false;
 }
 return true;
}
}
#endif
