// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_LEARNING_PROGRESS_H
#define LINTHESIA_LEARNING_PROGRESS_H
#include "PracticeHistory.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
namespace LearningProgress {
// An open, reproducible learning objective, not Synthesia's proprietary
// progression algorithm. Best accuracy for both hands (50%), right (25%),
// left (25%). Three completed sessions establish a full evidence weight.
// No progress from automatic playback or uncompleted practice attempts.
struct Report {
 int points=0;
 std::array<int,3> bestAccuracy{{0,0,0}};
 int completed=0;
 std::string next;
};
inline Report Analyze(const std::vector<PracticeHistory::Attempt>& attempts){
 Report report;
 for(const auto& a:attempts){
  if(!a.complete||a.expected<=0||a.hit<0||a.hit>a.expected)continue;
  const int hand=std::max(0,std::min(2,a.hands));
  report.bestAccuracy[hand]=std::max(report.bestAccuracy[hand],a.Accuracy());
  ++report.completed;
 }
 const double score=report.bestAccuracy[0]*.5+
                report.bestAccuracy[1]*.25+report.bestAccuracy[2]*.25;
 const double evidence=std::min(1.0,report.completed/3.0);
 report.points=std::max(0,std::min(100,int(std::lround(score*evidence))));
 if(report.bestAccuracy[1]<80)report.next="Practicar mano derecha";
 else if(report.bestAccuracy[2]<80)report.next="Practicar mano izquierda";
 else if(report.bestAccuracy[0]<90)report.next="Practicar ambas manos";
 else report.next="Mantener precisión y velocidad";
 return report;
}
}
#endif
