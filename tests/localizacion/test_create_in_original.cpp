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
static bool fileExists(const string& p){struct stat s{};return stat(p.c_str(),&s)==0;}
static bool has(const std::vector<string>& a,const string& b){
 return std::find(a.begin(),a.end(),b)!=a.end();
}
int main(){
 char tmp[]="/tmp/linthesia-create-everywhere-XXXXXX";
 char* path=mkdtemp(tmp);assert(path);
 assert(setenv("XDG_DATA_HOME",path,1)==0);
 const string installed=string(path)+"/music";
 assert(mkdir(installed.c_str(),0700)==0);
 const string hannon=installed+"/Hannon";
 const string learning=installed+"/Learning";
 assert(mkdir(hannon.c_str(),0700)==0);
 assert(mkdir(learning.c_str(),0700)==0);
 {std::ofstream f(hannon+"/Lesson_1.mid",std::ios::binary);f<<"ORIGINAL_NO_MODIFY";}
 PersonalFolders personal;
 assert(personal.Ready());
 string piano,error;
 assert(personal.Create(personal.Root(),"piano",piano,error));
 FolderPlacements placements(installed,personal.Root());
 string created;
 // Exact user case: new folder in bundled Hannon, no original file altered.
 assert(personal.CreateLinkedFolder("Nueva en Hannon",created,error));
 assert(personal.OwnedChild(created));
 assert(created.find("/.subcarpetas/")!=string::npos);
 assert(personal.Root()+"/Nueva en Hannon"!=created);
 assert(placements.Place(created,hannon,error));
 assert(placements.Parent(created)==hannon);
 assert(has(placements.Children(hannon),created));
 assert(fileExists(hannon+"/Lesson_1.mid"));
 assert(!fileExists(hannon+"/Nueva en Hannon"));
 // The new folder behaves like all personal folders, with nested folders.
 string deeper;
 assert(personal.Create(created,"Subnivel",deeper,error));
 assert(fileExists(deeper));
 // The same visible name may be used under ANOTHER bundled parent, with
 // no global collisions in hidden storage or the visible parent.
 string another;
 assert(personal.CreateLinkedFolder("Nueva en Hannon",another,error));
 assert(placements.Place(another,learning,error));
 assert(created!=another);
 assert(has(placements.Children(learning),another));
 assert(placements.Parent(another)==learning);
 // A duplicate visible name within Hannon must fail to link, and a
 // newly created unlinked directory is fully rolled back.
 string extra;
 assert(personal.CreateLinkedFolder("Nueva en Hannon",extra,error));
 assert(!placements.Place(extra,hannon,error));
 personal.CancelLinkedFolder(extra);
 assert(!fileExists(extra));
 // A cancelled dialog must never create folders.
 assert(!personal.CreateLinkedFolder("../fuera",extra,error));
 assert(!personal.CreateLinkedFolder(" ",extra,error));
 // Reload preserves original and nested personal placement.
 FolderPlacements reopened(installed,personal.Root());
 assert(reopened.Parent(created)==hannon);
 assert(reopened.Parent(another)==learning);
 assert(has(reopened.Children(hannon),created));
 assert(fileExists(deeper));
 assert(fileExists(hannon+"/Lesson_1.mid"));
 // Can return the created folder back to main using existing action.
 assert(reopened.ReturnToMain(created,error));
 assert(reopened.Parent(created)==installed);
 assert(has(reopened.Children(installed),created));
 assert(fileExists(deeper));
 std::cout<<"CREATE_ANY_FOLDER_OK: original/Hannon, separate names, rollback, "
             "subfolders, persistent virtual placement, original MIDI intact\n";
}
