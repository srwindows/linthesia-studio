// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_AUDIO_PEAK_GUARD_H
#define LINTHESIA_AUDIO_PEAK_GUARD_H
#include <algorithm>
#include <cmath>
namespace AudioPeakGuard {
// Transparent below -3.74 dBFS, smoothly limits the louder transients
// toward -1.5 dBFS. Applies linked stereo gain so it preserves the image.
// Unlike a hard clip this function is continuous with continuous slope at
// the knee: no clicks from samples crossing the threshold.
inline void StereoSoftLimit(float& left,float& right){
 if(!std::isfinite(left))left=0;
 if(!std::isfinite(right))right=0;
 constexpr float knee=0.650000f;
 constexpr float ceiling=0.841395f; // -1.5 dBFS
 const float amplitude=std::max(std::abs(left),std::abs(right));
 if(amplitude<=knee)return;
 const float range=ceiling-knee;
 const float limited=knee+range*std::tanh((amplitude-knee)/range);
 const float factor=limited/amplitude;
 left*=factor;right*=factor;
}
}
#endif
