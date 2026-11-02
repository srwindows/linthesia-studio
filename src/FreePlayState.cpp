// SPDX-License-Identifier: GPL-2.0-or-later
#include "FreePlayState.h"
#include "TitleState.h"
#include "Renderer.h"
#include "TextWriter.h"
#include "libmidi/MidiEvent.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <iomanip>
#include <sstream>
namespace {
std::string clockLabel(uint64_t micros){
 const int seconds=int(micros/1000000);
 char buffer[32];
 snprintf(buffer,sizeof(buffer),"%02d:%02d",seconds/60,seconds%60);
 return buffer;
}
void button(Renderer& renderer,const ButtonState& box,
            const std::string& label,bool highlighted=false){
 renderer.SetColor(highlighted?45:22,highlighted?127:71,
                   highlighted?135:92,245);
 renderer.DrawRoundedGradient(box.x,box.y,box.w,box.h,6,
             Renderer::ToColor(highlighted?54:35,highlighted?148:104,
                               highlighted?150:129),
             Renderer::ToColor(12,40,56));
 renderer.SetColor(255,255,255);
 TextWriter text(box.x+10,box.y+(box.h-16)/2,renderer,false,14);
 text<<Text(label,Renderer::ToColor(242,248,252));
}
}
std::string FreePlayState::SessionPath()const{
 return std::string(g_get_user_data_dir())+"/linthesia/freeplay/last.session";
}
void FreePlayState::SaveSession(){
 const std::string dir=std::string(g_get_user_data_dir())+"/linthesia/freeplay";
 if(g_mkdir_with_parents(dir.c_str(),0700)!=0 ||
    !m_recorder.SaveSession(SessionPath()))
  m_notice="No se pudo guardar la sesión libre";
}
void FreePlayState::Init(){
 m_visual_origin=GetStateMicroseconds();
 m_visual_now=m_visual_origin;
 Resize();
 if(m_recorder.Restore(SessionPath()))
   m_notice="Sesión anterior recuperada sin tocar MIDI originales";
 else
   m_notice="Toca con el ratón o conecta un MIDI IN en Configuración";
}
int FreePlayState::KeyboardTop()const {
 return std::max(290,GetStateHeight()-173);
}
int FreePlayState::KeyboardLeft()const{
 return 30;
}
int FreePlayState::KeyboardWidth()const{
 return std::max(300,GetStateWidth()-60);
}
int FreePlayState::KeyX(int pitch)const{
 if(pitch<21||pitch>108)return KeyboardLeft();
 int count=0;
 for(int key=21;key<=pitch;++key){
  const int c=key%12;
  if(c!=1&&c!=3&&c!=6&&c!=8&&c!=10)++count;
 }
 const bool black=(pitch%12==1||pitch%12==3||pitch%12==6||
                   pitch%12==8||pitch%12==10);
 const float unit=float(KeyboardWidth())/52.f;
 return KeyboardLeft()+int((black?count:count-1)*unit-
                                 (black?unit*.32f:0.f));
}
int FreePlayState::KeyWidth(int pitch)const{
 const int degree=pitch%12;
 const bool black=degree==1||degree==3||degree==6||
                  degree==8||degree==10;
 return std::max(3,int(KeyboardWidth()/52.f*(black?.64f:1.f)));
}
void FreePlayState::Resize(){
 const int w=GetStateWidth(),h=GetStateHeight();
 const int top=20;
 m_back=ButtonState(24,top,91,39);
 m_record=ButtonState(126,top,142,39);
 m_stop=ButtonState(278,top,88,39);
 m_play=ButtonState(376,top,128,39);
 m_save=ButtonState(514,top,143,39);
 m_minus=ButtonState(681,top,34,39);
 m_plus=ButtonState(816,top,34,39);
 m_tracks_button=ButtonState(w-183,top,152,39);
 const int panel=w-350;
 const int start=142;
 const int step=h<720?46:53;
 for(size_t i=0;i<FreePlay::Recorder::MaxTracks;++i){
  const int row=start+int(i)*step;
  m_mutes[i]=ButtonState(panel+13,row+22,86,28);
  m_programs[i]=ButtonState(panel+103,row+22,125,28);
  m_deletes[i]=ButtonState(panel+233,row+22,96,28);
 }
 m_quant=ButtonState(panel+13,start+int(FreePlay::Recorder::MaxTracks)*step+9,
                      147,34);
}
void FreePlayState::ApplyPrograms(){
 if(!m_state.midi_out)return;
 for(size_t i=0;i<m_recorder.tracks.size();++i){
  auto simple=MidiEventSimple(uint8_t(0xC0|i),m_recorder.tracks[i].program,0);
  m_state.midi_out->Write(MidiEvent::Build(simple));
 }
}
void FreePlayState::StopPlayback(){
 m_visual.ReleaseAll(m_visual_now);
 if(m_state.midi_out)m_state.midi_out->Reset();
 m_playing=false;
 m_play_cursors.fill(0);
 m_pressed.fill(0);
}
void FreePlayState::StartPlayback(){
 if(m_recorder.tracks.empty()){
  m_notice="Graba una pista primero";return;
 }
 StopPlayback();
 ApplyPrograms();
 m_clock_start=GetStateMicroseconds();
 m_current=0;
 m_playing=true;
 m_notice="Reproduciendo las pistas grabadas";
}
void FreePlayState::StartRecording(){
 if(m_recorder.recording){return;}
 if(m_recorder.tracks.size()>=FreePlay::Recorder::MaxTracks){
  m_notice="Máximo seis pistas";return;
 }
 StopPlayback();
 if(!m_recorder.Start())return;
 ApplyPrograms();
 m_clock_start=GetStateMicroseconds();
 m_current=0;
 m_playing=true;
 m_notice="Grabando pista "+std::to_string(m_recorder.activeTrack+1);
 SaveSession();
}
void FreePlayState::StopRecording(){
 if(!m_recorder.recording)return;
 m_recorder.Stop(m_current);
 StopPlayback();
 SaveSession();
 m_notice="Pista grabada. Puedes grabar otra o exportar MIDI";
}
void FreePlayState::AdvancePlayback(){
 if(!m_playing)return;
 m_current=GetStateMicroseconds()-m_clock_start;
 bool remaining=false;
 for(size_t i=0;i<m_recorder.tracks.size();++i){
  // Never echo a track through its own live recording channel.
  if(m_recorder.recording && int(i)==m_recorder.activeTrack)continue;
  const auto& track=m_recorder.tracks[i];
  auto& cursor=m_play_cursors[i];
  while(cursor<track.events.size() &&
        track.events[cursor].microseconds<=m_current){
   const auto& e=track.events[cursor++];
   if(track.muted)continue;
   const uint8_t kind=e.status&0xf0;
   if(kind==0x80||kind==0x90){
     m_visual.Event(uint8_t(kind|i),e.data1,e.data2,m_visual_now);
     if(kind==0x90 && e.data2)m_pressed[e.data1]=1;
     else m_pressed[e.data1]=0;
   }
   if(m_state.midi_out && kind>=0x80&&kind<=0xe0){
    const auto ev=MidiEvent::Build(MidiEventSimple(
           uint8_t(kind|i),e.data1,e.data2));
    m_state.midi_out->Write(ev);
   }
  }
  if(cursor<track.events.size())remaining=true;
 }
 if(!remaining && !m_recorder.recording && m_current>100000){
  StopPlayback();
  m_notice="Reproducción terminada";
 }
}
void FreePlayState::RecordInput(){
 if(!m_state.midi_in)return;
 int n=0;
 while(m_state.midi_in->KeepReading() && n++<4096){
  auto event=m_state.midi_in->Read();
  MidiEventSimple simple;
  if(!event.GetSimpleEvent(&simple))continue;
  if(!FreePlay::Recorder::ValidEvent(simple.status,
                         simple.byte1,simple.byte2))continue;
  const uint8_t kind=simple.status&0xf0;
  if(kind==0x90||kind==0x80){
    m_visual.Event(simple.status,simple.byte1,simple.byte2,m_visual_now);
    if(kind==0x90&&simple.byte2)m_pressed[simple.byte1]=1;
    else m_pressed[simple.byte1]=0;
  }
  if(m_recorder.recording)
    m_recorder.Push(m_current,simple.status,simple.byte1,simple.byte2);
  // MIDI thru only to the output chosen by the user. No device changes.
  if(m_state.midi_out)
    m_state.midi_out->Write(event);
 }
}
int FreePlayState::KeyboardHit(int x,int y)const{
 const int top=KeyboardTop(),left=KeyboardLeft();
 if(x<left||x>=left+KeyboardWidth()||y<top||y>=top+98)return -1;
 // Black-key hit test first, as on a real piano.
 if(y<top+62){
  for(int pitch=21;pitch<=108;++pitch){
   const int degree=pitch%12;
   if(degree!=1&&degree!=3&&degree!=6&&degree!=8&&degree!=10)continue;
   const int x0=KeyX(pitch);
   if(x>=x0&&x<x0+KeyWidth(pitch))return pitch;
  }
 }
 for(int pitch=21;pitch<=108;++pitch){
  const int degree=pitch%12;
  if(degree==1||degree==3||degree==6||degree==8||degree==10)continue;
  const int x0=KeyX(pitch);
  if(x>=x0&&x<x0+KeyWidth(pitch))return pitch;
 }
 return -1;
}
void FreePlayState::KeyboardNote(int note,bool on){
 if(note<21||note>108)return;
 m_pressed[note]=on?1:0;
 m_visual.Event(uint8_t(on?0x90:0x80),uint8_t(note),
                uint8_t(on?98:0),m_visual_now);
 const auto e=MidiEventSimple(uint8_t(on?0x90:0x80),
                              uint8_t(note),uint8_t(on?98:0));
 if(m_state.midi_out)m_state.midi_out->Write(MidiEvent::Build(e));
 if(m_recorder.recording)
   m_recorder.Push(m_current,e.status,e.byte1,e.byte2);
}
void FreePlayState::Finish(){
 if(m_recorder.recording)StopRecording();
 StopPlayback();
 SaveSession();
}
void FreePlayState::Update(){
 m_visual_now=GetStateMicroseconds();
 m_visual.Update(m_visual_now);
 const auto mouse=Mouse();
 m_back.Update(mouse);m_record.Update(mouse);m_stop.Update(mouse);
 m_play.Update(mouse);m_save.Update(mouse);m_tracks_button.Update(mouse);
 if(m_show_tracks){
  m_quant.Update(mouse);
  for(size_t i=0;i<FreePlay::Recorder::MaxTracks;++i){
   m_mutes[i].Update(mouse);
   m_programs[i].Update(mouse);
   m_deletes[i].Update(mouse);
  }
 }else m_quant.hit=false;
 m_minus.Update(mouse);m_plus.Update(mouse);
 // The interactive 88-key piano remains independent of external MIDI.
 if(mouse.newPress.left){
  const int key=KeyboardHit(mouse.x,mouse.y);
  if(key>=0){m_mouse_note=key;KeyboardNote(key,true);}
 }
 if(mouse.released.left && m_mouse_note>=0){
  KeyboardNote(m_mouse_note,false);
  m_mouse_note=-1;
 }
 if(m_tracks_button.hit)m_show_tracks=!m_show_tracks;
 if(m_back.hit||IsKeyPressed(KeyEscape)){
  Finish();
  ChangeState(new TitleState(m_state));
  return;
 }
 if(m_stop.hit||IsKeyPressed(KeySpace)){
  if(m_recorder.recording)StopRecording();else StopPlayback();
 }
 if(m_record.hit){
  if(m_recorder.recording)StopRecording();
  else StartRecording();
 }
 if(m_play.hit&&!m_recorder.recording)StartPlayback();
 if(m_show_tracks&&m_quant.hit&&!m_recorder.recording){
  m_recorder.quantize=!m_recorder.quantize;
  SaveSession();
 }
 if((m_minus.hit||m_plus.hit)&&!m_recorder.recording){
  m_recorder.bpm=std::max(20,std::min(400,m_recorder.bpm+
                                  (m_plus.hit?1:-1)));
  SaveSession();
 }
 for(size_t i=0;m_show_tracks&&i<m_recorder.tracks.size();++i){
  if(m_mutes[i].hit){
   m_recorder.tracks[i].muted=!m_recorder.tracks[i].muted;
   SaveSession();
  }
  if(m_programs[i].hit&&!m_recorder.recording){
   const int program=(m_recorder.tracks[i].program+1)%128;
   m_recorder.SetProgram(i,program);
   SaveSession();
  }
  if(m_deletes[i].hit&&!m_recorder.recording){
   StopPlayback();
   m_recorder.Delete(i);
   SaveSession();
   m_notice="Pista eliminada";break;
  }
 }
 AdvancePlayback();
 RecordInput();
 if(m_save.hit&&!m_recorder.recording&&m_recorder.tracks.empty())
   m_notice="Graba primero una pista para poder guardar el MIDI";
 if(m_save.hit&&!m_recorder.recording&&!m_recorder.tracks.empty()){
  const std::string dir=std::string(g_get_home_dir())+
                               "/Música/Linthesia/Grabaciones";
  if(g_mkdir_with_parents(dir.c_str(),0700)!=0)
    m_notice="No se pudo crear la carpeta de grabaciones";
  else{
   const std::string filename=dir+"/Linthesia_"+std::to_string(
                                  g_get_real_time())+".mid";
   m_notice=m_recorder.Save(filename)?
            "MIDI exportado: "+filename:"Error al exportar MIDI";
  }
 }
}
void FreePlayState::Draw(Renderer& renderer)const{
 const int w=GetStateWidth(),h=GetStateHeight();
 const int ky=KeyboardTop(),kx=KeyboardLeft(),kw=KeyboardWidth();
 const int canvasTop=113,canvasBottom=ky-10;
 // Visual hierarchy: compact transport, immersive music area, one 88-key piano.
 renderer.SetColor(9,15,25,255);
 renderer.DrawQuad(0,0,w,h);
 renderer.SetColor(13,31,46);
 renderer.DrawQuad(0,0,w,79);
 renderer.SetColor(31,68,88);
 renderer.DrawQuad(0,77,w,2);
 // Soft note canvas; no permanent track controls obscuring the music.
 renderer.SetColor(8,20,34);
 renderer.DrawQuad(kx,canvasTop,kw,std::max(0,canvasBottom-canvasTop));
 renderer.SetColor(18,40,58,152);
 for(int i=1;i<5;++i){
  const int y=canvasTop+(canvasBottom-canvasTop)*i/5;
  renderer.DrawQuad(kx+1,y,kw-2,1);
 }
 for(int pitch=24;pitch<=108;pitch+=12){
  const int x=KeyX(pitch);
  renderer.SetColor(56,91,112,112);
  renderer.DrawQuad(x,canvasTop+1,1,canvasBottom-canvasTop);
 }
 // Every MIDI NoteOn makes a glowing upward trace. While held it grows,
 // on NoteOff its tail detaches and fades using smoothstep over 1.45s.
 // Draw completed notes first and held notes over them.
 for(int pass=0;pass<2;++pass){
  for(const auto& note:m_visual.notes){
   if(note.held!=(pass==1) || note.pitch<21||note.pitch>108)continue;
   const int alpha=m_visual.Alpha(note,m_visual_now);
   if(alpha<=0)continue;
   const double age=double(m_visual_now-note.onset)/1000000.0;
   const double off=note.held?0.0:
        double(std::max<uint64_t>(m_visual_now,note.release)-note.release)/1000000.0;
   const int scroll=int(age*94.0);
   const int noteBase=ky-12-int(off*94.0);
   const int top=ky-12-std::max(9,scroll);
   const int y1=std::max(canvasTop+2,top);
   const int y2=std::min(canvasBottom-2,noteBase);
   if(y2<=y1)continue;
   const bool high=note.pitch>=60;
   const bool black=note.pitch%12==1||note.pitch%12==3||
         note.pitch%12==6||note.pitch%12==8||note.pitch%12==10;
   const int x=KeyX(note.pitch);
   const int width=KeyWidth(note.pitch);
   const int innerX=x+std::max(1,width/6);
   const int innerW=std::max(3,width-std::max(2,width/3));
   const int glowW=std::max(width,innerW+8);
   renderer.SetColor(high?54:245,high?150:169,
                     high?255:87,std::max(0,alpha/7));
   renderer.DrawQuad(x-2,y1-3,glowW+4,y2-y1+6);
   renderer.SetColor(high?72:243,high?184:187,
                     high?255:113,alpha);
   renderer.DrawQuad(innerX,y1,innerW,y2-y1);
   renderer.SetColor(246,249,251,std::min(210,alpha*3/4));
   renderer.DrawQuad(innerX+1,y1,2,std::max(1,y2-y1));
   if(note.held){
    renderer.SetColor(high?136:255,high?224:216,high?255:150,
                       std::min(255,alpha));
    renderer.DrawQuad(x-2,ky-12,width+4,4);
   }
  }
 }
 renderer.SetColor(141,191,214);
 TextWriter canvasTitle(kx+16,canvasTop+14,renderer,false,16);
 canvasTitle<<Text("MODO LIBRE · INTERPRETACIÓN EN VIVO",
                   Renderer::ToColor(167,205,222));
 TextWriter hint(kx+16,canvasTop+37,renderer,false,12);
 hint<<Text("Toca con el ratón o con tu teclado MIDI · Las notas muestran su duración",
           Renderer::ToColor(116,153,171));
 // Small musical feedback without taking space from the notation.
 std::string activeLabel;
 static const char* names[]={"Do","Do#","Re","Re#","Mi","Fa",
                             "Fa#","Sol","Sol#","La","La#","Si"};
 for(int pitch=21;pitch<=108;++pitch){
  if(!m_pressed[pitch])continue;
  if(!activeLabel.empty())activeLabel+="  ";
  activeLabel+=std::string(names[pitch%12])+std::to_string(pitch/12-1);
  if(activeLabel.size()>45){activeLabel+="…";break;}
 }
 if(!activeLabel.empty()){
  TextWriter live(std::max(kx+330,kx+kw-400),canvasTop+16,
                  renderer,false,14);
  live<<Text(activeLabel,Renderer::ToColor(160,225,242));
 }
 // Keyboard with shared hit-test geometry, aligned exactly with note trails.
 renderer.SetColor(5,12,21);
 renderer.DrawQuad(kx-4,ky-6,kw+8,108);
 for(int pitch=21;pitch<=108;++pitch){
  const int degree=pitch%12;
  if(degree==1||degree==3||degree==6||degree==8||degree==10)continue;
  const int x=KeyX(pitch),width=KeyWidth(pitch);
  const bool active=m_pressed[pitch]!=0;
  renderer.SetColor(active?109:220,active?204:230,active?242:239);
  renderer.DrawQuad(x,ky,std::max(1,width-1),96);
  renderer.SetColor(active?60:145,active?141:171,active?197:183);
  renderer.DrawQuad(x,ky+93,std::max(1,width-1),3);
  if(degree==0){
   renderer.SetColor(18,48,65);
   TextWriter n(x+2,ky+75,renderer,false,11);
   n<<Text("C"+std::to_string(pitch/12-1),
                 Renderer::ToColor(43,65,79));
  }
 }
 for(int pitch=21;pitch<=108;++pitch){
  const int degree=pitch%12;
  if(degree!=1&&degree!=3&&degree!=6&&degree!=8&&degree!=10)continue;
  const int x=KeyX(pitch);
  const bool active=m_pressed[pitch]!=0;
  renderer.SetColor(active?74:21,active?151:33,active?214:49);
  renderer.DrawQuad(x,ky,KeyWidth(pitch),62);
  renderer.SetColor(active?121:37,active?205:52,active?252:68);
  renderer.DrawQuad(x+2,ky+2,std::max(1,KeyWidth(pitch)-4),3);
 }
 // Minimal transport and hierarchy.
 button(renderer,m_back,"Inicio");
 button(renderer,m_record,m_recorder.recording?"● Grabando":"● Grabar",
              m_recorder.recording);
 button(renderer,m_stop,"Parar");
 button(renderer,m_play,"Reproducir",m_playing&&!m_recorder.recording);
 button(renderer,m_save,"Guardar MIDI");
 button(renderer,m_minus,"−");
 button(renderer,m_plus,"+");
 button(renderer,m_tracks_button,m_show_tracks?"Ocultar pistas":"Pistas (6)",
                   m_show_tracks);
 renderer.SetColor(255,255,255);
 TextWriter bpm(722,31,renderer,false,17);
 bpm<<Text(std::to_string(m_recorder.bpm)+" BPM",
                       Renderer::ToColor(222,242,252));
 TextWriter timer(std::max(899,w-365),31,renderer,false,16);
 timer<<Text(clockLabel(m_playing?m_current:
               m_visual_now-m_visual_origin),Renderer::ToColor(161,217,231));
 const bool rec=m_recorder.recording;
 renderer.SetColor(rec?242:95,rec?111:206,rec?114:175);
 renderer.DrawQuad(kx,94,7,7);
 TextWriter state(kx+15,89,renderer,false,14);
 state<<Text(rec?"Grabando pista "+std::to_string(m_recorder.activeTrack+1):
             m_playing?"Reproduciendo pistas":"Preparado para tocar",
             Renderer::ToColor(rec?245:164,rec?150:211,rec?155:225));
 if(m_show_tracks){
  // Single collapsible studio panel, never competing with the playing area.
  const int panel=w-350;
  const int step=h<720?46:53;
  const int ptop=111;
  const int pheight=std::min(ky-ptop-6,418);
  renderer.SetColor(14,34,49,247);
  renderer.DrawQuad(panel,ptop,340,pheight);
  renderer.SetColor(34,112,139);
  renderer.DrawQuad(panel,ptop,3,pheight);
  renderer.SetColor(255,255,255);
  TextWriter panelTitle(panel+18,ptop+10,renderer,false,18);
  panelTitle<<Text("Pistas grabadas",Renderer::ToColor(228,241,247));
  for(size_t i=0;i<FreePlay::Recorder::MaxTracks;++i){
   const int row=142+int(i)*step;
   const bool existing=i<m_recorder.tracks.size();
   renderer.SetColor(i%2?22:19,i%2?52:46,i%2?68:61);
   renderer.DrawQuad(panel+9,row,320,step-3);
   renderer.SetColor(255,255,255);
   TextWriter rowTitle(panel+17,row+3,renderer,false,13);
   rowTitle<<Text("Pista "+std::to_string(i+1)+(existing?
      (" · "+std::to_string(m_recorder.tracks[i].events.size())+" eventos"):
       " · Vacía"),Renderer::ToColor(existing?236:137,
                                    existing?245:177,existing?247:193));
   if(existing){
    button(renderer,m_mutes[i],
            m_recorder.tracks[i].muted?"Silenciada":"Activada",
            !m_recorder.tracks[i].muted);
    button(renderer,m_programs[i],
            "Instrumento "+std::to_string(m_recorder.tracks[i].program+1));
    button(renderer,m_deletes[i],"Eliminar");
   }
  }
  button(renderer,m_quant,m_recorder.quantize?"Cuantizar: sí":"Cuantizar: no",
                                      m_recorder.quantize);
 }
 renderer.SetColor(17,34,46);
 renderer.DrawQuad(0,ky+105,w,std::max(0,h-ky-105));
 renderer.SetColor(255,255,255);
 TextWriter footer(kx,ky+115,renderer,false,12);
 const std::string concise=m_notice.size()>100?
                              m_notice.substr(0,97)+"…":m_notice;
 footer<<Text(concise,Renderer::ToColor(172,212,221));
 TextWriter device(std::max(15,w-365),ky+115,renderer,false,11);
 device<<Text(m_state.midi_in?"MIDI IN conectado":"MIDI IN no seleccionado",
                       Renderer::ToColor(134,172,184));
}
