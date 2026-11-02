#ifndef LINTHESIA_DRAG_PAGE_NAVIGATION_H
#define LINTHESIA_DRAG_PAGE_NAVIGATION_H

#include <algorithm>
namespace DragPageNavigation {
struct Rect { int x=0,y=0,w=0,h=0; };
enum class Target {
    None=0, FolderPrevious=1, FolderNext=2,
    MidiPrevious=3, MidiNext=4
};
enum class DragKind { Folder, Midi };
struct State {
    int width=0,height=0,fileListTop=0;
    int folderPage=0,folderPages=1,midiPage=0,midiPages=1;
    Rect folderPrevious,folderNext,midiPrevious,midiNext;
};
inline bool Inside(const Rect& r,int x,int y) {
    return r.w>0 && r.h>0 &&
           x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h;
}
inline bool NearbyPager(const Rect& prev,const Rect& next,int x,int y) {
    const int l=std::min(prev.x,next.x)-30;
    const int t=std::min(prev.y,next.y)-27;
    const int r=std::max(prev.x+prev.w,next.x+next.w)+30;
    const int b=std::max(prev.y+prev.h,next.y+next.h)+27;
    return x>=l && x<r && y>=t && y<b;
}
inline Target Pager(const Rect& prev,const Rect& next,int x,
                    int page,int pages,bool midi) {
    const int midpoint=(prev.x+prev.w/2 + next.x+next.w/2)/2;
    if(x<midpoint)
        return page>0?(midi?Target::MidiPrevious:
                            Target::FolderPrevious):Target::None;
    return page+1<pages?(midi?Target::MidiNext:
                             Target::FolderNext):Target::None;
}
inline Target Hit(const State& s,int x,int y,
                  DragKind kind=DragKind::Midi) {
    // Priorizar el grupo de navegacion visible. Se admite tambien el
    // texto "Pagina N de M" y un margen de error alrededor de los botones.
    if(s.folderPages>1 &&
       NearbyPager(s.folderPrevious,s.folderNext,x,y))
        return Pager(s.folderPrevious,s.folderNext,x,s.folderPage,
                     s.folderPages,false);
    if(kind==DragKind::Midi && s.midiPages>1 &&
       NearbyPager(s.midiPrevious,s.midiNext,x,y))
        return Pager(s.midiPrevious,s.midiNext,x,s.midiPage,
                     s.midiPages,true);

    // Lateral: 70 pixeles es mas facil de alcanzar con un grupo flotante.
    // En ningun caso debe activarse paginacion al tocar el escritorio.
    if(s.width<160 || s.height<220 || y<89 || y>=s.height-82)
        return Target::None;
    const int direction=x<70?-1:(x>=s.width-70?1:0);
    if(!direction)return Target::None;

    // Carpetas siempre cambian las paginas de carpetas, aun cuando pasan
    // por encima de la zona de archivos MIDI.
    if(kind==DragKind::Folder ||
       (y<s.fileListTop && s.folderPages>1)) {
        if(direction<0 && s.folderPage>0)return Target::FolderPrevious;
        if(direction>0 && s.folderPage+1<s.folderPages)
            return Target::FolderNext;
        return Target::None;
    }

    // Archivos arrastrados sobre la lista cambian las paginas MIDI.
    if(direction<0 && s.midiPage>0)return Target::MidiPrevious;
    if(direction>0 && s.midiPage+1<s.midiPages)return Target::MidiNext;
    return Target::None;
}
}
#endif
