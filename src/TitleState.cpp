// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "TitleState.h"
#include "TrackSelectionState.h"

#include "Version.h"
#include "CompatibleSystem.h"

#include "MenuLayout.h"
#include "UserSettings.h"
#include "FileSelector.h"
#include "Renderer.h"
#include "Textures.h"
#include "Tga.h"
#include "GameState.h"
#include "SongLibState.h"
#include "FreePlayState.h"
#include "RecentSongs.h"
#include "SongMetadata.h"

#include "libmidi/Midi.h"
#include "libmidi/MidiUtil.h"
#include "MidiComm.h"
#include "AdvancedSettings.h"
#include "FluidEngine.h"
#include <SDL2/SDL.h>
#include <glib.h>
#include <sys/stat.h>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>

using namespace std;

const static string OutputKeySpecialDisabled = "[no output device]";
const static string InputKeySpecialDisabled = "[no input device]";


TitleState::~TitleState() {
   for(auto*& preview:m_scene_previews){
     if(preview){Tga::Release(preview);preview=nullptr;}
   }

   if (m_output_tile) delete m_output_tile;
   if (m_input_tile) delete m_input_tile;
   if (m_file_tile) delete m_file_tile;
   if (m_keyboard_size_tile) delete m_keyboard_size_tile;
}

void TitleState::Init() {

   ArrangeFooter();
   ArrangeSettings();

   string last_output_device = AdvancedSettingsIO::Load().midiOutput;
   string last_input_device = AdvancedSettingsIO::Load().midiInput;
   m_keyboard_size = KeyboardSize(UserSetting::Get(KEYBOARD_SIZE_KEY, ""));
   m_saved_settings=AdvancedSettingsIO::Load();
   m_draft_settings=m_saved_settings;
   m_draft_keyboard=m_keyboard_size;
   const string saved_rate = UserSetting::Get(REFRESH_RATE_KEY, "300");
   if (saved_rate == "30" || saved_rate == "60" ||
       saved_rate == "120" || saved_rate == "300")
     m_refresh_rate = std::stoi(saved_rate);

   // midi_out could be in one of three states right now:
   //    1. We just started and were passed a null MidiCommOut pointer
   // Or, we're returning from track selection either with:
   //    2. a null MidiCommOut because the user didn't want any output, or
   //    3. a valid MidiCommOut we constructed previously.
   if (!m_state.midi_out) {

      // Try to find the previously used device
     MidiCommDescriptionList devices = MidiCommOut::GetDeviceList();
     MidiCommDescriptionList::const_iterator it;
     for (it = devices.begin(); it != devices.end(); it++) {
       if (it->name == last_output_device) {
         try {m_state.midi_out = new MidiCommOut(it->id);}
         catch (...) {m_state.midi_out = nullptr;}
         if(m_state.midi_out)break;
       }
     }

     // Next, if we couldn't find a previously used device,
     // use the first one
     if (last_output_device.empty() && !m_state.midi_out) {
       // Do not start a sound engine or touch a physical synth automatically.
       // When FluidSynth is the only option, the user chooses it explicitly.
       for(const auto& device:devices){
         if(device.client<0 ||
            device.name.find("FANTOM")!=std::string::npos)continue;
         m_state.midi_out=new MidiCommOut(device.id);
         break;
       }
     }
   }

   if (!m_state.midi_in) {

     // Try to find the previously used device
     MidiCommDescriptionList devices = MidiCommIn::GetDeviceList();
     MidiCommDescriptionList::const_iterator it;
     for (it = devices.begin(); it != devices.end(); it++) {
       if (it->name == last_input_device) {
         try {
           m_state.midi_in = new MidiCommIn(it->id);
           break;
         }
         catch (MidiErrorCode) {
           m_state.midi_in = 0;
         }
       }
     }

     // If we couldn't find the previously used device,
     // disabling by default (i.e. leaving it null) is
     // completely acceptable.
   }

   int output_device_id = -1;
   if (m_state.midi_out) {
     output_device_id = m_state.midi_out->GetDeviceDescription().id;
     m_state.midi_out->Reset();
   }

   int input_device_id = -1;
   if (m_state.midi_in) {
     input_device_id = m_state.midi_in->GetDeviceDescription().id;
     m_state.midi_in->Reset();
   }

   const bool compress_height = (GetStateHeight() < 750);
   const int initial_y = (compress_height ? 230 : 360);
   const int each_y = (compress_height ? 94 : 100);

   m_file_tile = new StringTile((GetStateWidth() - StringTileWidth) / 2,
                                initial_y + each_y*0,
                                GetTexture(SongBox));

   m_file_tile->SetString(m_state.song_title);

   const MidiCommDescriptionList output_devices = MidiCommOut::GetDeviceList();
   const MidiCommDescriptionList input_devices = MidiCommIn::GetDeviceList();

   m_output_tile = new DeviceTile((GetStateWidth() - DeviceTileWidth) / 2,
                                  initial_y + each_y*1,
                                  output_device_id,
                                  DeviceTileOutput,
                                  output_devices,
                                  GetTexture(InterfaceButtons),
                                  GetTexture(OutputBox));

   m_input_tile = new DeviceTile((GetStateWidth() - DeviceTileWidth) / 2,
                                 initial_y + each_y*2,
                                 input_device_id,
                                 DeviceTileInput,
                                 input_devices,
                                 GetTexture(InterfaceButtons),
                                 GetTexture(InputBox));

  m_keyboard_size_tile = new EnumTile(
                                 (GetStateWidth() - DeviceTileWidth) / 2,
                                 initial_y + each_y*3,
                                 m_keyboard_size,
                                 "Tamaño del teclado:",
                                 GetTexture(InterfaceButtons),
                                 GetTexture(EmptyBox));
}

void TitleState::ArrangeFooter() {
   const int y = GetStateHeight() - Layout::ScreenMarginY/2 -
                 Layout::ButtonHeight/2;
   const int left = Layout::ScreenMarginX;
   const int right = GetStateWidth()-Layout::ScreenMarginX-Layout::ButtonWidth;
   m_back_button = ButtonState(left,y,Layout::ButtonWidth,Layout::ButtonHeight);
   m_continue_button = ButtonState(right,y,Layout::ButtonWidth,Layout::ButtonHeight);

   // Four controls in one line at standard width, two lines on narrow screens.
   const bool narrow = GetStateWidth() < 800;
   m_songs_button = ButtonState(left+(narrow?0:Layout::ButtonWidth+16),
      y-(narrow?54:0),Layout::ButtonWidth,Layout::ButtonHeight);
   m_settings_button = ButtonState(narrow?right:left+2*(Layout::ButtonWidth+16),
      y-(narrow?54:0),Layout::ButtonWidth,Layout::ButtonHeight);
   m_free_button=ButtonState(left+(narrow?Layout::ButtonWidth+16:
                                     3*(Layout::ButtonWidth+16)),
                            y-(narrow?54:0),Layout::ButtonWidth,
                            Layout::ButtonHeight);
}

void TitleState::ArrangeSettings() {
   // Control rectangles are calculated responsively in Update/DrawAdvancedSettings.
}

void TitleState::SaveKeyboardSize() {
   UserSetting::Set(KEYBOARD_SIZE_KEY,m_keyboard_size.GetValue());
   m_state.keyboard=Keyboard::GetKeyboardDefaults(m_keyboard_size);
}

void TitleState::Resize() {
    const bool compress_height = (GetStateHeight() < 750);
    const int initial_y = (compress_height ? 230 : 360);
    const int each_y = (compress_height ? 94 : 100);

    m_file_tile->SetX((GetStateWidth() - StringTileWidth) / 2);
    m_file_tile->SetY(initial_y + each_y * 0);

    m_output_tile->SetX((GetStateWidth() - DeviceTileWidth) / 2);
    m_output_tile->SetY(initial_y + each_y * 1);
    
    m_input_tile->SetX((GetStateWidth() - DeviceTileWidth) / 2);
    m_input_tile->SetY(initial_y + each_y*2);

    m_keyboard_size_tile->SetX((GetStateWidth() - DeviceTileWidth) / 2);
    m_keyboard_size_tile->SetY(initial_y + each_y*3);

    ArrangeFooter();
    ArrangeSettings();
}

void TitleState::Update() {
  MidiCommOut::UpdateDeviceList();
  MidiCommIn::UpdateDeviceList();
  m_input_tile->ReplaceDeviceList(MidiCommIn::GetDeviceList());
  m_output_tile->ReplaceDeviceList(MidiCommOut::GetDeviceList());

  MouseInfo mouse = Mouse();

  if (m_skip_next_mouse_up) {
    mouse.released.left = false;
    m_skip_next_mouse_up = false;
  }

  if(m_settings_open){
    UpdateAdvancedSettings(mouse);
    return; // Prevent underlying home buttons from receiving modal clicks.
  }
  // Modal choices first: never accidentally activate songs or Exit underneath.
  if(m_home_open>=0){
    UpdateHomeOptions(mouse);
    return;
  }
  if(mouse.released.left){
    for(int c=0;c<5;++c){
      const auto box=HomeCardAt(c);
      if(mouse.x>=box.x && mouse.x<box.x+box.w &&
         mouse.y>=box.y && mouse.y<box.y+box.h){
        OpenHomeOptions(c);
        return;
      }
    }
  }
  m_continue_button.Update(mouse);
  m_back_button.Update(mouse);
  m_songs_button.Update(mouse);
  m_settings_button.Update(mouse);
  m_free_button.Update(mouse);

  if(m_free_button.hit){
    m_skip_next_mouse_up=true;
    ChangeState(new FreePlayState(m_state));
    return;
  }
  if(m_songs_button.hit){
    if(m_state.midi_out){
      m_state.midi_out->Reset();
      m_output_tile->TurnOffPreview();
    }
    m_skip_next_mouse_up=true;
    ChangeState(new SongLibState(m_state));
    return;
  }
  if(m_settings_button.hit){
    if(m_settings_open)CloseAdvancedSettings();
    else OpenAdvancedSettings();
    m_tooltip="";
    return;
  }
   if (IsKeyPressed(KeyEscape) || m_back_button.hit) {
     delete m_state.midi_out;
     m_state.midi_out = 0;

     delete m_state.midi_in;
     m_state.midi_in = 0;

     delete m_state.midi;
     m_state.midi = 0;

     Compatible::GracefulShutdown();
     return;
   }

   if (IsKeyPressed(KeyEnter) || m_continue_button.hit) {

     if (m_state.midi_out)
       m_state.midi_out->Reset();

     if (m_state.midi_in)
       m_state.midi_in->Reset();
 
 
     UserSetting::Set(KEYBOARD_SIZE_KEY, m_keyboard_size.GetValue());
     m_state.keyboard = Keyboard::GetKeyboardDefaults(m_keyboard_size);

     ChangeState(new TrackSelectionState(m_state));
     return;
   }

   m_tooltip = "";

   if (m_back_button.hovering)
     m_tooltip = "Salir de Linthesia - Studio.";

   if (m_continue_button.hovering)
     m_tooltip = "Ir a la selección de pistas.";

   if (m_songs_button.hovering)
     m_tooltip = "Abrir biblioteca de canciones MIDI.";

   if (m_settings_button.hovering)
     m_tooltip = "Abrir Configuración de audio, MIDI, pantalla y FluidSynth.";
  if(m_free_button.hovering)
    m_tooltip="Tocar libremente, grabar hasta seis pistas y exportar MIDI.";

}

void TitleState::PlayDevicePreview(microseconds_t delta_microseconds) {
  if (!m_output_tile->IsPreviewOn())
    return;

  if (!m_state.midi_out)
    return;

  MidiEventListWithTrackId evs = m_state.midi->Update(delta_microseconds);

  for (MidiEventListWithTrackId::const_iterator i = evs.begin(); i != evs.end(); ++i) {
    m_state.midi_out->Write(i->second);
  }
}

void TitleState::Draw(Renderer &renderer) const {
  if(m_settings_open){
    DrawAdvancedSettings(renderer);
    return;
  }

  const bool compress_height = (GetStateHeight() < 750);
  const static int TitleWidth = 507;
  const int TitleY = (compress_height ? 43 : 100);

  int left = GetStateWidth() / 2 - TitleWidth / 2;

  renderer.SetColor(White);
  renderer.DrawTga(GetTexture(TitleLogo), left, TitleY);
  // Preserve the original GPL project's logo and add the fork's identity
  // as separately rendered text, rather than altering licensed artwork.
  TextWriter studioBrand(left+TitleWidth/2-60,
                         TitleY+(compress_height ? 97 : 117),
                         renderer,false,16);
  studioBrand << Text("— STUDIO —",Renderer::ToColor(187,226,238));
  renderer.DrawTga(GetTexture(GameMusicThemes), left+3, TitleY + (compress_height ? 120 : 150) );

  TextWriter version(Layout::ScreenMarginX,
                     GetStateHeight() - Layout::ScreenMarginY - Layout::SmallFontSize * 2,
                     renderer, false, Layout::SmallFontSize);

  version << Text("Linthesia - Studio · versión " + LinthesiaVersionString, Gray);

  Layout::DrawHorizontalRule(renderer,
                             GetStateWidth(),
                             GetStateHeight() - Layout::ScreenMarginY);

  Layout::DrawButton(renderer, m_continue_button, GetTexture(ButtonChooseTracks));
  Layout::DrawButton(renderer, m_back_button, GetTexture(ButtonExit));
  Layout::DrawButton(renderer, m_songs_button, GetTexture(ButtonSongs));
  Layout::DrawButton(renderer, m_settings_button, GetTexture(ButtonSettings));
  renderer.SetColor(34,92,117,248);
  renderer.DrawRoundedGradient(m_free_button.x,m_free_button.y,
       m_free_button.w,m_free_button.h,5,
       Renderer::ToColor(41,117,143),
       Renderer::ToColor(18,56,81));
  renderer.SetColor(255,255,255);
  TextWriter freeTitle(m_free_button.x+22,m_free_button.y+14,renderer,false,16);
  freeTitle<<Text("MODO LIBRE",Renderer::ToColor(237,249,255));

  // Five editable rows; the dedicated settings editor remains available.
  DrawHomeCards(renderer);
  if(!m_home_status.empty()){
    const int y=HomeCardAt(4).y+HomeCardAt(4).h+5;
    TextWriter feedback(GetStateWidth()/2,y,renderer,true,12);
    feedback<<Text(m_home_status,Renderer::ToColor(184,221,214));
  }
  if(m_home_open>=0)DrawHomeOptions(renderer);

  if(!m_tooltip.empty() && m_home_open<0){
    renderer.SetColor(White);
    TextWriter tooltip(GetStateWidth()/2,
                       GetStateHeight()-Layout::ScreenMarginY-22,
                       renderer,true,Layout::TitleFontSize);
    tooltip << m_tooltip;
  }

}


// Compact home selectors intentionally reuse the same persistent settings
// path as Configuración rather than creating an unrelated audio/MIDI state.
TitleState::HomeCard TitleState::HomeCardAt(int index) const{
  const int W=GetStateWidth(),H=GetStateHeight();
  const int width=std::min(510,std::max(270,W-40));
  const int height=std::max(51,std::min(66,(H-310)/5));
  const int space=std::max(4,std::min(9,(H-560)/12+7));
  const int start=H<750?std::max(167,H-5*(height+space)-85):
                            std::max(300,(H-5*(height+space))/2+78);
  return {(W-width)/2,start+index*(height+space),width,height};
}

std::vector<TitleState::HomeChoice> TitleState::HomeOptions(int card)const{
  std::vector<HomeChoice> choices;
  if(card==0){
    for(const auto& recent:RecentSongs::List(10,RecentSongs::PlaybackOrder)){
      const auto slash=recent.path.find_last_of('/');
      choices.push_back({recent.path.substr(slash==std::string::npos?
                                             0:slash+1),recent.path});
    }
    choices.push_back({"Buscar otra canción en la biblioteca…","@biblioteca"});
  }else if(card==1 || card==2){
    choices.push_back({"Desactivado",""});
    const auto& devices=card==1?m_output_choices:m_input_choices;
    for(const auto& device:devices)
      choices.push_back({device.name,device.name});
  }else if(card==3){
    for(int n:{37,49,61,76,88})
      choices.push_back({std::to_string(n)+" teclas",std::to_string(n)});
  }else if(card==4){
    for(const auto& name:m_audio_devices)
      choices.push_back({name=="default"?"Predeterminado (sistema)":name,name});
  }
  return choices;
}
void TitleState::OpenHomeOptions(int card){
  if(card<0||card>4)return;
  m_draft_settings=AdvancedSettingsIO::Load();
  m_saved_settings=m_draft_settings;
  m_draft_keyboard=m_keyboard_size;
  if(card==1||card==2||card==4)RefreshDeviceChoices();
  m_home_choices=HomeOptions(card);
  m_home_open=card;m_home_page=0;m_home_status.clear();
}
bool TitleState::LoadRecentSong(const std::string& path){
  try{
    Midi* next=new Midi(Midi::ReadFromFile(path));
    delete m_state.midi;
    m_state.midi=next;
    m_state.song_path=path;
    m_state.song_title=path.substr(path.find_last_of('/')+1);
    const auto dot=m_state.song_title.find_last_of('.');
    if(dot!=std::string::npos)m_state.song_title.resize(dot);
    const auto metadata=SongMetadata::Load(path);
    if(!metadata.title.empty())m_state.song_title=metadata.title;
    m_state.track_properties.clear();
    m_state.musicxml_score_path.clear();
    m_file_tile->SetString(m_state.song_title);
    RecentSongs::RememberLastSelected(path);
    m_home_status="Canción seleccionada · Elegir pistas para comenzar.";
    return true;
  }catch(const MidiError& error){
    m_home_status="MIDI no válido: "+error.GetErrorDescription();
  }catch(const std::exception& error){
    m_home_status="No se pudo cargar el MIDI: "+std::string(error.what());
  }catch(...){
    m_home_status="No se pudo cargar el MIDI seleccionado.";
  }
  return false;
}
void TitleState::SelectHomeOption(int card,int option){
  if(card<0||card>4||option<0||
      option>=static_cast<int>(m_home_choices.size()))return;
  const auto selection=m_home_choices[option];
  if(card==0){
    if(selection.value!="@biblioteca")LoadRecentSong(selection.value);
    return;
  }
  // Apply only this one setting: changing the keyboard or audio must NOT
  // reconnect other saved hardware ports (e.g. a powered-off FANTOM).
  if(card==3){
    m_keyboard_size=KeyboardSize(selection.value);
    m_draft_keyboard=m_keyboard_size;
    SaveKeyboardSize();
    m_home_status="Tamaño de teclado actualizado y guardado.";
    return;
  }
  AdvancedSettings updated=AdvancedSettingsIO::Load();
  if(card==4){
    updated.audioDevice=selection.value;
    const bool fluidOutput=m_state.midi_out &&
        m_state.midi_out->GetDeviceDescription().name=="FluidSynth interno";
    if(fluidOutput && FluidEngine::IsRunning()){
      std::string error;
      if(!FluidEngine::Start(updated,error)){
        m_home_status="No se pudo cambiar la salida de audio: "+error;
        return;
      }
    }
  }else{
    if(card==1)updated.midiOutput=selection.value;
    if(card==2)updated.midiInput=selection.value;
  }
  // Prepare new ports before disposing of the working connection.
  MidiCommOut* nextOut=nullptr;
  MidiCommIn* nextIn=nullptr;
  try{
    if(card==1 && !selection.value.empty()){
      const auto devices=MidiCommOut::GetDeviceList();
      auto it=std::find_if(devices.begin(),devices.end(),[&](const auto& d){
        return d.name==selection.value;
      });
      if(it==devices.end())throw std::runtime_error("Salida MIDI desconectada");
      nextOut=new MidiCommOut(static_cast<unsigned int>(it-devices.begin()));
    }
    if(card==2 && !selection.value.empty()){
      const auto devices=MidiCommIn::GetDeviceList();
      auto it=std::find_if(devices.begin(),devices.end(),[&](const auto& d){
        return d.name==selection.value;
      });
      if(it==devices.end())throw std::runtime_error("Entrada MIDI desconectada");
      nextIn=new MidiCommIn(static_cast<unsigned int>(it-devices.begin()));
    }
  }catch(const std::exception& error){
    delete nextOut;delete nextIn;
    m_home_status="Dispositivo no disponible: "+std::string(error.what());
    return;
  }catch(...){
    delete nextOut;delete nextIn;
    m_home_status="No se pudo abrir el dispositivo seleccionado.";
    return;
  }
  std::string error;
  if(!AdvancedSettingsIO::Save(updated,error)){
    delete nextOut;delete nextIn;
    m_home_status="No se pudo guardar: "+error;
    return;
  }
  if(card==1){
    if(m_state.midi_out){m_state.midi_out->Reset();delete m_state.midi_out;}
    m_state.midi_out=nextOut;
    UserSetting::Set(OUTPUT_DEVICE_KEY,selection.value.empty()?
                               "[no output device]":selection.value);
    if(m_output_tile)m_output_tile->TurnOffPreview();
  }
  if(card==2){
    delete m_state.midi_in;
    m_state.midi_in=nextIn;
    UserSetting::Set(INPUT_DEVICE_KEY,selection.value.empty()?
                               "[no input device]":selection.value);
  }
  m_saved_settings=updated;
  m_draft_settings=updated;
  m_home_status=card==4?"Salida de audio de FluidSynth guardada.":
                          "Dispositivo MIDI actualizado y guardado.";
}
bool TitleState::UpdateHomeOptions(const MouseInfo& mouse){
  if(m_home_open<0)return false;
  if(IsKeyPressed(KeyEscape)){
    m_home_open=-1;m_home_choices.clear();return true;
  }
  if(!mouse.released.left)return true;
  const int W=GetStateWidth(),H=GetStateHeight();
  const int width=std::min(640,W-36),rowH=34;
  const int visible=std::max(2,std::min(10,(H-165)/rowH));
  const int count=std::min(visible,
                       static_cast<int>(m_home_choices.size()));
  const int height=104+count*rowH+36;
  const int x=(W-width)/2,y=(H-height)/2;
  const auto hit=[&](int xx,int yy,int ww,int hh){
    return mouse.x>=xx && mouse.x<xx+ww &&
           mouse.y>=yy && mouse.y<yy+hh;
  };
  if(!hit(x,y,width,height) || hit(x+width-87,y+9,72,28)){
    m_home_open=-1;m_home_choices.clear();return true;
  }
  const int pages=std::max(1,(static_cast<int>(m_home_choices.size())+
                               visible-1)/visible);
  if(pages>1){
    if(hit(x+width-200,y+height-37,83,29)){
      m_home_page=std::max(0,m_home_page-1);return true;
    }
    if(hit(x+width-107,y+height-37,83,29)){
      m_home_page=std::min(pages-1,m_home_page+1);return true;
    }
  }
  if(hit(x+12,y+64,width-24,count*rowH)){
    const int choice=m_home_page*visible+(mouse.y-y-64)/rowH;
    if(choice>=0 && choice<static_cast<int>(m_home_choices.size())){
      const int original=m_home_open;
      const bool browse=original==0 &&
                 m_home_choices[choice].value=="@biblioteca";
      m_home_open=-1;
      if(browse){
        m_home_choices.clear();
        m_skip_next_mouse_up=true;
        ChangeState(new SongLibState(m_state));
        return true; // Never access the old state after state transition.
      }
      SelectHomeOption(original,choice);
      m_home_choices.clear();
    }
  }
  return true;
}
void TitleState::DrawHomeCards(Renderer& renderer)const{
  const std::string currentOut=m_state.midi_out?
    m_state.midi_out->GetDeviceDescription().name:"Desactivado";
  const std::string currentIn=m_state.midi_in?
    m_state.midi_in->GetDeviceDescription().name:"Desactivado";
  const auto& a=m_saved_settings;
  const std::string song=m_state.song_title.empty()?
                                        "Elegir canción":m_state.song_title;
  const std::string audio=a.audioDevice=="default"?
                               "Predeterminado (sistema)":a.audioDevice;
  const std::string labels[5]={"Canción","Dispositivo de salida MIDI",
     "Dispositivo de entrada MIDI","Tamaño del teclado",
     "Dispositivo de audio · FluidSynth"};
  const std::string values[5]={song,currentOut,currentIn,
                                 m_keyboard_size.AsText(),audio};
  for(int i=0;i<5;++i){
    const auto r=HomeCardAt(i);
    renderer.SetColor(87,102,109,255);
    renderer.DrawRoundedGradient(r.x,r.y,r.w,r.h,7,
          Renderer::ToColor(63,77,83),Renderer::ToColor(37,49,57));
    renderer.SetColor(255,255,255);
    TextWriter label(r.x+15,r.y+6,renderer,false,14);
    label<<Text(labels[i],Renderer::ToColor(205,225,231));
    TextWriter val(r.x+17,r.y+29,renderer,false,15);
    // TextWriter does not clip by pixel width: leave room for the
    // right-hand arrow, and never cut UTF-8 inside a multibyte character.
    std::string shown=values[i];
    const int maxLetters=std::max(16,(r.w-94)/8);
    if(g_utf8_validate(shown.c_str(),-1,nullptr) &&
       g_utf8_strlen(shown.c_str(),-1)>maxLetters){
      gchar* first=g_utf8_substring(shown.c_str(),0,maxLetters-3);
      shown=std::string(first)+"...";
      g_free(first);
    }else if(static_cast<int>(shown.size())>maxLetters)
      shown=shown.substr(0,maxLetters-3)+"...";
    val<<Text(shown,Renderer::ToColor(247,250,252));
    renderer.SetColor(31,92,113,245);
    renderer.DrawQuad(r.x+r.w-45,r.y+10,33,r.h-20);
    renderer.SetColor(255,255,255);
    TextWriter arrow(r.x+r.w-28,r.y+17,renderer,true,19);
    arrow<<Text("v",Renderer::ToColor(228,250,255));
  }
}
void TitleState::DrawHomeOptions(Renderer& renderer)const{
  if(m_home_open<0)return;
  const int W=GetStateWidth(),H=GetStateHeight();
  const int width=std::min(640,W-36),rowH=34;
  const int visible=std::max(2,std::min(10,(H-165)/rowH));
  const int count=std::min(visible,
                         static_cast<int>(m_home_choices.size()));
  const int height=104+count*rowH+36;
  const int x=(W-width)/2,y=(H-height)/2;
  renderer.SetColor(8,18,26,225);
  renderer.DrawQuad(0,0,W,H);
  renderer.SetColor(25,40,50,255);
  renderer.DrawQuad(x,y,width,height);
  renderer.SetColor(72,154,169,255);
  renderer.DrawQuad(x,y,width,3);
  const char* heads[]={"Últimas 10 canciones reproducidas",
    "Seleccionar salida MIDI","Seleccionar entrada MIDI",
    "Seleccionar tamaño de teclado","Seleccionar salida de audio"};
  renderer.SetColor(255,255,255);
  TextWriter heading(x+17,y+13,renderer,false,22);
  heading<<Text(heads[m_home_open],Renderer::ToColor(237,250,253));
  renderer.SetColor(65,86,94,255);
  renderer.DrawQuad(x+width-87,y+9,72,28);
  renderer.SetColor(255,255,255);
  TextWriter close(x+width-50,y+14,renderer,true,13);
  close<<Text("Cerrar",Renderer::ToColor(237,246,248));
  const auto pref=m_saved_settings;
  const std::string active=m_home_open==1?pref.midiOutput:
       m_home_open==2?pref.midiInput:
       m_home_open==3?m_keyboard_size.GetValue():
       m_home_open==4?pref.audioDevice:m_state.song_path;
  for(int i=0;i<count;++i){
    const int index=m_home_page*visible+i,ry=y+64+i*rowH;
    if(index>=static_cast<int>(m_home_choices.size()))break;
    const auto& ch=m_home_choices[index];
    const bool selected=ch.value==active;
    renderer.SetColor(selected?41:(i%2?34:30),
                      selected?102:(i%2?55:49),selected?119:64,255);
    renderer.DrawQuad(x+12,ry,width-24,rowH-3);
    renderer.SetColor(255,255,255);
    TextWriter text(x+24,ry+8,renderer,false,15);
    std::string name=ch.label;
    const int maxChars=std::max(15,(width-105)/8);
    if(g_utf8_validate(name.c_str(),-1,nullptr) &&
       g_utf8_strlen(name.c_str(),-1)>maxChars){
      gchar* cut=g_utf8_substring(name.c_str(),0,maxChars-3);
      name=std::string(cut)+"...";
      g_free(cut);
    }else if(static_cast<int>(name.size())>maxChars)
      name=name.substr(0,maxChars-3)+"...";
    text<<Text(name,Renderer::ToColor(233,248,249));
    if(selected){
      TextWriter tick(x+width-53,ry+9,renderer,true,13);
      tick<<Text("OK",Renderer::ToColor(152,232,216));
    }
  }
  if(m_home_open==4){
    TextWriter hint(x+16,y+height-52,renderer,false,12);
    hint<<Text("Sólo FluidSynth interno · MIDI externo usa su propia salida",
               Renderer::ToColor(160,205,217));
  }
  const int pages=std::max(1,(static_cast<int>(m_home_choices.size())+
                               visible-1)/visible);
  TextWriter counter(x+17,y+height-26,renderer,false,13);
  counter<<Text(std::to_string(m_home_page+1)+" / "+std::to_string(pages),
                  Renderer::ToColor(187,215,227));
  if(pages>1){
    for(int i=0;i<2;++i){
      const int bx=x+width-(i?107:200);
      renderer.SetColor(55,91,103,255);
      renderer.DrawQuad(bx,y+height-37,83,29);
      renderer.SetColor(255,255,255);
      TextWriter button(bx+41,y+height-31,renderer,true,14);
      button<<Text(i?"Siguiente":"Anterior",
                   Renderer::ToColor(233,247,250));
    }
  }
}
