#include "TrackSelectionPersistence.h"
#include <glib.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
static void require(bool x,const char* msg){if(!x){std::cerr<<msg<<'\n';std::exit(1);}}
int main(){
 char dir[]="/tmp/linthesia-track-prefs-XXXXXX";
 require(mkdtemp(dir)!=nullptr,"temp directory failed");
 require(setenv("XDG_DATA_HOME",dir,1)==0,"cannot set XDG_DATA_HOME");
 namespace tp=TrackSelectionPersistence;
 const std::string a="/tmp/cancion-A.mid",b="/tmp/cancion-B.mid";
 const std::vector<tp::Choice> original={
  {1,13,true},{3,2,false},{6,15,true},{5,1,false},
  {2,3,false},{4,5,true},{0,4,false},{1,6,true},
  {0,8,false},{6,14,true}};
 std::vector<tp::Choice> loaded={{4,1,true}};
 int page=99;
 require(!tp::Load(a,10,loaded,page),"unexpected prefs before save");
 require(tp::Save(a,original,1),"unable to save prefs");
 require(tp::FilePath(a)!=tp::FilePath(b),"song path collision");
 require(!tp::Load(b,10,loaded,page),"different song inherited prefs");
 require(!tp::Load(a,9,loaded,page),"modified track count inherited old prefs");
 require(tp::Load(a,10,loaded,page),"saved preferences not restored");
 require(page==1,"pagination page not restored");
 require(loaded.size()==original.size(),"track rows lost");
 for(size_t i=0;i<loaded.size();++i){
  require(loaded[i].mode==original[i].mode,"mode not persistent");
  require(loaded[i].color==original[i].color,"color not persistent");
  require(loaded[i].retry==original[i].retry,"retry toggle not persistent");
 }
 require(tp::Save(a,original,0),"second save failed");
 require(tp::Load(a,10,loaded,page)&&page==0,"page did not persist updates");
 // Broken INI file must be rejected instead of propagating invalid enums.
 require(g_file_set_contents(tp::FilePath(a).c_str(),"[Song]\npath=/tmp/cancion-A.mid\ntracks=10\n[Track0]\nmode=999\ncolor=-1\nretry=4\n",-1,nullptr),"corrupt write failed");
 require(!tp::Load(a,10,loaded,page),"invalid saved settings accepted");
 require(!tp::Save("relative.mid",original,0),"relative path unsafe");
 std::cout<<"OK: per-MIDI modes, colors, retry and page persist; isolation and validation correct\n";
 return 0;
}
