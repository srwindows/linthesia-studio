// Linthesia GPLv2: última canción entre ejecuciones de la aplicación.
#include "RecentSongs.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
int main(){
  char folder[]="/tmp/linthesia-ultima-cancion-XXXXXX";
  char* dir=mkdtemp(folder);
  assert(dir);
  assert(setenv("XDG_DATA_HOME",dir,1)==0);
  const std::string one=std::string(dir)+"/uno.mid";
  const std::string two=std::string(dir)+"/dos.mid";
  const std::string three=std::string(dir)+"/tres.mid";
  for(const auto& path:{one,two,three}){
    std::ofstream stream(path,std::ios::binary);
    stream<<"MThd";
  }
  using namespace RecentSongs;
  assert(LastSelectedPath().empty());
  RecordPlay(one);
  assert(LastSelectedPath()==one); // migración del historial anterior
  RememberLastSelected(two);
  assert(LastSelectedPath()==two);
  RecordPlay(three);
  assert(LastSelectedPath()==two); // Recientes no cambia la elección
  RememberLastSelected("/no/existe/midi.mid");
  assert(LastSelectedPath()==two); // ruta inválida no reemplaza la válida
  RememberLastSelected(three);
  assert(LastSelectedPath()==three);
  assert(access((std::string(dir)+"/linthesia/ultima-cancion.ini").c_str(),F_OK)==0);
  unlink(three.c_str());
  assert(LastSelectedPath()==one); // canción desaparecida: siguiente disponible
  RememberLastSelected(two);
  assert(LastSelectedPath()==two); // persistencia recuperable al reabrir
  auto list=List(50,PlaybackOrder);
  assert(list.size()==1 && list.front().path==one); // historial intacto
  std::cout<<"OK: última canción seleccionada persistente, recuperación y precedencia sobre Recientes\n";
}
