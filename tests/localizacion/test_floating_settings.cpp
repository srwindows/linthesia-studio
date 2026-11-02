#include "FloatingSettingsLayout.h"
#include <cassert>
#include <iostream>
int main(){
 for(int sw:{720,800,1024,1280,1920})for(int sh:{540,720,1080}){
  const auto a=FloatingSettings::Fit(sw,sh);
  assert(a.x>=8 && a.y>=99 && a.x+a.w<=sw);
  assert(a.y+a.h<=sh && a.w<=500 && a.rows>=2);
  assert(a.entry(a.rows-1).y+a.entry(a.rows-1).h<a.navY());
  assert(a.tab(0).y<a.tab(5).y);
  assert(a.tab(9).x>a.tab(5).x);
  assert(a.header().contains(a.x+20,a.y+16));
  for(int i=0;i<4;++i)assert(a.action(i).x>=a.x && a.action(i).x+a.action(i).w<=a.x+a.w);
  const auto moved=FloatingSettings::Fit(sw,sh,sw+500,-200);
  assert(moved.x+moved.w<=sw && moved.y>=99);
 }
 const auto full=FloatingSettings::Fit(1280,720);
 assert(full.w<=500 && full.x>=700); // majority of scenery remains visible.
 std::cout<<"OK: floating 2-row tabs, window drag bounds and clear scene space\n";
}
