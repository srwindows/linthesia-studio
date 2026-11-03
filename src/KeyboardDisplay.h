// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#ifndef __KEYBOARDDISPLAY_H
#define __KEYBOARDDISPLAY_H

#include <array>
#include <set>
#include <map>
#include <tuple>
#include <vector>
#include <string>

#include "TrackTile.h"
#include "PianoHands.h"
#include "HandPractice.h"
#include "TrackProperties.h"
#include "Keyboard.h"

#include "libmidi/Note.h"
#include "libmidi/MidiTypes.h"

typedef std::map<std::string, Track::TrackColor> KeyNames;

// Parámetros gráficos efectivos de cada modo, conservados por separado.
struct PlayerVisual {
 int width=90,radius=3,opacity=100,glow=20,brightness=100;
 int noteLength=100;
 int guideSize=1,guideOpacity=85,keyOpacity=100;
 int depth=65,angle=38,horizon=30,stageWidth=100,backgroundOpacity=100;
 bool shadows=true;
 int effectSpeed=100,effectCount=72,effectSpread=100,effectSize=100;
 int effectPalette=0,effectBirth=1;
 int effectWind=100;
 int effectLife=100;
 int effectTurbulence=85;
 int effectInitialVelocity=100;
 int effectBloom=115;
 int effectOpacity=70;
 int effectBlend=0;
 int effectRays=1;
 int effectSmoke=1;
 int effectTrails=1;
 int effectExplosion=130;
 int effectFlameHeight=100;
 int scenePreset=-1,sceneLightIntensity=55,sceneLightColor=0;
 int sceneCameraMotion=40,scenePianistMotion=55;
 int sceneYaw=92,scenePitch=26,sceneZoom=405;
 int sceneCameraFocus=0;
};

class KeyboardDisplay {
public:

  const static microseconds_t NoteWindowLength = 330000;

  KeyboardDisplay(const Keyboard& keyboard, int pixelWidth, int pixelHeight);
 ~KeyboardDisplay();

  void Draw(Renderer &renderer, const Tga *key_tex[3], const Tga *note_tex[4],
            int x, int y, const TranslatedNoteSet &notes, microseconds_t show_duration,
            microseconds_t current_time, const std::vector<Track::Properties> &track_properties,
            const MidiEventMicrosecondList &bar_line_usecs,
            bool perspective3d=false,int pianoPercent=16,
            bool showGuides=true,bool showNames=true,
            int style2D=0,int style3D=0,
            const PlayerVisual& visual=PlayerVisual(),
            const PianoHands::Guidance& handGuidance=PianoHands::Guidance(),
            bool showFallingNotes=true);

  void SetKeyActive(const std::string &key_name, bool active, Track::TrackColor color);
  // Per-note colours derived from the same classifier used by Sol/Fa score.
  // Never modifies the source MIDI or any track selection.
  void SetHandVisualization(const HandPractice::Classifier* classifier,
                            bool enabled){
    m_hands=enabled?classifier:nullptr;
  }
  void SetManualKeyboardNotes(const std::set<int>& pressed,int splitPoint){
    m_manual_keyboard_notes=pressed;
    m_manual_split_point=splitPoint;
  }

  void ResetActiveKeys() {
    m_active_keys.clear();
  }

private:

  struct NoteTexDimensions {

    int tex_width;
    int tex_height;

    int left;
    int right;

    int crown_start;
    int crown_end;

    int heel_start;
    int heel_end;
  };

  const static NoteTexDimensions WhiteNoteDimensions;
  const static NoteTexDimensions BlackNoteDimensions;

  struct KeyTexDimensions {

    int tex_width;
    int tex_height;

    int left;
    int right;

    int top;
    int bottom;
  };

  const static KeyTexDimensions BlackKeyDimensions;

  void DrawWhiteKeys(Renderer &renderer, bool active_only, int key_count, int key_width,
                     int key_height, int key_space, int x_offset, int y_offset) const;

  void DrawBlackKeys(Renderer &renderer, const Tga *tex, bool active_only, int white_key_count,
                     int white_width, int black_width, int black_height, int key_space,
                     int x_offset, int y_offset, int black_offset) const;

  void DrawRail(Renderer &renderer, const Tga *tex, int x, int y, int width) const;
  void DrawShadow(Renderer &renderer, const Tga *tex, int x, int y, int width) const;

  void DrawGuides(Renderer &renderer, int key_count, int key_width, int key_space,
                  int x_offset, int y, int y_offset) const;
  void DrawBars(Renderer &renderer, int x, int y, int y_offset,       
                int y_roll_under, int final_width,
                microseconds_t show_duration, microseconds_t current_time,
                const MidiEventMicrosecondList &bar_line_usecs) const;

  void DrawNotePass(Renderer &renderer, const Tga *tex_white, const Tga *tex_black,
                    int white_width, int key_space, int black_width, int black_offset,
                    int x_offset, int y, int y_offset, int final_width, int y_roll_under,
                    const TranslatedNoteSet &notes, microseconds_t show_duration,
                    microseconds_t current_time, const std::vector<Track::Properties> &track_properties) const;

  // This takes the rectangle where the actual note block should appear and transforms
  // it to the multi-quad (with relatively complicated texture coordinates) using the
  // passed-in texture descriptor, and then draws the result
  void DrawNote(Renderer &renderer, const Tga *tex, const NoteTexDimensions &tex_dimensions,
                int x, int y, int w, int h, int color_id) const;

  // This works very much like DrawNote
  void DrawBlackKey(Renderer &renderer, const Tga *tex, const KeyTexDimensions &tex_dimensions,
                    int x, int y, int w, int h, int sprite_index) const;

  int GetNoteNameFontSize(int key_width) const;

  const Keyboard& m_keyboard;
  KeyNames m_active_keys;
  // Reflejo exacto de teclas sonando en el tiempo MIDI de este fotograma.
  // Las notas manuales siguen gobernadas por SetKeyActive.
  KeyNames m_frame_active_keys;
  KeyNames m_frame_contact_keys;
  // Recorrido gradual de teclas físicas; independiente del avance MIDI.
  std::map<std::string,float> m_key_travel;
  struct ImpactVisual {
    int x,y,width;
    float age;
    SDL_Color color;
    unsigned seed;
    float fade=1.0f;
  };
  struct ReleasedImpact {
    ImpactVisual visual;
    microseconds_t noteOff=0;
  };
  // Retain the last frame's exact key center and timbre for a 420-ms tail.
  mutable std::map<std::string,ImpactVisual> m_current_impacts;
  mutable std::map<std::string,ImpactVisual> m_previous_impacts;
  mutable std::map<std::string,ReleasedImpact> m_release_impacts;
  std::map<std::string,Uint32> m_impact_started;
  mutable std::vector<ImpactVisual> m_impact_visuals;
  Uint32 m_last_travel_frame=0;
  Uint32 m_effect_frame_ticks=0;
  Uint64 m_effect_epoch=0;
  double m_effect_time=0.0;
  // Per-note animation starts at the FIRST visible drawing, including
  // notes already in the roll when a song is opened or after seeking.
  mutable std::map<std::tuple<int,int,microseconds_t,microseconds_t>,
                   std::pair<double,double>> m_visible_births;
  double m_last_birth_cleanup=0.0;
  float m_keyboard_center=0;
 std::array<Tga*,24> m_stage_textures{};
 std::array<bool,24> m_stage_failed{};
 double m_stage_pose_time=0.0,m_stage_previous_time=0.0;
  float m_stage_pitch_pose=0.0f;
  PianoHands::Animator m_live_hand_animator;
  Uint64 m_live_hand_clock=0;

  int m_width;
  int m_height;
  bool m_display_note_names=true;
  int m_guide_style=0;
  PlayerVisual m_visual;
  bool m_shadow_pass=false;
  bool m_render_3d=false;
  bool m_cinematic_3d=false;
  const HandPractice::Classifier* m_hands=nullptr;
  std::set<int> m_manual_keyboard_notes;
  int m_manual_split_point=60;
  static Track::TrackColor HandColor(HandPractice::Hand hand){
    return hand==HandPractice::Left?Track::TangoSkyBlue:
                                     Track::TangoOrange;
  }
  Track::TrackColor NoteColor(const TranslatedNote& n,
                 Track::TrackColor trackColor)const{
    return m_hands?HandColor(m_hands->ForNote(n)):trackColor;
  }

};

#endif // __KEYBOARDDISPLAY_H
