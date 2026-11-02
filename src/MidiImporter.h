#ifndef LINTHESIA_MIDI_IMPORTER_H
#define LINTHESIA_MIDI_IMPORTER_H
#include <string>
#include <vector>
namespace MidiImporter {
 struct Result {int copied=0,folders=0,skipped=0; std::vector<std::string> errors;};
 bool IsFixedFolder(const std::string& path,const std::string& personalRoot);
 Result Import(const std::vector<std::string>& selected,const std::string& destination);
}
#endif
