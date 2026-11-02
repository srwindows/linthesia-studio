// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "DeviceTile.h"
#include "TextWriter.h"
#include "Renderer.h"
#include "Tga.h"

const static int GraphicWidth = 36;
const static int GraphicHeight = 36;

DeviceTile::DeviceTile(int x, int y, int device_id, DeviceTileType type,
                       const MidiCommDescriptionList &device_list,
                       Tga *button_graphics, Tga *frame_graphics) :
  m_x(x),
  m_y(y),
  m_preview_on(false),
  m_device_id(device_id),
  m_device_list(device_list),
  m_tile_type(type),
  m_button_graphics(button_graphics),
  m_frame_graphics(frame_graphics) {

  // Initialize the size and position of each button
  whole_tile = ButtonState(0, 0, DeviceTileWidth, DeviceTileHeight);
  button_mode_left  = ButtonState(  6, 38, GraphicWidth, GraphicHeight);
  button_mode_right = ButtonState(428, 38, GraphicWidth, GraphicHeight);
  button_preview    = ButtonState(469, 38, GraphicWidth, GraphicHeight);
}

void DeviceTile::Update(const MouseInfo &translated_mouse) {

  // Update the mouse state of each button
  whole_tile.Update(translated_mouse);
  button_preview.Update(translated_mouse);
  button_mode_left.Update(translated_mouse);
  button_mode_right.Update(translated_mouse);

  if (m_device_list.size() > 0) {
    const int last_device = static_cast<int>(m_device_list.size() - 1);

    if (button_mode_left.hit) {
      if (m_device_id == -1)
        m_device_id = last_device;

      else
        --m_device_id;
    }

    if (button_mode_right.hit) {
      if (m_device_id == last_device)
        m_device_id = -1;

      else
        ++m_device_id;
    }
  }

  if (button_preview.hit)
    m_preview_on = !m_preview_on;
  
}

int DeviceTile::LookupGraphic(TrackTileGraphic graphic, bool button_hovering) const {

  // There are three sets of graphics
  // set 0: window lit, hovering
  // set 1: window lit, not-hovering
  // set 2: window unlit, (implied not-hovering)
  int graphic_set = 2;
  if (whole_tile.hovering)
    graphic_set--;

  if (button_hovering)
    graphic_set--;

  const int set_offset = GraphicWidth * Graphic_COUNT;
  const int graphic_offset = GraphicWidth * graphic;

  return (set_offset * graphic_set) + graphic_offset;
}

void DeviceTile::Draw(Renderer &renderer) const {

  renderer.SetOffset(m_x, m_y);

  const SDL_Color hover = Renderer::ToColor(0xFF,0xFF,0xFF);
  const SDL_Color no_hover = Renderer::ToColor(0xE0,0xE0,0xE0);
  renderer.SetColor(whole_tile.hovering ? hover : no_hover);
  renderer.DrawTga(m_frame_graphics, 0, 0);

  // Choose the last (gray) color in the TrackTile bitmap
  int color_offset = GraphicHeight * Track::UserSelectableColorCount;

  renderer.DrawTga(m_button_graphics, BUTTON_RECT(button_mode_left),
                   LookupGraphic(GraphicLeftArrow,  button_mode_left.hovering),
                   color_offset);

  renderer.DrawTga(m_button_graphics, BUTTON_RECT(button_mode_right),
                   LookupGraphic(GraphicRightArrow, button_mode_right.hovering),
                   color_offset);
  
  TrackTileGraphic preview_graphic = GraphicPreviewTurnOn;
  if (m_preview_on)
    preview_graphic = GraphicPreviewTurnOff;

  renderer.DrawTga(m_button_graphics, BUTTON_RECT(button_preview),
                   LookupGraphic(preview_graphic, button_preview.hovering),
                   color_offset);

  // Draw mode text
  TextWriter mode(44, 49, renderer, false, 14);
  if (m_device_list.size() == 0)
    mode << "[No se encontraron dispositivos]";

  else {

    // A -1 for device_id means "disabled"
    if (m_device_id >= 0) {
      // Keep the actual ALSA device name unchanged for MIDI routing;
      // localize only the generic system-provided display label.
      std::string device_name = m_device_list[m_device_id].name;
      const std::string generic_port = "Synth input port";
      if (device_name.compare(0, generic_port.size(), generic_port) == 0)
        device_name.replace(0, generic_port.size(), "Puerto de sintetizador");
      mode << device_name;
    }

    else {
      switch (m_tile_type) {
      case DeviceTileOutput:
        mode << "[Sin salida: solo visualización, sin sonido]";
        break;

      case DeviceTileInput:
        mode << "[Sin entrada: acompañamiento sin puntuación]";
        break;
      }
    }
  }

  renderer.ResetOffset();
}

void DeviceTile::DrawReadOnly(Renderer &renderer) const {
  // Conservar el dibujo y los valores del recuadro original.
  renderer.SetOffset(m_x,m_y);
  renderer.SetColor(Renderer::ToColor(0xE0,0xE0,0xE0));
  renderer.DrawTga(m_frame_graphics,0,0);
  TextWriter mode(44,49,renderer,false,14);
  if(m_device_id>=0 && m_device_id<static_cast<int>(m_device_list.size())){
    std::string name=m_device_list[m_device_id].name;
    const std::string prefix="Synth input port";
    if(name.compare(0,prefix.size(),prefix)==0)
      name.replace(0,prefix.size(),"Puerto de sintetizador");
    mode << name;
  }else if(m_tile_type==DeviceTileInput){
    mode << "[Sin entrada: acompañamiento sin puntuación]";
  }else{
    mode << "[Sin salida: solo visualización, sin sonido]";
  }
  renderer.ResetOffset();
}

void DeviceTile::ReplaceDeviceList(const MidiCommDescriptionList &device_list)
{
  if (m_device_id != -1)
  {
      // Try to find currently selected device amoung devices in the new list
      std::string current_name = m_device_list[m_device_id].name;
      int device_id = 0;
      bool replaced = false;
      int size = device_list.size();
      for (; device_id < size; device_id++)
      {
        std::string iter_name = device_list[device_id].name;
        if (current_name == iter_name)
        {
          // Found device
          m_device_id = device_id;
          replaced = true;
          break;
        }
      }
      // The selected device was removed
      if (!replaced)
        m_device_id = -1;
  }
  m_device_list = device_list;
}

