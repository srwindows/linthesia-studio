// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#ifndef __TRACK_PROPERTIES_H
#define __TRACK_PROPERTIES_H

#include "Renderer.h"

namespace Track {

  // See ModeText in TrackTile.cpp for titles
  // See TractSelectionState.cpp for description
  enum Mode {

    ModePlayedAutomatically,
    ModeYouPlay,
    ModeYouPlaySilently,
    ModeLearning,
    ModeLearningSilently,
    ModePlayedButHidden,
    ModeNotPlayed,

    ModeCount
  };

  // Based on the Open Source icon theme "Tango" color scheme
  // with a few changes.  (e.g. Chameleon NoteBlack is a little
  // darker to distinguish it from NoteWhite, ScarletRed is a
  // little brighter to make it easier on the eyes, etc.)
  // 16 colores de pista, uno para cada canal/parte, y dos estados internos.
  // Mantener índices y colores de los seis originales por compatibilidad.
  const static int UserSelectableColorCount = 16;
  const static int ColorCount = UserSelectableColorCount + 2;

  enum TrackColor {

    TangoSkyBlue = 0,
    TangoChameleon,
    TangoOrange,
    TangoButter,
    TangoPlum,
    TangoScarletRed,
    Cyan,
    Turquoise,
    Emerald,
    Lime,
    Coral,
    Pink,
    Fuchsia,
    Indigo,
    Copper,
    Olive,

    FlatGray,
    MissedNote
  };

  const static SDL_Color ColorNoteWhite[ColorCount] = {
    { 114, 159, 207, 0xFF },
    { 138, 226,  52, 0xFF },
    { 252, 175,  62, 0xFF },
    { 252, 235,  87, 0xFF },
    { 173, 104, 180, 0xFF },
    { 238,  94,  94, 0xFF },

    {  82, 208, 237, 0xFF },
    {  68, 212, 189, 0xFF },
    {  65, 188, 123, 0xFF },
    { 198, 242,  69, 0xFF },
    { 255, 141, 109, 0xFF },
    { 251, 152, 192, 0xFF },
    { 223, 112, 217, 0xFF },
    { 147, 130, 244, 0xFF },
    { 204, 144,  88, 0xFF },
    { 173, 187,  85, 0xFF },

    {  90,  90,  90, 0xFF },
    {  60,  60,  60, 0xFF }
  };

  const static SDL_Color ColorNoteHit[ColorCount] = {
    { 192, 222, 255, 0xFF },
    { 203, 255, 152, 0xFF },
    { 255, 216, 152, 0xFF },
    { 255, 247, 178, 0xFF },
    { 255, 218, 251, 0xFF },
    { 255, 178, 178, 0xFF },

    { 171, 244, 255, 0xFF },
    { 169, 244, 221, 0xFF },
    { 156, 237, 174, 0xFF },
    { 232, 255, 157, 0xFF },
    { 255, 203, 178, 0xFF },
    { 255, 211, 234, 0xFF },
    { 253, 188, 244, 0xFF },
    { 202, 192, 255, 0xFF },
    { 234, 193, 144, 0xFF },
    { 213, 226, 133, 0xFF },

    { 180, 180, 180, 0xFF },
    {  60,  60,  60, 0xFF }
  };

  const static SDL_Color ColorNoteBlack[ColorCount] = {
    {  52, 101, 164, 0xFF },
    {  86, 157,  17, 0xFF },
    { 245, 121,   0, 0xFF },
    { 218, 195,   0, 0xFF },
    { 108,  76, 113, 0xFF },
    { 233,  49,  49, 0xFF },

    {   0, 117, 151, 0xFF },
    {   0, 126, 112, 0xFF },
    {  16, 110,  63, 0xFF },
    { 127, 154,   8, 0xFF },
    { 187,  68,  50, 0xFF },
    { 183,  70, 120, 0xFF },
    { 145,  40, 141, 0xFF },
    {  75,  62, 166, 0xFF },
    { 123,  77,  37, 0xFF },
    { 101, 117,  40, 0xFF },

    {  90,  90,  90, 0xFF },
    {  60,  60,  60, 0xFF }
  };

  struct Properties {

    Properties() :
      mode(ModeNotPlayed),
      color(TangoSkyBlue),
      is_retry_on(false) {
    }

    Mode mode;
    TrackColor color;
    bool is_retry_on;
    int instrument_program=-1; // -1: usar el del MIDI
    int instrument_bank=0;
  };

}; // end namespace

#endif // __TRACK_PROPERTIES_H
