#include "FolderPlacements.h"
#include <glib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <set>
#include <algorithm>
#include <cstring>
#include <limits.h>
#include <dirent.h>

namespace {
std::string canonical(const std::string& p){
 char* q=realpath(p.c_str(),nullptr);
 if(!q)return {};
 std::string s(q);free(q);return s;
}
bool within(const std::string& top,const std::string& p){
 return !top.empty()&&(p==top || (p.size()>top.size()&&
               p.compare(0,top.size(),top)==0&&p[top.size()]=='/'));
}
bool isFolder(const std::string& p){
 struct stat st{};
 return lstat(p.c_str(),&st)==0&&S_ISDIR(st.st_mode);
}
std::string dirnameOf(const std::string& p){
 auto pos=p.find_last_of('/');
 return pos==std::string::npos?"":p.substr(0,pos);
}
std::string digest(const std::string& p){
 gchar* h=g_compute_checksum_for_string(G_CHECKSUM_SHA256,p.c_str(),-1);
 std::string result(h);g_free(h);return result;
}
}
FolderPlacements::FolderPlacements(const std::string& sampleRoot,
                                   const std::string& personalRoot)
 :samples(canonical(sampleRoot)),personal(canonical(personalRoot)){
 const char* data=g_get_user_data_dir();
 if(data && data[0]=='/'){
  std::string home=std::string(data)+"/linthesia";
  if(g_mkdir_with_parents(home.c_str(),0700)==0)
   file=home+"/ubicaciones-carpetas.ini";
 }
 Load();
}
bool FolderPlacements::Allowed(const std::string& path)const{
 if(!isFolder(path))return false; // Reject symlinks in folder mapping.
 const std::string abs=canonical(path);
 return (within(samples,abs)||within(personal,abs));
}
std::string FolderPlacements::PhysicalParent(const std::string& path) const {
 const std::string abs=canonical(path);
 if(abs==samples||abs==personal)return {};
 return dirnameOf(abs);
}
std::string FolderPlacements::Parent(const std::string& path)const{
 const std::string abs=canonical(path);
 for(const auto& r:relations)
  if(r.first==abs && Allowed(r.second))return r.second;
 return {};
}
std::vector<std::string> FolderPlacements::Children(const std::string& path)const{
 std::vector<std::string> children;
 const std::string dest=canonical(path);
 for(const auto& relation:relations)
  if(relation.second==dest && Allowed(relation.first))
   children.push_back(relation.first);
 return children;
}
bool FolderPlacements::ReferencesWithin(const std::string& p)const{
 const std::string path=canonical(p);
 for(const auto& pair:relations)
  if(within(path,pair.first)||within(path,pair.second))return true;
 return false;
}
bool FolderPlacements::CanContain(const std::string& source,
                                  const std::string& destination,
                                  std::string& error) const{
 const std::string from=canonical(source),to=canonical(destination);
 if(from==personal+"/1"){
    error="Añadidos está bloqueada.";return false;
 }
 if(!Allowed(from)||!Allowed(to)||from==samples||from==personal||
    to.empty()||from.empty()){
  error="Selecciona dos carpetas válidas.";return false;
 }
 if(from==to){error="Una carpeta no puede contenerse a sí misma.";return false;}
 // Check the *combined* physical + virtual directory hierarchy for cycles.
 std::set<std::string> seen;
 std::string current=to;
 for(int i=0;i<10000 && !current.empty();++i){
  if(current==from){error="No se puede crear un ciclo entre carpetas.";return false;}
  if(!seen.insert(current).second){
    error="La organización tiene un ciclo anterior.";return false;
  }
  const std::string logical=Parent(current);
  current=!logical.empty()?logical:PhysicalParent(current);
 }
 if(!current.empty()){error="Demasiados niveles de carpetas.";return false;}
 // Same visible name in the destination would make the GUI ambiguous.
 const std::string name=from.substr(from.find_last_of('/')+1);
 for(const auto& other:Children(to))
  if(other!=from && other.substr(other.find_last_of('/')+1)==name){
   error="Ya existe una subcarpeta visible con ese nombre.";return false;
  }
 const std::string diskChild=to+"/"+name;
 if(diskChild!=from && isFolder(diskChild) && Parent(diskChild).empty()){
  error="Ya existe una carpeta con ese nombre en el destino.";return false;
 }
 return true;
}
bool FolderPlacements::Place(const std::string& source,const std::string& destination,
                             std::string& error){
 if(!CanContain(source,destination,error))return false;
 const std::string from=canonical(source),to=canonical(destination);
 const auto previous=relations;
 relations.erase(std::remove_if(relations.begin(),relations.end(),
    [&](const std::pair<std::string,std::string>& r){return r.first==from;}),
    relations.end());
 relations.emplace_back(from,to);
 if(!Store(error)){
  relations=previous;return false;
 }
 return true;
}
bool FolderPlacements::RestoreSnapshot(
 const PlacementSnapshot& expected,const PlacementSnapshot& desired,
 std::string& error){
 if(relations!=expected){
  error="Las ubicaciones han cambiado. No se deshace para evitar pérdidas.";
  return false;
 }
 // Only restore directories that still exist and belong to this library.
 for(const auto& link:desired){
  if(link.first==personal+"/1"){
    error="Añadidos está bloqueada.";return false;
  }
  if(!Allowed(link.first)||!Allowed(link.second)){
   error="Una carpeta ya no existe o está fuera de la biblioteca.";
   return false;
  }
 }
 const auto old=relations;
 relations=desired;
 if(!Store(error)){relations=old;return false;}
 return true;
}
bool FolderPlacements::RelocateBatch(const std::vector<std::string>& folders,
                                     const std::string& destination,
                                     std::string& error){
 if(folders.empty() || !Allowed(destination)){
  error="No se ha seleccionado un destino válido.";return false;
 }
 const std::string to=canonical(destination);
 const bool root=to==samples;
 const auto old=relations;
 std::set<std::string> seen;
 for(const auto& source:folders){
  const std::string from=canonical(source);
  if(from==personal+"/1"){
    error="Añadidos está bloqueada.";relations=old;return false;
  }
  if(from.empty() || !seen.insert(from).second){
   error="Hay carpetas duplicadas o desaparecidas.";relations=old;return false;
  }
  if(root){
   if(!CanReturnToMain(from,error)){relations=old;return false;}
   relations.erase(std::remove_if(relations.begin(),relations.end(),
     [&](const std::pair<std::string,std::string>& r){return r.first==from;}),
     relations.end());
   const std::string parent=PhysicalParent(from);
   if(parent!=samples && parent!=personal)
    relations.emplace_back(from,samples);
  }else{
   if(!CanContain(from,to,error)){relations=old;return false;}
   relations.erase(std::remove_if(relations.begin(),relations.end(),
     [&](const std::pair<std::string,std::string>& r){return r.first==from;}),
     relations.end());
   relations.emplace_back(from,to);
  }
 }
 if(!Store(error)){relations=old;return false;}
 return true;
}
bool FolderPlacements::CanReturnToMain(const std::string& source,
                                      std::string& error) const{
 const std::string from=canonical(source);
 if(from==personal+"/1"){
    error="Añadidos está bloqueada.";return false;
 }
 if(!Allowed(from)||from==samples||from==personal){
  error="No se puede devolver esta carpeta.";return false;
 }
 const std::string virtualParent=Parent(from);
 const std::string physicalParent=PhysicalParent(from);
 if(virtualParent==samples ||
    (virtualParent.empty() && (physicalParent==samples ||
                                physicalParent==personal))){
  error="Esta carpeta ya está en la pantalla principal.";return false;
 }
 const std::string label=from.substr(from.find_last_of('/')+1);
 // The main screen merges the installed and the personal root. Disallow
 // duplicate visible titles to keep all folders individually accessible.
 for(const std::string& root:{samples,personal}){
  DIR* dir=opendir(root.c_str());
  if(!dir)continue;
  struct dirent* entry;
  while((entry=readdir(dir))){
   const std::string name(entry->d_name);
   if(name!=label)continue;
   const std::string candidate=root+"/"+name;
   if(candidate!=from && isFolder(candidate) && Parent(candidate).empty()){
    closedir(dir);
    error="Ya existe una carpeta con ese nombre en la pantalla principal.";
    return false;
   }
  }
  closedir(dir);
 }
 for(const auto& candidate:Children(samples)){
  if(candidate!=from &&
     candidate.substr(candidate.find_last_of('/')+1)==label){
   error="Ya existe una carpeta con ese nombre en la pantalla principal.";
   return false;
  }
 }
 return true;
}
bool FolderPlacements::ReturnToMain(const std::string& source,
                                    std::string& error){
 if(!CanReturnToMain(source,error))return false;
 const std::string from=canonical(source);
 const auto previous=relations;
 relations.erase(std::remove_if(relations.begin(),relations.end(),
   [&](const std::pair<std::string,std::string>& item){
     return item.first==from;
   }),relations.end());
 // If the physical folder is already in one of the displayed roots,
 // removing the virtual association is sufficient. Otherwise an explicit
 // virtual link to the combined library root is needed to show it there.
 const std::string parent=PhysicalParent(from);
 if(parent!=samples && parent!=personal)
  relations.emplace_back(from,samples);
 if(!Store(error)){
  relations=previous;
  return false;
 }
 return true;
}

std::string FolderPlacements::DisplayName(const std::string& path) const{
 const std::string key=canonical(path);
 auto found=aliases.find(key);
 if(found!=aliases.end())return found->second;
 return path.substr(path.find_last_of('/')+1);
}
bool FolderPlacements::SetDisplayName(const std::string& path,
                                       const std::string& name,
                                       std::string& error){
 const std::string abs=canonical(path);
 if(abs.empty()||!Allowed(abs) || !within(samples,abs) || abs==samples){
   error="Solo las carpetas originales se pueden renombrar visualmente.";
   return false;
 }
 if(name.empty()||name.size()>120||name=="."||name==".."||
    name.find('/')!=std::string::npos||name.find('\\')!=std::string::npos||
    !g_utf8_validate(name.c_str(),name.size(),nullptr)){
   error="Nombre de carpeta no valido.";return false;
 }
 if(g_unichar_isspace(g_utf8_get_char(name.c_str()))||
    g_unichar_isspace(g_utf8_get_char(g_utf8_prev_char(name.c_str()+name.size())))){
   error="El nombre no puede empezar ni terminar con espacios.";return false;
 }
 for(unsigned char c:name){
   if(c<32||c==127){error="El nombre contiene caracteres no admitidos.";return false;}
 }
 const auto previous=aliases;
 if(name==abs.substr(abs.find_last_of('/')+1))aliases.erase(abs);
 else aliases[abs]=name;
 if(!Store(error)){aliases=previous;return false;}
 return true;
}
bool FolderPlacements::RemapAfterRename(const std::string& oldPath,
                                         const std::string& newPath,
                                         std::string& error){
 if(oldPath.empty() || !Allowed(newPath)){
   error="La nueva ubicacion de la carpeta no es valida.";return false;
 }
 const auto previous=relations;
 const auto substitute=[&](std::string& path){
   if(within(oldPath,path))path=newPath+path.substr(oldPath.size());
 };
 for(auto& item:relations){
   substitute(item.first);
   substitute(item.second);
 }
 if(relations==previous)return true;
 for(const auto& item:relations){
   if(!Allowed(item.first)||!Allowed(item.second)){
     relations=previous;
     error="La organizacion contiene una ruta invalida tras renombrar.";
     return false;
   }
 }
 if(!Store(error)){relations=previous;return false;}
 return true;
}
void FolderPlacements::Load(){
 if(file.empty())return;
 GKeyFile* key=g_key_file_new();
 if(!g_key_file_load_from_file(key,file.c_str(),G_KEY_FILE_NONE,nullptr)){
  g_key_file_unref(key);return;
 }
 gsize count=0;
 gchar** keys=g_key_file_get_keys(key,"carpetas",&count,nullptr);
 for(gsize i=0;i<count;++i){
  gchar* from=g_key_file_get_string(key,"rutas",keys[i],nullptr);
  gchar* to=g_key_file_get_string(key,"carpetas",keys[i],nullptr);
  if(from&&to && digest(from)==keys[i] && Allowed(from)&&Allowed(to))
   relations.emplace_back(canonical(from),canonical(to));
  g_free(from);g_free(to);
 }
 g_strfreev(keys);
 // Los alias tienen un grupo independiente y rutas verificadas.
 gsize aliasCount=0;
 gchar** aliasKeys=g_key_file_get_keys(key,"nombres",&aliasCount,nullptr);
 for(gsize i=0;aliasKeys&&i<aliasCount;++i){
  gchar* source=g_key_file_get_string(key,"nombres-rutas",aliasKeys[i],nullptr);
  gchar* display=g_key_file_get_string(key,"nombres",aliasKeys[i],nullptr);
  if(source&&display&&digest(source)==aliasKeys[i] &&
     Allowed(source)&&within(samples,canonical(source)) &&
     g_utf8_validate(display,-1,nullptr))
    aliases[canonical(source)]=display;
  g_free(source);g_free(display);
 }
 g_strfreev(aliasKeys);
 g_key_file_unref(key);
}
bool FolderPlacements::Store(std::string& error)const{
 if(file.empty()){error="No se puede guardar la organización.";return false;}
 GKeyFile* key=g_key_file_new();
 for(const auto& r:relations){
  const std::string id=digest(r.first);
  g_key_file_set_string(key,"rutas",id.c_str(),r.first.c_str());
  g_key_file_set_string(key,"carpetas",id.c_str(),r.second.c_str());
 }
 for(const auto& item:aliases){
  const std::string id=digest(item.first);
  g_key_file_set_string(key,"nombres-rutas",id.c_str(),item.first.c_str());
  g_key_file_set_string(key,"nombres",id.c_str(),item.second.c_str());
 }
 gsize len=0;GError* ge=nullptr;
 gchar* data=g_key_file_to_data(key,&len,&ge);
 bool ok=data && g_file_set_contents(file.c_str(),data,len,&ge);
 if(!ok){
  error=ge?ge->message:"No se puede escribir la organización.";
 }
 if(ge)g_error_free(ge);
 g_free(data);g_key_file_unref(key);
 return ok;
}
