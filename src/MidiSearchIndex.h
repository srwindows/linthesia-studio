#ifndef LINTHESIA_MIDI_SEARCH_INDEX_H
#define LINTHESIA_MIDI_SEARCH_INDEX_H
#include <string>
#include <vector>
class FolderPlacements;
struct MidiSearchHit {
 std::string name;
 std::string filePath;
 std::string folderPath;
 std::string visibleRoute;
};
class MidiSearchIndex {
public:
 static std::vector<MidiSearchHit> Search(
     const std::string& originalRoot,
     const std::string& personalRoot,
     const FolderPlacements& placements,
     const std::string& query);
};
#endif
