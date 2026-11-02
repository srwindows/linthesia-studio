// -*- mode: c++; coding: utf-8 -*-

// Linthesia

#ifndef __SONGLIB_STATE_H
#define __SONGLIB_STATE_H

#include "SharedState.h"
#include "GameState.h"
#include "SongTile.h"
#include "PersonalFolders.h"
#include "FolderPlacements.h"
#include "MidiSearchIndex.h"
#include "MidiImporter.h"
#include <glib.h>
#include "CompatibleSystem.h"
#include "FileSelector.h"
#include "MenuLayout.h"
#include "LibraryLayout.h"
#include <sqlite3.h>

#include <vector>
#include <set>
#include "AdvancedSettings.h"
#include "SongMetadata.h"
#include <string>
#include <memory>
#include <atomic>
#include <thread>

const int ColumnMargin = Layout::ScreenMarginX * 2;
const int PagesButtonsY = 
    Layout::ScreenMarginY + Layout::ScreenMarginY / 2 + 
    SongTileHeight / 2 - Layout::ButtonHeight/2;

class SongLibState : public GameState {

public:
    SongLibState(const SharedState &state): 
    m_state(state){
    }
    virtual ~SongLibState();

protected:
    virtual void Init();
    virtual void Update();
    virtual void Resize();
    virtual void Draw(Renderer &renderer) const;
    virtual bool WantsTextInput() const {
       return m_editing_name||m_search_editing||m_meta_editing;
    }
    virtual void OnTextInput(const std::string& text);

private:

    void UpdateSongTiles();
    void OpenTitleState(string path);
    int MidiAt(int x,int y) const;
    void SelectMidi(int index,bool toggle,bool range);
    void OpenMidiMenu(int x,int y,int index);
    void BuildMidiDestinations();
    void ExecuteMidiAction();
    bool SafeMidiSource(const std::string& path) const;
    void UpdateFolderDragHover(int x,int y);
    void NavigateDuringFolderDrag(const std::string& target);
    void ExecuteFolderBatch();
    struct EditAction {
        enum class Kind { MidiMove, FolderLinks, FolderOrder, PhysicalFolder, FolderRename, FolderAlias };
        Kind kind=Kind::MidiMove;
        std::string title,folder,source,target,beforeName,afterName;
        std::vector<std::pair<std::string,std::string>> files;
        FolderPlacements::PlacementSnapshot beforeLinks,afterLinks;
        std::vector<std::string> beforeOrder,afterOrder;
    };
    void RecordEdit(const EditAction& edit);
    void ApplyEdit(bool undo);
    bool MoveMidiPairs(const std::vector<std::pair<std::string,std::string>>& pairs,
                       bool undo,std::string& error);
    void ClearEditHistory();
    void ClearSelections();
    void NavigateDuringMidiDrag(const std::string& path);
    void UpdateMidiDragHover(int x,int y);
    void UpdateDragPagination(int x,int y);
    void UpdateSongTilesPage();

    void GoUpDirectory();
    void RunMidiSearch();
    void StartMidiImport();
    void PollMidiImport();
    void CancelMidiImport();
    bool FixedFolder(const std::string& path)const;
    std::string RecentFolderPath() const;
    void OpenRecentOptions();
    bool RecentGearHit(int x,int y) const;
    void SelectSearchHit(const MidiSearchHit& hit);
    void StartCreateFolder();
    void FinishCreateFolder(bool confirm);
    void StartRenameFolder();
    bool RenameFolder(const std::string& source,const std::string& name,
                      std::string& result,std::string& error);
    void RenameOrderEntry(const std::string& parent,const std::string& oldName,
                          const std::string& newName);
    bool CanCreateHere() const;
    std::string VisibleFolderPath() const;
    int TileAt(int x,int y) const;
    void OpenContext(int x,int y,int index);
    void OpenConfirmation(int action,const std::string& src,
                          const std::string& target);
    void ExecuteConfirmation();
    void SwapPositions(const std::string& src,const std::string& target);
    void ApplyDropChoice(bool moveInside);
    void CompleteDrag(int x,int y);
    std::string TitleFor(const std::string& path) const;
    std::string NameFor(const std::string& path) const;

    int ContentLeft();
    int ContentRight();

    int m_page_count;
    int m_current_page;
    int m_columns;
    int m_tiles_per_page;
    int m_file_page=0;
    int m_file_page_count=1;
    int m_file_rows_per_page=1;
    int m_file_list_heading=0;
    int m_file_list_top=0;
    int m_file_list_left=0;
    int m_file_list_width=0;
    int m_folder_count=0;
    int m_midi_count=0;
    LibraryLayout::MidiGrid m_midi_grid;
    bool m_compact_subfolders=false;
    LibraryLayout::CompactFolderLayout m_compact_geometry;
    ButtonState m_file_prev_button,m_file_next_button;
    ButtonState m_search_field,m_search_button;
    bool m_search_editing=false,m_search_results_open=false;
    std::string m_search_query,m_highlight_path;
    std::set<std::string> m_selected_midis;
    std::string m_selection_anchor,m_last_clicked_midi;
    Uint32 m_last_midi_click=0;
    bool m_select_all_latch=false;
    bool m_midi_drag_candidate=false,m_midi_select_dragging=false;
    bool m_midi_move_candidate=false,m_midi_move_dragging=false;
    std::vector<std::string> m_midi_moving_files;
    std::string m_midi_drag_source_folder;
    std::string m_midi_hover_destination;
    Uint32 m_midi_hover_started=0;
    int m_midi_navigation_steps=0;
    bool m_midi_drag_additive=false;
    int m_midi_drag_x=0,m_midi_drag_y=0;
    int m_midi_drag_last_x=0,m_midi_drag_last_y=0;
    std::set<std::string> m_midi_before_drag;
    bool m_midi_menu=false,m_midi_destinations_open=false;
    int m_midi_menu_x=0,m_midi_menu_y=0,m_midi_destination_page=0;
    std::vector<std::pair<std::string,std::string>> m_midi_destinations;
    int m_midi_confirmation=0; // 1: enviar MIDI a papelera, 2: mover MIDI
    std::vector<std::string> m_midi_action_files;
    std::string m_midi_action_target;
    std::vector<MidiSearchHit> m_search_hits;
    int m_search_page=0;

    ButtonState m_undo_button,m_redo_button,m_import_button;
    GPid m_import_pid=0;
    int m_import_stdout=-1;
    std::string m_import_output;
    struct ImportJob {
        std::atomic<bool> done{false};
        MidiImporter::Result result;
    };
    std::shared_ptr<ImportJob> m_import_job;
    std::vector<EditAction> m_undo_stack,m_redo_stack;
    bool m_history_shortcut_latch=false;
    bool m_had_keyboard_focus=false;
    ButtonState m_back_button;
    ButtonState m_create_folder_button;
    ButtonState m_path_up_button;
    ButtonState m_next_page_button;
    ButtonState m_prev_page_button;

    string m_current_path;
    string m_base_path;
    PersonalFolders m_personal;
    std::unique_ptr<FolderPlacements> m_placements;
    bool m_meta_editing=false;
    int m_meta_field=0;
    std::string m_meta_path;
    SongMetadata::Entry m_meta_entry;
    std::string& MetaTextField();
    void OpenMetaEditor();
    void CloseMetaEditor(bool save);
    bool m_editing_name = false;
    bool m_renaming_folder = false;
    std::string m_rename_path;
    std::string m_rename_original_name;
    string m_folder_name;
    string m_error;
    string m_notice;
    bool m_context_open=false;
    bool m_recent_options_open=false;
    bool m_recent_gear_pressed=false;
    int m_recent_dropdown=0; // 0 cerrado, 1 cantidad, 2 orden
    int m_recent_limit_draft=20,m_recent_sort_draft=1;
    bool m_trash_open=false;
    int m_trash_page=0;
    int m_context_x=0,m_context_y=0;
    std::string m_context_path;
    int m_confirm=0; // 1: trash; 2: nest; 3: restore; 4: return to main
    std::string m_confirm_src,m_confirm_target;
    AdvancedSettings m_preferences;
    std::set<std::string> m_selected_folders;
    std::string m_folder_anchor,m_last_clicked_folder;
    Uint32 m_last_folder_click=0;
    std::vector<std::string> m_folder_drag_files,m_folder_action_files;
    std::string m_folder_drag_origin,m_folder_action_target;
    std::string m_folder_hover_target;
    Uint32 m_folder_hover_started=0;
    int m_folder_drag_navigations=0;
    bool m_folder_batch_confirm=false;
    bool m_folder_rect_candidate=false,m_folder_rect_selecting=false;
    bool m_folder_rect_additive=false;
    int m_folder_rect_x=0,m_folder_rect_y=0;
    int m_folder_rect_last_x=0,m_folder_rect_last_y=0;
    std::set<std::string> m_folder_before_rect;
    std::string m_pressed_path,m_drag_path;
    int m_press_x=0,m_press_y=0;
    bool m_dragging=false;
    bool m_drop_choice=false;
    std::string m_drop_src,m_drop_target;
    Uint32 m_page_hover_start=0;
    int m_page_hover_target=0;

    std::vector<SongTile> m_song_tiles;

    SharedState m_state;
    bool m_skip_next_mouse_up = false;
};

#endif // __SONGLIB_STATE_H
