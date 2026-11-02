#include "MidiSearchIndex.h"
#include "SongMetadata.h"
#include "FolderPlacements.h"
#include <glib.h>
#include <algorithm>
#include <set>
#include <utility>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <dirent.h>

namespace {
bool isMidi(const std::string& name) {
 const auto dot=name.find_last_of('.');
 if(dot==std::string::npos)return false;
 const auto ext=name.substr(dot);
 return g_ascii_strcasecmp(ext.c_str(),".mid")==0 ||
        g_ascii_strcasecmp(ext.c_str(),".midi")==0 ||
           g_ascii_strcasecmp(ext.c_str(),".rmi")==0 ||
           g_ascii_strcasecmp(ext.c_str(),".kar")==0 ||
           g_ascii_strcasecmp(ext.c_str(),".musicxml")==0 ||
           g_ascii_strcasecmp(ext.c_str(),".mxl")==0 ||
           g_ascii_strcasecmp(ext.c_str(),".xml")==0;
}
bool plainDir(const std::string& path) {
 struct stat st{};
 return lstat(path.c_str(),&st)==0 && S_ISDIR(st.st_mode);
}
bool plainFile(const std::string& path) {
 struct stat st{};
 return lstat(path.c_str(),&st)==0 && S_ISREG(st.st_mode);
}
std::string fold(const std::string& text){
 if(!g_utf8_validate(text.c_str(),-1,nullptr))return g_ascii_strdown(text.c_str(),-1);
 gchar* s=g_utf8_casefold(text.c_str(),-1);
 std::string result(s?s:"");g_free(s);return result;
}
std::string label(const std::string& path){
 return path.substr(path.find_last_of('/')+1);
}
}
std::vector<MidiSearchHit> MidiSearchIndex::Search(
 const std::string& originalRoot,const std::string& personalRoot,
 const FolderPlacements& placements,const std::string& query){
 std::vector<MidiSearchHit> hits;
 if(query.empty()||!g_utf8_validate(query.c_str(),-1,nullptr))return hits;
 const std::string wanted=fold(query);
 // Traverse the *visible* tree, not arbitrary machine files. Skip symlinks,
 // hidden trash/storage and virtual subfolders at their former parents.
 std::set<std::string> visited;
 std::vector<std::pair<std::string,std::string>> pending;
 pending.emplace_back(originalRoot,"");
 pending.emplace_back(personalRoot,"");
 while(!pending.empty()){
  const auto entry=pending.back();pending.pop_back();
  const auto& current=entry.first;
  if(!plainDir(current)||!visited.insert(current).second)continue;
  DIR* dir=opendir(current.c_str());
  if(!dir)continue;
  std::vector<std::string> children;
  struct dirent* item;
  while((item=readdir(dir))){
   const std::string name=item->d_name;
   if(name.empty()||name[0]=='.')continue;
   const std::string path=current+"/"+name;
   if(plainDir(path)){
    if(placements.Parent(path).empty())children.push_back(path);
   }else if(plainFile(path)&&isMidi(name)){
    const auto meta=SongMetadata::Load(path);
    const auto searchable=fold(name+" "+meta.title+" "+meta.subtitle+
                          " "+meta.composer+" "+meta.arranger+" "+meta.tags);
    if(searchable.find(wanted)!=std::string::npos){
      const std::string route=entry.second.empty()?"/":entry.second;
      hits.push_back({meta.title.empty()?name:meta.title,
                      path,current,route});
    }
   }
  }
  closedir(dir);
  for(const auto& virtualChild:placements.Children(current))
   if(plainDir(virtualChild))children.push_back(virtualChild);
  std::sort(children.begin(),children.end());
  children.erase(std::unique(children.begin(),children.end()),children.end());
  for(auto it=children.rbegin();it!=children.rend();++it)
   pending.emplace_back(*it,entry.second+"/"+label(*it));
 }
 std::stable_sort(hits.begin(),hits.end(),[](const MidiSearchHit& a,
                                              const MidiSearchHit& b){
  const std::string left=fold(a.name),right=fold(b.name);
  if(left!=right)return left<right;
  if(a.visibleRoute!=b.visibleRoute)return a.visibleRoute<b.visibleRoute;
  return a.filePath<b.filePath;
 });
 return hits;
}
