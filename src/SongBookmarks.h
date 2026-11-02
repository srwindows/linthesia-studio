// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_SONG_BOOKMARKS_H
#define LINTHESIA_SONG_BOOKMARKS_H
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <glib/gstdio.h>
#include <string>
#include <vector>
namespace SongBookmarks {
struct Mark {long long time=0;std::string name;};
inline std::string Filename(const std::string& song){
 if(song.empty()||song.size()>4096||song[0]!='/')return {};
 gchar* digest=g_compute_checksum_for_string(G_CHECKSUM_SHA256,song.c_str(),-1);
 const std::string filename=std::string(g_get_user_data_dir())+
       "/linthesia/bookmarks/"+(digest?digest:"")+".ini";
 g_free(digest);return filename;
}
inline std::vector<Mark> Load(const std::string& song){
 std::vector<Mark> marks;
 const auto fn=Filename(song);
 if(fn.empty())return marks;
 GKeyFile* file=g_key_file_new();
 if(g_key_file_load_from_file(file,fn.c_str(),G_KEY_FILE_NONE,nullptr)){
  gchar* path=g_key_file_get_string(file,"meta","song",nullptr);
  if(path && song==path){
   for(int i=0;i<64;++i){
    const auto g="mark_"+std::to_string(i);
    if(!g_key_file_has_group(file,g.c_str()))break;
    const auto time=g_key_file_get_int64(file,g.c_str(),"time",nullptr);
    gchar* label=g_key_file_get_string(file,g.c_str(),"name",nullptr);
    if(time>=0 && time<24LL*3600*1000000 &&
       label && g_utf8_validate(label,-1,nullptr) && strlen(label)<48)
      marks.push_back({time,label});
    g_free(label);
   }
  }
  g_free(path);
 }
 g_key_file_free(file);
 std::sort(marks.begin(),marks.end(),[](auto&a,auto&b){return a.time<b.time;});
 return marks;
}
inline bool Save(const std::string& song,const std::vector<Mark>& marks){
 const auto fn=Filename(song);
 if(fn.empty()||marks.size()>64)return false;
 const auto slash=fn.find_last_of('/');
 if(g_mkdir_with_parents(fn.substr(0,slash).c_str(),0700)!=0)return false;
 GKeyFile* f=g_key_file_new();
 g_key_file_set_string(f,"meta","song",song.c_str());
 for(size_t i=0;i<marks.size();++i){
  const auto group="mark_"+std::to_string(i);
  g_key_file_set_int64(f,group.c_str(),"time",marks[i].time);
  g_key_file_set_string(f,group.c_str(),"name",marks[i].name.c_str());
 }
 gsize size=0;gchar* bytes=g_key_file_to_data(f,&size,nullptr);
 GError* e=nullptr;
 const bool ok=bytes&&g_file_set_contents(fn.c_str(),bytes,size,&e);
 if(e)g_error_free(e);
 g_free(bytes);g_key_file_free(f);return ok;
}
inline bool Add(std::vector<Mark>&marks,long long time,
                const std::string& label){
 if(time<0||time>24LL*3600*1000000||marks.size()>=64||
    label.empty()||label.size()>40||!g_utf8_validate(label.c_str(),-1,nullptr))
  return false;
 for(const auto& m:marks)if(std::llabs(m.time-time)<200000)return false;
 marks.push_back({time,label});
 std::sort(marks.begin(),marks.end(),[](auto&a,auto&b){return a.time<b.time;});
 return true;
}
inline bool RemoveNearest(std::vector<Mark>&marks,long long at,long long radius=1000000){
 auto it=std::min_element(marks.begin(),marks.end(),[at](auto&a,auto&b){
   return std::llabs(a.time-at)<std::llabs(b.time-at);
 });
 if(it==marks.end()||std::llabs(it->time-at)>radius)return false;
 marks.erase(it);return true;
}
inline long long Next(const std::vector<Mark>& marks,long long at){
 if(marks.empty())return -1;
 auto it=std::find_if(marks.begin(),marks.end(),[at](const Mark& m){
  return m.time>at+100000;
 });
 return it==marks.end()?marks.front().time:it->time;
}
}
#endif
