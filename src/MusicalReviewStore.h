// GPL-2.0-or-later. Sidecar-only, never modify original MIDI or samples.
#ifndef LINTHESIA_MUSICAL_REVIEW_STORE_H
#define LINTHESIA_MUSICAL_REVIEW_STORE_H
#include "HandCorrectionStore.h"
#include "MusicalReasoner.h"
#include <glib.h>
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
namespace MusicalReviewStore {
inline std::string File(const std::string& midi){
  const auto base=HandCorrectionStore::File(midi);
  return base.empty()?std::string():base+".musical.ini";
}
inline std::vector<MusicalReasoner::Suggestion> Load(const std::string& song){
  std::vector<MusicalReasoner::Suggestion> out;
  const auto path=File(song);
  if(path.empty())return out;
  GKeyFile* f=g_key_file_new();
  if(g_key_file_load_from_file(f,path.c_str(),G_KEY_FILE_NONE,nullptr)){
    const int total=g_key_file_get_integer(f,"Changes","total",nullptr);
    for(int i=0;i<std::min(10000,std::max(0,total));++i){
      const std::string g="Correction"+std::to_string(i);
      if(!g_key_file_has_key(f,g.c_str(),"when",nullptr))continue;
      MusicalReasoner::Suggestion s;
      s.track=g_key_file_get_integer(f,g.c_str(),"track",nullptr);
      s.original=g_key_file_get_integer(f,g.c_str(),"original",nullptr);
      s.replacement=g_key_file_get_integer(f,g.c_str(),"replacement",nullptr);
      s.when=g_key_file_get_int64(f,g.c_str(),"when",nullptr);
      s.reason="Guardada por el usuario";
      if(s.track<1024 && s.original>=0&&s.original<=127 &&
         s.replacement>=-1&&s.replacement<=127 && s.when>=0)
        out.push_back(s);
    }
  }
  g_key_file_free(f);
  return out;
}
inline bool Save(const std::string& song,
              const std::vector<MusicalReasoner::Suggestion>& items){
  const auto path=File(song);
  if(path.empty()||items.size()>10000)return false;
  GKeyFile* f=g_key_file_new();
  g_key_file_set_integer(f,"Changes","total",items.size());
  for(size_t i=0;i<items.size();++i){
    const auto& s=items[i];
    const std::string g="Correction"+std::to_string(i);
    g_key_file_set_integer(f,g.c_str(),"track",s.track);
    g_key_file_set_integer(f,g.c_str(),"original",s.original);
    g_key_file_set_integer(f,g.c_str(),"replacement",s.replacement);
    g_key_file_set_int64(f,g.c_str(),"when",s.when);
  }
  gsize len=0; gchar* data=g_key_file_to_data(f,&len,nullptr);
  const std::string tmp=path+".tmp";
  GError* e=nullptr;
  const bool wrote=data && g_file_set_contents(tmp.c_str(),data,len,&e);
  if(e)g_error_free(e);
  const bool success=wrote && std::rename(tmp.c_str(),path.c_str())==0;
  if(!success)std::remove(tmp.c_str());
  g_free(data);g_key_file_free(f);return success;
}
}
#endif
