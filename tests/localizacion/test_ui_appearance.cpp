#include "UiAppearance.h"
#include <cassert>
#include <cstdio>
int main(){
 UiAppearance::Configure(100,0);
 assert(UiAppearance::LogicalWidth(1280,720)==1280);
 assert(UiAppearance::LogicalHeight(1280,720)==720);
 assert(UiAppearance::ToLogicalX(600,1280,720)==600);
 int r=70,g=90,b=110;UiAppearance::Transform(r,g,b);
 assert(r==70 && g==90 && b==110);
 UiAppearance::Configure(150,1);
 // 150% applies fully at 1920x1080, with a 1280x720 logical canvas.
 assert(UiAppearance::LogicalWidth(1920,1080)==1280);
 assert(UiAppearance::LogicalHeight(1920,1080)==720);
 assert(UiAppearance::ToLogicalX(600,1920,1080)==400);
 assert(UiAppearance::ToLogicalY(375,1920,1080)==250);
 // At 1440x900 the effective scale is 125%, protecting the home screen.
 assert(UiAppearance::LogicalWidth(1440,900)==1152);
 assert(UiAppearance::LogicalHeight(1440,900)==720);
 // Small windows clamp zoom so controls remain available.
 assert(UiAppearance::LogicalWidth(760,540)>=720);
 assert(UiAppearance::LogicalHeight(760,540)>=540);
 UiAppearance::Transform(r,g,b);
 assert(r!=70 || g!=90 || b!=110);
 UiAppearance::Configure(100,2);
 int dark=45,mid=45,bright=45;
 UiAppearance::Transform(dark,mid,bright);
 assert(dark<45 && mid<45 && bright<45);
 UiAppearance::Configure(10,5);
 assert(UiAppearance::Scale()==75 && UiAppearance::Theme()==2);
 puts("PASS: escalado lógico, ratón, límites y tres temas.");
}
