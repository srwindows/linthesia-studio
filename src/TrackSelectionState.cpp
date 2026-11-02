// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "TrackSelectionState.h"

#include "TitleState.h"
#include "PlayingState.h"
#include "MenuLayout.h"
#include "Renderer.h"
#include "Textures.h"

#include "libmidi/Midi.h"
#include "libmidi/MidiUtil.h"
#include "MidiComm.h"
#include "TrackSelectionPersistence.h"
#include "AdvancedSettings.h"
#include "HandPractice.h"
#include "HandAutoDetector.h"
#include "HandCorrectionStore.h"
#include "MusicalReviewStore.h"
#include <algorithm>
#include <sstream>

using namespace std;

TrackSelectionState::TrackSelectionState(const SharedState &state) :
  m_page_count(0),
  m_current_page(0),
  m_tiles_per_page(0),
  m_preview_on(false),
  m_first_update_after_seek(false),
  m_preview_track_id(0),
  m_state(state) {
}

void TrackSelectionState::Init() {

  m_detected_hands_summary.clear();
  // Keep the selection shared with the player and the existing settings
  // window. Never make it a disconnected, decorative UI toggle.
  m_practice_hand=AdvancedSettingsIO::Load().practiceHands;
  m_color_palette_tile=-1;
  m_detector_open=false;
  m_instrument_tile=-1;
  m_soundfont_presets=FluidEngine::Presets();
  for(int i=0;i<3;++i)m_hand_buttons[i]=HandChoiceBounds(i);
  m_detect_button=AutoDetectHandsBounds();
  for(int i=0;i<11;++i)m_modal_buttons[i]=DetectorModalControl(i);
  m_you_play_silent_pulse=0;
  m_learning_silent_pulse=0;
  if (m_state.midi_out)
    m_state.midi_out->Reset();

  Midi &m = *m_state.midi;
  // A new session has no in-memory preferences. Restore choices from the
  // actual song, not a global default or another song's last selection.
  if(m_state.track_properties.size()!=m.Tracks().size()){
    std::vector<TrackSelectionPersistence::Choice> saved;
    int savedPage=0;
    if(TrackSelectionPersistence::Load(m_state.song_path,
                 m.Tracks().size(),saved,savedPage)){
      m_state.track_properties.resize(saved.size());
      for(size_t j=0;j<saved.size();++j){
        m_state.track_properties[j].mode=static_cast<Track::Mode>(saved[j].mode);
        m_state.track_properties[j].color=static_cast<Track::TrackColor>(saved[j].color);
        m_state.track_properties[j].is_retry_on=saved[j].retry;
        m_state.track_properties[j].instrument_program=saved[j].program;
        m_state.track_properties[j].instrument_bank=saved[j].bank;
      }
      m_current_page=savedPage;
    }
  }

  // Prepare a very simple count of the playable tracks first
  int track_count = 0;
  for (size_t i = 0; i < m.Tracks().size(); ++i) {
    if (m.Tracks()[i].Notes().size())
      track_count++;
  }

  int w = (GetStateWidth() - 2 * Layout::ScreenMarginX-6*5)/7;
  m_apply_all_not_played       = ButtonState(
                                    Layout::ScreenMarginX,
                                    112 - Layout::ButtonHeight/2,
                                    w, Layout::ButtonHeight);
  m_apply_all_played_auto      = ButtonState(
                                    Layout::ScreenMarginX + w + 5,
                                    112 - Layout::ButtonHeight/2,
                                    w, Layout::ButtonHeight);
  m_apply_all_you_play         = ButtonState(
                                    Layout::ScreenMarginX + (w + 5) * 2,
                                    112 - Layout::ButtonHeight/2,
                                    w, Layout::ButtonHeight);
  m_apply_all_you_play_silent  = ButtonState(
                                    Layout::ScreenMarginX + (w + 5) * 3,
                                    112 - Layout::ButtonHeight/2,
                                    w, Layout::ButtonHeight);
  m_apply_all_learning         = ButtonState(
                                    Layout::ScreenMarginX + (w + 5) * 4,
                                    112 - Layout::ButtonHeight/2,
                                    w, Layout::ButtonHeight);
  m_apply_all_learning_silent  = ButtonState(
                                    Layout::ScreenMarginX + (w + 5) * 5,
                                    112 - Layout::ButtonHeight/2,
                                    w, Layout::ButtonHeight);
  m_apply_all_played_hidden    = ButtonState(
                                    Layout::ScreenMarginX + (w + 5) * 6,
                                    112 - Layout::ButtonHeight/2,
                                    w, Layout::ButtonHeight);

  m_back_button = ButtonState(
    Layout::ScreenMarginX,
    GetStateHeight() - Layout::ScreenMarginY/2 - Layout::ButtonHeight/2,
    Layout::ButtonWidth, Layout::ButtonHeight);

  // Export setup belongs to track selection, not to the performing stage.
  // Same baseline as «Volver al inicio», outside any instrument card.
  m_record_video_button=ButtonState(
    m_back_button.x+m_back_button.w+18,m_back_button.y,238,Layout::ButtonHeight);

  m_continue_button = ButtonState(
    GetStateWidth() - Layout::ScreenMarginX - Layout::ButtonWidth,
    GetStateHeight() - Layout::ScreenMarginY/2 - Layout::ButtonHeight/2,
    Layout::ButtonWidth, Layout::ButtonHeight);

  // Determine how many track tiles we can fit
  // horizontally and vertically. Integer division
  // helps us round down here.
  int tiles_across = (GetStateWidth() + Layout::ScreenMarginX) /
    (TrackTileWidth + Layout::ScreenMarginX);

  tiles_across = max(tiles_across, 1);

  int tiles_down = (GetStateHeight() - 60 - Layout::ScreenMarginX - Layout::ScreenMarginY * 2) /
    (TrackTileHeight + Layout::ScreenMarginX);

  tiles_down = max(tiles_down, 1);

  // Calculate how many pages of tracks there will be
  m_tiles_per_page = tiles_across * tiles_down;

  m_page_count        = track_count / m_tiles_per_page;
  const int remainder = track_count % m_tiles_per_page;
  if (remainder > 0)
    m_page_count++;
  m_page_count=std::max(1,m_page_count);
  m_current_page=std::max(0,std::min(m_current_page,m_page_count-1));

  // If we have fewer than one row of tracks, just
  // center the tracks we do have
  if (track_count < tiles_across)
    tiles_across = track_count;

  // Determine how wide that many track tiles will
  // actually be, so we can center the list
  int all_tile_widths = tiles_across * TrackTileWidth + (tiles_across-1) * Layout::ScreenMarginX;
  int global_x_offset = (GetStateWidth() - all_tile_widths) / 2;

  const static int starting_y = 180;

  int tiles_on_this_line = 0;
  int tiles_on_this_page = 0;
  int current_y = starting_y;
  
  m_track_tiles.clear();
  
  for (size_t i = 0; i < m.Tracks().size(); ++i) {

    const MidiTrack &t = m.Tracks()[i];
    if (t.Notes().size() == 0)
      continue;

    int x = global_x_offset + (TrackTileWidth + Layout::ScreenMarginX)*tiles_on_this_line;
    int y = current_y;

    Track::Mode mode = Track::ModePlayedAutomatically;
    bool is_retry_on = false;
    if (t.IsPercussion())
      mode = Track::ModePlayedButHidden;

    Track::TrackColor color = static_cast<Track::TrackColor>((m_track_tiles.size()) % Track::UserSelectableColorCount);

    // If we came back here from StatePlaying, reload all our preferences
    if (m_state.track_properties.size() > i) {

      color = m_state.track_properties[i].color;
      mode = m_state.track_properties[i].mode;
      is_retry_on = m_state.track_properties[i].is_retry_on;
    }

    TrackTile tile(x, y, i, color, mode, is_retry_on);

    if(i<m_state.track_properties.size() &&
       m_state.track_properties[i].instrument_program>=0){
      const auto& prop=m_state.track_properties[i];
      tile.SetInstrument(prop.instrument_bank,prop.instrument_program,
                         InstrumentPresetName(prop.instrument_bank,
                                              prop.instrument_program));
    }
    m_track_tiles.push_back(tile);


    tiles_on_this_line++;
    tiles_on_this_line %= tiles_across;

    if (!tiles_on_this_line)
      current_y += TrackTileHeight + Layout::ScreenMarginX;


    tiles_on_this_page++;
    tiles_on_this_page %= m_tiles_per_page;

    if (!tiles_on_this_page) {
      current_y = starting_y;
      tiles_on_this_line = 0;
    }
  }
}

void TrackSelectionState::Resize() {
    // Preserve currently edited selections while rebuilding tile geometry.
    if(!m_track_tiles.empty())m_state.track_properties=BuildTrackProperties();
    Init();
}

vector<Track::Properties> TrackSelectionState::BuildTrackProperties() const {

  vector<Track::Properties> props;
  for (size_t i = 0; i < m_state.midi->Tracks().size(); ++i) {
    props.push_back(Track::Properties());
  }

  // Populate it with the tracks that have notes
  for (vector<TrackTile>::const_iterator i = m_track_tiles.begin(); i != m_track_tiles.end(); ++i) {
    props[i->GetTrackId()].color = i->GetColor();
    props[i->GetTrackId()].mode = i->GetMode();
    props[i->GetTrackId()].is_retry_on = i->IsRetryOn();
    props[i->GetTrackId()].instrument_bank=i->InstrumentBank();
    props[i->GetTrackId()].instrument_program=i->InstrumentProgram();
  }

  return props;
}

void TrackSelectionState::PersistCurrent(){
  if(m_state.song_path.empty()||!m_state.midi)return;
  const auto props=BuildTrackProperties();
  std::vector<TrackSelectionPersistence::Choice> saved;
  std::ostringstream signature;
  signature << m_current_page << ':' << props.size();
  for(const auto& p:props){
    const int mode=static_cast<int>(p.mode);
    const int color=static_cast<int>(p.color);
    signature << ',' << mode << '/' << color << '/' << int(p.is_retry_on)
              << '/' << p.instrument_bank << '/' << p.instrument_program;
    saved.push_back({mode,color,p.is_retry_on,p.instrument_program,p.instrument_bank});
  }
  if(signature.str()!=m_last_persisted &&
     TrackSelectionPersistence::Save(m_state.song_path,saved,m_current_page))
    m_last_persisted=signature.str();
}

ButtonState TrackSelectionState::ColorPaletteBounds() const {
  // Cuatro columnas por cuatro filas: exactamente dieciséis colores.
  const int width=352, height=291;
  if (m_color_palette_tile<0 ||
      static_cast<size_t>(m_color_palette_tile)>=m_track_tiles.size())
    return ButtonState(0,0,0,0);

  const TrackTile &tile=m_track_tiles[m_color_palette_tile];
  const ButtonState icon=tile.ButtonColor();
  const int anchorX=tile.GetX()+icon.x+icon.w/2;
  int left=anchorX-width/2;
  left=std::max(8,std::min(left,GetStateWidth()-width-8));
  int top=tile.GetY()+TrackTileHeight+7;
  if(top+height>GetStateHeight()-Layout::ScreenMarginY-6)
    top=tile.GetY()-height-7;
  top=std::max(8,std::min(top,GetStateHeight()-height-8));
  return ButtonState(left,top,width,height);
}

int TrackSelectionState::ColorPaletteHit(int x,int y) const {
  const ButtonState panel=ColorPaletteBounds();
  if(!panel.w)return -1;
  const int startX=panel.x+17,startY=panel.y+38;
  const int swatchW=72,swatchH=51,gap=9;
  for(int i=0;i<Track::UserSelectableColorCount;++i){
    const int cellX=startX+(i%4)*(swatchW+gap);
    const int cellY=startY+(i/4)*(swatchH+gap);
    if(x>=cellX && x<cellX+swatchW &&
       y>=cellY && y<cellY+swatchH)return i;
  }
  return -1;
}

ButtonState TrackSelectionState::HandChoiceBounds(int button) const {
  const int width=std::max(72,std::min(108,(GetStateWidth()-420)/3));
  const int spacing=5;
  const int start=GetStateWidth()-16-(width*3+spacing*2);
  return ButtonState(start+button*(width+spacing),142,width,28);
}

ButtonState TrackSelectionState::AutoDetectHandsBounds() const {
  const auto group=HandChoiceBounds(0);
  const int width=145;
  // On compact screens shorten the heading, never overlap it.
  return ButtonState(group.x-width-77,142,width,28);
}

ButtonState TrackSelectionState::DetectorModalBounds() const {
  const int width=std::min(720,std::max(440,GetStateWidth()-28));
  const int height=525;
  return ButtonState((GetStateWidth()-width)/2,
                     (GetStateHeight()-height)/2,width,height);
}

ButtonState TrackSelectionState::DetectorModalControl(int id) const {
  const auto p=DetectorModalBounds();
  switch(id){
  case 0:return ButtonState(p.x+p.w-43,p.y+11,28,27); // close
  case 1:return ButtonState(p.x+145,p.y+89,195,32); // piano
  case 2:return ButtonState(p.x+349,p.y+89,195,32); // all
  case 3:return ButtonState(p.x+145,p.y+147,195,32); // auto
  case 4:return ButtonState(p.x+349,p.y+147,195,32); // manual
  case 5:return ButtonState(p.x+220,p.y+205,43,33); // -
  case 6:return ButtonState(p.x+420,p.y+205,43,33); // +
  case 7:return ButtonState(p.x+22,p.y+p.h-56,190,38); // preview
  case 8:return ButtonState(p.x+p.w-234,p.y+p.h-56,130,38); // apply
  case 9:return ButtonState(p.x+p.w-95,p.y+p.h-56,77,38); // cancel
  case 10:return ButtonState(p.x+220,p.y+p.h-56,210,38); // restore approved notes
  default:return ButtonState(0,0,0,0);
  }
}

void TrackSelectionState::OpenHandsDetector(){
  m_detector_open=true;
  // The modal's ButtonState instances use the same state transitions
  // as all menu controls and are reinitialized for a new modal session.
  for(int i=0;i<11;++i)m_modal_buttons[i]=DetectorModalControl(i);
  m_detector_source_piano=true;
  m_detector_automatic=true;
  m_detector_split=AdvancedSettingsIO::Load().handSplitPoint;
  m_detector_preview_split=m_detector_split;
  m_detector_preview_ready=false;
  m_review_proposals.clear();m_review_checked.clear();m_review_page=0;
  m_detector_left=m_detector_right=0;
  m_detector_feedback="Elige el método y pulsa Analizar para ver las notas.";
}

void TrackSelectionState::HandleDetectorModalClick(int id){
  if(id<0||id>10)return;
  if((id==5||id==6)&&m_detector_automatic)return;
  if(id==8&&!m_detector_preview_ready)return;
    if(id==0||id==9){m_detector_open=false;return;}
    if(id==1||id==2){
      const bool piano=(id==1);
      if(m_detector_source_piano!=piano){
        m_detector_source_piano=piano;
        m_detector_preview_ready=false;
        m_detector_feedback="Origen cambiado. Pulsa Analizar.";
      }
    }else if(id==3||id==4){
      const bool automatic=(id==3);
      if(m_detector_automatic!=automatic){
        m_detector_automatic=automatic;
        m_detector_preview_ready=false;
        m_detector_feedback="Modo cambiado. Pulsa Analizar.";
      }
    }else if(id==5||id==6){
      if(!m_detector_automatic){
        m_detector_split=std::max(48,std::min(84,m_detector_split+
                                              (id==5?-1:1)));
        m_detector_preview_ready=false;
        m_detector_feedback="Corte cambiado. Pulsa Analizar.";
      }
    }else if(id==7){
      DetectHandsFromSelectedTracks();
    }else if(id==8){
      ApplyHandsDetection();
    }else if(id==10){
      if(MusicalReviewStore::Save(m_state.song_path,{})){
        m_review_checked.assign(m_review_proposals.size(),false);
        m_detector_feedback="Correcciones de notas retiradas; se mantienen las manos.";
        m_hand_status=m_detector_feedback;
        m_hand_status_until=SDL_GetTicks()+3500;
      }else{
        m_detector_feedback="No se pudieron restaurar las notas; revisa permisos.";
      }
    }
}

void TrackSelectionState::DetectHandsFromSelectedTracks(){
  m_detector_preview_ready=false;
  if(!m_state.midi)return;
  const auto& tracks=m_state.midi->Tracks();
  std::vector<bool> chosen(tracks.size(),false);
  bool pianoSelected=false;
  // Source selection is explicit. Never include muted tracks or drums.
  for(const auto& tile:m_track_tiles){
    const size_t i=tile.GetTrackId();
    if(i>=tracks.size()||tracks[i].IsPercussion()||
       tile.GetMode()==Track::ModeNotPlayed ||
       tile.GetMode()==Track::ModePlayedButHidden)continue;
    const bool piano=tracks[i].InstrumentName().find("Piano")!=std::string::npos;
    if(piano)pianoSelected=true;
    if(!m_detector_source_piano||piano)chosen[i]=true;
  }
  auto found=HandAutoDetector::Detect(m_state.midi->Notes(),chosen,
                       m_detector_split,
                       m_detector_source_piano && pianoSelected,
                       m_detector_automatic);
  if(!found.success){
    m_detector_feedback=m_detector_source_piano?
       "No hay pistas de piano activas con notas. Elige Todas activas.":
       "No hay notas en las pistas activas. Selecciona una pista primero.";
    return;
  }
  // Preview exactly the classification played by the existing engine,
  // including saved corrections, without writing to its MIDI file.
  HandPractice::Classifier classifier;
  classifier.Build(m_state.midi->Notes(),found.splitPoint);
  const auto annotation=HandCorrectionStore::LoadAll(m_state.song_path);
  for(const auto& change:annotation.hands)
    classifier.SetCorrection(change.track,change.pitch,
                             change.when,change.hand);
  found.left=found.right=0;
  for(const auto& note:m_state.midi->Notes()){
    if(note.track_id>=chosen.size()||!chosen[note.track_id]||
       note.note_id>127||note.channel==9)continue;
    if(classifier.ForNote(note)==HandPractice::Left)++found.left;
    else ++found.right;
  }
  m_detector_preview_ready=true;
  m_detector_preview_split=found.splitPoint;
  m_detector_left=found.left;
  m_detector_right=found.right;
  // Asistencia musical verificable: intervalos, contexto tonal, duración y
  // voz. No es una red neuronal entrenada; jamás se aplica sin aprobación.
  m_review_proposals=MusicalReasoner::Analyze(m_state.midi->Notes(),chosen,
                                      m_state.midi->GetCurrentKeyAccidentals());
  m_review_checked.assign(m_review_proposals.size(),false);
  m_review_page=0;
  m_detector_feedback="Notas sospechosas: "+std::to_string(m_review_proposals.size())+
            ". Selecciona solo las correcciones que consideres musicales.";
}

void TrackSelectionState::ApplyHandsDetection(){
  if(!m_detector_preview_ready){
    m_detector_feedback="Pulsa Analizar primero; puedes cancelar sin cambios.";
    return;
  }
  AdvancedSettings updated=AdvancedSettingsIO::Load();
  // Persist approved suggestions in a separate reversible sidecar. Never
  // modify the original MIDI, and preserve older explicit approvals.
  auto saved=MusicalReviewStore::Load(m_state.song_path);
  size_t applied=0;
  for(size_t i=0;i<m_review_proposals.size();++i){
    if(!m_review_checked[i])continue;
    const auto& proposal=m_review_proposals[i];
    saved.erase(std::remove_if(saved.begin(),saved.end(),
       [&](const MusicalReasoner::Suggestion& s){
         return s.track==proposal.track && s.original==proposal.original &&
                s.when==proposal.when;
       }),saved.end());
    saved.push_back(proposal);++applied;
  }
  if(applied && !MusicalReviewStore::Save(m_state.song_path,saved)){
    m_detector_feedback="No se pudieron guardar las correcciones; no se aplicó nada.";
    return;
  }
  updated.handSplitPoint=m_detector_preview_split;
  std::string error;
  if(!AdvancedSettingsIO::Save(updated,error)){
    m_detector_feedback="No se pudo guardar la separación: "+error;
    return;
  }
  m_detected_hands_summary="Detección: izquierda "+
       std::to_string(m_detector_left)+" / derecha "+
       std::to_string(m_detector_right)+" · corte MIDI "+
       std::to_string(m_detector_preview_split)+
       " · propuestas aprobadas "+std::to_string(applied);
  m_hand_status=m_detected_hands_summary;
  m_hand_status_until=SDL_GetTicks()+3500;
  m_detector_open=false;
}

std::string TrackSelectionState::InstrumentPresetName(int bank,int program) const{
  for(const auto& preset:m_soundfont_presets)
    if(preset.bank==bank&&preset.program==program)return preset.name;
  return "Banco "+std::to_string(bank)+" · "+std::to_string(program+1);
}

void TrackSelectionState::OpenInstrumentPicker(int tile){
  if(tile<0||size_t(tile)>=m_track_tiles.size())return;
  if(!m_state.midi_out || !m_state.midi_out->IsInternalFluid()){
    m_hand_status="Selecciona FluidSynth interno como salida MIDI para usar el SF2.";
    m_hand_status_until=SDL_GetTicks()+4200;
    return;
  }
  m_soundfont_presets=FluidEngine::Presets();
  if(m_soundfont_presets.empty()){
    m_hand_status="No hay presets cargados: comprueba el SF2 en Configuración > Audio.";
    m_hand_status_until=SDL_GetTicks()+4200;
    return;
  }
  m_instrument_tile=tile;
  m_instrument_page=0;
  const auto& t=m_track_tiles[tile];
  for(size_t i=0;i<m_soundfont_presets.size();++i)
    if(m_soundfont_presets[i].bank==t.InstrumentBank() &&
       m_soundfont_presets[i].program==t.InstrumentProgram()){
      m_instrument_page=int(i)/16;break;
    }
  m_instrument_status="Elige un preset del SoundFont activo. El MIDI original no se modifica.";
}

bool TrackSelectionState::UpdateInstrumentPicker(const MouseInfo& mouse){
  if(m_instrument_tile<0)return false;
  const int left=(GetStateWidth()-640)/2;
  const int top=(GetStateHeight()-510)/2;
  if(IsKeyPressed(KeyEscape) ||
     (mouse.released.left && mouse.x>=left+598 && mouse.x<left+630 &&
      mouse.y>=top+12 && mouse.y<top+45)){
    m_instrument_tile=-1;return true;
  }
  if(!mouse.released.left)return true;
  if(mouse.y>=top+445 && mouse.y<top+481){
    if(mouse.x>=left+12 && mouse.x<left+155){
      m_track_tiles[m_instrument_tile].SetInstrument(0,-1,"");
      PersistCurrent();m_instrument_tile=-1;return true;
    }
    if(mouse.x>=left+345 && mouse.x<left+475)m_instrument_page=std::max(0,m_instrument_page-1);
    if(mouse.x>=left+482 && mouse.x<left+615)
      m_instrument_page=std::min<int>((m_soundfont_presets.size()-1)/16,m_instrument_page+1);
    return true;
  }
  for(int i=0;i<16;++i){
    const int x=left+16+(i%2)*308;
    const int y=top+76+(i/2)*44;
    if(mouse.x<x||mouse.x>=x+300||mouse.y<y||mouse.y>=y+38)continue;
    const size_t idx=size_t(m_instrument_page)*16+i;
    if(idx<m_soundfont_presets.size()){
      const auto& item=m_soundfont_presets[idx];
      m_track_tiles[m_instrument_tile].SetInstrument(item.bank,item.program,item.name);
      PersistCurrent();
      m_hand_status="Instrumento: "+item.name;
      m_hand_status_until=SDL_GetTicks()+2600;
      m_instrument_tile=-1;
    }
    return true;
  }
  return true;
}

void TrackSelectionState::DrawInstrumentPicker(Renderer& renderer) const{
  if(m_instrument_tile<0)return;
  const int left=(GetStateWidth()-640)/2;
  const int top=(GetStateHeight()-510)/2;
  const GLboolean blend=glIsEnabled(GL_BLEND);
  GLint prevSrc=GL_ONE,prevDst=GL_ZERO;
  glGetIntegerv(GL_BLEND_SRC,&prevSrc);
  glGetIntegerv(GL_BLEND_DST,&prevDst);
  glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  renderer.SetColor(5,19,27,210);
  renderer.DrawQuad(0,0,GetStateWidth(),GetStateHeight());
  glBlendFunc(prevSrc,prevDst);if(!blend)glDisable(GL_BLEND);
  renderer.SetColor(25,42,53);renderer.DrawQuad(left,top,640,510);
  renderer.SetColor(68,169,196);renderer.DrawQuad(left,top,640,4);
  auto text=[&](int x,int y,const std::string& label,int font=13){
    renderer.SetColor(255,255,255);
    TextWriter t(left+x,top+y,renderer,false,font);
    t<<Text(label,Renderer::ToColor(223,240,245));
  };
  text(18,16,"Elegir instrumento del SoundFont",21);
  text(18,52,"Pulsa un sonido para asignarlo a esta pista (sin alterar el MIDI).",12);
  auto button=[&](int x,int y,int w,int h,const std::string& label,bool selected=false){
    ButtonState b(left+x,top+y,w,h);b.Update(Mouse());
    Layout::DrawCompactButton(renderer,b,GetTexture(ButtonSongs),label,selected,true,13);
  };
  button(598,12,32,32,"X");
  for(int i=0;i<16;++i){
    const size_t idx=size_t(m_instrument_page)*16+i;
    if(idx>=m_soundfont_presets.size())break;
    const auto& item=m_soundfont_presets[idx];
    std::string label=std::to_string(item.bank)+":"+
                      std::to_string(item.program+1)+"  "+item.name;
    if(label.size()>37)label=label.substr(0,34)+"...";
    const auto& tile=m_track_tiles[m_instrument_tile];
    const bool selected=tile.InstrumentProgram()==item.program &&
                        tile.InstrumentBank()==item.bank;
    button(16+(i%2)*308,76+(i/2)*44,300,38,label,selected);
  }
  button(12,445,143,36,"Original MIDI");
  text(170,455,"Página "+std::to_string(m_instrument_page+1)+" / "+
       std::to_string((m_soundfont_presets.size()+15)/16),12);
  button(345,445,130,36,"Anterior");
  button(482,445,133,36,"Siguiente");
  text(18,488,m_instrument_status,11);
}

void TrackSelectionState::ChoosePracticeHand(int choice) {
  // Display order: izquierda, ambas, derecha. Stored order: ambas,
  // derecha, izquierda. Do not swap RH/LH when saving preferences.
  const int modes[3]={HandPractice::LeftOnly,HandPractice::Both,
                      HandPractice::RightOnly};
  if(choice<0||choice>=3)return;
  const int requested=modes[choice];
  if(requested==m_practice_hand)return;
  AdvancedSettings preferences=AdvancedSettingsIO::Load();
  preferences.practiceHands=requested;
  std::string error;
  if(!AdvancedSettingsIO::Save(preferences,error)) {
    m_hand_status="No se pudo guardar la selección de manos: "+error;
    m_hand_status_until=SDL_GetTicks()+3500;
    return;
  }
  m_practice_hand=requested;
  m_hand_status=std::string("Manos para practicar: ")+
                HandPractice::PracticeName(requested);
  m_hand_status_until=SDL_GetTicks()+1800;
}

void TrackSelectionState::Update() {
  // A modal detector consumes mouse and Esc; nothing in the underlying
  // track cards, transport or hand selector can receive these clicks.
  if(m_instrument_tile>=0){
    UpdateInstrumentPicker(Mouse());return;
  }
  if(m_detector_open){
    if(IsKeyPressed(KeyEscape)){m_detector_open=false;return;}
    const MouseInfo mouse=Mouse();
    for(int id=0;id<11;++id){
      m_modal_buttons[id].Update(mouse);
      if(m_modal_buttons[id].hit){
        HandleDetectorModalClick(id);
        return;
      }
    }
    if(mouse.released.left && m_detector_preview_ready){
      const auto panel=DetectorModalBounds();
      if(mouse.y>=panel.y+353 && mouse.y<panel.y+455){
        const int line=(mouse.y-panel.y-353)/33;
        if(mouse.x>=panel.x+22 && mouse.x<panel.x+panel.w-18 &&
           line>=0 && line<3){
          const size_t i=size_t(m_review_page)*3+size_t(line);
          if(i<m_review_checked.size())m_review_checked[i]=!m_review_checked[i];
        }
      }else if(mouse.y>=panel.y+322 && mouse.y<panel.y+348){
        if(mouse.x>=panel.x+panel.w-182 && mouse.x<panel.x+panel.w-108)
          m_review_page=std::max(0,m_review_page-1);
        if(mouse.x>=panel.x+panel.w-104 && mouse.x<panel.x+panel.w-22)
          m_review_page=std::min<int>((m_review_proposals.size()+2)/3-1,
                                      m_review_page+1);
        m_review_page=std::max(0,m_review_page);
      }
    }
    return;
  }
  // La paleta modal consume sus propios eventos: no seleccionar pistas,
  // cambiar de página ni tocar otros botones mientras está abierta.
  if(m_color_palette_tile>=0){
    if(IsKeyPressed(KeyEscape)){
      m_color_palette_tile=-1;
      return;
    }
    const MouseInfo mouse=Mouse();
    if(mouse.released.left){
      const int chosen=ColorPaletteHit(mouse.x,mouse.y);
      if(chosen>=0 && static_cast<size_t>(m_color_palette_tile)<m_track_tiles.size())
        m_track_tiles[m_color_palette_tile].SetColor(
            static_cast<Track::TrackColor>(chosen));
      m_color_palette_tile=-1; // Fuera del panel: cancelar, sin cambiar.
      PersistCurrent();
    }
    return;
  }
  // Hand selection is independent from track modes and available before
  // the user starts playback. Consume its click, so it cannot also trigger
  // a track or navigation button underneath.
  const MouseInfo handMouse=Mouse();
  m_detect_button.Update(handMouse);
  if(m_detect_button.hit){
    OpenHandsDetector();
    return;
  }
  for(int option=0;option<3;++option){
    m_hand_buttons[option].Update(handMouse);
    if(m_hand_buttons[option].hit){
      ChoosePracticeHand(option);
      return;
    }
  }
  m_apply_all_not_played.Update(MouseInfo(Mouse()));
  m_apply_all_played_auto.Update(MouseInfo(Mouse()));
  m_apply_all_you_play.Update(MouseInfo(Mouse()));
  m_apply_all_you_play_silent.Update(MouseInfo(Mouse()));
  m_apply_all_learning.Update(MouseInfo(Mouse()));
  m_apply_all_learning_silent.Update(MouseInfo(Mouse()));
  m_apply_all_played_hidden.Update(MouseInfo(Mouse()));

  m_continue_button.Update(MouseInfo(Mouse()));
  m_back_button.Update(MouseInfo(Mouse()));
  m_record_video_button.Update(MouseInfo(Mouse()));
  if(m_record_video_button.hit){
    if(m_state.midi_out)m_state.midi_out->Reset();
    m_state.track_properties=BuildTrackProperties();
    PersistCurrent();
    ChangeState(new PlayingState(m_state,true));
    return;
  }

  if (IsKeyPressed(KeyEscape) || m_back_button.hit) {

    if (m_state.midi_out)
      m_state.midi_out->Reset();

    m_state.track_properties = BuildTrackProperties();
    PersistCurrent();
    ChangeState(new TitleState(m_state));
    return;
  }

  if (m_apply_all_not_played.hit) {
      for (size_t i = 0; i < m_track_tiles.size(); ++i) {
          m_track_tiles[i].SetMode(Track::ModeNotPlayed);
      }
  }

  if (m_apply_all_played_auto.hit) {
      for (size_t i = 0; i < m_track_tiles.size(); ++i) {
          m_track_tiles[i].SetMode(Track::ModePlayedAutomatically);
      }
  }

  if (m_apply_all_you_play.hit) {
      for (size_t i = 0; i < m_track_tiles.size(); ++i) {
          m_track_tiles[i].SetMode(Track::ModeYouPlay);
      }
  }

  if (m_apply_all_you_play_silent.hit) {
      m_you_play_silent_pulse=SDL_GetTicks();
      for (size_t i = 0; i < m_track_tiles.size(); ++i) {
          m_track_tiles[i].SetMode(Track::ModeYouPlaySilently);
      }
  }

  if (m_apply_all_learning.hit) {
      for (size_t i = 0; i < m_track_tiles.size(); ++i) {
          m_track_tiles[i].SetMode(Track::ModeLearning);
      }
  }

  if (m_apply_all_learning_silent.hit) {
      m_learning_silent_pulse=SDL_GetTicks();
      for (size_t i = 0; i < m_track_tiles.size(); ++i) {
          m_track_tiles[i].SetMode(Track::ModeLearningSilently);
      }
  }

  if (m_apply_all_played_hidden.hit) {
      for (size_t i = 0; i < m_track_tiles.size(); ++i) {
          m_track_tiles[i].SetMode(Track::ModePlayedButHidden);
      }
  }


  if (IsKeyPressed(KeyEnter) || m_continue_button.hit) {

    if (m_state.midi_out)
      m_state.midi_out->Reset();

    m_state.track_properties = BuildTrackProperties();
    PersistCurrent();
    ChangeState(new PlayingState(m_state));

    return;
  }

  if (IsKeyPressed(KeyDown) || IsKeyPressed(KeyRight)) {
    m_current_page++;

    if (m_current_page == m_page_count)
      m_current_page = 0;
    PersistCurrent();
  }

  if (IsKeyPressed(KeyUp) || IsKeyPressed(KeyLeft)) {
    m_current_page--;

    if (m_current_page < 0)
      m_current_page += m_page_count;
    PersistCurrent();
  }

  m_tooltip=m_detected_hands_summary;
  const ButtonState detector=AutoDetectHandsBounds();
  const MouseInfo currentMouse=Mouse();
  if(currentMouse.x>=detector.x && currentMouse.x<detector.x+detector.w &&
     currentMouse.y>=detector.y && currentMouse.y<detector.y+detector.h)
    m_tooltip="Analizar notas de las pistas activas y separar izquierda/derecha.";
  if (m_apply_all_you_play_silent.hovering)
    m_tooltip="Todas las pistas: las tocas tú, sin salida de sonido.";
  if (m_apply_all_learning_silent.hovering)
    m_tooltip="Todas las pistas: aprendizaje sin sonido MIDI.";

  for(int option=0;option<3;++option) {
    const ButtonState area=HandChoiceBounds(option);
    const MouseInfo mouse=Mouse();
    if(mouse.x>=area.x && mouse.x<area.x+area.w &&
       mouse.y>=area.y && mouse.y<area.y+area.h) {
      m_tooltip=option==0?"Practicar sólo la mano izquierda; la derecha puede acompañar.":
                option==1?"Practicar ambas manos.":
                          "Practicar sólo la mano derecha; la izquierda puede acompañar.";
    }
  }
  if(!m_hand_status.empty()) {
    if(SDL_GetTicks()<m_hand_status_until)
      m_tooltip=m_hand_status;
    else
      m_hand_status.clear();
  }
  if (m_back_button.hovering)
    m_tooltip = "Volver a la pantalla de inicio.";

  if (m_continue_button.hovering)
    m_tooltip = "Empezar a tocar con esta configuración.";

  // Our delta milliseconds on the first frame after we seek down to the
  // first note is extra long because the seek takes a while.  By skipping
  // the "Play" that update, we don't have an artificially fast-forwarded
  // start.
  if (!m_first_update_after_seek)
    PlayTrackPreview(static_cast<microseconds_t>(GetDeltaMicroseconds()));

  m_first_update_after_seek = false;

  // Do hit testing on each tile button on this page
  size_t start = m_current_page * m_tiles_per_page;
  size_t end = min( static_cast<size_t>((m_current_page+1) * m_tiles_per_page), m_track_tiles.size() );
  for (size_t i = start; i < end; ++i) {

    TrackTile &t = m_track_tiles[i];

    MouseInfo mouse = MouseInfo(Mouse());
    mouse.x -= t.GetX();
    mouse.y -= t.GetY();

    t.Update(mouse);

    if(t.HitInstrumentName()){
      OpenInstrumentPicker(int(i));return;
    }
    if(t.ButtonColor().hit){
      m_color_palette_tile=static_cast<int>(i);
      return; // No lanzar ninguna otra acción en esta pulsación.
    }

    if (t.ButtonLeft().hovering || t.ButtonRight().hovering) {

      switch (t.GetMode()) {
      case Track::ModeNotPlayed:
        m_tooltip = "No se reproduce ni se muestra esta pista.";
        break;

      case Track::ModePlayedAutomatically:
        m_tooltip = "Esta pista se reproduce automáticamente.";
        break;

      case Track::ModePlayedButHidden:
        m_tooltip = "Esta pista suena pero permanece oculta.";
        break;

      case Track::ModeYouPlay:
        m_tooltip = "Tocas esta pista con tu teclado.";
        break;

      case Track::ModeYouPlaySilently:
        m_tooltip = "Tocas esta pista sin que suene por la salida MIDI.";
        break;

      case Track::ModeLearning:
        m_tooltip = "La reproducción espera a que toques las notas.";
        break;

      case Track::ModeLearningSilently:
        m_tooltip = "Espera tus notas y no emite sonido MIDI.";
        break;

      case Track::ModeCount:
        break;
      }
    }

    if (t.ButtonPreview().hovering) {
      if (t.IsPreviewOn())
        m_tooltip = "Desactivar la escucha previa de esta pista.";

      else
        m_tooltip = "Escuchar una muestra de esta pista.";
    }

    if (t.ButtonColor().hovering)
        m_tooltip = "Elegir el color de las notas de esta pista.";

    if (t.ButtonRetry().hovering) {
      if (t.IsRetryOn())
        m_tooltip = "No repetir fragmentos fallidos.";

      else
        m_tooltip = "Repetir fragmentos fallidos.";
    }

    if (t.HitPreviewButton()) {

      if (m_state.midi_out)
        m_state.midi_out->Reset();

      if (t.IsPreviewOn()) {

        // Turn off any other preview modes
        for (size_t j = 0; j < m_track_tiles.size(); ++j) {
          if (i == j)
            continue;

          m_track_tiles[j].TurnOffPreview();
        }

        const microseconds_t PreviewLeadIn  = 25000;
        const microseconds_t PreviewLeadOut = 25000;

        m_preview_on = true;
        m_preview_track_id = t.GetTrackId();
        m_state.midi->Reset(PreviewLeadIn, PreviewLeadOut);
        PlayTrackPreview(0);

        // Find the first note in this track so we can skip right to the good part.
        microseconds_t additional_time = -PreviewLeadIn;
        const MidiTrack &track = m_state.midi->Tracks()[m_preview_track_id];
        for (size_t i = 0; i < track.Events().size(); ++i) {

          const MidiEvent &ev = track.Events()[i];
          if (ev.Type() == MidiEventType_NoteOn && ev.NoteVelocity() > 0) {
            additional_time += track.EventUsecs()[i] - m_state.midi->GetDeadAirStartOffsetMicroseconds() - 1;
            break;
          }
        }

        PlayTrackPreview(additional_time);
        m_first_update_after_seek = true;
      }

      else
        m_preview_on = false;
    }
  }
  // Buttons were updated above: persist only when the serialized selection
  // changes, never write the disk every frame.
  PersistCurrent();
}

void TrackSelectionState::PlayTrackPreview(microseconds_t delta_microseconds) {

  if (!m_preview_on)
    return;

  MidiEventListWithTrackId evs = m_state.midi->Update(delta_microseconds);

  for (MidiEventListWithTrackId::const_iterator i = evs.begin(); i != evs.end(); ++i) {
    const MidiEvent &ev = i->second;

    if (i->first != m_preview_track_id)
      continue;

    if (m_state.midi_out){
      const int channel=ev.Channel();
      const auto& tile=m_track_tiles[m_preview_track_id];
      if(m_state.midi_out->IsInternalFluid() && tile.InstrumentProgram()>=0 &&
         channel<16){
        if(ev.Type()==MidiEventType_ProgramChange)continue;
        if(ev.Type()==MidiEventType_NoteOn && ev.NoteVelocity()>0){
          FluidEngine::SelectPreset(channel,tile.InstrumentBank(),
                                   tile.InstrumentProgram());
        }
      }
      m_state.midi_out->Write(ev);
    }
  }
}

void TrackSelectionState::Draw(Renderer &renderer) const {

  Layout::DrawTitle(renderer, "Aplicar ajustes a todas las pistas");
  Layout::DrawHorizontalRule(renderer, GetStateWidth(), Layout::ScreenMarginY);

  Layout::DrawButton(renderer, m_apply_all_not_played, GetTexture(ButtonNotPlayed));
  Layout::DrawButton(renderer, m_apply_all_played_auto, GetTexture(ButtonPlayedAuto));
  Layout::DrawButton(renderer, m_apply_all_you_play, GetTexture(ButtonYouPlay));
  // Las modalidades silenciosas conservan su funcionamiento, pero muestran
  // el pictograma limpio (sin diagonal tachada). Una S discreta identifica
  // "silencioso" para no confundirlas con sus variantes audibles.
  const auto drawSilent=[&](const ButtonState& button,const Tga* symbol,
                            Uint32 lastClick){
    const Uint32 now=SDL_GetTicks();
    const bool settling=lastClick!=0 && now-lastClick<180;
    const bool pressed=button.depressed || settling;
    const int dy=pressed?3:0;
    renderer.SetColor(pressed?150:(button.hovering?255:224),
                      pressed?165:(button.hovering?255:224),
                      pressed?171:(button.hovering?255:224));
    renderer.DrawTga(symbol,button.x+dy/2,button.y+dy);
    // Una señal S pequeña sustituye la raya, no tapa el pictograma.
    const int labelX=button.x+93;
    const int labelY=button.y+29+dy;
    renderer.SetColor(pressed?60:69,pressed?103:93,pressed?101:94);
    renderer.DrawQuad(labelX-7,labelY-2,17,17);
    renderer.SetColor(255,255,255);
    TextWriter silence(labelX+1,labelY,renderer,true,12);
    silence << Text("S",Renderer::ToColor(238,245,242));
    if(pressed){
      // Línea inferior oscura = botón visualmente hundido.
      renderer.SetColor(52,71,76);
      renderer.DrawQuad(button.x+15,button.y+59,99,3);
    }
  };
  drawSilent(m_apply_all_you_play_silent,GetTexture(ButtonYouPlay),
             m_you_play_silent_pulse);
  Layout::DrawButton(renderer, m_apply_all_learning, GetTexture(ButtonLearning));
  drawSilent(m_apply_all_learning_silent,GetTexture(ButtonLearning),
             m_learning_silent_pulse);
  Layout::DrawButton(renderer, m_apply_all_played_hidden, GetTexture(ButtonPlayedHidden));

  Layout::DrawSubTitle(renderer,GetStateWidth()<1000?
         "Pistas para tocar":"Seleccionar las pistas para tocar");
  // The player already uses the same persisted three-way hand selector.
  // Place it beside the track-selection heading, without occupying a row
  // of track cards or requiring scrolling on smaller screens.
  const ButtonState& firstHand=m_hand_buttons[0];
  Layout::DrawCompactButton(renderer,m_detect_button,GetTexture(ButtonSongs),"Detectar manos");
  const int captionX=firstHand.x-66;
  renderer.SetColor(255,255,255);
  TextWriter handCaption(captionX,149,renderer,false,14);
  handCaption<<Text("Manos:",Renderer::ToColor(220,233,233));
  const char* hands[3]={"Izquierda","Ambas","Derecha"};
  const int modes[3]={HandPractice::LeftOnly,HandPractice::Both,
                      HandPractice::RightOnly};
  for(int index=0;index<3;++index)
    Layout::DrawCompactButton(renderer,m_hand_buttons[index],GetTexture(ButtonSongs),
                              hands[index],m_practice_hand==modes[index]);
  Layout::DrawHorizontalRule(renderer, GetStateWidth(), Layout::ScreenMarginY*2);
  Layout::DrawHorizontalRule(renderer, GetStateWidth(), GetStateHeight() - Layout::ScreenMarginY);

  Layout::DrawCompactButton(renderer,m_record_video_button,
                            GetTexture(ButtonSongs),"Exportar vídeo del escenario");
  Layout::DrawButton(renderer, m_continue_button, GetTexture(ButtonPlaySong));
  Layout::DrawButton(renderer, m_back_button, GetTexture(ButtonBackToTitle));

  // Write our page count on the screen
  TextWriter pagination(GetStateWidth()/2+200, GetStateHeight() - Layout::SmallFontSize - 30,
                        renderer, true, Layout::ButtonFontSize);

  pagination << Text(STRING("Página " << (m_current_page+1) << " de " <<
                            m_page_count << " (usa las flechas para cambiar de página)"), Gray);

  // Center wide automatic-detection diagnostics in the footer; the old
  // +200px shift overlapped "Tocar canción" at 1280px window width.
  TextWriter tooltip(GetStateWidth()/2, GetStateHeight() - Layout::SmallFontSize - 54,
                     renderer, true, 13);

  tooltip << m_tooltip;

  Tga *buttons = GetTexture(InterfaceButtons);
  Tga *box = GetTexture(TrackPanel);

  // Draw each track tile on the current page
  size_t start = m_current_page * m_tiles_per_page;
  size_t end = min(static_cast<size_t>((m_current_page+1) * m_tiles_per_page), m_track_tiles.size());

  for (size_t i = start; i < end; ++i) {
    m_track_tiles[i].Draw(renderer, m_state.midi, buttons, box);
  }

  // Superponer los colores a las tarjetas para no recortarlos con el
  // contenido del pentagrama ni con otras pistas.
  if(m_color_palette_tile>=0 &&
     static_cast<size_t>(m_color_palette_tile)<m_track_tiles.size()){
    const ButtonState panel=ColorPaletteBounds();
    const int originX=panel.x+17,originY=panel.y+38;
    const int swatchW=72,swatchH=51,gap=9;
    const char* names[]={"Azul","Verde","Naranja","Amarillo",
                         "Violeta","Rojo","Cian","Turquesa",
                         "Esmeralda","Lima","Coral","Rosa",
                         "Fucsia","Índigo","Cobre","Oliva"};
    const int current=static_cast<int>(m_track_tiles[m_color_palette_tile].GetColor());
    const MouseInfo mouse=Mouse();
    renderer.SetColor(25,39,40);
    renderer.DrawQuad(panel.x,panel.y,panel.w,panel.h);
    renderer.SetColor(102,144,139);
    renderer.DrawQuad(panel.x,panel.y,panel.w,3);
    renderer.SetColor(255,255,255);
    TextWriter title(panel.x+panel.w/2,panel.y+12,renderer,true,16);
    title << Text("Color de la pista",Renderer::ToColor(236,243,239));
    for(int i=0;i<Track::UserSelectableColorCount;++i){
      const int sx=originX+(i%4)*(swatchW+gap);
      const int sy=originY+(i/4)*(swatchH+gap);
      const bool active=i==current;
      const bool hover=ColorPaletteHit(mouse.x,mouse.y)==i;
      renderer.SetColor(active?247:(hover?185:69),
                        active?247:(hover?219:86),
                        active?247:(hover?228:89));
      renderer.DrawQuad(sx,sy,swatchW,swatchH);
      renderer.SetColor(Track::ColorNoteBlack[i]);
      renderer.DrawQuad(sx+3,sy+3,swatchW-6,swatchH-6);
      renderer.SetColor(Track::ColorNoteWhite[i]);
      renderer.DrawQuad(sx+7,sy+7,swatchW-14,23);
      renderer.SetColor(255,255,255);
      TextWriter label(sx+swatchW/2,sy+33,renderer,true,12);
      label << Text(names[i],Renderer::ToColor(245,248,246));
      if(active){
        renderer.SetColor(255,255,255);
        renderer.DrawQuad(sx,sy,swatchW,2);
        renderer.DrawQuad(sx,sy+swatchH-2,swatchW,2);
        renderer.DrawQuad(sx,sy,2,swatchH);
        renderer.DrawQuad(sx+swatchW-2,sy,2,swatchH);
      }
    }
  }

  if(m_instrument_tile>=0)DrawInstrumentPicker(renderer);
  if(m_detector_open){
    const auto m=DetectorModalBounds();
    // Dim the original track cards without hiding them. This is a
    // floating modal panel over the current selection, not another page.
    const GLboolean blendBefore=glIsEnabled(GL_BLEND);
    GLint previousSrc=GL_ONE, previousDst=GL_ZERO;
    glGetIntegerv(GL_BLEND_SRC,&previousSrc);
    glGetIntegerv(GL_BLEND_DST,&previousDst);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    renderer.SetColor(10,20,25,193);
    renderer.DrawQuad(0,0,GetStateWidth(),GetStateHeight());
    glBlendFunc(previousSrc,previousDst);
    if(!blendBefore)glDisable(GL_BLEND);
    renderer.SetColor(28,41,46);
    renderer.DrawQuad(m.x,m.y,m.w,m.h);
    renderer.SetColor(74,160,154);
    renderer.DrawQuad(m.x,m.y,m.w,4);
    renderer.SetColor(255,255,255);
    TextWriter heading(m.x+25,m.y+16,renderer,false,22);
    heading<<Text("Detector de manos y análisis musical",
                  Renderer::ToColor(242,252,249));
    TextWriter intro(m.x+25,m.y+52,renderer,false,13);
    intro<<Text("Configura la detección antes de aplicarla; el MIDI no cambia.",
                Renderer::ToColor(183,206,209));
    const auto label=[&](const char* txt,int yy){
      renderer.SetColor(255,255,255);
      TextWriter t(m.x+25,m.y+yy,renderer,false,14);
      t<<Text(txt,Renderer::ToColor(215,232,234));
    };
    label("Pistas:",94);
    label("Método:",152);
    label("Corte MIDI:",211);
    const char* words[]={"X","Piano seleccionado","Todas activas",
                        "Automática","Manual","−","+",
                        "Analizar","Aplicar","Cancelar","Restaurar notas"};
    for(int id=0;id<11;++id){
      const bool selected=(id==1&&m_detector_source_piano)||
                          (id==2&&!m_detector_source_piano)||
                          (id==3&&m_detector_automatic)||
                          (id==4&&!m_detector_automatic);
      const bool enabled=!((id==5||id==6)&&m_detector_automatic) &&
                         !(id==8&&!m_detector_preview_ready);
      Layout::DrawCompactButton(renderer,m_modal_buttons[id],GetTexture(ButtonSongs),
                                words[id],selected,enabled,14);
    }
    renderer.SetColor(255,255,255);
    TextWriter cut(m.x+333,m.y+209,renderer,true,20);
    cut<<Text(m_detector_automatic?"AUTO":
              std::to_string(m_detector_split),
              Renderer::ToColor(234,250,240));
    TextWriter help(m.x+24,m.y+254,renderer,false,13);
    help<<Text(m_detector_automatic?
          "Automática: estima el registro de cada mano a partir de las notas.":
          "Manual: ajusta el punto MIDI con − / + y vuelve a analizar.",
          Renderer::ToColor(170,199,202));
    if(m_detector_preview_ready){
      TextWriter count(m.x+24,m.y+284,renderer,false,14);
      count<<Text("Izquierda: "+std::to_string(m_detector_left)+
          "     Derecha: "+std::to_string(m_detector_right)+
          "     Corte MIDI: "+std::to_string(m_detector_preview_split),
          Renderer::ToColor(155,236,200));
    }else{
      TextWriter status(m.x+24,m.y+285,renderer,false,13);
      status<<Text(m_detector_feedback,Renderer::ToColor(246,202,154));
    }
    TextWriter safe(m.x+24,m.y+308,renderer,false,12);
    safe<<Text("Sostenidos/bemoles: notación automática según tonalidad y compás.",
               Renderer::ToColor(160,191,194));
    // Editable preview of the contextual reasoner's proposals, rather than
    // blindly changing chromatic tones or legitimate ornaments.
    renderer.SetColor(255,255,255);
    TextWriter reviewHeading(m.x+24,m.y+328,renderer,false,12);
    reviewHeading<<Text("Revisión musical (clic = aprobar/rechazar): "+
      std::to_string(m_review_proposals.size())+" propuestas",
      Renderer::ToColor(177,225,234));
    if(m_review_proposals.size()>3){
      TextWriter page(m.x+m.w-260,m.y+326,renderer,false,12);
      page<<Text("[<]  "+std::to_string(m_review_page+1)+"/"+
         std::to_string((m_review_proposals.size()+2)/3)+"  [>]",
         Renderer::ToColor(232,242,245));
      // clickable rectangles are drawn to coincide with the hit targets.
      Layout::DrawCompactButton(renderer,
        ButtonState(m.x+m.w-182,m.y+322,74,26),GetTexture(ButtonSongs),"<");
      Layout::DrawCompactButton(renderer,
        ButtonState(m.x+m.w-104,m.y+322,82,26),GetTexture(ButtonSongs),">");
    }
    for(int row=0;row<3;++row){
      const size_t i=size_t(m_review_page)*3+row;
      if(i>=m_review_proposals.size())break;
      const auto& r=m_review_proposals[i];
      const int y=m.y+353+row*33;
      renderer.SetColor(m_review_checked[i]?38:24,
                        m_review_checked[i]?94:58,72);
      renderer.DrawQuad(m.x+22,y,m.w-44,30);
      renderer.SetColor(255,255,255);
      TextWriter item(m.x+30,y+7,renderer,false,12);
      const std::string choice=m_review_checked[i]?"[X]":"[ ]";
      const std::string display=choice+" "+MidiEvent::NoteName(r.original)+
        " -> "+(r.replacement<0?"silenciar":MidiEvent::NoteName(r.replacement))+
        " (criterio "+std::to_string(r.confidence)+"/100)  "+r.reason;
      item<<Text(display.substr(0,89),Renderer::ToColor(224,240,244));
    }
    if(m_review_proposals.empty()&&m_detector_preview_ready){
      TextWriter empty(m.x+25,m.y+366,renderer,false,13);
      empty<<Text("Sin anomalías claras: no se proponen cambios de notas.",
                  Renderer::ToColor(159,226,178));
    }
  }

}
