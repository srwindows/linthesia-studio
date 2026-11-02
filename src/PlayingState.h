// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#ifndef __PLAYING_STATE_H
#define __PLAYING_STATE_H

#include <string>
#include <vector>
#include <set>

#include "libmidi/Midi.h"
#include "SharedState.h"
#include "AdvancedSettings.h"
#include "GameState.h"
#include "KeyboardDisplay.h"
#include "HandPractice.h"
#include "StaffEngraving.h"
#include "PianoScoreTracks.h"
#include "FullSongPlayback.h"
#include "SongBookmarks.h"
#include "VideoExporter.h"
#include "MusicalReasoner.h"
#include <deque>
#include "HandCorrectionStore.h"
#include <map>
#include <tuple>
#include "MidiComm.h"

struct ActiveNote {

  bool operator()(const ActiveNote &lhs, const ActiveNote &rhs) const {
    if (lhs.note_id < rhs.note_id)
      return true;

    if (lhs.note_id > rhs.note_id)
      return false;

    if (lhs.channel < rhs.channel)
      return true;

    if (lhs.channel > rhs.channel)
      return false;

    return false;
  }

  NoteId note_id;
  unsigned char channel;
  int velocity;
};

typedef std::set<ActiveNote, ActiveNote> ActiveNoteSet;

class PlayingState : public GameState {
public:
  PlayingState(const SharedState &state,bool promptVideoOnEnter=false);
  ~PlayingState();
  bool ResetKeyboardActive();

protected:
  virtual void Init();
  virtual void Update();
  virtual void Resize();
  virtual void Draw(Renderer &renderer) const;
  void Finish() override;

private:

  std::set<int> m_pressed_notes;
  std::set<int> m_required_notes;
  std::set<int> m_sent_light_notes;
  void UpdateLightGuides();
  void ClearLightGuides();

  void userPressedKey(int note_number, bool active);
  void filePressedKey(int note_number, bool active, size_t track_id);
  bool areAllRequiredKeysPressed();
  bool isKeyPressed(int note_number);
  bool isUserPlayableTrack(size_t track_id);
  bool isNoteInPlayableRange(int note_number);

  int CalcKeyboardHeight() const;
  void SetupNoteState();
  HandPractice::Classifier m_hand_classifier;
  std::set<size_t> m_score_tracks;
  void RefreshScoreTracks();
  bool m_full_song_audio=false;
  std::vector<Track::Properties> m_saved_practice_track_modes;
  void ToggleFullSongAudio();
  void RestorePracticeTrackModes();
  FullSongPlayback::Result AudioTrackCount() const;
  std::map<std::tuple<int,int,microseconds_t>,std::string> m_xml_marks;
  void LoadMusicXmlMarks();
  std::string MarkForNote(const TranslatedNote& note) const;
  struct StaffRenderData {
    StaffEngraving::Layout layout;
    std::vector<const TranslatedNote*> source;
  };
  struct StaffViewport {
    int left=0,top=0,width=0,height=0,lineStart=0,staffEnd=0,step=5;
    int bottom[2]={0,0};
    microseconds_t horizon=2500000;
    bool expanded=false;
    int systems=1,systemDelta=0;
    int SystemAt(microseconds_t time,microseconds_t begin) const {
      if(systems<=1||time<=begin)return 0;
      const auto span=std::max<microseconds_t>(1,horizon/systems);
      return std::min(systems-1,int((time-begin)/span));
    }
    int XAt(microseconds_t time,microseconds_t begin) const {
      const int sys=SystemAt(time,begin);
      const auto span=std::max<microseconds_t>(1,horizon/systems);
      const auto phase=std::max<microseconds_t>(0,time-begin-sys*span);
      const auto dx=std::min<microseconds_t>(span,phase);
      return lineStart+16+int(double(dx)/span*(staffEnd-lineStart-33));
    }
    int Bottom(int part,int sys) const {
      return bottom[part]+sys*systemDelta;
    }
  };
  StaffViewport GetStaffViewport() const;
  StaffRenderData PrepareStaffRender(microseconds_t now,
                                     microseconds_t horizon) const;
  bool HandleStaffHandEditor(const MouseInfo& mouse);
  bool SaveHandAnnotations();
  std::vector<HandPractice::Correction> m_bulk_undo;
  bool m_bulk_undo_available=false;
  microseconds_t m_staff_page_start=-1; // -1: follow playback; other: fixed page
  std::string m_hand_edit_feedback;
  void RebuildHandClassification();
  void DrawGrandStaff(Renderer& renderer) const;
  bool PracticeThisEvent(size_t track,int pitch,microseconds_t when) const;

  void ResetSong();
  void SeekTo(microseconds_t target);
  bool HandleTransport(const MouseInfo &mouse,bool &changed);
  void DrawTransport(Renderer &renderer) const;
  // Exportación del escenario 3D/2D sin incluir el escritorio ni la barra.
  bool m_video_prompt_on_enter=false;
  bool m_video_dialog=false;
  bool m_video_restore_paused=false;
  bool m_video_previous_paused=false;
  bool m_video_previous_loop=false;
  bool m_video_audio=true;
  int m_video_sink_index=0;
  std::vector<std::string> m_video_sinks;
  int m_video_format=0,m_video_resolution=1,m_video_fps=1;
  int m_video_dropdown=0;
  microseconds_t m_video_previous_position=0;
  std::vector<Track::Properties> m_video_previous_modes;
  std::string m_video_path,m_video_status;
  mutable VideoExporter m_video;
  void OpenVideoDialog();
  void CloseVideoDialog();
  void ChooseVideoFile();
  void ChooseVideoFolder();
  bool HandleVideoDialog(const MouseInfo& mouse);
  void DrawVideoDialog(Renderer& renderer) const;
  void StartVideoExport();
  void StopVideoExport(bool cancel);
  void CaptureVideoFrame() const;

  void RefreshChord(bool immediate=false);
  std::string m_current_chord="—";
  std::string m_pending_chord="—";
  unsigned long m_chord_pending_since=0;
  unsigned long m_chord_last_refresh=0;
  void Play(microseconds_t delta_microseconds);
  // Programas elegidos por pista: solo salida FluidSynth interna, nunca
  // mensajes adicionales al Roland FANTOM u otros equipos externos.
  int m_program_ch[16]={};
  int m_bank_ch[16]={};
  std::map<std::tuple<size_t,int,microseconds_t>,int> m_review_pitch_changes;
  std::map<std::tuple<size_t,int,int>,std::deque<int>> m_review_active;
  std::vector<size_t> m_review_event_cursors;
  void RebuildMusicalEventCursors(microseconds_t time);
  void ApplyApprovedMusicalCorrections();
  void ResetInstrumentCache();
  void ApplyTrackInstrument(size_t track,int channel);


  bool m_scrubbing=false;
  double m_scrub_fraction=0.0;
  bool m_transport_mouse_press=false;
  bool m_show_help=false;
  // A-B Loop. Los tiempos reales se guardan en microsegundos MIDI,
  // no en píxeles ni en segundos del reloj externo.
  std::vector<SongBookmarks::Mark> m_song_bookmarks;
  std::string m_bookmark_feedback;
  bool m_loop_active=false;
  int m_loop_error_baseline=0;
  bool m_loop_errors_monitoring=false;
  bool m_loop_defined=false;
  bool m_loop_editor=false;
  bool m_loop_restore_paused=false;
  bool m_loop_dragging=false;
  bool m_loop_draft_valid=false;
  bool m_loop_prompt=false;
  microseconds_t m_loop_start=0;
  microseconds_t m_loop_end=0;
  double m_loop_draft_start=0.0;
  double m_loop_draft_end=0.0;
  void OpenLoopEditor();
  void CloseLoopEditor(bool apply);
  void DrawLoopEditor(Renderer& renderer) const;
  bool HandleLoopEditor(const MouseInfo& mouse);
  double LoopFraction(int mouseX) const;

  // Editor del reproductor: seis pestañas y preferencias persistentes.
  bool m_settings_open=false;
  bool m_settings_paused_before=false;
  int m_settings_x=-1,m_settings_y=-1;
  bool m_settings_dragging=false;
  int m_settings_drag_dx=0,m_settings_drag_dy=0;
  int m_settings_tab=0;
  int m_settings_page=0;
  std::string m_settings_feedback;
  AdvancedSettings m_settings_before;
  bool m_settings_view_before=false;
  double m_settings_tempo_before=1.0;
  bool m_settings_loop_before=false;
  void OpenPlayerSettings();
  void SavePlayerSettings();
  void CancelPlayerSettings();
  void ResetPlayerSettingsPage();
  bool HandlePlayerSettings(const MouseInfo& mouse);
  void DrawPlayerSettings(Renderer& renderer) const;
  void ClosePlayerSettings();
  bool m_orbit_dragging=false;
  int m_orbit_last_x=0,m_orbit_last_y=0;
  bool m_view_3d=false;
  double m_tempo_factor=1.0;
  int m_missed_notes=0;
  int EffectiveTempoBPM() const;
  void AdjustTempoBPM(int delta);
  void ResetTempoBPM();
  void Listen();

  double CalculateScoreMultiplier() const;

  bool m_paused;

  int MinPlayableNote = 0;
  int MaxPlayableNote = 120;

  KeyboardDisplay *m_keyboard;
  microseconds_t m_show_duration;
  TranslatedNoteSet m_notes;
  TranslatedNoteSet m_notes_history;

  bool m_any_you_play_tracks;
  bool m_any_learning_track;
  size_t m_look_ahead_you_play_note_count;

  ActiveNoteSet m_active_notes;

  bool m_first_update;
  bool m_practice_saved=false;
  void SavePracticeAttempt(bool completed);


  SharedState m_state;
  AdvancedSettings m_midi_preferences;
  int m_current_combo;

  double m_title_alpha;
  double m_max_allowed_title_alpha;

  // For octave sliding
  int m_note_offset;

  // For retries
  bool m_should_retry;
  bool m_should_wait_after_retry;
  microseconds_t m_retry_start;

  void eraseUntilTime(microseconds_t time);

  NoteState findNodeState(const TranslatedNote& note, TranslatedNoteSet& notes, NoteState default_note_state);
};

#endif // __PLAYING_STATE_H
