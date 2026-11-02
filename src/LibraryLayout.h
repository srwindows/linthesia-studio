#ifndef LINTHESIA_LIBRARY_LAYOUT_H
#define LINTHESIA_LIBRARY_LAYOUT_H
#include <algorithm>

// Pure geometry: keep folder buttons unchanged, display MIDI as separate rows.
namespace LibraryLayout {
// Small child-folder buttons share the line with Volver + the current path.
// Remaining buttons wrap to subsequent rows without clipping or overlapping.
// This mode is used INSIDE a folder, not on the main folder screen.
struct CompactFolderLayout {
 int buttonWidth=138,buttonHeight=34,gap=8;
 int firstY=26,wrapY=122,rowStep=41;
 int pathWidth=0,firstX=0,firstCapacity=0,wrapColumns=1;
 int pageSize=1,pageCount=1,listTop=114,fileRows=1,filePages=1;
 int rowHeight=30;

 struct Point {int x,y;};
 Point Position(int localIndex) const{
  if(localIndex<firstCapacity)
   return {firstX+localIndex*(buttonWidth+gap),firstY};
  const int wrapped=localIndex-firstCapacity;
  return {16+(wrapped%wrapColumns)*(buttonWidth+gap),
          wrapY+(wrapped/wrapColumns)*rowStep};
 }
};
inline CompactFolderLayout Compact(int width,int height,int folders,int files,
                                   int pathCharacters){
 CompactFolderLayout g{};
 g.buttonWidth=std::min(138,std::max(64,width-32));
 g.pathWidth=std::min(std::max(0,pathCharacters)*8,
                      std::min(260,std::max(64,width/3)));
 g.firstX=16+176+32+g.pathWidth+12;
 const int insideRight=width-270;
 if(g.firstX+g.buttonWidth<=insideRight)
  g.firstCapacity=1+(insideRight-g.firstX-g.buttonWidth)/
                       (g.buttonWidth+g.gap);
 g.wrapColumns=std::max(1,(width-32+g.gap)/
                             (g.buttonWidth+g.gap));
 const int bottom=std::max(150,height-102);
 const int reserved=files>0?
       std::min(180,std::max(70,(height-220)/2)):0;
 const int lastBottom=std::max(118,bottom-reserved);
 const int wrapRows=(lastBottom>=g.wrapY+g.buttonHeight)?
       1+(lastBottom-g.wrapY-g.buttonHeight)/g.rowStep:0;
 g.pageSize=std::max(1,g.firstCapacity+g.wrapColumns*wrapRows);
 g.pageCount=std::max(1,(folders+g.pageSize-1)/g.pageSize);
 // Place the file list immediately below the actually displayed child buttons.
 const int visible=std::min(folders,g.pageSize);
 if(visible>0){
   const auto last=g.Position(visible-1);
   g.listTop=std::max(114,last.y+g.buttonHeight+18);
 }
 g.fileRows=std::max(1,(bottom-g.listTop)/g.rowHeight);
 g.filePages=std::max(1,(files+g.fileRows-1)/g.fileRows);
 return g;
}
inline int CompactListTop(const CompactFolderLayout& g,int visibleCount){
 if(visibleCount<=0)return 114;
 const auto last=g.Position(std::min(g.pageSize,visibleCount)-1);
 return std::max(114,last.y+g.buttonHeight+18);
}
// MIDI fill downwards, then advance to the next contrasting column.
struct MidiGrid {
 int columns=1,rows=1,columnWidth=220,gap=20,rowHeight=30;
 int left=24,top=114,capacity=1,pages=1;
 struct Slot {int x,y,column;};
 Slot Position(int position)const {
  const int col=position/rows, row=position%rows;
  return {left+col*(columnWidth+gap),top+row*rowHeight,col};
 }
};
inline MidiGrid Files(int width,int height,int listTop,int fileCount){
 MidiGrid g{};
 g.top=listTop;
 const int available=std::max(100,width-48);
 g.columns=std::max(1,std::min(10,(available+20)/(210+20)));
 g.gap=20;
 g.columnWidth=(available-g.gap*(g.columns-1))/g.columns;
 g.rows=std::max(1,(height-102-g.top)/g.rowHeight);
 g.capacity=std::max(1,g.columns*g.rows);
 g.pages=std::max(1,(fileCount+g.capacity-1)/g.capacity);
 return g;
}
struct Geometry {
 int columns,folderRows,folderPageSize,folderPages;
 int listHeadingY,listTop,listLeft,listWidth;
 int fileRows,filePages,rowHeight;
};
inline Geometry Compute(int width,int height,int folders,int files,
                        int folderWidth=510,int folderHeight=80){
 const int margin=16,top=126,bottom=height-102,gap=32;
 const int step=folderHeight+20;
 Geometry g{};
 g.columns=std::max(1,(width-2*margin+gap)/(folderWidth+gap));
 const int maxRows=std::max(1,(bottom-top+20)/step);
 if(files>0 && folders>0){
    // Reserve a meaningful lower section for the file list.
    const int reserve=std::max(150,(bottom-top)*50/100);
    g.folderRows=std::max(1,std::min(maxRows,(bottom-top-reserve)/step));
 } else g.folderRows=maxRows;
 g.folderPageSize=std::max(1,g.columns*g.folderRows);
 g.folderPages=std::max(1,(folders+g.folderPageSize-1)/g.folderPageSize);
 g.listLeft=margin+8;
 g.listWidth=std::max(100,width-2*g.listLeft);
 g.rowHeight=30;
 const int visibleFolderRows=std::min(g.folderRows,
                    (folders+g.columns-1)/g.columns);
 g.listHeadingY=folders>0?top+visibleFolderRows*step:top;
 g.listTop=folders>0?g.listHeadingY+8:top;
 g.fileRows=std::max(1,(bottom-g.listTop)/g.rowHeight);
 g.filePages=std::max(1,(files+g.fileRows-1)/g.fileRows);
 return g;
}
}
#endif
