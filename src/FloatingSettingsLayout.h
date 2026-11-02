// GPL-2.0-or-later. Movable non-modal player settings geometry.
#ifndef LINTHESIA_FLOATING_SETTINGS_LAYOUT_H
#define LINTHESIA_FLOATING_SETTINGS_LAYOUT_H
#include <algorithm>
namespace FloatingSettings {
struct Rect {int x,y,w,h; bool contains(int px,int py) const {
 return px>=x && px<x+w && py>=y && py<y+h;
}};
struct Layout {
 int x,y,w,h,rows;
 int tabTop()const{return y+56;}
 int contentTop()const{return y+139;}
 int navY()const{return y+h-88;}
 int actionY()const{return y+h-46;}
 Rect bounds() const{return {x,y,w,h};}
 Rect header() const{return {x,y,w,47};}
 Rect tab(int index)const{
  const int width=(w-20)/5;
  return {x+10+(index%5)*width,tabTop()+(index/5)*35,width-2,32};
 }
 Rect entry(int row)const{return {x+10,contentTop()+row*46,w-20,40};}
 Rect slider(int row)const{return {x+w-211,contentTop()+row*46+2,118,35};}
 Rect minus(int row)const{return {x+w-89,contentTop()+row*46+2,35,35};}
 Rect plus(int row)const{return {x+w-49,contentTop()+row*46+2,35,35};}
 Rect action(int button)const{
  const int bw=96,space=4;
  return {x+w-12-(4-button)*(bw+space)+space,actionY(),bw,32};
 }
 Rect prev()const{return {x+13,navY(),95,30};}
 Rect next()const{return {x+w-108,navY(),95,30};}
 Rect close()const{return {x+w-32,y+9,24,24};}
};
inline Layout Fit(int screenW,int screenH,int oldX=-1,int oldY=-1){
 const int w=std::min(440,std::max(350,screenW-24));
 const int h=std::min(510,std::max(330,screenH-145));
 const int initX=std::max(8,screenW-w-16),initY=std::max(105,(screenH-h)/2);
 const int x=oldX<0?initX:std::max(8,std::min(oldX,std::max(8,screenW-w-8)));
 const int y=oldY<0?initY:std::max(99,std::min(oldY,std::max(99,screenH-h-8)));
 const int rows=std::max(2,std::min(5,(h-242)/46));
 return {x,y,w,h,rows};
}
}
#endif
