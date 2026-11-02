#include "DragPageNavigation.h"
#include <cassert>
#include <iostream>
using namespace DragPageNavigation;
int main(){
 State s{};
 s.width=1400; s.height=900; s.fileListTop=480;
 s.folderPages=3;s.folderPage=0; s.midiPages=4;s.midiPage=0;
 s.folderPrevious={1040,150,65,35};s.folderNext={1115,150,65,35};
 s.midiPrevious={1125,815,90,35};s.midiNext={1230,815,90,35};
 assert(Hit(s,1140,160)==Target::FolderNext);
 assert(Hit(s,1260,830)==Target::MidiNext);
 assert(Hit(s,1390,300)==Target::FolderNext);
 assert(Hit(s,1390,570)==Target::MidiNext);
 assert(Hit(s,1390,20)==Target::None);
 assert(Hit(s,1390,899)==Target::None);
 assert(Hit(s,30,570)==Target::None);
 s.folderPage=1;s.midiPage=2;
 assert(Hit(s,1050,160)==Target::FolderPrevious);
 assert(Hit(s,1140,160)==Target::FolderNext);
 assert(Hit(s,1135,825)==Target::MidiPrevious);
 assert(Hit(s,1250,825)==Target::MidiNext);
 assert(Hit(s,5,250)==Target::FolderPrevious);
 assert(Hit(s,5,600)==Target::MidiPrevious);
 s.folderPage=2;s.midiPage=3;
 assert(Hit(s,1390,300)==Target::None);
 assert(Hit(s,1390,570)==Target::None);
 assert(Hit(s,5,300)==Target::FolderPrevious);
 assert(Hit(s,5,570)==Target::MidiPrevious);
 // Subcarpetas: paginacion MIDI arriba, carpetas abajo.
 State compact{};
 compact.width=1150;compact.height=760;compact.fileListTop=245;
 compact.folderPage=0;compact.folderPages=4;compact.midiPage=0;compact.midiPages=3;
 compact.folderPrevious={989,703,65,26};compact.folderNext={1064,703,65,26};
 compact.midiPrevious={1046,16,39,30};compact.midiNext={1093,16,39,30};
 assert(Hit(compact,1100,26)==Target::MidiNext);
 assert(Hit(compact,1090,710)==Target::FolderNext);
 assert(Hit(compact,1140,170)==Target::FolderNext);
 assert(Hit(compact,1140,450)==Target::MidiNext);
 compact.midiPages=1;
 assert(Hit(compact,1140,450)==Target::None);
 // Casos reales: con carpetas agarradas, cruzar la lista de MIDI NO
 // debe cambiar las paginas MIDI ni dejar el arrastre bloqueado.
 compact.folderPages=4; compact.folderPage=0;
 compact.midiPages=1;
 assert(Hit(compact,1140,450,DragKind::Folder)==Target::FolderNext);
 compact.midiPages=3;
 assert(Hit(compact,1140,450,DragKind::Folder)==Target::FolderNext);
 assert(Hit(compact,1140,450,DragKind::Midi)==Target::MidiNext);
 assert(Hit(compact,1100,16,DragKind::Midi)==Target::MidiNext);
 // Zona ampliada: el usuario puede apuntar cerca de la paginacion.
 assert(Hit(compact,1090,686,DragKind::Folder)==Target::FolderNext);
 compact.folderPage=2;
 assert(Hit(compact,1030,686,DragKind::Folder)==Target::FolderPrevious);
 std::cout<<"OK: paginacion durante arrastre por flechas, zonas ampliadas y bordes\n";
}
