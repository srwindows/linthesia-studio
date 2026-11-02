#include "LibraryLayout.h"
#include <cassert>
#include <iostream>
int main(){
 using LibraryLayout::Compute;
 // Files alone fill the list area. No MIDI is rendered as a folder button.
 auto files=Compute(1280,720,0,18);
 assert(files.columns==2);
 assert(files.fileRows>=10);
 assert(files.rowHeight==30);
 assert(files.filePages>=2);
 assert(files.folderPages==1);
 assert(files.listTop==files.listHeadingY);
 // Folder cards remain 510x80, adaptive rows give space to MIDI files.
 auto mixed=Compute(1280,720,5,15);
 assert(mixed.columns==2);
 assert(mixed.folderPageSize>=2);
 assert(mixed.folderPages>=2);
 assert(mixed.fileRows>=4);
 assert(mixed.filePages>=2);
 assert(mixed.listTop>126+80);
 // Don't leave two empty folder rows above the list for short folders.
 auto shortmixed=Compute(1280,720,1,3);
 assert(shortmixed.listHeadingY< mixed.listHeadingY);
 assert(shortmixed.fileRows>=mixed.fileRows);
 // Small windows stack folders in one column, MIDI stays in rows.
 auto narrow=Compute(800,600,7,14);
 assert(narrow.columns==1);
 assert(narrow.folderPageSize>=1);
 assert(narrow.fileRows>=2);
 assert(narrow.filePages>1);
 assert(narrow.listLeft+narrow.listWidth<=800);
 assert(narrow.listTop==narrow.listHeadingY+8);
 // Folder-only libraries keep their original full-height grid.
 auto folders=Compute(1280,720,7,0);
 assert(folders.folderPageSize>mixed.folderPageSize);
 // Exactly matching capacity does not introduce an empty extra page.
 auto exact=Compute(1280,720,4,0);
 assert(exact.folderPages==1);
 // Tall screens show more files without adding button-style columns.
 auto tall=Compute(1920,1080,2,30);
 assert(tall.fileRows>mixed.fileRows);
 assert(tall.listLeft+tall.listWidth<=1920);
 // Child folders are compact buttons next to the actual path and wrap
 // to further upper rows when necessary; never overlap text, each other,
 // or the unmodified single-name-per-row MIDI list below them.
 for(int width:{640,800,1024,1280,1920}){
  for(int height:{480,600,720,1080}){
   for(int count:{0,1,2,6,20,60}){
    for(int pathLen:{10,25,75}){
     auto c=LibraryLayout::Compact(width,height,count,15,pathLen);
     assert(c.buttonWidth<=138 && c.buttonHeight==34);
     assert(c.buttonWidth+32<=width);
     assert(c.pageSize>0 && c.pageCount>=1);
     assert(c.fileRows>=1 && c.filePages>=1);
     if(count>c.pageSize){
       assert(LibraryLayout::CompactListTop(c,1) <=
              LibraryLayout::CompactListTop(c,c.pageSize));
     }
     assert(c.listTop+30<=height-86);
     const int visible=std::min(count,c.pageSize);
     for(int j=0;j<visible;++j){
      auto pos=c.Position(j);
      assert(pos.x>=16 && pos.x+c.buttonWidth<=width-16);
      assert(pos.y>=16 && pos.y+c.buttonHeight<c.listTop);
      if(j<c.firstCapacity)
        assert(pos.x>=224+c.pathWidth+12);
      for(int k=0;k<j;++k){
       auto prev=c.Position(k);
       assert(pos.x>=prev.x+c.buttonWidth ||
              prev.x>=pos.x+c.buttonWidth ||
              pos.y>=prev.y+c.buttonHeight ||
              prev.y>=pos.y+c.buttonHeight);
      }
     }
    }
   }
  }
 }
 // Column major MIDI filling: exhaust one column before the next.
 for(int w:{640,800,1280,1920}){
   for(int h:{480,600,720,1080}){
     for(int count:{0,10,130}){
       auto layout=LibraryLayout::Files(w,h,132,count);
       assert(layout.columns>=1 && layout.rows>=1);
       assert(layout.capacity==layout.columns*layout.rows);
       assert(layout.pages==std::max(1,(count+layout.capacity-1)/layout.capacity));
       for(int i=0;i<layout.capacity;++i){
          auto slot=layout.Position(i);
          assert(slot.x>=24 && slot.x+layout.columnWidth<=w-24);
          assert(slot.y>=132);
          assert(slot.y+layout.rowHeight<=h-102);
          if(i>0){
             auto prev=layout.Position(i-1);
             if(i%layout.rows!=0){
                assert(slot.x==prev.x);
                assert(slot.y==prev.y+layout.rowHeight);
             }else{
                assert(slot.x>prev.x);
                assert(slot.y==132);
             }
          }
       }
     }
   }
 }
 std::cout<<"MIDI_LIST_LAYOUT_OK: folders/buttons, file/rows, separate pages, "
             "800/1280/1920 dimensions\n";
}
