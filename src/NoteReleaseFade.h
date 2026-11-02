// SPDX-License-Identifier: GPL-2.0-or-later
// Visual-only release envelope. Musical NoteOff and keyboard states are exact.
#ifndef LINTHESIA_NOTE_RELEASE_FADE_H
#define LINTHESIA_NOTE_RELEASE_FADE_H
#include <algorithm>
#include <cstdint>
namespace NoteReleaseFade {
constexpr std::int64_t DurationUs=420000; // 420 ms; unaffected by BPM and FPS.
// Continuous from 1 at NoteOff down to 0, with zero derivative at both ends.
inline float Gain(std::int64_t now,std::int64_t noteOff){
 if(now<=noteOff)return 1.0f;
 if(now>=noteOff+DurationUs)return 0.0f;
 const float t=float(now-noteOff)/float(DurationUs);
 return 1.0f-t*t*(3.0f-2.0f*t);
}
inline bool Visible(std::int64_t now,std::int64_t noteOff){
 return now>=noteOff && now<noteOff+DurationUs;
}
}
#endif
