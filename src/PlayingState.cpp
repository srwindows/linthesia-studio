// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "PlayingState.h"
#include "Effect3DPalette.h"
#include "StageScenes.h"
#include "StageCamera.h"
#include "FloatingSettingsLayout.h"
#include "PracticeError.h"
#include "ChordDetector.h"
#include "StaffNotation.h"
#include "MusicalReviewStore.h"
#include "FluidEngine.h"
#include "RecentSongs.h"
#include "PracticeHistory.h"
#include "LoopErrorPolicy.h"
#include "TrackSelectionState.h"
#include "StatsState.h"
#include "Renderer.h"
#include "Textures.h"
#include "CompatibleSystem.h"

#include <stdio.h>
#include <string>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <glib.h>
#include <unistd.h>
#include <cctype>

#include "StringUtil.h"
#include "MenuLayout.h"
#include "TextWriter.h"
#include "UserSettings.h"

#include "libmidi/Midi.h"
#include "libmidi/MidiTrack.h"
#include "libmidi/MidiEvent.h"
#include "libmidi/MidiUtil.h"

#include "MidiComm.h"
#include "Keyboard.h"

using namespace std;


void PlayingState::SetupNoteState() {

  TranslatedNoteSet old = m_notes;
  m_notes.clear();

  for (TranslatedNoteSet::const_iterator i = old.begin(); i != old.end(); ++i) {
    TranslatedNote n = *i;

    n.state = AutoPlayed;
    n.retry_state = AutoPlayed;
    if (isUserPlayableTrack(n.track_id) && isNoteInPlayableRange(n.note_id)
        && m_hand_classifier.Selected(n,m_midi_preferences.practiceHands))
    {
      n.state = UserPlayable;
      n.retry_state = UserPlayable;
    }
    m_notes.insert(n);
  }
}

bool PlayingState::isNoteInPlayableRange(int note_number) {
  return note_number >= m_state.keyboard.GetMinPlayableNote() && note_number <= m_state.keyboard.GetMaxPlayableNote();
}

void PlayingState::ResetSong() {

  if (m_state.midi_out)
    m_state.midi_out->Reset();

  if (m_state.midi_in)
    m_state.midi_in->Reset();

  // TODO: These should be moved to a configuration file
  // along with ALL other "const static something" variables.
  const static microseconds_t LeadIn = 5500000;
  const static microseconds_t LeadOut = 1000000;

  if (!m_state.midi)
    return;

  m_state.midi->Reset(LeadIn, LeadOut);
  ResetInstrumentCache();

  m_review_pitch_changes.clear();
  for(const auto& e:MusicalReviewStore::Load(m_state.song_path))
    m_review_pitch_changes[{e.track,e.original,e.when}]=e.replacement;
  m_notes = m_state.midi->Notes();
  ApplyApprovedMusicalCorrections();
  RebuildMusicalEventCursors(m_state.midi->GetSongPositionInMicroseconds());
  m_staff_page_start=-1; // New song resumes following its real playback.
  LoadMusicXmlMarks();
  m_notes_history.clear();
  RebuildHandClassification(); // estimate once; reload per-song manual overrides
  SetupNoteState();

  m_state.stats = SongStatistics();
  m_practice_saved=false;
  m_song_bookmarks=SongBookmarks::Load(m_state.song_path);
  m_bookmark_feedback.clear();
  m_loop_error_baseline=0;
  m_loop_errors_monitoring=false;
  m_state.stats.total_note_count = static_cast<int>(m_notes.size());
  m_missed_notes=0;
  ResetTempoBPM();

  m_current_combo = 0;

  m_note_offset = 0;
  m_max_allowed_title_alpha = 1.0;

  m_should_retry = false;
  m_should_wait_after_retry = false;
  m_retry_start = m_state.midi->GetNextBarInMicroseconds(-1000000000);
}

PlayingState::PlayingState(const SharedState &state,bool promptVideoOnEnter) :
  m_paused(false),
  m_keyboard(0),
  m_any_you_play_tracks(false),
  m_any_learning_track(false),
  m_first_update(true),
  m_should_retry(false),
  m_should_wait_after_retry(false),
  m_retry_start(0),
  m_state(state) {
  m_video_prompt_on_enter=promptVideoOnEnter;
}

void PlayingState::Init() {
  m_midi_preferences=AdvancedSettingsIO::Load();
  m_view_3d=m_midi_preferences.player3D;
  // Mantener los ajustes musicales junto a las demás preferencias.

  if (!m_state.midi)
    throw GameStateError("Reproductor: no se ha cargado un MIDI.");

  RefreshScoreTracks();
  m_look_ahead_you_play_note_count = 0;
  for (size_t i = 0; i < m_state.track_properties.size(); ++i) {

    if (m_state.track_properties[i].mode == Track::ModeYouPlay ||
        m_state.track_properties[i].mode == Track::ModeYouPlaySilently ||
        m_state.track_properties[i].mode == Track::ModeLearning ||
        m_state.track_properties[i].mode == Track::ModeLearningSilently) {
      m_look_ahead_you_play_note_count += m_state.midi->Tracks()[i].Notes().size();
      m_any_you_play_tracks = true;
    }

    if (m_state.track_properties[i].mode == Track::ModeLearning ||
        m_state.track_properties[i].mode == Track::ModeLearningSilently)
    {
      m_any_learning_track = true;
    } 
  }

  // Always honor the explicitly selected tracks on entering the player.
  // The user can request a full arrangement with the dedicated button;
  // never silently unmute the accompaniment just because one piano is solo.
  // This is independent of the PCM volume fix in FluidEngine.cpp.

  string min_key = UserSetting::Get(MIN_KEY_KEY, "");
  if (strtol(min_key.c_str(), NULL, 10) > 0) {
    MinPlayableNote = strtol(min_key.c_str(), NULL, 10);
    printf("Set minimal key to %d\n", MinPlayableNote);
  }
  string max_key = UserSetting::Get(MAX_KEY_KEY, "");
  if (strtol(max_key.c_str(), NULL, 10) > 0) {
    MaxPlayableNote = strtol(max_key.c_str(), NULL, 10);
    printf("Set maximal key to %d\n", MaxPlayableNote);
  }

  // This many microseconds of the song will
  // be shown on the screen at once
  const static microseconds_t DefaultShowDurationMicroseconds = 3250000;
  m_show_duration = m_midi_preferences.playerNoteDurationMs*1000LL;

  m_keyboard = new KeyboardDisplay(m_state.keyboard, GetStateWidth() - Layout::ScreenMarginX*2, CalcKeyboardHeight());

  // La barra de progreso requiere puntero visible mientras suena.
  Compatible::ShowMouseCursor();

  ResetSong();
  RecentSongs::RecordPlay(m_state.song_path);
  RecentSongs::RememberLastSelected(m_state.song_path);

  m_state.dpms_thread->pauseScreensaver(true && !m_any_learning_track);
  // The selection screen's recording button enters with the configuration
  // dialog already open. The rendered 3D scene itself has no record button.
  if(m_video_prompt_on_enter)OpenVideoDialog();
}

void PlayingState::SavePracticeAttempt(bool completed){
  if(m_practice_saved)return;
  const auto &stats=m_state.stats;
  const int expected=std::max(stats.notes_user_could_have_played,
                              stats.notes_user_actually_played);
  // No historical record for songs played only by the computer; wait
  // until the learner has produced measurable performance data.
  if(expected<=0 || !m_any_you_play_tracks || m_state.song_path.empty())return;
  PracticeHistory::Attempt a;
  a.hit=std::max(0,std::min(expected,stats.notes_user_actually_played));
  a.expected=expected;
  a.errors=std::max(0,stats.stray_notes);
  a.bpm=EffectiveTempoBPM();
  a.points=std::max(0,static_cast<int>(stats.score));
  a.hands=m_midi_preferences.practiceHands;
  a.complete=completed;
  if(PracticeHistory::Append(m_state.song_path,a))
    m_practice_saved=true;
}

void PlayingState::ClearLightGuides(){
  if(m_sent_light_notes.empty())return;
  if(m_state.midi_out){
    const int channel=std::max(0,std::min(15,
                             m_midi_preferences.midiLightChannel-1));
    for(int pitch:m_sent_light_notes)
      m_state.midi_out->Write(MidiEvent::Build(
           MidiEventSimple(uint8_t(0x80|channel),
                           uint8_t(pitch),uint8_t(0))));
  }
  m_sent_light_notes.clear();
}
void PlayingState::UpdateLightGuides(){
  // A lighting device is manually opted in, by name and MIDI channel.
  // Devices not declaring light semantics will often SOUND these notes,
  // so internal FluidSynth and Roland FANTOM are never used for guides.
  const bool permitted=m_midi_preferences.midiLightGuides &&
     m_state.midi_in && m_state.midi_out && !m_paused &&
     m_state.midi_out->GetDeviceDescription().client>=0 &&
     m_state.midi_out->GetDeviceDescription().name.find("FANTOM")==std::string::npos;
  if(!permitted){ClearLightGuides();return;}
  const int channel=std::max(0,std::min(15,
                           m_midi_preferences.midiLightChannel-1));
  std::set<int> upcoming;
  for(int pitch:m_required_notes)
    if(pitch>=21&&pitch<=108)upcoming.insert(pitch);
  for(int pitch:m_sent_light_notes)
    if(!upcoming.count(pitch))
      m_state.midi_out->Write(MidiEvent::Build(
           MidiEventSimple(uint8_t(0x80|channel),
                           uint8_t(pitch),uint8_t(0))));
  for(int pitch:upcoming)
    if(!m_sent_light_notes.count(pitch))
      m_state.midi_out->Write(MidiEvent::Build(
           MidiEventSimple(uint8_t(0x90|channel),
                           uint8_t(pitch),uint8_t(1))));
  m_sent_light_notes=std::move(upcoming);
}

void PlayingState::Finish() 
{
  if(m_video.Active())StopVideoExport(true);
  ClearLightGuides();
  m_state.dpms_thread->pauseScreensaver(false);
}

PlayingState::~PlayingState() {
  if(m_video.Active())StopVideoExport(true);
  Compatible::ShowMouseCursor();
}

int PlayingState::CalcKeyboardHeight() const {
  // Start with the size of the screen
  int height = GetStateHeight();

  // Pie compacto: maximizar la superficie del piano roll.
  height -= 43+94; // reservar cabecera y escala superior

  return height;
}

void PlayingState::RebuildMusicalEventCursors(microseconds_t time){
  m_review_event_cursors.clear();
  m_review_active.clear();
  if(!m_state.midi)return;
  for(const auto& track:m_state.midi->Tracks()){
    const auto& timestamps=track.EventUsecs();
    m_review_event_cursors.push_back(
      std::upper_bound(timestamps.begin(),timestamps.end(),time)-timestamps.begin());
  }
}
void PlayingState::ApplyApprovedMusicalCorrections(){
  // Correct the rendered note stream and learning prompts, not the source
  // MIDI. External physical instruments must never receive inferred pitches.
  if(!m_state.midi_out || !m_state.midi_out->IsInternalFluid() ||
     m_review_pitch_changes.empty())return;
  TranslatedNoteSet adjusted;
  for(const auto& note:m_notes){
    TranslatedNote fixed=note;
    const auto it=m_review_pitch_changes.find(
        std::make_tuple(note.track_id,int(note.note_id),note.start));
    if(it!=m_review_pitch_changes.end()){
      if(it->second<0)continue; // approved phantom-note deletion
      fixed.note_id=static_cast<NoteId>(it->second);
    }
    adjusted.insert(fixed);
  }
  m_notes.swap(adjusted);
}

void PlayingState::ResetInstrumentCache(){
  for(int i=0;i<16;++i){m_program_ch[i]=-1;m_bank_ch[i]=-1;}
}
void PlayingState::ApplyTrackInstrument(size_t track,int channel){
  if(!m_state.midi_out || !m_state.midi_out->IsInternalFluid()||
     track>=m_state.track_properties.size()||channel<0||channel>=16)
    return;
  const auto& prop=m_state.track_properties[track];
  if(prop.instrument_program<0)return;
  const int bank=prop.instrument_bank;
  const int prog=prop.instrument_program;
  if(m_bank_ch[channel]==bank && m_program_ch[channel]==prog)return;
  if(FluidEngine::SelectPreset(channel,bank,prog)){
    m_bank_ch[channel]=bank;m_program_ch[channel]=prog;
  }
}

void PlayingState::Play(microseconds_t delta_microseconds) {

  // Move notes, time tracking, everything
  // delta_microseconds = 0 means, that we are on pause
  MidiEventListWithTrackId evs = m_state.midi->Update(delta_microseconds);

  // These cycle is for keyboard updates (not falling keys)
  const size_t length = evs.size();
  for(size_t i = 0; i < length; ++i) {

    const size_t &track_id = evs[i].first;
    const MidiEvent &ev = evs[i].second;
    // Associate each event with its original exact timestamp; a rendered
    // frame may contain many simultaneous NoteOns on different tracks.
    microseconds_t eventTime=-1;
    if(track_id<m_review_event_cursors.size() &&
       track_id<m_state.midi->Tracks().size()){
      const auto& times=m_state.midi->Tracks()[track_id].EventUsecs();
      const size_t index=m_review_event_cursors[track_id]++;
      if(index<times.size())eventTime=times[index];
    }
    const bool internal=m_state.midi_out && m_state.midi_out->IsInternalFluid();
    const bool isOn=ev.Type()==MidiEventType_NoteOn && ev.NoteVelocity()>0;
    const bool isOff=ev.Type()==MidiEventType_NoteOff ||
                  (ev.Type()==MidiEventType_NoteOn && ev.NoteVelocity()==0);
    int correctedPitch=(isOn||isOff)?int(ev.NoteNumber()):-2;
    if(internal && !m_review_pitch_changes.empty() && (isOn||isOff)){
      const auto activeKey=std::make_tuple(track_id,int(ev.Channel()),int(ev.NoteNumber()));
      if(isOn){
        const auto fix=m_review_pitch_changes.find(
            std::make_tuple(track_id,int(ev.NoteNumber()),eventTime));
        if(fix!=m_review_pitch_changes.end())correctedPitch=fix->second;
        m_review_active[activeKey].push_back(correctedPitch);
      }else{
        auto it=m_review_active.find(activeKey);
        if(it!=m_review_active.end() && !it->second.empty()){
          correctedPitch=it->second.front();
          it->second.pop_front();
        }
      }
    }

    // Draw refers to the keys lighting up (automatically) -- not necessarily
    // the falling notes.  The KeyboardDisplay object contains its own logic
    // to decide how to draw the falling notes
    bool draw = false;
    bool play = false;
    switch (m_state.track_properties[track_id].mode) {
    case Track::ModeNotPlayed:           draw = false;  play = false;  break;
    case Track::ModePlayedButHidden:     draw = false;  play = true;   break;
    case Track::ModeYouPlay:             draw = false;  play = false;  break;
    case Track::ModeYouPlaySilently:     draw = false;  play = false;  break;
    case Track::ModeLearning:            draw = false;  play = false;   break;
    case Track::ModeLearningSilently:    draw = false;  play = false;  break;
    case Track::ModePlayedAutomatically: draw = true;   play = true;   break;
    case Track::ModeCount: break;
    }

    // For hand-specific practice, the OTHER hand plays automatically
    // as accompaniment, even when both hands share a single MIDI track.
    // Only note events are redirected; controller/program messages retain
    // their existing per-track semantics.
    if(m_midi_preferences.practiceHands!=HandPractice::Both &&
       isUserPlayableTrack(track_id) &&
       (ev.Type()==MidiEventType_NoteOn ||
        ev.Type()==MidiEventType_NoteOff) &&
       !PracticeThisEvent(track_id,ev.NoteNumber(),
             m_state.midi->GetSongPositionInMicroseconds())){
      play=true;draw=true;
    }

    // Even in "You Play" tracks, we have to play the non-note
    // events as per usual.
    if (m_state.track_properties[track_id].mode
        && ev.Type() != MidiEventType_NoteOn
        && ev.Type() != MidiEventType_NoteOff)
      play = true;

    if (ev.Type() == MidiEventType_NoteOn || ev.Type() == MidiEventType_NoteOff) {
      int vel = ev.NoteVelocity();
      const string name = correctedPitch<0?"":MidiEvent::NoteName(correctedPitch);

      bool active = (vel > 0);
      // Display pressed or released a key based on information from a MIDI-file.
      // If this line is deleted, than no notes will be pressed automatically.
      // It is not related to falling notes.
      if (draw && correctedPitch>=0)
        m_keyboard->SetKeyActive(name, active, m_state.track_properties[track_id].color);
      if(correctedPitch>=0)
        filePressedKey(correctedPitch, active, track_id);
    }

    if (play && m_state.midi_out)
    {
      // Patch by track, not by displayed label. No extra MIDI commands to
      // hardware devices; a shared MIDI channel is selected before each note.
      const bool overrideSF2=m_state.midi_out->IsInternalFluid() &&
         track_id<m_state.track_properties.size() &&
         m_state.track_properties[track_id].instrument_program>=0 &&
         ev.Channel()<16;
      if(overrideSF2 && ev.Type()==MidiEventType_ProgramChange)
        continue; // ignore original program changes for this track
      if(overrideSF2 && ev.Type()==MidiEventType_NoteOn &&
         ev.NoteVelocity()>0)
        ApplyTrackInstrument(track_id,ev.Channel());
      else if(ev.Type()==MidiEventType_ProgramChange && ev.Channel()<16){
        m_program_ch[ev.Channel()]=ev.ProgramNumber();
        m_bank_ch[ev.Channel()]=-1;
      }
      // Clone event; sidecar note fixes are allowed only for internal SF2.
      if(correctedPitch==-1)continue;
      MidiEvent ev_out = ev;
      if(internal && (isOn||isOff) && correctedPitch>=0 &&
         correctedPitch!=int(ev.NoteNumber())){
        MidiEventSimple raw;
        if(ev.GetSimpleEvent(&raw)){
          raw.byte1=static_cast<unsigned char>(correctedPitch);
          ev_out=MidiEvent::Build(raw);
        }
      }
      int vel = ev.NoteVelocity();
      // Scale note's volume before playing
      ev_out.SetVelocity(vel * m_state.base_volume);
      m_state.midi_out->Write(ev_out);
    }
  }
}

double PlayingState::CalculateScoreMultiplier() const {
  const static double MaxMultiplier = 5.0;
  double multiplier = 1.0;

  const double combo_addition = m_current_combo / 10.0;
  multiplier += combo_addition;

  return min(MaxMultiplier, multiplier);
}

void PlayingState::Listen() {
  if (!m_state.midi_in)
    return;

  while (m_state.midi_in->KeepReading()) {

    microseconds_t cur_time = m_state.midi->GetSongPositionInMicroseconds();
    MidiEvent ev = m_state.midi_in->Read();
    if (m_state.midi_in->ShouldReconnect())
    {
        m_state.midi_in->Reconnect();
        if(m_state.midi_out)m_state.midi_out->Reconnect();
        continue;
    }


    // Just eat input if we're paused
    if (m_paused)
      continue;

    // Channel filter is opt-in; Omni (0) preserves existing behaviour.
    if(ev.Type()!=MidiEventType_Unknown &&
       m_midi_preferences.midiChannel>0 &&
       ev.Channel()!=m_midi_preferences.midiChannel-1)
      continue;
    // Optional MIDI Thru sends the original channel/velocity unchanged.
    // Matched learning notes below must not be output twice.
    if(m_midi_preferences.midiThru && m_state.midi_out &&
       (ev.Type()==MidiEventType_NoteOn ||
        ev.Type()==MidiEventType_NoteOff ||
        ev.Type()==MidiEventType_Controller ||
        ev.Type()==MidiEventType_ProgramChange))
      m_state.midi_out->Write(ev);
    if(ev.Type()!=MidiEventType_NoteOn && ev.Type()!=MidiEventType_NoteOff)
      continue;

    // Octave Sliding
    ev.ShiftNote(m_note_offset);

    int note_number = ev.NoteNumber();
    string note_name = MidiEvent::NoteName(note_number);

    // On key release we have to look for existing "active" notes and turn them off.
    if (ev.Type() == MidiEventType_NoteOff || ev.NoteVelocity() == 0) {

      // NOTE: This assumes mono-channel input.  If they're piping an entire MIDI file
      //       (or even the *same* MIDI file) through another source, we could get the
      //       same NoteId on different channels -- and this code would start behaving
      //       incorrectly.
      for (ActiveNoteSet::iterator i = m_active_notes.begin(); i != m_active_notes.end(); ++i) {
        if (ev.NoteNumber() != i->note_id)
          continue;

        // Play it on the correct channel to turn the note we started
        // previously, off.
        ev.SetChannel(i->channel);
        if (m_state.midi_out && !m_midi_preferences.midiThru)
          m_state.midi_out->Write(ev);

        m_active_notes.erase(i);
        break;
      }

      // User releases the key
      // If we delete this line, than all pressed keys will be gray until
      // it is unpressed automatically
      m_keyboard->SetKeyActive(note_name, false, Track::FlatGray);
      userPressedKey(note_number, false);
      continue;
    }

    TranslatedNoteSet::iterator closest_match = m_notes.end();
    for (TranslatedNoteSet::iterator i = m_notes.begin(); i != m_notes.end(); ++i) {

      const microseconds_t window_start = i->start - (KeyboardDisplay::NoteWindowLength / 2);
      const microseconds_t window_end = i->start + (KeyboardDisplay::NoteWindowLength / 2);

      // As soon as we start processing notes that couldn't possibly
      // have been played yet, we're done.
      if (window_start > cur_time)
        break;

      if (i->state != UserPlayable)
        continue;

      if (window_end > cur_time && i->note_id == ev.NoteNumber()) {

        if (closest_match == m_notes.end()) {
          closest_match = i;
          continue;
        }

        microseconds_t this_distance = cur_time - i->start;
        if (i->start > cur_time)
          this_distance = i->start - cur_time;

        microseconds_t known_best = cur_time - closest_match->start;
        if (closest_match->start > cur_time)
          known_best = closest_match->start - cur_time;

        if (this_distance < known_best)
          closest_match = i;
      }
    }

    Track::TrackColor note_color = Track::FlatGray;

    if (closest_match != m_notes.end()) {
      note_color = m_state.track_properties[closest_match->track_id].color;

      // "Open" this note so we can catch the close later and turn off
      // the note.
      ActiveNote n;
      n.channel = closest_match->channel;
      n.note_id = closest_match->note_id;
      n.velocity = closest_match->velocity;
      m_active_notes.insert(n);

      // Play it
      ev.SetChannel(n.channel);
      ev.SetVelocity(n.velocity);

      bool silently =
          m_state.track_properties[closest_match->track_id].mode == Track::ModeYouPlaySilently ||
          m_state.track_properties[closest_match->track_id].mode == Track::ModeLearningSilently;
      if (m_state.midi_out && !silently && !m_midi_preferences.midiThru)
        m_state.midi_out->Write(ev);

      // Adjust our statistics
      const static double NoteValue = 100.0;
      m_state.stats.score += NoteValue * CalculateScoreMultiplier() * (m_state.song_speed / 100.0);

      m_state.stats.notes_user_could_have_played++;
      m_state.stats.speed_integral += m_state.song_speed;

      m_state.stats.notes_user_actually_played++;
      m_current_combo++;
      m_state.stats.longest_combo = max(m_current_combo, m_state.stats.longest_combo);

      TranslatedNote replacement = *closest_match;
      replacement.state = UserHit;

      m_notes.erase(closest_match);
      m_notes.insert(replacement);
    }

    else {
      // Un error exige una tecla incorrecta EN EL INSTANTE que hay
      // una nota de práctica que deba pulsarse. No se penalizan
      // silencios, acompañamientos, pistas automáticas ni repeticiones
      // de un note-on de una tecla ya mantenida.
      std::vector<int> expectedKeys;
      const bool evaluationActive=m_any_you_play_tracks && !m_paused &&
          !m_scrubbing && !m_settings_open && !m_loop_editor;
      if(evaluationActive){
        for(const auto& note:m_notes){
          if(note.start-KeyboardDisplay::NoteWindowLength/2 > cur_time)
            break;
          if(note.state!=UserPlayable || !isUserPlayableTrack(note.track_id))
            continue;
          if(note.start+KeyboardDisplay::NoteWindowLength/2 >= cur_time)
            expectedKeys.push_back(note.note_id);
        }
      }
      if(PracticeError::WrongPress(note_number,isKeyPressed(note_number),
                                   evaluationActive,expectedKeys))
        m_state.stats.stray_notes++;
    }

    m_state.stats.total_notes_user_pressed++;
    // Display a pressed key by an user
    // Display a colored key, if it is pressed correctly
    // Otherwise display a grey key
    // 
    // If we comment this code, than a missed user pressed key will not shown.
    // But correct presed key will be shown as usual.
    m_keyboard->SetKeyActive(note_name, true, note_color);
    userPressedKey(note_number, true);
  }
}


void PlayingState::RefreshChord(bool immediate) {
  const unsigned long now=SDL_GetTicks();
  if(!immediate && now-m_chord_last_refresh<65)return;
  m_chord_last_refresh=now;
  const microseconds_t position=m_state.midi->GetSongPositionInMicroseconds();
  std::vector<int> sounding;
  sounding.reserve(16);
  const auto& notes=m_state.midi->Notes();
  for(const auto& note:notes){
    if(note.start>position)break;
    if(note.end<=position || note.channel==9 ||
       note.track_id>=m_state.track_properties.size())continue;
    if(m_state.track_properties[note.track_id].mode==Track::ModeNotPlayed)
      continue;
    sounding.push_back(static_cast<int>(note.note_id));
  }
  for(const int note:m_pressed_notes)sounding.push_back(note);
  const std::string candidate=ChordDetector::Detect(sounding);
  if(immediate){
    m_current_chord=m_pending_chord=candidate;
    m_chord_pending_since=now;
  }else if(candidate!=m_pending_chord){
    m_pending_chord=candidate;
    m_chord_pending_since=now;
  }else if(now-m_chord_pending_since>=115){
    m_current_chord=candidate;
  }
}

int PlayingState::EffectiveTempoBPM() const {
  if(!m_state.midi)return 120;
  const double original=m_state.midi->GetCurrentBPM();
  return static_cast<int>(original*m_tempo_factor+0.5);
}
void PlayingState::AdjustTempoBPM(int delta) {
  if(!m_state.midi)return;
  const int original=static_cast<int>(m_state.midi->GetCurrentBPM());
  if(original<=0)return;
  const int target=std::max(20,std::min(400,EffectiveTempoBPM()+delta));
  m_tempo_factor=target/static_cast<double>(original);
  // Compatibilidad del sistema de puntuación del reproductor.
  m_state.song_speed=static_cast<int>(100.0*m_tempo_factor+0.5);
}
void PlayingState::ResetTempoBPM() {
  m_tempo_factor=1.0;
  m_state.song_speed=100;
}

void PlayingState::SeekTo(microseconds_t target){
  ClearLightGuides();
  if(!m_state.midi)return;
  const microseconds_t start=m_state.midi->GetDeadAirStartOffsetMicroseconds();
  const microseconds_t earliest=m_state.midi->GetSongInitialPositionInMicroseconds();
  const microseconds_t length=m_state.midi->GetSongLengthInMicroseconds();
  // Se admite la cuenta previa original para que Inicio no se salte
  // el primer NoteOn. Nunca saltar detrás del último evento del tema.
  target=std::max(earliest,std::min(target,start+std::max<microseconds_t>(0,length-1000)));
  if(m_loop_defined && m_loop_active && target<=m_loop_start+2000)
    m_loop_error_baseline=m_state.stats.stray_notes;
  if(m_state.midi_out)m_state.midi_out->Reset(); // all-notes-off ANTES de GoTo
  if(m_state.midi_in)m_state.midi_in->Reset();
  m_state.midi->GoTo(target);
  ResetInstrumentCache();
  RebuildMusicalEventCursors(target);
  m_pressed_notes.clear();
  m_required_notes.clear();
  m_active_notes.clear();
  m_keyboard->ResetActiveKeys();
  m_notes=m_state.midi->Notes();
  ApplyApprovedMusicalCorrections();
  m_notes_history.clear();
  SetupNoteState();
  eraseUntilTime(target);
  RefreshChord(true);
  m_should_retry=false;
  m_should_wait_after_retry=false;
  m_retry_start=target;
  m_current_combo=0;
  // Al desplazarse, restaurar programa, banco y volumen relevantes
  // para no dejar notas reanudadas con un instrumento equivocado.
  if(m_state.midi_out){
    const unsigned char restoreCC[]={0,32,7,10,11,64,91,93};
    struct Stored {bool set=false;microseconds_t when=0;MidiEvent event;};
    Stored program[16],controllers[16][8];
    const auto& tracks=m_state.midi->Tracks();
    for(size_t t=0;t<tracks.size();++t){
      const auto& es=tracks[t].Events();
      const auto& ts=tracks[t].EventUsecs();
      for(size_t i=0;i<es.size() && i<ts.size();++i){
        if(ts[i]>target)break;
        const MidiEvent& e=es[i];
        const auto type=e.Type();
        if(type!=MidiEventType_ProgramChange &&
           type!=MidiEventType_Controller)continue;
        const unsigned int ch=e.Channel();
        if(ch>=16)continue;
        Stored* slot=nullptr;
        if(type==MidiEventType_ProgramChange)slot=&program[ch];
        else {
          MidiEventSimple raw;
          if(!e.GetSimpleEvent(&raw))continue;
          for(size_t k=0;k<8;k++)if(raw.byte1==restoreCC[k]){
            slot=&controllers[ch][k];break;
          }
        }
        if(slot && (!slot->set || slot->when<=ts[i])){
          slot->set=true;slot->when=ts[i];slot->event=e;
        }
      }
    }
    for(int ch=0;ch<16;ch++){
      for(int k=0;k<2;k++)
        if(controllers[ch][k].set)m_state.midi_out->Write(controllers[ch][k].event);
      if(program[ch].set)m_state.midi_out->Write(program[ch].event);
      for(int k=2;k<8;k++)
        if(controllers[ch][k].set)m_state.midi_out->Write(controllers[ch][k].event);
    }
  }
  // Recuperar notas automáticas que cruzan exactamente el punto de salto.
  for(const auto& n:m_state.midi->Notes()){
    if(n.start>=target || n.end<=target ||
       n.track_id>=m_state.track_properties.size())continue;
    const auto& props=m_state.track_properties[n.track_id];
    if(props.mode==Track::ModePlayedAutomatically ||
       props.mode==Track::ModePlayedButHidden){
      if(props.mode==Track::ModePlayedAutomatically)
        m_keyboard->SetKeyActive(MidiEvent::NoteName(n.note_id),true,props.color);
      if(m_state.midi_out){
        MidiEvent e=MidiEvent::Build(MidiEventSimple(
            static_cast<unsigned char>(0x90|(n.channel&0x0f)),
            static_cast<unsigned char>(n.note_id),
            static_cast<unsigned char>(std::max(1,std::min(n.velocity,127)))));
        e.SetVelocity(static_cast<int>(n.velocity*m_state.base_volume));
        m_state.midi_out->Write(e);
      }
    }
  }
}

double PlayingState::LoopFraction(int mouseX) const {
  const int barX=58;
  const int barWidth=std::max(90,GetStateWidth()-116);
  return std::max(0.0,std::min(1.0,
      (mouseX-barX)/static_cast<double>(barWidth)));
}

void PlayingState::OpenLoopEditor() {
  m_loop_editor=true;
  m_loop_restore_paused=m_paused;
  m_paused=true;
  m_loop_dragging=false;
  m_loop_prompt=false;
  m_loop_draft_valid=m_loop_defined;
  const microseconds_t start=m_state.midi->GetDeadAirStartOffsetMicroseconds();
  const microseconds_t length=std::max<microseconds_t>(
      1,m_state.midi->GetSongLengthInMicroseconds());
  m_loop_draft_start=m_loop_defined ?
    std::max(0.0,std::min(1.0,(m_loop_start-start)/static_cast<double>(length))) : 0.0;
  m_loop_draft_end=m_loop_defined ?
    std::max(0.0,std::min(1.0,(m_loop_end-start)/static_cast<double>(length))) : 0.0;
  if(m_state.midi_out)m_state.midi_out->Reset();
  m_state.dpms_thread->pauseScreensaver(false);
}

void PlayingState::CloseLoopEditor(bool apply){
  if(apply && m_loop_draft_valid){
    const microseconds_t base=m_state.midi->GetDeadAirStartOffsetMicroseconds();
    const microseconds_t duration=m_state.midi->GetSongLengthInMicroseconds();
    const double lower=std::min(m_loop_draft_start,m_loop_draft_end);
    const double upper=std::max(m_loop_draft_start,m_loop_draft_end);
    if((upper-lower)*duration < 100000){m_loop_prompt=true;return;}
    m_loop_start=base+static_cast<microseconds_t>(duration*lower);
    m_loop_end=base+static_cast<microseconds_t>(duration*upper);
    m_loop_defined=true;
    m_loop_active=true;
  } else if(apply && !m_loop_draft_valid) {
    m_loop_prompt=true;
    return;
  }
  m_loop_editor=false;
  m_loop_dragging=false;
  m_paused=m_loop_restore_paused;
  m_state.dpms_thread->pauseScreensaver(!m_any_learning_track&&!m_paused);
  // Reactiva sonidos continuos después de la pausa en modo editor
  // y coloca el cabezal dentro del intervalo seleccionado.
  microseconds_t position=m_state.midi->GetSongPositionInMicroseconds();
  if(m_loop_active &&
     (position<m_loop_start || position>=m_loop_end))
    position=m_loop_start;
  SeekTo(position);
}

bool PlayingState::HandleLoopEditor(const MouseInfo& mouse){
  if(!m_loop_editor)return false;
  const int width=GetStateWidth();
  const int barWidth=std::max(90,width-116);
  const bool bar=mouse.y>=56 && mouse.y<=94 &&
                 mouse.x>=58 && mouse.x<=58+barWidth;
  const int left=std::max(10,(width-460)/2);
  const bool clear=mouse.x>=left+112 && mouse.x<left+214 &&
                   mouse.y>=202 && mouse.y<=236;
  const bool apply=mouse.x>=left+246 && mouse.x<left+348 &&
                   mouse.y>=202 && mouse.y<=236;
  if(mouse.newPress.left && bar){
    // Cada nuevo clic reinicia el rango anterior; se admite arrastrar
    // tanto desde la izquierda como desde la derecha.
    m_loop_draft_start=LoopFraction(mouse.x);
    m_loop_draft_end=m_loop_draft_start;
    m_loop_draft_valid=false;
    m_loop_prompt=false;
    m_loop_dragging=true;
  }
  if(m_loop_dragging){
    if(mouse.held.left){
      m_loop_draft_end=LoopFraction(mouse.x);
    }
    if(mouse.released.left){
      m_loop_draft_end=LoopFraction(mouse.x);
      const auto len=m_state.midi->GetSongLengthInMicroseconds();
      m_loop_draft_valid=
          std::abs(m_loop_draft_end-m_loop_draft_start)*len>=100000;
      m_loop_dragging=false;
      m_loop_prompt=!m_loop_draft_valid;
    }
    return true;
  }
  if(mouse.released.left && clear){
    m_loop_draft_valid=false;
    m_loop_defined=false;
    m_loop_active=false;
    m_loop_prompt=false;
    m_loop_draft_start=m_loop_draft_end=0.0;
  }else if(mouse.released.left && apply){
    CloseLoopEditor(true);
  }
  return true;
}

namespace {
// Una única geometría compartida entre renderizado y ratón.
// 17 elementos en una sola línea, también al reducir la ventana.
struct RibbonBox {
  int x,y,w,h;
  bool Contains(const MouseInfo& m) const {
    return m.x>=x && m.x<x+w && m.y>=y && m.y<y+h;
  }
};
RibbonBox Ribbon(int index,int width){
  struct Base {int x,y,w,h;};
  const Base boxes[]={
    {0,10,55,34},    // 0 Volver
    {59,10,48,34},   // 1 Ayuda
    {111,10,35,34},  // 2 retroceder
    {150,10,35,34},  // 3 play
    {189,10,35,34},  // 4 avanzar
    {231,10,167,34}, // 5 tempo LCD
    {310,11,20,15},  // 6 tempo+
    {310,29,20,15},  // 7 tempo-
    {335,13,58,29},  // 8 REST
    {404,10,90,34},  // 9 COMPÁS
    {499,10,98,34},  // 10 LECTOR
    {602,10,76,34},  // 11 FALLOS
    {683,10,131,34}, // 12 ACORDE
    {819,10,137,34}, // 13 TONALIDAD
    {961,10,68,34},  // 14 BUCLE
    {1034,10,52,34}, // 15 VISTA
    {1091,10,75,34}  // 16 AJUSTES
  };
  if(index<0 || index>17)return {0,0,0,0};
  // Inicio encabeza la MISMA fila, sin desplazamiento a otra banda.
  const double f=std::min(1.3,std::max(0.39,(width-12)/1230.0));
  if(index==17)return {6,10,std::max(24,static_cast<int>(52*f)),34};
  const Base a=boxes[index];
  return {6+static_cast<int>((a.x+57)*f),a.y,
    std::max(8,static_cast<int>(a.w*f)),a.h};
}
int RibbonTextSize(int width,bool value=false){
  const int base=value?15:12;
  return width<850 ? base-3 : (width<1040 ? base-2 : base);
}
}
bool PlayingState::HandleTransport(const MouseInfo& mouse,bool& changed){
  const int width=GetStateWidth();
  const int barX=58,barW=std::max(90,width-116);
  const bool onBar=mouse.x>=barX && mouse.x<=barX+barW &&
                   mouse.y>=56 && mouse.y<=94;
  if((mouse.newPress.right || mouse.released.right) &&
      Ribbon(14,width).Contains(mouse) && !m_settings_open){
    if(!m_loop_editor)OpenLoopEditor();
    changed=true;return false;
  }
  if(m_loop_editor){
    HandleLoopEditor(mouse);
    changed=true;return false;
  }
  // A floating editor intercepts mouse events only inside its bounds.
  // Transport and performance continue normally around the window.
  if(m_settings_open && HandlePlayerSettings(mouse)){
    changed=mouse.newPress.left || mouse.released.left;
    return false;
  }
  // Recording is controlled only from the floating Ajustes window.
  // Reopening settings while recording must remain possible.
  if(m_video.Active() && mouse.released.left &&
     Ribbon(16,width).Contains(mouse)){
    if(!m_settings_open)OpenPlayerSettings();
    changed=true;return false;
  }
  if(m_video.Active())return false;
  // No floating mode buttons across the 3D stage. F2 still controls
  // the score; F6 toggles whole-song listening without visual clutter.
  // Hand practice is configured on the track-selection screen or under
  // Ajustes > Evaluación. It does NOT overlay the performance scene.
  // Orbital camera in the real Blender mesh viewer:
  // right-drag rotates, the wheel dollys in/out, no MIDI keys stolen.
  // Do not intercept the floating settings window or transport controls.
  const bool orbitArea=mouse.y>98 &&
       mouse.y<GetStateHeight()-65 && mouse.x>12 &&
       mouse.x<GetStateWidth()-12 &&
       (!m_settings_open ||
          !FloatingSettings::Fit(GetStateWidth(),GetStateHeight(),
             m_settings_x,m_settings_y).bounds().contains(mouse.x,mouse.y));
  const bool orbitEnabled=m_view_3d &&
       m_midi_preferences.playerStyle3D==2 &&
       m_midi_preferences.scenePreset>=0;
  if(orbitEnabled && orbitArea && mouse.wheelY){
    m_midi_preferences.sceneZoom=StageCamera::Clamp(
       m_midi_preferences.sceneZoom-mouse.wheelY*35,180,1250);
    m_midi_preferences.sceneView=0;
    std::string error;
    if(!m_settings_open) AdvancedSettingsIO::Save(m_midi_preferences,error);
  }
  if(orbitEnabled && mouse.newPress.right && orbitArea){
    m_orbit_dragging=true;
    m_orbit_last_x=mouse.x;
    m_orbit_last_y=mouse.y;
  }
  if(m_orbit_dragging && mouse.held.right){
    const int dx=mouse.x-m_orbit_last_x;
    const int dy=mouse.y-m_orbit_last_y;
    if(dx || dy){
      m_midi_preferences.sceneYaw=StageCamera::WrapYaw(
         m_midi_preferences.sceneYaw+dx/2);
      m_midi_preferences.scenePitch=StageCamera::Clamp(
         m_midi_preferences.scenePitch-dy/3,5,78);
      m_midi_preferences.sceneView=0;
    }
    m_orbit_last_x=mouse.x;
    m_orbit_last_y=mouse.y;
  }
  if(m_orbit_dragging && mouse.released.right){
    m_orbit_dragging=false;
    std::string error;
    if(!m_settings_open) AdvancedSettingsIO::Save(m_midi_preferences,error);
  }
  if(mouse.newPress.left && onBar){
    m_scrubbing=true;
    m_scrub_fraction=LoopFraction(mouse.x);
  }
  if(m_scrubbing){
    if(mouse.held.left){
      m_scrub_fraction=LoopFraction(mouse.x);
      changed=true;
    }
    if(mouse.released.left){
      const auto start=m_state.midi->GetDeadAirStartOffsetMicroseconds();
      const auto len=m_state.midi->GetSongLengthInMicroseconds();
      SeekTo(start+static_cast<microseconds_t>(m_scrub_fraction*len));
      m_scrubbing=false;changed=true;
    }
    return false;
  }
  if(!mouse.released.left)return false;
  auto hit=[&](int i){return Ribbon(i,width).Contains(mouse);};
  if(hit(17)){
    // Vuelta real al inicio musical: sin salir del reproductor.
    // SeekTo apaga las notas sostenidas y reconstruye el estado MIDI.
    const microseconds_t first=m_state.midi->GetSongInitialPositionInMicroseconds();
    SeekTo(first);
    changed=true;
    return false;
  }
  if(hit(0)){
    SavePracticeAttempt(false);
    RecentSongs::RecordPoints(m_state.song_path,static_cast<int>(m_state.stats.score));
    if(m_state.midi_out)m_state.midi_out->Reset();
    if(m_state.midi_in)m_state.midi_in->Reset();
    RestorePracticeTrackModes();
    ChangeState(new TrackSelectionState(m_state));
    return true;
  }
  if(hit(1)){m_show_help=!m_show_help;changed=true;return false;}
  if(hit(3)){
    m_paused=!m_paused;
    m_state.dpms_thread->pauseScreensaver(!m_any_learning_track&&!m_paused);
    changed=true;return false;
  }
  const microseconds_t at=m_state.midi->GetSongPositionInMicroseconds();
  if(hit(2)){SeekTo(at-5000000);changed=true;}
  if(hit(4)){SeekTo(at+5000000);changed=true;}
  if(hit(6)){AdjustTempoBPM(+1);changed=true;}
  if(hit(7)){AdjustTempoBPM(-1);changed=true;}
  if(hit(8)){ResetTempoBPM();changed=true;}
  if(hit(14)){
    if(!m_loop_defined)OpenLoopEditor();
    else {
      m_loop_active=!m_loop_active;
      if(m_loop_active && (at<m_loop_start || at>=m_loop_end))
        SeekTo(m_loop_start);
    }
    changed=true;
  }
  if(hit(15)){
    m_view_3d=!m_view_3d;
    m_midi_preferences.player3D=m_view_3d;
    std::string saveError;
    AdvancedSettingsIO::Save(m_midi_preferences,saveError);
    changed=true;
  }
  if(hit(16)){OpenPlayerSettings();changed=true;}
  return false;
}

void PlayingState::Resize() {
    delete  m_keyboard;
    m_keyboard = new KeyboardDisplay(m_state.keyboard, GetStateWidth() - Layout::ScreenMarginX*2, CalcKeyboardHeight());
}

void PlayingState::Update() {

  // Calculate how visible the title bar should be
  const static double fade_in_ms = 350.0;
  const static double stay_ms = 2500.0;
  const static double fade_ms = 500.0;

  m_title_alpha = 0.0;
  unsigned long ms = GetStateMicroseconds() * max(m_state.song_speed, 50) / 100000;

  if (double(ms) <= stay_ms)
    m_title_alpha = min(1.0, ms / fade_in_ms);

  if (double(ms) >= stay_ms)
    m_title_alpha = min(max((fade_ms - (ms - stay_ms)) / fade_ms, 0.0), 1.0);

  // Lock down the alpha so that if you are slowing the song down as it
  // fades out, it doesn't cut back into a much higher alpha value
  m_title_alpha = min(m_title_alpha, m_max_allowed_title_alpha);

  if (double(ms) > stay_ms)
    m_max_allowed_title_alpha = m_title_alpha;

  // Detect encoder crashes while the song is still playing, restoring
  // all original track modes and showing the actual FFmpeg error-log path.
  // Previously a broken pipe could silently leave a frozen "recording" UI.
  if(m_video.Active() && m_video.Failed()){
    StopVideoExport(false);
    return;
  }
  // Esc cancela una grabación activa y elimina el fichero incompleto.
  // No se interpreta como salida a la pantalla de selección de pistas.
  if(m_video.Active() && IsKeyPressed(KeyEscape)){
    StopVideoExport(true);
    return;
  }
  // Un diálogo modal de exportación no permite clics en el teclado ni en
  // el transporte; la canción permanece pausada hasta confirmar.
  if(m_video_dialog){
    HandleVideoDialog(Mouse());
    if(IsKeyPressed(KeyEscape))CloseVideoDialog();
    return;
  }
  // The score editor handles noteheads before the orbit camera can
  // treat a right click as a scene rotation.
  if(HandleStaffHandEditor(Mouse()))return;
  bool transport_changed=false;
  if(HandleTransport(Mouse(),transport_changed))return;
  // El editor permanece pausado; Esc cancela sin tocar el rango aplicado.
  if(m_loop_editor){
    if(IsKeyPressed(KeyEscape))CloseLoopEditor(false);
    return;
  }
  if(m_settings_open && IsKeyPressed(KeyEscape))
    ClosePlayerSettings(); // never stop the MIDI clock while editing
  if(m_show_help && IsKeyPressed(KeyEscape)){
    m_show_help=false;return;
  }

  microseconds_t delta_microseconds = static_cast<microseconds_t>(GetDeltaMicroseconds());

  // The 100 term is really paired with the playback speed, but this
  // formation is less likely to produce overflow errors.
  delta_microseconds = static_cast<microseconds_t>(
      delta_microseconds*m_tempo_factor);

  if (m_paused || m_scrubbing || transport_changed)
    delta_microseconds = 0;

  // Our delta milliseconds on the first frame after state start is extra
  // long because we just reset the MIDI.  By skipping the "Play" that
  // update, we don't have an artificially fast-forwarded start.
  if (!m_first_update) {
    if (areAllRequiredKeysPressed())
    {
      // Limitar el transporte al punto B antes de procesar eventos MIDI.
      if(m_loop_active && !m_paused){
        const microseconds_t at=m_state.midi->GetSongPositionInMicroseconds();
        if(at>=m_loop_end){
          SeekTo(m_loop_start);
          delta_microseconds=0;
        }else if(at+delta_microseconds>=m_loop_end){
          delta_microseconds=std::max<microseconds_t>(0,m_loop_end-at);
        }
      }
      Play(delta_microseconds);
      // La vuelta al punto A limpia notas y reconstruye instrumentos/teclado.
      if(m_loop_active && !m_paused &&
         m_state.midi->GetSongPositionInMicroseconds()>=m_loop_end)
        SeekTo(m_loop_start);
//    m_should_wait_after_retry = false; // always reset onces pressed
    }
    else {
      m_current_combo = 0;
    }

    Listen();
  }
  UpdateLightGuides();

  // The learner controls this threshold in Transporte.  Evaluate *after*
  // reading input to count only actual wrong presses.  Never alter song
  // tempo, score, or the user's MIDI file.  A disabled threshold does
  // nothing and leaves the existing looping engine unchanged.
  if(m_loop_active && m_loop_defined && !m_paused &&
     m_midi_preferences.loopRestartErrors>0){
    if(!m_loop_errors_monitoring){
      m_loop_error_baseline=m_state.stats.stray_notes;
      m_loop_errors_monitoring=true;
    }else if(LoopErrorPolicy::ShouldRestart(
               m_midi_preferences.loopRestartErrors,
               m_loop_error_baseline,m_state.stats.stray_notes,
               m_loop_defined,m_loop_active,m_paused)){
      SeekTo(m_loop_start);
      m_loop_error_baseline=m_state.stats.stray_notes;
    }
  }else{
    m_loop_errors_monitoring=false;
  }

  m_first_update = false;
  RefreshChord();

  microseconds_t cur_time = m_state.midi->GetSongPositionInMicroseconds();

  // Delete notes that are finished playing (and are no longer available to hit)
  TranslatedNoteSet::iterator i = m_notes.begin();
  while (i != m_notes.end()) {
    TranslatedNoteSet::iterator note = i++;

    const microseconds_t window_end = note->start + (KeyboardDisplay::NoteWindowLength / 2);

    if (m_state.midi_in && note->state == UserPlayable && window_end <= cur_time){
      // Solo notas del intérprete realmente omitidas; las automáticas
      // jamás incrementan el contador. Se cuenta una vez por transición.
      ++m_missed_notes;
      TranslatedNote note_copy = *note;
      note_copy.state = UserMissed;

      m_notes.erase(note);
      m_notes.insert(note_copy);

      // Re-connect the (now-invalid) iterator to the replacement
      note = m_notes.find(note_copy);

      if (m_state.track_properties[note->track_id].is_retry_on
          && !m_should_wait_after_retry)
        // They missed a note and should retry
        // We don't count misses while waiting after retry
        m_should_retry = true;
    }

    if (note->start > cur_time)
      break;

    if (note->end < cur_time && window_end < cur_time) {

      if (note->state == UserMissed) {
        // They missed a note, reset the combo counter
        m_current_combo = 0;

        m_state.stats.notes_user_could_have_played++;
        m_state.stats.speed_integral += m_state.song_speed;
      }

      TranslatedNote history_note = *note;
      m_notes_history.insert(history_note);
      m_notes.erase(note);
    }
  }

  if(IsKeyPressed(KeyGreater))
    m_note_offset += 12;

  if(IsKeyPressed(KeyLess))
    m_note_offset -= 12;

  if (IsKeyPressed(KeyUp)) {
    m_show_duration -= 250000;

    const static microseconds_t MinShowDuration = 250000;
    if (m_show_duration < MinShowDuration)
      m_show_duration = MinShowDuration;
  }

  if (IsKeyPressed(KeyDown)) {
    m_show_duration += 250000;

    const static microseconds_t MaxShowDuration = 10000000;
    if (m_show_duration > MaxShowDuration)
      m_show_duration = MaxShowDuration;
  }

  if (IsKeyPressed(KeyLeft)) AdjustTempoBPM(-1);
  if (IsKeyPressed(KeyRight)) AdjustTempoBPM(+1);

  if (IsKeyPressed(KeyVolumeDown)) {
    m_state.base_volume -= 0.1;
    if (m_state.base_volume < 0.1)
      m_state.base_volume = 0;
  }

  if (IsKeyPressed(KeyVolumeUp)) {
    m_state.base_volume += 0.1;
    if (m_state.base_volume > 1.9) // Maximum volume is 200%
      m_state.base_volume = 2;
  }

  if (IsKeyPressed(KeyForward)) {
    // Go 5 seconds forward
    microseconds_t cur_time = m_state.midi->GetSongPositionInMicroseconds();
    microseconds_t new_time = cur_time + 5000000;
    SeekTo(new_time);
  }
  else
  if (IsKeyPressed(KeyBackward)) {
    // Go 5 seconds back
    microseconds_t cur_time = m_state.midi->GetSongPositionInMicroseconds();
    microseconds_t new_time = cur_time - 5000000;
    SeekTo(new_time);
  }
  else
  {
    // Check retry conditions
    // track_properties
    microseconds_t next_bar_time =
        m_state.midi->GetNextBarInMicroseconds(m_retry_start);
    microseconds_t cur_time = m_state.midi->GetSongPositionInMicroseconds();
    // Check point in future
    microseconds_t checkpoint_time = cur_time + delta_microseconds + 1;
//  microseconds_t checkpoint_time = cur_time;
    bool next_bar_exists = next_bar_time != 0;
    bool next_bar_reached = checkpoint_time > next_bar_time;
    if (next_bar_exists && next_bar_reached)
    {
      if (m_should_retry)
      {
        TranslatedNoteSet old = m_notes;
        old.insert(m_notes_history.begin(), m_notes_history.end());

        // Forget failed notes
        m_should_retry = false;
        // Should wait after retry for initial keys to be pressed
        m_should_wait_after_retry = true;

        microseconds_t delta_microseconds = static_cast<microseconds_t>(GetDeltaMicroseconds());
        microseconds_t new_time= m_retry_start-delta_microseconds;
        // Retry
        m_state.midi->GoTo(new_time);
        RebuildMusicalEventCursors(new_time);
        ResetInstrumentCache();
        m_required_notes.clear();
        m_pressed_notes.clear();
        m_state.midi_out->Reset();
        m_keyboard->ResetActiveKeys();
        TranslatedNoteSet def = m_state.midi->Notes();
        if(m_state.midi_out->IsInternalFluid() && !m_review_pitch_changes.empty()){
          TranslatedNoteSet adjusted;
          for(const auto& note:def){
            TranslatedNote n=note;
            const auto it=m_review_pitch_changes.find(
                std::make_tuple(note.track_id,int(note.note_id),note.start));
            if(it!=m_review_pitch_changes.end()){
              if(it->second<0)continue;
              n.note_id=static_cast<NoteId>(it->second);
            }
            adjusted.insert(n);
          }
          def.swap(adjusted);
        }

        // Set retry_state
        // For each current node
        // from SetupNoteState
        m_notes.clear();
        m_notes_history.clear();
        for (TranslatedNoteSet::iterator i = def.begin(); i != def.end(); i++) {
          TranslatedNote n = *i;

          n.state = AutoPlayed;
          n.retry_state = AutoPlayed;
          if (isUserPlayableTrack(n.track_id) && isNoteInPlayableRange(n.note_id))
          {
            n.state = UserPlayable;
            n.retry_state = findNodeState(n, old, UserPlayable);
          }

          m_notes.insert(n);
        }

        // To avoid checks for keys that start before and stop after new_time 
        eraseUntilTime(new_time);
      }
      else
      {
        // Handle new retry block
        m_retry_start = cur_time;
      }
    }
  }

  // Familiar Synthesia shortcuts, mapped to Linthesia's own controls;
  // no dependency on the commercial executable or its saved settings.
  if(!m_settings_open && (IsKeyPressed(KeyF2)||
                          IsKeyPressed(KeyF3)||IsKeyPressed(KeyF5))){
    if(IsKeyPressed(KeyF2))
      m_midi_preferences.notationVisible=!m_midi_preferences.notationVisible;
    if(IsKeyPressed(KeyF3)){
      const int sizes[]={75,100,125,150,175,200};
      const int current=m_midi_preferences.notationZoom;
      auto next=std::find_if(std::begin(sizes),std::end(sizes),
                             [current](int value){return value>current;});
      m_midi_preferences.notationZoom=
        next==std::end(sizes)?75:*next;
    }
    if(IsKeyPressed(KeyF5))
      m_midi_preferences.fallingNotesVisible=
           !m_midi_preferences.fallingNotesVisible;
    std::string error;
    if(!AdvancedSettingsIO::Save(m_midi_preferences,error))
      fprintf(stderr,"Linthesia: no se pudieron guardar atajos: %s\n",
              error.c_str());
  }
  if(!m_settings_open && IsKeyPressed(KeyF6))ToggleFullSongAudio();
  if(!m_settings_open && IsKeyPressed(KeyBackspace) && m_loop_defined){
    SeekTo(m_loop_start);
  }
  if(!m_settings_open && (IsKeyPressed(KeyF7)||
                          IsKeyPressed(KeyF8)||IsKeyPressed(KeyF9))){
    const auto time=std::max<microseconds_t>(0,
                 m_state.midi->GetSongPositionInMicroseconds());
    if(IsKeyPressed(KeyF7)){
      const auto label="M"+std::to_string(m_song_bookmarks.size()+1);
      if(SongBookmarks::Add(m_song_bookmarks,time,label)){
        SongBookmarks::Save(m_state.song_path,m_song_bookmarks);
        m_bookmark_feedback="Marcador "+label+" creado";
      }else m_bookmark_feedback="Marcador ya existente o límite 64";
    }
    if(IsKeyPressed(KeyF8)){
      const auto target=SongBookmarks::Next(m_song_bookmarks,time);
      if(target>=0){SeekTo(target);m_bookmark_feedback="Marcador siguiente";}
    }
    if(IsKeyPressed(KeyF9)){
      if(SongBookmarks::RemoveNearest(m_song_bookmarks,time)){
        SongBookmarks::Save(m_state.song_path,m_song_bookmarks);
        m_bookmark_feedback="Marcador eliminado";
      }
    }
  }
  if (IsKeyPressed(KeySpace))
  {
    m_paused = !m_paused;
    m_state.dpms_thread->pauseScreensaver(!m_any_learning_track && !m_paused);
  }

  if (IsKeyPressed(KeyEscape)) {
    SavePracticeAttempt(false);
    RecentSongs::RecordPoints(m_state.song_path,static_cast<int>(m_state.stats.score));
    if (m_state.midi_out)
      m_state.midi_out->Reset();

    if (m_state.midi_in)
      m_state.midi_in->Reset();

    RestorePracticeTrackModes();
    ChangeState(new TrackSelectionState(m_state));
    return;
  }

  if(m_video.Active() && m_state.midi->IsSongOver()){
    StopVideoExport(false);
    return; // Conservar el reproductor y sus ajustes tras terminar.
  }
  if (m_state.midi->IsSongOver()) {
    SavePracticeAttempt(true);
    RecentSongs::RecordPoints(m_state.song_path,static_cast<int>(m_state.stats.score));
    if (m_state.midi_out)
      m_state.midi_out->Reset();

    if (m_state.midi_in)
      m_state.midi_in->Reset();

    const bool goToStatistics=m_state.midi_in && m_any_you_play_tracks &&
                              !m_full_song_audio;
    RestorePracticeTrackModes();
    if(goToStatistics)
      ChangeState(new StatsState(m_state));
    else
      ChangeState(new TrackSelectionState(m_state));

    return;
  }
}


namespace {
std::string ClockText(microseconds_t us){
  const unsigned long sec=static_cast<unsigned long>(
      std::max<microseconds_t>(0,us)/1000000);
  return STRING(sec/60 << ":" << setfill('0') << setw(2) << sec%60);
}
}


namespace {
struct PlayerSettingEntry {
  int id; std::string title; int value; int minimum,maximum,step;
  std::string unit; bool toggle=false;
};
int clampInt(int value,int minimum,int maximum){
  return std::max(minimum,std::min(maximum,value));
}
void Apply3DProfile(AdvancedSettings& prefs,int style){
  // Elegir un estilo carga un perfil completo, editable por el usuario.
  prefs.playerStyle3D=clampInt(style,0,2);
  if(style==2){ // Cinematográfico: escenario frontal, halo y partículas.
    prefs.player3DDepth=0; prefs.player3DAngle=0;
    prefs.player3DHorizon=10; prefs.player3DStageWidth=100;
    prefs.player3DHeight=20; prefs.player3DWidth=105;
    prefs.player3DRadius=7; prefs.player3DOpacity=91;
    prefs.player3DGlow=94; prefs.player3DBrightness=129;
    prefs.player3DGuideSize=1; prefs.player3DGuideOpacity=18;
    prefs.player3DKeyOpacity=100; prefs.player3DBackgroundOpacity=100;
  }else if(style==1){ // Moderno: profundidad y luz controladas.
    prefs.player3DDepth=46; prefs.player3DAngle=24;
    prefs.player3DHorizon=12; prefs.player3DStageWidth=112;
    prefs.player3DHeight=15; prefs.player3DWidth=110;
    prefs.player3DRadius=8; prefs.player3DOpacity=84;
    prefs.player3DGlow=58; prefs.player3DBrightness=118;
    prefs.player3DGuideSize=1; prefs.player3DGuideOpacity=18;
    prefs.player3DKeyOpacity=96; prefs.player3DBackgroundOpacity=88;
  }else{ // Clásico: lectura amplia sin efecto de embudo.
    prefs.player3DDepth=38; prefs.player3DAngle=20;
    prefs.player3DHorizon=10; prefs.player3DStageWidth=116;
    prefs.player3DHeight=15; prefs.player3DWidth=108;
    prefs.player3DRadius=4; prefs.player3DOpacity=100;
    prefs.player3DGlow=18; prefs.player3DBrightness=108;
    prefs.player3DGuideSize=1; prefs.player3DGuideOpacity=28;
    prefs.player3DKeyOpacity=100; prefs.player3DBackgroundOpacity=92;
  }
  prefs.player3DShadows=true;
}
std::vector<PlayerSettingEntry> PlayerSettingEntries(
    const AdvancedSettings& p,int tab,int bpm,bool loopActive){
  std::vector<PlayerSettingEntry> e;
  const auto add=[&](int id,const std::string& label,int val,int min,int max,
                     int step=1,const std::string&unit="",bool toggle=false){
    e.push_back({id,label,val,min,max,step,unit,toggle});
  };
  switch(tab){
    case 0: // Pantalla
      add(1,"Barra inferior",p.playerFooter,0,1,1,"",true);
      add(2,"Mostrar compás",p.playerMeter,0,1,1,"",true);
      add(3,"Mostrar lector de compases",p.playerReader,0,1,1,"",true);
      add(4,"Mostrar acorde",p.playerChord,0,1,1,"",true);
      add(5,"Mostrar tonalidad",p.playerKey,0,1,1,"",true);
      add(6,"Nombres de teclas",p.playerNames,0,1,1,"",true);
      break;
    case 1: // Transporte
      add(100,"Tempo actual",bpm,20,400,1," BPM");
      add(101,"Restaurar tempo MIDI",0,0,1,1,"",true);
      add(102,"Bucle A-B",loopActive,0,1,1,"",true);
      add(103,"Marcadores de compás",p.playerBarNumbers,0,1,1,"",true);
      add(105,"Reiniciar A-B tras N errores (0 desactivado)",
          p.loopRestartErrors,0,20);
      add(104,"Progreso por colores",p.playerTheme,0,2,1,"");
      break;
    case 2: // Evaluación
      add(120,"Contar teclas incorrectas",p.playerErrors,0,1,1,"",true);
      add(121,"Reconocimiento de acordes",p.playerChord,0,1,1,"",true);
      add(122,"Mostrar tonalidad",p.playerKey,0,1,1,"",true);
      add(123,"Resultados visibles",p.playerFooter,0,1,1,"",true);
      add(124,std::string("Manos a practicar: ")+
          HandPractice::PracticeName(p.practiceHands),p.practiceHands,0,2);
      add(125,"Pentagramas Sol y Fa",p.notationVisible,0,1,1,"",true);
      add(133,"Mostrar notas descendentes (F5)",
          p.fallingNotesVisible,0,1,1,"",true);
      add(134,"Luces MIDI estándar (requiere teclado compatible)",
          p.midiLightGuides,0,1,1,"",true);
      add(135,"Canal de luces MIDI (1-16)",p.midiLightChannel,1,16);
      add(131,"Partitura ampliada",p.notationExpanded,0,1,1,"",true);
      add(132,"Zoom de partitura",p.notationZoom,75,200,5);
      add(126,"Cuantización visual (0 libre, 1 corchea, 2 semicorchea)",
          p.notationQuantize,0,2);
      add(127,"División automática en nota MIDI (60=C4)",
          p.handSplitPoint,48,84);
      add(128,"Corregir manos con clic en pentagrama",
          p.handEditEnabled,0,1,1,"",true);
      add(129,"Herramienta: 0 manos / 1 digitación",
          p.handEditorTool,0,1);
      add(130,"Dedo a asignar (1 pulgar ... 5 meñique)",
          p.fingerToAssign,1,5);
      break;
    case 3: // Modo 2D
      add(200,"Estilo 2D (0 clásico, 1 neón)",p.playerStyle2D,0,1);
      add(201,"Altura del piano",p.player2DHeight,10,30,1,"%");
      add(202,"Anchura de notas",p.player2DWidth,50,110,5,"%");
      add(203,"Radio de esquinas",p.player2DRadius,0,16,1," px");
      add(204,"Opacidad de notas",p.player2DOpacity,10,100,5,"%");
      add(205,"Resplandor de notas",p.player2DGlow,0,100,5,"%");
      add(206,"Brillo de notas",p.player2DBrightness,20,150,5,"%");
      add(207,"Grosor de guías",p.player2DGuideSize,1,4,1," px");
      add(208,"Opacidad de guías",p.player2DGuideOpacity,0,100,5,"%");
      add(209,"Opacidad del teclado",p.player2DKeyOpacity,20,100,5,"%");
      add(210,"Sombras de notas",p.player2DShadows,0,1,1,"",true);
      add(211,"Opacidad del fondo",p.player2DBackgroundOpacity,10,100,5,"%");
      break;
    case 4: // Modo 3D
      add(300,"Estilo 3D (0 clásico, 1 moderno, 2 cinematográfico)",p.playerStyle3D,0,2);
      add(301,"Profundidad de perspectiva",p.player3DDepth,0,100,5,"%");
      add(302,"Inclinación",p.player3DAngle,0,65,1,"°");
      add(303,"Altura del horizonte",p.player3DHorizon,10,60,1,"%");
      add(304,"Anchura del escenario",p.player3DStageWidth,40,120,5,"%");
      add(305,"Altura del piano",p.player3DHeight,10,30,1,"%");
      add(306,"Anchura de notas",p.player3DWidth,50,110,5,"%");
      add(316,"Longitud de notas 3D",p.player3DNoteLength,40,200,10,"%");
      add(307,"Redondeo de notas",p.player3DRadius,0,16,1," px");
      add(308,"Opacidad de notas",p.player3DOpacity,10,100,5,"%");
      add(309,"Fluorescencia",p.player3DGlow,0,100,5,"%");
      add(310,"Brillo de notas",p.player3DBrightness,20,150,5,"%");
      add(311,"Grosor de guías",p.player3DGuideSize,1,4,1," px");
      add(312,"Opacidad de guías",p.player3DGuideOpacity,0,100,5,"%");
      add(313,"Opacidad del teclado",p.player3DKeyOpacity,20,100,5,"%");
      add(314,"Opacidad del escenario",p.player3DBackgroundOpacity,10,100,5,"%");
      add(315,"Sombras de notas",p.player3DShadows,0,1,1,"",true);
      break;
    case 5: // Compás
      add(400,"Mostrar pulsos",p.playerBeatIndicator,0,1,1,"",true);
      add(401,"Modo (0 números, 1 barras)",p.playerBeatMode,0,1);
      add(402,"Números de compás en barra",p.playerBarNumbers,0,1,1,"",true);
      add(403,"Display de métrica",p.playerMeter,0,1,1,"",true);
      add(404,"Lector compás actual / total",p.playerReader,0,1,1,"",true);
      break;
    case 6: // Colores y guías
      add(500,"Paleta del progreso (0-2)",p.playerTheme,0,2);
      add(501,"Guías visibles",p.playerGuides,0,1,1,"",true);
      add(502,"Guías 2D: transparencia",p.player2DGuideOpacity,0,100,5,"%");
      add(503,"Guías 3D: transparencia",p.player3DGuideOpacity,0,100,5,"%");
      add(504,"Resplandor 2D",p.player2DGlow,0,100,5,"%");
      add(505,"Fluorescencia 3D",p.player3DGlow,0,100,5,"%");
      break;
    case 8: // Efectos 3D (separados del modelado y el tempo)
      add(700,"Velocidad del efecto",p.effectSpeed,25,200,5,"%");
      add(701,"Cantidad de partículas",p.effectCount,0,144,12);
      add(702,"Dispersión de partículas",p.effectSpread,0,200,5,"%");
      add(703,"Tamaño de partículas",p.effectSize,25,220,5,"%");
      add(704,std::string("Colores · ")+Effect3DPalette::Name(p.effectPalette),
          p.effectPalette,0,8,1);
      add(705,"Nacimiento (0 clásico, 1 explosión superior)",
          p.effectBirth,0,1,1);
      break;
    case 9: // Escenarios creados en Blender y luces editables
      add(800,std::string("Escenario: ")+StageScenes::Name(StageScenes::ClampPreset(p.scenePreset)),
          p.scenePreset<0?0:p.scenePreset-5,0,7); // user-visible stages 1..7
      add(801,"Potencia de luces",p.sceneLightIntensity,0,100,5,"%");
      add(802,std::string("Color de luces: ")+StageScenes::ColorName(p.sceneLightColor),p.sceneLightColor,0,6);
      add(803,"Movimiento de cámara",p.sceneCameraMotion,0,100,5,"%");
      add(804,"Movimiento del pianista",p.scenePianistMotion,0,100,5,"%");
      add(805,std::string("Vista 3D: ")+StageCamera::ViewName(p.sceneView),
          p.sceneView,0,8,1);
      add(806,"Rotación horizontal",p.sceneYaw,-180,180,5,"°");
      add(807,"Elevación de cámara",p.scenePitch,5,78,1,"°");
      add(808,"Distancia de cámara",p.sceneZoom,180,1250,25," cm");
      add(809,"Enfoque (0 escenario, 1 músico, 2 manos)",
          p.sceneCameraFocus,0,2,1);
      break;
    default: // Avanzado, parámetros efectivos
      add(600,"Duración visible de notas",p.playerNoteDurationMs,500,10000,250," ms");
      add(601,"Nombres de teclas",p.playerNames,0,1,1,"",true);
      add(602,"Guías visibles",p.playerGuides,0,1,1,"",true);
      add(603,"Sombra 2D",p.player2DShadows,0,1,1,"",true);
      add(604,"Sombra 3D",p.player3DShadows,0,1,1,"",true);
      break;
  }
  return e;
}
void SetPlayerSetting(AdvancedSettings& p,int id,int requested){
  const auto set=[&](int& field,int lo,int hi){field=clampInt(requested,lo,hi);};
  const bool flag=requested!=0;
  switch(id){
    case 1: p.playerFooter=flag;break;
    case 2: p.playerMeter=flag;break;
    case 3: p.playerReader=flag;break;
    case 4: p.playerChord=flag;break;
    case 5: p.playerKey=flag;break;
    case 6: p.playerNames=flag;break;
    case 103: p.playerBarNumbers=flag;break;
    case 105: set(p.loopRestartErrors,0,20);break;
    case 104: set(p.playerTheme,0,2);break;
    case 120: p.playerErrors=flag;break;
    case 121: p.playerChord=flag;break;
    case 122: p.playerKey=flag;break;
    case 123: p.playerFooter=flag;break;
    case 124: set(p.practiceHands,0,2);break;
    case 125: p.notationVisible=flag;break;
    case 133: p.fallingNotesVisible=flag;break;
    case 134: p.midiLightGuides=flag;break;
    case 135: set(p.midiLightChannel,1,16);break;
    case 131: p.notationExpanded=flag;break;
    case 132: set(p.notationZoom,75,200);break;
    case 126: set(p.notationQuantize,0,2);break;
    case 127: set(p.handSplitPoint,48,84);break;
    case 128: p.handEditEnabled=flag;break;
    case 129: set(p.handEditorTool,0,1);break;
    case 130: set(p.fingerToAssign,1,5);break;
    case 200: set(p.playerStyle2D,0,1);break;
    case 201: set(p.player2DHeight,10,30);break;
    case 202: set(p.player2DWidth,50,110);break;
    case 203: set(p.player2DRadius,0,16);break;
    case 204: set(p.player2DOpacity,10,100);break;
    case 205: set(p.player2DGlow,0,100);break;
    case 206: set(p.player2DBrightness,20,150);break;
    case 207: set(p.player2DGuideSize,1,4);break;
    case 208: set(p.player2DGuideOpacity,0,100);break;
    case 209: set(p.player2DKeyOpacity,20,100);break;
    case 210: p.player2DShadows=flag;break;
    case 211: set(p.player2DBackgroundOpacity,10,100);break;
    case 300: Apply3DProfile(p,clampInt(requested,0,2));break;
    case 301: set(p.player3DDepth,0,100);break;
    case 302: set(p.player3DAngle,0,65);break;
    case 303: set(p.player3DHorizon,10,60);break;
    case 304: set(p.player3DStageWidth,40,120);break;
    case 305: set(p.player3DHeight,10,30);break;
    case 306: set(p.player3DWidth,50,110);break;
    case 316: set(p.player3DNoteLength,40,200);break;
    case 307: set(p.player3DRadius,0,16);break;
    case 308: set(p.player3DOpacity,10,100);break;
    case 309: set(p.player3DGlow,0,100);break;
    case 310: set(p.player3DBrightness,20,150);break;
    case 311: set(p.player3DGuideSize,1,4);break;
    case 312: set(p.player3DGuideOpacity,0,100);break;
    case 313: set(p.player3DKeyOpacity,20,100);break;
    case 314: set(p.player3DBackgroundOpacity,10,100);break;
    case 315: p.player3DShadows=flag;break;
    case 400: p.playerBeatIndicator=flag;break;
    case 401: set(p.playerBeatMode,0,1);break;
    case 402: p.playerBarNumbers=flag;break;
    case 403: p.playerMeter=flag;break;
    case 404: p.playerReader=flag;break;
    case 500: set(p.playerTheme,0,2);break;
    case 501: p.playerGuides=flag;break;
    case 502: set(p.player2DGuideOpacity,0,100);break;
    case 503: set(p.player3DGuideOpacity,0,100);break;
    case 504: set(p.player2DGlow,0,100);break;
    case 505: set(p.player3DGlow,0,100);break;
    case 600: set(p.playerNoteDurationMs,500,10000);break;
    case 601: p.playerNames=flag;break;
    case 602: p.playerGuides=flag;break;
    case 603: p.player2DShadows=flag;break;
    case 700: set(p.effectSpeed,25,200);break;
    case 701: set(p.effectCount,0,144);break;
    case 702: set(p.effectSpread,0,200);break;
    case 703: set(p.effectSize,25,220);break;
    case 704: set(p.effectPalette,0,8);break;
    case 705: set(p.effectBirth,0,1);break;
    case 800:{
      const int next=requested<=0?-1:StageScenes::ClampPreset(requested+5);
      if(next>=6 && next!=p.scenePreset){
        p.sceneYaw=next==6?88:0;
        p.scenePitch=next==6?28:18;
        p.sceneZoom=next==6?385:(next==12?585:(next==11?435:340));
        p.sceneView=0;p.sceneCameraFocus=0;
      }
      p.scenePreset=next;
      break;
    }
    case 801: set(p.sceneLightIntensity,0,100);break;
    case 802: set(p.sceneLightColor,0,6);break;
    case 803: set(p.sceneCameraMotion,0,100);break;
    case 804: set(p.scenePianistMotion,0,100);break;
    case 805:{
      p.sceneView=clampInt(requested,0,8);
      if(p.sceneView){
        const auto v=StageCamera::Preset(p.sceneView);
        p.sceneYaw=v.yaw;p.scenePitch=v.pitch;p.sceneZoom=v.zoom;
        p.sceneCameraFocus=p.sceneView==8?2:
                           (p.sceneView==7?1:0);
      }
      break;
    }
    case 806: set(p.sceneYaw,-180,180);p.sceneView=0;break;
    case 807: set(p.scenePitch,5,78);p.sceneView=0;break;
    case 808: set(p.sceneZoom,180,1250);p.sceneView=0;break;
    case 809: set(p.sceneCameraFocus,0,2);p.sceneView=0;break;
    case 604: p.player3DShadows=flag;break;
  }
}
} // namespace

void PlayingState::OpenPlayerSettings(){
  if(m_settings_open)return;
  m_settings_before=m_midi_preferences;
  m_settings_view_before=m_view_3d;
  m_settings_tempo_before=m_tempo_factor;
  m_settings_loop_before=m_loop_active;
  m_settings_paused_before=m_paused;
  m_settings_open=true;
  m_settings_feedback="Valores originales";
  m_settings_page=0;
  m_settings_tab=m_midi_preferences.scenePreset>=0?9:(m_view_3d?4:3);
  // Keep playback, its MIDI notes, and the current scene alive.
  const auto bounds=FloatingSettings::Fit(GetStateWidth(),GetStateHeight());
  const auto retained=FloatingSettings::Fit(GetStateWidth(),GetStateHeight(),
                   m_settings_x,m_settings_y);
  m_settings_x=retained.x;
  m_settings_y=retained.y;
  m_settings_dragging=false;
}
void PlayingState::SavePlayerSettings(){
  m_midi_preferences.player3D=m_view_3d;
  m_midi_preferences.playerNoteDurationMs=static_cast<int>(m_show_duration/1000);
  std::string error;
  if(!AdvancedSettingsIO::Save(m_midi_preferences,error)){
    m_settings_feedback="ERROR al guardar: "+error;
    fprintf(stderr,"Linthesia: error al guardar ajustes: %s\n",error.c_str());
    return; // No cerrar el panel si el disco rechaza el guardado.
  }
  // Verificar recuperacion persistente, no dar el guardado por bueno
  // sin poder leer el archivo que se acaba de escribir.
  const AdvancedSettings saved=AdvancedSettingsIO::Load();
  if(saved.player3D!=m_midi_preferences.player3D ||
     saved.player3DDepth!=m_midi_preferences.player3DDepth ||
     saved.player3DNoteLength!=m_midi_preferences.player3DNoteLength ||
     saved.effectSpeed!=m_midi_preferences.effectSpeed ||
     saved.effectCount!=m_midi_preferences.effectCount ||
     saved.effectSpread!=m_midi_preferences.effectSpread ||
     saved.effectSize!=m_midi_preferences.effectSize ||
     saved.effectPalette!=m_midi_preferences.effectPalette ||
     saved.effectBirth!=m_midi_preferences.effectBirth ||
     saved.scenePreset!=m_midi_preferences.scenePreset ||
     saved.sceneLightIntensity!=m_midi_preferences.sceneLightIntensity ||
     saved.sceneLightColor!=m_midi_preferences.sceneLightColor ||
     saved.sceneCameraMotion!=m_midi_preferences.sceneCameraMotion ||
     saved.scenePianistMotion!=m_midi_preferences.scenePianistMotion ||
     saved.sceneYaw!=m_midi_preferences.sceneYaw ||
     saved.scenePitch!=m_midi_preferences.scenePitch ||
     saved.sceneZoom!=m_midi_preferences.sceneZoom ||
     saved.sceneView!=m_midi_preferences.sceneView ||
     saved.sceneCameraFocus!=m_midi_preferences.sceneCameraFocus ||
     saved.playerNoteDurationMs!=m_midi_preferences.playerNoteDurationMs ||
     saved.practiceHands!=m_midi_preferences.practiceHands ||
     saved.notationVisible!=m_midi_preferences.notationVisible ||
     saved.fallingNotesVisible!=m_midi_preferences.fallingNotesVisible ||
     saved.midiLightGuides!=m_midi_preferences.midiLightGuides ||
     saved.midiLightChannel!=m_midi_preferences.midiLightChannel ||
     saved.loopRestartErrors!=m_midi_preferences.loopRestartErrors ||
     saved.notationExpanded!=m_midi_preferences.notationExpanded ||
     saved.notationZoom!=m_midi_preferences.notationZoom ||
     saved.notationQuantize!=m_midi_preferences.notationQuantize ||
     saved.handSplitPoint!=m_midi_preferences.handSplitPoint ||
     saved.handEditEnabled!=m_midi_preferences.handEditEnabled ||
     saved.handEditorTool!=m_midi_preferences.handEditorTool ||
     saved.fingerToAssign!=m_midi_preferences.fingerToAssign){
    m_settings_feedback="Error: los ajustes no se recuperan del disco";
    return;
  }
  m_settings_before=m_midi_preferences;
  m_settings_view_before=m_view_3d;
  m_settings_tempo_before=m_tempo_factor;
  m_settings_loop_before=m_loop_active;
  m_settings_open=false;
  m_settings_feedback.clear();
  // Save configuration without seeking, stopping or resetting any note.
}
void PlayingState::CancelPlayerSettings(){
  const bool practiceChanged=
      m_midi_preferences.practiceHands!=m_settings_before.practiceHands;
  const bool splitChanged=
      m_midi_preferences.handSplitPoint!=m_settings_before.handSplitPoint;
  m_midi_preferences=m_settings_before;
  if(splitChanged)RebuildHandClassification();
  if(practiceChanged||splitChanged){
    m_required_notes.clear();SetupNoteState();
  }
  m_view_3d=m_settings_view_before;
  m_tempo_factor=m_settings_tempo_before;
  m_state.song_speed=static_cast<int>(100.0*m_tempo_factor+0.5);
  m_loop_active=m_settings_loop_before;
  m_show_duration=m_midi_preferences.playerNoteDurationMs*1000LL;
  m_settings_open=false;
  m_settings_feedback.clear();
  // Cancel is an in-memory rollback: it must not rewind the song.
}
void PlayingState::ClosePlayerSettings(){CancelPlayerSettings();}
void PlayingState::ResetPlayerSettingsPage(){
  AdvancedSettings defaults;
  const auto rows=PlayerSettingEntries(defaults,m_settings_tab,
                                        EffectiveTempoBPM(),m_loop_active);
  for(const auto& row:rows){
    if(row.id==100 || row.id==101 || row.id==102)continue;
    SetPlayerSetting(m_midi_preferences,row.id,row.value);
  }
  if(m_settings_tab==2){
    RebuildHandClassification();
    m_required_notes.clear();SetupNoteState();
  }
  if(m_settings_tab==3)m_view_3d=false;
  if(m_settings_tab==4)m_view_3d=true;
  if(m_settings_tab==3 || m_settings_tab==4)
    m_midi_preferences.player3D=m_view_3d;
  m_show_duration=m_midi_preferences.playerNoteDurationMs*1000LL;
  m_settings_feedback="Valores predeterminados previsualizados";
}
bool PlayingState::HandlePlayerSettings(const MouseInfo& mouse){
  if(!m_settings_open)return false;
  auto win=FloatingSettings::Fit(GetStateWidth(),GetStateHeight(),
                                  m_settings_x,m_settings_y);
  const FloatingSettings::Rect videoButton{
    win.x+win.w-168,win.y+9,126,28};
  // The video exporter is reachable from Ajustes during playback, with no
  // extra floating button obscuring the 3D scene. The selection screen has
  // a second access point for users preparing a new song.
  if(mouse.released.left && videoButton.contains(mouse.x,mouse.y)){
    if(m_video.Active())StopVideoExport(false);
    else{
      ClosePlayerSettings();
      OpenVideoDialog();
    }
    return true;
  }
  // A normal title bar drag moves this editor without obscuring the music.
  if(mouse.newPress.left && win.header().contains(mouse.x,mouse.y) &&
     !videoButton.contains(mouse.x,mouse.y) &&
     !win.close().contains(mouse.x,mouse.y)){
    m_settings_dragging=true;
    m_settings_drag_dx=mouse.x-win.x;
    m_settings_drag_dy=mouse.y-win.y;
    return true;
  }
  if(m_settings_dragging){
    if(mouse.held.left){
      const auto target=FloatingSettings::Fit(GetStateWidth(),GetStateHeight(),
                        mouse.x-m_settings_drag_dx,mouse.y-m_settings_drag_dy);
      m_settings_x=target.x;m_settings_y=target.y;
    }
    if(mouse.released.left)m_settings_dragging=false;
    return true;
  }
  if(!win.bounds().contains(mouse.x,mouse.y))return false;
  const auto entries=PlayerSettingEntries(m_midi_preferences,m_settings_tab,
                                          EffectiveTempoBPM(),m_loop_active);
  const int maxPage=entries.empty()?0:static_cast<int>((entries.size()-1)/win.rows);
  if(mouse.released.left && win.close().contains(mouse.x,mouse.y)){
    CancelPlayerSettings();return true;
  }
  if(mouse.released.left){
    for(int i=0;i<10;++i){
      const auto rect=win.tab(i);
      if(rect.contains(mouse.x,mouse.y)){
        m_settings_tab=i;m_settings_page=0;
        if(i==3)m_view_3d=false;
        else if(i==4||i==8||i==9)m_view_3d=true;
        m_settings_feedback="Previsualización sin guardar";
        return true;
      }
    }
  }
  const bool interact=mouse.released.left || mouse.held.left ||
                      mouse.newPress.left;
  if(interact){
    for(int row=0;row<win.rows;++row){
      const int index=m_settings_page*win.rows+row;
      if(index>=static_cast<int>(entries.size()))break;
      const auto& e=entries[index];
      if(!win.entry(row).contains(mouse.x,mouse.y))continue;
      int value=e.value;bool changed=false;
      if(mouse.released.left && win.minus(row).contains(mouse.x,mouse.y)){
        value-=e.step;changed=true;
      }else if(mouse.released.left && win.plus(row).contains(mouse.x,mouse.y)){
        value+=e.step;changed=true;
      }else if(e.toggle && mouse.released.left &&
               win.slider(row).contains(mouse.x,mouse.y)){
        value=!e.value;changed=true;
      }else if(!e.toggle &&
               (mouse.newPress.left || mouse.held.left || mouse.released.left) &&
               win.slider(row).contains(mouse.x,mouse.y)){
        const auto r=win.slider(row);
        const double fraction=clampInt(mouse.x-r.x,0,r.w)/double(r.w);
        value=e.minimum+static_cast<int>(
          ((e.maximum-e.minimum)*fraction)/e.step+0.5)*e.step;
        changed=true;
      }
      if(!changed)continue;
      value=clampInt(value,e.minimum,e.maximum);
      if(e.id==100)AdjustTempoBPM(value-EffectiveTempoBPM());
      else if(e.id==101)ResetTempoBPM();
      else if(e.id==102)m_loop_active=m_loop_defined && !m_loop_active;
      else {
        SetPlayerSetting(m_midi_preferences,e.id,value);
        if(e.id==127)RebuildHandClassification();
        if(e.id==124||e.id==127){
          m_required_notes.clear();SetupNoteState();
        }
        m_show_duration=m_midi_preferences.playerNoteDurationMs*1000LL;
      }
      m_settings_feedback="Vista previa en directo · Sin guardar";
      return true;
    }
  }
  if(!mouse.released.left)return true;
  if(win.prev().contains(mouse.x,mouse.y)){
    m_settings_page=std::max(0,m_settings_page-1);return true;
  }
  if(win.next().contains(mouse.x,mouse.y)){
    m_settings_page=std::min(maxPage,m_settings_page+1);return true;
  }
  if(win.action(0).contains(mouse.x,mouse.y)){
    ResetPlayerSettingsPage();return true;
  }
  if(win.action(1).contains(mouse.x,mouse.y)){
    CancelPlayerSettings();return true;
  }
  if(win.action(2).contains(mouse.x,mouse.y)){
    m_settings_feedback="Vista previa activa · Guardar para conservar";
    return true;
  }
  if(win.action(3).contains(mouse.x,mouse.y)){
    SavePlayerSettings();return true;
  }
  return true;
}
void PlayingState::DrawPlayerSettings(Renderer& renderer) const{
  const auto win=FloatingSettings::Fit(GetStateWidth(),GetStateHeight(),
                                       m_settings_x,m_settings_y);
  const auto entries=PlayerSettingEntries(m_midi_preferences,m_settings_tab,
                                          EffectiveTempoBPM(),m_loop_active);
  const int maxPage=entries.empty()?0:static_cast<int>((entries.size()-1)/win.rows);
  const auto text=[&](int x,int y,const std::string& value,int size,SDL_Color color){
    renderer.SetColor(255,255,255);
    TextWriter writer(x,y,renderer,false,size);writer<<Text(value,color);
  };
  // This is a movable window, NOT a full-screen dark modal overlay.
  renderer.ForceTexture(0);
  renderer.SetColor(5,15,26,216);
  renderer.DrawQuad(win.x-4,win.y-4,win.w+8,win.h+8);
  renderer.SetColor(16,38,53,238);
  renderer.DrawQuad(win.x,win.y,win.w,win.h);
  renderer.SetColor(41,154,183,245);
  renderer.DrawQuad(win.x,win.y,win.w,3);
  renderer.SetColor(23,72,97,240);
  renderer.DrawQuad(win.x+1,win.y+4,win.w-2,41);
  text(win.x+13,win.y+12,"AJUSTES · MOVER",14,
       Renderer::ToColor(244,248,251));
  text(win.x+15,win.y+36,"La canción sigue sonando · Vista previa inmediata",10,
       Renderer::ToColor(165,213,227));
  // Video is now an editor action, not a permanent performance HUD.
  const int vbX=win.x+win.w-168,vbY=win.y+9;
  const bool exporting=m_video.Active();
  const MouseInfo vbMouse=Mouse();
  const bool vbHover=vbMouse.x>=vbX && vbMouse.x<vbX+126 &&
                     vbMouse.y>=vbY && vbMouse.y<vbY+28;
  renderer.SetColor(exporting?(vbHover?151:115):(vbHover?37:28),
                    exporting?(vbHover?84:59):(vbHover?133:96),
                    exporting?(vbHover?69:49):(vbHover?151:124));
  renderer.DrawQuad(vbX,vbY,126,28);
  renderer.SetColor(97,210,218);
  renderer.DrawQuad(vbX,vbY,126,2);
  text(vbX+8,vbY+7,exporting?"Finalizar vídeo":"Exportar vídeo",12,
       Renderer::ToColor(248,251,250));
  renderer.SetColor(110,62,69);
  const auto cross=win.close();renderer.DrawQuad(cross.x,cross.y,cross.w,cross.h);
  text(cross.x+7,cross.y+5,"×",16,Renderer::ToColor(255,249,249));
  const std::string tabs[]={"Pantalla","Transporte","Evaluación","2D",
    "3D","Compás","Colores","Avanzado","Efectos 3D","Escenarios"};
  for(int tab=0;tab<10;++tab){
    auto r=win.tab(tab);
    renderer.SetColor(tab==m_settings_tab?39:27,
                      tab==m_settings_tab?119:61,
                      tab==m_settings_tab?143:80,245);
    renderer.DrawQuad(r.x,r.y,r.w,r.h);
    text(r.x+5,r.y+10,tabs[tab],tab==8?10:11,
         Renderer::ToColor(232,246,253));
  }
  for(int row=0;row<win.rows;++row){
    const int index=m_settings_page*win.rows+row;
    if(index>=static_cast<int>(entries.size()))break;
    const auto& e=entries[index];
    const auto r=win.entry(row);
    renderer.SetColor(row%2?26:22,row%2?53:48,67,246);
    renderer.DrawQuad(r.x,r.y,r.w,r.h);
    // Labels sit above values and no longer overlap the controls.
    text(r.x+10,r.y+5,e.title,10,Renderer::ToColor(227,241,247));
    const auto slider=win.slider(row);
    renderer.SetColor(32,78,98,232);
    renderer.DrawQuad(slider.x,slider.y+2,slider.w,31);
    if(!e.toggle){
      const double fraction=(e.value-e.minimum)/
        double(std::max(1,e.maximum-e.minimum));
      renderer.SetColor(51,173,175);
      renderer.DrawQuad(slider.x+2,slider.y+25,
        static_cast<int>((slider.w-4)*fraction),4);
    }
    text(slider.x+6,slider.y+7,e.toggle?(e.value?"Sí":"No"):
         std::to_string(e.value)+e.unit,11,Renderer::ToColor(221,249,231));
    for(int button=0;button<2;++button){
      const auto br=button?win.plus(row):win.minus(row);
      renderer.SetColor(43,110,124);
      renderer.DrawQuad(br.x,br.y,br.w,br.h);
      text(br.x+12,br.y+7,button?"+":"−",16,
           Renderer::ToColor(247,251,252));
    }
  }
  const auto simple=[&](FloatingSettings::Rect r,const std::string& title,bool hot=false){
    renderer.SetColor(hot?34:34,hot?130:78,hot?134:98,249);
    renderer.DrawQuad(r.x,r.y,r.w,r.h);
    text(r.x+9,r.y+9,title,11,Renderer::ToColor(234,249,250));
  };
  simple(win.prev(),"Anterior");
  text(win.x+win.w/2-35,win.navY()+8,
       STRING("Página "<<m_settings_page+1<<"/"<<maxPage+1),11,
       Renderer::ToColor(189,222,231));
  simple(win.next(),"Siguiente");
  simple(win.action(0),"Restaurar");
  simple(win.action(1),"Cancelar");
  simple(win.action(2),"Aplicar");
  simple(win.action(3),"Guardar",true);
  // Feedback is an unobtrusive status line, and does not overlap buttons.
  text(win.x+13,win.y+win.h-110,
       m_settings_feedback.empty()?"Cambios temporales hasta Guardar":m_settings_feedback,
       10,Renderer::ToColor(171,216,225));
}

void PlayingState::DrawLoopEditor(Renderer &renderer) const {
  const int width=GetStateWidth(),height=GetStateHeight();
  const int left=std::max(10,(width-460)/2);
  // Ventana translúcida encima del roll; deja la escala A-B visible y activa.
  renderer.SetColor(5,16,25,106);
  renderer.DrawQuad(0,96,width,height-96);
  renderer.SetColor(13,43,56,214);
  renderer.DrawQuad(left,112,460,145);
  renderer.SetColor(100,192,193,185);
  renderer.DrawQuad(left,112,460,2);
  renderer.SetColor(255,255,255);
  TextWriter title(left+230,126,renderer,true,19);
  title<<Text("Seleccionar bucle A-B",Renderer::ToColor(247,251,249));
  renderer.SetColor(255,255,255);
  TextWriter info(left+230,159,renderer,true,13);
  info<<Text("Arrastra en la barra, de izquierda a derecha o al revés",
              Renderer::ToColor(204,235,234));
  const auto button=[&](int x,const std::string& caption,bool primary){
    const MouseInfo mouse=Mouse();
    const bool hover=mouse.x>=x && mouse.x<x+102 &&
                     mouse.y>=202 && mouse.y<=236;
    renderer.SetColor(primary?(hover?45:32):(hover?76:47),
                      primary?(hover?149:105):(hover?103:81),
                      primary?(hover?128:95):(hover?104:98),231);
    renderer.DrawQuad(x,202,102,34);
    renderer.SetColor(247,250,249);
    TextWriter t(x+51,212,renderer,true,14);
    t<<Text(caption,Renderer::ToColor(242,250,247));
  };
  button(left+112,"Borrar",false);
  button(left+246,"Aplicar",true);
  if(m_loop_prompt){
    renderer.SetColor(255,255,255);
    TextWriter warn(left+230,243,renderer,true,11);
    warn<<Text("Selecciona al menos 0,1 segundos de la barra",
               Renderer::ToColor(255,202,166));
  }
}

void PlayingState::DrawTransport(Renderer &renderer) const {
  const int width=GetStateWidth(),height=GetStateHeight();
  const auto& midi=*m_state.midi;
  const microseconds_t songLength=midi.GetSongLengthInMicroseconds();
  const microseconds_t offset=midi.GetDeadAirStartOffsetMicroseconds();
  const microseconds_t actualTime=midi.GetSongPositionInMicroseconds();
  const microseconds_t position=std::max<microseconds_t>(0,actualTime-offset);
  const double progress=m_scrubbing?m_scrub_fraction:
      std::max(0.0,std::min(1.0,midi.GetSongPercentageComplete()));
  const microseconds_t preview=m_scrubbing?
      offset+static_cast<microseconds_t>(songLength*progress):actualTime;
  const MouseInfo mouse=Mouse();
  const int barX=58,barW=std::max(90,width-116);

  // Una sola fila: transporte, tempo con flechas verticales, lector,
  // fallos incorrectos, acorde, tonalidad, bucle, 3D y ajustes.
  renderer.SetColor(7,35,56);
  renderer.DrawQuad(0,0,width,55);
  renderer.SetColor(20,79,116);
  renderer.DrawQuad(0,0,width,4);
  renderer.SetColor(77,143,166);
  renderer.DrawQuad(0,53,width,2);
  auto control=[&](int id,const std::string& caption,bool enabled=false){
    const RibbonBox rect=Ribbon(id,width);
    const bool hover=rect.Contains(mouse);
    const bool down=hover&&mouse.held.left;
    renderer.SetColor(enabled?16:(down?8:(hover?41:26)),
        enabled?101:(down?72:(hover?118:86)),
        enabled?99:(down?107:(hover?155:118)),255);
    renderer.DrawQuad(rect.x,rect.y+(down?1:0),rect.w,rect.h-(down?1:0));
    renderer.SetColor(enabled?68:72,enabled?234:147,enabled?148:169);
    renderer.DrawQuad(rect.x,rect.y+(down?1:0),rect.w,2);
    renderer.SetColor(255,255,255);
    TextWriter label(rect.x+rect.w/2,
        rect.y+rect.h/2-6+(down?1:0),renderer,true,
        RibbonTextSize(width));
    label<<Text(caption,Renderer::ToColor(242,250,252));
  };
  auto lcd=[&](int id,const std::string& label,
               const std::string& value,bool visible=true){
    const RibbonBox rect=Ribbon(id,width);
    renderer.SetColor(5,27,43);
    renderer.DrawQuad(rect.x,rect.y,rect.w,rect.h);
    renderer.SetColor(47,105,133);
    renderer.DrawQuad(rect.x,rect.y,rect.w,1);
    if(visible){
      renderer.SetColor(255,255,255);
      TextWriter t(rect.x+5,rect.y+3,renderer,false,RibbonTextSize(width)-2);
      t<<Text(label,Renderer::ToColor(167,210,222));
      TextWriter v(rect.x+5,rect.y+18,renderer,false,RibbonTextSize(width,true));
      v<<Text(value,Renderer::ToColor(244,250,247));
    }
  };
  control(17,"Inicio");
  control(0,"Volver");
  control(1,"Ayuda");
  control(2,"◀");
  control(3,m_paused?"▶":"Ⅱ");
  control(4,"▶");
  const RibbonBox tempo=Ribbon(5,width);
  renderer.SetColor(5,27,43);
  renderer.DrawQuad(tempo.x,tempo.y,tempo.w,tempo.h);
  renderer.SetColor(48,109,130);
  renderer.DrawQuad(tempo.x,tempo.y,tempo.w,1);
  renderer.SetColor(255,255,255);
  TextWriter tempoLabel(tempo.x+5,tempo.y+3,renderer,false,
                        RibbonTextSize(width)-2);
  tempoLabel<<Text("BPM",Renderer::ToColor(166,215,224));
  TextWriter tempoNumber(tempo.x+6,tempo.y+18,renderer,false,
                        RibbonTextSize(width,true));
  tempoNumber<<Text(STRING(EffectiveTempoBPM()),Renderer::ToColor(238,251,247));
  control(6,"▲");
  control(7,"▼");
  control(8,"REST");
  const auto& bars=midi.GetMusicalBars();
  unsigned int totalBars=bars.empty()?0:bars.back().number;
  const std::string musicalPosition=midi.GetMusicalPosition(preview);
  const size_t separator=musicalPosition.find(':');
  const std::string currentBar=separator==std::string::npos?"—":
                               musicalPosition.substr(0,separator);
  // Indicador cíclico de tiempos SOBRE la métrica: 1..N.
  // Se basa en la posición musical (pulsos MIDI) y no en el reloj UI.
  const RibbonBox meterBox=Ribbon(9,width);
  renderer.SetColor(5,27,43);
  renderer.DrawQuad(meterBox.x,meterBox.y,meterBox.w,meterBox.h);
  renderer.SetColor(47,105,133);
  renderer.DrawQuad(meterBox.x,meterBox.y,meterBox.w,1);
  // The opening meter already applies during the count-in (negative
  // preview time), otherwise a 3/4 song incorrectly flashes 4/4.
  unsigned int meterNumerator=bars.empty()?4:bars.front().numerator;
  unsigned int meterDenominator=bars.empty()?4:bars.front().denominator;
  for(const auto& bar:bars){
    if(bar.time>preview)break;
    meterNumerator=bar.numerator;
    meterDenominator=bar.denominator;
  }
  const size_t beatColon=musicalPosition.find(':');
  const unsigned int currentBeat=beatColon==std::string::npos ? 1U :
    std::max(1,std::atoi(musicalPosition.c_str()+beatColon+1));
  if(m_midi_preferences.playerMeter){
    renderer.SetColor(255,255,255);
    if(m_midi_preferences.playerBeatIndicator){
      const unsigned int n=std::min(32U,std::max(1U,meterNumerator));
      const int usable=std::max(12,meterBox.w-8);
      if(m_midi_preferences.playerBeatMode==0 && n<=12){
        for(unsigned int i=0;i<n;i++){
          const int x=meterBox.x+4+static_cast<int>((i+.5)*usable/n);
          const bool active=i+1==currentBeat;
          renderer.SetColor(255,255,255);
          TextWriter beat(x,meterBox.y+2,renderer,true,width<900?8:10);
          beat<<Text(std::to_string(i+1),
               i==0?Renderer::ToColor(active?255:196,83,79):
                    Renderer::ToColor(89,active?251:156,130));
        }
      }else{
        for(unsigned int i=0;i<n;i++){
          const int x=meterBox.x+4+static_cast<int>(i*usable/n);
          const int next=meterBox.x+4+static_cast<int>((i+1)*usable/n);
          renderer.SetColor(i==0?218:70,i==0?58:170,i==0?51:107);
          renderer.DrawQuad(x,meterBox.y+5,std::max(1,next-x-1),
                            (i+1==currentBeat)?9:5);
        }
      }
    }
    renderer.SetColor(255,255,255);
    TextWriter metric(meterBox.x+meterBox.w/2,meterBox.y+19,renderer,true,
                      RibbonTextSize(width,true));
    metric<<Text(STRING(meterNumerator<<"/"<<meterDenominator),
                 Renderer::ToColor(238,248,244));
  }
  lcd(10,"Lector",STRING(currentBar<<"/"<<totalBars),
      m_midi_preferences.playerReader);
  // Fallos = solamente teclas mal pulsadas, no notas omitidas.
  lcd(11,"Fallos",STRING(m_state.stats.stray_notes),
      m_midi_preferences.playerErrors);
  lcd(12,"Acorde",m_current_chord,m_midi_preferences.playerChord);
  lcd(13,"Tonalidad",midi.GetCurrentKeySignature(),
      m_midi_preferences.playerKey);
  control(14,"Bucle",m_loop_active);
  control(15,m_view_3d?"2D":"3D");
  // Rueda dentada vectorial: el icono no depende de archivos gráficos.
  control(16,"",m_settings_open);
  const RibbonBox gear=Ribbon(16,width);
  const int gcx=gear.x+(width>950?17:gear.w/2);
  const int gcy=gear.y+gear.h/2;
  renderer.SetColor(218,242,247);
  renderer.DrawQuad(gcx-4,gcy-12,8,24);
  renderer.DrawQuad(gcx-12,gcy-4,24,8);
  renderer.DrawQuad(gcx-8,gcy-8,16,16);
  renderer.SetColor(19,70,97);
  renderer.DrawQuad(gcx-4,gcy-4,8,8);
  if(width>950){
    renderer.SetColor(255,255,255);
    TextWriter settingLabel(gear.x+32,gear.y+10,renderer,false,12);
    settingLabel<<Text("Ajustes",Renderer::ToColor(244,250,252));
  }

  // Escala musical: lectura de cada compás desde los pulsos y los cambios
  // de métrica MIDI. Las etiquetas se separan adaptativamente por píxeles.
  renderer.SetColor(17,33,43);
  renderer.DrawQuad(0,56,width,38);
  renderer.SetColor(33,58,62);
  renderer.DrawQuad(barX,77,barW,12);
  int lastLabel=-1000;
  int lastTick=-1000;
  for(const auto& bar:bars){
    if(songLength<=0)break;
    const double f=(bar.time-offset)/static_cast<double>(songLength);
    if(f<0 || f>1)continue;
    const int bx=barX+static_cast<int>(f*barW);
    if(bx-lastTick>=3){
      renderer.SetColor(77,113,121);
      renderer.DrawQuad(bx,67,1,22);
      lastTick=bx;
    }
    if(m_midi_preferences.playerBarNumbers && bx-lastLabel>=62){
      renderer.SetColor(217,230,234);
      TextWriter mark(bx+2,58,renderer,false,11);
      mark<<Text(std::to_string(bar.number),
                  Renderer::ToColor(181,210,214));
      lastLabel=bx;
    }
  }
  // Persistent custom markers are distinct from the single A-B loop.
  // Their actual MIDI timestamps remain stable under tempo changes.
  for(size_t i=0;i<m_song_bookmarks.size();++i){
    const auto& mark=m_song_bookmarks[i];
    if(mark.time<offset||mark.time>offset+songLength||songLength<=0)continue;
    const int x=barX+int(double(mark.time-offset)/songLength*barW);
    renderer.SetColor(255,215,116);
    renderer.DrawQuad(x-2,76,4,14);
    renderer.SetColor(255,255,255);
    TextWriter label(x+3,59,renderer,false,10);
    label<<Text(mark.name,Renderer::ToColor(250,213,135));
  }
  const int filled=std::max(0,std::min(barW,
      static_cast<int>(progress*barW)));
  if(filled>0){
    static const int progressColors[][3]={{28,137,65},{31,134,184},{174,124,33}};
    const int theme=m_midi_preferences.playerTheme;
    renderer.SetColor(progressColors[theme][0],progressColors[theme][1],
                      progressColors[theme][2]);
    renderer.DrawQuad(barX,78,filled,10);
    renderer.SetColor(114,218,188);
    renderer.DrawQuad(barX,78,filled,2);
  }
  // A-B conservado en la barra incluso al desactivar el bucle.
  // El rango del editor se actualiza continuamente al arrastrar.
  if(m_loop_defined || m_loop_editor){
    const microseconds_t base=midi.GetDeadAirStartOffsetMicroseconds();
    const microseconds_t duration=std::max<microseconds_t>(
        1,midi.GetSongLengthInMicroseconds());
    const double from=m_loop_editor?m_loop_draft_start:
        (m_loop_start-base)/static_cast<double>(duration);
    const double to=m_loop_editor?m_loop_draft_end:
        (m_loop_end-base)/static_cast<double>(duration);
    if(m_loop_defined || m_loop_draft_valid || m_loop_dragging){
      const int xa=barX+static_cast<int>(std::min(from,to)*barW);
      const int xb=barX+static_cast<int>(std::max(from,to)*barW);
      if(xb>xa){
        renderer.SetColor(m_loop_active?67:154,214,119,132);
        renderer.DrawQuad(xa,77,xb-xa,12);
      }
      renderer.SetColor(246,229,137);
      renderer.DrawQuad(xa-2,63,4,28);
      renderer.DrawQuad(xb-2,63,4,28);
    }
  }
  const int cursor=barX+filled;
  renderer.SetColor(238,250,254);
  renderer.DrawQuad(cursor-2,65,4,27);
  renderer.SetColor(255,255,255);
  TextWriter t1(6,73,renderer,false,12);
  t1<<Text(ClockText(std::max<microseconds_t>(0,preview-offset)),
       Renderer::ToColor(236,246,247));
  TextWriter t2(width-47,73,renderer,false,12);
  t2<<Text(ClockText(songLength),
       Renderer::ToColor(236,246,247));

  const int effectiveBpm=EffectiveTempoBPM();
  const int originalBpm=midi.GetCurrentBPM();
  // Pie configurable desde Ajustes, sin duplicar volumen ni omisiones.
  if(m_midi_preferences.playerFooter){
  const int bottom=height-34;
  renderer.SetColor(23,31,39);
  renderer.DrawQuad(0,bottom,width,34);
  renderer.SetColor(65,87,97);
  renderer.DrawQuad(0,bottom,width,1);
  renderer.SetColor(241,250,255);
  TextWriter score(12,bottom+10,renderer,false,12);
  score<<Text(STRING("Puntos "<<static_cast<int>(m_state.stats.score)
            <<"  |  Factor x"<<fixed<<setprecision(1)<<CalculateScoreMultiplier()),
            Renderer::ToColor(222,235,244));
  if(width>650){
    TextWriter time(280,bottom+10,renderer,false,12);
    time<<Text(STRING(ClockText(position)<<" / "<<ClockText(songLength)
             <<"  ("<<static_cast<int>(progress*100+0.5)<<"%)"
             <<"   Tempo "<<effectiveBpm<<" BPM (MIDI "<<originalBpm<<")"),
             Renderer::ToColor(170,224,217));
  }
  if(width>940){
    renderer.SetColor(255,255,255);
    std::string transportStatus=m_paused?"En pausa":"Reproduciendo";
    if(!m_paused && !m_required_notes.empty()){
      transportStatus="Esperando tecla";
      if(m_required_notes.size()!=1)transportStatus+="s";
      transportStatus+=" ";
      int shown=0;
      for(int id:m_required_notes){
        if(shown++>=3){transportStatus+="...";break;}
        if(shown>1)transportStatus+="+";
        transportStatus+=MidiEvent::NoteName(id);
      }
    }
    TextWriter extra(680,bottom+10,renderer,false,12);
    extra<<Text(STRING("Pulsaciones erróneas: "<<m_state.stats.stray_notes
               <<"   Vista "<<(m_view_3d?"3D":"2D")<<"   "
               <<transportStatus),
               Renderer::ToColor(212,229,237));
  }
  } // playerFooter
  if(m_show_help){
    const int hw=std::min(width-30,660),hx=(width-hw)/2;
    renderer.SetColor(8,37,56,247);
    renderer.DrawQuad(hx,99,hw,80);
    renderer.SetColor(70,151,171);
    renderer.DrawQuad(hx,99,hw,2);
    renderer.SetColor(255,255,255);
    TextWriter help(hx+16,117,renderer,false,14);
    help<<Text("Espacio: pausa   Izquierda/derecha: tempo BPM   Arriba/abajo: zoom",
               Renderer::ToColor(232,246,249));
    TextWriter help2(hx+16,147,renderer,false,13);
    help2<<Text("F2 partitura · F3 zoom · F5 notas · F6 canción completa · F7/8/9 marcadores",
                Renderer::ToColor(207,231,239));
  }
  if(m_loop_editor)DrawLoopEditor(renderer);
  if(m_settings_open)DrawPlayerSettings(renderer);
  // The performance scene deliberately ends here: no selection-hand,
  // notation or whole-song buttons overlay the falling note graphics.

}

void PlayingState::LoadMusicXmlMarks(){
  m_xml_marks.clear();
  if(m_state.musicxml_score_path.empty())return;
  std::ifstream in(m_state.musicxml_score_path);
  std::string magic;
  if(!std::getline(in,magic)||magic!="LINTHESIA_MUSICXML_SCORE_1")return;
  std::string line;
  for(int i=0;i<200000 && std::getline(in,line);++i){
    if(line.size()>200)continue;
    std::istringstream row(line);
    int channel=-1,pitch=-1;
    long long position=-1;
    std::string tags;
    if(!(row>>channel>>pitch>>position>>tags))continue;
    if(channel<0||channel>15||pitch<0||pitch>127||position<0||
       tags.size()>128)continue;
    m_xml_marks[std::make_tuple(channel,pitch,
                                static_cast<microseconds_t>(position))]=tags;
  }
}
std::string PlayingState::MarkForNote(const TranslatedNote& note) const{
  const auto key=std::make_tuple(int(note.channel),int(note.note_id),note.start);
  auto found=m_xml_marks.find(key);
  if(found!=m_xml_marks.end())return found->second;
  // MIDI time conversion may differ by 1-2 us in tempo rounding.
  for(int off=-3;off<=3;++off){
    auto near=m_xml_marks.find(std::make_tuple(int(note.channel),
                int(note.note_id),note.start+off));
    if(near!=m_xml_marks.end())return near->second;
  }
  return "";
}

bool PlayingState::SaveHandAnnotations(){
  return HandCorrectionStore::SaveAll(m_state.song_path,
       m_hand_classifier.Corrections(),m_hand_classifier.Fingerings());
}

FullSongPlayback::Result PlayingState::AudioTrackCount() const{
  std::vector<bool> notes,drums;
  for(const auto& track:m_state.midi->Tracks()){
    notes.push_back(!track.Notes().empty());
    drums.push_back(track.IsPercussion());
  }
  return FullSongPlayback::Count(m_state.track_properties,notes,drums);
}
void PlayingState::RestorePracticeTrackModes(){
  if(!m_full_song_audio)return;
  m_state.track_properties=m_saved_practice_track_modes;
  m_saved_practice_track_modes.clear();
  m_full_song_audio=false;
}
void PlayingState::ToggleFullSongAudio(){
  if(!m_state.midi)return;
  const auto target=m_state.midi->GetSongPositionInMicroseconds();
  if(m_full_song_audio){
    RestorePracticeTrackModes();
  }else{
    m_saved_practice_track_modes=m_state.track_properties;
    std::vector<bool> notes,drums;
    for(const auto& track:m_state.midi->Tracks()){
      notes.push_back(!track.Notes().empty());
      drums.push_back(track.IsPercussion());
    }
    FullSongPlayback::EnableAll(m_state.track_properties,notes,drums);
    m_full_song_audio=true;
  }
  // Reconstruct and re-prime the synth so disabled tracks do not leave
  // stale notes/controllers. Source MIDI and saved track options untouched.
  SeekTo(target);
  RefreshScoreTracks();
}
void PlayingState::RefreshScoreTracks(){
  std::vector<PianoScoreTracks::Candidate> tracks;
  const auto& source=m_state.midi->Tracks();
  tracks.reserve(source.size());
  for(size_t i=0;i<source.size();++i){
    const auto mode=i<m_state.track_properties.size()?
       m_state.track_properties[i].mode:Track::ModeNotPlayed;
    const bool visible=mode!=Track::ModeNotPlayed &&
                       mode!=Track::ModePlayedButHidden;
    const bool user=mode==Track::ModeYouPlay ||
       mode==Track::ModeYouPlaySilently ||
       mode==Track::ModeLearning ||
       mode==Track::ModeLearningSilently;
    const auto name=source[i].InstrumentName();
    const bool piano=name.find("Piano")!=std::string::npos;
    tracks.push_back({i,visible,user,piano,source[i].IsPercussion(),
                      source[i].Notes().size()});
  }
  m_score_tracks=PianoScoreTracks::Choose(tracks);
}

void PlayingState::RebuildHandClassification(){
  RefreshScoreTracks();
  // Reconstruct using the complete immutable set; the rolling onscreen
  // collection may contain evaluated notes or only a part of the song.
  m_hand_classifier.Build(m_state.midi->Notes(),
                          m_midi_preferences.handSplitPoint);
  const auto annotations=HandCorrectionStore::LoadAll(m_state.song_path);
  for(const auto &correction:annotations.hands)
    m_hand_classifier.SetCorrection(correction.track,correction.pitch,
                                     correction.when,correction.hand);
  for(const auto &fingering:annotations.fingers)
    m_hand_classifier.SetFinger(fingering.track,fingering.pitch,
                                fingering.when,fingering.finger);
}

PlayingState::StaffViewport PlayingState::GetStaffViewport() const {
  StaffViewport g;
  g.expanded=m_midi_preferences.notationExpanded;
  if(g.expanded){
    g.left=20;g.top=128;
    g.width=std::max(620,GetStateWidth()-40);
    g.height=std::max(340,GetStateHeight()-250);
    g.systems=g.height>=430?2:1;
    g.systemDelta=g.systems==2?g.height/2:0;
    g.step=std::max(5,std::min(11,6*m_midi_preferences.notationZoom/100));
    g.lineStart=g.left+142;
    g.staffEnd=g.left+g.width-25;
    g.bottom[0]=g.top+g.height*(g.systems==2?17:33)/100;
    g.bottom[1]=g.top+g.height*(g.systems==2?37:67)/100;
    g.horizon=8000000LL*100/m_midi_preferences.notationZoom;
  }else{
    // One readable system at the upper right of the stage.
    // The selected pianist remains visible on the left.
    g.width=std::min(870,std::max(590,GetStateWidth()*54/100));
    g.width=std::min(g.width,GetStateWidth()-28);
    g.left=GetStateWidth()-g.width-16;
    g.top=146;
    g.height=250;
    g.step=7;
    g.systems=1;g.systemDelta=0;
    g.lineStart=g.left+125;
    g.staffEnd=g.left+g.width-18;
    g.bottom[0]=g.top+97;
    g.bottom[1]=g.top+199;
    g.horizon=3000000LL*100/
        std::max(100,m_midi_preferences.notationZoom);
  }
  return g;
}

PlayingState::StaffRenderData PlayingState::PrepareStaffRender(
    microseconds_t now,microseconds_t horizon) const {
  StaffRenderData view;
  std::vector<StaffEngraving::Note> scoreNotes;
  // Preserve exact pointers into the existing translated MIDI note set;
  // annotations and audio continue to use original onset and track ID.
  for(const auto& n:m_notes){
    // TranslatedNoteSet is ordered by onset; stop once notes exceed
    // the visible score horizon, even on very long arrangements.
    if(n.start>now+horizon)break;
    if(n.note_id<21||n.note_id>108||n.channel==9||
       n.track_id>=m_state.track_properties.size()||
       (!m_score_tracks.empty() &&
        m_score_tracks.count(n.track_id)==0))continue;
    const auto mode=m_state.track_properties[n.track_id].mode;
    if(mode==Track::ModeNotPlayed||mode==Track::ModePlayedButHidden)continue;
    if(n.end<=now||n.start>now+horizon)continue;
    if(!m_hand_classifier.Selected(n,m_midi_preferences.practiceHands))
      continue;
    const int hand=m_hand_classifier.ForNote(n)==HandPractice::Right?0:1;
    scoreNotes.push_back({int(n.note_id),hand,n.start,n.end,
                          view.source.size()});
    view.source.push_back(&n);
    if(scoreNotes.size()>=180)break; // prevents pathological render spikes
  }
  std::vector<StaffEngraving::Bar> bars;
  for(const auto& b:m_state.midi->GetMusicalBars()){
    if(b.time>now+horizon)break;
    bars.push_back({b.time,int(b.number)});
  }
  view.layout=StaffEngraving::Build(scoreNotes,std::move(bars),
          now,now+horizon,std::max(30,EffectiveTempoBPM()));
  return view;
}

bool PlayingState::HandleStaffHandEditor(const MouseInfo& mouse){
  if(m_settings_open || m_loop_editor || !m_midi_preferences.notationVisible ||
     (!mouse.released.left&&!mouse.released.right))return false;
  const auto view=GetStaffViewport();
  const int left=view.left,top=view.top,width=view.width;
  if(mouse.x<left || mouse.x>left+width ||
     mouse.y<top || mouse.y>top+view.height)return false;
  // Buttons work even when manual hand editing is off.
  if(mouse.released.left && mouse.y>=top+14 && mouse.y<=top+42){
    const int offset=mouse.x-left;
    if(offset>=width-112&&offset<=width-8){
      m_midi_preferences.notationExpanded=!m_midi_preferences.notationExpanded;
    }else if(offset>=width-205&&offset<=width-177){
      m_midi_preferences.notationZoom=std::max(75,m_midi_preferences.notationZoom-25);
    }else if(offset>=width-170&&offset<=width-142){
      m_midi_preferences.notationZoom=std::min(200,m_midi_preferences.notationZoom+25);
    }else if(view.expanded && offset>=width-510 && offset<=width-477){
      const auto base=m_staff_page_start<0?
         std::max<microseconds_t>(0,m_state.midi->GetSongPositionInMicroseconds()):
         m_staff_page_start;
      m_staff_page_start=std::max<microseconds_t>(0,base-view.horizon);
    }else if(view.expanded && offset>=width-467 && offset<=width-434){
      const auto base=m_staff_page_start<0?
         std::max<microseconds_t>(0,m_state.midi->GetSongPositionInMicroseconds()):
         m_staff_page_start;
      const auto end=m_state.midi->GetSongLengthInMicroseconds();
      m_staff_page_start=std::min<microseconds_t>(end,base+view.horizon);
    }else if(view.expanded && offset>=width-425 && offset<=width-373){
      m_staff_page_start=-1; // follow the current playback position again
    }else if(!m_midi_preferences.handEditEnabled) return view.expanded;
    else goto handle_staff_note;
    std::string error;
    if(!AdvancedSettingsIO::Save(m_midi_preferences,error))
      m_hand_edit_feedback="No se guardó el zoom: "+error;
    return true;
  }
handle_staff_note:
  if(!m_midi_preferences.handEditEnabled)return view.expanded;
  // Bulk edition uses the already existing A-B region in the transport.
  // These buttons never affect the MIDI or the 3D scene.
  if(mouse.released.left && mouse.y>=top+view.height-27 && mouse.y<=top+view.height-3){
    const int rel=mouse.x-left;
    if(rel>=12 && rel<=122 || rel>=132 && rel<=242){
      if(!m_loop_defined||m_loop_start>=m_loop_end){
        m_hand_edit_feedback="Primero selecciona un tramo en Bucle A-B";
        return true;
      }
      const auto previous=m_hand_classifier.Corrections();
      const auto hand=rel<=122?HandPractice::Left:HandPractice::Right;
      const size_t affected=m_hand_classifier.AssignRange(m_loop_start,m_loop_end,hand);
      if(affected==0){
        m_hand_edit_feedback="El tramo A-B no contiene notas";
        return true;
      }
      if(!SaveHandAnnotations()){
        m_hand_classifier.RestoreCorrections(previous);
        m_hand_edit_feedback="Error al guardar tramo";
        return true;
      }
      m_bulk_undo=previous;
      m_bulk_undo_available=true;
      m_hand_edit_feedback=STRING(affected<<" notas asignadas al tramo");
      m_required_notes.clear();SetupNoteState();
    }else if(rel>=252 && rel<=352 && m_bulk_undo_available){
      const auto current=m_hand_classifier.Corrections();
      m_hand_classifier.RestoreCorrections(m_bulk_undo);
      if(SaveHandAnnotations()){
        m_bulk_undo_available=false;
        m_hand_edit_feedback="Tramo restaurado";
        m_required_notes.clear();SetupNoteState();
      }else{
        m_hand_classifier.RestoreCorrections(current);
        m_hand_edit_feedback="Error al deshacer tramo";
      }
    }
    return true;
  }
  // The score notehead is the same point used by DrawGrandStaff; clicking
  // with the left button chooses the left hand, right chooses the right.
  const int lineStart=view.lineStart,staffEnd=view.staffEnd;
  const int bottoms[2]={view.bottom[0],view.bottom[1]};
  const auto now=m_state.midi->GetSongPositionInMicroseconds();
  const auto scoreStart=view.expanded && m_staff_page_start>=0?
                        m_staff_page_start:now;
  const int bpm=std::max(30,EffectiveTempoBPM());
  const microseconds_t horizon=view.horizon;
  int bestDistance=12*12;
  const TranslatedNote* chosen=nullptr;
  const auto staff=PrepareStaffRender(scoreStart,horizon);
  const int fifths=m_state.midi->GetCurrentKeyAccidentals();
  int displayed=0;
  for(const auto& glyph:staff.layout.notes){
    const auto& note=*staff.source[glyph.note.source];
    const auto hand=glyph.note.hand==0?HandPractice::Right:HandPractice::Left;
    const int part=glyph.note.hand;
    const auto visual=HandPractice::VisualQuantize(
         glyph.start,bpm,m_midi_preferences.notationQuantize);
    const auto offset=std::max<microseconds_t>(0,visual-scoreStart);
    if(offset>horizon)continue;
    const int system=view.SystemAt(visual,scoreStart);
    const int x=view.XAt(visual,scoreStart);
    const int noteBottom=view.Bottom(part,system);
    const int steps=StaffNotation::Spell(note.note_id,hand,fifths).offset;
    const int y=std::max(noteBottom-(view.expanded?view.step*11:48),
                   std::min(noteBottom+(view.expanded?view.step*3:15),
                              noteBottom-steps*view.step));
    const int d=(mouse.x-x)*(mouse.x-x)+(mouse.y-y)*(mouse.y-y);
    if(d<bestDistance){bestDistance=d;chosen=&note;}
    if(++displayed>=70)break;
  }
  if(chosen){
    const auto before=m_hand_classifier.Corrections();
    const auto beforeFingers=m_hand_classifier.Fingerings();
    bool changed=false;
    if(m_midi_preferences.handEditorTool==1){
      changed=mouse.released.right?
        m_hand_classifier.ClearFinger(chosen->track_id,chosen->note_id,
                                      chosen->start):
        m_hand_classifier.SetFinger(chosen->track_id,chosen->note_id,
                                  chosen->start,m_midi_preferences.fingerToAssign);
    }else{
      const auto hand=mouse.released.right?HandPractice::Right:
                                             HandPractice::Left;
      changed=m_hand_classifier.SetCorrection(chosen->track_id,
                                    chosen->note_id,chosen->start,hand);
    }
    if(changed){
      if(!SaveHandAnnotations()){
        m_hand_classifier.RestoreCorrections(before);
        for(const auto& current:m_hand_classifier.Fingerings())
          m_hand_classifier.ClearFinger(current.track,current.pitch,current.when);
        for(const auto& finger:beforeFingers)
          m_hand_classifier.SetFinger(finger.track,finger.pitch,finger.when,finger.finger);
        m_hand_edit_feedback="Error guardando anotación";
      }else{
        m_hand_edit_feedback=m_midi_preferences.handEditorTool==1?
            "Digitación guardada":"Mano guardada";
        m_required_notes.clear();SetupNoteState();
      }
    }
  }
  // Consume clicks anywhere within the editor to prevent scene rotation.
  return true;
}

void PlayingState::DrawGrandStaff(Renderer& renderer) const {
  const auto view=GetStaffViewport();
  const int left=view.left,top=view.top,width=view.width;
  const int lineStart=view.lineStart,staffEnd=view.staffEnd;
  const auto now=m_state.midi->GetSongPositionInMicroseconds();
  const auto scoreStart=view.expanded && m_staff_page_start>=0?
                        m_staff_page_start:now;
  const int bpm=std::max(30,EffectiveTempoBPM());
  const int fifths=m_state.midi->GetCurrentKeyAccidentals();
  const microseconds_t horizon=view.horizon; // score zoom only
  renderer.ForceTexture(0);
  // Opaque enough to read notes against the moving Blender stage.
  renderer.SetColor(9,25,40,view.expanded?250:243);
  renderer.DrawQuad(left,top,width,view.height);
  renderer.SetColor(63,143,166,240);
  renderer.DrawQuad(left,top,width,2);
  const char* heading=m_midi_preferences.handEditEnabled?
       (m_midi_preferences.handEditorTool==1?
        "DIGITACIÓN: izq=asignar · der=borrar":"EDITAR MANOS: izq=FA · der=SOL"):
       "PENTAGRAMA · SOL / FA";
  renderer.SetColor(255,255,255);
  TextWriter header(left+12,top+7,renderer,false,12);
  header<<Text(heading,Renderer::ToColor(211,236,249));
  // The key signature and meter are original MIDI metadata, not guessed.
  renderer.SetColor(255,255,255);
  TextWriter signature(left+12,top+22,renderer,false,10);
  signature<<Text(m_state.midi->GetCurrentKeySignature()+"  "+
                  m_state.midi->GetCurrentTimeSignature(),
                  Renderer::ToColor(163,199,213));
  // Explicit zoom and expand controls are part of the score, not the 3D
  // camera; their state persists separately from other settings.
  const int controls[3]={width-205,width-170,width-112};
  const int widths[3]={28,28,104};
  const char* labels[3]={"-","+",
     view.expanded?"Compacta":"Ampliar"};
  for(int i=0;i<3;++i){
    renderer.SetColor(34,76,97,235);
    renderer.DrawQuad(left+controls[i],top+15,widths[i],26);
    renderer.SetColor(255,255,255);
    TextWriter btn(left+controls[i]+6,top+22,renderer,false,12);
    btn<<Text(labels[i],Renderer::ToColor(231,244,249));
  }
  if(view.expanded){
    const int offsets[3]={width-510,width-467,width-425};
    const int widths[3]={33,33,52};
    const char* labels[3]={"<",">","Seguir"};
    for(int i=0;i<3;++i){
      renderer.SetColor(28,83,106,235);
      renderer.DrawQuad(left+offsets[i],top+15,widths[i],26);
      renderer.SetColor(255,255,255);
      TextWriter nav(left+offsets[i]+6,top+22,renderer,false,12);
      nav<<Text(labels[i],Renderer::ToColor(227,246,248));
    }
    if(m_staff_page_start>=0){
      renderer.SetColor(255,255,255);
      TextWriter page(left+width-365,top+23,renderer,false,10);
      page<<Text("Página manual",Renderer::ToColor(245,204,143));
    }
  }
  renderer.SetColor(255,255,255);
  TextWriter percent(left+width-248,top+23,renderer,false,10);
  percent<<Text(STRING(m_midi_preferences.notationZoom<<"%"),
                Renderer::ToColor(201,230,242));
  const int bottom[2]={view.bottom[0],view.bottom[1]}; // Sol/Fa
  const bool active[2]={
    m_midi_preferences.practiceHands!=HandPractice::LeftOnly,
    m_midi_preferences.practiceHands!=HandPractice::RightOnly
  };
  for(int sys=0;sys<view.systems;++sys){
    for(int part=0;part<2;++part){
      const int y0=view.Bottom(part,sys);
      renderer.SetColor(255,255,255);
      TextWriter t(left+12,y0-29,renderer,false,11);
      t<<Text(STRING((part==0?"SOL · Derecha":"FA · Izquierda")
                 <<(view.systems>1?(sys==0?" · I":" · II"):"")),
            active[part]?Renderer::ToColor(214,239,247):
                         Renderer::ToColor(117,138,147));
      for(int j=0;j<5;++j){
        renderer.SetColor(active[part]?115:70,
                          active[part]?161:99,active[part]?183:112,162);
        renderer.DrawQuad(lineStart,y0-2*view.step*j,staffEnd-lineStart,1);
      }
    }
  }
  // The MIDI's actual measure positions, including changes of meter
  // and tempo, determine the score's vertical bar lines.
  for(const auto& bar:m_state.midi->GetMusicalBars()){
    if(bar.time<scoreStart)continue;
    if(bar.time>scoreStart+horizon)break;
    const int sys=view.SystemAt(bar.time,scoreStart);
    const int xx=view.XAt(bar.time,scoreStart);
    renderer.SetColor(109,153,174,130);
    renderer.DrawQuad(xx,view.Bottom(0,sys)-8*view.step,1,
                    view.Bottom(1,sys)-view.Bottom(0,sys)+8*view.step);
    renderer.SetColor(255,255,255);
    TextWriter barNumber(xx+3,view.expanded?
                         view.Bottom(0,sys)-8*view.step-19:top+20,
                         renderer,false,9);
    barNumber<<Text(STRING(bar.number),Renderer::ToColor(155,184,196));
  }
  // Fixed time cursor for current playback: notes are positioned relative
  // to it, not by camera position or the 3D grand piano geometry.
  if(now>=scoreStart && now<=scoreStart+horizon){
    const int sys=view.SystemAt(now,scoreStart);
    renderer.SetColor(102,220,204,165);
    renderer.DrawQuad(view.XAt(now,scoreStart),
                      view.Bottom(0,sys)-8*view.step,2,
                      view.Bottom(1,sys)-view.Bottom(0,sys)+8*view.step);
  }
  const auto staff=PrepareStaffRender(scoreStart,horizon);
  struct Geometry {
    int x=0,y=0,tip=0,stemX=0;
    bool visible=false;
    bool down=false;
  };
  std::vector<Geometry> geometry(staff.layout.notes.size());
  bool multipleVoices[2]={false,false};
  for(const auto& g:staff.layout.notes)
    if(g.voice==1)multipleVoices[g.note.hand]=true;
  // An accidental persists for its letter and octave until the next
  // real barline, across voices within the same staff.
  std::map<std::tuple<int,int,int,int>,int> localAccidentals;
  int displayed=0;
  for(size_t index=0;index<staff.layout.notes.size();++index){
    const auto& glyph=staff.layout.notes[index];
    const auto& n=*staff.source[glyph.note.source];
    const auto hand=glyph.note.hand==0?HandPractice::Right:HandPractice::Left;
    const int part=glyph.note.hand;
    const auto visual=HandPractice::VisualQuantize(
               glyph.start,bpm,m_midi_preferences.notationQuantize);
    const auto offset=std::max<microseconds_t>(0,visual-scoreStart);
    if(offset>horizon)continue;
    const int sys=view.SystemAt(visual,scoreStart);
    const int x=view.XAt(visual,scoreStart);
    const int noteBottom=view.Bottom(part,sys);
    const auto spelling=StaffNotation::Spell(n.note_id,hand,fifths);
    const int offsetSteps=spelling.offset;
    const int y=noteBottom-offsetSteps*view.step;
    const int clampedY=std::max(noteBottom-(view.expanded?view.step*11:48),
                 std::min(noteBottom+(view.expanded?view.step*3:15),y));
    const auto base=Track::ColorNoteWhite[m_state.track_properties[n.track_id].color];
    const int opacity=active[part]?235:94;
    const auto rhythm=StaffNotation::RhythmFromDuration(
                       glyph.end-glyph.start,bpm);
    const auto value=rhythm.value;
    const auto accidentalKey=std::make_tuple(
        glyph.bar,part,spelling.octave,spelling.letter);
    const auto previousAccidental=localAccidentals.find(accidentalKey);
    const int existingAlteration=previousAccidental!=localAccidentals.end()?
       previousAccidental->second:
       StaffNotation::SignatureAlteration(spelling.letter,fifths);
    const char accidentalMark=existingAlteration==spelling.alteration?0:
            (spelling.alteration==0?'n':
            (spelling.alteration>0?'#':'b'));
    localAccidentals[accidentalKey]=spelling.alteration;
    const bool sounding=n.start<=now && n.end>now;
    // Each independent polyphonic voice has a stable stem direction.
    const bool down=multipleVoices[part]?glyph.voice==1:offsetSteps>4;
    if(glyph.tuplet && !glyph.beamPrev){
      renderer.SetColor(255,255,255);
      TextWriter number(x+3,clampedY-(down?18:29),renderer,false,11);
      number<<Text(STRING(glyph.tuplet),Renderer::ToColor(250,210,145));
    }
    const int stemX=x+(down?-6:5);
    const int stemY=down?clampedY:clampedY-21;
    geometry[index]={x,clampedY,down?clampedY+21:clampedY-21,stemX,true,down};
    renderer.SetColor(base.r,base.g,base.b,opacity);
    if(accidentalMark && !glyph.tieIn){
      renderer.SetColor(255,255,255);
      // Música impresa: alteraciones tipográficas reales, no caracteres
      // ASCII que se parecen a números o letras al ampliar la partitura.
      TextWriter acc(x-19,clampedY-13,renderer,false,19,"FreeSerif.ttf");
      const char* symbol=accidentalMark=='#'?"♯":
                         (accidentalMark=='b'?"♭":"♮");
      acc<<Text(symbol,Renderer::ToColor(221,235,243));
    }
    if(m_hand_classifier.IsCorrected(n.track_id,n.note_id,n.start) &&
       !glyph.tieIn){
      renderer.SetColor(239,189,74,226);
      renderer.DrawQuad(x-8,clampedY-6,16,1);
    }
    renderer.DrawRoundedGradient(x-6,clampedY-4,12,8,3,
              Renderer::ToColor(base.r,base.g,base.b,opacity),
              Renderer::ToColor(base.r/2+45,base.g/2+45,opacity));
    if(StaffNotation::Hollow(value)){
      renderer.SetColor(9,25,40,opacity);
      renderer.DrawQuad(x-3,clampedY-1,6,3);
    }
    if(rhythm.dots){
      renderer.SetColor(base.r,base.g,base.b,opacity);
      for(int dot=0;dot<rhythm.dots;++dot)
        renderer.DrawQuad(x+9+dot*5,clampedY-2,3,3);
    }
    if(StaffNotation::Stem(value)){
      renderer.SetColor(base.r,base.g,base.b,opacity);
      renderer.DrawQuad(stemX,stemY,2,21);
      // Adjacent short notes share a beam instead of individual flags.
      if(!glyph.beamPrev&&!glyph.beamNext)
        for(int flag=0;flag<StaffNotation::Flags(value);++flag)
          renderer.DrawQuad(stemX+(down?-6:0),
              stemY+(down?17-flag*6:1+flag*6),8,2);
    }
    if(sounding){
      renderer.SetColor(247,228,152,225);
      renderer.DrawQuad(x-8,clampedY+5,16,1);
    }
    const std::string tags=MarkForNote(n);
    if(!glyph.tieIn && !tags.empty()){
      renderer.SetColor(247,218,158,220);
      if(tags.find("staccato")!=std::string::npos ||
         tags.find("staccatissimo")!=std::string::npos)
        renderer.DrawQuad(x-1,clampedY-(down?12:30),3,3);
      if(tags.find("tenuto")!=std::string::npos)
        renderer.DrawQuad(x-5,clampedY-(down?12:30),10,2);
      if(tags.find("accent")!=std::string::npos ||
         tags.find("marcato")!=std::string::npos){
        TextWriter accent(x-5,clampedY-(down?18:37),renderer,false,13);
        accent<<Text(">",Renderer::ToColor(250,210,144));
      }
      if(tags.find("fermata")!=std::string::npos){
        TextWriter fermata(x-6,clampedY-44,renderer,false,13);
        fermata<<Text("^",Renderer::ToColor(240,229,173));
      }
    }
    const int finger=m_hand_classifier.Finger(n.track_id,n.note_id,n.start);
    if(finger && !glyph.tieIn){
      renderer.SetColor(255,255,255);
      TextWriter digit(x+9,clampedY-15,renderer,false,12);
      digit<<Text(STRING(finger),Renderer::ToColor(252,217,134));
    }
    if(offsetSteps<0 || offsetSteps>8){
      renderer.SetColor(160,198,212,150);
      if(offsetSteps>8){
        for(int step=10;step<=offsetSteps && step<=12;step+=2){
          const int ledgerY=noteBottom-step*view.step;
          if(ledgerY>=top+23)
            renderer.DrawQuad(x-9,ledgerY,18,1);
        }
      }else{
        for(int step=-2;step>=offsetSteps && step>=-4;step-=2){
          const int ledgerY=noteBottom-step*view.step;
          if(ledgerY<=top+view.height-4)
            renderer.DrawQuad(x-9,ledgerY,18,1);
        }
      }
    }
    if(++displayed>=70)break;
  }
  // Grouped stems with one/two horizontal beam strokes, independent
  // for each voice and not across measure lines or ties.
  for(size_t i=0;i<staff.layout.notes.size();++i){
    const auto& a=staff.layout.notes[i];
    if(!a.beamNext || !geometry[i].visible)continue;
    for(size_t j=i+1;j<staff.layout.notes.size();++j){
      const auto& b=staff.layout.notes[j];
      if(b.start>a.end+60000000LL/bpm/16)break;
      if(!b.beamPrev||!geometry[j].visible ||
         a.note.hand!=b.note.hand||a.voice!=b.voice||
         b.start!=a.end ||
         view.SystemAt(a.start,scoreStart)!=view.SystemAt(b.start,scoreStart))
        continue;
      const int x1=geometry[i].stemX,x2=geometry[j].stemX;
      if(x2<=x1)break;
      renderer.SetColor(170,220,237,216);
      const int strokes=StaffNotation::Flags(
            StaffNotation::ValueFromDuration(a.end-a.start,bpm));
      for(int beam=0;beam<strokes && beam<2;++beam){
        for(int x=x1;x<=x2;x+=2){
          const float fraction=float(x-x1)/float(x2-x1);
          const int tip=int(geometry[i].tip+
                    fraction*(geometry[j].tip-geometry[i].tip));
          const int beamY=tip+(geometry[i].down?-beam*6:beam*6);
          renderer.DrawQuad(x,beamY,3,3);
        }
      }
      break;
    }
  }
  // A held MIDI note crossing a true barline acquires a tie and
  // another written head, without generating a second MIDI NoteOn.
  for(size_t i=0;i<staff.layout.notes.size();++i){
    const auto& a=staff.layout.notes[i];
    if(!a.tieOut||!geometry[i].visible)continue;
    for(size_t j=i+1;j<staff.layout.notes.size();++j){
      const auto& b=staff.layout.notes[j];
      if(b.start>a.end)break;
      if(!b.tieIn||b.note.source!=a.note.source||
         b.start!=a.end||!geometry[j].visible ||
         view.SystemAt(a.start,scoreStart)!=view.SystemAt(b.start,scoreStart))
        continue;
      const int x1=geometry[i].x+7,x2=geometry[j].x-7;
      renderer.SetColor(224,226,182,208);
      if(x2>x1+4)
        for(int x=x1;x<=x2;x+=2){
          const float t=float(x-x1)/float(x2-x1);
          const int yy=geometry[i].y+7+int(5.f*std::sin(t*3.14159265f));
          renderer.DrawQuad(x,yy,3,2);
        }
      break;
    }
  }
  // MusicXML phrase slurs are distinct from MIDI ties, and never
  // trigger additional notes. Curve joins matching notes in the same
  // MIDI channel and staff when both ends are in the visible score.
  std::map<std::pair<int,int>,size_t> phraseStart;
  for(size_t i=0;i<staff.layout.notes.size();++i){
    if(!geometry[i].visible)continue;
    const auto& g=staff.layout.notes[i];
    const auto& n=*staff.source[g.note.source];
    const std::string marks=MarkForNote(n);
    const auto key=std::make_pair(g.note.hand,int(n.channel));
    if(marks.find("slur-start")!=std::string::npos)
      phraseStart[key]=i;
    if(marks.find("slur-stop")!=std::string::npos){
      auto found=phraseStart.find(key);
      if(found!=phraseStart.end() && found->second<i){
        const auto& from=geometry[found->second];
        const auto& to=geometry[i];
        if(to.x>from.x+8 &&
           view.SystemAt(staff.layout.notes[found->second].start,scoreStart)==
           view.SystemAt(g.start,scoreStart)){
          renderer.SetColor(233,200,150,215);
          for(int x=from.x;x<=to.x;x+=3){
            const float t=float(x-from.x)/(to.x-from.x);
            const int y=int(from.y+(to.y-from.y)*t-
                            18.f*std::sin(t*3.14159265f));
            renderer.DrawQuad(x,y,4,2);
          }
        }
        phraseStart.erase(found);
      }
    }
  }
  // Voice-specific rests only inside measured gaps between actual notes.
  // Vector symbols avoid reliance on external music fonts.
  for(const auto& rest:staff.layout.rests){
    const auto middle=rest.start+(rest.end-rest.start)/2;
    if(middle<scoreStart||middle>scoreStart+horizon)continue;
    const int sys=view.SystemAt(middle,scoreStart);
    const int x=view.XAt(middle,scoreStart);
    const int yy=view.Bottom(rest.hand,sys)+(rest.voice==0?-21:7);
    const auto restRhythm=StaffNotation::RhythmFromDuration(
                              rest.end-rest.start,bpm);
    renderer.SetColor(176,214,225,185);
    if(rest.value==StaffNotation::NoteValue::Whole ||
       rest.value==StaffNotation::NoteValue::Half){
      renderer.DrawQuad(x-6,yy,12,3);
    }else if(rest.value==StaffNotation::NoteValue::Quarter){
      renderer.DrawQuad(x-3,yy-10,4,9);
      renderer.DrawQuad(x,yy-2,4,8);
      renderer.DrawQuad(x-3,yy+5,5,3);
    }else{
      renderer.DrawQuad(x,yy-10,2,21);
      renderer.DrawQuad(x+1,yy-8,7,3);
      if(rest.value==StaffNotation::NoteValue::Sixteenth)
        renderer.DrawQuad(x+1,yy-3,7,3);
    }
    for(int dot=0;dot<restRhythm.dots;++dot)
      renderer.DrawQuad(x+10+dot*4,yy,2,2);
  }
  if(m_midi_preferences.handEditEnabled){
    const char* captions[3]={"Tramo A-B: IZQ","Tramo A-B: DER","Deshacer tramo"};
    const int offsets[3]={12,132,252}, widths[3]={110,110,100};
    for(int i=0;i<3;++i){
      renderer.SetColor(i==2?48:23,i==2?68:96,i==2?81:112,237);
      renderer.DrawQuad(left+offsets[i],top+view.height-28,widths[i],24);
      renderer.SetColor(255,255,255);
      TextWriter button(left+offsets[i]+5,top+view.height-22,renderer,false,12);
      button<<Text(captions[i],Renderer::ToColor(228,245,249));
    }
    renderer.SetColor(255,255,255);
    TextWriter info(left+365,top+view.height-22,renderer,false,11);
    info<<Text(m_hand_edit_feedback.empty()?
      STRING("Dedo "<<m_midi_preferences.fingerToAssign):
      m_hand_edit_feedback,Renderer::ToColor(195,227,220));
  }
  renderer.SetColor(189,216,228,230);
  TextWriter legend(left+width-105,top+7,renderer,false,10);
  legend<<Text(HandPractice::PracticeName(m_midi_preferences.practiceHands),
               Renderer::ToColor(182,235,209));
}

void PlayingState::Draw(Renderer &renderer) const {

  const Tga *key_tex[3] = { GetTexture(PlayKeyRail),
                            GetTexture(PlayKeyShadow),
                            GetTexture(PlayKeysBlack, true) };

  const Tga *note_tex[4] = { GetTexture(PlayNotesWhiteShadow, true),
                             GetTexture(PlayNotesBlackShadow, true),
                             GetTexture(PlayNotesWhiteColor, true),
                             GetTexture(PlayNotesBlackColor, true) };
  renderer.ForceTexture(0);

  // Visualización separada por modo: ajustes inmediatos mientras se edita.
  PlayerVisual visual;
  const auto &p=m_midi_preferences;
  if(m_view_3d){
    visual.effectSpeed=p.effectSpeed;visual.effectCount=p.effectCount;
    visual.effectSpread=p.effectSpread;visual.effectSize=p.effectSize;
    visual.effectPalette=p.effectPalette;visual.effectBirth=p.effectBirth;
    visual.scenePreset=p.scenePreset;visual.sceneLightIntensity=p.sceneLightIntensity;
    visual.sceneLightColor=p.sceneLightColor;
    visual.sceneCameraMotion=p.sceneCameraMotion;visual.scenePianistMotion=p.scenePianistMotion;
    visual.sceneYaw=p.sceneYaw;visual.scenePitch=p.scenePitch;
    visual.sceneZoom=p.sceneZoom;
    visual.sceneCameraFocus=p.sceneCameraFocus;
    visual.width=p.player3DWidth;visual.radius=p.player3DRadius;
    visual.noteLength=p.player3DNoteLength;
    visual.opacity=p.player3DOpacity;visual.glow=p.player3DGlow;
    visual.brightness=p.player3DBrightness;visual.guideSize=p.player3DGuideSize;
    visual.guideOpacity=p.player3DGuideOpacity;visual.keyOpacity=p.player3DKeyOpacity;
    visual.shadows=p.player3DShadows;visual.depth=p.player3DDepth;
    visual.angle=p.player3DAngle;visual.horizon=p.player3DHorizon;
    visual.stageWidth=p.player3DStageWidth;
    visual.backgroundOpacity=p.player3DBackgroundOpacity;
  }else{
    visual.width=p.player2DWidth;visual.radius=p.player2DRadius;
    visual.opacity=p.player2DOpacity;visual.glow=p.player2DGlow;
    visual.brightness=p.player2DBrightness;visual.guideSize=p.player2DGuideSize;
    visual.guideOpacity=p.player2DGuideOpacity;visual.keyOpacity=p.player2DKeyOpacity;
    visual.shadows=p.player2DShadows;
    visual.backgroundOpacity=p.player2DBackgroundOpacity;
  }
  // Use the precise edited MIDI hand and finger assignments to guide
  // the existing skinned Blender performer. No synthetic NoteOn is created:
  // KeyboardDisplay uses the actual held-note state before moving a finger.
  PianoHands::Guidance handGuidance;
  if(m_view_3d && p.scenePreset==6){
    const auto now=m_state.midi->GetSongPositionInMicroseconds();
    for(const auto &note:m_notes){
      if(note.start>now)break;
      if(note.end<=now || note.note_id<21 || note.note_id>108 ||
         note.channel==9 || note.track_id>=m_state.track_properties.size() ||
         (!m_score_tracks.empty()&&!m_score_tracks.count(note.track_id)) ||
         !m_hand_classifier.Selected(note,p.practiceHands))
        continue;
      const auto mode=m_state.track_properties[note.track_id].mode;
      if(mode==Track::ModeNotPlayed||mode==Track::ModePlayedButHidden)
        continue;
      const int key=int(note.note_id);
      const int finger=m_hand_classifier.Finger(note.track_id,key,note.start);
      // If two tracks sustain the same pitch, prefer an actual annotated
      // finger over an automatic fallback; physical MIDI key remains shared.
      if(handGuidance.hands[key]>=0 && handGuidance.fingers[key]>0
         && finger==0)continue;
      handGuidance.hands[key]=m_hand_classifier.ForNote(note)==
                            HandPractice::Left?0:1;
      handGuidance.fingers[key]=finger;
    }
  }
  // Filter *notes*, not only the practice/evaluation state. Previously
  // Left/Right buttons changed scoring but both hands still fell in green.
  // The hand assigned to each note is also used for the two distinct colours.
  TranslatedNoteSet focusedNotes;
  if(!m_score_tracks.empty()){
    for(const auto& n:m_notes)
      if(m_score_tracks.count(n.track_id) &&
         m_hand_classifier.Selected(n,p.practiceHands))
        focusedNotes.insert(n);
  }
  const auto& visibleNotes=!m_score_tracks.empty()?focusedNotes:m_notes;
  m_keyboard->SetHandVisualization(&m_hand_classifier,
                                    !m_score_tracks.empty());
  m_keyboard->SetManualKeyboardNotes(m_pressed_notes,p.handSplitPoint);
  m_keyboard->Draw(renderer, key_tex, note_tex, Layout::ScreenMarginX, 94, visibleNotes, m_show_duration,
                   m_state.midi->GetSongPositionInMicroseconds(), m_state.track_properties,
                   m_state.midi->GetBarLines(),m_view_3d,
                   (m_view_3d ? m_midi_preferences.player3DHeight :
                                m_midi_preferences.player2DHeight),
                   m_midi_preferences.playerGuides,
                   m_midi_preferences.playerNames,
                   m_midi_preferences.playerStyle2D,
                   m_midi_preferences.playerStyle3D,
                   visual,handGuidance,
                   m_midi_preferences.fallingNotesVisible);

  // Copia del framebuffer de la escena ANTES de dibujar el transporte,
  // las ventanas y los textos de configuración. Nada del escritorio entra.
  if(m_video.Active())CaptureVideoFrame();
  if(m_midi_preferences.notationVisible)DrawGrandStaff(renderer);
  DrawTransport(renderer);
  if(!m_video_status.empty()&&!m_video.Active()&&!m_video_dialog){
    renderer.SetColor(20,50,61);
    renderer.DrawQuad(18,GetStateHeight()-60,
                      std::min(GetStateWidth()-36,820),20);
    renderer.SetColor(255,255,255);
    TextWriter videoStatus(25,GetStateHeight()-57,renderer,false,11);
    videoStatus<<Text(m_video_status.substr(0,124),
                      Renderer::ToColor(213,241,223));
  }
  if(m_video_dialog)DrawVideoDialog(renderer);

  // Show the combo
  if (m_current_combo > 5) {
    int combo_font_size = 20;
    combo_font_size += (m_current_combo / 10);

    int combo_x = GetStateWidth() / 2;
    int combo_y = std::max(67,CalcKeyboardHeight()-155);

    TextWriter combo_text(combo_x, combo_y, renderer, true, combo_font_size);
    combo_text << STRING(m_current_combo << " ¡Combo!");
  }
}


void PlayingState::userPressedKey(int note_number, bool active)
{
    if (active)
    {
        if (m_should_wait_after_retry)
        {
            m_should_retry = false; // to ensure
            m_should_wait_after_retry = false;
        }
        m_pressed_notes.insert(note_number);
        m_required_notes.erase(note_number);
        m_state.dpms_thread->handleKeyPress();
    }
    else
        m_pressed_notes.erase(note_number);
}

void PlayingState::filePressedKey(int note_number, bool active, size_t track_id)
{
    // "Tocar" debe detener el tiempo igual que "Aprender" al llegar
    // a cada nota. Se exige pulsación MIDI correcta para continuar.
    if (isUserPlayableTrack(track_id) &&
        PracticeThisEvent(track_id,note_number,
                 m_state.midi->GetSongPositionInMicroseconds()))
    {
        if (active && isNoteInPlayableRange(note_number))
        {
            // Si la nota ya se mantiene pulsada no hay que bloquearla.
            if(!isKeyPressed(note_number))
                m_required_notes.insert(note_number);
        }
        else if(!active)
            m_required_notes.erase(note_number);
    }
}

bool PlayingState::PracticeThisEvent(size_t track,int pitch,
                                     microseconds_t when) const{
  return m_hand_classifier.SelectedEvent(track,pitch,when,
                                          m_midi_preferences.practiceHands);
}

bool PlayingState::isKeyPressed(int note_number)
{
    return (m_pressed_notes.find(note_number) != m_pressed_notes.end());
}

bool PlayingState::areAllRequiredKeysPressed()
{
    return m_required_notes.empty();
}

bool PlayingState::isUserPlayableTrack(size_t track_id)
{
  return (m_state.track_properties[track_id].mode == Track::ModeYouPlay ||
          m_state.track_properties[track_id].mode == Track::ModeYouPlaySilently ||
          m_state.track_properties[track_id].mode == Track::ModeLearning ||
          m_state.track_properties[track_id].mode == Track::ModeLearningSilently);
}

void PlayingState::eraseUntilTime(microseconds_t time)
{
  for (TranslatedNoteSet::const_iterator i = m_notes.begin(); i != m_notes.end();) {
    TranslatedNoteSet::const_iterator j = i;
    TranslatedNote n = *i;
    i++;

    // Erase very old notes
    if (n.end < time)
      m_notes.erase(j);
    else
    // Hit still visible once
    if (n.start <= time)
    {
      n.state = UserHit;
      m_notes.erase(j);
      m_notes.insert(n);
    }
  }
}

NoteState PlayingState::findNodeState(const TranslatedNote& note, TranslatedNoteSet& notes, NoteState default_note_state)
{
  // Search by comparing start, end, note_id and track_id
  TranslatedNoteSet::iterator n = notes.find(note);
  if (n == notes.end())
      return default_note_state;

  return n->state;
}
