#include "MidiImporter.h"
#include <glib.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
#include <set>
#include <string>
#include <vector>
namespace {
bool isDir(const std::string& p){struct stat s{};return lstat(p.c_str(),&s)==0 && S_ISDIR(s.st_mode);}
bool isFile(const std::string& p){struct stat s{};return lstat(p.c_str(),&s)==0 && S_ISREG(s.st_mode);}
bool midi(const std::string& p){
 const size_t dot=p.rfind('.');
 if(dot==std::string::npos)return false;
 const std::string ext=p.substr(dot);
 return g_ascii_strcasecmp(ext.c_str(),".mid")==0 ||
        g_ascii_strcasecmp(ext.c_str(),".midi")==0;
}
std::string name(const std::string& p){
 const size_t slash=p.rfind('/');
 return slash==std::string::npos?p:p.substr(slash+1);
}
std::string unique(const std::string& folder,const std::string& wanted){
 std::string result=folder+"/"+wanted;
 struct stat state{};
 if(lstat(result.c_str(),&state)!=0)return result;
 const size_t dot=wanted.rfind('.');
 const std::string base=dot==std::string::npos?wanted:wanted.substr(0,dot);
 const std::string ext=dot==std::string::npos?"":wanted.substr(dot);
 for(int n=2;n<100000;++n){
    result=folder+"/"+base+" - copia "+std::to_string(n)+ext;
    if(lstat(result.c_str(),&state)!=0)return result;
 }
 return {};
}
std::string real(const std::string& path){
 char* value=realpath(path.c_str(),nullptr);
 if(!value)return {};
 std::string out(value);free(value);return out;
}
bool inside(const std::string& base,const std::string& path){
 return path==base || (path.size()>base.size() &&
        path.compare(0,base.size(),base)==0 && path[base.size()]=='/');
}
void traverse(const std::string& src,const std::string& dest,
              int depth,MidiImporter::Result& result){
 if(depth>40){result.errors.push_back("Demasiados niveles: "+src);return;}
 if(isFile(src)){
    if(!midi(src)){++result.skipped;return;}
    const std::string output=unique(dest,name(src));
    if(output.empty()){result.errors.push_back("Demasiados duplicados: "+name(src));return;}
    GFile* from=g_file_new_for_path(src.c_str());
    GFile* to=g_file_new_for_path(output.c_str());
    GError* problem=nullptr;
    const bool ok=g_file_copy(from,to,G_FILE_COPY_NOFOLLOW_SYMLINKS,
                               nullptr,nullptr,nullptr,&problem);
    g_object_unref(from);g_object_unref(to);
    if(ok)++result.copied;
    else{
       result.errors.push_back("No se pudo importar "+name(src)+": "+
                             (problem?problem->message:"Error de E/S"));
       if(problem)g_error_free(problem);
    }
    return;
 }
 if(!isDir(src)){++result.skipped;return;} // Never follow symlinks.
 // Do not recursively copy the fixed folder into itself.
 const std::string absSource=real(src);
 const std::string absDest=real(dest);
 if(absSource.empty()||absDest.empty()||inside(absSource,absDest)){
    result.errors.push_back("La carpeta de origen contiene el destino: "+src);
    return;
 }
 const std::string child=unique(dest,name(src));
 if(child.empty()||g_mkdir_with_parents(child.c_str(),0700)!=0){
    result.errors.push_back("No se puede crear la subcarpeta: "+name(src));
    return;
 }
 ++result.folders;
 DIR* dir=opendir(src.c_str());
 if(!dir){result.errors.push_back("No se puede abrir: "+src);return;}
 std::vector<std::string> children;
 struct dirent* e=nullptr;
 while((e=readdir(dir))){
   const std::string item=e->d_name;
   if(item=="."||item=="..")continue;
   children.push_back(src+"/"+item);
 }
 closedir(dir);
 std::sort(children.begin(),children.end());
 for(const auto& item:children)traverse(item,child,depth+1,result);
}
}
namespace MidiImporter {
bool IsFixedFolder(const std::string& path,const std::string& personalRoot){
 // Exact match only: similarly named folders elsewhere are movable.
 return path==personalRoot+"/1";
}
Result Import(const std::vector<std::string>& selected,const std::string& dest){
 Result result;
 if(!isDir(dest)){result.errors.push_back("Añadidos no está disponible.");return result;}
 const std::string canonicalDest=real(dest);
 std::set<std::string> distinct;
 for(const auto& src:selected){
    if(src.empty() || src[0]!='/'){++result.skipped;continue;}
    if(!distinct.insert(src).second)continue;
    if(src==dest||real(src)==canonicalDest){
       result.errors.push_back("No se puede importar Añadidos dentro de sí misma.");
       continue;
    }
    traverse(src,dest,0,result);
 }
 return result;
}
}
