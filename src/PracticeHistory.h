// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_PRACTICE_HISTORY_H
#define LINTHESIA_PRACTICE_HISTORY_H
#include <string>
#include <algorithm>
#include <vector>
namespace PracticeHistory {
struct Attempt {
 long long timestamp=0;      // Unix microseconds
 int hit=0,expected=0,errors=0,bpm=0,points=0;
 int hands=0;                // 0 both, 1 right, 2 left
 bool complete=false;
 int Accuracy() const { return expected>0?
   std::max(0,std::min(100,int(100LL*hit/expected))):0; }
};
std::string Filename(const std::string &song);
std::vector<Attempt> Load(const std::string &song);
bool Append(const std::string &song,Attempt attempt);
}
#endif
