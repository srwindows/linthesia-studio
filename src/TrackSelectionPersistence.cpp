// GPL-2.0-or-later. Persist per-MIDI track choices; no MIDI file changes.
#include "TrackSelectionPersistence.h"
#include <glib.h>
#include <algorithm>
#include <string>
namespace TrackSelectionPersistence {
std::string FilePath(const std::string& song){
  if(song.empty()||song[0]!='/'||song.size()>4096)return {};
  gchar* hash=g_compute_checksum_for_string(G_CHECKSUM_SHA256,song.c_str(),-1);
  const std::string file=std::string(g_get_user_data_dir())+
       "/linthesia/pistas/"+hash+".ini";
  g_free(hash);return file;
}
bool Load(const std::string& song,unsigned tracks,
          std::vector<Choice>& choices,int& page){
  const auto path=FilePath(song);
  if(path.empty()||tracks==0||tracks>1024)return false;
  GKeyFile* file=g_key_file_new();
  const bool open=g_key_file_load_from_file(file,path.c_str(),
                                             G_KEY_FILE_NONE,nullptr);
  bool valid=open;
  if(valid){
    gchar* stored=g_key_file_get_string(file,"Song","path",nullptr);
    const int count=g_key_file_get_integer(file,"Song","tracks",nullptr);
    valid=stored && song==stored && count==static_cast<int>(tracks);
    g_free(stored);
  }
  std::vector<Choice> loaded;
  if(valid){
    for(unsigned i=0;i<tracks;++i){
      const std::string group="Track"+std::to_string(i);
      const int mode=g_key_file_get_integer(file,group.c_str(),"mode",nullptr);
      const int color=g_key_file_get_integer(file,group.c_str(),"color",nullptr);
      const int retry=g_key_file_get_integer(file,group.c_str(),"retry",nullptr);
      // Restrict to the versioned Mode and color enums. No arbitrary state.
      if(mode<0||mode>=7||color<0||color>=16||retry<0||retry>1 ||
         !g_key_file_has_key(file,group.c_str(),"mode",nullptr)||
         !g_key_file_has_key(file,group.c_str(),"color",nullptr)||
         !g_key_file_has_key(file,group.c_str(),"retry",nullptr)){
        valid=false;break;
      }
      const int program=g_key_file_has_key(file,group.c_str(),"program",nullptr)?
          g_key_file_get_integer(file,group.c_str(),"program",nullptr):-1;
      const int bank=g_key_file_has_key(file,group.c_str(),"bank",nullptr)?
          g_key_file_get_integer(file,group.c_str(),"bank",nullptr):0;
      if(program < -1 || program>127 || bank<0 || bank>16383){valid=false;break;}
      loaded.push_back({mode,color,retry!=0,program,bank});
    }
  }
  if(valid){
    choices=std::move(loaded);
    page=std::max(0,g_key_file_get_integer(file,"Song","page",nullptr));
  }
  g_key_file_free(file);
  return valid;
}
bool Save(const std::string& song,const std::vector<Choice>& choices,int page){
  const auto path=FilePath(song);
  if(path.empty()||choices.empty()||choices.size()>1024)return false;
  for(const Choice& item:choices)
    if(item.mode<0||item.mode>=7||item.color<0||item.color>=16 ||
       item.program < -1||item.program>127||item.bank<0||item.bank>16383)
      return false;
  GKeyFile* file=g_key_file_new();
  g_key_file_set_integer(file,"Song","version",1);
  g_key_file_set_string(file,"Song","path",song.c_str());
  g_key_file_set_integer(file,"Song","tracks",choices.size());
  g_key_file_set_integer(file,"Song","page",std::max(0,page));
  for(unsigned i=0;i<choices.size();++i){
    const std::string group="Track"+std::to_string(i);
    const auto& c=choices[i];
    g_key_file_set_integer(file,group.c_str(),"mode",c.mode);
    g_key_file_set_integer(file,group.c_str(),"color",c.color);
    g_key_file_set_integer(file,group.c_str(),"retry",c.retry?1:0);
    g_key_file_set_integer(file,group.c_str(),"program",c.program);
    g_key_file_set_integer(file,group.c_str(),"bank",c.bank);
  }
  gsize length=0;
  gchar* data=g_key_file_to_data(file,&length,nullptr);
  const std::string dir=path.substr(0,path.find_last_of('/'));
  bool ok=false;
  if(data&&g_mkdir_with_parents(dir.c_str(),0700)==0){
    GError* error=nullptr;
    ok=g_file_set_contents(path.c_str(),data,length,&error);
    if(!ok)g_warning("Linthesia: no se pudo guardar la seleccion de pistas: %s",
                      error?error->message:"error desconocido");
    if(error)g_error_free(error);
  }
  g_free(data);g_key_file_free(file);
  return ok;
}
}
