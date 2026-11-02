#include "UiAppearance.h"
#include <algorithm>
namespace {
int scale=100,theme=0;
int effective(int selected,int width,int height){
 // Never shrink the logical canvas so far that essential controls cannot fit.
 // A 760x540 window retains its original layout at zoom >=100%.
 // Above 100%, preserve a minimum 780 x 720 logical canvas
 // because the start page has four full-sized device/file tiles.
 const int maxByWidth=width>0?width*100/780:100;
 const int maxByHeight=height>0?height*100/720:100;
 const int maxZoom=std::max(100,std::min(maxByWidth,maxByHeight));
 return selected<=100?selected:std::min(selected,maxZoom);
}
int clamp(int v){return std::max(0,std::min(v,255));}
}
namespace UiAppearance {
void Configure(int s,int t){
 scale=std::max(75,std::min(s,150));
 theme=std::max(0,std::min(t,2));
}
int Scale(){return scale;}
int EffectiveScaleFor(int selected,int w,int h){return effective(selected,w,h);}
int Theme(){return theme;}
int LogicalWidth(int w,int h){return w*100/effective(scale,w,h);}
int LogicalHeight(int w,int h){return h*100/effective(scale,w,h);}
int ToLogicalX(int x,int w,int h){return x*100/effective(scale,w,h);}
int ToLogicalY(int y,int w,int h){return y*100/effective(scale,w,h);}
void Transform(int& r,int& g,int& b){
 if(theme==0)return; // Clásico: exactamente los colores originales.
 const int mx=std::max(r,std::max(g,b));
 const int mn=std::min(r,std::min(g,b));
 if(mx>215 && mn>200)return; // Texto blanco permanece blanco.
 if(theme==1){
  // Metallic blue retains luminance but cools every neutral panel and accent.
  if(mx-mn<35 && mx<195){
   r=clamp(r*82/100);g=clamp(g*95/100+2);b=clamp(b*110/100+8);
  }else if(g>=r && b>=r && r<140){
   r=clamp(r*78/100);g=clamp(g*105/100);b=clamp(b*123/100+5);
  }
 }else{
  // High contrast makes panel edges brighter and darks deeper.
  if(mx<85){r=clamp(r*65/100);g=clamp(g*65/100);b=clamp(b*65/100);}
  else if(mx<205){
   r=clamp((r-90)*130/100+95);
   g=clamp((g-90)*130/100+95);
   b=clamp((b-90)*130/100+95);
  }
 }
}
}
