// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#ifndef __TITLE_STATE_H
#define __TITLE_STATE_H

#include "SharedState.h"
#include "GameState.h"
#include "MenuLayout.h"
#include "libmidi/MidiTypes.h"
#include "DeviceTile.h"
#include "StringTile.h"
#include "EnumTile.h"
#include "AdvancedSettings.h"
#include <array>
#include <vector>
#include <glib.h>


// class Midi;
// class MidiCommOut;
class Tga;

class TitleState : public GameState {
public:

  // You can pass 0 in for state.midi_out to have the title
  // screen pick a device for you.
  TitleState(const SharedState &state) :
    m_state(state),
    m_output_tile(0),
    m_input_tile(0),
    m_file_tile(0),
    m_keyboard_size_tile(nullptr),
    m_skip_next_mouse_up(false) {
  }

  ~TitleState();

protected:
  virtual void Init();
  virtual void Update();
  virtual void Resize();
  virtual void Draw(Renderer &renderer) const;
  virtual bool WantsTextInput() const {return m_editing_path!=0;}
  virtual void OnTextInput(const std::string& text);

private:
  void PlayDevicePreview(microseconds_t delta_microseconds);
  // Five editable home cards, with a bounded in-window option popup.
  // A selection only changes persisted settings after successful validation.
  struct HomeChoice {std::string label,value;};
  struct HomeCard {int x,y,w,h;};
  HomeCard HomeCardAt(int index) const;
  std::vector<HomeChoice> HomeOptions(int index) const;
  void OpenHomeOptions(int index);
  void DrawHomeOptions(Renderer& renderer) const;
  bool UpdateHomeOptions(const MouseInfo& mouse);
  void SelectHomeOption(int card,int option);
  void DrawHomeCards(Renderer& renderer) const;
  bool LoadRecentSong(const std::string& path);
  int m_home_open=-1,m_home_page=0;
  std::vector<HomeChoice> m_home_choices;
  std::string m_home_status;

  KeyboardSize m_keyboard_size;

  ButtonState m_continue_button;
  void ArrangeFooter();
  void ArrangeSettings();
  void SaveKeyboardSize();
  void OpenAdvancedSettings();
  void CloseAdvancedSettings();
  void DrawAdvancedSettings(Renderer& renderer) const;
  void UpdateAdvancedSettings(const MouseInfo& mouse);
  bool ApplyAdvancedSettings(bool close);
  void ResetSettingsSection();
  void ModifySetting(int id,int direction);
  std::vector<std::string> SettingsOptionsFor(int id) const;
  void SelectSettingsChoice(int id,int index);
  void BrowseSettingsPath(int id);
  void PollSettingsFileDialog();
  void CancelSettingsFileDialog();
  void EnumerateSoundfonts();
  struct SettingsRow{
    std::string name,value,subtitle;
    int id=0;
    bool readOnly=false;
  };
  std::vector<SettingsRow> SettingsRows() const;
  void RefreshDeviceChoices();
  std::string SettingsDeviceName(bool input)const;
  void PlayAudioTest();

  ButtonState m_songs_button;
  ButtonState m_free_button;
  ButtonState m_settings_button;
  ButtonState m_rate_left, m_rate_right;
  ButtonState m_keys_left, m_keys_right;
  bool m_settings_open = false;
  int m_refresh_rate = 300;
  AdvancedSettings m_saved_settings,m_draft_settings;
  KeyboardSize m_draft_keyboard;
  int m_section=0,m_settings_page=0,m_editing_path=0;
  int m_choice_open_id=0,m_choice_page=0;
  bool m_settings_dirty=false,m_test_note=false,m_reset_pending=false;
  std::string m_path_before_edit;
  bool m_path_paste_latch=false;
  GPid m_file_dialog_pid=0;
  int m_file_dialog_fd=-1,m_file_dialog_id=0;
  std::string m_file_dialog_output;
  unsigned int m_test_note_until=0;
  std::string m_settings_status;
  mutable std::array<Tga*,12> m_scene_previews{};
  mutable std::array<bool,12> m_scene_previews_failed{};
  std::vector<std::string> m_soundfonts,m_audio_devices;
  MidiCommDescriptionList m_input_choices,m_output_choices;
  int m_draft_in_index=-1,m_draft_out_index=-1,m_audio_device_index=0;

  ButtonState m_back_button;

  SharedState m_state;

  std::string m_last_input_note_name;
  std::string m_tooltip;

  DeviceTile *m_output_tile;
  DeviceTile *m_input_tile;

  EnumTile *m_keyboard_size_tile;
  StringTile *m_file_tile;

  bool m_skip_next_mouse_up;
};

#endif // __TITLE_STATE_H
