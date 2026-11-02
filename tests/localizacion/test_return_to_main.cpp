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
static bool includes(const std::vector<string>& v,const string& s){
 return std::find(v.begin(),v.end(),s)!=v.end();
}
int main(){
 char path[]="/tmp/linthesia-return-main-XXXXXX";
 char* temp=mkdtemp(path);assert(temp);
 assert(setenv("XDG_DATA_HOME",temp,1)==0);
 string sample=string(temp)+"/originales";
 assert(mkdir(sample.c_str(),0700)==0);
 string hannon=sample+"/Hannon",lesson=sample+"/Learning";
 assert(mkdir(hannon.c_str(),0700)==0);
 assert(mkdir(lesson.c_str(),0700)==0);
 string nestedOriginal=lesson+"/Romantic";
 assert(mkdir(nestedOriginal.c_str(),0700)==0);
 {std::ofstream f(hannon+"/leccion.mid");f<<"MThd_SIN_ALTERAR";}
 {std::ofstream f(nestedOriginal+"/pieza.mid");f<<"MThd_INTACTO";}
 PersonalFolders folders;
 string piano,child,error;
 assert(folders.Create(folders.Root(),"piano",piano,error));
 assert(folders.Create(piano,"Ensayos",child,error));
 {std::ofstream f(piano+"/tema.mid");f<<"MThd_TEMA_PIANO";}
 {std::ofstream f(child+"/midi.mid");f<<"MThd_MIDI_ENSAYOS";}
 {
  FolderPlacements manager(sample,folders.Root());
  assert(!manager.CanReturnToMain(hannon,error)); // already at main
  assert(manager.Place(hannon,piano,error));
  assert(manager.CanReturnToMain(hannon,error));
  assert(manager.ReturnToMain(hannon,error)); // original returns to main
  assert(manager.Parent(hannon).empty());
  assert(!includes(manager.Children(piano),hannon));
  assert(exists(hannon+"/leccion.mid"));
  assert(exists(piano+"/tema.mid"));
  assert(!manager.CanReturnToMain(hannon,error));
  // Personal top-level relocated inside an installed example and returned.
  assert(manager.Place(piano,lesson,error));
  assert(manager.ReturnToMain(piano,error));
  assert(manager.Parent(piano).empty());
  assert(exists(child+"/midi.mid"));
  // Physically nested user folder appears in unified main without moving bytes.
  assert(manager.CanReturnToMain(child,error));
  assert(manager.ReturnToMain(child,error));
  assert(manager.Parent(child)==sample);
  assert(includes(manager.Children(sample),child));
  assert(exists(child+"/midi.mid"));
  assert(!manager.CanReturnToMain(child,error));
  // A bundled subfolder can be returned to main (its physical path protected).
  assert(manager.ReturnToMain(nestedOriginal,error));
  assert(manager.Parent(nestedOriginal)==sample);
  assert(includes(manager.Children(sample),nestedOriginal));
  assert(exists(nestedOriginal+"/pieza.mid"));
 }
 {
  FolderPlacements reopened(sample,folders.Root());
  assert(reopened.Parent(hannon).empty());
  assert(reopened.Parent(piano).empty());
  assert(reopened.Parent(child)==sample);
  assert(reopened.Parent(nestedOriginal)==sample);
  assert(includes(reopened.Children(sample),child));
  // Reject duplicate visible folder names across the combined roots.
  string duplicate;
  assert(folders.Create(piano,"Hannon",duplicate,error));
  assert(!reopened.CanReturnToMain(duplicate,error));
  assert(!reopened.ReturnToMain(duplicate,error));
  assert(exists(duplicate)); // no data loss on rejected return
  assert(!reopened.ReturnToMain("/etc",error));
  assert(!reopened.ReturnToMain(sample,error));
  assert(exists(hannon+"/leccion.mid"));
 }
 std::cout<<"RETURN_TO_MAIN_OK: bundled and personal folders, nested physical paths, "
             "duplicate safety, reload, original MIDI intact\n";
}
