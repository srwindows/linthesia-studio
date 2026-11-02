#include "PersonalFolders.h"
#include <glib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <linux/fs.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>

namespace {
bool inside(const std::string& a,const std::string& b){
 return b==a || (b.size()>a.size()&&b.compare(0,a.size(),a)==0&&b[a.size()]=='/');
}
std::string resolved(const std::string& p){
 char* ptr=realpath(p.c_str(),nullptr);
 if(!ptr)return {};
 std::string s(ptr);free(ptr);return s;
}
std::string leaf(const std::string& p){
 return p.substr(p.find_last_of('/')+1);
}
bool exactDirectory(const std::string& p){
 struct stat s{};
 return lstat(p.c_str(),&s)==0&&S_ISDIR(s.st_mode);
}
bool noReplace(const std::string& a,const std::string& b,std::string& error){
 int rc=syscall(SYS_renameat2,AT_FDCWD,a.c_str(),AT_FDCWD,b.c_str(),RENAME_NOREPLACE);
 if(rc==0)return true;
 error=errno==EEXIST?"Ya existe una carpeta con ese nombre en el destino.":
       errno==EXDEV?"No se pueden mover carpetas entre discos distintos.":
       std::string("Error al mover carpeta: ")+strerror(errno);
 return false;
}
bool writeFile(const std::string& file,const std::string& content){
 int fd=open(file.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(fd<0)return false;
 size_t offset=0;
 while(offset<content.size()){
  ssize_t n=write(fd,content.data()+offset,content.size()-offset);
  if(n<=0){close(fd);unlink(file.c_str());return false;}
  offset+=n;
 }
 bool ok=fsync(fd)==0;
 close(fd);
 if(!ok)unlink(file.c_str());
 return ok;
}
}
PersonalFolders::PersonalFolders(){
 const char* data=g_get_user_data_dir();
 if(!data||data[0]!='/')return;
 const std::string base=std::string(data)+"/linthesia";
 if(g_mkdir_with_parents((base+"/canciones").c_str(),0700)!=0)return;
 m_root=resolved(base+"/canciones");
 if(g_mkdir_with_parents((base+"/papelera").c_str(),0700)!=0)return;
 if(g_mkdir_with_parents((base+"/orden").c_str(),0700)!=0)return;
 m_trash=resolved(base+"/papelera");
 m_order=resolved(base+"/orden");
}
bool PersonalFolders::Owns(const std::string& p)const{
 if(m_root.empty())return false;
 std::string real=resolved(p);
 return !real.empty()&&inside(m_root,real);
}
bool PersonalFolders::WritableFolder(const std::string& p)const{
 return Owns(p)&&exactDirectory(p);
}
bool PersonalFolders::OwnedChild(const std::string& p)const{
 return WritableFolder(p) && resolved(p)!=m_root;
}
bool PersonalFolders::Create(const std::string& parent,const std::string& name,
                              std::string& output,std::string& error)const{
 if(!WritableFolder(parent)){error="Destino fuera de tus carpetas personales.";return false;}
 if(name.empty()||name.size()>120||name=="."||name==".."||
    name.find('/')!=std::string::npos || name.find('\\')!=std::string::npos||
    !g_utf8_validate(name.c_str(),name.size(),nullptr)){
   error="Nombre no válido.";return false;
 }
 if(g_unichar_isspace(g_utf8_get_char(name.c_str()))||
    g_unichar_isspace(g_utf8_get_char(g_utf8_prev_char(name.c_str()+name.size())))){
   error="El nombre no puede empezar ni terminar con espacios.";return false;
 }
 for(unsigned char c:name)if(c<32||c==127){error="El nombre contiene controles.";return false;}
 output=parent+"/"+name;
 if(mkdir(output.c_str(),0700)!=0){
   error=errno==EEXIST?"Ya existe una carpeta con ese nombre.":
                           "No se pudo crear la carpeta: "+std::string(strerror(errno));
   return false;
 }
 return true;
}
bool PersonalFolders::CreateLinkedFolder(const std::string& name,
                                       std::string& created,
                                       std::string& error)const{
 if(!Ready()){
  error="No se ha podido preparar el almacenamiento de carpetas.";return false;
 }
 // This hidden location prevents example subfolders from appearing as
 // additional top-level cards in the personal library.
 const std::string hidden=m_root+"/.subcarpetas";
 if(mkdir(hidden.c_str(),0700)!=0 && errno!=EEXIST){
  error="No se pudo preparar la carpeta personal.";return false;
 }
 if(!exactDirectory(hidden)||!Owns(hidden)){
  error="El almacenamiento de subcarpetas no es seguro.";return false;
 }
 gchar* uuid=g_uuid_string_random();
 const std::string group=hidden+"/"+std::string(uuid);
 g_free(uuid);
 if(mkdir(group.c_str(),0700)!=0){
  error="No se pudo preparar la nueva subcarpeta.";return false;
 }
 if(!Create(group,name,created,error)){
  rmdir(group.c_str());
  return false;
 }
 return true;
}
void PersonalFolders::CancelLinkedFolder(const std::string& created)const{
 if(!OwnedChild(created))return;
 const std::string parent=created.substr(0,created.find_last_of('/'));
 if(!inside(m_root+"/.subcarpetas",parent))return;
 if(rmdir(created.c_str())==0)rmdir(parent.c_str());
}
bool PersonalFolders::Rename(const std::string& src,const std::string& name,
                             std::string& result,std::string& error)const{
 const std::string source=resolved(src);
 if(source==m_root+"/1"){
   error="Añadidos no se puede renombrar.";return false;
 }
 if(!OwnedChild(src) || source.empty()){
   error="Solo se pueden renombrar las carpetas personales.";return false;
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
   if(c<32||c==127){error="El nombre contiene caracteres de control.";return false;}
 }
 const std::string parent=source.substr(0,source.find_last_of('/'));
 if(!WritableFolder(parent)){error="La carpeta principal no es editable.";return false;}
 result=parent+"/"+name;
 if(result==source)return true;
 return noReplace(source,result,error); // Nunca sobrescribir carpetas existentes.
}
bool PersonalFolders::Move(const std::string& src,const std::string& targetParent,
                           std::string& output,std::string& error)const{
 if(resolved(src)==m_root+"/1"){
    error="Añadidos está bloqueada y no se puede mover.";return false;
 }
 if(!OwnedChild(src)){error="Solo se pueden mover carpetas personales.";return false;}
 if(!WritableFolder(targetParent)){error="El destino debe ser una carpeta personal.";return false;}
 const std::string from=resolved(src),dest=resolved(targetParent);
 if(inside(from,dest)){error="No puedes introducir una carpeta dentro de sí misma.";return false;}
 output=dest+"/"+leaf(from);
 if(output==from){error="Esa carpeta ya está en el destino.";return false;}
 return noReplace(from,output,error);
}
bool PersonalFolders::Trash(const std::string& path,std::string& ticket,std::string& error)const{
 if(resolved(path)==m_root+"/1"){
    error="Añadidos está bloqueada y no se puede eliminar.";return false;
 }
 if(!OwnedChild(path)||m_trash.empty()){error="Las carpetas originales están protegidas.";return false;}
 gchar* guid=g_uuid_string_random();
 ticket=m_trash+"/elemento-"+std::string(guid);
 g_free(guid);
 if(mkdir(ticket.c_str(),0700)!=0){error="No se puede preparar la papelera.";return false;}
 const std::string origin=resolved(path);
 if(!writeFile(ticket+"/origen",origin+"\n")){
   rmdir(ticket.c_str());error="No se pudo guardar el origen de la carpeta.";return false;
 }
 if(!noReplace(origin,ticket+"/carpeta",error)){
   unlink((ticket+"/origen").c_str());rmdir(ticket.c_str());return false;
 }
 return true;
}
std::vector<DeletedFolder> PersonalFolders::Deleted() const{
 std::vector<DeletedFolder> entries;
 if(m_trash.empty())return entries;
 DIR* dir=opendir(m_trash.c_str());
 if(!dir)return entries;
 struct dirent* entry;
 while((entry=readdir(dir))){
  std::string name(entry->d_name);
  if(name.compare(0,9,"elemento-")!=0)continue;
  std::string ticket=m_trash+"/"+name;
  if(!exactDirectory(ticket)||!exactDirectory(ticket+"/carpeta"))continue;
  gchar* buf=nullptr;gsize size=0;
  if(!g_file_get_contents((ticket+"/origen").c_str(),&buf,&size,nullptr))continue;
  std::string origin(buf,size);g_free(buf);
  if(!origin.empty() && origin.back()=='\n')origin.pop_back();
  if(origin.empty()||origin.find('\n')!=std::string::npos)continue;
  entries.push_back({ticket,origin});
 }
 closedir(dir);
 std::sort(entries.begin(),entries.end(),[](const DeletedFolder& a,
                                             const DeletedFolder& b){
  return a.ticket>b.ticket;
 });
 return entries;
}
bool PersonalFolders::Restore(const std::string& ticket,std::string& result,
                              std::string& error)const{
 const auto list=Deleted();
 const auto it=std::find_if(list.begin(),list.end(),
    [&](const DeletedFolder& e){return e.ticket==ticket;});
 if(it==list.end()){error="No existe esa carpeta en la papelera.";return false;}
 std::string origin=it->origin;
 size_t separator=origin.find_last_of('/');
 if(separator==std::string::npos){error="Origen inválido.";return false;}
 std::string parent=origin.substr(0,separator);
 if(!WritableFolder(parent)){error="La carpeta de destino original no existe.";return false;}
 if(!noReplace(ticket+"/carpeta",origin,error))return false;
 unlink((ticket+"/origen").c_str());
 rmdir(ticket.c_str());
 result=origin;
 return true;
}
std::string PersonalFolders::OrderFile(const std::string& dir)const {
 struct stat st{};
 if(m_order.empty()||stat(dir.c_str(),&st)!=0||!S_ISDIR(st.st_mode))return {};
 return m_order+"/"+std::to_string(static_cast<unsigned long long>(st.st_dev))+
        "-"+std::to_string(static_cast<unsigned long long>(st.st_ino))+".txt";
}
std::vector<std::string> PersonalFolders::ReadOrder(const std::string& dir) const{
 std::vector<std::string> ordered;
 std::string file=OrderFile(dir);
 if(file.empty())return ordered;
 gchar* contents=nullptr;gsize len=0;
 if(!g_file_get_contents(file.c_str(),&contents,&len,nullptr))return ordered;
 std::string bytes(contents,len);g_free(contents);
 size_t start=0;
 while(start<bytes.size()&&ordered.size()<50000){
  size_t end=bytes.find('\n',start);
  if(end==std::string::npos)end=bytes.size();
  std::string name=bytes.substr(start,end-start);
  if(!name.empty()&&name.size()<=256&&name.find('/')==std::string::npos)
   ordered.push_back(name);
  start=end+1;
 }
 return ordered;
}
bool PersonalFolders::SaveOrder(const std::string& dir,const std::vector<std::string>& names,
                                std::string& error)const {
 std::string file=OrderFile(dir);
 if(file.empty()){error="No se puede guardar el orden.";return false;}
 std::string bytes;
 for(const auto& name:names){
  if(name.find('\n')!=std::string::npos||name.find('/')!=std::string::npos){
   error="Nombre no válido en el orden.";return false;
  }
  bytes+=name+"\n";
 }
 GError* ge=nullptr;
 bool ok=g_file_set_contents(file.c_str(),bytes.c_str(),bytes.size(),&ge);
 if(!ok){
  error=ge?ge->message:"No se pudo guardar el orden.";
  if(ge)g_error_free(ge);
 }
 return ok;
}
