#include "FolderPlacements.h"
#include "PersonalFolders.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
using std::string;
static bool exists(const string& p){struct stat st{};return stat(p.c_str(),&st)==0;}
static bool has(const std::vector<string>& v,const string& p){
 return std::find(v.begin(),v.end(),p)!=v.end();
}
int main(){
 char path[]="/tmp/linthesia-placement-XXXXXX";
 char* tmp=mkdtemp(path);assert(tmp);
 assert(setenv("XDG_DATA_HOME",tmp,1)==0);
 string sample=string(tmp)+"/instalados";
 assert(mkdir(sample.c_str(),0700)==0);
 string hannon=sample+"/Hannon",favorites=sample+"/Favorites";
 assert(mkdir(hannon.c_str(),0700)==0);
 assert(mkdir(favorites.c_str(),0700)==0);
 {std::ofstream file(hannon+"/pieza_original.mid");file<<"ORIGINAL_MIDI_123";}
 PersonalFolders folders;
 string piano,error;
 assert(folders.Create(folders.Root(),"piano",piano,error));
 {std::ofstream file(piano+"/tema_de_piano.mid");file<<"MIDI_PIANO_ABC";}
 string jazz;
 assert(folders.Create(folders.Root(),"jazz",jazz,error));
 {
   FolderPlacements links(sample,folders.Root());
   assert(links.CanContain(hannon,piano,error));
   assert(links.Place(hannon,piano,error));
   assert(links.Parent(hannon)==piano);
   assert(has(links.Children(piano),hannon));
   assert(!has(links.Children(sample),hannon));
   // Bundled sample files stay where they were originally.
   assert(exists(hannon+"/pieza_original.mid"));
   assert(exists(piano+"/tema_de_piano.mid"));
   assert(!exists(piano+"/Hannon"));
   // An original folder cannot become its own virtual descendant.
   assert(!links.Place(piano,hannon,error));
   assert(!links.Place(hannon,hannon,error));
   // Move Hannon logically from piano to Favorites, both original folders.
   assert(links.Place(hannon,favorites,error));
   assert(!has(links.Children(piano),hannon));
   assert(has(links.Children(favorites),hannon));
   assert(exists(hannon+"/pieza_original.mid"));
   assert(links.Place(hannon,piano,error));
   // personal -> original also works non-destructively.
   assert(links.Place(jazz,hannon,error));
   assert(has(links.Children(hannon),jazz));
   assert(!links.Place(hannon,jazz,error)); // virtual ancestry cycle
   assert(!links.Place(favorites,favorites,error));
   assert(!links.Place(sample,piano,error)); // root can't be nested
 }
 {
   FolderPlacements saved(sample,folders.Root());
   assert(saved.Parent(hannon)==piano);
   assert(saved.Parent(jazz)==hannon);
   assert(has(saved.Children(piano),hannon));
   assert(has(saved.Children(hannon),jazz));
   // Still show MIDI in both virtual source and destination directories.
   assert(exists(piano+"/tema_de_piano.mid"));
   assert(exists(hannon+"/pieza_original.mid"));
 }
 // User source->user destination remains possible physically.
 string moved;
 assert(folders.Move(jazz,piano,moved,error)); // standalone manager allows this
 assert(exists(moved));
 std::cout<<"HANNON_PIANO_VIRTUAL_OK original MIDI preserved, nested display, persisted, cycles rejected\n";
}
