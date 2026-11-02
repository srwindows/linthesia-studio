#include "PersonalFolders.h"
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
using namespace std;
static bool exists(const string& s){struct stat st{};return lstat(s.c_str(),&st)==0;}
int main(){
 char tmp[]="/tmp/linthesia-folders-XXXXXX";
 char* dir=mkdtemp(tmp);assert(dir);
 assert(setenv("XDG_DATA_HOME",dir,1)==0);
 PersonalFolders folders;
 assert(folders.Ready());
 assert(exists(folders.Root()));
 string created,error;
 assert(folders.Create(folders.Root(),"Mis partituras",created,error));
 assert(exists(created));
 assert(folders.Owns(created));
 string nested;
 assert(folders.Create(created,"Clásica y jazz",nested,error));
 assert(exists(nested));
 assert(!folders.Create(folders.Root(),"Mis partituras",created,error));
 assert(!folders.Create(folders.Root(),"../insegura",created,error));
 assert(!folders.Create(folders.Root(),"/tmp/insegura",created,error));
 assert(!folders.Create(folders.Root()," ",created,error));
 assert(!folders.Create(folders.Root(),"Inicio/",created,error));
 assert(!folders.Create("/tmp","Fuera",created,error));
 assert(!folders.Owns("/etc"));
 string link=folders.Root()+"/enlace-externo";
 assert(symlink("/etc",link.c_str())==0);
 assert(!folders.Owns(link));
 assert(!folders.Create(link,"NoCrear",created,error));
 assert(!exists("/etc/NoCrear"));
 string dest,result;
 assert(folders.Create(folders.Root(),"Destino",dest,error));
 assert(!folders.Move("/etc",dest,result,error));
 assert(!folders.Trash("/etc",result,error));
 assert(!folders.Move(folders.Root(),dest,result,error));
 assert(!folders.Move(created,nested,result,error)); // no self-nesting
 assert(!folders.Move(created,created,result,error));
 assert(!folders.Move(created,"/tmp",result,error));
 // A real folder containing nested MIDI is moved, never copied.
 string song=created+"/muestra.mid";
 {ofstream f(song,ios::binary);f<<"MThd";} // keep raw bytes intact
 const string actual=created;
 assert(folders.Move(actual,dest,result,error));
 assert(result==dest+"/Mis partituras");
 assert(!exists(actual)&&exists(result+"/muestra.mid"));
 assert(exists(result+"/Clásica y jazz"));
 assert(!folders.Move(result,dest,result,error)); // same destination
 // Order persists with the folder inode even when it is moved.
 string orderError;
 assert(folders.SaveOrder(result,{"Clásica y jazz","muestra.mid"},orderError));
 auto order=folders.ReadOrder(result);
 assert(order.size()==2&&order[0]=="Clásica y jazz");
 // Save and reread sorting of any arbitrary sequence of folders.
 assert(folders.SaveOrder(folders.Root(),{"Destino","Mis canciones","Hannon"},orderError));
 auto reopened=PersonalFolders();
 auto rootOrder=reopened.ReadOrder(reopened.Root());
 assert(rootOrder.size()==3&&rootOrder[0]=="Destino"&&rootOrder[2]=="Hannon");
 // A move into an occupied path must not overwrite any folder.
 string duplicate;
 assert(folders.Create(dest,"Mis partituras",duplicate,error)==false);
 string second,secondDest;
 assert(folders.Create(folders.Root(),"Otra",second,error));
 assert(folders.Create(dest,"Otra",secondDest,error));
 assert(!folders.Move(second,dest,result,error));
 assert(exists(second)&&exists(secondDest));
 // Send a nonempty folder into a recoverable trash ticket.
 string ticket;
 assert(folders.Trash(dest+"/Mis partituras",ticket,error));
 assert(!exists(dest+"/Mis partituras"));
 assert(exists(ticket+"/carpeta/muestra.mid"));
 auto trashed=folders.Deleted();
 bool found=false;
 for(const auto& item:trashed)if(item.ticket==ticket &&
   item.origin==dest+"/Mis partituras")found=true;
 assert(found);
 // Refuse restore collisions; preserve entire folder in trash.
 string obstruct;
 assert(folders.Create(dest,"Mis partituras",obstruct,error));
 assert(!folders.Restore(ticket,result,error));
 assert(exists(ticket+"/carpeta/muestra.mid")&&exists(obstruct));
 assert(rmdir(obstruct.c_str())==0);
 assert(folders.Restore(ticket,result,error));
 assert(result==dest+"/Mis partituras"&&exists(result+"/muestra.mid"));
 assert(!exists(ticket));
 assert(folders.ReadOrder(result).size()==2);
 assert(!folders.Restore(ticket,result,error));
 cout<<"CARPETAS: creación real, Unicode, subcarpetas, duplicados y seguridad OK"<<endl;
 return 0;
}
