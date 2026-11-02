// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include <netinet/in.h>

#include "MidiUtil.h"

using namespace std;

unsigned long BigToSystem32(unsigned long x) {
  return ntohl(x);
}

unsigned short BigToSystem16(unsigned short x) {
  return ntohs(x);
}

unsigned long parse_variable_length(istream &in) {
  register unsigned long value = in.get();

  if (in.good() && (value & 0x80) ) {
    value &= 0x7F;

    register unsigned long c;
    do {
      c = in.get();
      value = (value << 7) + (c & 0x7F);
    } while (in.good() && (c & 0x80) );
  }

  return(value);
}

string MidiError::GetErrorDescription() const {
  switch (m_error) {
  case MidiError_UnknownHeaderType:
    return "Cabecera desconocida.\n\nProbablemente no sea un archivo MIDI válido.";
  case MidiError_BadFilename:
    return "No se pudo abrir el archivo. Comprueba que exista.";
  case MidiError_NoHeader:
    return "No se puede leer la cabecera MIDI. Archivo demasiado corto.";
  case MidiError_BadHeaderSize:
    return "Tamaño de cabecera incorrecto.";
  case MidiError_Type2MidiNotSupported:
    return "No se admite MIDI de tipo 2.";
  case MidiError_BadType0Midi:
    return "Un MIDI de tipo 0 debe contener una sola pista.";
  case MidiError_SMTPETimingNotImplemented:
    return "No se admite la división temporal SMPTE.";

  case MidiError_BadTrackHeaderType:
    return "Tipo de cabecera de pista desconocido.";
  case MidiError_TrackHeaderTooShort:
    return "El archivo terminó antes de completar la cabecera de pista.";
  case MidiError_TrackTooShort:
    return "Datos insuficientes para leer toda la pista.";
  case MidiError_BadTrackEnd:
    return "La pista MIDI no termina con el evento de fin de pista.";

  case MidiError_EventTooShort:
    return "Los datos terminaron antes de completar un evento MIDI.";
  case MidiError_UnknownEventType:
    return "Se ha encontrado un tipo de evento MIDI desconocido.";
  case MidiError_UnknownMetaEventType:
    return "Tipo de metae­vento MIDI desconocido.";

  case MidiError_MM_NoDevice:
    return "No se pudo abrir el dispositivo MIDI seleccionado.";
  case MidiError_MM_NotEnabled:
    return "No se pudo activar el dispositivo MIDI.";
  case MidiError_MM_AlreadyAllocated:
    return "El dispositivo MIDI ya está en uso.";
  case MidiError_MM_BadDeviceID:
    return "El identificador del dispositivo MIDI está fuera de rango.";
  case MidiError_MM_InvalidParameter:
    return "Parámetro no válido para el dispositivo MIDI.";
  case MidiError_MM_NoDriver:
    return "El controlador MIDI seleccionado no está instalado.";
  case MidiError_MM_NoMemory:
    return "No se puede reservar memoria para el dispositivo MIDI.";
  case MidiError_MM_Unknown:
    return "Se ha producido un error desconocido de entrada/salida MIDI.";

  case MidiError_NoInputAvailable:
    return "Se intentó leer un evento MIDI de una entrada vacía.";
  case MidiError_MetaEventOnInput:
    return "La entrada MIDI ha enviado un metae­vento.";

  case MidiError_InputError:
    return "El controlador de entrada MIDI ha comunicado un error.";
  case MidiError_InvalidInputErrorBehavior:
    return "Valor no válido de InputError. Opciones: report, ignore y use.";

  case MidiError_RequestedTempoFromNonTempoEvent:
    return "Se solicitaron datos de tempo de un evento que no es de tempo.";

  default:
    return STRING("Código de error MIDI desconocido (" << m_error << ").");
  }
}

string GetMidiEventTypeDescription(MidiEventType type) {
  switch (type) {
  case MidiEventType_Meta:             return "Meta";
  case MidiEventType_SysEx:            return "Exclusivo del sistema";

  case MidiEventType_NoteOff:          return "Nota desactivada";
  case MidiEventType_NoteOn:           return "Nota activada";
  case MidiEventType_Aftertouch:       return "Presión posterior";
  case MidiEventType_Controller:       return "Controlador";
  case MidiEventType_ProgramChange:    return "Cambio de programa";
  case MidiEventType_ChannelPressure:  return "Presión de canal";
  case MidiEventType_PitchWheel:       return "Rueda de afinación";

  case MidiEventType_Unknown:          return "Desconocido";
  default:                             return "TIPO DE EVENTO NO VÁLIDO";
  }
}

string GetMidiMetaEventTypeDescription(MidiMetaEventType type) {
  switch (type) {
  case MidiMetaEvent_SequenceNumber:   return "Número de secuencia";

  case MidiMetaEvent_Text:             return "Text";
  case MidiMetaEvent_Copyright:        return "Derechos de autor";
  case MidiMetaEvent_TrackName:        return "Nombre de pista";
  case MidiMetaEvent_Instrument:       return "Instrumento";
  case MidiMetaEvent_Lyric:            return "Letra";
  case MidiMetaEvent_Marker:           return "Marcador";
  case MidiMetaEvent_Cue:              return "Punto de referencia";
  case MidiMetaEvent_PatchName:        return "Nombre de sonido";
  case MidiMetaEvent_DeviceName:       return "Nombre de dispositivo";

  case MidiMetaEvent_EndOfTrack:       return "Fin de pista";
  case MidiMetaEvent_TempoChange:      return "Cambio de tempo";
  case MidiMetaEvent_SMPTEOffset:      return "Desfase SMPTE";
  case MidiMetaEvent_TimeSignature:    return "Compás";
  case MidiMetaEvent_KeySignature:     return "Tonalidad";

  case MidiMetaEvent_Proprietary:      return "Propietario";

  case MidiMetaEvent_ChannelPrefix:    return "Prefijo de canal (obsoleto)";
  case MidiMetaEvent_MidiPort:         return "Puerto MIDI (obsoleto)";

  case MidiMetaEvent_Unknown:          return "Metaevento desconocido";
  default:                             return "METAEVENTO NO VÁLIDO";
  }
}

string const InstrumentNames[InstrumentCount] = {
  "Piano de cola acústico",
  "Piano acústico brillante",
  "Piano de cola eléctrico",
  "Piano honky-tonk",
  "Piano eléctrico 1",
  "Piano eléctrico 2",
  "Clave",
  "Clavinet",
  "Celesta",
  "Glockenspiel",
  "Caja de música",
  "Vibráfono",
  "Marimba",
  "Xilófono",
  "Campanas tubulares",
  "Dulcémele",
  "Órgano de tiradores",
  "Órgano de percusión",
  "Órgano rock",
  "Órgano de iglesia",
  "Órgano de lengüetas",
  "Acordeón",
  "Armónica",
  "Acordeón de tango",
  "Guitarra acústica de nailon",
  "Guitarra acústica de acero",
  "Guitarra eléctrica de jazz",
  "Guitarra eléctrica limpia",
  "Guitarra eléctrica apagada",
  "Guitarra con saturación",
  "Guitarra distorsionada",
  "Armónicos de guitarra",
  "Bajo acústico",
  "Bajo eléctrico con dedos",
  "Bajo eléctrico con púa",
  "Bajo sin trastes",
  "Bajo slap 1",
  "Bajo slap 2",
  "Bajo sintetizado 1",
  "Bajo sintetizado 2",
  "Violín",
  "Viola",
  "Violonchelo",
  "Contrabajo",
  "Cuerdas con trémolo",
  "Cuerdas en pizzicato",
  "Arpa orquestal",
  "Timbales",
  "Conjunto de cuerdas 1",
  "Conjunto de cuerdas 2",
  "Cuerdas sintéticas 1",
  "Cuerdas sintéticas 2",
  "Coro (ah)",
  "Coro (oh)",
  "Voz sintetizada",
  "Golpe orquestal",
  "Trompeta",
  "Trombón",
  "Tuba",
  "Trompeta con sordina",
  "Trompa",
  "Sección de metales",
  "Metales sintéticos 1",
  "Metales sintéticos 2",
  "Saxofón soprano",
  "Saxofón alto",
  "Saxofón tenor",
  "Saxofón barítono",
  "Oboe",
  "Corno inglés",
  "Fagot",
  "Clarinete",
  "Flautín",
  "Flauta travesera",
  "Flauta dulce",
  "Flauta de pan",
  "Botella soplada",
  "Shakuhachi",
  "Silbido",
  "Ocarina",
  "Solista 1 (onda cuadrada)",
  "Solista 2 (diente de sierra)",
  "Solista 3 (calíope)",
  "Solista 4 (soplo)",
  "Solista 5 (charang)",
  "Solista 6 (voz)",
  "Solista 7 (quintas)",
  "Solista 8 (bajo y solista)",
  "Colchón 1 (nueva era)",
  "Colchón 2 (cálido)",
  "Colchón 3 (polisintetizador)",
  "Colchón 4 (coro)",
  "Colchón 5 (arco)",
  "Colchón 6 (metálico)",
  "Colchón 7 (halo)",
  "Colchón 8 (barrido)",
  "Efecto 1 (lluvia)",
  "Efecto 2 (banda sonora)",
  "Efecto 3 (cristal)",
  "Efecto 4 (atmósfera)",
  "Efecto 5 (brillo)",
  "Efecto 6 (duendes)",
  "Efecto 7 (ecos)",
  "Efecto 8 (ciencia ficción)",
  "Sitar",
  "Banjo",
  "Shamisen",
  "Koto",
  "Kalimba",
  "Gaita",
  "Violín folk",
  "Shanai",
  "Campanillas",
  "Agogó",
  "Tambores metálicos",
  "Bloque de madera",
  "Tambor taiko",
  "Tom melódico",
  "Batería sintetizada",
  "Platillo invertido",
  "Ruido de trastes de guitarra",
  "Ruido de respiración",
  "Orilla del mar",
  "Canto de pájaros",
  "Timbre de teléfono",
  "Helicóptero",
  "Aplausos",
  "Disparo",

  //
  // NOTE: These aren't actually General MIDI instruments!
  //
  "Percusión", // for Tracks that use Channel 10 or 16
  "Varios instrumentos"     // for Tracks that use more than one
};
