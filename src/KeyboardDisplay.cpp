// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "KeyboardDisplay.h"
#include "TrackProperties.h"
#include "Effect3DPalette.h"
#include "LinthesiaError.h"
#include "StringUtil.h"

#include "Renderer.h"
#include "Textures.h"
#include "Tga.h"
#include "libmidi/MidiEvent.h"
#include "NoteColumnSizing.h"
#include "ShortNoteVisual.h"
#include "NoteReleaseFade.h"
#include "StageScenes.h"
#include "Tga.h"
#include "LinthesiaError.h"
#include <cmath>
#include <algorithm>

using namespace std;

const KeyboardDisplay::NoteTexDimensions KeyboardDisplay::WhiteNoteDimensions = { 32, 128, 4, 25, 22, 28, 93, 100 };
const KeyboardDisplay::NoteTexDimensions KeyboardDisplay::BlackNoteDimensions = { 32,  64, 8, 20,  3,  8, 49,  55 };

struct KeyTexDimensions {
  int tex_width;
  int tex_height;

  int left;
  int right;

  int top;
  int bottom;
};

const KeyboardDisplay::KeyTexDimensions KeyboardDisplay::BlackKeyDimensions = { 32, 128, 8, 20, 15, 109 };
// Semilla estable por nota para que no haya parpadeos aleatorios entre frames.
static unsigned NoteParticleSeed(const std::string& note){
  unsigned hash=2166136261u;
  for(unsigned char c:note){hash^=c;hash*=16777619u;}
  return hash;
}

KeyboardDisplay::KeyboardDisplay(const Keyboard& keyboard, int pixelWidth, int pixelHeight) :
  m_keyboard(keyboard),
  m_width(pixelWidth),
  m_height(pixelHeight) {
  }
KeyboardDisplay::~KeyboardDisplay(){
  for(auto*& texture:m_stage_textures){
    if(texture){Tga::Release(texture);texture=nullptr;}
  }
}

void KeyboardDisplay::Draw(Renderer &renderer, const Tga *key_tex[3], const Tga *note_tex[4], int x, int y,
                           const TranslatedNoteSet &notes, microseconds_t show_duration, microseconds_t current_time,
                           const vector<Track::Properties> &track_properties,
                           const MidiEventMicrosecondList &bar_line_usecs,
                           bool perspective3d,int pianoPercent,
                           bool showGuides,bool showNames,
                           int style2D,int style3D,const PlayerVisual& visual,
                           const PianoHands::Guidance& handGuidance,
                           bool showFallingNotes) {
  // Un único reloj por fotograma: independiente de pausas, tempo y saltos MIDI.
  const Uint64 counter=SDL_GetPerformanceCounter();
  if(!m_effect_epoch)m_effect_epoch=counter;
  m_effect_time=double(counter-m_effect_epoch)/SDL_GetPerformanceFrequency();
  m_effect_frame_ticks=SDL_GetTicks();
  // Prune only notes absent from the view, not sustained notes.
  if(m_effect_time-m_last_birth_cleanup>2.0){
    for(auto it=m_visible_births.begin();it!=m_visible_births.end();){
      if(m_effect_time-it->second.second>3.0)
        it=m_visible_births.erase(it);
      else ++it;
    }
    m_last_birth_cleanup=m_effect_time;
  }
  m_display_note_names=showNames;
  m_guide_style=style2D;
  m_visual=visual;
  renderer.SetEffect3D(m_visual.effectSpeed,m_visual.effectCount,
                       m_visual.effectSpread,m_visual.effectSize,m_visual.effectBirth);
  renderer.SetParticleStudio(m_visual.effectWind,m_visual.effectLife,m_visual.effectTurbulence,m_visual.effectInitialVelocity,m_visual.effectBloom,m_visual.effectOpacity,m_visual.effectBlend,m_visual.effectRays,m_visual.effectSmoke,m_visual.effectTrails,m_visual.effectExplosion,m_visual.effectFlameHeight);
  m_render_3d=perspective3d;
  m_cinematic_3d=perspective3d && style3D==2;
  // Los eventos NoteOn/NoteOff pueden perderse tras un salto temporal
  // o con varias pistas iguales. El estado del teclado 3D se deriva de
  // las notas que siguen sonando en ESTE fotograma, no del dibujo de la
  // caída ni del estado anterior del reproductor.
  m_frame_active_keys=m_hands?KeyNames{}:m_active_keys;
  m_frame_contact_keys.clear();
  // Track MIDI playback and physical user presses separately. Otherwise the
  // previous implementation included accompaniment and the hidden hand from
  // m_active_keys even in left-only practice, showing false green key hits.
  if(m_hands)
    for(const int midi:m_manual_keyboard_notes){
      if(midi<21||midi>108)continue;
      m_frame_active_keys[MidiEvent::NoteName(midi)]=HandColor(
          midi<m_manual_split_point?HandPractice::Left:HandPractice::Right);
    }
  if(m_render_3d || m_hands){
    for(const auto& note:notes){
      if(note.start>current_time)break;
      if(note.end<=current_time ||
         note.track_id>=track_properties.size() ||
         note.note_id>127)continue;
      const Track::Properties& p=track_properties[note.track_id];
      const auto name=MidiEvent::NoteName(note.note_id);
      const auto handColor=NoteColor(note,p.color);
      if(p.mode==Track::ModePlayedAutomatically)
        m_frame_active_keys[name]=handColor;
      else if(note.state==UserPlayable &&
              (p.mode==Track::ModeYouPlay ||
               p.mode==Track::ModeYouPlaySilently ||
               p.mode==Track::ModeLearning ||
               p.mode==Track::ModeLearningSilently))
        m_frame_contact_keys[name]=handColor;
    }
  }
  if(m_render_3d){
    const Uint32 now=m_effect_frame_ticks;
    const Uint32 elapsed=m_last_travel_frame?
        std::min<Uint32>(60,now-m_last_travel_frame):16;
    m_last_travel_frame=now;
    for(const auto& key:m_frame_active_keys)
      if(m_impact_started.find(key.first)==m_impact_started.end())
        m_impact_started[key.first]=now;
    for(auto it=m_impact_started.begin();it!=m_impact_started.end();){
      if(m_frame_active_keys.count(it->first)==0)
        it=m_impact_started.erase(it);
      else ++it;
    }
    for(const auto& key:m_frame_active_keys)
        m_key_travel.emplace(key.first,0.0f);
    for(auto iter=m_key_travel.begin();iter!=m_key_travel.end();){
      const bool down=m_frame_active_keys.count(iter->first)!=0;
      const float target=down?1.0f:0.0f;
      const float response=1.0f-std::exp(-float(elapsed)/(down?70.0f:110.0f));
      iter->second+=(target-iter->second)*response;
      if(!down && iter->second<0.01f)
        iter=m_key_travel.erase(iter);
      else ++iter;
    }
  }

  // Source: Measured from Yamaha P-70
  const static double WhiteWidthHeightRatio = 6.8181818;
  const static double BlackWidthHeightRatio = 7.9166666;
  const static double WhiteBlackWidthRatio = 0.5454545;
  const double MaxKeyboardRatio = std::max(10,std::min(25,pianoPercent))/100.0;

  const int white_key_count = m_keyboard.GetWhiteKeyCount();

  // Calculate the largest white key size we can, and then
  // leave room for a single pixel space between each key
  int white_width = (m_width / white_key_count) - 1;
  int white_space = 1;

  int white_height = static_cast<int>(white_width * WhiteWidthHeightRatio);
  double white_height_ratio = white_height * 1.0 / m_height;
  double height_scale = 1.0;
  if (white_height_ratio > MaxKeyboardRatio)
  {
    height_scale = MaxKeyboardRatio / white_height_ratio;
    white_height = static_cast<int>(white_width * WhiteWidthHeightRatio * height_scale);
  }

  const int black_width = static_cast<int>(white_width * WhiteBlackWidthRatio);
  const int black_height = static_cast<int>(black_width * BlackWidthHeightRatio * height_scale);
  const int black_offset = white_width - (black_width / 2);

  // The dimensions given to the keyboard object are bounds.  Because of pixel
  // rounding, the keyboard will usually occupy less than the maximum in
  // either direction.
  //
  // So, we just try to center the keyboard inside the bounds.
  const int final_width = (white_width + white_space) * white_key_count;
  const int x_offset = (m_width - final_width) / 2;
  m_keyboard_center=x+x_offset+final_width/2.0f;
  const int y_offset = (m_height - white_height);

  // Give the notes a little more room to work with so they can roll under
  // the keys without distortion
  const int y_roll_under = white_height*3/4;

  // Symbolic names for the arbitrary array passed in here
  enum { Rail, Shadow, BlackKey };

  // Proyección en perspectiva de las notas y guías. El piano permanece
  // bidimensional: evita deformar las teclas al practicar.
  if(m_cinematic_3d){
    // Distinct, independent render layers:
    // 1. Blender stage and its performers occupy the scene background.
    // 2. The piano-roll notes are always rendered later using the same
    //    key-coordinate mapping as the fixed, interactive keyboard below.
    // Stage camera, scene geometry and grand piano never determine a
    // note's gameplay trajectory or destination.
    renderer.ForceTexture(0);
    renderer.SetColor(0,0,0,255);
    renderer.DrawQuad(x,y,m_width,y_offset);
    renderer.BeginPianoRollClip(x,y,x+m_width,y+y_offset);
    const int stage=StageScenes::ClampPreset(m_visual.scenePreset);
    if(stage>=0){
      const int activeKeys=static_cast<int>(m_frame_active_keys.size()+
                                             m_frame_contact_keys.size());
      const double delta=m_stage_previous_time>0?
          std::max(0.0,std::min(.06,m_effect_time-m_stage_previous_time)):.017;
      m_stage_previous_time=m_effect_time;
      m_stage_pose_time+=delta*(activeKeys?
                (1.4+.15*std::min(9,activeKeys)):0.25);
      // Map a MIDI pitch register to an approximate hand-pose family.
      // This animates both hands responsively, but is NOT a per-finger IK
      // reconstruction of the recorded piano performance.
      uint32_t pitchHash=0;
      for(const auto& key:m_frame_active_keys)
        for(const unsigned char c:key.first)
          pitchHash=pitchHash*33u+static_cast<uint32_t>(c);
      for(const auto& key:m_frame_contact_keys)
        for(const unsigned char c:key.first)
          pitchHash=pitchHash*33u+static_cast<uint32_t>(c);
      const float target=activeKeys?float(pitchHash%StageScenes::AnimationFrames):0.0f;
      m_stage_pitch_pose+=(target-m_stage_pitch_pose)*
            (1.0f-std::exp(-float(delta)*3.2f));
      const int pose=stage==6?
         StageScenes::PianistPose(m_stage_pose_time,activeKeys,
                    static_cast<uint32_t>(std::lround(m_stage_pitch_pose)),
                    m_visual.scenePianistMotion):
         StageScenes::PerformancePose(m_stage_pose_time,activeKeys,
                    m_visual.scenePianistMotion,stage);
      const int asset=stage==6?7+pose:stage>=7?18+(stage-7):stage; // extra duo and band slots
      if(!m_stage_textures[asset]&&!m_stage_failed[asset]){
        const std::string path=stage==6?
             std::string("scenarios/pianista_")+(pose<10?"0":"")+std::to_string(pose):
             std::string("scenarios/escena_")+
               (stage<10?"0":"")+std::to_string(stage);
        try{
          m_stage_textures[asset]=Tga::Load(path);
          m_stage_textures[asset]->SetSmooth(true);
        }catch(const std::exception& ex){
          fprintf(stderr,"Linthesia: no se pudo cargar escenario '%s': %s\n",
                  path.c_str(),ex.what());
          m_stage_failed[asset]=true;
        }
      }
      // The live Blender grand receives the EXACT currently sounding
      // NoteOn/NoteOff state (including manually played notes). Do not
      // synthesize a pose from pitch hashes or note-count: ten separate
      // articulated fingers and all 88 keys follow the real MIDI pitch.
      std::array<bool,128> held{};
      for(const auto& key:m_frame_active_keys){
        const int midi=PianoHands::ParseNoteName(key.first);
        if(midi>=21 && midi<=108)held[midi]=true;
      }
      for(const auto& key:m_frame_contact_keys){
        const int midi=PianoHands::ParseNoteName(key.first);
        if(midi>=21 && midi<=108)held[midi]=true;
      }
      const Uint64 handTime=SDL_GetPerformanceCounter();
      const float handDelta=m_live_hand_clock?
          float(double(handTime-m_live_hand_clock)/SDL_GetPerformanceFrequency()):.016f;
      m_live_hand_clock=handTime;
      m_live_hand_animator.Update(held,handDelta,m_visual.scenePianistMotion,
                                  handGuidance);
      const bool live3D=renderer.DrawBlenderScene3D(
          stage,pose,x,y,m_width,y_offset,m_visual.sceneYaw,
          m_visual.scenePitch,m_visual.sceneZoom,
          m_visual.sceneLightIntensity,m_visual.sceneLightColor,
          stage==6?&m_live_hand_animator.Current():nullptr,
          m_visual.sceneCameraFocus,m_visual.sceneCameraMotion,
          static_cast<float>(m_effect_time));
      if(live3D){
        // The stage deliberately draws no falling musical notes.
        // The pianist now has actual editable 3D lights. Old 2D triangle
        // beams pierced the human character like cardboard; preserve these
        // only in the other six abstract/motion-graphics stage presets.
        if(stage<6)renderer.DrawStageLights(x,y,m_width,y_offset,
             m_visual.sceneLightIntensity,m_visual.sceneLightColor,
             m_effect_time,m_visual.sceneCameraMotion);
      }else if(const Tga* background=m_stage_textures[asset]){
        const double camera=StageScenes::CameraOffset(
                              m_effect_time,m_visual.sceneCameraMotion);
        const int shift=static_cast<int>(m_width*.033*camera);
        renderer.SetColor(255,255,255,222);
        renderer.DrawStretchedTga(background,x+shift-10,y,
                                  m_width+20,y_offset);
        // Darker mid-plane keeps user-selected note colours distinguishable.
        renderer.ForceTexture(0);
        renderer.SetColor(0,0,0,76);
        renderer.DrawQuad(x,y,m_width,y_offset);
        renderer.DrawStageLights(x,y,m_width,y_offset,m_visual.sceneLightIntensity,
                                 m_visual.sceneLightColor,m_effect_time,
                                 m_visual.sceneCameraMotion);
      }
    }
  }else if(perspective3d){
    // Fondo en sombra a ambos lados del plano inclinado 3D.
    renderer.ForceTexture(0);
    renderer.SetColor(22,29,36,std::max(25,m_visual.backgroundOpacity*255/100));
    renderer.DrawQuad(x,y,m_width,y_offset);
    // Recorte físico: los bloques no pueden aparecer en el vacío entre
    // el final del escenario y la superficie del teclado.
    const int sceneTop=y+static_cast<int>(y_offset*
                              std::max(10,std::min(60,m_visual.horizon))/100.0);
    renderer.BeginPianoRollClip(x,sceneTop,x+m_width,y+y_offset);
    renderer.BeginPianoPerspective(x+x_offset+final_width/2.0f,
                                   y,y+y_offset,style3D,
                                   m_visual.depth,m_visual.angle,
                                   m_visual.horizon,m_visual.stageWidth);
  }
  // Draw background for falling notes
  if(m_cinematic_3d){
    if(showGuides){
      // Líneas verticales discretas, sin suelo inclinado ni cuadrícula horizontal.
      renderer.ForceTexture(0);
      for(int i=0;i<=white_key_count;++i){
        const int xx=x+x_offset+i*(white_width+white_space);
        renderer.SetColor(103,125,141,
            (i%7==0?m_visual.guideOpacity:std::max(3,m_visual.guideOpacity/4))*255/100);
        renderer.DrawQuad(xx,y,1,y_offset);
      }
    }
  }else if(showGuides)
    DrawGuides(renderer, white_key_count, white_width, white_space, x + x_offset, y, y_offset);
  else {
    renderer.ForceTexture(0);
    renderer.SetColor(35,38,42);
    renderer.SetColor(35,38,42,m_visual.backgroundOpacity*255/100);
    renderer.DrawQuad(x+x_offset,y,final_width,y_offset);
  }

  // Draw tempo bars
  if(!m_cinematic_3d)
    DrawBars(renderer, x+x_offset, y, y_offset, y_roll_under, final_width,
             show_duration, current_time, bar_line_usecs);

  // Keyboard, key contact, pianist and progression continue working
  // even with falling note blocks hidden (sheet music only, F5).
  if(showFallingNotes){
  // Do two passes on the notes, the first for note shadows and the second
  // for the note blocks themselves.  This is to avoid shadows being drawn
  // on top of notes.
  renderer.SetColor(Renderer::ToColor(255, 255, 255));
  m_shadow_pass=true;
  if(m_visual.shadows)DrawNotePass(renderer, note_tex[0], note_tex[1], white_width, white_space, black_width,
               black_offset, x + x_offset, y, y_offset, final_width, y_roll_under, notes, show_duration,
               current_time, track_properties);

  m_shadow_pass=false;
  if(perspective3d)renderer.BeginBlenderNotes3D();
  DrawNotePass(renderer, note_tex[2], note_tex[3], white_width, white_space, black_width,
               black_offset, x + x_offset, y, y_offset, final_width, y_roll_under, notes, show_duration,
               current_time, track_properties);
  if(perspective3d)renderer.EndBlenderNotes3D();

  } // showFallingNotes
  if(perspective3d){
    if(!m_cinematic_3d)renderer.EndPianoPerspective();
    renderer.EndPianoRollClip();
  }

  const int ActualKeyboardWidth = white_width*white_key_count + white_space*(white_key_count-1);

  // Black out the background of where the keys are about to appear
  renderer.SetColor(Renderer::ToColor(0, 0, 0));
  renderer.DrawQuad(x + x_offset, y+y_offset, ActualKeyboardWidth, white_height);

  if(m_render_3d){
    renderer.SetColor(13,20,30,255);
    renderer.DrawQuad(x+x_offset-2,y+y_offset-7,ActualKeyboardWidth+4,7);
    renderer.SetColor(75,91,111,255);
    renderer.DrawQuad(x+x_offset-2,y+y_offset-7,ActualKeyboardWidth+4,1);
  }else
    DrawShadow(renderer,key_tex[Shadow],x+x_offset,
               y+y_offset+white_height-10,ActualKeyboardWidth);
  m_impact_visuals.clear();
  m_current_impacts.clear();
  if(m_render_3d)renderer.BeginBlenderKeyboard3D();
  DrawWhiteKeys(renderer, false, white_key_count, white_width, white_height, white_space, x+x_offset, y+y_offset);
  DrawBlackKeys(renderer, key_tex[BlackKey], false, white_key_count, white_width,
                black_width, black_height, white_space, x+x_offset, y+y_offset, black_offset);
  if(m_render_3d)renderer.EndBlenderKeyboard3D();
  // A released key is immediately UP for MIDI and mechanical animation,
  // but its previous flame keeps a visual tail at precisely the same key.
  // Key center, width, color and seed are retained from the previous frame.
  if(m_render_3d){
    for(const auto& last:m_previous_impacts)
      if(m_current_impacts.count(last.first)==0)
        m_release_impacts[last.first]={last.second,current_time};
    for(const auto& current:m_current_impacts)
      m_release_impacts.erase(current.first); // re-attack has priority
    m_previous_impacts=m_current_impacts;
    for(auto it=m_release_impacts.begin();it!=m_release_impacts.end();){
      // Keep only the visual plume alive after NoteOff. The physical key
      // rises immediately and audio stops at exact MIDI time. Unlike the
      // old 420ms release fade, a plume must have enough flight time to
      // reach 40-47% of the scene height before dissipating.
      const float sinceRelease=float(current_time-it->second.noteOff)/1000000.f;
      const float flightAge=it->second.visual.age+sinceRelease;
      if(sinceRelease<0.f ||
         flightAge*(m_visual.effectSpeed/100.f)>=1.10f*m_visual.effectLife/100.f){
        it=m_release_impacts.erase(it);continue;
      }
      auto last=it->second.visual;
      last.age=flightAge;
      last.fade=1.0f;  // GLSL + particle lifetime control visual fading
      m_impact_visuals.push_back(last);
      ++it;
    }
  }else{
    m_previous_impacts.clear();
    m_release_impacts.clear();
  }
  // The bottom keyboard is always the playable one. All impact particles
  // and note/key illumination belong here, for stages 6 through 11 alike.
  if(m_render_3d){
    if(m_cinematic_3d)
      renderer.DrawPianoContactLightBar(x+x_offset,y+y_offset,
                                        ActualKeyboardWidth,m_visual.glow);
    for(const auto& impact:m_impact_visuals)
      renderer.DrawPianoImpactParticles(impact.x,impact.y,impact.width,
          impact.age,impact.color,impact.seed,m_visual.glow,
          static_cast<float>(m_effect_time),impact.fade);
  }
  if(m_render_3d){
    renderer.SetColor(7,12,19,255);
    renderer.DrawQuad(x+x_offset,y+y_offset+white_height-3,
                      ActualKeyboardWidth,4);
  }else{
    DrawShadow(renderer,key_tex[Shadow],x+x_offset,y+y_offset,ActualKeyboardWidth);
    DrawRail(renderer,key_tex[Rail],x+x_offset,y+y_offset,ActualKeyboardWidth);
  }

  // Top of the screen shadow and rail
   DrawShadow(renderer, key_tex[Shadow], x+x_offset, y, ActualKeyboardWidth+1);
   DrawRail(renderer, key_tex[Rail], x+x_offset, y, ActualKeyboardWidth+1);
}

void KeyboardDisplay::DrawWhiteKeys(Renderer &renderer, bool active_only, int key_count, int key_width, int key_height,
                                    int key_space, int x_offset, int y_offset) const {
  SDL_Color white = Renderer::ToColor(255, 255, 255);

  char current_white = m_keyboard.GetStartingNote();
  int current_octave = m_keyboard.GetStartingOctave();
  for (int i = 0; i < key_count; ++i) {

    // Check to see if this is one of the active notes
    const string note_name = STRING(current_white << current_octave);

    const KeyNames& activeKeys=(m_render_3d||m_hands)?
                            m_frame_active_keys:m_active_keys;
    KeyNames::const_iterator find_result = activeKeys.find(note_name);
    bool active = (find_result != activeKeys.end());

    SDL_Color c = white;
    bool cue=false;
    if(active)
      c=Track::ColorNoteWhite[find_result->second];
    else if(m_render_3d){
      const auto contact=m_frame_contact_keys.find(note_name);
      cue=contact!=m_frame_contact_keys.end();
      if(cue)c=Track::ColorNoteWhite[contact->second];
    }

    if ((active_only && active) || !active_only) {
      // El teclado sigue el color REAL de la pista MIDI. La paleta de
      // efectos solo colorea llamas/particulas, no la tecla ni la nota.
      c.a=static_cast<Uint8>(std::max(20,std::min(100,m_visual.keyOpacity))*255/100);
      renderer.SetColor(c);

      const int key_x = i * (key_width + key_space) + x_offset;
      if(m_render_3d){
        const auto travel=m_key_travel.find(note_name);
        const float depression=travel!=m_key_travel.end()?travel->second:0.0f;
        renderer.DrawPianoKey3D(key_x,y_offset,key_width,key_height,
                                false,depression,cue,c,m_keyboard_center);
        if(active){
          const auto hit=m_impact_started.find(note_name);
          if(hit!=m_impact_started.end()){
            const ImpactVisual impact={key_x+key_width/2,y_offset,
               key_width,(m_effect_frame_ticks-hit->second)/1000.0f,
               Effect3DPalette::Color(Track::ColorNoteWhite[find_result->second],
                   m_visual.effectPalette,static_cast<int>(find_result->second)),
               NoteParticleSeed(note_name)};
            m_impact_visuals.push_back(impact);
            m_current_impacts[note_name]=impact;
          }
        }
      }else renderer.DrawQuad(key_x,y_offset,key_width,key_height);
      
      const SDL_Color text_color1 (Renderer::ToColor(0x50,0x50,0x50));
      if(m_display_note_names){
        int font_size=GetNoteNameFontSize(key_width);
        const float footX=key_x+(key_width-font_size)/2.0f;
        const int labelX=m_render_3d?
          static_cast<int>(m_keyboard_center+
                           (footX-m_keyboard_center)*1.068f):int(footX);
        if(m_render_3d)glDisable(GL_DEPTH_TEST);
        TextWriter title(labelX,
            y_offset+key_height-font_size-8,renderer,false,font_size);
        title<<Text(note_name.c_str(),text_color1);
        if(m_render_3d)glEnable(GL_DEPTH_TEST);
      }
    }

    current_white++;

    if (current_white == 'H')
      current_white = 'A';

    if (current_white == 'C')
      current_octave++;
   }

  renderer.SetColor(white);
}

void KeyboardDisplay::DrawBlackKey(Renderer &renderer, const Tga *tex, const KeyTexDimensions &tex_dimensions,
                                   int x, int y, int w, int h, int sprite_index) const {

  const KeyTexDimensions &d = tex_dimensions;

  const int tex_w = d.right - d.left;
  const double width_scale = double(w) / double(tex_w);
  const double full_tex_width = d.tex_width * width_scale;
  const double left_offset = d.left * width_scale;

  const int src_x = sprite_index * d.tex_width;
  const int dest_x = int(x - left_offset) - 1;
  const int dest_w = int(full_tex_width);

  const int tex_h = d.bottom - d.top;
  const double height_scale = double(h) / double(tex_h);
  const double full_tex_height = d.tex_height * height_scale;
  const double top_offset = d.top * height_scale;

  const int dest_y = int(y - top_offset) - 1;
  const int dest_h = int(full_tex_height);

  renderer.DrawStretchedTga(tex, dest_x, dest_y, dest_w, dest_h, src_x, 0, d.tex_width, d.tex_height);
}

void KeyboardDisplay::DrawBlackKeys(Renderer &renderer, const Tga *tex, bool active_only, int white_key_count,
                                    int white_width, int black_width, int black_height, int key_space,
                                    int x_offset, int y_offset, int black_offset) const {

  char current_white = m_keyboard.GetStartingNote();
  int current_octave = m_keyboard.GetStartingOctave();
  for (int i = 0; i < white_key_count; ++i) {
    // Don't allow a very last black key
    if (i == white_key_count - 1)
      break;

    switch (current_white) {
    case 'A':
    case 'C':
    case 'D':
    case 'F':
    case 'G': {
      // Check to see if this is one of the active notes
      const string note_name = STRING(current_white << '#' << current_octave);

      const KeyNames& activeKeys=(m_render_3d||m_hands)?
                            m_frame_active_keys:m_active_keys;
      KeyNames::const_iterator find_result = activeKeys.find(note_name);
      bool active = (find_result != activeKeys.end());
      const auto cueIt=m_frame_contact_keys.find(note_name);
      const bool cue=m_render_3d && !active &&
                     cueIt!=m_frame_contact_keys.end();

      // play_KeysBlack.tga tiene 8 sprites de 32 px: seis colores, gris
      // y la tecla NEGRA tradicional en el índice 7. El antiguo valor
      // Track::MissedNote ahora vale 17 tras ampliar las pistas a 16
      // colores, y apuntaba fuera de la textura, produciendo teclas verdes.
      // El color de las notas que caen permanece independiente del teclado.
      constexpr int BlackKeySpriteIndex = 7;
      if (!active_only || active) {
        const int start_x = i * (white_width + key_space) + x_offset + black_offset;
        renderer.SetColor(255,255,255,m_visual.keyOpacity*255/100);
        if(m_render_3d){
          const auto travel=m_key_travel.find(note_name);
          const float depression=travel!=m_key_travel.end()?travel->second:0.0f;
          const Track::TrackColor color=active?find_result->second:
               (cue?cueIt->second:Track::FlatGray);
          // Colores de las teclas: exclusivamente el de la pista.
          const SDL_Color accent=active||cue?
               Track::ColorNoteWhite[color]:Renderer::ToColor(40,45,55);
          renderer.DrawPianoKey3D(start_x,y_offset,black_width,black_height,
                                  true,depression,cue,accent,m_keyboard_center);
          if(active){
            const auto hit=m_impact_started.find(note_name);
            if(hit!=m_impact_started.end()){
              const ImpactVisual impact={start_x+black_width/2,y_offset,
                 black_width,(m_effect_frame_ticks-hit->second)/1000.0f,
                 Effect3DPalette::Color(Track::ColorNoteWhite[color],
                     m_visual.effectPalette,int(color)),NoteParticleSeed(note_name)};
              m_impact_visuals.push_back(impact);
              m_current_impacts[note_name]=impact;
            }
          }
        }else
          DrawBlackKey(renderer,tex,BlackKeyDimensions,start_x,y_offset,
                       black_width,black_height,BlackKeySpriteIndex);
        if(active && !m_render_3d && black_width>=3 && black_height>=8){
          // El sprite negro es siempre el mismo. Dibujar encima el estado
          // MIDI activo, también para C#, D#, F#, G# y A#.
          const int idx=static_cast<int>(find_result->second);
          const SDL_Color base=(idx>=0 && idx<Track::ColorCount)?
               Track::ColorNoteHit[idx]:Renderer::ToColor(166,248,216);
          const int x1=start_x+1;
          const int w1=std::max(2,black_width-2);
          const int bottom=y_offset+black_height-2;
          const int alpha=std::max(130,m_visual.keyOpacity*195/100);
          if(m_render_3d){
            SDL_Color highlight={static_cast<Uint8>(std::min(255,int(base.r)+45)),
                static_cast<Uint8>(std::min(255,int(base.g)+45)),
                static_cast<Uint8>(std::min(255,int(base.b)+45)),255};
            SDL_Color foot={static_cast<Uint8>(base.r*55/100),
                static_cast<Uint8>(base.g*55/100),
                static_cast<Uint8>(base.b*55/100),255};
            renderer.DrawRoundedGradient(x1,y_offset+2,w1,
                std::max(4,black_height-8),2,highlight,foot);
            renderer.SetColor(base.r,base.g,base.b,58);
            renderer.DrawQuad(x1,y_offset,std::max(1,w1),2);
          }else{
            renderer.SetColor(base.r,base.g,base.b,48);
            renderer.DrawQuad(start_x-2,y_offset+3,black_width+4,
                              std::max(4,black_height-5));
            renderer.SetColor(base.r,base.g,base.b,alpha);
            renderer.DrawQuad(x1,y_offset+2,w1,
                              std::max(4,black_height-6));
          }
          // Señal luminosa de contacto, sin cubrir de blanco el relieve 3D.
          if(m_render_3d){
            renderer.SetColor(base.r,base.g,base.b,185);
            renderer.DrawQuad(x1+1,y_offset+1,std::max(1,w1-2),2);
            renderer.SetColor(base.r,base.g,base.b,58);
            renderer.DrawQuad(x1+1,y_offset+5,std::max(1,w1-2),
                              std::max(1,bottom-y_offset-16));
          }else{
            renderer.SetColor(245,255,248,185);
            renderer.DrawQuad(x1,y_offset+2,w1,2);
            renderer.SetColor(235,255,248,90);
            renderer.DrawQuad(x1+1,y_offset+6,std::max(1,w1-2),
                              std::max(1,bottom-y_offset-15));
          }
        }
        renderer.SetColor(255,255,255);
      }
    }

    }

    current_white++;
    if (current_white == 'H')
      current_white = 'A';

    if (current_white == 'C')
      current_octave++;
  }
}

void DrawWidthStretched(Renderer &renderer, const Tga *tex, int x, int y, int width) {

  renderer.DrawStretchedTga(tex, x, y, width, tex->GetHeight(), 0, 0, tex->GetWidth(), tex->GetWidth());
}

void KeyboardDisplay::DrawRail(Renderer &renderer, const Tga *tex, int x, int y, int width) const {

  const static int RailOffsetY = -4;
  DrawWidthStretched(renderer, tex, x, y + RailOffsetY, width);
}

void KeyboardDisplay::DrawShadow(Renderer &renderer, const Tga *tex, int x, int y, int width) const {

  const static int ShadowOffsetY = 10;
  DrawWidthStretched(renderer, tex, x, y + ShadowOffsetY, width);
}

void KeyboardDisplay::DrawGuides(Renderer &renderer, int key_count, int key_width, int key_space,
                                 int x_offset, int y, int y_offset) const {

  const static int PixelsOffKeyboard = 2;
  int keyboard_width = key_width*key_count + key_space*(key_count-1);

  // Fill the background of the note-falling area
  renderer.ForceTexture(0);
  const bool neon=m_guide_style==1;
  renderer.SetColor(neon?13:0x24,neon?17:0x26,neon?42:0x29,
                    m_visual.backgroundOpacity*255/100);
  renderer.DrawQuad(x_offset, y, keyboard_width, y_offset - PixelsOffKeyboard);

  const SDL_Color thick(neon?Renderer::ToColor(34,83,136):
                             Renderer::ToColor(0x39,0x3c,0x40));
  const SDL_Color thin(neon?Renderer::ToColor(24,34,82):
                            Renderer::ToColor(0x2b,0x2e,0x31));

  char current_white = m_keyboard.GetStartingNote() - 1;
  int current_octave = m_keyboard.GetStartingOctave();
  for (int i = 0; i < key_count + 1; ++i) {

    const int key_x = i * (key_width + key_space) + x_offset - 1;

    int guide_thickness = 1;
    SDL_Color guide_color = thin;

    bool draw_guide = true;
    switch (current_white) {
    case 'C':
    case 'D':
      guide_color = thin;
    break;

    case 'F':
    case 'G':
    case 'A': {
      guide_color = thick;
      guide_thickness = 2;
      break;
    }

    default: {
      draw_guide = false;
      break;
    }
    }

    if (draw_guide) {
      guide_color.a=static_cast<Uint8>(m_visual.guideOpacity*255/100);
      renderer.SetColor(guide_color);
      const int thickness=std::max(1,m_visual.guideSize*guide_thickness);
      renderer.DrawQuad(key_x-thickness/2,y,thickness,y_offset-PixelsOffKeyboard);
    }

    current_white++;
    if (current_white == 'H')
      current_white = 'A';

    if (current_white == 'C')
      current_octave++;
  }
}

void KeyboardDisplay::DrawBars(Renderer &renderer, int x, int y, int y_offset,
        int y_roll_under, int final_width,
        microseconds_t show_duration, microseconds_t current_time,
        const MidiEventMicrosecondList &bar_line_usecs) const {
   int i=0;
   MidiEventMicrosecondList::const_iterator j = bar_line_usecs.begin();
   // En 3D el compás orienta pero no atraviesa visualmente las notas.
   // El 2D mantiene su presentación original.
   const SDL_Color bar_color=m_render_3d?
       SDL_Color{58,77,88,92}:Renderer::ToColor(0x4e,0x53,0x57);
   const SDL_Color text_color=m_render_3d?
       SDL_Color{112,133,143,185}:Renderer::ToColor(0x9e,0xa7,0xab);
   for (; j != bar_line_usecs.end(); ++j, ++i) {
      renderer.SetColor(bar_color);
      microseconds_t bar_usec = *j;
      // Skip previous bars
      if (bar_usec < current_time)
        continue;
      // This list is sorted by note start time.  The moment we encounter
      // a bar scrolled off the window, we're done drawing
      if (bar_usec > current_time + show_duration)
        break;

      const double scaling_factor = static_cast<double>(y_offset) / static_cast<double>(show_duration);

      const long long roll_under = static_cast<int>(y_roll_under / scaling_factor);
      const long long adjusted_offset = max(bar_usec - current_time, -roll_under);

      // Convert our times to pixel coordinates
      const int y_bar_offset = y - static_cast<int>(adjusted_offset * scaling_factor) + y_offset;
      renderer.DrawQuad(x, y_bar_offset, final_width, m_render_3d?1:2);

      // Add a label with a bar number
      TextWriter bar_writer(x+4, y_bar_offset-14, renderer, false, Layout::BarFontSize);
      bar_writer << Text(STRING(i+1), text_color);
   }
}

void KeyboardDisplay::DrawNote(Renderer &renderer, const Tga *tex, const NoteTexDimensions &tex_dimensions,
                               int x, int y, int w, int h, int color_id) const {

  const NoteTexDimensions &d = tex_dimensions;

  // Width is super-easy
  const int tex_note_w = d.right - d.left;

  const double width_scale = double(w) / double(tex_note_w);
  const double full_tex_width = d.tex_width * width_scale;
  const double left_offset = d.left * width_scale;

  const int src_x = (color_id * d.tex_width);
  const int dest_x = int(x - left_offset);
  const int dest_w = int(full_tex_width);

  // Now we draw the note in three sections:
  // - Crown (fixed (relative) height)
  // - Middle (variable height)
  // - Heel (fixed (relative) height)

  // Force the note to be at least as large as the crown + heel height
  const double crown_h = (d.crown_end - d.crown_start) * width_scale;
  const double heel_h = (d.heel_end - d.heel_start) * width_scale;
  const double min_height = crown_h + heel_h + 1.0;

  if (h < min_height) {
    const int diff = int(min_height - h);
    h += diff;
    y -= diff;
  }

  // We actually use the width scale in height calculations
  // to keep the proportions fixed.
  const double crown_start_offset = d.crown_start * width_scale;
  const double crown_end_offset = d.crown_end * width_scale;
  const double heel_height = double(d.heel_end - d.heel_start) * width_scale;
  const double bottom_height = double(d.tex_height - d.heel_end) * width_scale;

  const int dest_y1 = int(y - crown_start_offset);
  const int dest_y2 = int(dest_y1 + crown_end_offset);
  const int dest_y3 = int((y+h) - heel_height);
  const int dest_y4 = int(dest_y3 + bottom_height);

  renderer.DrawStretchedTga(tex, dest_x, dest_y1, dest_w, dest_y2 - dest_y1, src_x, 0, d.tex_width, d.crown_end);
  renderer.DrawStretchedTga(tex, dest_x, dest_y2, dest_w, dest_y3 - dest_y2, src_x, d.crown_end,
                            d.tex_width, d.heel_start - d.crown_end);

  renderer.DrawStretchedTga(tex, dest_x, dest_y3, dest_w, dest_y4 - dest_y3, src_x, d.heel_start,
                            d.tex_width, d.tex_height - d.heel_start);
}


void KeyboardDisplay::DrawNotePass(Renderer &renderer, const Tga *tex_white, const Tga *tex_black, int white_width,
                                   int key_space, int black_width, int black_offset, int x_offset, int y,
                                   int y_offset, int final_width, int y_roll_under, const TranslatedNoteSet &notes,
                                   microseconds_t show_duration, microseconds_t current_time,
                                   const vector<Track::Properties> &track_properties) const {

  // Shiny music domain knowledge
  const static unsigned int NotesPerOctave = 12;
  const static unsigned int WhiteNotesPerOctave = 7;
  const static bool IsBlackNote[12] = { false, true,  false, true,  false, false,
                                        true,  false, true,  false, true,  false };

  // The constants used in the switch below refer to the number
  // of white keys off 'C' that type of piano starts on
  int keyboard_type_offset = m_keyboard.GetKeyboardTypeOffset();

  // This array describes how to "stack" notes in a single place.  The IsBlackNote array
  // then tells which one should be shifted slightly to the right
  const static int NoteToWhiteNoteOffset[12] = { 0, -1, -1, -2, -2, -2, -3, -3, -4, -4, -5, -5 };

  const static int MinNoteHeight = 3;

  // Una repetición MIDI sin separación real es una sola figura visual:
  // sin dos tapas brillantes encadenadas ni costuras en el mismo carril.
  // Se preservan los NoteOn originales para aprendizaje y evaluación.
  typedef TranslatedNoteSet::const_iterator NoteIter;
  std::vector<std::pair<NoteIter,microseconds_t>> visual_notes;
  std::map<std::pair<int,int>,std::size_t> most_recent;
  for(NoteIter note=notes.begin();note!=notes.end();++note){
    const auto key=std::make_pair(static_cast<int>(note->track_id),
                                  static_cast<int>(note->note_id));
    auto prior=most_recent.find(key);
    if(m_render_3d && prior!=most_recent.end()){
      auto& previous=visual_notes[prior->second];
      const auto& old=*previous.first;
      // Un hueco inferior a un píxel de pantalla no debe producir
      // dos cápsulas con dos tapas superpuestas (costura evidente).
      const microseconds_t onePixel=std::max<microseconds_t>(
          1000,show_duration/std::max(1,y_offset));
      const microseconds_t gap=note->start-previous.second;
      if(gap>=-onePixel && gap<=onePixel &&
         old.state==note->state && old.retry_state==note->retry_state){
        previous.second=std::max(previous.second,note->end);
        continue;
      }
    }
    most_recent[key]=visual_notes.size();
    visual_notes.emplace_back(note,note->end);
  }
  bool drawing_black = false;
  for (int toggle = 0; toggle < 2; ++toggle) {

    for (const auto& visual : visual_notes) {
      const NoteIter i=visual.first;
      const microseconds_t visual_end=visual.second;
      // This list is sorted by note start time.  The moment we encounter
      // a note scrolled off the window, we're done drawing
      if (i->start > current_time + show_duration)
        break;

      const Track::Mode mode = track_properties[i->track_id].mode;
      if (mode == Track::ModeNotPlayed || mode == Track::ModePlayedButHidden)
        continue;

      const int octave = (i->note_id / NotesPerOctave) - m_keyboard.GetStartingOctave();
      const int octave_base = i->note_id % NotesPerOctave;
      const int stack_offset = NoteToWhiteNoteOffset[octave_base];
      const bool is_black = IsBlackNote[octave_base];

      if (drawing_black != is_black)
        continue;

      const int octave_offset = (octave - 1) * WhiteNotesPerOctave;
      const int inner_octave_offset = (octave_base + stack_offset);
      const int generalized_black_offset = (is_black?black_offset:0);

      const double scaling_factor = static_cast<double>(y_offset) / static_cast<double>(show_duration);

      const long long roll_under = static_cast<int>(y_roll_under / scaling_factor);
      const long long adjusted_start = max(i->start - current_time, -roll_under);
      const long long adjusted_end   = max(visual_end-current_time, 0LL);

      if (adjusted_end < adjusted_start)
        continue;
      // After NoteOff, retain ONLY a graphical ghost for the release
      // window. The keyboard and MIDI engine have already released it.
      const bool releasing=m_render_3d &&
          i->start<=current_time &&
          NoteReleaseFade::Visible(current_time,visual_end);
      const float releaseFade=releasing?
          NoteReleaseFade::Gain(current_time,visual_end):1.0f;
      // Unlike a note that has not yet arrived, a released note must
      // disappear completely after the short smooth visual envelope.
      if(m_render_3d && visual_end<=current_time && !releasing)
        continue;

      // Convert our times to pixel coordinates
      const int y_end   = y - static_cast<int>(adjusted_start * scaling_factor) + y_offset;
      const int y_start = y - static_cast<int>(adjusted_end   * scaling_factor) + y_offset;

      const int start_x = (octave_offset + inner_octave_offset + keyboard_type_offset) * (white_width + key_space)
        + generalized_black_offset + x_offset;

      const int left = start_x - 1;
      int top = y_start;
      const int width = (is_black ? black_width : white_width) + 2;
      if ((start_x < x_offset) || (left + width > final_width + x_offset ))
        continue;
      int height = y_end - y_start;
      const bool sounding=(i->start<=current_time && i->end>current_time);
      if(m_cinematic_3d){
        // The front of a 3D note sinks into the piano on arrival; do NOT
        // pin a luminous head on the surface for the whole MIDI sustain.
        // The first frame is flush with the key; subsequent frames consume
        // the remaining mesh, without affecting NoteOn/NoteOff or the keys.
        const int limit=ShortNoteVisual::Maximum(white_width,m_visual.noteLength);
        const auto compact=(sounding||releasing)?
          ShortNoteVisual::Absorb(y+y_offset,limit,
             static_cast<float>((current_time-i->start)*.000001)):
          ShortNoteVisual::Fit(top,top+height,limit);
        top=compact.top;
        height=compact.height;
        if(height<=0)continue; // fully absorbed, still musically sustained
      }

      // Force a note to be a minimum height at all times
      // except when scrolling off underneath the keyboard and
      // coming in from the top of the screen.
      const bool hitting_bottom = (adjusted_start + current_time != i->start);
      const bool hitting_top    = (adjusted_end   + current_time != i->end);

      if (!hitting_bottom && !hitting_top) {
        while ( (height) < MinNoteHeight) height++;
      }

      const Track::TrackColor color = NoteColor(*i,
                      track_properties[i->track_id].color);
      const int &brush_id = (((i->state == UserMissed) || (i->retry_state == UserMissed)) ? Track::MissedNote : color);

      // The 3D column must stay narrower than the physical key, including
      // black keys; keep legacy 2D sizing and its Width control untouched.
      const auto thin3D=NoteColumnSizing::Fit3D(start_x,
            is_black?black_width:white_width,is_black,m_visual.width);
      const int newWidth=m_render_3d?thin3D.width:
            std::max(3,width*m_visual.width/100);
      const int newLeft=m_render_3d?thin3D.left:
            left+(width-newWidth)/2;
      if(m_render_3d){
        // Material continuo: el halo acompaña SOLO al contorno de la
        // cápsula. Las teclas negras y las 16 pistas conservan su color.
        // Color de la nota 3D invariable ante los ajustes de los efectos.
        // El canal MIDI sigue controlando el vidrio neon y su transparencia.
        const SDL_Color base=Track::ColorNoteWhite[brush_id];
        // In a full Blender scene, protect the performer from saturated,
        // opaque flame columns. Keep the keyboard-contact effect separate.
        const int brightness=std::max(20,std::min(
                      m_cinematic_3d?150:150,m_visual.brightness));
        const auto cap=[](int n)->Uint8{
          return static_cast<Uint8>(std::max(0,std::min(255,n)));
        };
        const SDL_Color neon={
          cap(base.r*brightness/125),
          cap(base.g*brightness/125),
          cap(base.b*brightness/125),255
        };
        // Tras tocar la línea del teclado, la nota MIDI permanece
        // visible hasta NoteOff, pero deja traslucir la tecla hundida.
        // El tramo que todavía cae conserva la opacidad elegida.
        const int noteOpacity=sounding?
            std::max(16,std::min(m_visual.opacity,58)):m_visual.opacity;
        // Optional low-opacity note waterfall in a close-up to avoid
        // blocking the face, without altering ordinary note mechanics.
        const int cinematicOpacity=noteOpacity; // user's opacity, not hidden 27% cap
        float visibleBirthAge=-1.0f;
        if(m_cinematic_3d && m_visual.effectBirth==1 &&
           !sounding && top+height>=y && top<y+y_offset){
          const auto identity=std::make_tuple(int(i->track_id),
                       int(i->note_id),i->start,visual_end);
          auto result=m_visible_births.emplace(identity,
                            std::make_pair(m_effect_time,m_effect_time));
          result.first->second.second=m_effect_time;
          visibleBirthAge=static_cast<float>(
                                m_effect_time-result.first->second.first);
        }
        renderer.DrawNeonCapsule(newLeft,top,newWidth,height,
            std::max(2,m_visual.radius),neon,
            cinematicOpacity,
            m_visual.glow, // restore actual fluorescence slider
            m_shadow_pass,
            static_cast<float>(m_effect_time +
                    i->note_id*.31),m_cinematic_3d,
            sounding?static_cast<float>((current_time-i->start)*.000001):-1.0f,
            // First visible display clock; works for song starts and seeks.
            visibleBirthAge,
            NoteParticleSeed(MidiEvent::NoteName(i->note_id)) ^
             static_cast<unsigned>(i->track_id*0x9e3779b9u) ^
             static_cast<unsigned>(i->start),
            std::max(y,top),releaseFade);
      }else if(m_shadow_pass){
        renderer.SetColor(255,255,255,int(m_visual.opacity*255/100*releaseFade));
        DrawNote(renderer,(drawing_black?tex_black:tex_white),
                 (drawing_black?BlackNoteDimensions:WhiteNoteDimensions),
                 newLeft,top,newWidth,height,brush_id);
      }else{
        const SDL_Color raw=drawing_black?Track::ColorNoteBlack[brush_id]:
                                            Track::ColorNoteWhite[brush_id];
        const int bright=m_visual.brightness;
        const int red=std::min(255,raw.r*bright/100);
        const int green=std::min(255,raw.g*bright/100);
        const int blue=std::min(255,raw.b*bright/100);
        const int rad=std::min({m_visual.radius,newWidth/2,
                                std::max(0,height/2)});
        if(m_visual.glow>0){
          renderer.SetColor(red,green,blue,int(std::max(8,m_visual.glow*70/100)*releaseFade));
          renderer.DrawQuad(newLeft-2,top-2,newWidth+4,height+4);
        }
        // Silueta redondeada, con alfa real y un borde iluminado.
        renderer.SetColor(red,green,blue,int(m_visual.opacity*255/100*releaseFade));
        if(rad==0)renderer.DrawQuad(newLeft,top,newWidth,height);
        else{
          renderer.DrawQuad(newLeft+rad,top,newWidth-2*rad,height);
          renderer.DrawQuad(newLeft,top+rad,newWidth,height-2*rad);
          for(int row=0;row<rad;row++){
            const int dx=static_cast<int>(rad-std::sqrt(
                static_cast<double>(rad*rad-(rad-row)*(rad-row))));
            renderer.DrawQuad(newLeft+dx,top+row,newWidth-2*dx,1);
            renderer.DrawQuad(newLeft+dx,top+height-1-row,newWidth-2*dx,1);
          }
        }
        renderer.SetColor(255,255,255,int(std::min(180,m_visual.opacity*255/100)*releaseFade));
        renderer.DrawQuad(newLeft+rad,top+1,std::max(1,newWidth-2*rad),1);
      }

      //const SDL_Color text_color (Renderer::ToColor(0x90,0x90,0x90));
      //TextWriter note_text(left + 3, y + height - 20, renderer, false, 11);
      //note_text << Text(STRING(i->note_id), text_color);
    }

    drawing_black = !drawing_black;
  }
}

void KeyboardDisplay::SetKeyActive(const string &key_name, bool active, Track::TrackColor color) {
  if (active)
    m_active_keys[key_name] = color;

  else
    m_active_keys.erase(key_name);
}

int KeyboardDisplay::GetNoteNameFontSize(int key_width) const
{
  return (key_width * Layout::NoteNameSize / 30);
}

