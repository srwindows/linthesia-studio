// SPDX-License-Identifier: GPL-2.0-or-later
#include "PracticeHistory.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <algorithm>
#include <climits>
namespace {
bool isSong(const std::string &song){
 if(song.empty()||song.size()>4096||song[0]!='/')return false;
 struct stat st{};
 return lstat(song.c_str(),&st)==0 && S_ISREG(st.st_mode);
}
std::string path(const std::string& song){
 gchar* digest=g_compute_checksum_for_string(G_CHECKSUM_SHA256,
                           song.c_str(),-1);
 const std::string name=digest?digest:"";
 g_free(digest);
 return std::string(g_get_user_data_dir())+
        "/linthesia/practica/"+name+".ini";
}
int number(GKeyFile* data,const char* group,const char* key,int def=0){
 GError* error=nullptr;
 auto value=g_key_file_get_integer(data,group,key,&error);
 if(error){g_error_free(error);return def;}
 return value;
}
std::vector<PracticeHistory::Attempt> get(const std::string& song){
 std::vector<PracticeHistory::Attempt> attempts;
 GKeyFile* file=g_key_file_new();
 if(!g_key_file_load_from_file(file,path(song).c_str(),
                             G_KEY_FILE_NONE,nullptr)){
  g_key_file_free(file);return attempts;
 }
 gchar* saved=g_key_file_get_string(file,"meta","song",nullptr);
 const bool matches=saved && song==saved;
 g_free(saved);
 if(!matches){g_key_file_free(file);return attempts;}
 for(int i=0;i<50;++i){
   const auto section=std::string("attempt_")+std::to_string(i);
   if(!g_key_file_has_group(file,section.c_str()))break;
   PracticeHistory::Attempt a;
   a.timestamp=g_key_file_get_int64(file,section.c_str(),"time",nullptr);
   a.hit=std::max(0,number(file,section.c_str(),"hit"));
   a.expected=std::max(0,number(file,section.c_str(),"expected"));
   a.errors=std::max(0,number(file,section.c_str(),"errors"));
   a.bpm=std::max(0,number(file,section.c_str(),"bpm"));
   a.points=std::max(0,number(file,section.c_str(),"points"));
   a.hands=std::max(0,std::min(2,number(file,section.c_str(),"hands")));
   a.complete=number(file,section.c_str(),"complete")!=0;
   if(a.expected>0 && a.hit<=a.expected && a.timestamp>0)
     attempts.push_back(a);
 }
 g_key_file_free(file);
 return attempts;
}
}
namespace PracticeHistory{
std::string Filename(const std::string& song){
 return isSong(song)?path(song):"";
}
std::vector<Attempt> Load(const std::string &song){
 return isSong(song)?get(song):std::vector<Attempt>();
}
bool Append(const std::string &song,Attempt attempt){
 if(!isSong(song)||attempt.expected<=0||attempt.hit<0||
    attempt.hit>attempt.expected||attempt.errors<0)return false;
 if(attempt.timestamp<=0)attempt.timestamp=g_get_real_time();
 auto items=get(song);
 items.insert(items.begin(),attempt);
 if(items.size()>50)items.resize(50);
 GKeyFile* file=g_key_file_new();
 g_key_file_set_string(file,"meta","song",song.c_str());
 for(size_t i=0;i<items.size();++i){
   const std::string group="attempt_"+std::to_string(i);
   const auto& a=items[i];
   g_key_file_set_int64(file,group.c_str(),"time",a.timestamp);
   g_key_file_set_integer(file,group.c_str(),"hit",a.hit);
   g_key_file_set_integer(file,group.c_str(),"expected",a.expected);
   g_key_file_set_integer(file,group.c_str(),"errors",a.errors);
   g_key_file_set_integer(file,group.c_str(),"bpm",a.bpm);
   g_key_file_set_integer(file,group.c_str(),"points",a.points);
   g_key_file_set_integer(file,group.c_str(),"hands",a.hands);
   g_key_file_set_integer(file,group.c_str(),"complete",a.complete?1:0);
 }
 gsize len=0;gchar* bytes=g_key_file_to_data(file,&len,nullptr);
 g_key_file_free(file);
 const std::string directory=std::string(g_get_user_data_dir())+
                                  "/linthesia/practica";
 if(g_mkdir_with_parents(directory.c_str(),0700)!=0){
  g_free(bytes);return false;
 }
 GError* error=nullptr;
 const bool ok=bytes && g_file_set_contents(path(song).c_str(),bytes,len,&error);
 if(error){g_warning("Linthesia: historial: %s",error->message);g_error_free(error);}
 g_free(bytes);
 return ok;
}
}
