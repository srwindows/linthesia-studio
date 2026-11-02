// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_FREEPLAY_STATE_H
#define LINTHESIA_FREEPLAY_STATE_H
#include "FreePlayRecorder.h"
#include "FreePlayVisualNotes.h"
#include "SharedState.h"
#include "GameState.h"
#include "MenuLayout.h"
#include <array>
#include <string>
class FreePlayState : public GameState {
public:
 explicit FreePlayState(const SharedState& state):m_state(state){}
protected:
 void Init() override;
 void Resize() override;
 void Update() override;
 void Draw(Renderer& renderer) const override;
 void Finish() override;
private:
 SharedState m_state;
 FreePlay::Recorder m_recorder;
 ButtonState m_back,m_record,m_stop,m_play,m_save,m_tracks_button,m_quant,m_minus,m_plus;
 std::array<ButtonState,FreePlay::Recorder::MaxTracks> m_mutes,m_programs,m_deletes;
 std::array<size_t,FreePlay::Recorder::MaxTracks> m_play_cursors{};
 std::array<int,128> m_pressed{};
 FreePlay::VisualNotes m_visual;
 uint64_t m_visual_origin=0,m_visual_now=0;
 bool m_show_tracks=false;
 int KeyboardTop()const;
 int KeyboardLeft()const;
 int KeyboardWidth()const;
 int KeyX(int pitch)const;
 int KeyWidth(int pitch)const;
 int m_mouse_note=-1;
 int KeyboardHit(int x,int y) const;
 void KeyboardNote(int note,bool on);
 uint64_t m_clock_start=0,m_current=0;
 bool m_playing=false;
 std::string m_notice;
 std::string SessionPath() const;
 void SaveSession();
 void StartPlayback();
 void StopPlayback();
 void StartRecording();
 void StopRecording();
 void RecordInput();
 void AdvancePlayback();
 void ApplyPrograms();
};
#endif
