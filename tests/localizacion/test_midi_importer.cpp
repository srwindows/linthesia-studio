#include "MidiImporter.h"
#include "PersonalFolders.h"
#include "FolderPlacements.h"
#include <glib.h>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
static void mkdirSafe(const std::string& p){assert(mkdir(p.c_str(),0700)==0);}
static void file(const std::string& p,const std::string& data="MThd"){
 std::ofstream f(p,std::ios::binary);f<<data;assert(f.good());
}
static bool exists(const std::string& p){return access(p.c_str(),F_OK)==0;}
int main(){
 char base[]="/tmp/linthesia-importer-test-XXXXXX";
 assert(mkdtemp(base)!=nullptr);
 const std::string root(base),files=root+"/fuentes";
 mkdirSafe(files);
 const std::string a=files+"/A",b=files+"/B",sub=a+"/Subgrupo";
 mkdirSafe(a);mkdirSafe(b);mkdirSafe(sub);
 file(a+"/Tema.mid","primer");file(a+"/otro.MIDI","segundo");
 file(sub+"/deep.mid","subarchivo");
 file(sub+"/nota.txt","no se importa");
 file(b+"/Tema.mid","otro origen");
 file(files+"/directo.mid","directo");
 file(files+"/foto.jpg","no MIDI");
 assert(symlink("/etc/passwd",(a+"/contrabando.mid").c_str())==0);
 // Isolated PersonalFolders: do not modify user's library.
 assert(setenv("XDG_DATA_HOME",base,1)==0);
 PersonalFolders personal;
 assert(personal.Ready());
 std::string target,error;
 assert(personal.Create(personal.Root(),"1",target,error));
 const auto result=MidiImporter::Import(
     {files+"/directo.mid",a,b,files+"/foto.jpg",a},target);
 assert(result.errors.empty());
 assert(result.copied==5); // directo, A two, A/Subgrupo, B Tema
 assert(result.folders==3);
 assert(result.skipped>=2);
 assert(exists(target+"/directo.mid"));
 assert(exists(target+"/A/Tema.mid"));
 assert(exists(target+"/A/otro.MIDI"));
 assert(exists(target+"/A/Subgrupo/deep.mid"));
 assert(!exists(target+"/A/Subgrupo/nota.txt"));
 assert(!exists(target+"/A/contrabando.mid"));
 assert(exists(target+"/B/Tema.mid"));
 // Reimport does not overwrite existing MIDI files.
 const auto repeat=MidiImporter::Import({files+"/directo.mid"},target);
 assert(repeat.copied==1 && exists(target+"/directo - copia 2.mid"));
 const auto circular=MidiImporter::Import({target},target);
 assert(!circular.errors.empty() && circular.copied==0);
 assert(MidiImporter::IsFixedFolder(target,personal.Root()));
 assert(!MidiImporter::IsFixedFolder(a,personal.Root()));
 const std::string otherRoot=root+"/bundled";
 mkdirSafe(otherRoot);
 FolderPlacements placements(otherRoot,personal.Root());
 std::string moveOutput,trash;
 std::string other;
 assert(personal.Create(personal.Root(),"Otra",other,error));
 assert(!personal.Move(target,other,moveOutput,error));
 assert(!personal.Trash(target,trash,error));
 assert(!placements.Place(target,other,error));
 assert(!placements.RelocateBatch({target},other,error));
 assert(!placements.CanReturnToMain(target,error));
 assert(exists(target+"/directo.mid"));
 puts("PASS: multi-file/folder MIDI copies, nested folders, duplicates, symlinks, fixed folder 1");
}
