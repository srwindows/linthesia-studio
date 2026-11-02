// Historia de reproducción / orden / privacidad de archivos existentes.
#include "RecentSongs.h"
#include "AdvancedSettings.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
using namespace RecentSongs;
int main(){
 char directory[]="/tmp/linthesia-recientes-test-XXXXXX";
 char* root=mkdtemp(directory);assert(root);
 assert(setenv("XDG_DATA_HOME",root,1)==0);
 assert(setenv("XDG_CONFIG_HOME",root,1)==0);
 std::string a=std::string(root)+"/a.mid";
 std::string b=std::string(root)+"/b.mid";
 std::string c=std::string(root)+"/c.mid";
 for(const auto& path:{a,b,c}){
  std::ofstream file(path,std::ios::binary);file<<"MThd";
 }
 assert(List(20,PlaybackOrder).empty());
 RecordPlay(b); RecordPlay(a); RecordPlay(c); RecordPlay(a);
 RecordPoints(a,500);RecordPoints(a,300);
 RecordPoints(b,900);RecordPoints(c,200);
 auto rows=List(20,PlaybackOrder);
 assert(rows.size()==3 && rows[0].path==a && rows[0].plays==2);
 assert(rows[0].bestPoints==500);
 rows=List(20,Alphabetical);
 assert(rows.size()==3 && rows[0].path==a && rows[1].path==b && rows[2].path==c);
 rows=List(20,MostPlayed);
 assert(rows[0].path==a && rows[0].plays==2);
 rows=List(20,HighestPoints);
 assert(rows[0].path==b && rows[0].bestPoints==900);
 rows=List(2,HighestPoints);
 assert(rows.size()==2 && rows[0].path==a && rows[1].path==c);
 // La lista se limita por RECIENCIA antes de aplicarle otro orden.
 unlink(c.c_str());
 rows=List(20,PlaybackOrder);assert(rows.size()==2);
 RecordPlay(std::string(root)+"/noexistente.mid");
 assert(List(20,PlaybackOrder).size()==2);
 auto conf=AdvancedSettingsIO::Load();
 conf.recentLimit=50;conf.recentSort=HighestPoints;
 std::string error;assert(AdvancedSettingsIO::Save(conf,error));
 conf=AdvancedSettingsIO::Load();
 assert(conf.recentLimit==50 && conf.recentSort==HighestPoints);
 assert(conf.alphabeticOrder==false);
 assert(access(a.c_str(),F_OK)==0 && access(b.c_str(),F_OK)==0);
 std::cout<<"OK: Recientes persistencia, reproducciones, mejores puntos, cuatro ordenes y archivos intactos\n";
}
