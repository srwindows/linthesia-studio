// -*- mode: c++; coding: utf-8 -*-

// Linthesia

#include "SongTile.h"
#include "TextWriter.h"
#include "Renderer.h"
#include "Tga.h"
#include <glib.h>

SongTile::SongTile(int x, int y, string path, string title, bool dir, Tga *frame_graphics) :
  m_x(x),
  m_y(y),
  m_path(path),
  m_name(title),
  m_dir(dir),
  m_frame_graphics(frame_graphics) {

  // Initialize the size and position of each button
  whole_tile = ButtonState(0, 0, SongTileWidth, SongTileHeight);
}

void SongTile::Update(const MouseInfo &translated_mouse) {
  whole_tile.Update(translated_mouse);
}

void SongTile::Draw(Renderer &renderer) const {

  renderer.SetOffset(m_x, m_y);

  const SDL_Color hover = Renderer::ToColor(0xFF,0xFF,0xFF);
  const SDL_Color no_hover = Renderer::ToColor(0xE0,0xE0,0xE0);
  renderer.SetColor(whole_tile.hovering ? hover : no_hover);
  renderer.DrawTga(m_frame_graphics, 0, 0);

  // Draw mode text
  TextWriter title(44, 49, renderer, false, Layout::ButtonFontSize);
  std::string shown=m_name;
  if(g_utf8_validate(shown.c_str(),shown.size(),nullptr) &&
     g_utf8_strlen(shown.c_str(),-1)>38){
    gchar* cut=g_utf8_substring(shown.c_str(),0,35);
    shown=std::string(cut)+"...";
    g_free(cut);
  }
  title << shown;

  renderer.ResetOffset();
}


// Compact buttons for child folders only; no change to the large
// graphical cards on the main screen.
void SongTile::DrawCompactFolder(Renderer& renderer,int width,int height) const{
    if(!m_dir||width<=0||height<=0)return;
    renderer.SetOffset(m_x,m_y);
    const bool hovering=whole_tile.hovering;
    renderer.SetColor(hovering?112:76,hovering?124:87,hovering?119:85);
    renderer.DrawQuad(0,0,width,height);
    renderer.SetColor(hovering?183:128,hovering?195:145,hovering?190:142);
    renderer.DrawQuad(0,0,width,1);
    renderer.DrawQuad(0,0,1,height);
    renderer.SetColor(40,47,47);
    renderer.DrawQuad(0,height-1,width,1);
    renderer.DrawQuad(width-1,0,1,height);
    const int maxChars=std::max(3,(width-16)/8);
    std::string text=m_name;
    if(g_utf8_validate(text.c_str(),text.size(),nullptr) &&
       g_utf8_strlen(text.c_str(),-1)>maxChars){
        gchar* shortened=g_utf8_substring(text.c_str(),0,maxChars-3);
        text=std::string(shortened)+"...";
        g_free(shortened);
    }
    renderer.SetColor(255,255,255);
    TextWriter title(width/2,(height-16)/2,renderer,true,16);
    title << Text(text,Renderer::ToColor(238,242,240));
    renderer.ResetOffset();
}

// Simple, single-column list: just the filename, one per line.
// No file-type or size columns, headers, musical/document icons or stripes.
void SongTile::DrawMidiRow(Renderer &renderer,int width,int /*row*/,bool selected) const {
    if(m_dir || width<=0)return;
    renderer.SetOffset(m_x,m_y);
    if(selected){
        renderer.SetColor(42,93,152);
        renderer.DrawQuad(0,0,width,29);
    }else if(whole_tile.hovering){
        renderer.SetColor(86,96,111);
        renderer.DrawQuad(0,0,width,29);
    }
    std::string label=m_name;
    const int maxChars=std::max(7,(width-25)/8);
    if(g_utf8_validate(label.c_str(),label.size(),nullptr) &&
       g_utf8_strlen(label.c_str(),-1)>maxChars){
        gchar* prefix=g_utf8_substring(label.c_str(),0,maxChars-3);
        label=std::string(prefix)+"...";
        g_free(prefix);
    }
    renderer.SetColor(255,255,255);
    TextWriter name(11,5,renderer,false,16);
    name << Text(label,Renderer::ToColor(selected?255:228,selected?255:232,selected?255:232));
    renderer.ResetOffset();
}
