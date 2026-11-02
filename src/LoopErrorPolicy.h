// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_LOOP_ERROR_POLICY_H
#define LINTHESIA_LOOP_ERROR_POLICY_H
#include <algorithm>
namespace LoopErrorPolicy {
inline bool ShouldRestart(int threshold,int errorsAtStart,int errorsNow,
                          bool loopDefined,bool loopActive,bool paused){
 return threshold>0&&threshold<=20 && loopDefined&&loopActive&&!paused
        &&errorsNow>=errorsAtStart
        &&errorsNow-errorsAtStart>=threshold;
}
}
#endif
