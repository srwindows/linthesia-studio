// GPL-2.0-or-later; choices belong to the MIDI file, not the session.
#ifndef LINTHESIA_TRACK_SELECTION_PERSISTENCE_H
#define LINTHESIA_TRACK_SELECTION_PERSISTENCE_H
#include <string>
#include <vector>
namespace TrackSelectionPersistence {
struct Choice { int mode=0; int color=0; bool retry=false; int program=-1; int bank=0; };
// Stored per canonical song path under $XDG_DATA_HOME/linthesia/pistas.
std::string FilePath(const std::string& song);
bool Load(const std::string& song,unsigned tracks,
          std::vector<Choice>& choices,int& page);
bool Save(const std::string& song,const std::vector<Choice>& choices,int page);
}
#endif
