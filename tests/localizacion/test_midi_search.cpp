#include "MidiSearchIndex.h"
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
static void folder(const string& p){assert(mkdir(p.c_str(),0700)==0);}
static void write(const string& p){std::ofstream f(p);f<<"MThd TEST";}
int main(){
 char tmp[]="/tmp/linthesia-search-regression-XXXXXX";
 char* base=mkdtemp(tmp);assert(base);
 assert(setenv("XDG_DATA_HOME",base,1)==0);
 string original=string(base)+"/original";
 folder(original); folder(original+"/Hannon");
 write(original+"/Hannon/Lesson_1.mid");
 write(original+"/Hannon/DUPLICADO.MID");
 PersonalFolders personal;assert(personal.Ready());
 string piano,error;assert(personal.Create(personal.Root(),"piano",piano,error));
 write(piano+"/duplicado.mid");
 string nested;assert(personal.Create(piano,"Subcarpeta",nested,error));
 write(nested+"/TemaFinal.midi");
 FolderPlacements placements(original,personal.Root());
 assert(placements.Place(original+"/Hannon",piano,error));
 auto matching=MidiSearchIndex::Search(original,personal.Root(),
                                      placements,"duplicado");
 assert(matching.size()==2);
 assert(matching[0].visibleRoute=="/piano"||
        matching[0].visibleRoute=="/piano/Hannon");
 assert(matching[0].folderPath!=matching[1].folderPath);
 assert(matching[0].filePath!=matching[1].filePath);
 assert(matching[0].visibleRoute!=matching[1].visibleRoute);
 auto lesson=MidiSearchIndex::Search(original,personal.Root(),
                                     placements,"LESSON");
 assert(lesson.size()==1);
 assert(lesson[0].visibleRoute=="/piano/Hannon");
 assert(lesson[0].name=="Lesson_1.mid");
 auto nestedHits=MidiSearchIndex::Search(original,personal.Root(),
                                         placements,"temafinal");
 assert(nestedHits.size()==1);
 assert(nestedHits[0].visibleRoute=="/piano/Subcarpeta");
 assert(MidiSearchIndex::Search(original,personal.Root(),
                                placements,"no existe").empty());
 assert(MidiSearchIndex::Search(original,personal.Root(),
                                placements,"").empty());
 // A symlink escaping the library must not be searched.
 string outside=string(base)+"/outside.mid";write(outside);
 assert(symlink(outside.c_str(),(piano+"/outside.mid").c_str())==0);
 assert(MidiSearchIndex::Search(original,personal.Root(),
                               placements,"outside").empty());
 // Hidden storage must not appear as a separate browsable folder.
 string hidden=personal.Root()+"/.secret";
 folder(hidden);write(hidden+"/OCULTO.mid");
 assert(MidiSearchIndex::Search(original,personal.Root(),
                               placements,"OCULTO").empty());
 std::cout<<"GLOBAL_SEARCH_OK: duplicate names, virtual routes, nested "
             "folders, case-insensitive extension, safety\n";
}
