// SPDX-License-Identifier: GPL-2.0-or-later
// Estimate a pitch boundary for chosen piano parts without editing MIDI.
// The existing HandPractice::Classifier still assigns each individual note.
#ifndef LINTHESIA_HAND_AUTO_DETECTOR_H
#define LINTHESIA_HAND_AUTO_DETECTOR_H
#include "HandPractice.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace HandAutoDetector {
struct Result {
 int splitPoint=60;
 size_t notes=0;
 size_t left=0;
 size_t right=0;
 bool pianoSelected=false;
 bool success=false;
};
// The candidates are evaluated on the distribution actually present in the
// selected part(s); no SysEx, no audio, no MIDI message and no file write.
inline int SuggestSplit(std::vector<int> pitches,int previous){
 const int fallback=std::max(48,std::min(84,previous));
 if(pitches.size()<12)return fallback;
 std::sort(pitches.begin(),pitches.end());
 const int q1=pitches[pitches.size()/4];
 const int q3=pitches[pitches.size()*3/4];
 // A high/low monophonic part may belong to just one hand. Do not create
 // a spurious opposite-hand voice by splitting a narrow melody in two.
 if(q3-q1<7 || q1>=65 || q3<=55)return 60;
 const int center=std::max(48,std::min(84,(q1+q3)/2));
 size_t histogram[128]={};
 for(int note:pitches)if(note>=0&&note<128)++histogram[note];
 const size_t n=pitches.size();
 double best=1e100;
 int chosen=60;
 // A smooth pitch valley near the inner quartiles is a reasonable prior.
 // Penalize strongly unbalanced results and large movements away from C4.
 for(int split=std::max(48,center-9);
         split<=std::min(84,center+9);++split){
   const size_t left=std::lower_bound(pitches.begin(),pitches.end(),split)-
                       pitches.begin();
   const size_t right=n-left;
   if(left<n/7 || right<n/7)continue;
   const double near=double(histogram[split-1]+histogram[split]);
   const double balance=std::abs(double(left)-double(right))/double(n);
   const double cost=near/double(n)*5.0 + balance*1.4 +
                     std::abs(split-60)*0.022 +
                     std::abs(split-center)*0.010;
   if(cost<best){best=cost;chosen=split;}
 }
 return best<1e99?chosen:fallback;
}
inline Result Detect(const TranslatedNoteSet& allNotes,
                     const std::vector<bool>& chosen,
                     int previousSplit,bool hasPiano=false,
                     bool automatic=true){
 Result out;
 out.splitPoint=std::max(48,std::min(84,previousSplit));
 out.pianoSelected=hasPiano;
 std::vector<int> pitches;
 for(const auto& note:allNotes){
  if(note.note_id>127 || note.channel==9 ||
     note.track_id>=chosen.size() || !chosen[note.track_id])continue;
  pitches.push_back(int(note.note_id));
 }
 if(pitches.empty())return out;
 if(automatic)out.splitPoint=SuggestSplit(std::move(pitches),out.splitPoint);
 // The player also classifies using the original complete MIDI note set.
 HandPractice::Classifier classifier;
 classifier.Build(allNotes,out.splitPoint);
 for(const auto& note:allNotes){
  if(note.note_id>127||note.channel==9||
     note.track_id>=chosen.size()||!chosen[note.track_id])continue;
  ++out.notes;
  if(classifier.ForNote(note)==HandPractice::Left)++out.left;
  else ++out.right;
 }
 out.success=out.notes>0;
 return out;
}
}
#endif
