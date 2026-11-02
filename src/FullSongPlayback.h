// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_FULL_SONG_PLAYBACK_H
#define LINTHESIA_FULL_SONG_PLAYBACK_H
#include "TrackProperties.h"
#include <vector>
#include <cstddef>
namespace FullSongPlayback {
struct Result {
 int playable=0,excluded=0,percussion=0;
};
// Playback override is transient: the user's practice track choices must
// remain available and untouched in the persistent song library.
inline Result Count(const std::vector<Track::Properties>& tracks,
                     const std::vector<bool>& hasNotes,
                     const std::vector<bool>& drums){
 Result r;
 for(size_t i=0;i<tracks.size();++i){
  if(i>=hasNotes.size() || !hasNotes[i])continue;
  if(i<drums.size() && drums[i])++r.percussion;
  if(tracks[i].mode==Track::ModePlayedAutomatically ||
     tracks[i].mode==Track::ModePlayedButHidden)++r.playable;
  else ++r.excluded;
 }
 return r;
}
inline void EnableAll(std::vector<Track::Properties>& tracks,
                  const std::vector<bool>& hasNotes,
                  const std::vector<bool>& drums){
 for(size_t i=0;i<tracks.size();++i){
  if(i>=hasNotes.size() || !hasNotes[i])continue;
  const bool drum=i<drums.size() && drums[i];
  tracks[i].mode=drum?Track::ModePlayedButHidden:
                       Track::ModePlayedAutomatically;
 }
}
inline int ClampNoteVelocity(double velocity){
 if(velocity<=0)return 0;
 if(velocity>=127)return 127;
 return static_cast<int>(velocity);
}
}
#endif
