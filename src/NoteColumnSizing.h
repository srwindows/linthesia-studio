#ifndef LINTHESIA_NOTE_COLUMN_SIZING_H
#define LINTHESIA_NOTE_COLUMN_SIZING_H

#include <algorithm>
#include <cmath>

namespace NoteColumnSizing {
struct Column { int left; int width; };
// Fit each 3D note to its OWN key, never the keyboard's white-key width.
// The familiar Width preference stays effective, but 100% means a slender
// column (~60% of its key) instead of an oversized rectangular shaft.
inline Column Fit3D(int keyLeft,int keyWidth,bool black,int setting){
  if(keyWidth<=0)return {keyLeft,0};
  const float factor=(black?.62f:.60f)*
      std::max(0.05f,std::min(2.0f,setting/100.0f));
  const int desired=static_cast<int>(std::lround(keyWidth*factor));
  const int minimum=std::min(3,keyWidth);
  const int maximum=std::max(minimum,keyWidth-2);
  const int width=std::max(minimum,std::min(maximum,desired));
  return {keyLeft+(keyWidth-width)/2,width};
}
} // namespace NoteColumnSizing
#endif
