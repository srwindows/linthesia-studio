// SPDX-License-Identifier: GPL-2.0-or-later
// Live MIDI note visualization shared by hardware input and 88-key mouse piano.
#ifndef LINTHESIA_FREE_PLAY_VISUAL_NOTES_H
#define LINTHESIA_FREE_PLAY_VISUAL_NOTES_H
#include <algorithm>
#include <cstdint>
#include <vector>
namespace FreePlay {
struct VisualNote {
 int pitch=60,channel=0,velocity=90;
 uint64_t onset=0,release=0;
 bool held=true;
};
// Display-only state. It never changes MIDI timing, velocity or recorder data.
class VisualNotes {
public:
 static constexpr uint64_t FadeUs=1450000;
 static constexpr uint64_t ScrollUs=8500000;
 std::vector<VisualNote> notes;
 void Event(uint8_t status,uint8_t key,uint8_t velocity,uint64_t now){
  const int kind=status&0xF0;
  const int channel=status&0x0F;
  if(key>127||(kind!=0x80&&kind!=0x90))return;
  const bool on=kind==0x90&&velocity>0;
  if(on){
   // A repeated note starts a new trace rather than joining the old one.
   for(auto it=notes.rbegin();it!=notes.rend();++it)
    if(it->held&&it->pitch==key&&it->channel==channel){
     it->held=false;it->release=now;break;
    }
   if(notes.size()>=512)
    notes.erase(notes.begin(),notes.begin()+128);
   notes.push_back({int(key),channel,int(velocity),now,0,true});
  }else{
   for(auto it=notes.rbegin();it!=notes.rend();++it)
    if(it->held&&it->pitch==key&&it->channel==channel){
      it->held=false;it->release=std::max(now,it->onset);break;
    }
  }
 }
 void Update(uint64_t now){
  notes.erase(std::remove_if(notes.begin(),notes.end(),
   [now](const VisualNote& note){
    return !note.held && now>=note.release &&
           now-note.release>=FadeUs;
   }),notes.end());
 }
 void ReleaseAll(uint64_t now){
  for(auto& note:notes)
   if(note.held){note.held=false;note.release=std::max(now,note.onset);}
 }
 int Alpha(const VisualNote& note,uint64_t now)const{
  if(note.held||now<=note.release)return 232;
  const auto passed=now-note.release;
  if(passed>=FadeUs)return 0;
  // Smoothstep prevents abrupt popping at either end of the fade.
  const double t=double(passed)/FadeUs;
  const double smooth=t*t*(3.0-2.0*t);
  return std::max(0,std::min(232,int(232*(1.0-smooth))));
 }
 size_t ActiveCount()const{
  return std::count_if(notes.begin(),notes.end(),
              [](const VisualNote&n){return n.held;});
 }
};
}
#endif
