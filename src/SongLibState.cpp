// -*- mode: c++; coding: utf-8 -*-

// Linthesia

#include "SongLibState.h"
#include "Textures.h"
#include "UserSettings.h"
#include "MenuLayout.h"
#include "TitleState.h"
#include "LinthesiaError.h"
#include "TextWriter.h"
#include "FolderOrdering.h"
#include "DragPageNavigation.h"
#include "RecentSongs.h"
#include <glib.h>
#include <gio/gio.h>
#include <cstring>
#include <set>
#include <algorithm>
#include <limits.h>
#include <cstdlib>
#include <cerrno>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "libmidi/Midi.h"
#include "MusicXmlImport.h"
#include "SongMetadata.h"

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace {
std::string shortName(const std::string& p){
 return p.substr(p.find_last_of('/')+1);
}
bool contained(const std::string& base,const std::string& child){
 char* base_real=realpath(base.c_str(),nullptr);
 char* child_real=realpath(child.c_str(),nullptr);
 if(!base_real || !child_real){free(base_real);free(child_real);return false;}
 const std::string b(base_real),c(child_real);
 free(base_real);free(child_real);
 return c==b || (c.size()>b.size() &&
     c.compare(0,b.size(),b)==0 && c[b.size()]=='/');
}
}
SongLibState::~SongLibState(){CancelMidiImport();}
void SongLibState::Init() {
    m_preferences=AdvancedSettingsIO::Load();
    m_back_button=ButtonState();
    m_import_button=ButtonState();
    m_create_folder_button=ButtonState();
    m_prev_page_button=ButtonState();
    m_next_page_button=ButtonState();
    m_path_up_button=ButtonState();

    // Crear la carpeta de importación al abrir la biblioteca si aún no existe.
    // Conserva el nombre físico 1 y no toca ningún contenido existente.
    if(m_personal.Ready()){
        const std::string fixed=m_personal.Root()+"/1";
        struct stat check{};
        if(lstat(fixed.c_str(),&check)!=0 && errno==ENOENT &&
           mkdir(fixed.c_str(),0700)!=0)
          g_warning("Linthesia: no se puede preparar Añadidos: %s",
                    strerror(errno));
    }
    m_base_path=UserSetting::Get(SONG_LIB_PATH_KEY,MUSICDIR);
    m_placements.reset(new FolderPlacements(m_base_path,m_personal.Root()));
    m_current_path=m_preferences.rememberFolder?
       UserSetting::Get(SONG_LIB_DIR_SETTINGS_KEY,m_base_path):m_base_path;
    if(m_current_path!=RecentFolderPath() &&
       !contained(m_base_path,m_current_path) &&
       !m_personal.Owns(m_current_path))
        m_current_path=m_base_path;
    m_current_page=0;
    m_page_count=1;
    m_columns=1;
    m_tiles_per_page=1;
    m_file_page=0;
    Resize();
    UpdateSongTiles();
}

int SongLibState::ContentLeft() {
    int columns_margins = ColumnMargin * (m_columns - 1);
    int columns_content = SongTileWidth * m_columns;
    
    int content_left_slim  = (GetStateWidth()  - SongTileWidth) / 2;

    int content_left_wide  = (GetStateWidth()  - columns_content - columns_margins) / 2;

    return (m_columns > 1) ? content_left_wide  : content_left_slim;
}

int SongLibState::ContentRight() {
    int columns_margins = ColumnMargin * (m_columns - 1);
    int columns_content = SongTileWidth * m_columns;
    
    int content_left_slim  = (GetStateWidth()  - SongTileWidth) / 2;
    int content_right_slim = content_left_slim + SongTileWidth;

    int content_left_wide  = (GetStateWidth()  - columns_content - columns_margins) / 2;
    int content_right_wide = content_left_wide + columns_content + columns_margins;

    return (m_columns > 1) ? content_right_wide : content_right_slim;
}

bool hasEnding (string fullString, string ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool isMidiFile(string f) {
    const auto dot=f.find_last_of('.');
    if(dot==std::string::npos)return false;
    const std::string ext=f.substr(dot);
    return g_ascii_strcasecmp(ext.c_str(),".mid")==0 ||
           g_ascii_strcasecmp(ext.c_str(),".midi")==0 ||
           g_ascii_strcasecmp(ext.c_str(),".rmi")==0 ||
           g_ascii_strcasecmp(ext.c_str(),".kar")==0 ||
           MusicXmlImport::IsXml(f);
}

string eraseSubstring(string subj, string erase) {
    std::string t = subj;
    std::string s = erase;
    std::string::size_type i = t.find(s);

    if (i != std::string::npos)
        t.erase(i, s.length());
    
    return t;
}
// https://stackoverflow.com/questions/1380463/sorting-a-vector-of-custom-objects
struct songtile_less_than_key
{
    inline bool operator() (const SongTile& struct1, const SongTile& struct2)
    {
        return (struct1.GetSortPath() < struct2.GetSortPath());
    }
};

void SongLibState::UpdateSongTiles() {
    m_selected_folders.clear();
    m_folder_anchor.clear();
    m_last_clicked_folder.clear();
    m_last_folder_click=0;
    m_folder_rect_candidate=false;
    m_folder_rect_selecting=false;
    m_folder_before_rect.clear();
    m_folder_drag_files.clear();
    m_folder_drag_origin.clear();
    m_folder_hover_target.clear();
    m_folder_hover_started=0;
    m_folder_drag_navigations=0;
    m_selected_midis.clear();
    m_selection_anchor.clear();
    m_last_clicked_midi.clear();
    m_last_midi_click=0;
    m_midi_drag_candidate=false;
    m_midi_select_dragging=false;
    m_midi_move_candidate=false;
    m_midi_move_dragging=false;
    m_midi_moving_files.clear();
    m_midi_drag_source_folder.clear();
    m_midi_hover_destination.clear();
    m_midi_hover_started=0;
    m_midi_navigation_steps=0;
    m_midi_before_drag.clear();
    m_song_tiles.clear();
    Tga* songs=GetTexture(SongBox);
    Tga* folders=GetTexture(DirBox);
    std::set<std::string> seen;
    auto scan=[&](const std::string& folder){
        DIR* dir=opendir(folder.c_str());
        if(!dir)return false;
        struct dirent* entry;
        while((entry=readdir(dir))){
            const std::string name(entry->d_name);
            if(name=="." || name==".." || name[0]=='.')continue;
            if(seen.count(name))continue;
            const std::string path=folder+"/"+name;
            // Hide a folder from its physical parent when visually relocated.
            if(m_placements && path!=RecentFolderPath() &&
               !m_placements->Parent(path).empty())continue;
            struct stat st{};
            if(lstat(path.c_str(),&st)!=0)continue;
            const bool is_dir=S_ISDIR(st.st_mode);
            if(!is_dir && !(S_ISREG(st.st_mode) && isMidiFile(name)))continue;
            seen.insert(name);
            std::string title=is_dir?NameFor(path):name;
            if(!is_dir){
              const auto info=SongMetadata::Load(path);
              if(!info.title.empty())title=info.title;
            }
            m_song_tiles.push_back(SongTile(0,0,path,title,is_dir,
                                            is_dir?folders:songs));
        }
        closedir(dir);
        return true;
    };
    if(m_current_path==RecentFolderPath()){
        // Vista virtual: referencias de lectura a archivos MIDI existentes.
        for(const auto& item:RecentSongs::List(m_preferences.recentLimit,
                                               m_preferences.recentSort)){
            const auto info=SongMetadata::Load(item.path);
            m_song_tiles.push_back(SongTile(0,0,item.path,
                     info.title.empty()?shortName(item.path):info.title,
                     false,songs));
        }
    }else if(!scan(m_current_path))
        throw LinthesiaError("No se puede abrir la carpeta: "+m_current_path);
    if(m_current_path==m_base_path && m_personal.Ready())
       scan(m_personal.Root());
    // Display linked source folders as ordinary tiles alongside all MIDI
    // already present in the destination directory.
    if(m_placements && m_current_path!=RecentFolderPath()){
        for(const auto& path:m_placements->Children(m_current_path)){
            if(path==RecentFolderPath())continue;
            const std::string name=shortName(path);
            if(seen.count(name))continue;
            seen.insert(name);
            m_song_tiles.push_back(SongTile(0,0,path,NameFor(path),true,folders));
        }
    }
    // Si el usuario ya tiene una carpeta física Recientes, reutilizar su
    // tarjeta. Los ficheros preexistentes NO se tocan ni se muestran aquí.
    if(m_current_path==m_base_path && !seen.count("Recientes"))
      m_song_tiles.push_back(SongTile(0,0,RecentFolderPath(),"Recientes",
                                      true,folders));
    if(m_current_path!=RecentFolderPath())
      std::sort(m_song_tiles.begin(),m_song_tiles.end(),songtile_less_than_key());
    const auto saved=m_current_path==RecentFolderPath()?
                     std::vector<std::string>():
                     m_personal.ReadOrder(m_current_path);
    // Los intercambios manuales guardados deben prevalecer sobre el orden
    // alfabético general. Si no hay orden guardado, se aplica el alfabético.
    if(!saved.empty()){
        std::map<std::string,int> ranks;
        for(size_t i=0;i<saved.size();++i)
            if(ranks.find(saved[i])==ranks.end())
                ranks[saved[i]]=static_cast<int>(i);
        const auto rank=[&](const SongTile& t){
            const std::string name=shortName(t.GetPath());
            const auto it=ranks.find(name);
            return it==ranks.end()?static_cast<int>(saved.size())+1:it->second;
        };
        std::stable_sort(m_song_tiles.begin(),m_song_tiles.end(),
          [&](const SongTile& x,const SongTile& y){return rank(x)<rank(y);});
    }
    // One collection, two visual zones: folder buttons first, MIDI list last.
    // Alphabetize files without changing the custom order of folders.
    if(m_current_path!=RecentFolderPath())
    std::stable_sort(m_song_tiles.begin(),m_song_tiles.end(),
      [](const SongTile& a,const SongTile& b){
        if(a.IsDir()!=b.IsDir())return a.IsDir();
        if(a.IsDir())return false;
        const std::string nameA=shortName(a.GetPath());
        const std::string nameB=shortName(b.GetPath());
        gchar* fa=g_utf8_casefold(nameA.c_str(),-1);
        gchar* fb=g_utf8_casefold(nameB.c_str(),-1);
        const int compare=g_strcmp0(fa,fb);
        g_free(fa);g_free(fb);
        return compare<0;
      });
    std::stable_partition(m_song_tiles.begin(),m_song_tiles.end(),
                          [](const SongTile& tile){return tile.IsDir();});
    if(m_current_path==m_base_path){
        auto pinned=std::find_if(m_song_tiles.begin(),m_song_tiles.end(),
             [&](const SongTile& tile){
                return tile.GetPath()==m_personal.Root()+"/1";});
        if(pinned!=m_song_tiles.end())
            std::rotate(m_song_tiles.begin(),pinned,pinned+1);
        auto recent=std::find_if(m_song_tiles.begin(),m_song_tiles.end(),
           [&](const SongTile& tile){return tile.GetPath()==RecentFolderPath();});
        if(recent!=m_song_tiles.end() && m_song_tiles.size()>1)
           std::rotate(m_song_tiles.begin()+1,recent,recent+1);
    }
    if(std::getenv("LINTHESIA_DRAG_TRACE")){
      std::fprintf(stderr,"[ORDER] folder=%s entries=",m_current_path.c_str());
      for(const auto& item:m_song_tiles)
        if(item.IsDir())std::fprintf(stderr,"%s;",shortName(item.GetPath()).c_str());
      std::fprintf(stderr," saved=%zu alphabetic=%d\n",saved.size(),
                   static_cast<int>(m_preferences.alphabeticOrder));
    }
    m_folder_count=static_cast<int>(std::count_if(m_song_tiles.begin(),
        m_song_tiles.end(),[](const SongTile& tile){return tile.IsDir();}));
    m_midi_count=static_cast<int>(m_song_tiles.size())-m_folder_count;
    m_file_page=0;
    Resize();
}

void SongLibState::Resize() {
    const int y=GetStateHeight()-Layout::ScreenMarginY/2-Layout::ButtonHeight/2;
    m_back_button=ButtonState(Layout::ScreenMarginX,y,
                              Layout::ButtonWidth,Layout::ButtonHeight);
    m_create_folder_button=ButtonState(Layout::ScreenMarginX+
         Layout::ButtonWidth+16,y,Layout::ButtonWidth,Layout::ButtonHeight);
    const int histX=std::max(Layout::ScreenMarginX+2*Layout::ButtonWidth+30,
                             GetStateWidth()/2-146);
    m_undo_button=ButtonState(histX,y+3,132,37);
    m_redo_button=ButtonState(histX+144,y+3,132,37);
    m_import_button=ButtonState(std::min(histX+288,GetStateWidth()-147),
                                y+3,132,37);
    const int top=(Layout::ScreenMarginY-Layout::ButtonHeight)/2;
    // Leave a dedicated line above the navigation buttons for "Página X de Y".
    // This placement applies only to the main library; the compact view
    // overrides its paging controls in UpdateSongTilesPage().
    const int pageButtonsY=top+14;
    m_prev_page_button=ButtonState(GetStateWidth()-2*Layout::ButtonWidth-48,
                                   pageButtonsY,Layout::ButtonWidth,Layout::ButtonHeight);
    m_next_page_button=ButtonState(GetStateWidth()-Layout::ButtonWidth-16,
                                   pageButtonsY,Layout::ButtonWidth,Layout::ButtonHeight);
    m_path_up_button=ButtonState(Layout::ScreenMarginX,top,
                                 Layout::ButtonWidth,Layout::ButtonHeight);
    UpdateSongTilesPage();
}

std::string SongLibState::RecentFolderPath()const{
 // Reutilizar Recientes preexistente como acceso virtual, sin alterar
 // absolutamente nada de su contenido físico.
 const std::string existing=m_personal.Root()+"/Recientes";
 struct stat entry{};
 if(lstat(existing.c_str(),&entry)==0 && S_ISDIR(entry.st_mode))
     return existing;
 // En una biblioteca nueva no se necesita crear un directorio.
 return m_personal.Root()+"/.linthesia-virtual-recientes";
}
bool SongLibState::FixedFolder(const std::string& path)const{
 return path==RecentFolderPath() ||
        MidiImporter::IsFixedFolder(path,m_personal.Root());
}
bool SongLibState::RecentGearHit(int x,int y) const{
    if(m_current_path!=m_base_path)return false;
    for(int i=0;i<m_folder_count;++i){
        const auto& tile=m_song_tiles[i];
        if(!tile.IsVisible()||tile.GetPath()!=RecentFolderPath())continue;
        const auto bounds=tile.WholeTile();
        const int side=36;
        const int left=tile.GetX()+bounds.w-side-11;
        const int top=tile.GetY()+(bounds.h-side)/2;
        return x>=left && x<left+side && y>=top && y<top+side;
    }
    return false;
}
void SongLibState::OpenRecentOptions(){
 const AdvancedSettings latest=AdvancedSettingsIO::Load();
 m_recent_limit_draft=latest.recentLimit;
 m_recent_sort_draft=latest.recentSort;
 m_recent_dropdown=0;
 m_recent_gear_pressed=false;
 m_last_clicked_folder.clear();
 m_recent_options_open=true;
 m_context_open=false;
 m_notice.clear();m_error.clear();
}
void SongLibState::CancelMidiImport(){
 if(m_import_pid){
   kill(m_import_pid,SIGTERM);
   int status=0;
   (void)waitpid(m_import_pid,&status,WNOHANG);
   g_spawn_close_pid(m_import_pid);
   m_import_pid=0;
 }
 if(m_import_stdout>=0){close(m_import_stdout);m_import_stdout=-1;}
 m_import_output.clear();
}
void SongLibState::StartMidiImport(){
 if(m_import_job && !m_import_job->done.load()){
    m_notice="La importación anterior sigue en curso.";return;
 }
 if(m_import_pid){m_notice="El explorador de importación ya está abierto.";return;}
 gchar* base=SDL_GetBasePath();
 if(!base){m_error="No se puede localizar el explorador de archivos.";return;}
 const std::string directory(base);SDL_free(base);
 std::string helper;
 for(const auto& tryPath:std::vector<std::string>{
      directory+"../share/linthesia/scripts/midi_import_picker.py",
      directory+"../../src/midi_import_picker.py"}){
   if(access(tryPath.c_str(),R_OK)==0){helper=tryPath;break;}
 }
 if(helper.empty()){m_error="Falta el explorador MIDI del programa.";return;}
 gchar* argv[]={const_cast<gchar*>("python3"),
                 const_cast<gchar*>(helper.c_str()),nullptr};
 GError* problem=nullptr;
 GPid pid=0;int fd=-1;
 const bool ok=g_spawn_async_with_pipes(nullptr,argv,nullptr,
        static_cast<GSpawnFlags>(G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD),
        nullptr,nullptr,&pid,nullptr,&fd,nullptr,&problem);
 if(!ok){
   m_error=problem?problem->message:"No se pudo abrir el explorador.";
   if(problem)g_error_free(problem);
   return;
 }
 m_import_pid=pid;m_import_stdout=fd;m_import_output.clear();
 const int flags=fcntl(fd,F_GETFL,0);
 if(flags>=0)fcntl(fd,F_SETFL,flags|O_NONBLOCK);
 m_error.clear();m_notice="Explorador abierto: selecciona archivos y carpetas.";
}
void SongLibState::PollMidiImport(){
 if(m_import_job && m_import_job->done.load(std::memory_order_acquire)){
    const MidiImporter::Result imported=m_import_job->result;
    m_import_job.reset();
    m_notice="Importados "+std::to_string(imported.copied)+" MIDI y "+
             std::to_string(imported.folders)+" carpetas en 1.";
    if(imported.skipped)
       m_notice+=" Ignorados: "+std::to_string(imported.skipped)+".";
    m_error=imported.errors.empty()?"":imported.errors.front();
    ClearEditHistory();
    UpdateSongTiles();
    if(m_current_path==m_base_path){m_current_page=0;UpdateSongTilesPage();}
 }
 if(!m_import_pid)return;
 char buf[4096];
 if(m_import_stdout>=0){
   for(;;){
     const ssize_t n=read(m_import_stdout,buf,sizeof(buf));
     if(n>0){
       if(m_import_output.size()<1024*1024)
         m_import_output.append(buf,std::min(static_cast<size_t>(n),
                  1024*1024-m_import_output.size()));
       continue;
     }
     if(n<0 && errno==EINTR)continue;
     break;
   }
 }
 int status=0;
 const pid_t ended=waitpid(m_import_pid,&status,WNOHANG);
 if(ended==0)return;
 if(m_import_stdout>=0){close(m_import_stdout);m_import_stdout=-1;}
 g_spawn_close_pid(m_import_pid);m_import_pid=0;
 const std::string output=m_import_output;m_import_output.clear();
 if(ended<0 || !WIFEXITED(status)||WEXITSTATUS(status)!=0){
   m_error="El explorador MIDI no se pudo ejecutar. Comprueba PySide6.";
   return;
 }
 std::vector<std::string> paths;
 size_t offset=0;
 while(offset<output.size()){
   const size_t end=output.find('\0',offset);
   if(end==std::string::npos)break;
   if(end>offset)paths.push_back(output.substr(offset,end-offset));
   offset=end+1;
 }
 if(paths.empty()){m_notice="Importación cancelada.";return;}
 std::string dest=m_personal.Root()+"/1";
 struct stat st{};
 if(lstat(dest.c_str(),&st)!=0){
    std::string outputPath,error;
    if(!m_personal.Create(m_personal.Root(),"1",outputPath,error)){
       m_error=error;return;
    }
    dest=outputPath;
 }else if(!S_ISDIR(st.st_mode)){
    m_error="La carpeta protegida 1 no es un directorio.";return;
 }
 // Copy in a worker: importing a large MIDI collection must not freeze
 // SDL rendering or block the window and its controls.
 const auto job=std::make_shared<ImportJob>();
 m_import_job=job;
 std::thread([job,paths,dest]{
    job->result=MidiImporter::Import(paths,dest);
    job->done.store(true,std::memory_order_release);
 }).detach();
 m_error.clear();
 m_notice="Importando archivos MIDI en 1... Puedes seguir utilizando Linthesia.";
}
void SongLibState::UpdateSongTilesPage() {
    m_compact_subfolders=(m_current_path!=m_base_path);
    if(m_compact_subfolders){
        const std::string path=VisibleFolderPath();
        const int chars=g_utf8_validate(path.c_str(),-1,nullptr)?
             g_utf8_strlen(path.c_str(),-1):static_cast<int>(path.size());
        m_compact_geometry=LibraryLayout::Compact(
             GetStateWidth(),GetStateHeight(),m_folder_count,m_midi_count,chars);
        const auto& g=m_compact_geometry;
        m_columns=g.wrapColumns;
        m_tiles_per_page=g.pageSize;
        m_page_count=g.pageCount;
        m_file_list_left=24;
        m_file_list_width=std::max(100,GetStateWidth()-48);
        m_current_page=std::max(0,std::min(m_current_page,m_page_count-1));
        const int begin=m_current_page*m_tiles_per_page;
        const int end=std::min(m_folder_count,begin+m_tiles_per_page);
        m_file_list_top=LibraryLayout::CompactListTop(g,end-begin);
        m_file_list_heading=m_file_list_top;
        m_midi_grid=LibraryLayout::Files(GetStateWidth(),GetStateHeight(),
                                         m_file_list_top,m_midi_count);
        m_file_rows_per_page=m_midi_grid.capacity;
        m_file_page_count=m_midi_grid.pages;
        m_file_page=std::max(0,std::min(m_file_page,m_file_page_count-1));
        for(auto& tile:m_song_tiles)tile.SetVisible(false);
        for(int i=begin;i<end;++i){
            const auto point=g.Position(i-begin);
            auto& tile=m_song_tiles[i];
            tile.SetX(point.x);
            tile.SetY(point.y);
            tile.SetHitSize(g.buttonWidth,g.buttonHeight);
            tile.SetVisible(true);
        }
        const int first=m_folder_count+m_file_page*m_file_rows_per_page;
        const int last=std::min(static_cast<int>(m_song_tiles.size()),
                                 first+m_file_rows_per_page);
        for(int i=first;i<last;++i){
            auto& file=m_song_tiles[i];
            const auto slot=m_midi_grid.Position(i-first);
            file.SetX(slot.x);
            file.SetY(slot.y);
            file.SetHitSize(m_midi_grid.columnWidth,m_midi_grid.rowHeight);
            file.SetVisible(true);
        }
        // Folder pagination stays in footer. MIDI pagination and search
        // occupy the reserved upper-right corner.
        m_prev_page_button=ButtonState(GetStateWidth()-161,
                                  GetStateHeight()-57,65,26);
        m_next_page_button=ButtonState(GetStateWidth()-86,
                                  GetStateHeight()-57,65,26);
        m_file_prev_button=ButtonState(GetStateWidth()-104,16,39,30);
        m_file_next_button=ButtonState(GetStateWidth()-57,16,39,30);
        m_search_field=ButtonState(GetStateWidth()-267,58,188,30);
        m_search_button=ButtonState(GetStateWidth()-75,58,59,30);
        return;
    }
    const auto g=LibraryLayout::Compute(GetStateWidth(),GetStateHeight(),
                                        m_folder_count,m_midi_count,
                                        SongTileWidth,SongTileHeight);
    m_columns=g.columns;
    m_tiles_per_page=g.folderPageSize;
    m_page_count=g.folderPages;
    m_file_page_count=g.filePages;
    m_file_rows_per_page=g.fileRows;
    m_file_list_heading=g.listHeadingY;
    m_file_list_top=g.listTop;
    m_file_list_left=g.listLeft;
    m_file_list_width=g.listWidth;
    m_current_page=std::max(0,std::min(m_current_page,m_page_count-1));
    m_file_page=std::max(0,std::min(m_file_page,m_file_page_count-1));
    for(auto& tile:m_song_tiles)tile.SetVisible(false);
    const int folderBegin=m_current_page*m_tiles_per_page;
    const int folderEnd=std::min(m_folder_count,folderBegin+m_tiles_per_page);
    const int gridWidth=m_columns*SongTileWidth+(m_columns-1)*ColumnMargin;
    const int left=(GetStateWidth()-gridWidth)/2;
    const int yTop=Layout::ScreenMarginY+SongTileHeight/2;
    for(int i=folderBegin;i<folderEnd;++i){
        const int local=i-folderBegin;
        auto& tile=m_song_tiles[i];
        tile.SetX(left+(local%m_columns)*(SongTileWidth+ColumnMargin));
        tile.SetY(yTop+(local/m_columns)*(SongTileHeight+20));
        tile.SetHitSize(SongTileWidth,SongTileHeight);
        tile.SetVisible(true);
    }
    const int fileBegin=m_folder_count+m_file_page*m_file_rows_per_page;
    const int fileEnd=std::min(static_cast<int>(m_song_tiles.size()),
                              fileBegin+m_file_rows_per_page);
    for(int i=fileBegin;i<fileEnd;++i){
        auto& file=m_song_tiles[i];
        file.SetX(m_file_list_left);
        file.SetY(m_file_list_top+(i-fileBegin)*g.rowHeight);
        file.SetHitSize(m_file_list_width,g.rowHeight);
        file.SetVisible(true);
    }
    // File pagination stays in the existing footer, away from folder
    // navigation and away from the filename list.
    const int navY=GetStateHeight()-Layout::ScreenMarginY/2-
                   Layout::ButtonHeight/2+6;
    m_file_prev_button=ButtonState(GetStateWidth()-243,navY,95,34);
    m_file_next_button=ButtonState(GetStateWidth()-132,navY,95,34);
}

bool SongLibState::CanCreateHere() const {
    if(m_current_path==RecentFolderPath())return false;
    return m_personal.Ready() && m_placements &&
         (contained(m_base_path,m_current_path) ||
          m_personal.WritableFolder(m_current_path));
}

std::string SongLibState::VisibleFolderPath() const {
    if(m_current_path==RecentFolderPath())return "Recientes";
    if(m_current_path==m_base_path)return "";
    std::vector<std::string> names;
    std::set<std::string> visited;
    std::string current=m_current_path;
    for(int i=0;i<64 && !current.empty();++i){
        if(current==m_base_path||current==m_personal.Root())break;
        if(!visited.insert(current).second)break;
        names.push_back(NameFor(current));
        const std::string linked=m_placements?m_placements->Parent(current):"";
        if(!linked.empty()){
            current=linked;
        }else{
            const size_t slash=current.find_last_of('/');
            if(slash==std::string::npos)break;
            const std::string physical=current.substr(0,slash);
            if(!contained(m_base_path,physical) &&
               !m_personal.Owns(physical))
                break;
            current=physical;
        }
    }
    std::reverse(names.begin(),names.end());
    std::string path;
    for(const auto& name:names)path+="/"+name;
    return path;
}

void SongLibState::StartCreateFolder(){
    if(!CanCreateHere()){
       m_error="No se puede crear una subcarpeta en esta ubicación.";
       return;
    }
    m_folder_name.clear();
    m_error.clear();
    m_editing_name=true;
    SDL_StartTextInput();
}

void SongLibState::StartRenameFolder(){
 const std::string source=m_context_path;
 m_context_open=false;
 if(FixedFolder(source)){
   m_error="Esta carpeta tiene nombre fijo.";return;
 }
 if(!m_personal.OwnedChild(source) && !contained(m_base_path,source)){
   m_error="No se puede renombrar esta carpeta.";return;
 }
    m_rename_path=source;
    m_rename_original_name=NameFor(source);
    m_folder_name=NameFor(source);
    m_renaming_folder=true;
 m_editing_name=true;
 m_error.clear();m_notice.clear();
 SDL_StartTextInput();
}

void SongLibState::RenameOrderEntry(const std::string& parent,
                                    const std::string& oldName,
                                    const std::string& newName){
 auto names=m_personal.ReadOrder(parent);
 bool changed=false;
 for(auto& name:names){
   if(name==oldName){name=newName;changed=true;}
 }
 if(changed){
   std::string error;
   if(!m_personal.SaveOrder(parent,names,error))
     m_notice+=" (no se pudo actualizar el orden guardado)";
 }
}

bool SongLibState::RenameFolder(const std::string& source,
                                const std::string& newName,
                                std::string& result,std::string& error){
 if(FixedFolder(source)){
   error="Esta carpeta tiene nombre fijo.";return false;
 }
 if(!m_personal.OwnedChild(source)){
   if(contained(m_base_path,source) && m_placements){
     if(!m_placements->SetDisplayName(source,newName,error))return false;
     result=source;
     return true;
   }
   error="Esta carpeta no se puede renombrar.";return false;
 }
 if(newName==shortName(source)){result=source;return true;}
 if(!m_personal.Rename(source,newName,result,error))return false;
 // La ruta fisica puede estar vinculada a otra zona de la biblioteca.
 // Si falla actualizar las vinculaciones, devolver su nombre anterior.
 if(m_placements && !m_placements->RemapAfterRename(source,result,error)){
   std::string restored,rollbackError;
   if(!m_personal.Rename(result,shortName(source),restored,rollbackError))
     error+="; ADVERTENCIA: fallo al restaurar el nombre anterior: "+rollbackError;
   return false;
 }
 return true;
}

std::string& SongLibState::MetaTextField(){
 switch(m_meta_field){
  case 0:return m_meta_entry.title;
  case 1:return m_meta_entry.subtitle;
  case 2:return m_meta_entry.composer;
  case 3:return m_meta_entry.arranger;
  case 4:return m_meta_entry.copyright;
  case 5:return m_meta_entry.license;
  default:return m_meta_entry.tags;
 }
}
void SongLibState::OpenMetaEditor(){
 const std::string song=!m_selected_midis.empty()?*m_selected_midis.begin():
                        m_last_clicked_midi;
 if(song.empty()||SongMetadata::Identity(song).empty()){
   m_notice="Selecciona un archivo MIDI o MusicXML antes de pulsar F10";
   return;
 }
 m_meta_entry=SongMetadata::Load(song);
 m_meta_path=song;
 m_meta_field=0;
 m_meta_editing=true;
 SDL_StartTextInput();
}
void SongLibState::CloseMetaEditor(bool save){
 if(save){
   if(SongMetadata::Save(m_meta_path,m_meta_entry)){
     m_notice="Metadatos guardados para "+shortName(m_meta_path);
   }else m_error="No se pudieron guardar los metadatos";
 }
 SDL_StopTextInput();
 m_meta_editing=false;
 m_meta_path.clear();
 if(save)UpdateSongTiles();
}
void SongLibState::OnTextInput(const std::string& text){
    if(m_meta_editing){
        if(m_meta_field>=7 || text.size()>120 ||
           !g_utf8_validate(text.c_str(),text.size(),nullptr))return;
        auto& value=MetaTextField();
        if(value.size()+text.size()>120)return;
        for(unsigned char ch:text)if(ch<32||ch==127)return;
        value+=text;return;
    }
    if(m_search_editing){
        if(m_search_query.size()+text.size()>120 ||
           !g_utf8_validate(text.c_str(),text.size(),nullptr))return;
        for(unsigned char c:text)if(c<32||c==127)return;
        m_search_query+=text;return;
    }
    if(!m_editing_name || m_folder_name.size()+text.size()>120 ||
       !g_utf8_validate(text.c_str(),text.size(),nullptr))return;
    for(unsigned char c:text)
       if(c<32 || c==127 || c=='/' || c=='\\')return;
    m_folder_name+=text;
}

void SongLibState::FinishCreateFolder(bool confirm){
    SDL_StopTextInput();
    m_editing_name=false;
    const bool rename=m_renaming_folder;
    m_renaming_folder=false;
    const std::string renameSource=m_rename_path;
    m_rename_path.clear();
    if(!confirm)return;
    if(rename){
        if(m_folder_name==NameFor(renameSource))return;
        for(const auto& tile:m_song_tiles){
            if(tile.GetPath()!=renameSource &&
               NameFor(tile.GetPath())==m_folder_name){
                m_error="Ya existe un elemento con ese nombre en esta carpeta.";
                return;
            }
        }
        std::string output,error;
        if(!RenameFolder(renameSource,m_folder_name,output,error)){
            m_error=error;return;
        }
        const bool virtualAlias=output==renameSource;
        if(!virtualAlias)
            RenameOrderEntry(m_current_path,shortName(renameSource),m_folder_name);
        EditAction edit;
        edit.kind=virtualAlias?EditAction::Kind::FolderAlias:
                               EditAction::Kind::FolderRename;
        // Para el alias, la etiqueta previa debe capturarse antes del cambio;
        // el diálogo empezó con ese nombre y está guardado por separado.
        edit.beforeName=m_rename_original_name;
        edit.afterName=m_folder_name;
        edit.title="Carpeta renombrada";
        edit.source=renameSource;edit.target=output;
        edit.folder=m_current_path;
        RecordEdit(edit);
        m_notice="Carpeta renombrada: "+m_folder_name;
        m_error.clear();
        UpdateSongTiles();
        for(size_t i=0;i<m_song_tiles.size();++i){
            if(m_song_tiles[i].GetPath()==output){
                m_current_page=static_cast<int>(i)/std::max(1,m_tiles_per_page);
                UpdateSongTilesPage();
                break;
            }
        }
        return;
    }
    std::string output,error;
    if(!CanCreateHere()){
        m_error="No se puede crear una carpeta en esta ubicación.";
        return;
    }
    // A folder is created inside the CURRENT visible folder, never at root
    // unless the current screen is the actual library root.
    for(const auto& tile:m_song_tiles){
        if(NameFor(tile.GetPath())==m_folder_name){
            m_error="Ya hay un elemento con ese nombre en esta carpeta.";
            return;
        }
    }
    const bool libraryRoot=(m_current_path==m_base_path);
    const bool bundled=contained(m_base_path,m_current_path) && !libraryRoot;
    const std::string destination=libraryRoot?m_personal.Root():m_current_path;
    if(bundled){
        // The bundled MIDI directory is read-only by design; create private
        // storage and attach the new folder to the bundled directory in the UI.
        if(!m_personal.CreateLinkedFolder(m_folder_name,output,error)){
            m_error=error;
            return;
        }
        if(!m_placements->Place(output,m_current_path,error)){
            m_personal.CancelLinkedFolder(output);
            m_error=error;
            return;
        }
    }else if(!m_personal.Create(destination,m_folder_name,output,error)){
        m_error=error;
        return;
    }
    ClearEditHistory(); // This creation is not reversible without deleting data.
    m_notice="Carpeta creada: "+m_folder_name;
    m_error.clear();
    UpdateSongTiles();
    for(size_t i=0;i<m_song_tiles.size();++i){
        if(m_song_tiles[i].GetPath()==output){
            m_current_page=static_cast<int>(i)/m_tiles_per_page;
            UpdateSongTilesPage();
            break;
        }
    }
}

void SongLibState::RunMidiSearch(){
    if(m_search_editing){SDL_StopTextInput();m_search_editing=false;}
    if(m_search_query.empty()){
        m_error="Escribe el nombre del MIDI que quieres buscar.";
        return;
    }
    m_search_hits=MidiSearchIndex::Search(m_base_path,m_personal.Root(),
                                          *m_placements,m_search_query);
    m_search_page=0;
    m_search_results_open=true;
    m_error.clear();
}
void SongLibState::SelectSearchHit(const MidiSearchHit& hit){
    m_search_results_open=false;
    m_current_path=hit.folderPath;
    UserSetting::Set(SONG_LIB_DIR_SETTINGS_KEY,m_current_path);
    m_current_page=0;
    m_highlight_path=hit.filePath;
    UpdateSongTiles();
    for(int i=m_folder_count;i<static_cast<int>(m_song_tiles.size());++i){
        if(m_song_tiles[i].GetPath()==hit.filePath){
            m_file_page=(i-m_folder_count)/std::max(1,m_file_rows_per_page);
            UpdateSongTilesPage();
            break;
        }
    }
    m_notice.clear();
    m_error.clear();
}
namespace {
bool pointerInside(int x,int y,int left,int top,int width,int height){
 return x>=left&&y>=top&&x<left+width&&y<top+height;
}
}
int SongLibState::TileAt(int x,int y) const{
 for(size_t i=0;i<m_song_tiles.size();++i){
  const auto& tile=m_song_tiles[i];
  const ButtonState hit=tile.WholeTile();
  if(tile.IsVisible() && tile.IsDir() &&
     pointerInside(x,y,tile.GetX(),tile.GetY(),hit.w,hit.h))
    return static_cast<int>(i);
 }
 return -1;
}
std::string SongLibState::NameFor(const std::string& path) const{
 if(path==RecentFolderPath())return "Recientes";
 if(path==m_personal.Root()+"/1")return "Añadidos";
 return m_placements?m_placements->DisplayName(path):shortName(path);
}
std::string SongLibState::TitleFor(const std::string& path) const{
 const std::string name=NameFor(path);
 if(g_utf8_validate(name.c_str(),name.size(),nullptr) &&
    g_utf8_strlen(name.c_str(),-1)>48){
   gchar* cut=g_utf8_substring(name.c_str(),0,45);
   std::string shown(cut);g_free(cut);return shown+"...";
 }
 return name;
}
void SongLibState::OpenContext(int x,int y,int index){
 if(m_song_tiles[index].GetPath()==RecentFolderPath()){
    m_context_open=true;
    m_context_path=RecentFolderPath();
    m_context_x=std::max(8,std::min(x,GetStateWidth()-218));
    m_context_y=std::max(8,std::min(y,GetStateHeight()-90));
    return;
 }
 if(FixedFolder(m_song_tiles[index].GetPath())){
    m_notice="Añadidos está protegida: no se puede mover, eliminar ni renombrar.";
    return;
 }
 m_context_open=true;
 m_context_path=m_song_tiles[index].GetPath();
 m_context_x=std::max(8,std::min(x,GetStateWidth()-218));
 m_context_y=std::max(8,std::min(y,GetStateHeight()-238));
 m_error.clear();
 m_notice.clear();
}

int SongLibState::MidiAt(int x,int y) const {
 for(int i=m_folder_count;i<static_cast<int>(m_song_tiles.size());++i){
  const auto& tile=m_song_tiles[i];
  const ButtonState hit=tile.WholeTile();
  if(tile.IsVisible() && pointerInside(x,y,tile.GetX(),tile.GetY(),hit.w,hit.h))
   return i;
 }
 return -1;
}
bool SongLibState::SafeMidiSource(const std::string& path) const {
 struct stat st{};
 return lstat(path.c_str(),&st)==0 && S_ISREG(st.st_mode) &&
        isMidiFile(shortName(path)) &&
        (contained(m_base_path,path)||m_personal.Owns(path));
}
void SongLibState::SelectMidi(int index,bool toggle,bool range){
 if(index<m_folder_count || index>=static_cast<int>(m_song_tiles.size()))return;
 const std::string clicked=m_song_tiles[index].GetPath();
 if(range && !m_selection_anchor.empty()){
  auto found=std::find_if(m_song_tiles.begin()+m_folder_count,m_song_tiles.end(),
    [&](const SongTile& tile){return tile.GetPath()==m_selection_anchor;});
  if(found!=m_song_tiles.end()){
   if(!toggle)m_selected_midis.clear();
   const int start=static_cast<int>(found-m_song_tiles.begin());
   for(int i=std::min(start,index);i<=std::max(start,index);++i)
    m_selected_midis.insert(m_song_tiles[i].GetPath());
   return;
  }
 }
 if(toggle){
  if(m_selected_midis.count(clicked))m_selected_midis.erase(clicked);
  else m_selected_midis.insert(clicked);
 }else{
  m_selected_midis.clear();
  m_selected_midis.insert(clicked);
 }
 m_selection_anchor=clicked;
}
void SongLibState::OpenMidiMenu(int x,int y,int index){
 const std::string path=m_song_tiles[index].GetPath();
 if(!m_selected_midis.count(path))SelectMidi(index,false,false);
 m_midi_menu=true;
 m_midi_menu_x=std::max(8,std::min(x,GetStateWidth()-265));
 m_midi_menu_y=std::max(8,std::min(y,GetStateHeight()-137));
 m_notice.clear();m_error.clear();
}
void SongLibState::BuildMidiDestinations(){
 m_midi_destinations.clear();
 std::set<std::string> seen;
 std::vector<std::pair<std::string,std::string>> pending{
  {m_base_path,"Biblioteca principal"},{m_personal.Root(),"Mis carpetas"}};
 while(!pending.empty() && seen.size()<20000){
  const auto node=pending.back();pending.pop_back();
  const std::string& dir=node.first;
  struct stat st{};
  if(dir.empty()||dir==RecentFolderPath()||
     lstat(dir.c_str(),&st)!=0||!S_ISDIR(st.st_mode)||
     !seen.insert(dir).second)continue;
  m_midi_destinations.push_back(node);
  std::vector<std::string> children;
  DIR* handle=opendir(dir.c_str());
  if(handle){
   struct dirent* entry;
   while((entry=readdir(handle))){
    const std::string name(entry->d_name);
    if(name.empty()||name[0]=='.')continue;
    const std::string child=dir+"/"+name;
    if(child==RecentFolderPath())continue;
    if(lstat(child.c_str(),&st)==0 && S_ISDIR(st.st_mode) &&
       (!m_placements || m_placements->Parent(child).empty()))
     children.push_back(child);
   }
   closedir(handle);
  }
  if(m_placements){
   for(const auto& child:m_placements->Children(dir))
    if(child!=RecentFolderPath())children.push_back(child);
  }
  std::sort(children.begin(),children.end());
  children.erase(std::unique(children.begin(),children.end()),children.end());
  for(const auto& child:children)
   pending.emplace_back(child,node.second+" / "+NameFor(child));
 }
 // Personal destinations, including Piano, must be at the top of page 1.
 // Keep the library examples afterwards; do not hide moved/linked folders.
 std::stable_sort(m_midi_destinations.begin(),m_midi_destinations.end(),
  [&](const std::pair<std::string,std::string>& a,
      const std::pair<std::string,std::string>& b){
    const bool ap=m_personal.Owns(a.first);
    const bool bp=m_personal.Owns(b.first);
    if(ap!=bp)return ap;
    gchar* al=g_utf8_casefold(a.second.c_str(),-1);
    gchar* bl=g_utf8_casefold(b.second.c_str(),-1);
    const int result=g_strcmp0(al,bl);g_free(al);g_free(bl);
    return result<0;
  });
 m_midi_destination_page=0;
 m_midi_destinations_open=true;
 m_midi_menu=false;
}

// Preserve the payload and selection when changing directories during a held
// mouse gesture.  UpdateSongTiles() normally clears the whole selection.
void SongLibState::NavigateDuringMidiDrag(const std::string& nextPath){
 if(nextPath==RecentFolderPath())return;
 struct stat st{};
 if(nextPath==m_current_path ||
    lstat(nextPath.c_str(),&st)!=0 || !S_ISDIR(st.st_mode) ||
    !(contained(m_base_path,nextPath) || m_personal.Owns(nextPath)))
  return;
 const auto files=m_midi_moving_files;
 const auto marked=m_selected_midis;
 const auto origin=m_midi_drag_source_folder;
 const int steps=m_midi_navigation_steps;
 const int x=m_midi_drag_x,y=m_midi_drag_y;
 m_current_path=nextPath;
 m_current_page=0;m_file_page=0;
 UserSetting::Set(SONG_LIB_DIR_SETTINGS_KEY,m_current_path);
 UpdateSongTiles();
 m_selected_midis=marked;
 m_midi_moving_files=files;
 m_midi_drag_source_folder=origin;
 m_midi_navigation_steps=steps+1;
 m_midi_move_candidate=true;
 m_midi_move_dragging=true;
 m_midi_drag_x=x;m_midi_drag_y=y;
 m_midi_hover_destination.clear();
 m_midi_hover_started=0;
 m_notice="Arrastra hasta la carpeta de destino y suelta para confirmar.";
 m_error.clear();
}
// Hover 520 ms sobre las flechas o el borde de la zona de contenido.
 // No mueve ningun archivo: solo cambia el indice de pagina y las tarjetas
 // visibles, conservando el grupo que sigue flotando con el puntero.
void SongLibState::UpdateDragPagination(int x,int y){
    using namespace DragPageNavigation;
    const auto rect=[](const ButtonState& b)->Rect{
        return {b.x,b.y,b.w,b.h};
    };
    const State pages{
        GetStateWidth(),GetStateHeight(),m_file_list_top,
        m_current_page,m_page_count,m_file_page,m_file_page_count,
        rect(m_prev_page_button),rect(m_next_page_button),
        rect(m_file_prev_button),rect(m_file_next_button)
    };
    const DragKind kind=m_dragging?DragKind::Folder:DragKind::Midi;
    const Target target=Hit(pages,x,y,kind);
    const int chosen=static_cast<int>(target);
    if(std::getenv("LINTHESIA_DRAG_TRACE") && m_page_hover_target!=chosen)
       std::fprintf(stderr,"[DRAG] hover target %d -> %d, mouse(%d,%d) folders=%d/%d files=%d/%d\n",
          m_page_hover_target,chosen,x,y,m_current_page+1,m_page_count,
          m_file_page+1,m_file_page_count);
    if(chosen==0){
        m_page_hover_target=0;
        m_page_hover_start=0;
        return;
    }
    const Uint32 now=SDL_GetTicks();
    if(m_page_hover_target!=chosen || m_page_hover_start==0){
        m_page_hover_target=chosen;
        m_page_hover_start=now;
        m_notice=std::string("Mantén pulsado para avanzar por las páginas de ")+
           ((chosen==3||chosen==4)?"archivos MIDI.":"carpetas.");
        return;
    }
    if(now-m_page_hover_start<450)return;
    switch(target){
        case Target::FolderPrevious: --m_current_page;break;
        case Target::FolderNext: ++m_current_page;break;
        case Target::MidiPrevious: --m_file_page;break;
        case Target::MidiNext: ++m_file_page;break;
        default:return;
    }
    UpdateSongTilesPage();
    if(std::getenv("LINTHESIA_DRAG_TRACE"))
       std::fprintf(stderr,"[DRAG] PAGE CHANGED folders=%d/%d files=%d/%d\n",
         m_current_page+1,m_page_count,m_file_page+1,m_file_page_count);
    // Una nueva pagina bajo el mismo puntero continua avanzando lentamente.
    m_page_hover_start=now;
    m_page_hover_target=static_cast<int>(Hit(State{
        GetStateWidth(),GetStateHeight(),m_file_list_top,
        m_current_page,m_page_count,m_file_page,m_file_page_count,
        rect(m_prev_page_button),rect(m_next_page_button),
        rect(m_file_prev_button),rect(m_file_next_button)},x,y,kind));
    if(!m_page_hover_target)m_page_hover_start=0;
    m_notice=(target==Target::MidiNext || target==Target::MidiPrevious)?
        ("MIDI: pagina "+std::to_string(m_file_page+1)+" de "+
         std::to_string(m_file_page_count)+"; sigue arrastrando."):
        ("Carpetas: pagina "+std::to_string(m_current_page+1)+" de "+
         std::to_string(m_page_count)+"; sigue arrastrando.");
}
void SongLibState::UpdateMidiDragHover(int x,int y){
 // Hover for a short moment to enter a folder or go up while still dragging.
 const int folder=TileAt(x,y);
 std::string destination;
 if(folder>=0 && m_song_tiles[folder].GetPath()!=RecentFolderPath())
  destination=m_song_tiles[folder].GetPath();
 else if(m_current_path!=m_base_path &&
         pointerInside(x,y,m_path_up_button.x,m_path_up_button.y,
                       m_path_up_button.w,m_path_up_button.h)){
  const std::string virtualParent=m_placements?
                              m_placements->Parent(m_current_path):"";
  const auto cut=m_current_path.find_last_of('/');
  const std::string physical=cut==std::string::npos?
                              m_base_path:m_current_path.substr(0,cut);
  destination=!virtualParent.empty()?virtualParent:
     ((m_personal.Owns(m_current_path) &&
       (physical==m_personal.Root()||!m_personal.Owns(physical))) ||
      (!contained(m_base_path,physical)&&!m_personal.Owns(physical)))?
      m_base_path:physical;
 }
 if(destination.empty()){
  m_midi_hover_destination.clear();
  m_midi_hover_started=0;
  return;
 }
 if(destination!=m_midi_hover_destination){
  m_midi_hover_destination=destination;
  m_midi_hover_started=SDL_GetTicks();
 }else if(SDL_GetTicks()-m_midi_hover_started>=static_cast<Uint32>(m_preferences.dragHoverMs)){
  NavigateDuringMidiDrag(destination);
 }
}
void SongLibState::ExecuteMidiAction(){
 const int action=m_midi_confirmation;
 m_midi_confirmation=0;
 if(action!=1 && action!=2)return;
 if(m_midi_action_files.empty()){m_error="No hay MIDI seleccionados.";return;}
 std::string target=m_midi_action_target;
 if(action==2){
  if(target==RecentFolderPath()){
    m_error="Recientes es automática y no acepta archivos.";return;
  }
  struct stat st{};
  if(lstat(target.c_str(),&st)!=0||!S_ISDIR(st.st_mode)||
     !(contained(m_base_path,target)||m_personal.WritableFolder(target))){
   m_error="La carpeta de destino ya no existe o no es válida.";return;
  }
 }
 // Check EVERY entry before touching any; never overwrite namesakes.
 std::set<std::string> names;
 for(const auto& source:m_midi_action_files){
  if(!SafeMidiSource(source)){
   m_error="El MIDI ya no existe o está fuera de la biblioteca: "+TitleFor(source);
   return;
  }
  if(action==2){
   const std::string out=target+"/"+shortName(source);
   struct stat st{};
   if(source==out){
    m_error="El MIDI ya está en esa carpeta: "+TitleFor(source);return;
   }
   if(!names.insert(shortName(source)).second ||
      lstat(out.c_str(),&st)==0 || errno!=ENOENT){
    m_error="El destino ya contiene ese nombre: "+TitleFor(source);return;
   }
  }
 }
 int completed=0;
 std::string failure;
 for(const auto& source:m_midi_action_files){
  GFile* from=g_file_new_for_path(source.c_str());
  GError* err=nullptr;
  gboolean ok=FALSE;
  if(action==1)ok=g_file_trash(from,nullptr,&err);
  else {
   GFile* to=g_file_new_for_path((target+"/"+shortName(source)).c_str());
   ok=g_file_move(from,to,G_FILE_COPY_NOFOLLOW_SYMLINKS,
                  nullptr,nullptr,nullptr,&err);
   g_object_unref(to);
  }
  g_object_unref(from);
  if(!ok){
   failure=TitleFor(source)+": "+(err?err->message:"operación no disponible");
   if(err)g_error_free(err);
   break;
  }
  ++completed;
 }
 m_midi_menu=false;
 m_midi_destinations_open=false;
 if(completed>0){
  if(action==2){
   EditAction edit;
   edit.kind=EditAction::Kind::MidiMove;
   edit.title=std::to_string(completed)+" MIDI movidos";
   for(int i=0;i<completed;++i){
    const auto& from=m_midi_action_files[i];
    edit.files.emplace_back(from,target+"/"+shortName(from));
   }
   RecordEdit(edit);
  }else ClearEditHistory(); // OS trash has no reliable reversible ticket.
  m_selected_midis.clear();
  m_selection_anchor.clear();m_highlight_path.clear();
  UpdateSongTiles();
 }
 if(!failure.empty())
  m_error="Completados "+std::to_string(completed)+"/"+
          std::to_string(m_midi_action_files.size())+". "+failure;
 else
  m_notice=std::to_string(completed)+" MIDI "+(action==1?
             "enviados a la papelera.":"movidos a "+TitleFor(target)+".");
 m_midi_action_files.clear();m_midi_action_target.clear();
}

void SongLibState::RecordEdit(const EditAction& edit){
 m_undo_stack.push_back(edit);
 if(m_undo_stack.size()>50)m_undo_stack.erase(m_undo_stack.begin());
 m_redo_stack.clear();
}
void SongLibState::ClearSelections(){
 m_selected_folders.clear();
 m_folder_anchor.clear();
 m_last_clicked_folder.clear();
 m_last_folder_click=0;
 m_selected_midis.clear();
 m_selection_anchor.clear();
 m_last_clicked_midi.clear();
 m_last_midi_click=0;
 // Cancel pending gestures; a focus loss must never complete a drag.
 m_folder_rect_candidate=false;
 m_folder_rect_selecting=false;
 m_midi_drag_candidate=false;
 m_midi_select_dragging=false;
 m_midi_move_candidate=false;
 m_midi_move_dragging=false;
 m_dragging=false;
 m_pressed_path.clear();
 m_drag_path.clear();
 m_folder_drag_files.clear();
 m_midi_moving_files.clear();
 m_folder_hover_target.clear();
 m_midi_hover_destination.clear();
 if(m_notice.find("seleccionad")!=std::string::npos ||
    m_notice.find("Arrastra")!=std::string::npos)
  m_notice.clear();
}
void SongLibState::ClearEditHistory(){
 m_undo_stack.clear();
 m_redo_stack.clear();
}
bool SongLibState::MoveMidiPairs(
 const std::vector<std::pair<std::string,std::string>>& files,
 bool undo,std::string& error){
 if(files.empty()){error="No hay movimientos que recuperar.";return false;}
 std::set<std::string> targets;
 // Verify all paths before moving anything. Prevent overwrites and symlinks.
 for(const auto& pair:files){
  const std::string& from=undo?pair.second:pair.first;
  const std::string& to=undo?pair.first:pair.second;
  struct stat st{},dst{};
  if(!SafeMidiSource(from)||from==to){
   error="El MIDI cambió de lugar: "+TitleFor(from);return false;
  }
  if(to.find_last_of('/')==std::string::npos ||
     !targets.insert(to).second){
   error="Destino duplicado en el historial.";return false;
  }
  const std::string parent=to.substr(0,to.find_last_of('/'));
  if(lstat(parent.c_str(),&st)!=0 || !S_ISDIR(st.st_mode) ||
     !(contained(m_base_path,parent)||m_personal.WritableFolder(parent))){
   error="La carpeta de destino ya no está disponible.";return false;
  }
  errno=0;
  if(lstat(to.c_str(),&dst)==0 || errno!=ENOENT){
   error="Existe un archivo en el destino: "+TitleFor(to);return false;
  }
 }
 std::vector<std::pair<std::string,std::string>> finished;
 for(const auto& pair:files){
  const std::string& from=undo?pair.second:pair.first;
  const std::string& to=undo?pair.first:pair.second;
  GFile* original=g_file_new_for_path(from.c_str());
  GFile* destination=g_file_new_for_path(to.c_str());
  GError* problem=nullptr;
  gboolean ok=g_file_move(original,destination,
     G_FILE_COPY_NOFOLLOW_SYMLINKS,nullptr,nullptr,nullptr,&problem);
  g_object_unref(original);g_object_unref(destination);
  if(!ok){
   error=problem?problem->message:"No se pudo mover el MIDI.";
   if(problem)g_error_free(problem);
   // Best-effort rollback for operations already moved.
   bool restored=true;
   for(auto it=finished.rbegin();it!=finished.rend();++it){
    GFile* dest=g_file_new_for_path(it->second.c_str());
    GFile* orig=g_file_new_for_path(it->first.c_str());
    GError* rollbackError=nullptr;
    const bool done=g_file_move(dest,orig,G_FILE_COPY_NOFOLLOW_SYMLINKS,
                         nullptr,nullptr,nullptr,&rollbackError);
    g_object_unref(dest);g_object_unref(orig);
    if(!done)restored=false;
    if(rollbackError)g_error_free(rollbackError);
   }
   if(!restored)error+=" AVISO: restauración parcial; comprueba los archivos.";
   return false;
  }
  finished.emplace_back(from,to);
 }
 return true;
}
void SongLibState::ApplyEdit(bool undo){
 auto& source=undo?m_undo_stack:m_redo_stack;
 if(source.empty())return;
 const EditAction edit=source.back();
 if((edit.kind==EditAction::Kind::PhysicalFolder &&
      (FixedFolder(edit.source)||FixedFolder(edit.target))) ||
    (edit.kind==EditAction::Kind::FolderRename &&
      (FixedFolder(edit.source)||FixedFolder(edit.target))) ||
    (edit.kind==EditAction::Kind::FolderLinks &&
      std::any_of(edit.beforeLinks.begin(),edit.beforeLinks.end(),
          [&](const std::pair<std::string,std::string>& link){return FixedFolder(link.first);})) ||
    (edit.kind==EditAction::Kind::FolderLinks &&
      std::any_of(edit.afterLinks.begin(),edit.afterLinks.end(),
          [&](const std::pair<std::string,std::string>& link){return FixedFolder(link.first);}))){
     m_error="Añadidos está protegida y no se puede mover desde el historial.";
     return;
 }
 std::string error;
 bool ok=false;
 switch(edit.kind){
  case EditAction::Kind::MidiMove:
   ok=MoveMidiPairs(edit.files,undo,error);
   break;
  case EditAction::Kind::FolderLinks:
   ok=m_placements && m_placements->RestoreSnapshot(
      undo?edit.afterLinks:edit.beforeLinks,
      undo?edit.beforeLinks:edit.afterLinks,error);
   break;
  case EditAction::Kind::FolderOrder:{
   const auto& expected=undo?edit.afterOrder:edit.beforeOrder;
   const auto& desired=undo?edit.beforeOrder:edit.afterOrder;
   if(m_personal.ReadOrder(edit.folder)!=expected)
    error="El orden se ha modificado desde esa operación.";
   else ok=m_personal.SaveOrder(edit.folder,desired,error);
   break;
  }
  case EditAction::Kind::FolderAlias:{
   ok=m_placements && m_placements->SetDisplayName(
       edit.source,undo?edit.beforeName:edit.afterName,error);
   break;
  }
  case EditAction::Kind::FolderRename:{
   const std::string& from=undo?edit.target:edit.source;
   const std::string& to=undo?edit.source:edit.target;
   std::string result;
   ok=RenameFolder(from,shortName(to),result,error);
   if(ok && result!=to){ok=false;error="El renombrado ha producido una ruta inesperada.";}
   if(ok)RenameOrderEntry(edit.folder,shortName(from),shortName(to));
   break;
  }
  case EditAction::Kind::PhysicalFolder:{
   const std::string& from=undo?edit.target:edit.source;
   const std::string& to=undo?edit.source:edit.target;
   const std::string parent=to.substr(0,to.find_last_of('/'));
   std::string output;
   ok=m_personal.Move(from,parent,output,error);
   if(ok && output!=to){
    ok=false;error="La carpeta se ha movido a una ubicación inesperada.";
   }
   break;
  }
 }
 if(!ok){
  m_error=error.empty()?"No se pudo aplicar el historial.":error;
  m_notice.clear();return;
 }
 source.pop_back();
 auto& destination=undo?m_redo_stack:m_undo_stack;
 destination.push_back(edit);
 // A physical folder may have moved while we were browsing inside it.
 // Never attempt to scan an old directory that no longer exists.
 struct stat current{};
 if(lstat(m_current_path.c_str(),&current)!=0 ||
    !S_ISDIR(current.st_mode)){
  m_current_path=m_base_path;
  m_current_page=0;
  UserSetting::Set(SONG_LIB_DIR_SETTINGS_KEY,m_current_path);
 }
 UpdateSongTiles();
 m_error.clear();
 m_notice=std::string(undo?"Deshecho: ":"Rehecho: ")+edit.title;
}
void SongLibState::OpenConfirmation(int action,const std::string& src,
                                    const std::string& target){
 if(FixedFolder(src)){
     m_context_open=false;
     m_error="Añadidos está bloqueada.";return;
 }
 m_context_open=false;
 m_confirm=action;
 m_confirm_src=src;m_confirm_target=target;
 m_error.clear();m_notice.clear();
}
void SongLibState::ExecuteConfirmation(){
 std::string result,error;
 bool ok=false;
 const auto before=m_placements?m_placements->Snapshot():
                         FolderPlacements::PlacementSnapshot();
 const int action=m_confirm;
 const std::string origin=m_confirm_src;
 if(m_confirm==1)
   ok=m_personal.Trash(m_confirm_src,result,error);
 else if(m_confirm==2)
   ok=m_personal.Move(m_confirm_src,m_confirm_target,result,error);
 else if(m_confirm==3)
   ok=m_personal.Restore(m_confirm_src,result,error);
 else if(m_confirm==4 && m_placements)
   ok=m_placements->ReturnToMain(m_confirm_src,error);
 if(ok){
   if(action==4 && m_placements){
    EditAction edit;edit.kind=EditAction::Kind::FolderLinks;
    edit.title="Carpeta devuelta a principales";
    edit.beforeLinks=before;edit.afterLinks=m_placements->Snapshot();
    RecordEdit(edit);
   }else if(action==2){
    EditAction edit;edit.kind=EditAction::Kind::PhysicalFolder;
    edit.title="Carpeta movida";
    edit.source=origin;edit.target=result;
    RecordEdit(edit);
   }else ClearEditHistory(); // Trash/restore can't be replayed safely.
   m_notice=m_confirm==1?"Carpeta enviada a la papelera: "+
                           TitleFor(m_confirm_src):
            m_confirm==2?"Carpeta movida: "+TitleFor(m_confirm_src):
            m_confirm==3?"Carpeta restaurada: "+TitleFor(result):
                         "Carpeta devuelta a principales: "+TitleFor(m_confirm_src);
   if(m_confirm==4){
       // Show the actual result immediately on the combined library root.
       m_current_path=m_base_path;
       UserSetting::Set(SONG_LIB_DIR_SETTINGS_KEY,m_current_path);
   }
   m_current_page=0;
   UpdateSongTiles();
   if(m_confirm==4){
       for(size_t i=0;i<m_song_tiles.size();++i){
           if(m_song_tiles[i].GetPath()==m_confirm_src){
               m_current_page=static_cast<int>(i)/m_tiles_per_page;
               UpdateSongTilesPage();
               break;
           }
       }
   }
 }else m_error=error;
 m_confirm=0;
 m_confirm_src.clear();m_confirm_target.clear();
}
void SongLibState::SwapPositions(const std::string& source,
                                  const std::string& target){
    if(FixedFolder(source)||FixedFolder(target)){
        m_error="Añadidos tiene posición fija.";return;
    }
    // Exchange exactly two cards, leaving all the other slots intact.
    std::vector<std::string> paths;
    paths.reserve(m_song_tiles.size());
    for(const auto& tile:m_song_tiles)paths.push_back(tile.GetPath());
    if(!FolderOrdering::SwapPositions(paths,source,target)){
        m_error="No se han encontrado las dos carpetas.";
        return;
    }
    std::vector<std::string> names;
    for(const auto& path:paths)names.push_back(shortName(path));
    std::string error;
    std::vector<std::string> beforeOrder=m_personal.ReadOrder(m_current_path);
    if(beforeOrder.empty()){
        for(const auto& tile:m_song_tiles)
            beforeOrder.push_back(shortName(tile.GetPath()));
    }
    if(!m_personal.SaveOrder(m_current_path,names,error)){
        m_error=error;
        return;
    }
    EditAction edit;edit.kind=EditAction::Kind::FolderOrder;
    edit.title="Posición de carpetas";
    edit.folder=m_current_path;
    edit.beforeOrder=beforeOrder;edit.afterOrder=names;
    RecordEdit(edit);
    const auto first=std::find_if(m_song_tiles.begin(),m_song_tiles.end(),
          [&](const SongTile& t){return t.GetPath()==source;});
    const auto second=std::find_if(m_song_tiles.begin(),m_song_tiles.end(),
          [&](const SongTile& t){return t.GetPath()==target;});
    if(first!=m_song_tiles.end()&&second!=m_song_tiles.end())
        std::iter_swap(first,second);
    m_error.clear();
    m_notice="Posiciones intercambiadas: "+TitleFor(source)+
             " y "+TitleFor(target);
    // Preserve the page where the user dropped the dragged folder.
    UpdateSongTilesPage();
}
void SongLibState::ApplyDropChoice(bool moveInside){
    const std::string source=m_drop_src,target=m_drop_target;
    if(FixedFolder(source)||(!moveInside&&FixedFolder(target))){
        m_drop_choice=false;m_drop_src.clear();m_drop_target.clear();
        m_error="Añadidos no se puede trasladar ni intercambiar.";
        return;
    }
    m_drop_choice=false;
    m_drop_src.clear();
    m_drop_target.clear();
    if(moveInside && target==RecentFolderPath()){
        m_error="Recientes no admite carpetas; se actualiza automáticamente.";
        return;
    }
    if(moveInside){
        std::string error,output;
        if(!m_placements || !m_placements->CanContain(source,target,error)){
            m_error=error.empty()?"No se puede realizar este movimiento.":error;
            return;
        }
        // Personal-to-personal folders retain their previous physical move
        // semantics where it cannot invalidate existing virtual references.
        const bool safePhysical=m_personal.OwnedChild(source) &&
             m_personal.WritableFolder(target) &&
             m_placements->Parent(source).empty() &&
             !m_placements->ReferencesWithin(source) &&
             !m_placements->ReferencesWithin(target);
        const auto before=m_placements->Snapshot();
        const bool success=safePhysical?
           m_personal.Move(source,target,output,error):
           m_placements->Place(source,target,error);
        if(!success){
            m_error=error;
            return;
        }
        EditAction edit;
        edit.title="Carpeta colocada dentro de otra";
        if(safePhysical){
            edit.kind=EditAction::Kind::PhysicalFolder;
            edit.source=source;edit.target=output;
        }else{
            edit.kind=EditAction::Kind::FolderLinks;
            edit.beforeLinks=before;edit.afterLinks=m_placements->Snapshot();
        }
        RecordEdit(edit);
        m_error.clear();
        m_notice="Carpeta guardada dentro de "+TitleFor(target)+
                 ": "+TitleFor(source);
        UpdateSongTiles();
    }else {
        SwapPositions(source,target);
    }
}

void SongLibState::NavigateDuringFolderDrag(const std::string& target){
 if(std::getenv("LINTHESIA_DRAG_TRACE"))
     std::fprintf(stderr,"[DRAG] FOLDER NAVIGATE from %s to %s\n",
         m_current_path.c_str(),target.c_str());
 struct stat st{};
 if(target==m_current_path || lstat(target.c_str(),&st)!=0||
    !S_ISDIR(st.st_mode) ||
    !(contained(m_base_path,target)||m_personal.Owns(target)))return;
 const auto carried=m_folder_drag_files;
 const auto highlighted=m_selected_folders;
 const auto origin=m_folder_drag_origin;
 const auto source=m_drag_path;
 const int steps=m_folder_drag_navigations;
 m_current_path=target;
 m_current_page=0;m_file_page=0;
 UserSetting::Set(SONG_LIB_DIR_SETTINGS_KEY,m_current_path);
 UpdateSongTiles();
 m_selected_folders=highlighted;
 m_folder_drag_files=carried;
 m_folder_drag_origin=origin;
 m_drag_path=source;
 m_pressed_path=source;
 m_dragging=true;
 m_folder_drag_navigations=steps+1;
 m_folder_hover_target.clear();
 m_folder_hover_started=0;
 m_notice="Mantén pulsado y suelta las carpetas donde quieras moverlas.";
}
void SongLibState::UpdateFolderDragHover(int x,int y){
 std::string target;
 // Una carpeta bajo el puntero es un destino potencial para SOLTAR.
 // Nunca abrirla automaticamente: en ese caso no da tiempo a elegir
 // el intercambio de posicion (Soltar aqui / Soltar dentro).
 // Unicamente el boton Subir puede navegar con una carpeta agarrada.
 if(m_current_path!=m_base_path &&
         pointerInside(x,y,m_path_up_button.x,m_path_up_button.y,
                       m_path_up_button.w,m_path_up_button.h)){
  const std::string linked=m_placements?
          m_placements->Parent(m_current_path):"";
  const auto slash=m_current_path.find_last_of('/');
  const std::string physical=slash==std::string::npos?
          m_base_path:m_current_path.substr(0,slash);
  target=!linked.empty()?linked:
      ((m_personal.Owns(m_current_path)&&
         (physical==m_personal.Root()||!m_personal.Owns(physical))) ||
       (!contained(m_base_path,physical)&&!m_personal.Owns(physical)))?
       m_base_path:physical;
 }
 if(target.empty()){
  m_folder_hover_target.clear();
  m_folder_hover_started=0;
 }else if(target!=m_folder_hover_target){
  m_folder_hover_target=target;
  m_folder_hover_started=SDL_GetTicks();
 }else if(SDL_GetTicks()-m_folder_hover_started>=static_cast<Uint32>(m_preferences.dragHoverMs)){
  NavigateDuringFolderDrag(target);
 }
}
void SongLibState::ExecuteFolderBatch(){
 if(!m_folder_batch_confirm)return;
 if(FixedFolder(m_folder_action_target) &&
    std::find_if(m_folder_action_files.begin(),m_folder_action_files.end(),
        [&](const std::string& path){return FixedFolder(path);})!=m_folder_action_files.end()){
   m_error="Añadidos no se puede trasladar.";
   m_folder_batch_confirm=false;return;
 }
 if(std::find_if(m_folder_action_files.begin(),m_folder_action_files.end(),
        [&](const std::string& path){return FixedFolder(path);})!=m_folder_action_files.end()){
   m_error="Añadidos no se puede trasladar.";
   m_folder_batch_confirm=false;return;
 }
 m_folder_batch_confirm=false;
 std::string error;
 const auto before=m_placements?m_placements->Snapshot():
                         FolderPlacements::PlacementSnapshot();
 const bool done=m_placements &&
   m_placements->RelocateBatch(m_folder_action_files,
                                m_folder_action_target,error);
 if(done){
  EditAction edit;edit.kind=EditAction::Kind::FolderLinks;
  edit.title=std::to_string(m_folder_action_files.size())+
             " carpetas organizadas";
  edit.beforeLinks=before;edit.afterLinks=m_placements->Snapshot();
  RecordEdit(edit);
  const int amount=static_cast<int>(m_folder_action_files.size());
  m_selected_folders.clear();
  m_folder_anchor.clear();
  UpdateSongTiles();
  m_notice=std::to_string(amount)+" carpeta(s) organizadas en "+
           TitleFor(m_folder_action_target)+".";
  m_error.clear();
 }else{
  m_error=error.empty()?"No se han movido carpetas.":error;
 }
 m_folder_action_files.clear();
 m_folder_action_target.clear();
}
void SongLibState::CompleteDrag(int x,int y){
    const std::string source=m_drag_path;
    if(FixedFolder(source)){m_dragging=false;m_drag_path.clear();
        m_folder_drag_files.clear();m_error="Añadidos está bloqueada.";return;}
    m_drag_path.clear();
    m_dragging=false;
    m_pressed_path.clear();
    m_page_hover_start=0;
    m_page_hover_target=0;
    const int steps=m_folder_drag_navigations;
    m_folder_drag_navigations=0;
    m_folder_hover_target.clear();m_folder_hover_started=0;
    if(source.empty()||m_folder_drag_files.empty())return;
    const int targetIndex=TileAt(x,y);
    const std::string target=targetIndex>=0?
       m_song_tiles[targetIndex].GetPath():
       (steps>0 && m_current_path!=m_folder_drag_origin?
                        m_current_path:std::string());
    if(target==RecentFolderPath()){
        m_error="Recientes es una lista automática: no admite carpetas.";
        m_folder_drag_files.clear();return;
    }
    if(target.empty()||m_selected_folders.count(target)){
        m_notice="Suelta sobre una carpeta de destino distinta.";
        m_folder_drag_files.clear();return;
    }
    if(m_folder_drag_files.size()==1 && steps==0 && targetIndex>=0){
        // Keep the existing one-folder swap/containment options.
        m_drop_src=source;
        m_drop_target=target;
        m_drop_choice=true;
    }else{
        m_folder_action_files=m_folder_drag_files;
        m_folder_action_target=target;
        m_folder_batch_confirm=true;
    }
    m_folder_drag_files.clear();
    m_error.clear();m_notice.clear();
}

void SongLibState::Update() {
    PollMidiImport();
    MouseInfo mouse = Mouse();
    const bool hasKeyboardFocus=SDL_GetKeyboardFocus()!=nullptr;
    if(m_had_keyboard_focus && !hasKeyboardFocus &&
       (!m_selected_folders.empty() || !m_selected_midis.empty()))
        ClearSelections();
    m_had_keyboard_focus=hasKeyboardFocus;

    if (m_skip_next_mouse_up) {
        mouse.released.left = false;
        m_skip_next_mouse_up = false;
    }

    if(m_meta_editing){
        const int width=std::min(720,GetStateWidth()-40);
        const int left=(GetStateWidth()-width)/2;
        const int top=(GetStateHeight()-490)/2;
        if(IsKeyPressed(KeyEscape)){CloseMetaEditor(false);return;}
        if(IsKeyPressed(KeyEnter)){CloseMetaEditor(true);return;}
        if(IsKeyPressed(KeyBackspace) && m_meta_field<7){
           auto& value=MetaTextField();
           if(!value.empty()){
              const char* end=value.c_str()+value.size();
              const char* previous=g_utf8_find_prev_char(value.c_str(),end);
              if(previous)value.resize(previous-value.c_str());
           }
        }
        if(mouse.released.left){
           if(pointerInside(mouse.x,mouse.y,left+20,top+58,width-40,9*36)){
              m_meta_field=std::min(8,(mouse.y-top-58)/36);
              if(m_meta_field>=7){
                 int& number=m_meta_field==7?m_meta_entry.difficulty:
                                               m_meta_entry.rating;
                 number=std::max(0,std::min(5,number+
                          (mouse.x>left+width-118?1:-1)));
              }
           }else if(pointerInside(mouse.x,mouse.y,left+30,top+430,230,42)){
             CloseMetaEditor(true);return;
           }else if(pointerInside(mouse.x,mouse.y,left+width-260,top+430,230,42)){
             CloseMetaEditor(false);return;
           }
        }
        return; // Modal. No song movement or deletion while editing.
    }
    if(IsKeyPressed(KeyF10)){
        OpenMetaEditor();return;
    }
    if(m_editing_name){
        if(IsKeyPressed(KeyEscape)){FinishCreateFolder(false);return;}
        if(IsKeyPressed(KeyEnter)){FinishCreateFolder(true);return;}
        if(IsKeyPressed(KeyBackspace) && !m_folder_name.empty()){
            const char* end=m_folder_name.c_str()+m_folder_name.size();
            const char* prev=g_utf8_find_prev_char(m_folder_name.c_str(),end);
            if(prev)m_folder_name.resize(prev-m_folder_name.c_str());
        }
        return; // All other controls are inactive during name entry.
    }

    if(m_search_results_open){
        const int width=std::min(780,GetStateWidth()-30);
        const int height=std::min(550,GetStateHeight()-36);
        const int left=(GetStateWidth()-width)/2;
        const int top=(GetStateHeight()-height)/2;
        const int per=std::max(1,(height-155)/43);
        const int pages=std::max(1,(static_cast<int>(m_search_hits.size())+
                                     per-1)/per);
        if(IsKeyPressed(KeyEscape)){m_search_results_open=false;return;}
        if(IsKeyPressed(KeyLeft)&&m_search_page>0){--m_search_page;return;}
        if(IsKeyPressed(KeyRight)&&m_search_page+1<pages){
            ++m_search_page;return;
        }
        if(mouse.released.left){
            if(pointerInside(mouse.x,mouse.y,left+width-99,top+13,82,30)){
                m_search_results_open=false;return;
            }
            if(pointerInside(mouse.x,mouse.y,left+16,top+69,
                             width-32,per*43)){
                const int index=m_search_page*per+(mouse.y-(top+69))/43;
                if(index>=0&&index<static_cast<int>(m_search_hits.size())){
                    const MidiSearchHit chosen=m_search_hits[index];
                    SelectSearchHit(chosen);
                }
                return;
            }
            if(pointerInside(mouse.x,mouse.y,left+width-226,top+height-52,
                             82,34)&&m_search_page>0){
                --m_search_page;return;
            }
            if(pointerInside(mouse.x,mouse.y,left+width-115,top+height-52,
                             82,34)&&m_search_page+1<pages){
                ++m_search_page;return;
            }
        }
        return;
    }
    if(m_search_editing){
        if(IsKeyPressed(KeyEscape)){
            SDL_StopTextInput();m_search_editing=false;return;
        }
        if(IsKeyPressed(KeyBackspace)&&!m_search_query.empty()){
            const char* previous=g_utf8_find_prev_char(
                        m_search_query.c_str(),
                        m_search_query.c_str()+m_search_query.size());
            if(previous)m_search_query.resize(previous-m_search_query.c_str());
            return;
        }
        if(IsKeyPressed(KeyEnter)){RunMidiSearch();return;}
        if(mouse.released.left){
            if(pointerInside(mouse.x,mouse.y,
                  m_search_button.x,m_search_button.y,
                  m_search_button.w,m_search_button.h)){
                RunMidiSearch();return;
            }
            if(!pointerInside(mouse.x,mouse.y,
                  m_search_field.x,m_search_field.y,
                  m_search_field.w,m_search_field.h)){
                SDL_StopTextInput();m_search_editing=false;
            }
        }
        return;
    }
    if(m_compact_subfolders && mouse.released.left){
        if(pointerInside(mouse.x,mouse.y,m_search_field.x,m_search_field.y,
                         m_search_field.w,m_search_field.h)){
            m_search_editing=true;
            SDL_StartTextInput();
            return;
        }
        if(pointerInside(mouse.x,mouse.y,m_search_button.x,m_search_button.y,
                         m_search_button.w,m_search_button.h)){
            RunMidiSearch();return;
        }
    }


    // MIDI context menu: changes never run until the user confirms.
    if(m_midi_confirmation){
        if(IsKeyPressed(KeyEscape)){
            m_midi_confirmation=0;m_midi_action_files.clear();
            m_midi_action_target.clear();return;
        }
        if(IsKeyPressed(KeyEnter)){ExecuteMidiAction();return;}
        const int x=GetStateWidth()/2,y=GetStateHeight()/2;
        if(mouse.released.left){
            if(pointerInside(mouse.x,mouse.y,x-206,y+65,176,44)){
                ExecuteMidiAction();return;
            }
            if(pointerInside(mouse.x,mouse.y,x+30,y+65,176,44)){
                m_midi_confirmation=0;m_midi_action_files.clear();
                m_midi_action_target.clear();return;
            }
        }
        return;
    }
    if(m_midi_destinations_open){
        const int width=std::min(760,GetStateWidth()-30);
        const int height=std::min(590,GetStateHeight()-32);
        const int left=(GetStateWidth()-width)/2;
        const int top=(GetStateHeight()-height)/2;
        const int per=std::max(1,(height-160)/38);
        const int pages=std::max(1,
          (static_cast<int>(m_midi_destinations.size())+per-1)/per);
        if(IsKeyPressed(KeyEscape)){m_midi_destinations_open=false;return;}
        if(IsKeyPressed(KeyLeft)&&m_midi_destination_page>0){
            --m_midi_destination_page;return;
        }
        if(IsKeyPressed(KeyRight)&&m_midi_destination_page+1<pages){
            ++m_midi_destination_page;return;
        }
        if(mouse.released.left){
            if(pointerInside(mouse.x,mouse.y,left+width-114,top+14,96,30)){
                m_midi_destinations_open=false;return;
            }
            if(pointerInside(mouse.x,mouse.y,left+16,top+75,
                             width-32,per*38)){
                int chosen=m_midi_destination_page*per+
                           (mouse.y-(top+75))/38;
                if(chosen>=0 && chosen<static_cast<int>(m_midi_destinations.size())){
                    m_midi_action_files.assign(m_selected_midis.begin(),
                                               m_selected_midis.end());
                    m_midi_action_target=m_midi_destinations[chosen].first;
                    if(access(m_midi_action_target.c_str(),W_OK)!=0){
                        m_error="La carpeta de destino es de solo lectura.";
                        return;
                    }
                    m_error.clear();
                    m_midi_confirmation=2;
                    m_midi_destinations_open=false;
                }
                return;
            }
            if(pointerInside(mouse.x,mouse.y,left+width-248,
                             top+height-57,102,36) &&
               m_midi_destination_page>0){--m_midi_destination_page;return;}
            if(pointerInside(mouse.x,mouse.y,left+width-128,
                             top+height-57,102,36) &&
               m_midi_destination_page+1<pages){
                ++m_midi_destination_page;return;
            }
        }
        return;
    }
    if(m_midi_menu){
        if(IsKeyPressed(KeyEscape) || mouse.released.right){
            m_midi_menu=false;return;
        }
        if(mouse.released.left){
            if(pointerInside(mouse.x,mouse.y,m_midi_menu_x+9,
                             m_midi_menu_y+35,237,34)){
                BuildMidiDestinations();return;
            }
            if(pointerInside(mouse.x,mouse.y,m_midi_menu_x+9,
                             m_midi_menu_y+78,237,34)){
                m_midi_action_files.assign(m_selected_midis.begin(),
                                           m_selected_midis.end());
                m_midi_action_target.clear();
                m_midi_confirmation=1;m_midi_menu=false;return;
            }
            m_midi_menu=false;
        }
        return;
    }
    if(m_folder_batch_confirm){
        if(IsKeyPressed(KeyEscape)){
            m_folder_batch_confirm=false;
            m_folder_action_files.clear();m_folder_action_target.clear();
            return;
        }
        if(IsKeyPressed(KeyEnter)){ExecuteFolderBatch();return;}
        if(mouse.released.left){
            const int x=GetStateWidth()/2,y=GetStateHeight()/2;
            if(pointerInside(mouse.x,mouse.y,x-206,y+65,176,44)){
                ExecuteFolderBatch();return;
            }
            if(pointerInside(mouse.x,mouse.y,x+30,y+65,176,44)){
                m_folder_batch_confirm=false;
                m_folder_action_files.clear();m_folder_action_target.clear();
                return;
            }
        }
        return;
    }
    if(m_drop_choice){
        // One dialogue, two explicit actions, plus cancellation.
        if(IsKeyPressed(KeyEscape)){
            m_drop_choice=false;
            m_drop_src.clear();m_drop_target.clear();
            return;
        }
        if(mouse.released.left){
            const int x=GetStateWidth()/2,y=GetStateHeight()/2;
            if(pointerInside(mouse.x,mouse.y,x-288,y+70,176,46)){
                ApplyDropChoice(false);return;
            }
            if(pointerInside(mouse.x,mouse.y,x-88,y+70,176,46)){
                ApplyDropChoice(true);
                return;
            }
            if(pointerInside(mouse.x,mouse.y,x+112,y+70,176,46)){
                m_drop_choice=false;
                m_drop_src.clear();m_drop_target.clear();
                return;
            }
        }
        return;
    }
    if(m_confirm){
        if(IsKeyPressed(KeyEscape)){
            m_confirm=0;return;
        }
        if(IsKeyPressed(KeyEnter)){
            ExecuteConfirmation();return;
        }
        const int x=GetStateWidth()/2,y=GetStateHeight()/2;
        if(mouse.released.left){
            if(pointerInside(mouse.x,mouse.y,x-206,y+65,176,44)){
                ExecuteConfirmation();return;
            }
            if(pointerInside(mouse.x,mouse.y,x+30,y+65,176,44)){
                m_confirm=0;return;
            }
        }
        return;
    }
    if(m_trash_open){
        if(IsKeyPressed(KeyEscape)){m_trash_open=false;return;}
        const auto entries=m_personal.Deleted();
        const int pages=std::max(1,static_cast<int>((entries.size()+5)/6));
        m_trash_page=std::max(0,std::min(m_trash_page,pages-1));
        if(IsKeyPressed(KeyLeft) && m_trash_page>0)--m_trash_page;
        if(IsKeyPressed(KeyRight) && m_trash_page+1<pages)++m_trash_page;
        if(mouse.released.left){
            const int x=GetStateWidth()/2,y=GetStateHeight()/2;
            const int top=y-184;
            if(pointerInside(mouse.x,mouse.y,x-280,top+65,560,6*42)){
                const int row=(mouse.y-(top+65))/42;
                const int index=m_trash_page*6+row;
                if(index>=0&&index<static_cast<int>(entries.size())){
                    m_trash_open=false;
                    OpenConfirmation(3,entries[index].ticket,"");
                }
            }else if(pointerInside(mouse.x,mouse.y,x-88,y+151,176,35))
                m_trash_open=false;
            else if(mouse.y>=top+24&&mouse.y<top+61){
                if(mouse.x<x-100 && m_trash_page>0)--m_trash_page;
                if(mouse.x>x+100 && m_trash_page+1<pages)++m_trash_page;
            }
        }
        return;
    }
    if(m_recent_options_open){
        const int w=std::min(660,GetStateWidth()-30);
        const int h=430;
        const int x=(GetStateWidth()-w)/2;
        const int y=(GetStateHeight()-h)/2;
        if(IsKeyPressed(KeyEscape)){
            if(m_recent_dropdown)m_recent_dropdown=0;
            else m_recent_options_open=false;
            return;
        }
        if(mouse.released.left && m_recent_dropdown){
            const int dropdown=m_recent_dropdown;
            const int top=y+(dropdown==1?140:216);
            const int count=dropdown==1?20:4;
            const int cols=dropdown==1?4:1;
            const int cellW=(w-200)/cols;
            const int cellH=dropdown==1?34:32;
            if(pointerInside(mouse.x,mouse.y,x+100,top,
                             cellW*cols,cellH*((count+cols-1)/cols))){
                const int row=(mouse.y-top)/cellH;
                const int col=(mouse.x-(x+100))/cellW;
                const int index=row*cols+col;
                if(index>=0 && index<count){
                    if(dropdown==1)m_recent_limit_draft=(index+1)*10;
                    else m_recent_sort_draft=index;
                }
            }
            // Clic exterior: cerrar la lista sin activar controles ocultos.
            m_recent_dropdown=0;
            return;
        }
        if(IsKeyPressed(KeyEnter) ||
           (mouse.released.left &&
            pointerInside(mouse.x,mouse.y,x+55,y+365,205,42))){
            AdvancedSettings actual=AdvancedSettingsIO::Load();
            actual.recentLimit=m_recent_limit_draft;
            actual.recentSort=m_recent_sort_draft;
            std::string error;
            if(!AdvancedSettingsIO::Save(actual,error)){
                m_error="No se guardó Recientes: "+error;return;
            }
            m_preferences=actual;
            m_recent_dropdown=0;
            m_recent_options_open=false;
            m_notice="Recientes: preferencias guardadas.";
            if(m_current_path==RecentFolderPath())UpdateSongTiles();
            return;
        }
        if(mouse.released.left){
            if(pointerInside(mouse.x,mouse.y,x+w-259,y+365,205,42)){
                m_recent_dropdown=0;m_recent_options_open=false;return;
            }
            const auto cycle=[](int n,int dir,int total){
                return (n+total+dir)%total;
            };
            if(pointerInside(mouse.x,mouse.y,x+45,y+100,48,42)){
                m_recent_limit_draft=std::max(1,m_recent_limit_draft-1);
            }else if(pointerInside(mouse.x,mouse.y,x+w-93,y+100,48,42)){
                m_recent_limit_draft=std::min(500,m_recent_limit_draft+1);
            }else if(pointerInside(mouse.x,mouse.y,x+100,y+100,w-200,42)){
                m_recent_dropdown=1;
            }else if(pointerInside(mouse.x,mouse.y,x+45,y+176,48,42)){
                m_recent_sort_draft=cycle(m_recent_sort_draft,-1,4);
            }else if(pointerInside(mouse.x,mouse.y,x+w-93,y+176,48,42)){
                m_recent_sort_draft=cycle(m_recent_sort_draft,+1,4);
            }else if(pointerInside(mouse.x,mouse.y,x+100,y+176,w-200,42)){
                m_recent_dropdown=2;
            }else if(!pointerInside(mouse.x,mouse.y,x,y,w,h)){
                // No propagar clics de este diálogo a la biblioteca.
                m_recent_options_open=false;
            }
        }
        return;
    }
    if(m_context_open){
        if(m_context_path==RecentFolderPath()){
            if(IsKeyPressed(KeyEscape)||mouse.released.right){
                m_context_open=false;return;
            }
            if(mouse.released.left){
                if(pointerInside(mouse.x,mouse.y,m_context_x+10,
                                  m_context_y+37,188,35))
                    OpenRecentOptions();
                else m_context_open=false;
            }
            return;
        }
        if(IsKeyPressed(KeyEscape) || mouse.released.right){
            m_context_open=false;return;
        }
        if(mouse.released.left){
            if(pointerInside(mouse.x,mouse.y,m_context_x+10,m_context_y+40,188,35)){
                if(m_personal.OwnedChild(m_context_path))
                    OpenConfirmation(1,m_context_path,"");
                else {
                    m_context_open=false;
                    m_error="Carpeta original protegida: no se puede eliminar.";
                }
            }else if(pointerInside(mouse.x,mouse.y,m_context_x+10,
                         m_context_y+84,188,35)){
                std::string error;
                if(m_placements &&
                   m_placements->CanReturnToMain(m_context_path,error))
                    OpenConfirmation(4,m_context_path,"");
                else{
                    m_context_open=false;
                    m_error=error.empty()?
                      "La carpeta ya está en principales.":error;
                }
            }else if(pointerInside(mouse.x,mouse.y,m_context_x+10,
                         m_context_y+128,188,35)){
                m_trash_open=true;m_context_open=false;m_trash_page=0;
            }else if(pointerInside(mouse.x,mouse.y,m_context_x+10,
                         m_context_y+172,188,35)){
                StartRenameFolder();
            }else m_context_open=false;
        }
        return;
    }
    // History controls are inert during dialogues and text entry: those
    // branches returned above. Debounce Ctrl+Z / Ctrl+Y by key release.
    m_undo_button.Update(mouse);
    m_redo_button.Update(mouse);
    if(m_current_path==m_base_path){
        m_import_button.Update(mouse);
        if(m_import_button.hit){StartMidiImport();return;}
    }
    const Uint8* keyboard=SDL_GetKeyboardState(nullptr);
    const bool control=(SDL_GetModState() & KMOD_CTRL)!=0;
    const bool shifted=(SDL_GetModState() & KMOD_SHIFT)!=0;
    const bool undoKey=control && !shifted && keyboard[SDL_SCANCODE_Z];
    const bool redoKey=control &&
       (keyboard[SDL_SCANCODE_Y] ||
       (shifted && keyboard[SDL_SCANCODE_Z]));
    if(!undoKey && !redoKey)m_history_shortcut_latch=false;
    const bool undoPress=m_undo_button.hit ||
       (undoKey && !m_history_shortcut_latch);
    const bool redoPress=m_redo_button.hit ||
       (redoKey && !m_history_shortcut_latch);
    if(undoPress||redoPress){
        m_history_shortcut_latch=undoKey||redoKey;
        if(undoPress && !m_undo_stack.empty())ApplyEdit(true);
        else if(redoPress && !m_redo_stack.empty())ApplyEdit(false);
        return;
    }
    // Ctrl+A selects all MIDI files in this folder, including other pages.
    const Uint8* keys=SDL_GetKeyboardState(nullptr);
    const bool selectAll=(SDL_GetModState() & KMOD_CTRL)!=0 &&
                         keys[SDL_SCANCODE_A];
    if(selectAll && !m_select_all_latch && m_midi_count>0){
        m_selected_midis.clear();
        for(int i=m_folder_count;i<static_cast<int>(m_song_tiles.size());++i)
            m_selected_midis.insert(m_song_tiles[i].GetPath());
        m_selection_anchor=m_song_tiles[m_folder_count].GetPath();
        m_notice=std::to_string(m_selected_midis.size())+" MIDI seleccionados.";
    }
    m_select_all_latch=selectAll;
    // Open the right-click menu for either files or folders.
    if(mouse.released.right){
        const int index=TileAt(mouse.x,mouse.y);
        if(index>=0 && m_song_tiles[index].IsDir())
            OpenContext(mouse.x,mouse.y,index);
        else {
            const int midi=MidiAt(mouse.x,mouse.y);
            if(midi>=0 && m_current_path!=RecentFolderPath())
                OpenMidiMenu(mouse.x,mouse.y,midi);
        }
        return;
    }
    // La rueda dentada pertenece a Recientes, pero es una acción
    // independiente de la apertura y del arrastre de su tarjeta.
    if(mouse.released.left && RecentGearHit(mouse.x,mouse.y) &&
       !m_dragging && !m_midi_move_dragging){
        m_recent_gear_pressed=false;
        m_pressed_path.clear();
        OpenRecentOptions();
        return;
    }
    if(m_recent_gear_pressed){
        if(mouse.released.left || !mouse.held.left)
            m_recent_gear_pressed=false;
        return;
    }
    if(mouse.newPress.left && RecentGearHit(mouse.x,mouse.y)){
        m_recent_gear_pressed=true;
        m_pressed_path.clear();
        m_last_clicked_folder.clear();
        return;
    }
    // Track the entire gesture, not just the mouse-release event.
    if(mouse.newPress.left){
        if(std::getenv("LINTHESIA_DRAG_TRACE"))
           std::fprintf(stderr,"[DRAG] DOWN(%d,%d) file=%d folder=%d\n",
                      mouse.x,mouse.y,MidiAt(mouse.x,mouse.y),TileAt(mouse.x,mouse.y));
        const int index=TileAt(mouse.x,mouse.y);
        m_pressed_path=(index>=0 && m_song_tiles[index].IsDir() &&
                        !FixedFolder(m_song_tiles[index].GetPath()))?
                          m_song_tiles[index].GetPath():"";
        m_press_x=mouse.x;m_press_y=mouse.y;
        m_dragging=false;m_drag_path.clear();
        m_folder_drag_files.clear();
        m_folder_drag_origin=m_current_path;
        m_folder_drag_navigations=0;
        m_page_hover_start=0;
        m_page_hover_target=0;
        m_folder_hover_target.clear();
        // La seleccion de carpetas no debe interceptar el arrastre MIDI.
        const int clickedMidi=MidiAt(mouse.x,mouse.y);
        m_folder_rect_candidate=m_preferences.multipleSelect &&
            m_pressed_path.empty() && clickedMidi<0 &&
            m_folder_count>0 && mouse.x>=16 &&
            mouse.x<GetStateWidth()-16 &&
            mouse.y>=(m_compact_subfolders?98:135) &&
            mouse.y<m_file_list_top-3;
        if(index>=0 && m_song_tiles[index].IsDir() &&
           FixedFolder(m_song_tiles[index].GetPath()))
            m_folder_rect_candidate=false;
        m_folder_rect_selecting=false;
        m_folder_rect_x=m_folder_rect_last_x=mouse.x;
        m_folder_rect_y=m_folder_rect_last_y=mouse.y;
        m_folder_rect_additive=(SDL_GetModState() & KMOD_CTRL)!=0;
        m_folder_before_rect=m_selected_folders;
        // A drag starting on any MIDI row or the empty list area is a
        // marquee-selection gesture, not a file-opening gesture.
        // Cualquier MIDI se puede coger directamente, incluso sin
        // seleccion previa. Ctrl/Mayus conserva la seleccion por rectangulo.
        const bool midiModifiers=(SDL_GetModState() &
                                    (KMOD_CTRL|KMOD_SHIFT))!=0;
        m_midi_move_candidate=clickedMidi>=0 && !midiModifiers &&
                              m_current_path!=RecentFolderPath();
        m_midi_move_dragging=false;
        m_midi_moving_files.clear();
        m_midi_drag_source_folder=m_current_path;
        m_midi_navigation_steps=0;
        m_midi_hover_destination.clear();
        m_midi_hover_started=0;
        if(m_midi_move_candidate){
            const std::string clicked=m_song_tiles[clickedMidi].GetPath();
            if(m_selected_midis.count(clicked))
                m_midi_moving_files.assign(m_selected_midis.begin(),
                                           m_selected_midis.end());
            else m_midi_moving_files={clicked};
        }
        m_page_hover_start=0;
        m_page_hover_target=0;
        m_midi_drag_candidate=!m_midi_move_candidate && (clickedMidi>=0 || (
            mouse.x>=m_file_list_left &&
            mouse.x<m_file_list_left+m_file_list_width &&
            mouse.y>=m_file_list_top &&
            mouse.y<GetStateHeight()-105)) &&
            m_pressed_path.empty() && m_midi_count>0;
        m_midi_select_dragging=false;
        m_midi_drag_x=m_midi_drag_last_x=mouse.x;
        m_midi_drag_y=m_midi_drag_last_y=mouse.y;
        m_midi_drag_additive=(SDL_GetModState() & KMOD_CTRL)!=0;
        m_midi_before_drag=m_selected_midis;
    }
    // Drag an empty part of the folder grid to select a rectangular group.
    if(m_folder_rect_candidate && mouse.held.left){
        m_folder_rect_last_x=mouse.x;
        m_folder_rect_last_y=mouse.y;
        if(abs(mouse.x-m_folder_rect_x)>9 ||
           abs(mouse.y-m_folder_rect_y)>9)
            m_folder_rect_selecting=true;
        if(m_folder_rect_selecting){
            const int left=std::min(m_folder_rect_x,mouse.x);
            const int right=std::max(m_folder_rect_x,mouse.x);
            const int top=std::min(m_folder_rect_y,mouse.y);
            const int bottom=std::max(m_folder_rect_y,mouse.y);
            m_selected_folders=m_folder_rect_additive?
                m_folder_before_rect:std::set<std::string>();
            for(int i=0;i<m_folder_count;++i){
                const auto& tile=m_song_tiles[i];
                if(!tile.IsVisible())continue;
                const auto size=tile.WholeTile();
                if(tile.GetX()<=right && tile.GetX()+size.w>=left &&
                   tile.GetY()<=bottom && tile.GetY()+size.h>=top &&
                   !FixedFolder(tile.GetPath()))
                    m_selected_folders.insert(tile.GetPath());
            }
            m_last_clicked_folder.clear();
            return;
        }
    }
    if(m_folder_rect_selecting && mouse.released.left){
        m_folder_rect_selecting=false;
        m_folder_rect_candidate=false;
        m_folder_before_rect.clear();
        m_notice=std::to_string(m_selected_folders.size())+
                 " carpeta(s) seleccionadas con el ratón.";
        return;
    }
    if(mouse.released.left){
        m_folder_rect_candidate=false;
        m_folder_before_rect.clear();
    }
    // Dragging an already selected file transports the entire selection.
    // Drop directly on a visible folder, or elsewhere outside the MIDI rows
    // to open the complete destination list. No movement before confirmation.
    if(m_midi_move_candidate && mouse.held.left){
        m_midi_drag_last_x=mouse.x;
        m_midi_drag_last_y=mouse.y;
        if(abs(mouse.x-m_midi_drag_x)>9 ||
           abs(mouse.y-m_midi_drag_y)>9)
            m_midi_move_dragging=true;
        if(m_midi_move_dragging){
            if(m_midi_moving_files.size()==1 &&
               !m_selected_midis.count(m_midi_moving_files.front())){
                m_selected_midis.clear();
                m_selected_midis.insert(m_midi_moving_files.front());
                m_selection_anchor=m_midi_moving_files.front();
            }
            UpdateMidiDragHover(mouse.x,mouse.y);
            UpdateDragPagination(mouse.x,mouse.y);
            return;
        }
    }
    if(m_midi_move_dragging && mouse.released.left){
        m_midi_move_dragging=false;
        m_midi_move_candidate=false;
        const int folder=TileAt(mouse.x,mouse.y);
        if(folder>=0 && m_song_tiles[folder].GetPath()==RecentFolderPath()){
            m_error="Recientes se rellena sola al reproducir: no admite archivos.";
        }else if(folder>=0){
            m_midi_action_files=m_midi_moving_files;
            m_midi_action_target=m_song_tiles[folder].GetPath();
            m_midi_confirmation=2;
            m_error.clear();
        }else if(m_midi_navigation_steps>0 &&
                  m_current_path!=m_midi_drag_source_folder){
            // Files can be released inside a folder reached by hovering,
            // even if that destination has no visible subfolder to drop on.
            m_midi_action_files=m_midi_moving_files;
            m_midi_action_target=m_current_path;
            m_midi_confirmation=2;
            m_error.clear();
        }else if(MidiAt(mouse.x,mouse.y)<0){
            BuildMidiDestinations();
            m_notice="Selecciona la carpeta donde soltar tus archivos.";
        }else{
            m_notice="Suelta fuera de la lista MIDI para elegir carpeta.";
        }
        m_midi_hover_destination.clear();
        m_midi_hover_started=0;
        m_midi_navigation_steps=0;
        m_midi_drag_source_folder.clear();
        m_midi_moving_files.clear();
        m_page_hover_start=0;m_page_hover_target=0;
        m_last_clicked_midi.clear();
        return;
    }
    if(mouse.released.left){
        m_midi_move_candidate=false;
        m_midi_moving_files.clear();
    }
    if(m_midi_drag_candidate && mouse.held.left){
        m_midi_drag_last_x=mouse.x;
        m_midi_drag_last_y=mouse.y;
        if(abs(mouse.x-m_midi_drag_x)>8 ||
           abs(mouse.y-m_midi_drag_y)>8)
            m_midi_select_dragging=true;
        if(m_midi_select_dragging){
            const int left=std::min(m_midi_drag_x,mouse.x);
            const int right=std::max(m_midi_drag_x,mouse.x);
            const int top=std::min(m_midi_drag_y,mouse.y);
            const int bottom=std::max(m_midi_drag_y,mouse.y);
            m_selected_midis=m_midi_drag_additive?
                             m_midi_before_drag:std::set<std::string>();
            for(int i=m_folder_count;i<static_cast<int>(m_song_tiles.size());++i){
                const auto& tile=m_song_tiles[i];
                if(!tile.IsVisible())continue;
                const ButtonState hit=tile.WholeTile();
                if(tile.GetX()<=right && tile.GetX()+hit.w>=left &&
                   tile.GetY()<=bottom && tile.GetY()+hit.h>=top)
                    m_selected_midis.insert(tile.GetPath());
            }
            m_last_clicked_midi.clear();
            m_highlight_path.clear();
            return;
        }
    }
    if(m_midi_select_dragging && mouse.released.left){
        m_midi_select_dragging=false;
        m_midi_drag_candidate=false;
        m_midi_before_drag.clear();
        m_notice=std::to_string(m_selected_midis.size())+
                 " MIDI seleccionados con el ratón.";
        return;
    }
    if(mouse.released.left){
        m_midi_drag_candidate=false;
        m_midi_before_drag.clear();
    }
    if(!m_pressed_path.empty() && mouse.held.left &&
       (abs(mouse.x-m_press_x)>12||abs(mouse.y-m_press_y)>12)){
        if(!m_dragging){
            if(!m_selected_folders.count(m_pressed_path)){
                m_selected_folders.clear();
                m_selected_folders.insert(m_pressed_path);
                m_folder_anchor=m_pressed_path;
            }
            // Snapshot all selected folders when their group is lifted.
            m_folder_drag_files.assign(m_selected_folders.begin(),
                                       m_selected_folders.end());
            m_folder_drag_files.erase(std::remove_if(m_folder_drag_files.begin(),
                m_folder_drag_files.end(),[&](const std::string& path){
                    return FixedFolder(path);
                }),m_folder_drag_files.end());
            m_drag_path=m_pressed_path;
        }
        if(!m_dragging && std::getenv("LINTHESIA_DRAG_TRACE"))
           std::fprintf(stderr,"[DRAG] FOLDER LIFT source=%s\n",m_pressed_path.c_str());
        m_dragging=true;
    }
    if(m_dragging && mouse.held.left){
        UpdateFolderDragHover(mouse.x,mouse.y);
        UpdateDragPagination(mouse.x,mouse.y);
        return;
    }
    if(m_dragging && mouse.released.left){
        CompleteDrag(mouse.x,mouse.y);
        return;
    }
    if(mouse.released.left)m_pressed_path.clear();

    m_back_button.Update(mouse);
    if(m_current_path!=RecentFolderPath()){
      m_create_folder_button.Update(mouse);
      if(m_create_folder_button.hit){StartCreateFolder();return;}
    }

    if(IsKeyPressed(KeyEscape) && !m_selected_folders.empty()){
        m_selected_folders.clear();
        m_folder_anchor.clear();
        m_last_clicked_folder.clear();
        return;
    }
    // Escape first clears a multi-selection, then returns on the next press.
    if(IsKeyPressed(KeyEscape) && !m_selected_midis.empty()){
        m_selected_midis.clear();
        m_selection_anchor.clear();
        m_last_clicked_midi.clear();
        return;
    }
    if (IsKeyPressed(KeyEscape) || m_back_button.hit) {
        CancelMidiImport();
        if (m_state.midi) {
            ChangeState(new TitleState(m_state));
            return;
        }
        else {
            delete m_state.midi_out;
            m_state.midi_out = 0;

            delete m_state.midi_in;
            m_state.midi_in = 0;

            delete m_state.midi;
            m_state.midi = 0;

            Compatible::GracefulShutdown();
            return;
        }
    }

    if (m_current_page + 1 < m_page_count) {
        m_next_page_button.Update(mouse);

        if (IsKeyPressed(KeyRight) || m_next_page_button.hit) {
            m_current_page++;
            UpdateSongTilesPage();
        }
    }

    if (m_current_page > 0) {
        m_prev_page_button.Update(mouse);

        if (IsKeyPressed(KeyLeft) || m_prev_page_button.hit) {
            m_current_page--;
            UpdateSongTilesPage();
        }
    }

    if(m_midi_count>0 && m_file_page_count>1){
        m_file_prev_button.Update(mouse);
        m_file_next_button.Update(mouse);
        if(m_file_prev_button.hit && m_file_page>0){
            --m_file_page;
            UpdateSongTilesPage();
            return;
        }
        if(m_file_next_button.hit && m_file_page+1<m_file_page_count){
            ++m_file_page;
            UpdateSongTilesPage();
            return;
        }
    }

    const std::string path_title=VisibleFolderPath();
    if (path_title.length() > 0) {
        m_path_up_button.Update(mouse);

        if (IsKeyPressed(KeyBackward) || m_path_up_button.hit) {
            m_skip_next_mouse_up = true;
            m_current_page = 0;
            GoUpDirectory();
        }
    }

    for(std::vector<SongTile>::size_type i = 0; i < m_song_tiles.size(); i++) {
        MouseInfo tile_mouse(mouse);
        tile_mouse.x -= m_song_tiles[i].GetX();
        tile_mouse.y -= m_song_tiles[i].GetY();
        m_song_tiles[i].Update(tile_mouse);
    }

    // Repeated plain mouse clicks ADD/REMOVE MIDI without keyboard modifiers.
    // Shift selects a contiguous range; Ctrl remains optional.
    // Double-click opens the song instead of toggling off its selection.
    const SDL_Keymod mods=SDL_GetModState();
    const bool ctrl=(mods & KMOD_CTRL)!=0;
    const bool shift=(mods & KMOD_SHIFT)!=0;
    if(mouse.released.left){
        const int chosen=MidiAt(mouse.x,mouse.y);
        if(chosen>=0){
            const std::string path=m_song_tiles[chosen].GetPath();
            const Uint32 now=SDL_GetTicks();
            const bool twice=!ctrl&&!shift &&
                path==m_last_clicked_midi &&
                (now-m_last_midi_click)<static_cast<Uint32>(m_preferences.doubleClickMs);
            m_last_clicked_midi=(!ctrl&&!shift)?path:std::string();
            m_last_midi_click=now;
            if(twice){
                OpenTitleState(path);
                return;
            }
            SelectMidi(chosen,m_preferences.multipleSelect && !shift,
                       m_preferences.multipleSelect && shift);
            m_highlight_path.clear();
            m_notice.clear();
            return;
        }
    }
    for(std::vector<SongTile>::size_type i = 0; i < m_song_tiles.size(); i++) {
        if(m_song_tiles[i].IsVisible() && m_song_tiles[i].WholeTile().hit) {

            if (m_song_tiles[i].IsDir()) {
                const std::string path=m_song_tiles[i].GetPath();
                const Uint32 now=SDL_GetTicks();
                const bool ctrl=(SDL_GetModState() & KMOD_CTRL)!=0;
                const bool shift=(SDL_GetModState() & KMOD_SHIFT)!=0;
                const bool twice=!ctrl&&!shift &&
                  path==m_last_clicked_folder &&
                  now-m_last_folder_click<static_cast<Uint32>(m_preferences.doubleClickMs);
                m_last_clicked_folder=(!ctrl&&!shift)?path:std::string();
                m_last_folder_click=now;
                if(twice){
                    // Double click opens the folder, just like a MIDI.
                    m_skip_next_mouse_up=true;
                    m_current_path=path;
                    UserSetting::Set(SONG_LIB_DIR_SETTINGS_KEY,m_current_path);
                    m_notice.clear();m_error.clear();
                    UpdateSongTiles();
                }else if(FixedFolder(path)){
                    m_notice="Carpeta protegida: doble clic para abrir.";
                }else if(shift && !m_folder_anchor.empty()){
                    auto first=std::find_if(m_song_tiles.begin(),
                      m_song_tiles.begin()+m_folder_count,
                      [&](const SongTile& tile){
                       return tile.GetPath()==m_folder_anchor;});
                    if(first!=m_song_tiles.begin()+m_folder_count){
                        const int from=first-m_song_tiles.begin();
                        const int to=static_cast<int>(i);
                        for(int k=std::min(from,to);k<=std::max(from,to);++k)
                            if(!FixedFolder(m_song_tiles[k].GetPath()))
                                m_selected_folders.insert(m_song_tiles[k].GetPath());
                    }else m_selected_folders.insert(path);
                }else{
                    if(!m_preferences.multipleSelect){
                        m_selected_folders.clear();
                        m_selected_folders.insert(path);
                    }else if(m_selected_folders.count(path))
                        m_selected_folders.erase(path);
                    else m_selected_folders.insert(path);
                    m_folder_anchor=path;
                }
                m_notice=std::to_string(m_selected_folders.size())+
                         " carpeta(s) seleccionadas. Doble clic: abrir.";
            }
            else {
                OpenTitleState(m_song_tiles[i].GetPath());
            }

            return;
        }
    }
    // Click in a genuine empty area deselects both folders and MIDI.
    // Never mistake toolbar buttons, search, or pagination for empty space.
    if(mouse.released.left &&
       TileAt(mouse.x,mouse.y)<0 && MidiAt(mouse.x,mouse.y)<0){
        const auto insideButton=[&](const ButtonState& button){
            return pointerInside(mouse.x,mouse.y,
                button.x,button.y,button.w,button.h);
        };
        const bool onControl=
            insideButton(m_back_button)||
            insideButton(m_create_folder_button)||
            insideButton(m_undo_button)||
            insideButton(m_redo_button)||
            (m_current_path==m_base_path && insideButton(m_import_button))||
            insideButton(m_prev_page_button)||
            insideButton(m_next_page_button)||
            (m_compact_subfolders && (
                insideButton(m_path_up_button)||
                insideButton(m_search_field)||
                insideButton(m_search_button)||
                insideButton(m_file_prev_button)||
                insideButton(m_file_next_button)));
        if(!onControl && (!m_selected_folders.empty() ||
                          !m_selected_midis.empty()))
            ClearSelections();
    }
}

void SongLibState::GoUpDirectory() {
    if(m_current_path==RecentFolderPath()){
      m_current_path=m_base_path;
      UserSetting::Set(SONG_LIB_DIR_SETTINGS_KEY,m_current_path);
      UpdateSongTiles();return;
    }
    std::string parent=m_current_path.substr(0,m_current_path.find_last_of("/"));
    const std::string virtualParent=m_placements?
                                    m_placements->Parent(m_current_path):"";
    if(!virtualParent.empty())
        parent=virtualParent;
    else if(m_personal.Owns(m_current_path) &&
       (parent==m_personal.Root() || !m_personal.Owns(parent)))
        parent=m_base_path;
    else if(!contained(m_base_path,parent) && !m_personal.Owns(parent))
        parent=m_base_path;
    m_current_path=parent;
    m_notice.clear();
    m_error.clear();
    UserSetting::Set(SONG_LIB_DIR_SETTINGS_KEY,m_current_path);
    UpdateSongTiles();
}

void SongLibState::OpenTitleState(string path) {
    Midi *midi = 0;
    std::string annotations;

    try {
        std::string input=path;
        if(MusicXmlImport::IsXml(path)){
            input=MusicXmlImport::Convert(path);
            annotations=input+".score.tsv";
        }
        midi = new Midi(Midi::ReadFromFile(input.c_str()));
    }
    catch (const std::exception &e) {
        Compatible::ShowError(std::string("No se pudo importar MusicXML: ")+e.what());
        midi=0;
    }
    catch (const MidiError &e) {
        string wrapped_description = STRING("Error al cargar el archivo: " <<
                                            path.c_str() <<
                                            "\n") + e.GetErrorDescription();

        Compatible::ShowError(wrapped_description);

        midi = 0;
    }

    if (midi) {
        // Never carry track modes/colors from a different MIDI file.
        if(m_state.song_path!=path)m_state.track_properties.clear();
        m_state.midi = midi;
        m_state.song_title = FileSelector::TrimFilename(path.c_str());
        const auto songInfo=SongMetadata::Load(path);
        if(!songInfo.title.empty())m_state.song_title=songInfo.title;
        m_state.song_path=path;
        m_state.musicxml_score_path=annotations;
        RecentSongs::RememberLastSelected(path);
        ChangeState(new TitleState(m_state));
    }
}

void SongLibState::Draw(Renderer &renderer) const {

    Layout::DrawButton(renderer, m_back_button,
        m_state.midi ? GetTexture(ButtonBackToTitle) : GetTexture(ButtonExit));
    if(m_current_path!=RecentFolderPath())
      Layout::DrawButton(renderer, m_create_folder_button,
                         GetTexture(ButtonCreateFolder));
    
    if(!m_compact_subfolders && m_current_page > 0) {
        Layout::DrawButton(renderer, m_prev_page_button, GetTexture(ButtonPageBack));
    }
    if(!m_compact_subfolders && m_current_page + 1 < m_page_count) {
        Layout::DrawButton(renderer, m_next_page_button, GetTexture(ButtonPageNext));
    }

    Layout::DrawHorizontalRule(renderer,
                             GetStateWidth(),
                             GetStateHeight() - Layout::ScreenMarginY);

    Layout::DrawHorizontalRule(renderer,
                             GetStateWidth(),
                             m_compact_subfolders?
                                  std::max(86,m_file_list_top-10):
                                  Layout::ScreenMarginY);

    // Main library toolbar: actions clearly show availability.
    const ButtonState historyButtons[2]={m_undo_button,m_redo_button};
    for(int index=0;index<2;++index){
        const bool available=index==0?!m_undo_stack.empty():
                                       !m_redo_stack.empty();
        const auto& button=historyButtons[index];
        renderer.SetColor(available?73:46,available?102:57,
                          available?104:57);
        renderer.DrawQuad(button.x,button.y,button.w,button.h);
        renderer.SetColor(255,255,255);
        TextWriter caption(button.x+button.w/2,button.y+9,
                           renderer,true,16);
        caption << Text(index==0?"Deshacer":"Rehacer",
                        Renderer::ToColor(available?242:150,
                                          available?246:153,available?244:151));
    }
    if(m_current_path==m_base_path){
        const auto& b=m_import_button;
        renderer.SetColor(73,107,102);
        renderer.DrawQuad(b.x,b.y,b.w,b.h);
        renderer.SetColor(255,255,255);
        TextWriter caption(b.x+b.w/2,b.y+9,renderer,true,16);
        caption << Text("Buscar",Renderer::ToColor(245,248,246));
    }
    for(int i=0;i<m_folder_count;++i){
        if(m_song_tiles[i].IsVisible()){
            const auto& tile=m_song_tiles[i];
            const bool selected=m_selected_folders.count(tile.GetPath())>0;
            if(m_dragging && selected &&
               m_current_path==m_folder_drag_origin)
                continue; // The selected cards are now carried by the mouse.
            renderer.ForceTexture(0);
            if(m_compact_subfolders)
                tile.DrawCompactFolder(renderer,
                    m_compact_geometry.buttonWidth,
                    m_compact_geometry.buttonHeight);
            else
                tile.Draw(renderer);
            if(tile.GetPath()==RecentFolderPath()){
                // El dibujo de la tarjeta conserva su tamaño y su estado
                // hover original; solo añadimos un engranaje independiente.
                const ButtonState area=tile.WholeTile();
                const int iconSize=36;
                const int gx=tile.GetX()+area.w-iconSize-11;
                const int gy=tile.GetY()+(area.h-iconSize)/2;
                const MouseInfo pointer=Mouse();
                const bool iconHover=RecentGearHit(pointer.x,pointer.y);
                if(iconHover){
                    // La tarjeta continúa resaltada mientras el icono
                    // recibe un segundo resaltado independiente.
                    renderer.SetColor(100,153,148);
                    renderer.DrawQuad(tile.GetX(),tile.GetY(),area.w,2);
                    renderer.DrawQuad(tile.GetX(),tile.GetY()+area.h-2,area.w,2);
                    renderer.DrawQuad(tile.GetX(),tile.GetY(),2,area.h);
                    renderer.DrawQuad(tile.GetX()+area.w-2,tile.GetY(),2,area.h);
                }
                renderer.SetColor(iconHover?74:62,iconHover?137:91,
                                  iconHover?135:92);
                renderer.DrawQuad(gx,gy,iconSize,iconSize);
                // Ocho dientes cuadrados y orificio central, sin añadir
                // fuentes ni archivos gráficos ajenos al proyecto.
                const int cx=gx+iconSize/2,cy=gy+iconSize/2;
                renderer.SetColor(iconHover?250:208,iconHover?252:219,
                                  iconHover?248:214);
                renderer.DrawQuad(cx-5,cy-11,10,22);
                renderer.DrawQuad(cx-11,cy-5,22,10);
                renderer.DrawQuad(cx-8,cy-8,16,16);
                for(int side=0;side<4;++side){
                  const int xx=(side%2)?cx+8:cx-11;
                  const int yy=(side<2)?cy-11:cy+8;
                  renderer.DrawQuad(xx,yy,4,4);
                }
                renderer.SetColor(iconHover?74:62,iconHover?137:91,
                                  iconHover?135:92);
                renderer.DrawQuad(cx-4,cy-4,8,8);
            }
            if(selected){
                const ButtonState area=tile.WholeTile();
                renderer.SetColor(89,189,225);
                renderer.DrawQuad(tile.GetX(),tile.GetY(),area.w,3);
                renderer.DrawQuad(tile.GetX(),tile.GetY()+area.h-3,area.w,3);
                renderer.DrawQuad(tile.GetX(),tile.GetY(),3,area.h);
                renderer.DrawQuad(tile.GetX()+area.w-3,tile.GetY(),3,area.h);
            }
        }
    }
    if(m_midi_count>0 && m_compact_subfolders){
        // Adjacent columns have a discreetly different background.
        const int fileFirst=m_file_page*m_file_rows_per_page;
        const int shown=std::min(m_midi_count-fileFirst,m_file_rows_per_page);
        for(int col=0;col<m_midi_grid.columns;++col){
            const int inColumn=std::max(0,std::min(m_midi_grid.rows,
                                             shown-col*m_midi_grid.rows));
            if(inColumn==0)continue;
            const int x=m_midi_grid.left+
                        col*(m_midi_grid.columnWidth+m_midi_grid.gap);
            renderer.SetColor(col%2?61:50,col%2?67:63,col%2?77:65);
            renderer.DrawQuad(x,m_midi_grid.top,m_midi_grid.columnWidth,
                              inColumn*m_midi_grid.rowHeight);
        }
    }
    if(m_midi_count>0){
        // Names remain the only contents of each MIDI row.
        for(size_t i=m_folder_count;i<m_song_tiles.size();++i){
            if(m_song_tiles[i].IsVisible()){
                // Lift the selected source rows into the mouse-held group.
                // They reappear unchanged if the drop is cancelled.
                if(m_midi_move_dragging &&
                   m_current_path==m_midi_drag_source_folder &&
                   m_selected_midis.count(m_song_tiles[i].GetPath())>0)
                    continue;
                renderer.ForceTexture(0);
                m_song_tiles[i].DrawMidiRow(renderer,
                   m_compact_subfolders?m_midi_grid.columnWidth:
                                        m_file_list_width,
                   static_cast<int>(i)-m_folder_count-
                   m_file_page*m_file_rows_per_page,
                   m_song_tiles[i].GetPath()==m_highlight_path ||
                   (m_selected_midis.count(m_song_tiles[i].GetPath())>0 &&
                    !m_midi_move_dragging));
            }
        }
        if(m_file_page_count>1 && !m_compact_subfolders){
            for(int i=0;i<2;++i){
                const auto& b=i?m_file_next_button:m_file_prev_button;
                const bool enabled=i?(m_file_page+1<m_file_page_count):
                                     m_file_page>0;
                renderer.SetColor(enabled?74:54,enabled?84:61,
                                  enabled?82:61);
                renderer.DrawQuad(b.x,b.y,b.w,b.h);
                renderer.SetColor(255,255,255);
                TextWriter direction(b.x+b.w/2,b.y+8,renderer,true,15);
                direction << Text(i?"Siguiente":"Anterior",
                                  Renderer::ToColor(enabled?229:143,
                                                    enabled?232:146,
                                                    enabled?230:145));
            }
        }
    }

    // Two small, separately labelled pagination controls for child-folder
    // and MIDI pages. Neither uses the old overlapping bitmap buttons.
    if(m_compact_subfolders){
        auto drawPager=[&](const char* label,const ButtonState& prev,
                           const ButtonState& next,int page,int pages){
            if(pages<=1)return;
            renderer.SetColor(255,255,255);
            TextWriter group(GetStateWidth()-239,prev.y+4,
                             renderer,false,13);
            group << Text(label,Renderer::ToColor(203,215,211));
            const ButtonState* controls[]={&prev,&next};
            for(int i=0;i<2;++i){
                const ButtonState& b=*controls[i];
                const bool enabled=i==0?page>0:page+1<pages;
                renderer.SetColor(enabled?83:49,enabled?99:59,
                                  enabled?92:57);
                renderer.DrawQuad(b.x,b.y,b.w,b.h);
                renderer.SetColor(255,255,255);
                TextWriter mark(b.x+b.w/2,b.y+3,renderer,true,16);
                mark << Text(i==0?"<":">",Renderer::ToColor(
                      enabled?232:140,enabled?240:148,enabled?236:144));
            }
        };
        drawPager("Carpetas",m_prev_page_button,m_next_page_button,
                  m_current_page,m_page_count);
    }

    if(m_compact_subfolders){
        renderer.SetColor(255,255,255);
        TextWriter pageInfo(GetStateWidth()-238,22,renderer,false,13);
        pageInfo << Text("MIDI "+
            std::to_string(m_file_page+1)+"/"+
            std::to_string(m_file_page_count),
            Renderer::ToColor(205,217,214));
        for(int i=0;i<2;++i){
            const ButtonState& button=i?m_file_next_button:m_file_prev_button;
            const bool enabled=i?m_file_page+1<m_file_page_count:m_file_page>0;
            renderer.SetColor(enabled?92:51,enabled?108:60,
                              enabled?107:60);
            renderer.DrawQuad(button.x,button.y,button.w,button.h);
            renderer.SetColor(255,255,255);
            TextWriter arrow(button.x+button.w/2,button.y+5,
                             renderer,true,19);
            arrow << Text(i?">":"<",Renderer::ToColor(
                              enabled?243:136,enabled?248:145,enabled?244:145));
        }
        renderer.SetColor(m_search_editing?106:69,
                          m_search_editing?125:82,
                          m_search_editing?125:83);
        renderer.DrawQuad(m_search_field.x,m_search_field.y,
                          m_search_field.w,m_search_field.h);
        renderer.SetColor(255,255,255);
        TextWriter searchText(m_search_field.x+7,m_search_field.y+6,
                              renderer,false,14);
        std::string query=m_search_query;
        if(query.empty())query="Buscar un MIDI...";
        else if(g_utf8_validate(query.c_str(),-1,nullptr) &&
                g_utf8_strlen(query.c_str(),-1)>21){
            gchar* tail=g_utf8_substring(query.c_str(),
                            g_utf8_strlen(query.c_str(),-1)-21,
                            g_utf8_strlen(query.c_str(),-1));
            query=std::string("...")+tail;g_free(tail);
        }
        if(m_search_editing)query+="_";
        searchText << Text(query,Renderer::ToColor(236,244,240));
        renderer.SetColor(74,91,86);
        renderer.DrawQuad(m_search_button.x,m_search_button.y,
                          m_search_button.w,m_search_button.h);
        renderer.SetColor(255,255,255);
        TextWriter go(m_search_button.x+m_search_button.w/2,
                      m_search_button.y+7,renderer,true,14);
        go << Text("Buscar",Renderer::ToColor(238,244,239));
    }

    if(!m_selected_midis.empty() && GetStateWidth()>=970){
        renderer.SetColor(255,255,255);
        TextWriter selected(GetStateWidth()-505,23,renderer,false,14);
        selected << Text(std::to_string(m_selected_midis.size())+
           " MIDI seleccionados",Renderer::ToColor(166,221,198));
    }
    if(m_midi_move_dragging){
        const MouseInfo cursor=Mouse();
        const int folder=TileAt(cursor.x,cursor.y);
        if(folder>=0){
            const auto& tile=m_song_tiles[folder];
            const ButtonState hit=tile.WholeTile();
            renderer.SetColor(99,205,139);
            renderer.DrawQuad(tile.GetX(),tile.GetY(),hit.w,3);
            renderer.DrawQuad(tile.GetX(),tile.GetY()+hit.h-3,hit.w,3);
            renderer.DrawQuad(tile.GetX(),tile.GetY(),3,hit.h);
            renderer.DrawQuad(tile.GetX()+hit.w-3,tile.GetY(),3,hit.h);
        }
        // Drag the selected *rows themselves*, not a substitute card.
        // Keep the original selection until the actual mouse-up; the visual
        // copies are attached to the cursor for every held-motion frame.
        const int n=static_cast<int>(m_midi_moving_files.size());
        const int rowHeight=29;
        const int width=std::min(420,std::max(230,m_midi_grid.columnWidth));
        const int maxRows=std::max(1,(GetStateHeight()-110)/rowHeight);
        const int maxColumns=std::max(1,std::min(3,(GetStateWidth()-22)/(width+7)));
        const int capacity=maxRows*maxColumns;
        const int shown=std::min(n,capacity);
        const int columns=std::max(1,(shown+maxRows-1)/maxRows);
        const int rows=std::min(maxRows,shown);
        const int stackWidth=columns*width+(columns-1)*7;
        const int stackHeight=rows*rowHeight+38+(n>shown?18:0);
        int x=cursor.x+16;
        int y=cursor.y+16;
        if(x+stackWidth+10>GetStateWidth())x=cursor.x-stackWidth-16;
        if(y+stackHeight+10>GetStateHeight())y=cursor.y-stackHeight-16;
        x=std::max(6,std::min(x,GetStateWidth()-stackWidth-7));
        y=std::max(6,std::min(y,GetStateHeight()-stackHeight-7));
        // A slight offset shadow makes it clear the rows are lifted.
        renderer.SetColor(21,33,42);
        renderer.DrawQuad(x+7,y+7,stackWidth,stackHeight);
        renderer.SetColor(42,76,117);
        renderer.DrawQuad(x,y,stackWidth,stackHeight);
        renderer.SetColor(111,185,210);
        renderer.DrawQuad(x,y,stackWidth,3);
        renderer.SetColor(255,255,255);
        TextWriter count(x+11,y+10,renderer,false,15);
        count << Text(std::to_string(n)+" MIDI seleccionados",
                      Renderer::ToColor(239,247,246));
        for(int i=0;i<shown;++i){
            const int col=i/maxRows;
            const int row=i%maxRows;
            const int rx=x+col*(width+7);
            const int ry=y+34+row*rowHeight;
            renderer.SetColor(42,93,152);
            renderer.DrawQuad(rx,ry,width,rowHeight-1);
            std::string name=shortName(m_midi_moving_files[i]);
            const int maxChars=std::max(7,(width-25)/8);
            if(g_utf8_validate(name.c_str(),-1,nullptr) &&
               g_utf8_strlen(name.c_str(),-1)>maxChars){
                gchar* cut=g_utf8_substring(name.c_str(),0,maxChars-3);
                name=std::string(cut)+"...";g_free(cut);
            }
            renderer.SetColor(255,255,255);
            TextWriter filename(rx+11,ry+5,renderer,false,16);
            filename << Text(name,Renderer::ToColor(255,255,255));
        }
        if(n>shown){
            renderer.SetColor(255,255,255);
            TextWriter more(x+11,y+34+rows*rowHeight,renderer,false,12);
            more << Text("+"+std::to_string(n-shown)+
                         " MIDI adicionales seleccionados",
                         Renderer::ToColor(224,238,246));
        }
        if(!m_midi_hover_destination.empty()){
            const int tipY=std::max(0,y-25);
            renderer.SetColor(33,67,54);
            renderer.DrawQuad(x,tipY,width,21);
            renderer.SetColor(255,255,255);
            TextWriter tip(x+9,tipY+3,renderer,false,13);
            tip << Text("Mantén para abrir: "+
                         TitleFor(m_midi_hover_destination),
                        Renderer::ToColor(216,250,221));
        }
    }
    if(m_midi_select_dragging){
        const int left=std::max(0,std::min(m_midi_drag_x,m_midi_drag_last_x));
        const int right=std::min(GetStateWidth()-1,
                             std::max(m_midi_drag_x,m_midi_drag_last_x));
        const int top=std::max(0,std::min(m_midi_drag_y,m_midi_drag_last_y));
        const int bottom=std::min(GetStateHeight()-1,
                             std::max(m_midi_drag_y,m_midi_drag_last_y));
        renderer.SetColor(101,185,218);
        renderer.DrawQuad(left,top,std::max(1,right-left),2);
        renderer.DrawQuad(left,bottom,std::max(1,right-left),2);
        renderer.DrawQuad(left,top,2,std::max(1,bottom-top));
        renderer.DrawQuad(right,top,2,std::max(1,bottom-top));
    }

    const std::string path_title=VisibleFolderPath();
    if (path_title.length() > 0) {
        // Draw mode text
        TextWriter title(Layout::ScreenMarginX + Layout::ButtonWidth + ColumnMargin, Layout::ScreenMarginY / 2 - 6, renderer, false, Layout::ButtonFontSize);
        std::string displayedPath=path_title;
        if(m_compact_subfolders && g_utf8_validate(displayedPath.c_str(),-1,nullptr)){
            const int maxChars=std::max(5,m_compact_geometry.pathWidth/8);
            const int length=g_utf8_strlen(displayedPath.c_str(),-1);
            if(length>maxChars){
                gchar* suffix=g_utf8_substring(displayedPath.c_str(),
                                               length-(maxChars-3),length);
                displayedPath="..."+std::string(suffix);
                g_free(suffix);
            }
        }
        title << displayedPath.c_str();

        Layout::DrawButton(renderer, m_path_up_button, GetTexture(ButtonDirBack));
    }

    renderer.SetColor(255,255,255);
    if(m_page_count>1 && !m_compact_subfolders){
        // Centre the page label directly ABOVE the paired navigation buttons.
        // Never occupy the footer, where Undo and Redo are located.
        const int groupCentre=(m_prev_page_button.x+
                                m_next_page_button.x+
                                m_next_page_button.w)/2;
        TextWriter pages(groupCentre,12,renderer,true,13);
        pages << Text("Página "+std::to_string(m_current_page+1)+
                      " de "+std::to_string(m_page_count),
                      Renderer::ToColor(204,215,215));
    }
    if(!m_notice.empty()){
        TextWriter note(Layout::ScreenMarginX,
                        m_compact_subfolders?GetStateHeight()-100:
                                            Layout::ScreenMarginY+10,
                        renderer,false,13);
        note << Text(m_notice,Renderer::ToColor(168,223,180));
    }
    if(!m_error.empty()){
        TextWriter warning(Layout::ScreenMarginX,
                           m_compact_subfolders?GetStateHeight()-100:
                                               Layout::ScreenMarginY+10,
                           renderer,false,13);
        warning << Text(m_error,Renderer::ToColor(255,174,160));
    }
    if(m_dragging){
        const MouseInfo cursor=Mouse();
        const int target=TileAt(cursor.x,cursor.y);
        if(target>=0 &&
           !m_selected_folders.count(m_song_tiles[target].GetPath())){
            const auto& tile=m_song_tiles[target];
            const auto size=tile.WholeTile();
            renderer.SetColor(94,204,134);
            renderer.DrawQuad(tile.GetX(),tile.GetY(),size.w,3);
            renderer.DrawQuad(tile.GetX(),tile.GetY()+size.h-3,size.w,3);
            renderer.DrawQuad(tile.GetX(),tile.GetY(),3,size.h);
            renderer.DrawQuad(tile.GetX()+size.w-3,tile.GetY(),3,size.h);
        }
        const int n=static_cast<int>(m_folder_drag_files.size());
        if(n>0){
            // Full folder labels on draggable cards, carried as one group.
            const int width=std::min(325,GetStateWidth()-35);
            const int rowH=43;
            const int maxRows=std::max(1,(GetStateHeight()-120)/rowH);
            const int maxCols=std::max(1,std::min(3,(GetStateWidth()-22)/(width+6)));
            const int shown=std::min(n,maxRows*maxCols);
            const int cols=std::max(1,(shown+maxRows-1)/maxRows);
            const int rows=std::min(maxRows,shown);
            const int blockWidth=cols*width+(cols-1)*6;
            const int blockHeight=31+rows*rowH+(n>shown?18:0);
            int x=cursor.x+14,y=cursor.y+14;
            if(x+blockWidth+8>GetStateWidth())x=cursor.x-blockWidth-14;
            if(y+blockHeight+8>GetStateHeight())y=cursor.y-blockHeight-14;
            x=std::max(5,std::min(x,GetStateWidth()-blockWidth-6));
            y=std::max(5,std::min(y,GetStateHeight()-blockHeight-6));
            renderer.SetColor(24,34,39);
            renderer.DrawQuad(x+6,y+6,blockWidth,blockHeight);
            renderer.SetColor(39,66,77);
            renderer.DrawQuad(x,y,blockWidth,blockHeight);
            renderer.SetColor(107,188,209);
            renderer.DrawQuad(x,y,blockWidth,3);
            renderer.SetColor(255,255,255);
            TextWriter header(x+10,y+8,renderer,false,16);
            header << Text(std::to_string(n)+" carpetas seleccionadas",
                           Renderer::ToColor(234,244,242));
            for(int i=0;i<shown;++i){
                const int col=i/maxRows,row=i%maxRows;
                const int cx=x+col*(width+6),cy=y+29+row*rowH;
                renderer.SetColor(64,87,93);
                renderer.DrawQuad(cx,cy,width,rowH-2);
                renderer.SetColor(118,169,173);
                renderer.DrawQuad(cx,cy,width,2);
                renderer.SetColor(255,255,255);
                std::string name=shortName(m_folder_drag_files[i]);
                const int maxChars=std::max(8,(width-48)/8);
                if(g_utf8_validate(name.c_str(),-1,nullptr) &&
                   g_utf8_strlen(name.c_str(),-1)>maxChars){
                    gchar* cut=g_utf8_substring(name.c_str(),0,maxChars-3);
                    name=std::string(cut)+"...";g_free(cut);
                }
                TextWriter nameText(cx+14,cy+12,renderer,false,16);
                nameText << Text(name,Renderer::ToColor(246,249,248));
            }
            if(n>shown){
                renderer.SetColor(255,255,255);
                TextWriter extra(x+10,y+31+rows*rowH,renderer,false,12);
                extra << Text("+"+std::to_string(n-shown)+" carpetas",
                              Renderer::ToColor(219,236,238));
            }
        }
        if(!m_folder_hover_target.empty()){
            const int hintY=GetStateHeight()-100;
            renderer.SetColor(39,77,54);
            renderer.DrawQuad(10,hintY,std::min(650,GetStateWidth()-20),30);
            renderer.SetColor(255,255,255);
            TextWriter hint(18,hintY+7,renderer,false,14);
            hint << Text("Mantén para subir: "+TitleFor(m_folder_hover_target),
                         Renderer::ToColor(223,246,221));
        }
    }
    if(m_folder_rect_selecting){
        const int x1=std::max(0,std::min(m_folder_rect_x,m_folder_rect_last_x));
        const int x2=std::min(GetStateWidth()-1,
                             std::max(m_folder_rect_x,m_folder_rect_last_x));
        const int y1=std::max(0,std::min(m_folder_rect_y,m_folder_rect_last_y));
        const int y2=std::min(GetStateHeight()-1,
                             std::max(m_folder_rect_y,m_folder_rect_last_y));
        renderer.SetColor(109,198,228);
        renderer.DrawQuad(x1,y1,std::max(1,x2-x1),2);
        renderer.DrawQuad(x1,y2,std::max(1,x2-x1),2);
        renderer.DrawQuad(x1,y1,2,std::max(1,y2-y1));
        renderer.DrawQuad(x2,y1,2,std::max(1,y2-y1));
    }
    if(m_folder_batch_confirm){
        const int x=GetStateWidth()/2,y=GetStateHeight()/2;
        const int width=std::min(640,GetStateWidth()-32);
        renderer.SetColor(29,42,41);
        renderer.DrawQuad(x-width/2,y-126,width,259);
        renderer.SetColor(111,157,144);
        renderer.DrawQuad(x-width/2,y-126,width,3);
        renderer.SetColor(255,255,255);
        TextWriter title(x,y-99,renderer,true,22);
        title << Text("Confirmar traslado de carpetas",
                      Renderer::ToColor(242,246,244));
        TextWriter count(x,y-56,renderer,true,17);
        count << Text(std::to_string(m_folder_action_files.size())+
             " carpeta(s) seleccionada(s)",
             Renderer::ToColor(227,238,235));
        TextWriter target(x,y-18,renderer,true,17);
        target << Text("Destino: "+TitleFor(m_folder_action_target),
                       Renderer::ToColor(220,236,226));
        TextWriter info(x,y+14,renderer,true,13);
        info << Text("Se conserva el contenido original y se organiza su ubicación.",
                     Renderer::ToColor(191,219,211));
        for(int i=0;i<2;++i){
            const int bx=i==0?x-206:x+30;
            renderer.SetColor(74,95,92);
            renderer.DrawQuad(bx,y+65,176,44);
            renderer.SetColor(255,255,255);
            TextWriter button(bx+88,y+76,renderer,true,17);
            button << Text(i==0?"Confirmar":"Cancelar",
                           Renderer::ToColor(235,241,239));
        }
        TextWriter help(x,y+117,renderer,true,12);
        help << Text("Intro: confirmar | Esc: cancelar",
                     Renderer::ToColor(189,210,205));
    }
    if(m_drop_choice){
        const int x=GetStateWidth()/2,y=GetStateHeight()/2;
        const int width=std::min(624,GetStateWidth()-20);
        renderer.SetColor(29,43,42);
        renderer.DrawQuad(x-width/2,y-137,width,293);
        renderer.SetColor(111,141,135);
        renderer.DrawQuad(x-width/2,y-137,width,3);
        renderer.SetColor(255,255,255);
        TextWriter title(x,y-116,renderer,true,24);
        title << Text("¿Dónde quieres soltar la carpeta?",
                      Renderer::ToColor(238,243,239));
        TextWriter from(x,y-73,renderer,true,18);
        from << Text("Mover: "+TitleFor(m_drop_src),
                     Renderer::ToColor(225,234,230));
        TextWriter into(x,y-43,renderer,true,18);
        into << Text("Sobre: "+TitleFor(m_drop_target),
                     Renderer::ToColor(210,229,220));
        TextWriter explanation(x,y-7,renderer,true,14);
        explanation << Text("Mover aquí: intercambiar sus posiciones",
                            Renderer::ToColor(221,235,224));
        TextWriter explanation2(x,y+17,renderer,true,14);
        explanation2 << Text("Soltar dentro: poner la carpeta en la otra",
                             Renderer::ToColor(221,235,224));
        std::string containmentError;
        const bool canNest=m_placements &&
             m_placements->CanContain(m_drop_src,m_drop_target,containmentError);
        for(int i=0;i<3;++i){
            const int left=x-288+i*200;
            const bool enabled=(i!=1 || canNest);
            renderer.SetColor(enabled?73:48,enabled?99:60,enabled?93:60);
            renderer.DrawQuad(left,y+70,176,46);
            renderer.SetColor(255,255,255);
            TextWriter button(left+88,y+82,renderer,true,18);
            button << Text(i==0?"Mover aquí":
                           i==1?"Soltar dentro":"Cancelar",
                           Renderer::ToColor(enabled?241:158,
                                             enabled?241:168,
                                             enabled?239:166));
        }
        TextWriter help(x,y+133,renderer,true,12);
        help << Text(canNest?
             "Elige una opción. Esc cancela; los archivos originales se protegen.":
             "No se puede anidar: "+containmentError,
            Renderer::ToColor(188,213,205));
    }
    if(m_context_open && m_context_path==RecentFolderPath()){
        renderer.SetColor(31,43,43);
        renderer.DrawQuad(m_context_x,m_context_y,208,85);
        renderer.SetColor(97,124,122);
        renderer.DrawQuad(m_context_x,m_context_y,208,2);
        renderer.SetColor(255,255,255);
        TextWriter name(m_context_x+10,m_context_y+9,renderer,false,13);
        name << Text("Recientes · protegida",
                     Renderer::ToColor(210,226,219));
        renderer.SetColor(69,82,81);
        renderer.DrawQuad(m_context_x+10,m_context_y+37,188,35);
        renderer.SetColor(255,255,255);
        TextWriter option(m_context_x+104,m_context_y+45,renderer,true,16);
        option << Text("Configurar Recientes",
                       Renderer::ToColor(236,232,228));
    }
    if(m_context_open && m_context_path!=RecentFolderPath()){
        renderer.SetColor(31,43,43);
        renderer.DrawQuad(m_context_x,m_context_y,208,224);
        renderer.SetColor(97,124,122);
        renderer.DrawQuad(m_context_x,m_context_y,208,2);
        renderer.SetColor(255,255,255);
        TextWriter label(m_context_x+10,m_context_y+9,renderer,false,13);
        label << Text("Carpeta: "+TitleFor(m_context_path),
                      Renderer::ToColor(210,226,219));
        renderer.SetColor(69,82,81);
        renderer.DrawQuad(m_context_x+10,m_context_y+40,188,35);
        renderer.SetColor(255,255,255);
        TextWriter remove(m_context_x+104,m_context_y+45,renderer,true,17);
        remove << Text(m_personal.OwnedChild(m_context_path)?
                            "Eliminar carpeta":"Carpeta protegida",
                       Renderer::ToColor(236,232,228));
        renderer.SetColor(69,82,81);
        renderer.DrawQuad(m_context_x+10,m_context_y+84,188,35);
        renderer.SetColor(255,255,255);
        std::string returnError;
        const bool canReturn=m_placements &&
             m_placements->CanReturnToMain(m_context_path,returnError);
        TextWriter home(m_context_x+104,m_context_y+93,renderer,true,15);
        home << Text(canReturn?"Devolver a principales":
                      "Ya está en principales",
                     Renderer::ToColor(canReturn?236:160,
                                       canReturn?232:172,
                                       canReturn?228:168));
        renderer.SetColor(69,82,81);
        renderer.DrawQuad(m_context_x+10,m_context_y+128,188,35);
        renderer.SetColor(255,255,255);
        TextWriter trash(m_context_x+104,m_context_y+133,renderer,true,17);
        trash << Text("Abrir papelera",Renderer::ToColor(236,232,228));
        const bool canRename=m_personal.OwnedChild(m_context_path)||
                contained(m_base_path,m_context_path);
        renderer.SetColor(canRename?69:49,canRename?82:60,canRename?81:59);
        renderer.DrawQuad(m_context_x+10,m_context_y+172,188,35);
        renderer.SetColor(255,255,255);
        TextWriter rename(m_context_x+104,m_context_y+177,renderer,true,17);
        rename << Text(canRename?"Renombrar carpeta":"No se puede renombrar",
                      Renderer::ToColor(canRename?236:160,canRename?232:172,canRename?228:168));
    }

    if(m_midi_menu){
        renderer.SetColor(30,45,45);
        renderer.DrawQuad(m_midi_menu_x,m_midi_menu_y,255,124);
        renderer.SetColor(105,142,135);
        renderer.DrawQuad(m_midi_menu_x,m_midi_menu_y,255,2);
        renderer.SetColor(255,255,255);
        TextWriter heading(m_midi_menu_x+9,m_midi_menu_y+9,
                           renderer,false,13);
        heading << Text(std::to_string(m_selected_midis.size())+
                        " MIDI seleccionados",Renderer::ToColor(213,234,229));
        for(int i=0;i<2;++i){
            const int y=m_midi_menu_y+35+i*43;
            renderer.SetColor(i==0?64:83,i==0?93:71,i==0?90:72);
            renderer.DrawQuad(m_midi_menu_x+9,y,237,34);
            renderer.SetColor(255,255,255);
            TextWriter line(m_midi_menu_x+19,y+7,renderer,false,15);
            line << Text(i==0?"Mover a carpeta...  >":"Eliminar (a papelera)...",
                         Renderer::ToColor(235,242,238));
        }
    }
    if(m_midi_destinations_open){
        const int width=std::min(760,GetStateWidth()-30);
        const int height=std::min(590,GetStateHeight()-32);
        const int left=(GetStateWidth()-width)/2;
        const int top=(GetStateHeight()-height)/2;
        const int per=std::max(1,(height-160)/38);
        const int pages=std::max(1,
          (static_cast<int>(m_midi_destinations.size())+per-1)/per);
        renderer.SetColor(27,42,43);
        renderer.DrawQuad(left,top,width,height);
        renderer.SetColor(103,146,136);
        renderer.DrawQuad(left,top,width,3);
        renderer.SetColor(255,255,255);
        TextWriter heading(left+17,top+15,renderer,false,22);
        heading << Text("Mover MIDI a carpeta",
                        Renderer::ToColor(240,246,242));
        renderer.SetColor(75,99,95);
        renderer.DrawQuad(left+width-114,top+14,96,30);
        renderer.SetColor(255,255,255);
        TextWriter close(left+width-66,top+21,renderer,true,14);
        close << Text("Cancelar",Renderer::ToColor(234,241,235));
        TextWriter detail(left+20,top+49,renderer,false,14);
        detail << Text("Selecciona una carpeta existente ("+
                        std::to_string(m_selected_midis.size())+" MIDI).",
                        Renderer::ToColor(193,220,216));
        for(int i=0;i<per;++i){
            const int at=m_midi_destination_page*per+i;
            if(at>=static_cast<int>(m_midi_destinations.size()))break;
            const auto& target=m_midi_destinations[at];
            const bool writable=access(target.first.c_str(),W_OK)==0;
            const int y=top+75+i*38;
            renderer.SetColor(i%2?50:60,i%2?68:76,i%2?70:77);
            renderer.DrawQuad(left+16,y,width-32,35);
            std::string text=target.second;
            const int maxChars=std::max(9,(width-58)/8);
            if(g_utf8_validate(text.c_str(),-1,nullptr) &&
               g_utf8_strlen(text.c_str(),-1)>maxChars){
                gchar* cut=g_utf8_substring(text.c_str(),0,maxChars-3);
                text=std::string(cut)+"...";g_free(cut);
            }
            renderer.SetColor(255,255,255);
            TextWriter label(left+27,y+8,renderer,false,15);
            label << Text(text+(writable?"":" (solo lectura)"),
                          Renderer::ToColor(writable?229:145,
                                            writable?237:157,writable?232:151));
        }
        renderer.SetColor(77,103,99);
        renderer.DrawQuad(left+width-248,top+height-57,102,36);
        renderer.DrawQuad(left+width-128,top+height-57,102,36);
        renderer.SetColor(255,255,255);
        TextWriter previous(left+width-197,top+height-48,renderer,true,14);
        previous << Text("Anterior",Renderer::ToColor(235,241,238));
        TextWriter next(left+width-77,top+height-48,renderer,true,14);
        next << Text("Siguiente",Renderer::ToColor(235,241,238));
        TextWriter counter(left+23,top+height-46,renderer,false,13);
        counter << Text("Página "+std::to_string(m_midi_destination_page+1)+
                         " de "+std::to_string(pages),
                         Renderer::ToColor(201,219,215));
        if(!m_error.empty()){
            TextWriter warning(left+24,top+height-75,renderer,false,13);
            warning << Text(m_error,Renderer::ToColor(255,178,162));
        }
    }
    if(m_midi_confirmation){
        const int x=GetStateWidth()/2,y=GetStateHeight()/2;
        const int width=std::min(660,GetStateWidth()-32);
        renderer.SetColor(29,42,41);
        renderer.DrawQuad(x-width/2,y-126,width,259);
        renderer.SetColor(109,140,135);
        renderer.DrawQuad(x-width/2,y-126,width,3);
        renderer.SetColor(255,255,255);
        TextWriter heading(x,y-101,renderer,true,23);
        heading << Text("Confirmación obligatoria",
                        Renderer::ToColor(237,246,241));
        TextWriter question(x,y-53,renderer,true,17);
        question << Text(m_midi_confirmation==1?
            "¿Enviar los MIDI seleccionados a la papelera?":
            "¿Mover los MIDI seleccionados a esta carpeta?",
            Renderer::ToColor(239,233,225));
        TextWriter count(x,y-22,renderer,true,16);
        count << Text(std::to_string(m_midi_action_files.size())+
                      " archivo(s) MIDI",Renderer::ToColor(235,239,232));
        TextWriter details(x,y+7,renderer,true,14);
        details << Text(m_midi_confirmation==1?
          "Recuperables desde la papelera del sistema.":
          "Destino: "+TitleFor(m_midi_action_target),
          Renderer::ToColor(193,222,216));
        for(int i=0;i<2;++i){
            const int left=i==0?x-206:x+30;
            renderer.SetColor(i==0?93:72,i==0?82:96,i==0?76:91);
            renderer.DrawQuad(left,y+65,176,44);
            renderer.SetColor(255,255,255);
            TextWriter button(left+88,y+77,renderer,true,17);
            button << Text(i==0?"Confirmar":"Cancelar",
                           Renderer::ToColor(237,242,240));
        }
        TextWriter hint(x,y+117,renderer,true,12);
        hint << Text("Intro: confirmar | Esc: cancelar",
                     Renderer::ToColor(192,212,207));
    }
    if(m_trash_open){
        const auto entries=m_personal.Deleted();
        const int x=GetStateWidth()/2,y=GetStateHeight()/2;
        const int top=y-184;
        renderer.SetColor(30,43,42);
        renderer.DrawQuad(x-300,top,600,382);
        renderer.SetColor(102,132,129);
        renderer.DrawQuad(x-300,top,600,3);
        renderer.SetColor(255,255,255);
        TextWriter heading(x,top+10,renderer,true,24);
        heading << Text("Papelera de carpetas",
                        Renderer::ToColor(233,241,237));
        const int pages=std::max(1,static_cast<int>((entries.size()+5)/6));
        TextWriter pagesInfo(x,top+39,renderer,true,14);
        pagesInfo << Text("Página "+std::to_string(m_trash_page+1)+
                             " de "+std::to_string(pages)+
                             " | Flechas: cambiar página",
                             Renderer::ToColor(195,214,210));
        for(int i=0;i<6;++i){
            const int index=m_trash_page*6+i;
            if(index>=static_cast<int>(entries.size()))break;
            renderer.SetColor(65,83,80);
            renderer.DrawQuad(x-280,top+65+i*42,560,37);
            renderer.SetColor(255,255,255);
            TextWriter line(x-268,top+72+i*42,renderer,false,16);
            line << Text("Recuperar: "+TitleFor(entries[index].origin),
                         Renderer::ToColor(229,239,233));
        }
        renderer.SetColor(73,94,92);
        renderer.DrawQuad(x-88,y+151,176,35);
        renderer.SetColor(255,255,255);
        TextWriter close(x,y+158,renderer,true,16);
        close << Text("Cerrar",Renderer::ToColor(238,241,239));
    }
    if(m_confirm){
        const int x=GetStateWidth()/2,y=GetStateHeight()/2;
        const int width=std::min(640,GetStateWidth()-32);
        renderer.SetColor(29,42,41);
        renderer.DrawQuad(x-width/2,y-126,width,259);
        renderer.SetColor(108,135,132);
        renderer.DrawQuad(x-width/2,y-126,width,3);
        renderer.SetColor(255,255,255);
        TextWriter heading(x,y-102,renderer,true,23);
        heading << Text("Confirmar operación",Renderer::ToColor(237,245,239));
        TextWriter question(x,y-50,renderer,true,16);
        question << Text(m_confirm==1?
            "¿Enviar esta carpeta a la papelera?":
            m_confirm==2?"¿Mover esta carpeta dentro de la carpeta elegida?":
            m_confirm==3?"¿Restaurar esta carpeta desde la papelera?":
                         "¿Devolver esta carpeta a la pantalla principal?",
            Renderer::ToColor(238,232,225));
        TextWriter name(x,y-18,renderer,true,15);
        name << Text(m_confirm==3?"La carpeta volverá a su ubicación anterior.":
                         TitleFor(m_confirm_src),
                         Renderer::ToColor(229,238,220));
        if(m_confirm==2){
            TextWriter destination(x,y+7,renderer,true,14);
            destination << Text("Destino: "+TitleFor(m_confirm_target),
                         Renderer::ToColor(192,222,216));
        }
        for(int i=0;i<2;++i){
            const int bx=i==0?x-206:x+30;
            renderer.SetColor(73,94,92);
            renderer.DrawQuad(bx,y+65,176,44);
            renderer.SetColor(255,255,255);
            TextWriter button(bx+88,y+76,renderer,true,18);
            button << Text(i==0?"Confirmar":"Cancelar",
                           Renderer::ToColor(236,239,237));
        }
        TextWriter help(x,y+117,renderer,true,12);
        help << Text("Intro: confirmar | Esc: cancelar",
                     Renderer::ToColor(190,210,204));
    }
    if(m_search_results_open){
        const int width=std::min(780,GetStateWidth()-30);
        const int height=std::min(550,GetStateHeight()-36);
        const int left=(GetStateWidth()-width)/2;
        const int top=(GetStateHeight()-height)/2;
        const int per=std::max(1,(height-155)/43);
        const int pages=std::max(1,(static_cast<int>(m_search_hits.size())+
                                    per-1)/per);
        renderer.SetColor(25,38,40);
        renderer.DrawQuad(left,top,width,height);
        renderer.SetColor(113,147,146);
        renderer.DrawQuad(left,top,width,3);
        renderer.SetColor(255,255,255);
        TextWriter title(left+17,top+18,renderer,false,21);
        title << Text("Resultados de búsqueda",
                      Renderer::ToColor(242,246,244));
        renderer.SetColor(76,97,96);
        renderer.DrawQuad(left+width-99,top+13,82,30);
        renderer.SetColor(255,255,255);
        TextWriter close(left+width-58,top+20,renderer,true,14);
        close << Text("Cerrar",Renderer::ToColor(244,244,244));
        TextWriter summary(left+19,top+48,renderer,false,13);
        summary << Text(std::to_string(m_search_hits.size())+
                        " resultados para: "+m_search_query,
                        Renderer::ToColor(190,210,207));
        for(int i=0;i<per;++i){
            const int index=m_search_page*per+i;
            if(index>=static_cast<int>(m_search_hits.size()))break;
            const MidiSearchHit& hit=m_search_hits[index];
            const int y=top+69+i*43;
            renderer.SetColor(i%2?52:60,i%2?65:73,i%2?67:75);
            renderer.DrawQuad(left+16,y,width-32,40);
            renderer.SetColor(255,255,255);
            std::string name=hit.name;
            if(g_utf8_strlen(name.c_str(),-1)>(width-62)/9){
                const int n=(width-62)/9;
                gchar* head=g_utf8_substring(name.c_str(),0,std::max(1,n-3));
                name=std::string(head)+"...";g_free(head);
            }
            TextWriter line(left+27,y+3,renderer,false,16);
            line << Text(name,Renderer::ToColor(240,247,241));
            std::string route=hit.visibleRoute;
            const int chars=std::max(6,(width-70)/8);
            if(g_utf8_validate(route.c_str(),-1,nullptr) &&
               g_utf8_strlen(route.c_str(),-1)>chars){
                const int len=g_utf8_strlen(route.c_str(),-1);
                gchar* suffix=g_utf8_substring(route.c_str(),
                                               len-(chars-3),len);
                route="..."+std::string(suffix);g_free(suffix);
            }
            TextWriter path(left+27,y+23,renderer,false,12);
            path << Text("Ruta: "+route,Renderer::ToColor(196,214,217));
        }
        if(m_search_hits.empty()){
            TextWriter none(GetStateWidth()/2,top+94,renderer,true,16);
            none << Text("No se encontraron archivos MIDI.",
                         Renderer::ToColor(237,233,226));
        }
        if(pages>1){
            renderer.SetColor(75,94,92);
            renderer.DrawQuad(left+width-226,top+height-52,82,34);
            renderer.DrawQuad(left+width-115,top+height-52,82,34);
            renderer.SetColor(255,255,255);
            TextWriter prev(left+width-185,top+height-44,renderer,true,14);
            prev << Text("Anterior",Renderer::ToColor(236,239,238));
            TextWriter next(left+width-74,top+height-44,renderer,true,14);
            next << Text("Siguiente",Renderer::ToColor(236,239,238));
            TextWriter count(left+30,top+height-43,renderer,false,13);
            count << Text("Página "+std::to_string(m_search_page+1)+
                            " de "+std::to_string(pages),
                            Renderer::ToColor(209,222,217));
        }
    }
    if(m_recent_options_open){
        const int w=std::min(660,GetStateWidth()-30);
        const int h=430;
        const int x=(GetStateWidth()-w)/2;
        const int y=(GetStateHeight()-h)/2;
        const MouseInfo pointer=Mouse();
        renderer.SetColor(29,44,45);
        renderer.DrawQuad(x,y,w,h);
        renderer.SetColor(104,147,140);
        renderer.DrawQuad(x,y,w,3);
        renderer.SetColor(255,255,255);
        TextWriter heading(x+w/2,y+20,renderer,true,24);
        heading << Text("Configurar Recientes",
                        Renderer::ToColor(242,247,245));
        TextWriter hint(x+w/2,y+57,renderer,true,14);
        hint << Text("Selecciona un valor o utiliza los ajustes laterales.",
                     Renderer::ToColor(187,213,208));
        const char* orders[]={"Alfabético","Última reproducción",
                              "Número de reproducciones","Puntos conseguidos"};
        for(int i=0;i<2;++i){
            const int yy=y+(i==0?100:176);
            const bool amount=i==0;
            renderer.SetColor(58,77,78);
            renderer.DrawQuad(x+35,yy,w-70,42);
            for(int side=0;side<2;++side){
                const int sx=side==0?x+45:x+w-93;
                const bool hover=pointerInside(pointer.x,pointer.y,
                                               sx,yy,48,42);
                renderer.SetColor(hover?83:70,hover?144:114,hover?135:108);
                renderer.DrawQuad(sx,yy,48,42);
                renderer.SetColor(255,255,255);
                TextWriter sign(sx+24,yy+9,renderer,true,20);
                sign << Text(side==0?"−":"+",Renderer::ToColor(245,249,247));
            }
            const bool centerHover=pointerInside(pointer.x,pointer.y,
                                                  x+100,yy,w-200,42);
            renderer.SetColor(centerHover?80:53,centerHover?112:76,
                              centerHover?110:76);
            renderer.DrawQuad(x+100,yy,w-200,42);
            renderer.SetColor(255,255,255);
            TextWriter value(x+w/2-7,yy+8,renderer,true,17);
            value << Text(amount?
                     ("Cantidad: "+std::to_string(m_recent_limit_draft)):
                     orders[std::max(0,std::min(m_recent_sort_draft,3))],
                     Renderer::ToColor(243,248,245));
            TextWriter arrow(x+w-119,yy+10,renderer,true,16);
            arrow << Text("v",Renderer::ToColor(196,226,217));
        }
        TextWriter info(x+w/2,y+287,renderer,true,13);
        info << Text("Cantidad: 10–200 en pasos de 10; laterales: ±1.",
                     Renderer::ToColor(181,207,201));
        renderer.SetColor(73,107,102);
        renderer.DrawQuad(x+55,y+365,205,42);
        renderer.DrawQuad(x+w-259,y+365,205,42);
        renderer.SetColor(255,255,255);
        TextWriter save(x+157,y+377,renderer,true,17);
        save << Text("Guardar",Renderer::ToColor(245,248,246));
        TextWriter cancel(x+w-157,y+377,renderer,true,17);
        cancel << Text("Cancelar",Renderer::ToColor(245,248,246));
        if(m_recent_dropdown){
            const int dropdown=m_recent_dropdown;
            const int top=y+(dropdown==1?140:216);
            const int count=dropdown==1?20:4;
            const int cols=dropdown==1?4:1;
            const int cellW=(w-200)/cols;
            const int cellH=dropdown==1?34:32;
            const int rows=(count+cols-1)/cols;
            renderer.SetColor(23,39,40);
            renderer.DrawQuad(x+97,top-3,cellW*cols+6,
                              rows*cellH+6);
            for(int i=0;i<count;++i){
                const int xx=x+100+(i%cols)*cellW;
                const int yy=top+(i/cols)*cellH;
                const bool chosen=dropdown==1?
                       m_recent_limit_draft==(i+1)*10:
                       m_recent_sort_draft==i;
                const bool hover=pointerInside(pointer.x,pointer.y,
                                               xx,yy,cellW,cellH);
                renderer.SetColor(chosen?72:(hover?86:54),
                                  chosen?133:(hover?112:76),
                                  chosen?125:(hover?107:76));
                renderer.DrawQuad(xx,yy,cellW-1,cellH-1);
                renderer.SetColor(255,255,255);
                TextWriter item(xx+cellW/2,yy+6,renderer,true,16);
                item << Text(dropdown==1?
                      std::to_string((i+1)*10):orders[i],
                      Renderer::ToColor(243,249,246));
            }
        }
    }
    if(m_editing_name){
        const int width=std::min(600,GetStateWidth()-32);
        const int x=(GetStateWidth()-width)/2;
        const int y=(GetStateHeight()-190)/2;
        renderer.SetColor(31,43,43);
        renderer.DrawQuad(x,y,width,190);
        renderer.SetColor(104,126,124);
        renderer.DrawQuad(x,y,width,2);
        renderer.DrawQuad(x,y+188,width,2);
        renderer.SetColor(255,255,255);
        TextWriter heading(GetStateWidth()/2,y+17,renderer,true,24);
        heading << Text(m_renaming_folder?"Renombrar carpeta":"Crear carpeta",
                        Renderer::ToColor(238,241,239));
        renderer.SetColor(70,81,81);
        renderer.DrawQuad(x+20,y+72,width-40,50);
        renderer.SetColor(255,255,255);
        TextWriter name(x+30,y+83,renderer,false,21);
        std::string shown=m_folder_name;
        if(g_utf8_validate(shown.c_str(),shown.size(),nullptr) &&
           g_utf8_strlen(shown.c_str(),-1)>36){
            gchar* last=g_utf8_offset_to_pointer(shown.c_str(),
                           g_utf8_strlen(shown.c_str(),-1)-33);
            shown="..."+std::string(last);
        }
        name << Text(shown+"|",Renderer::ToColor(240,245,244));
        TextWriter hint(GetStateWidth()/2,y+145,renderer,true,13);
        hint << Text(m_renaming_folder?
                     "Edita el nombre | Intro: guardar | Esc: cancelar":
                     "Escribe el nombre | Intro: crear | Esc: cancelar",
                     Renderer::ToColor(192,208,204));
    }
    if(m_meta_editing){
       const int width=std::min(720,GetStateWidth()-40);
       const int left=(GetStateWidth()-width)/2;
       const int top=(GetStateHeight()-490)/2;
       renderer.SetColor(17,35,48);
       renderer.DrawQuad(left,top,width,490);
       renderer.SetColor(53,141,167);
       renderer.DrawQuad(left,top,width,3);
       renderer.SetColor(255,255,255);
       TextWriter title(left+28,top+14,renderer,false,25);
       title<<Text("METADATOS DE LA CANCIÓN",
                   Renderer::ToColor(233,245,253));
       const char* names[]={"Título","Subtítulo","Compositor",
         "Arreglista","Copyright","Licencia","Etiquetas",
         "Dificultad (0–5)","Valoración (0–5)"};
       const std::string values[]={m_meta_entry.title,
         m_meta_entry.subtitle,m_meta_entry.composer,
         m_meta_entry.arranger,m_meta_entry.copyright,
         m_meta_entry.license,m_meta_entry.tags,
         std::to_string(m_meta_entry.difficulty),
         std::to_string(m_meta_entry.rating)};
       for(int i=0;i<9;++i){
         const int yy=top+58+i*36;
         renderer.SetColor(i==m_meta_field?48:28,
                           i==m_meta_field?94:52,
                           i==m_meta_field?111:69);
         renderer.DrawQuad(left+20,yy,width-40,33);
         renderer.SetColor(255,255,255);
         TextWriter field(left+33,yy+8,renderer,false,14);
         field<<Text(names[i],Renderer::ToColor(191,227,237));
         TextWriter value(left+width/2,yy+8,renderer,false,14);
         const std::string display=values[i].size()>40?
                                      values[i].substr(0,40)+"…":values[i];
         value<<Text(display+(i==m_meta_field&&i<7?"|":""),
                      Renderer::ToColor(244,251,254));
         if(i>=7){
           TextWriter edit(left+width-76,yy+7,renderer,false,15);
           edit<<Text("−  +",Renderer::ToColor(238,219,160));
         }
       }
       renderer.SetColor(36,114,132);
       renderer.DrawQuad(left+30,top+430,230,42);
       renderer.DrawQuad(left+width-260,top+430,230,42);
       renderer.SetColor(255,255,255);
       TextWriter save(left+68,top+441,renderer,false,16);
       save<<Text("Guardar",Renderer::ToColor(246,250,252));
       TextWriter cancel(left+width-218,top+441,renderer,false,16);
       cancel<<Text("Cancelar",Renderer::ToColor(246,250,252));
    }
}
