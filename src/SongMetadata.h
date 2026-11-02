// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_SONG_METADATA_H
#define LINTHESIA_SONG_METADATA_H
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <string>
namespace SongMetadata {
struct Entry {
 std::string title,subtitle,composer,arranger,copyright,license,tags;
 int difficulty=0,rating=0;
};
inline std::string Identity(const std::string& path){
 struct stat st{};
 if(path.empty()||path[0]!='/'||path.size()>4096 ||
    lstat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode) ||
    st.st_size>128LL*1024*1024)return {};
 struct Cached {off_t size=0;time_t modified=0;std::string sha;};
 static std::map<std::string,Cached> cache;
 auto found=cache.find(path);
 if(found!=cache.end()&&found->second.size==st.st_size&&
    found->second.modified==st.st_mtime)return found->second.sha;
 FILE* file=fopen(path.c_str(),"rb");
 if(!file)return {};
 GChecksum* sum=g_checksum_new(G_CHECKSUM_SHA256);
 std::array<unsigned char,8192> buffer{};
 size_t count=0;
 while((count=fread(buffer.data(),1,buffer.size(),file))>0)
  g_checksum_update(sum,buffer.data(),count);
 const bool ok=!ferror(file);
 fclose(file);
 const std::string hash=ok?g_checksum_get_string(sum):"";
 g_checksum_free(sum);
 cache[path]={st.st_size,st.st_mtime,hash};
 return hash;
}
inline std::string Filename(const std::string& path){
 const std::string id=Identity(path);
 return id.empty()?"":std::string(g_get_user_data_dir())+
         "/linthesia/metadata/"+id+".ini";
}
inline Entry Load(const std::string& path){
 Entry e;
 const auto fn=Filename(path);
 if(fn.empty())return e;
 GKeyFile* f=g_key_file_new();
 if(g_key_file_load_from_file(f,fn.c_str(),G_KEY_FILE_NONE,nullptr)){
  const auto text=[&](const char* key)->std::string{
   gchar* value=g_key_file_get_string(f,"Song",key,nullptr);
   const std::string out=value?value:"";
   g_free(value);
   return out.size()<257&&g_utf8_validate(out.c_str(),-1,nullptr)?
          out:std::string();
  };
  e.title=text("title");e.subtitle=text("subtitle");
  e.composer=text("composer");e.arranger=text("arranger");
  e.copyright=text("copyright");e.license=text("license");
  e.tags=text("tags");
  e.difficulty=std::max(0,std::min(5,g_key_file_get_integer(f,"Song","difficulty",nullptr)));
  e.rating=std::max(0,std::min(5,g_key_file_get_integer(f,"Song","rating",nullptr)));
 }
 g_key_file_free(f);return e;
}
inline bool Save(const std::string& path,const Entry& e){
 const auto fn=Filename(path);
 if(fn.empty())return false;
 const auto slash=fn.find_last_of('/');
 if(g_mkdir_with_parents(fn.substr(0,slash).c_str(),0700)!=0)return false;
 GKeyFile* f=g_key_file_new();
 const std::pair<const char*,std::string> fields[]={
  {"title",e.title},{"subtitle",e.subtitle},{"composer",e.composer},
  {"arranger",e.arranger},{"copyright",e.copyright},
  {"license",e.license},{"tags",e.tags}};
 for(const auto& item:fields){
  if(item.second.size()>256||!g_utf8_validate(item.second.c_str(),-1,nullptr)){
   g_key_file_free(f);return false;
  }
  g_key_file_set_string(f,"Song",item.first,item.second.c_str());
 }
 g_key_file_set_integer(f,"Song","difficulty",
                          std::max(0,std::min(5,e.difficulty)));
 g_key_file_set_integer(f,"Song","rating",std::max(0,std::min(5,e.rating)));
 gsize len=0;gchar* bytes=g_key_file_to_data(f,&len,nullptr);
 GError* error=nullptr;
 const bool ok=bytes&&g_file_set_contents(fn.c_str(),bytes,len,&error);
 if(error)g_error_free(error);
 g_free(bytes);g_key_file_free(f);
 return ok;
}
}
#endif
