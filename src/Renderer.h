// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#ifndef __RENDERER_H
#define __RENDERER_H

#include "OSGraphics.h"
#include "Tga.h"
#include "PianoHands.h"
#include "SDL.h"
#include <algorithm>
#include <vector>


class Renderer {
public:
  Renderer(SDL_Window* sdl_window);

  static SDL_Color ToColor(int r, int g, int b, int a = 0xFF);

  void SwapBuffers();

  // 0 will disable vsync, 1 will enable.
  void SetVSyncInterval(int interval = 1);

  void SetOffset(int x, int y) {
    m_xoffset = x; m_yoffset = y;
  }

  void ResetOffset() {
    SetOffset(0,0);
  }

  void SetEffect3D(int speed,int count,int spread,int particleSize,int birthMode=1){
    m_effect_speed=std::max(25,std::min(speed,200));
    m_effect_count=std::max(0,std::min(count,144));
    m_effect_spread=std::max(0,std::min(spread,200));
    m_effect_size=std::max(25,std::min(particleSize,220));
    m_effect_birth=std::max(0,std::min(birthMode,1));
  }

  // Independent procedural particle controls; no proprietary SeeMusic code.
  void SetParticleStudio(int wind,int life,int turbulence,int initialVelocity,
                         int bloom,int opacity,int blend,int rays,int smoke,
                         int trails,int explosion,int flameHeight){
    m_effect_wind=std::max(0,std::min(250,wind));
    m_effect_life=std::max(25,std::min(250,life));
    m_effect_turbulence=std::max(0,std::min(200,turbulence));
    m_effect_initial_velocity=std::max(0,std::min(250,initialVelocity));
    m_effect_bloom=std::max(0,std::min(200,bloom));
    m_effect_opacity=std::max(0,std::min(100,opacity));
    m_effect_blend=std::max(0,std::min(1,blend));
    m_effect_rays=std::max(0,std::min(1,rays));
    m_effect_smoke=std::max(0,std::min(1,smoke));
    m_effect_trails=std::max(0,std::min(1,trails));
    m_effect_explosion=std::max(0,std::min(250,explosion));
    m_effect_flame_height=std::max(20,std::min(130,flameHeight));
  }
  void ForceTexture(unsigned int texture_id);

  void SetColor(SDL_Color c);
  void SetColor(int r, int g, int b, int a = 0xFF);
  void DrawQuad(int x, int y, int w, int h);
  // Rectángulo redondeado con degradado vertical por vértices OpenGL.
  // La misma proyección del piano-roll se aplica al cuerpo y su halo.
  void DrawRoundedGradient(int x,int y,int w,int h,int radius,
                           SDL_Color top,SDL_Color bottom);
  // Cápsula fosforescente con halo perimetral continuo y cuerpo cristalino.
  void DrawNeonCapsule(int x,int y,int w,int h,int radius,
                       SDL_Color base,int opacity,int glow,bool shadow,
                       float phase=0.0f,bool cinematic=false,float hitAge=-1.0f,
                       float birthAge=-1.0f,unsigned birthSeed=0,int birthTipY=-100000,
                       float releaseFade=1.0f);
  void DrawTopBirthBurst(int x,int y,int w,SDL_Color base,
                         float age,unsigned seed,int glow);
  // Particulas originales deterministas sincronizadas con el NoteOn real.
  // Se renderizan DESPUES de las teclas para conservar el relieve del piano.
  void DrawPianoImpactParticles(int cx,int keyTop,int keyWidth,float elapsed,
                                SDL_Color accent,unsigned seed,int strength,float animationTime=-1.0f,
                                float releaseFade=1.0f);
  void DrawPianoContactLightBar(int x,int y,int width,int glow);
  void DrawStageLights(int x,int y,int w,int h,int intensity,int tint,
                       double elapsed,int movement);
  // Genuine Blender mesh geometry with free 3D orbital camera.
  bool DrawBlenderScene3D(int scene,int pose,int x,int y,int width,int height,
                          int yaw,int pitch,int zoom,int lightLevel,int lightTint,
                          const PianoHands::Frame* liveHands=nullptr,
                          int cameraFocus=0,
                          int cameraMotion=0,float elapsedSeconds=0.f);
  // Tecla física en perspectiva con cara superior, biseles y frontal.
  // press: recorrido amortiguado [0,1]; cue: guía sin pulsar.
  void DrawPianoKey3D(int x,int y,int w,int h,bool black,
                      float press,bool cue,SDL_Color accent,float keyboardCenter);
  // Renderizado con las mallas originales generadas en Blender.
  void BeginBlenderKeyboard3D();
  void EndBlenderKeyboard3D();
  void BeginBlenderNotes3D();
  void EndBlenderNotes3D();




  // Retroalimentación común para botones gráficos y botones dibujados
  // por código. No interviene en el cálculo ni en los eventos de clic.
  void BeginPressFrame(int mouseX,int mouseY,bool leftHeld);
  void DrawPressFeedback();
  // Transformación proyectiva reversible, aplicada únicamente al piano-roll.
  void BeginPianoPerspective(float centerX,float topY,float bottomY,
                             int perspectiveStyle=0,
                             int depth=65,int angle=38,int horizon=30,
                             int stageWidth=100);
  void EndPianoPerspective();
  // Recorta exclusivamente la zona de notas: nunca sobre el teclado.
  void BeginPianoRollClip(int left,int top,int right,int bottom);
  void EndPianoRollClip();

  void DrawTga(const Tga *tga, int x, int y) const;
  void DrawTga(const Tga *tga, int x, int y, int width, int height,
               int src_x, int src_y) const;

  void DrawStretchedTga(const Tga *tga, int x, int y, int w, int h) const;
  void DrawStretchedTga(const Tga *tga, int x, int y, int w, int h,
                        int src_x, int src_y, int src_w, int src_h) const;


  /// @brief draws the tga centered limits to the width and height and keeping the aspect ratio of the tga
  /// @param tga the image to draw
  /// @param cx the center x of the tga to draw
  /// @param cy the center y of the tga to draw
  /// @param max_w the maximum width of the image
  /// @param max_h the maximum height of the image
  void DrawCenteredTga(const Tga *tga, int cx, int cy, int max_w, int max_h) const;

private:

  // NOTE: These are used externally by the friend
  // class TextWriter (along with the context)
  int m_xoffset;
  int m_yoffset;

  int m_effect_speed=100,m_effect_count=72,m_effect_spread=100,m_effect_size=100;
  int m_effect_wind=100,m_effect_life=100,m_effect_turbulence=85,m_effect_initial_velocity=100;
  int m_effect_bloom=115,m_effect_opacity=70,m_effect_blend=0;
  int m_effect_rays=1,m_effect_smoke=1,m_effect_trails=1;
  int m_effect_explosion=130,m_effect_flame_height=100;
  int m_effect_birth=1;
  SDL_Window* m_sdl_window;
  GLint m_old_scissor[4]={0,0,0,0};
  bool m_scissor_was_enabled=false;

  friend class Text;
  friend class TextWriter;
};

#endif // __RENDERER_H
