// SPDX-License-Identifier: GPL-2.0-or-later
// Original Linthesia MIDI free-play recorder. No commercial program code.
#ifndef LINTHESIA_FREE_PLAY_RECORDER_H
#define LINTHESIA_FREE_PLAY_RECORDER_H
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace FreePlay {
struct Event {
 uint64_t microseconds=0;
 uint8_t status=0,data1=0,data2=0;
};
struct Track {
 std::vector<Event> events;
 uint8_t program=0;
 bool muted=false;
};
class Recorder {
public:
 static constexpr size_t MaxTracks=6;
 int bpm=120;
 bool quantize=false;
 bool recording=false;
 int activeTrack=-1;
 std::vector<Track> tracks;
 bool Start();
 void Push(uint64_t microseconds,uint8_t status,uint8_t data1,uint8_t data2);
 void Stop(uint64_t microseconds);
 bool Delete(size_t index);
 bool SetProgram(size_t index,int program);
 std::vector<uint8_t> ToMidi() const;
 bool Save(const std::string& path) const;
 bool Restore(const std::string& path);
 bool SaveSession(const std::string& path)const;
 static bool ValidEvent(uint8_t status,uint8_t d1,uint8_t d2);
};
}
#endif
