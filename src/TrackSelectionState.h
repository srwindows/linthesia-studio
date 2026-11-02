// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#ifndef __TRACKSELECTION_STATE_H
#define __TRACKSELECTION_STATE_H

#include "SharedState.h"
#include "GameState.h"
#include "TrackTile.h"
#include "FluidEngine.h"
#include "MusicalReasoner.h"
#include "libmidi/MidiTypes.h"
#include "MidiComm.h"

#include <vector>

class TrackSelectionState : public GameState {
public:

  TrackSelectionState(const SharedState &state);

protected:
  virtual void Init();
  virtual void Update();
  virtual void Resize();
  virtual void Draw(Renderer &renderer) const;

private:
  void PlayTrackPreview(microseconds_t additional_time);
  std::vector<Track::Properties> BuildTrackProperties() const;
  void PersistCurrent();
  // Set the hand-practice mode before entering the player.
  // The player already reads AdvancedSettingsIO::Load() on Init().
  ButtonState HandChoiceBounds(int button) const;
  // Reuse ButtonState::Update() for the same hover/held/released
  // mechanics as existing Linthesia menu and track buttons.
  ButtonState m_hand_buttons[3];
  ButtonState m_detect_button;
  ButtonState m_modal_buttons[11];
  void ChoosePracticeHand(int choice);
  int m_practice_hand=0; // 0 ambas, 1 derecha, 2 izquierda
  std::string m_hand_status;
  Uint32 m_hand_status_until=0;
  ButtonState AutoDetectHandsBounds() const;
  // Modal configuration: opening never runs detection or changes settings.
  void OpenHandsDetector();
  void DetectHandsFromSelectedTracks(); // preview only, non-destructive
  void ApplyHandsDetection(); // explicit save
  void HandleDetectorModalClick(int id);
  ButtonState DetectorModalBounds() const;
  ButtonState DetectorModalControl(int id) const;
  bool m_detector_open=false;
  bool m_detector_source_piano=true;
  bool m_detector_automatic=true;
  bool m_detector_preview_ready=false;
  int m_detector_split=60;
  int m_detector_preview_split=60;
  size_t m_detector_left=0,m_detector_right=0;
  std::string m_detector_feedback;
  std::vector<MusicalReasoner::Suggestion> m_review_proposals;
  std::vector<bool> m_review_checked;
  int m_review_page=0;

  std::string m_detected_hands_summary;
  std::vector<FluidEngine::Preset> m_soundfont_presets;
  int m_instrument_tile=-1,m_instrument_page=0;
  std::string m_instrument_status;
  void OpenInstrumentPicker(int tile);
  bool UpdateInstrumentPicker(const MouseInfo& mouse);
  void DrawInstrumentPicker(Renderer& renderer) const;
  std::string InstrumentPresetName(int bank,int program) const;
  ButtonState ColorPaletteBounds() const;
  int ColorPaletteHit(int x,int y) const;

  int m_page_count;
  int m_current_page;
  int m_tiles_per_page;

  bool m_preview_on;
  bool m_first_update_after_seek;
  size_t m_preview_track_id;

  ButtonState m_record_video_button;
  ButtonState m_continue_button;
  ButtonState m_back_button;

  ButtonState m_apply_all_not_played;
  ButtonState m_apply_all_played_auto;
  ButtonState m_apply_all_you_play;
  ButtonState m_apply_all_you_play_silent;
  ButtonState m_apply_all_learning;
  ButtonState m_apply_all_learning_silent;
  ButtonState m_apply_all_played_hidden;

  // Retroalimentación visual de los dos modos silenciosos (sin tachado).
  Uint32 m_you_play_silent_pulse=0;
  Uint32 m_learning_silent_pulse=0;

  std::string m_tooltip;

  std::vector<TrackTile> m_track_tiles;
  // Índice visible de la pista cuyo color se está seleccionando; -1 cerrada.
  int m_color_palette_tile=-1;

  std::string m_last_persisted;
  SharedState m_state;
};

#endif // __TRACKSELECTION_STATE_H
