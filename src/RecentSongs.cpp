// Licencia: GNU GPL versión 2, conforme al archivo COPYING de Linthesia.
// Linthesia: registro local de canciones ejecutadas. No se tocan las pistas.
#include "RecentSongs.h"
#include <glib.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstdlib>
#include <climits>
#include <limits.h>
#include <stdlib.h>
#include <vector>
namespace {
bool valid(const std::string& path){
 if(path.empty()||path[0]!='/'||path.size()>4096)return false;
 struct stat st{};
 if(lstat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode))return false;
 const auto dot=path.find_last_of('.');
 if(dot==std::string::npos)return false;
 const std::string suffix=path.substr(dot);
 return g_ascii_strcasecmp(suffix.c_str(),".mid")==0||
        g_ascii_strcasecmp(suffix.c_str(),".midi")==0;
}
std::vector<RecentSongs::Entry> load(){
 std::vector<RecentSongs::Entry> rows;
 GKeyFile* cfg=g_key_file_new();
 if(!g_key_file_load_from_file(cfg,RecentSongs::FilePath().c_str(),
                               G_KEY_FILE_NONE,nullptr)){
  g_key_file_free(cfg);return rows;
 }
 gsize n=0;
 gchar** groups=g_key_file_get_groups(cfg,&n);
 for(gsize i=0;i<n && i<20000;i++){
  gchar* path=g_key_file_get_string(cfg,groups[i],"path",nullptr);
  if(path && valid(path)){
   gchar* sha=g_compute_checksum_for_string(G_CHECKSUM_SHA256,path,-1);
   const std::string expected=std::string("midi-")+sha;
   g_free(sha);
   if(expected==groups[i]){
    RecentSongs::Entry row;
    row.path=path;
    row.lastPlayed=g_key_file_get_int64(cfg,groups[i],"last",nullptr);
    row.plays=std::max(0,g_key_file_get_integer(cfg,groups[i],"plays",nullptr));
    row.bestPoints=std::max(0,g_key_file_get_integer(cfg,groups[i],"best",nullptr));
    if(row.plays && row.lastPlayed>0)rows.push_back(row);
   }
  }
  g_free(path);
 }
 g_strfreev(groups);
 g_key_file_free(cfg);
 return rows;
}
bool recentFirst(const RecentSongs::Entry& a,const RecentSongs::Entry& b){
 if(a.lastPlayed!=b.lastPlayed)return a.lastPlayed>b.lastPlayed;
 return a.path<b.path;
}
void save(std::vector<RecentSongs::Entry> entries){
 std::sort(entries.begin(),entries.end(),recentFirst);
 if(entries.size()>5000)entries.resize(5000);
 GKeyFile* cfg=g_key_file_new();
 for(const auto& r:entries){
  if(!valid(r.path))continue;
  gchar* sha=g_compute_checksum_for_string(G_CHECKSUM_SHA256,
                                            r.path.c_str(),-1);
  const std::string group=std::string("midi-")+sha;
  g_free(sha);
  g_key_file_set_string(cfg,group.c_str(),"path",r.path.c_str());
  g_key_file_set_int64(cfg,group.c_str(),"last",r.lastPlayed);
  g_key_file_set_integer(cfg,group.c_str(),"plays",
                         std::min(r.plays,static_cast<unsigned int>(INT_MAX)));
  g_key_file_set_integer(cfg,group.c_str(),"best",r.bestPoints);
 }
 gsize len=0;
 gchar* bytes=g_key_file_to_data(cfg,&len,nullptr);
 const std::string dir=std::string(g_get_user_data_dir())+"/linthesia";
 if(g_mkdir_with_parents(dir.c_str(),0700)==0 && bytes){
  GError* error=nullptr;
  if(!g_file_set_contents(RecentSongs::FilePath().c_str(),bytes,len,&error))
   g_warning("Linthesia: no se guardó Recientes: %s",
             error?error->message:"error desconocido");
  if(error)g_error_free(error);
 }
 g_free(bytes);
 g_key_file_free(cfg);
}
}
namespace RecentSongs{
std::string FilePath(){
 return std::string(g_get_user_data_dir())+"/linthesia/recientes.ini";
}
namespace {
std::string lastPathFile(){
 return std::string(g_get_user_data_dir())+"/linthesia/ultima-cancion.ini";
}
}
std::string LastSelectedPath(){
 GKeyFile* file=g_key_file_new();
 std::string selected;
 if(g_key_file_load_from_file(file,lastPathFile().c_str(),
                              G_KEY_FILE_NONE,nullptr)){
   gchar* p=g_key_file_get_string(file,"Sesion","ultimo-midi",nullptr);
   if(p && valid(p))selected=p;
   g_free(p);
 }
 g_key_file_free(file);
 if(!selected.empty())return selected;
 // Primera ejecución tras actualizar: recuperar Recientes por fecha real.
 auto items=load();
 std::sort(items.begin(),items.end(),recentFirst);
 return items.empty()?std::string():items.front().path;
}
void RememberLastSelected(const std::string& path){
 if(path.empty())return;
 char* canonical=realpath(path.c_str(),nullptr);
 if(!canonical)return;
 const std::string resolved(canonical);
 free(canonical);
 if(!valid(resolved))return;
 GKeyFile* file=g_key_file_new();
 g_key_file_set_string(file,"Sesion","ultimo-midi",resolved.c_str());
 gsize length=0;
 gchar* bytes=g_key_file_to_data(file,&length,nullptr);
 const std::string dirname=std::string(g_get_user_data_dir())+"/linthesia";
 if(bytes && g_mkdir_with_parents(dirname.c_str(),0700)==0){
   GError* error=nullptr;
   if(!g_file_set_contents(lastPathFile().c_str(),bytes,length,&error))
     g_warning("Linthesia: error al guardar la última canción: %s",
               error?error->message:"desconocido");
   if(error)g_error_free(error);
 }
 g_free(bytes);
 g_key_file_free(file);
}
void RecordPlay(const std::string& path){
 if(!valid(path))return;
 auto rows=load();
 auto it=std::find_if(rows.begin(),rows.end(),
   [&](const Entry& a){return a.path==path;});
 if(it==rows.end()){
  rows.emplace_back();
  it=rows.end()-1;
  it->path=path;
 }
 if(it->plays<static_cast<unsigned int>(INT_MAX))++it->plays;
 it->lastPlayed=g_get_real_time();
 save(rows);
}
void RecordPoints(const std::string& path,int points){
 if(!valid(path) || points<=0)return;
 auto rows=load();
 const auto it=std::find_if(rows.begin(),rows.end(),
    [&](const Entry& a){return a.path==path;});
 if(it==rows.end()||points<=it->bestPoints)return;
 it->bestPoints=points;
 save(rows);
}
std::vector<Entry> List(int limit,int order){
 auto rows=load();
 std::sort(rows.begin(),rows.end(),recentFirst);
 rows.resize(std::min(rows.size(),static_cast<size_t>(
                 std::max(1,std::min(500,limit)))));
 if(order==Alphabetical)
  std::stable_sort(rows.begin(),rows.end(),[](const Entry& a,const Entry& b){
   const auto name=[](const Entry& e){
     return e.path.substr(e.path.find_last_of('/')+1);
   };
   gchar* af=g_utf8_casefold(name(a).c_str(),-1);
   gchar* bf=g_utf8_casefold(name(b).c_str(),-1);
   const int compare=g_strcmp0(af,bf);
   g_free(af);g_free(bf);
   return compare<0;
  });
 if(order==MostPlayed)
  std::stable_sort(rows.begin(),rows.end(),[](const Entry& a,const Entry& b){
   return a.plays>b.plays;
  });
 if(order==HighestPoints)
  std::stable_sort(rows.begin(),rows.end(),[](const Entry& a,const Entry& b){
   return a.bestPoints>b.bestPoints;
  });
 return rows;
}
}
