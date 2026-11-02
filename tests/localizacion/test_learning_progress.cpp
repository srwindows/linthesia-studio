#include "LearningProgress.h"
#include <cassert>
#include <iostream>
using namespace LearningProgress;
using PracticeHistory::Attempt;
int main(){
 assert(Analyze({}).points==0);
 Attempt a; a.expected=100;a.hit=100;a.hands=0;a.complete=true;
 auto one=Analyze({a});
 assert(one.completed==1 && one.points==17);
 a.hands=1;
 auto two=Analyze({a,Attempt{0,100,100,0,120,0,0,true}});
 assert(two.points==50);
 a.hands=2;
 std::vector<Attempt> trio(3);
 for(int i=0;i<3;i++){
  trio[i].expected=100;trio[i].hit=100;trio[i].hands=i;
  trio[i].complete=true;
 }
 auto perfect=Analyze(trio);
 assert(perfect.points==100 && perfect.completed==3);
 trio[0].complete=false;
 assert(Analyze(trio).points<100);
 trio[0].complete=true;trio[0].hit=0;
 assert(Analyze(trio).next=="Practicar ambas manos");
 std::cout<<"OK: 0-100 graded learning progress, hands, completeness, recommendations\n";
}
