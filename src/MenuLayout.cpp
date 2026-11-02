// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "MenuLayout.h"
#include "TextWriter.h"
#include "Renderer.h"

using namespace std;

namespace Layout {

  void DrawTitle(Renderer &renderer, const string &title) {

    TextWriter title_writer(ScreenMarginX, ScreenMarginY - TitleFontSize - 10,
                            renderer, false, TitleFontSize);
    title_writer << title;
  }

  void DrawSubTitle(Renderer &renderer, const string &title) {

    TextWriter title_writer(ScreenMarginX, ScreenMarginY * 2 - TitleFontSize - 10,
                            renderer, false, TitleFontSize);
    title_writer << title;
  }

  void DrawHorizontalRule(Renderer &renderer, int state_width, int y) {

    renderer.SetColor(0x50, 0x50, 0x50);
    renderer.DrawQuad(ScreenMarginX, y - 1, state_width - 2*ScreenMarginX, 3);
  }

  void DrawButton(Renderer &renderer, const ButtonState &button, const Tga *tga) {

    const static SDL_Color color = Renderer::ToColor(0xE0,0xE0,0xE0);
    const static SDL_Color color_hover = Renderer::ToColor(0xFF,0xFF, 0xFF);

    // La textura ya trae las esquinas redondeadas y su propia sombra.
    // Oscurecer y desplazarla un píxel mientras se presiona, sin quads
    // adicionales que oculten esas esquinas o invadan el fondo.
    renderer.SetColor(button.depressed ?
                      Renderer::ToColor(0xAD,0xAD,0xAD) :
                      (button.hovering ? color_hover : color));
    renderer.DrawTga(tga, button.x, button.y+(button.depressed?1:0));
  }


  void DrawCompactButton(Renderer &renderer, const ButtonState &button,
                         const Tga *chrome, const std::string &caption,
                         bool selected, bool enabled, int fontSize) {
    if (!chrome || button.w < 30 || button.h < 18) return;
    // La textura de Canciones utiliza los mismos biseles, radios, sombra y
    // brillos de todos los botones clásicos de Linthesia. Su superficie
    // aprovechable es 176x46 dentro del atlas de 256x64.
    // Tres cortes: bordes sin deformar en anchura y una franja central SIN
    // texto, para que incluso los botones alargados tengan marco idéntico.
    const int edge = 14;
    const int y = button.y + (enabled && button.depressed ? 1 : 0);
    const int tint = !enabled ? 128 :
                     button.depressed ? 173 : button.hovering ? 255 : 224;
    renderer.SetColor(tint,tint,tint);
    renderer.DrawStretchedTga(chrome,button.x,y,edge,button.h,0,0,edge,46);
    renderer.DrawStretchedTga(chrome,button.x+edge,y,button.w-2*edge,button.h,
                              150,0,3,46);
    renderer.DrawStretchedTga(chrome,button.x+button.w-edge,y,edge,button.h,
                              162,0,edge,46);
    // LED de selección dentro de la superficie, no por encima del borde.
    if (selected && enabled) {
      renderer.SetColor(126,231,177);
      renderer.DrawQuad(button.x+edge, y+button.h-6,button.w-2*edge,2);
    }
    renderer.SetColor(255,255,255);
    TextWriter label(button.x+button.w/2,
                     y+std::max(1,(button.h-fontSize)/2-2),
                     renderer,true,fontSize,"FreeSerif.ttf");
    label << Text(caption,enabled ?
                    Renderer::ToColor(242,242,242):
                    Renderer::ToColor(153,153,153));
  }


} // End namespace Layout
