#include "MusicXmlImport.h"
#include <cassert>
#include <string>
#include <iostream>
int main(){
 assert(MusicXmlImport::IsXml("Nocturno.musicxml"));
 assert(MusicXmlImport::IsXml("NOCTURNO.MXL"));
 assert(MusicXmlImport::IsXml("Partitura.xml"));
 assert(!MusicXmlImport::IsXml("bach.mid"));
 assert(!MusicXmlImport::IsXml("unsafe.musicxml.exe"));
 const auto path=MusicXmlImport::Converter();
 assert(path.find("/scripts/musicxml_to_midi.py")!=std::string::npos);
 std::cout<<"OK MusicXML/MXL extension detection, known local converter path\n";
}
