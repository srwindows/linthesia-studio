#include "SongBookmarks.h"
#include <cassert>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <iostream>
int main(){
 char dir[]="/tmp/linthesia-bookmarks-XXXXXX";
 assert(mkdtemp(dir));
 g_setenv("XDG_DATA_HOME",dir,TRUE);
 std::string path=std::string(dir)+"/example.mid";
 auto a=SongBookmarks::Load(path);
 assert(a.empty());
 assert(SongBookmarks::Add(a,4000000,"M2"));
 assert(SongBookmarks::Add(a,1000000,"M1"));
 assert(!SongBookmarks::Add(a,1010000,"Duplicado"));
 assert(a[0].time==1000000 && a[1].time==4000000);
 assert(SongBookmarks::Next(a,0)==1000000);
 assert(SongBookmarks::Next(a,1200000)==4000000);
 assert(SongBookmarks::Next(a,4000000)==1000000);
 assert(SongBookmarks::Save(path,a));
 auto b=SongBookmarks::Load(path);
 assert(b.size()==2 && b[0].name=="M1");
 assert(!SongBookmarks::RemoveNearest(b,2500000,100000));
 assert(SongBookmarks::RemoveNearest(b,4000000,100000));
 assert(b.size()==1 && b[0].time==1000000);
 assert(SongBookmarks::Save(path,b));
 assert(SongBookmarks::Load(path).size()==1);
 assert(!SongBookmarks::Add(b,0,""));
 assert(!SongBookmarks::Save("",b));
 assert(SongBookmarks::Load("/other/path.mid").empty());
 std::cout<<"OK: portable named MIDI markers, persistence, sort, jump/wrap, duplicate prevention, removal\n";
}
