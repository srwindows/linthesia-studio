// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "LinthesiaError.h"
#include "StringUtil.h"

using namespace std;

string LinthesiaError::GetErrorDescription() const {

  switch (m_error) {
  case Error_StringSpecified: 
    return m_optional_string;

  case Error_BadPianoType: 
    return "Tipo de piano no válido.";

  case Error_BadGameState:
    return "Error interno: estado de juego no válido.";

  default:
    return STRING("Código de error de Linthesia desconocido (" << m_error << ").");
  }
}

