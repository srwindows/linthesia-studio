#ifndef LINTHESIA_UI_APPEARANCE_H
#define LINTHESIA_UI_APPEARANCE_H
namespace UiAppearance {
 void Configure(int scale,int theme);
 int Scale();
 int EffectiveScaleFor(int selected,int physicalWidth,int physicalHeight);
 int Theme();
 int LogicalWidth(int physicalWidth,int physicalHeight);
 int LogicalHeight(int physicalWidth,int physicalHeight);
 int ToLogicalX(int physicalX,int physicalWidth,int physicalHeight);
 int ToLogicalY(int physicalY,int physicalWidth,int physicalHeight);
 void Transform(int& red,int& green,int& blue);
}
#endif
