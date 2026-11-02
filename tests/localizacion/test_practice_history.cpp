#include "PracticeHistory.h"
#include <cassert>
#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <fstream>
#include <iostream>
int main(){
 char home[]="/tmp/linthesia-practice-progress-XXXXXX";
 assert(mkdtemp(home));
 g_setenv("XDG_DATA_HOME",home,TRUE);
 const std::string song=std::string(home)+"/cancion.musicxml";
 std::ofstream(song)<<"<score/>";
 assert(PracticeHistory::Load(song).empty());
 assert(PracticeHistory::Filename(song).find("/linthesia/practica/")!=std::string::npos);
 PracticeHistory::Attempt attempt;
 attempt.timestamp=1000000;
 attempt.expected=10;attempt.hit=3;attempt.errors=2;
 attempt.bpm=90;attempt.points=300;attempt.hands=1;
 attempt.complete=false;
 assert(attempt.Accuracy()==30);
 assert(PracticeHistory::Append(song,attempt));
 auto rows=PracticeHistory::Load(song);
 assert(rows.size()==1&&!rows[0].complete&&rows[0].Accuracy()==30);
 for(int i=0;i<54;++i){
  auto next=attempt;
  next.timestamp=1000001+i;
  next.hit=std::min(10,i%11);
  next.complete=true;
  next.hands=2;
  assert(PracticeHistory::Append(song,next));
 }
 rows=PracticeHistory::Load(song);
 assert(rows.size()==50);
 assert(rows[0].timestamp==1000054);
 assert(rows[0].hands==2&&rows[0].complete);
 assert(rows[0].hit==53%11);
 assert(rows[49].timestamp==1000005);
 auto latest=rows[0];
 latest.expected=0;
 assert(!PracticeHistory::Append(song,latest));
 latest.expected=5;latest.hit=6;
 assert(!PracticeHistory::Append(song,latest));
 assert(PracticeHistory::Load(song).size()==50);
 assert(PracticeHistory::Load(std::string(home)+"/missing.mid").empty());
 std::cout<<"OK: progress attempts per song, 50 record retention, accuracy, incomplete, malformed, isolation\n";
}
