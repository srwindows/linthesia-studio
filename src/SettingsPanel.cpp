#include "TitleState.h"
#include "AdvancedSettings.h"
#include "FluidEngine.h"
#include "UserSettings.h"
#include "MidiComm.h"
#include "Renderer.h"
#include "Tga.h"
#include "TextWriter.h"
#include "Version.h"
#include "UiAppearance.h"
#include "MidiActivity.h"
#include "Effect3DPalette.h"
#include "StageScenes.h"
#include <SDL2/SDL.h>
#include <glib.h>
#include <sys/stat.h>
#include <algorithm>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>

namespace {
std::string clip(const std::string& v,int chars){
 if(chars<=5||!g_utf8_validate(v.c_str(),-1,nullptr)||g_utf8_strlen(v.c_str(),-1)<=chars)
  return v;
 gchar* cut=g_utf8_substring(v.c_str(),0,chars-3);
 std::string r=std::string(cut)+"...";g_free(cut);return r;
}
bool folderExists(const std::string& p){
 struct stat st{};
 return !p.empty() && stat(p.c_str(),&st)==0 && S_ISDIR(st.st_mode);
}
bool fontExists(const std::string& p){
 struct stat st{};
 if(stat(p.c_str(),&st)!=0 || !S_ISREG(st.st_mode)||p.size()<4)return false;
 return p.substr(p.size()-4)==".sf2" || p.substr(p.size()-4)==".sf3";
}
std::string onoff(bool b){return b?"Activado":"Desactivado";}
int cycle(int i,int n,int dir){return n?((i+n+dir%n)%n):0;}
void scanFonts(std::vector<std::string>& result,const std::string& dir){
 GDir* it=g_dir_open(dir.c_str(),0,nullptr);
 if(!it)return;
 const gchar* f=nullptr;
 while((f=g_dir_read_name(it))){
  const std::string candidate=dir+"/"+f;
  if(fontExists(candidate) &&
     std::find(result.begin(),result.end(),candidate)==result.end())
   result.push_back(candidate);
 }
 g_dir_close(it);
}
}
void TitleState::EnumerateSoundfonts(){
 m_soundfonts.clear();
 scanFonts(m_soundfonts,"/usr/share/sounds/sf2");
 scanFonts(m_soundfonts,"/usr/share/soundfonts");
 scanFonts(m_soundfonts,std::string(g_get_home_dir())+"/SoundFonts");
 scanFonts(m_soundfonts,std::string(g_get_home_dir())+"/.local/share/soundfonts");
 for(const auto& entry:m_draft_settings.extraSoundfonts)
    if(fontExists(entry) &&
       std::find(m_soundfonts.begin(),m_soundfonts.end(),entry)==m_soundfonts.end())
       m_soundfonts.push_back(entry);
 if(fontExists(m_draft_settings.soundfont) &&
    std::find(m_soundfonts.begin(),m_soundfonts.end(),
              m_draft_settings.soundfont)==m_soundfonts.end())
    m_soundfonts.push_back(m_draft_settings.soundfont);
 std::sort(m_soundfonts.begin(),m_soundfonts.end());
}
void TitleState::RefreshDeviceChoices(){
 MidiCommIn::UpdateDeviceList();MidiCommOut::UpdateDeviceList();
 m_input_choices=MidiCommIn::GetDeviceList();
 m_output_choices=MidiCommOut::GetDeviceList();
 m_draft_in_index=-1;m_draft_out_index=-1;
 for(size_t i=0;i<m_input_choices.size();++i)
  if(m_draft_settings.midiInput==m_input_choices[i].name)m_draft_in_index=i;
 for(size_t i=0;i<m_output_choices.size();++i)
  if(m_draft_settings.midiOutput==m_output_choices[i].name)m_draft_out_index=i;
 m_audio_devices={"default"};
 if(m_draft_settings.audioDriver=="pulseaudio"){
  gchar* stdoutText=nullptr;
  GError* spawnError=nullptr;
  gchar* argv[]={const_cast<gchar*>("pactl"),
                 const_cast<gchar*>("list"),
                 const_cast<gchar*>("short"),
                 const_cast<gchar*>("sinks"),nullptr};
  const gboolean ran=g_spawn_sync(nullptr,argv,nullptr,G_SPAWN_SEARCH_PATH,
                        nullptr,nullptr,&stdoutText,nullptr,nullptr,&spawnError);
  if(ran && stdoutText){
   std::string output(stdoutText);
   size_t start=0;
   while(start<output.size()){
    const size_t end=output.find('\n',start);
    const std::string line=output.substr(start,end==std::string::npos?
                                          std::string::npos:end-start);
    const auto first=line.find('\t');
    const auto second=first==std::string::npos?std::string::npos:
                            line.find('\t',first+1);
    if(first!=std::string::npos){
      std::string name=line.substr(first+1,second==std::string::npos?
                                          std::string::npos:second-first-1);
      if(!name.empty())m_audio_devices.push_back(name);
    }
    if(end==std::string::npos)break;
    start=end+1;
   }
  }
  if(stdoutText)g_free(stdoutText);
  if(spawnError)g_error_free(spawnError);
 }
 if(m_draft_settings.audioDriver=="alsa"){
  void** hints=nullptr;
  if(snd_device_name_hint(-1,"pcm",&hints)==0 && hints){
   for(void** p=hints;*p;p++){
    char* name=snd_device_name_get_hint(*p,"NAME");
    if(name && *name && std::string(name)!="null")
     m_audio_devices.push_back(name);
    free(name);
   }
   snd_device_name_free_hint(hints);
  }
 }
 if(!m_draft_settings.audioDevice.empty() &&
    std::find(m_audio_devices.begin(),m_audio_devices.end(),
              m_draft_settings.audioDevice)==m_audio_devices.end())
  m_audio_devices.push_back(m_draft_settings.audioDevice);
 auto pos=std::find(m_audio_devices.begin(),m_audio_devices.end(),
                  m_draft_settings.audioDevice);
 m_audio_device_index=std::distance(m_audio_devices.begin(),pos);
 if(m_audio_device_index>=static_cast<int>(m_audio_devices.size()))m_audio_device_index=0;
}
std::string TitleState::SettingsDeviceName(bool input)const{
 const auto& devices=input?m_input_choices:m_output_choices;
 const int i=input?m_draft_in_index:m_draft_out_index;
 return i<0 || i>=static_cast<int>(devices.size())?"Desactivado":devices[i].name;
}
void TitleState::OpenAdvancedSettings(){
 m_settings_open=true;
 m_draft_settings=AdvancedSettingsIO::Load();
 m_draft_settings.fps=m_refresh_rate;
 m_draft_keyboard=m_keyboard_size;
 // Mantener la preferencia almacenada aunque el sintetizador
 // esté desconectado al abrir la pantalla de configuración.
 // El dispositivo conectado se informa en las filas de estado.
 if(m_draft_settings.libraryPath.empty())
  m_draft_settings.libraryPath=UserSetting::Get(SONG_LIB_PATH_KEY,MUSICDIR);
 m_saved_settings=m_draft_settings;
 m_section=0;m_settings_page=0;m_editing_path=0;
 m_choice_open_id=0;m_choice_page=0;
 m_reset_pending=false;
 m_settings_dirty=false;m_settings_status="Ajustes cargados.";
 EnumerateSoundfonts();RefreshDeviceChoices();
 if(m_state.midi_out)m_output_tile->TurnOffPreview();
}
void TitleState::CloseAdvancedSettings(){
 CancelSettingsFileDialog();
 if(m_test_note){FluidEngine::NoteOff();m_test_note=false;}
 if((!m_state.midi_out ||
     m_state.midi_out->GetDeviceDescription().name!="FluidSynth interno") &&
    FluidEngine::IsRunning())FluidEngine::Shutdown();
 m_settings_open=false;
 m_editing_path=0;m_choice_open_id=0;m_choice_page=0;
 m_settings_dirty=false;
}
void TitleState::OnTextInput(const std::string& value){
 if(value.find('\n')!=std::string::npos || value.find('\r')!=std::string::npos)return;
 std::string* p=m_editing_path==30?&m_draft_settings.soundfont:
                m_editing_path==40?&m_draft_settings.libraryPath:nullptr;
 if(p && p->size()+value.size()<2048){
  // Draft text is committed only with Intro. Esc/click outside restores
  // the original path and does not create a phantom pending change.
  *p+=value;
 }
}
std::vector<TitleState::SettingsRow> TitleState::SettingsRows()const{
 const auto& a=m_draft_settings;
 std::vector<SettingsRow> r;
 auto add=[&](int id,const std::string& name,const std::string& value,
              const std::string& hint="",bool locked=false){
  r.push_back({name,value,hint,id,locked});
 };
 switch(m_section){
 case 0:{
  add(1,"Frecuencia de actualización",std::to_string(a.fps)+" FPS","Se aplica por completo en el próximo arranque.");
  add(2,"Tamaño del piano",m_draft_keyboard.AsText(),"37, 49, 61, 76 u 88 teclas.");
  add(3,"Resolución de ventana",std::to_string(a.windowWidth)+" × "+std::to_string(a.windowHeight));
  add(4,"Pantalla completa",onoff(a.fullscreen));
  int windowW=1280,windowH=720;
  SDL_Window* current=SDL_GetKeyboardFocus();
  if(current)SDL_GetWindowSize(current,&windowW,&windowH);
  const int realZoom=UiAppearance::EffectiveScaleFor(a.uiScale,windowW,windowH);
  add(5,"Escala de interfaz",std::to_string(a.uiScale)+" %"+
      (realZoom==a.uiScale?"":" ("+std::to_string(realZoom)+" % efectivo)"),
      "La ampliación depende del espacio disponible.");

  add(6,"Tema visual",(a.theme==1?"Azul metálico":a.theme==2?"Alto contraste":"Clásico"),
      "Colores de paneles, fondo y acentos.");
  break;
 }
 case 1:
  add(10,"Motor de salida",a.audioDriver=="pulseaudio"?"PulseAudio / PipeWire":
                                a.audioDriver=="alsa"?"ALSA":"JACK");
  add(11,"Dispositivo de audio",a.audioDevice,"Salida FluidSynth (default = predeterminada).");
  add(12,"Frecuencia de muestreo",std::to_string(a.sampleRate)+" Hz");
  add(13,"Tamaño de buffer",std::to_string(a.bufferSize)+" muestras");
  add(14,"Volumen de FluidSynth",std::to_string(a.masterVolume)+" %",
      "Control de volumen del motor interno.");
  add(16,"Refuerzo USB FANTOM con limitador",
      a.fantomUsbBoostDb==0?"Desactivado":
      "+"+std::to_string(a.fantomUsbBoostDb)+" dB",
      "Sólo FANTOM USB; normaliza nivel con limitador a -1,5 dB. "
      "Baja el volumen de altavoces antes de aumentarlo.");
  add(15,"Prueba de sonido","Reproducir Do4","Sonará durante 350 milisegundos.");
  break;
 case 2:
  add(20,"Entrada MIDI",SettingsDeviceName(true),
      "Selecciona un puerto MIDI detectado.");
  add(21,"Salida MIDI",SettingsDeviceName(false),
      "Incluye FluidSynth interno como destino.");
  add(22,"Canal de entrada",a.midiChannel==0?"Omni":std::to_string(a.midiChannel));
  add(25,"MIDI Thru",onoff(a.midiThru),
      "Envía notas recibidas a la salida, sin alterar el aprendizaje.");
  add(23,"Entrada conectada",m_state.midi_in?
      m_state.midi_in->GetDeviceDescription().name:"Sin entrada","Estado real actual.",true);
  add(24,"Salida conectada",m_state.midi_out?
      m_state.midi_out->GetDeviceDescription().name:"Sin salida","Estado real actual.",true);
  add(26,"Monitor MIDI","Reiniciar contador",
      "16 canales en tiempo real, sin transmitir notas.");
  break;
 case 3:
  add(30,"Ruta del SoundFont",a.soundfont,
      "Examinar permite elegir visualmente un archivo .sf2 o .sf3.");
  add(31,"SoundFonts disponibles",std::to_string(m_soundfonts.size())+
      " bancos · Seleccionado: "+a.soundfont,
      "Puedes elegir bancos registrados o del sistema.");
  add(37,"Añadir SoundFont","Explorar archivos .sf2 y .sf3",
      "Registra un banco de cualquier carpeta, sin copiarlo.");
  add(38,"Quitar del catálogo","Desregistrar banco seleccionado",
      "No borra los archivos SF2/SF3 originales.");
  add(32,"Polifonía",std::to_string(a.polyphony)+" voces");
  add(33,"Ganancia",std::to_string(a.gain)+" %");
  add(34,"Reverberación",onoff(a.reverb));
  add(35,"Chorus",onoff(a.chorus));
  add(70,"Reverb · Tamaño",std::to_string(a.reverbSize)+" %",
      "Dimensión virtual de la sala.");
  add(71,"Reverb · Amortiguación",std::to_string(a.reverbDamp)+" %",
      "Absorción de altas frecuencias.");
  add(72,"Reverb · Ancho estéreo",std::to_string(a.reverbWidth)+" %");
  add(73,"Reverb · Nivel",std::to_string(a.reverbLevel)+" %");
  add(74,"Chorus · Voces",std::to_string(a.chorusVoices));
  add(75,"Chorus · Nivel",std::to_string(a.chorusLevel)+" %");
  add(76,"Chorus · Velocidad",std::to_string(a.chorusRate/10)+"."+
      std::to_string(a.chorusRate%10)+" Hz");
  add(77,"Chorus · Profundidad",std::to_string(a.chorusDepth)+" ms");
  add(36,"Probar banco","Reproducir Do4","Usa el banco y los efectos seleccionados.");
  break;
 case 4:{
  add(40,"Carpeta principal MIDI",a.libraryPath,
      "Examinar abre el selector de carpetas de KDE.");
  add(41,"Recordar última carpeta",onoff(a.rememberFolder));
  add(42,"Orden alfabético",onoff(a.alphabeticOrder),
      "Desactivar permite respetar el orden manual guardado.");
  add(44,"Recientes · cantidad",std::to_string(a.recentLimit)+" archivos",
      "Máximo de canciones reproducidas que se muestran en Recientes.");
  const char* sorts[]={"Alfabético","Última reproducción",
                        "Número de reproducciones","Puntos conseguidos"};
  add(45,"Recientes · ordenar por",sorts[std::max(0,std::min(a.recentSort,3))],
      "Solo cambia la presentación; no mueve archivos MIDI.");
  add(43,"Última carpeta",UserSetting::Get(SONG_LIB_DIR_SETTINGS_KEY,""),
      "Información de la biblioteca.",true);
  break;
 }
 case 5:
  add(50,"Selección múltiple",onoff(a.multipleSelect));
  add(51,"Pausa para abrir carpeta",std::to_string(a.dragHoverMs)+" ms",
      "Durante arrastrar y soltar.");
  add(52,"Intervalo del doble clic",std::to_string(a.doubleClickMs)+" ms");
  add(53,"Confirmación al eliminar","Obligatoria",
      "No se permite eliminar sin aprobación.",true);
  add(54,"Deshacer y Rehacer","50 operaciones por sesión",
      "Protección de archivos activos.",true);
  break;
 case 7:
  add(80,"Velocidad de los efectos",std::to_string(a.effectSpeed)+" %",
      "Movimiento de la llama y las partículas, sin modificar los BPM.");
  add(81,"Cantidad de partículas",std::to_string(a.effectCount),
      "Emisión simultánea por nota MIDI (0 desactiva las partículas).");
  add(82,"Dispersión de partículas",std::to_string(a.effectSpread)+" %",
      "Anchura del zigzag y separación lateral respecto a la tecla.");
  add(83,"Tamaño de las partículas",std::to_string(a.effectSize)+" %",
      "Escala del destello y de su pequeña estela luminosa.");
  add(84,"Paleta de colores",Effect3DPalette::Name(a.effectPalette),
      "Presets cromáticos originales; Original conserva el color de pista.");
  add(85,"Nacimiento de las notas",a.effectBirth?
      "Explosión superior → fuego descendente":"Clásico (actual)",
      "Elige cómo nace cada nota; se conserva el impacto inferior.");
  break;
 case 8:
  add(90,"Escenario 3D",StageScenes::Name(a.scenePreset),
      "Siete escenas modeladas en Blender; puedes volver al fondo original.");
  add(91,"Intensidad de luces",std::to_string(a.sceneLightIntensity)+" %",
      "Iluminación ambiental superpuesta al render del escenario.");
  add(92,"Color del juego de luces",StageScenes::ColorName(a.sceneLightColor),
      "Selección independiente de los colores de las pistas MIDI.");
  add(93,"Movimiento de cámara",std::to_string(a.sceneCameraMotion)+" %",
      "Desplazamiento cinematográfico suave, sin mover el teclado.");
  add(94,"Animación del pianista",std::to_string(a.scenePianistMotion)+" %",
      "Animación Blender adaptativa al número de teclas MIDI activas.");
  break;
 case 6:
  add(60,"Diagnóstico en terminal",onoff(a.diagnostics));
  add(61,"FluidSynth",FluidEngine::Status(),"Estado real del sintetizador.",true);
  add(62,"Puertos MIDI",std::to_string(m_input_choices.size())+" entradas, "+
      std::to_string(m_output_choices.size())+" salidas","Dispositivos detectados.",true);
  add(63,"Archivo de configuración",AdvancedSettingsIO::Filename(),
      "Ajustes persistentes en el usuario.",true);
  add(64,"Versión de Linthesia",LinthesiaVersionString,"Información real.",true);
  break;
 }
 return r;
}
// Every enumerated setting presents all possible choices before a change.
// Hardware destinations and installed SF2/SF3 banks come from the live
// enumerations; no driver or device is represented as detected unless found.
std::vector<std::string> TitleState::SettingsOptionsFor(int id)const {
 const auto& a=m_draft_settings;
 const std::vector<std::string> switches={"Desactivado","Activado"};
 switch(id){
 case 1:return {"30 FPS","60 FPS","120 FPS","300 FPS"};
 case 2:return {"37 teclas","49 teclas","61 teclas","76 teclas","88 teclas"};
 case 3:return {"960 × 600","1024 × 640","1280 × 720",
                "1600 × 900","1920 × 1080"};
 case 4:case 25:case 34:case 35:case 41:case 42:case 50:case 60:
   return switches;
 case 5:return {"75 %","100 %","125 %","150 %"};
 case 6:return {"Clásico","Azul metálico","Alto contraste"};
 case 10:return {"PulseAudio / PipeWire","ALSA","JACK"};
 case 11:return m_audio_devices;
 case 12:return {"44100 Hz","48000 Hz","96000 Hz"};
 case 13:return {"256 muestras","512 muestras","1024 muestras","2048 muestras"};
 case 16:return {"Desactivado","+6 dB","+12 dB","+18 dB","+24 dB"};
 case 14:case 33:{
    std::vector<std::string> result;
    for(int x=0;x<=100;x+=10)result.push_back(std::to_string(x)+" %");
    return result;
 }
 case 20:case 21:{
    std::vector<std::string> result={"Desactivado"};
    for(const auto& d:(id==20?m_input_choices:m_output_choices))
      result.push_back(d.name);
    return result;
 }
 case 22:{
    std::vector<std::string> result={"Omni"};
    for(int x=1;x<=16;++x)result.push_back(std::to_string(x));
    return result;
 }
 case 31:{
    if(m_soundfonts.empty())return {};
    return m_soundfonts;
 }
 case 32:return {"32 voces","64 voces","128 voces","256 voces",
                 "512 voces","1024 voces"};
 case 70:case 71:case 72:case 73:case 75:{
    std::vector<std::string> vals;
    for(int pct=0;pct<=100;pct+=10)
       vals.push_back(std::to_string(pct)+" %");
    return vals;
 }
 case 74:{
    std::vector<std::string> vals;
    for(int n=1;n<=16;++n)vals.push_back(std::to_string(n)+" voces");
    return vals;
 }
 case 76:{
    std::vector<std::string> vals;
    for(int n=1;n<=50;++n)
       vals.push_back(std::to_string(n/10)+"."+std::to_string(n%10)+" Hz");
    return vals;
 }
 case 77:{
    std::vector<std::string> vals;
    for(int n=0;n<=21;++n)
       vals.push_back(std::to_string(n)+" ms");
    return vals;
 }
 case 44:return {"5 archivos","10 archivos","20 archivos","30 archivos",
                 "50 archivos","100 archivos","200 archivos","500 archivos"};
 case 45:return {"Alfabético","Última reproducción",
                 "Número de reproducciones","Puntos conseguidos"};
 case 51:{
    std::vector<std::string> result;
    for(int x=400;x<=1600;x+=100)result.push_back(std::to_string(x)+" ms");
    return result;
 }
 case 52:{
    std::vector<std::string> result;
    for(int x=250;x<=700;x+=50)result.push_back(std::to_string(x)+" ms");
    return result;
 }
 case 80:case 81:case 82:case 83:{
   std::vector<std::string> result;
   const int begin=id==80?25:(id==83?25:0);
   const int end=id==81?144:(id==83?220:200);
   const int step=id==81?12:(id==82?10:5);
   for(int x=begin;x<=end;x+=step)
      result.push_back(std::to_string(x)+(id==81?"":" %"));
   return result;
 }
 case 90:{
   std::vector<std::string> options={"Sin escenario (fondo actual)"};
   for(int i=6;i<StageScenes::Count;++i)
     options.push_back(StageScenes::Name(i));
   return options;
 }
 case 92:{
   std::vector<std::string> options;
   for(int i=0;i<7;++i)options.push_back(StageScenes::ColorName(i));
   return options;
 }
 case 91:case 93:case 94:{
   std::vector<std::string> options;
   for(int v=0;v<=100;v+=10)options.push_back(std::to_string(v)+" %");
   return options;
 }
 case 85:return {"Clásico (actual)","Explosión superior → fuego descendente"};
 case 84:{
   std::vector<std::string> result;
   for(int i=0;i<Effect3DPalette::Count;++i)
       result.push_back(Effect3DPalette::Name(i));
   return result;
 }
 default:return {};
 }
}
void TitleState::CancelSettingsFileDialog(){
 if(m_file_dialog_pid){
   kill(m_file_dialog_pid,SIGTERM);
   int status=0;
   (void)waitpid(m_file_dialog_pid,&status,WNOHANG);
   g_spawn_close_pid(m_file_dialog_pid);
   m_file_dialog_pid=0;
 }
 if(m_file_dialog_fd>=0){
   close(m_file_dialog_fd);
   m_file_dialog_fd=-1;
 }
 m_file_dialog_id=0;
 m_file_dialog_output.clear();
}
void TitleState::PollSettingsFileDialog(){
 if(!m_file_dialog_pid)return;
 char buf[1024];
 if(m_file_dialog_fd>=0){
   for(;;){
     const ssize_t n=read(m_file_dialog_fd,buf,sizeof(buf));
     if(n>0){
       if(m_file_dialog_output.size()<4096)
         m_file_dialog_output.append(buf,std::min(
                 static_cast<size_t>(n),
                 static_cast<size_t>(4096)-m_file_dialog_output.size()));
       continue;
     }
     if(n<0 && errno==EINTR)continue;
     break; // No output available: never block the rendering loop.
   }
 }
 int status=0;
 const pid_t finished=waitpid(m_file_dialog_pid,&status,WNOHANG);
 if(finished==0)return;
 const int id=m_file_dialog_id;
 std::string selected=m_file_dialog_output;
 if(m_file_dialog_fd>=0){close(m_file_dialog_fd);m_file_dialog_fd=-1;}
 g_spawn_close_pid(m_file_dialog_pid);
 m_file_dialog_pid=0;m_file_dialog_id=0;m_file_dialog_output.clear();
 if(finished<0){
    m_settings_status="El selector de archivos se ha cerrado.";
    return;
 }
 if(!WIFEXITED(status)||WEXITSTATUS(status)!=0)return; // Cancelled.
 while(!selected.empty() && (selected.back()=='\n'||selected.back()=='\r'))
    selected.pop_back();
 if(selected.empty())return;
 if((id==30||id==37) && !fontExists(selected)){
    m_settings_status="Selecciona un SoundFont .sf2 o .sf3.";
    return;
 }
 if(id==40 && !folderExists(selected)){
    m_settings_status="Selecciona una carpeta existente.";
    return;
 }
 if(id==30||id==37){
    m_draft_settings.soundfont=selected;
    auto& registered=m_draft_settings.extraSoundfonts;
    if(std::find(registered.begin(),registered.end(),selected)==registered.end())
       registered.push_back(selected);
    EnumerateSoundfonts();
 }else if(id==40)m_draft_settings.libraryPath=selected;
 else return;
 m_settings_dirty=true;
 m_settings_status="Ruta seleccionada. Pulsa Aplicar para guardar.";
}
void TitleState::BrowseSettingsPath(int id){
 if(id!=30&&id!=40&&id!=37)return;
 if(m_file_dialog_pid){
    m_settings_status="Ya está abierto el selector de archivos.";
    return;
 }
 if(!g_find_program_in_path("kdialog")){
    m_settings_status="No se encontró el explorador de archivos KDE.";
    return;
 }
 const bool isSoundfont=id==30||id==37;
 const std::string current=isSoundfont?m_draft_settings.soundfont:
                                            m_draft_settings.libraryPath;
 const std::string starting=current.empty()?std::string(g_get_home_dir()):
                                          current;
 GPid child=0;
 int outputFd=-1;
 GError* error=nullptr;
 gboolean ok=false;
 if(isSoundfont){
   gchar* argv[]={const_cast<gchar*>("kdialog"),
      const_cast<gchar*>("--getopenfilename"),
      const_cast<gchar*>(starting.c_str()),
      const_cast<gchar*>("*.sf2 *.sf3|Archivos SoundFont"),
      const_cast<gchar*>("--title"),
      const_cast<gchar*>("Seleccionar SoundFont"),nullptr};
   ok=g_spawn_async_with_pipes(nullptr,argv,nullptr,
       static_cast<GSpawnFlags>(G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD),
       nullptr,nullptr,&child,nullptr,&outputFd,nullptr,&error);
 }else{
   gchar* argv[]={const_cast<gchar*>("kdialog"),
      const_cast<gchar*>("--getexistingdirectory"),
      const_cast<gchar*>(starting.c_str()),
      const_cast<gchar*>("--title"),
      const_cast<gchar*>("Seleccionar carpeta MIDI"),nullptr};
   ok=g_spawn_async_with_pipes(nullptr,argv,nullptr,
       static_cast<GSpawnFlags>(G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD),
       nullptr,nullptr,&child,nullptr,&outputFd,nullptr,&error);
 }
 if(!ok){
    m_settings_status=std::string("No se pudo abrir el explorador: ")+
                           (error?error->message:"error desconocido");
    if(error)g_error_free(error);
    return;
 }
 // File picker must never stop drawing, input events or Cancel.
 m_file_dialog_pid=child;
 m_file_dialog_id=id;
 m_file_dialog_fd=outputFd;
 m_file_dialog_output.clear();
 const int flags=fcntl(outputFd,F_GETFL,0);
 if(flags>=0)fcntl(outputFd,F_SETFL,flags|O_NONBLOCK);
 m_settings_status="Explorador abierto. Linthesia sigue operativa.";
}
void TitleState::SelectSettingsChoice(int id,int index){
 const auto choices=SettingsOptionsFor(id);
 if(index<0||index>=static_cast<int>(choices.size()))return;
 AdvancedSettings& a=m_draft_settings;
 const bool yes=index==1;
 switch(id){
 case 1:a.fps=std::vector<int>{30,60,120,300}.at(index);break;
 case 2:m_draft_keyboard=KeyboardSize(std::to_string(
              std::vector<int>{37,49,61,76,88}.at(index)));break;
 case 3:{
   const std::vector<std::pair<int,int>> sizes={{960,600},{1024,640},
      {1280,720},{1600,900},{1920,1080}};
   a.windowWidth=sizes[index].first;
   a.windowHeight=sizes[index].second;break;
 }
 case 4:a.fullscreen=yes;break;
 case 5:a.uiScale=std::vector<int>{75,100,125,150}.at(index);break;
 case 6:a.theme=index;break;
 case 10:
   a.audioDriver=std::vector<std::string>{"pulseaudio","alsa","jack"}.at(index);
   a.audioDevice="default";RefreshDeviceChoices();break;
 case 11:a.audioDevice=choices[index];m_audio_device_index=index;break;
 case 12:a.sampleRate=std::vector<int>{44100,48000,96000}.at(index);break;
 case 13:a.bufferSize=std::vector<int>{256,512,1024,2048}.at(index);break;
 case 14:a.masterVolume=index*10;break;
 case 16:a.fantomUsbBoostDb=index*6;break;
 case 20:m_draft_in_index=index-1;
   a.midiInput=index==0?"":choices[index];break;
 case 21:m_draft_out_index=index-1;
   a.midiOutput=index==0?"":choices[index];break;
 case 22:a.midiChannel=index;break;
 case 25:a.midiThru=yes;break;
 case 31:a.soundfont=choices[index];break;
 case 32:a.polyphony=std::vector<int>{32,64,128,256,512,1024}.at(index);break;
 case 33:a.gain=index*10;break;
 case 34:a.reverb=yes;break;
 case 35:a.chorus=yes;break;
 case 70:a.reverbSize=index*10;break;
 case 71:a.reverbDamp=index*10;break;
 case 72:a.reverbWidth=index*10;break;
 case 73:a.reverbLevel=index*10;break;
 case 74:a.chorusVoices=index+1;break;
 case 75:a.chorusLevel=index*10;break;
 case 76:a.chorusRate=index+1;break;
 case 77:a.chorusDepth=index;break;
 case 41:a.rememberFolder=yes;break;
 case 42:a.alphabeticOrder=yes;break;
 case 44:a.recentLimit=std::vector<int>{5,10,20,30,50,100,200,500}.at(index);break;
 case 45:a.recentSort=index;break;
 case 50:a.multipleSelect=yes;break;
 case 51:a.dragHoverMs=400+index*100;break;
 case 52:a.doubleClickMs=250+index*50;break;
 case 80:a.effectSpeed=25+index*5;break;
 case 81:a.effectCount=index*12;break;
 case 82:a.effectSpread=index*10;break;
 case 83:a.effectSize=25+index*5;break;
 case 84:a.effectPalette=index;break;
 case 85:a.effectBirth=index;break;
 case 90:
   a.scenePreset=index==0?-1:StageScenes::ClampPreset(index+5);
   if(index>=1){
     // The pianist must be seen from the side to show his hands touching the
     // keyboard. All singers and instrumentalists face the audience by default.
     a.sceneYaw=index==1?88:0;
     a.scenePitch=index==1?28:18;
     a.sceneZoom=index==1?385:(index==7?585:(index==6?435:340));
     a.sceneView=0;a.sceneCameraFocus=0;
   }
   break;
 case 91:a.sceneLightIntensity=index*10;break;
 case 92:a.sceneLightColor=index;break;
 case 93:a.sceneCameraMotion=index*10;break;
 case 94:a.scenePianistMotion=index*10;break;
 case 60:a.diagnostics=yes;break;
 default:return;
 }
 m_settings_dirty=true;
 m_settings_status="Cambios pendientes · Aplicar para guardar.";
}
void TitleState::ModifySetting(int id,int direction){
 AdvancedSettings& a=m_draft_settings;
 auto choose=[&](int value,const std::vector<int>& values)->int{
  auto pos=std::find(values.begin(),values.end(),value);
  int index=pos==values.end()?0:static_cast<int>(pos-values.begin());
  return values[cycle(index,values.size(),direction)];
 };
 auto chooseStr=[&](const std::string& value,const std::vector<std::string>& values)->std::string{
  auto pos=std::find(values.begin(),values.end(),value);
  int index=pos==values.end()?0:static_cast<int>(pos-values.begin());
  return values[cycle(index,values.size(),direction)];
 };
 switch(id){
 case 1:a.fps=choose(a.fps,{30,60,120,300});break;
 case 2:if(direction<0)m_draft_keyboard.previous();else m_draft_keyboard.next();break;
 case 3:{
  const std::vector<std::pair<int,int>> modes={{960,600},{1024,640},{1280,720},
                                                {1600,900},{1920,1080}};
  int i=0;
  for(size_t j=0;j<modes.size();j++)
   if(modes[j].first==a.windowWidth)i=j;
  auto selected=modes[cycle(i,modes.size(),direction)];
  a.windowWidth=selected.first;a.windowHeight=selected.second;
  break;
 }
 case 4:a.fullscreen=!a.fullscreen;break;
 case 5:a.uiScale=std::max(75,std::min(150,a.uiScale+direction*25));break;
 case 6:a.theme=(a.theme+3+direction)%3;break;
 case 10:a.audioDriver=chooseStr(a.audioDriver,{"pulseaudio","alsa","jack"});
         a.audioDevice="default";RefreshDeviceChoices();break;
 case 11:
  m_audio_device_index=cycle(m_audio_device_index,m_audio_devices.size(),direction);
  a.audioDevice=m_audio_devices[m_audio_device_index];break;
 case 12:a.sampleRate=choose(a.sampleRate,{44100,48000,96000});break;
 case 13:a.bufferSize=choose(a.bufferSize,{256,512,1024,2048});break;
 case 14:a.masterVolume=std::max(0,std::min(100,a.masterVolume+direction*10));break;
 case 16:a.fantomUsbBoostDb=std::max(0,std::min(24,
       a.fantomUsbBoostDb+direction*6));break;
 case 15:case 36:PlayAudioTest();return;
 case 20:
  m_draft_in_index=m_input_choices.empty()?-1:
     (direction>0?(m_draft_in_index+1)%static_cast<int>(m_input_choices.size()):
      m_draft_in_index<=0?-1:m_draft_in_index-1);
  a.midiInput=m_draft_in_index<0?"":m_input_choices[m_draft_in_index].name;
  break;
 case 21:
  m_draft_out_index=m_output_choices.empty()?-1:
     (direction>0?(m_draft_out_index+1)%static_cast<int>(m_output_choices.size()):
      m_draft_out_index<=0?-1:m_draft_out_index-1);
  a.midiOutput=m_draft_out_index<0?"":m_output_choices[m_draft_out_index].name;
  break;
 case 22:a.midiChannel=(a.midiChannel+17+direction)%17;break;
 case 25:a.midiThru=!a.midiThru;break;
 case 26:MidiActivity::Reset();
         m_settings_status="Monitor MIDI reiniciado.";return;
 case 30:case 40:
  m_editing_path=id;m_settings_status="Escribe la ruta y pulsa Intro.";
  return;
 case 31:
  if(m_soundfonts.empty()){m_settings_status="No se encontraron SoundFonts.";return;}
  {
   auto pos=std::find(m_soundfonts.begin(),m_soundfonts.end(),a.soundfont);
   int index=pos==m_soundfonts.end()?0:pos-m_soundfonts.begin();
   a.soundfont=m_soundfonts[cycle(index,m_soundfonts.size(),direction)];
  }
  break;
 case 32:a.polyphony=choose(a.polyphony,{32,64,128,256,512,1024});break;
 case 33:a.gain=std::max(0,std::min(100,a.gain+direction*10));break;
 case 34:a.reverb=!a.reverb;break;
 case 35:a.chorus=!a.chorus;break;
 case 41:a.rememberFolder=!a.rememberFolder;break;
 case 42:a.alphabeticOrder=!a.alphabeticOrder;break;
 case 44:a.recentLimit=choose(a.recentLimit,{5,10,20,30,50,100,200,500});break;
 case 45:a.recentSort=(a.recentSort+4+direction)%4;break;
 case 50:a.multipleSelect=!a.multipleSelect;break;
 case 51:a.dragHoverMs=std::max(400,std::min(1600,a.dragHoverMs+direction*100));break;
 case 52:a.doubleClickMs=std::max(250,std::min(700,a.doubleClickMs+direction*50));break;
 case 80:a.effectSpeed=std::max(25,std::min(200,a.effectSpeed+direction*5));break;
 case 81:a.effectCount=std::max(0,std::min(144,a.effectCount+direction*12));break;
 case 82:a.effectSpread=std::max(0,std::min(200,a.effectSpread+direction*10));break;
 case 83:a.effectSize=std::max(25,std::min(220,a.effectSize+direction*5));break;
 case 84:a.effectPalette=cycle(a.effectPalette,Effect3DPalette::Count,direction);break;
 case 85:a.effectBirth=cycle(a.effectBirth,2,direction);break;
 case 90:{
   const int current=a.scenePreset<6?0:a.scenePreset-5;
   const int next=cycle(current,StageScenes::Count-5,direction);
   a.scenePreset=next==0?-1:StageScenes::ClampPreset(next+5);
   break;
 }
 case 91:a.sceneLightIntensity=std::max(0,std::min(100,a.sceneLightIntensity+direction*10));break;
 case 92:a.sceneLightColor=cycle(a.sceneLightColor,7,direction);break;
 case 93:a.sceneCameraMotion=std::max(0,std::min(100,a.sceneCameraMotion+direction*10));break;
 case 94:a.scenePianistMotion=std::max(0,std::min(100,a.scenePianistMotion+direction*10));break;
 case 60:a.diagnostics=!a.diagnostics;break;
 default:return;
 }
 m_settings_dirty=true;
 m_settings_status="Cambios pendientes · Aplicar para guardar.";
}
void TitleState::PlayAudioTest(){
 std::string error;
 if(!FluidEngine::Start(m_draft_settings,error)){
  m_settings_status="Audio: "+error;
  return;
 }
 FluidEngine::AllSoundsOff();
 FluidEngine::NoteOn();
 m_test_note=true;
 m_test_note_until=SDL_GetTicks()+350;
 m_settings_status="Prueba de sonido activa: Do4.";
}
bool TitleState::ApplyAdvancedSettings(bool close){
 const AdvancedSettings& a=m_draft_settings;
 if(!folderExists(a.libraryPath)){
  m_settings_status="La carpeta de biblioteca no existe. No se guardó.";
  return false;
 }
 if(!fontExists(a.soundfont)){
  m_settings_status="El SoundFont SF2/SF3 no existe. No se guardó.";
  return false;
 }
 // Hardware output is changed only after the user explicitly applies.
 // Prepare FluidSynth before touching the previous MIDI device connection.
 const bool fluidSelected=a.midiOutput=="FluidSynth interno";
 const bool fluidParametersChanged=
  a.soundfont!=m_saved_settings.soundfont ||
  a.audioDriver!=m_saved_settings.audioDriver ||
  a.audioDevice!=m_saved_settings.audioDevice ||
  a.sampleRate!=m_saved_settings.sampleRate ||
  a.bufferSize!=m_saved_settings.bufferSize ||
  a.masterVolume!=m_saved_settings.masterVolume ||
  a.fantomUsbBoostDb!=m_saved_settings.fantomUsbBoostDb ||
  a.gain!=m_saved_settings.gain ||
  a.polyphony!=m_saved_settings.polyphony ||
  a.reverb!=m_saved_settings.reverb ||
  a.chorus!=m_saved_settings.chorus ||
  a.reverbSize!=m_saved_settings.reverbSize ||
  a.reverbDamp!=m_saved_settings.reverbDamp ||
  a.reverbWidth!=m_saved_settings.reverbWidth ||
  a.reverbLevel!=m_saved_settings.reverbLevel ||
  a.chorusVoices!=m_saved_settings.chorusVoices ||
  a.chorusLevel!=m_saved_settings.chorusLevel ||
  a.chorusRate!=m_saved_settings.chorusRate ||
  a.chorusDepth!=m_saved_settings.chorusDepth;
 if(fluidSelected && (!FluidEngine::IsRunning() || fluidParametersChanged)){
  std::string error;
  if(!FluidEngine::Start(a,error)){
   m_settings_status="No se aplicó: "+error;
   return false;
  }
 }
 std::string error;
 if(!AdvancedSettingsIO::Save(a,error)){
  m_settings_status="No se guardó: "+error;
  return false;
 }
 UserSetting::Set(REFRESH_RATE_KEY,std::to_string(a.fps));
 UserSetting::Set(SONG_LIB_PATH_KEY,a.libraryPath);
 UserSetting::Set(KEYBOARD_SIZE_KEY,m_draft_keyboard.GetValue());

 const std::string currentOutput=m_state.midi_out?
    m_state.midi_out->GetDeviceDescription().name:"";
 const std::string currentInput=m_state.midi_in?
    m_state.midi_in->GetDeviceDescription().name:"";
 if(currentOutput!=a.midiOutput){
  if(m_state.midi_out){m_state.midi_out->Reset();delete m_state.midi_out;m_state.midi_out=nullptr;}
  if(!a.midiOutput.empty()){
   auto devices=MidiCommOut::GetDeviceList();
   for(size_t i=0;i<devices.size();i++){
    if(devices[i].name!=a.midiOutput)continue;
    try{m_state.midi_out=new MidiCommOut(i);}catch(...){m_state.midi_out=nullptr;}
    break;
   }
  }
 }
 if(currentInput!=a.midiInput){
  if(m_state.midi_in){m_state.midi_in->Reset();delete m_state.midi_in;m_state.midi_in=nullptr;}
  if(!a.midiInput.empty()){
   auto devices=MidiCommIn::GetDeviceList();
   for(size_t i=0;i<devices.size();i++){
    if(devices[i].name!=a.midiInput)continue;
    try{m_state.midi_in=new MidiCommIn(i);}catch(...){m_state.midi_in=nullptr;}
    break;
   }
  }
 }
 UserSetting::Set(OUTPUT_DEVICE_KEY,m_state.midi_out?
           m_state.midi_out->GetDeviceDescription().name:"[no output device]");
 UserSetting::Set(INPUT_DEVICE_KEY,m_state.midi_in?
           m_state.midi_in->GetDeviceDescription().name:"[no input device]");
 m_refresh_rate=a.fps;
 m_keyboard_size=m_draft_keyboard;
 m_state.keyboard=Keyboard::GetKeyboardDefaults(m_keyboard_size);
 // Refresh the underlying home-screen device widgets to avoid stale selection.
 int out=-1,in=-1;
 const auto outputs=MidiCommOut::GetDeviceList();
 const auto inputs=MidiCommIn::GetDeviceList();
 if(m_state.midi_out){
  for(size_t i=0;i<outputs.size();i++)
   if(outputs[i].name==m_state.midi_out->GetDeviceDescription().name)out=i;
 }
 if(m_state.midi_in){
  for(size_t i=0;i<inputs.size();i++)
   if(inputs[i].name==m_state.midi_in->GetDeviceDescription().name)in=i;
 }
 delete m_output_tile;
 m_output_tile=new DeviceTile(0,0,out,DeviceTileOutput,outputs,
            GetTexture(InterfaceButtons),GetTexture(OutputBox));
 delete m_input_tile;
 m_input_tile=new DeviceTile(0,0,in,DeviceTileInput,inputs,
            GetTexture(InterfaceButtons),GetTexture(InputBox));
 Resize();
 // Change window geometry last. Resize triggers a new UI layout on the next frame.
 SDL_Window* win=SDL_GetKeyboardFocus();
 if(!win)win=SDL_GetMouseFocus();
 if(win){
  const bool full=(SDL_GetWindowFlags(win)&SDL_WINDOW_FULLSCREEN_DESKTOP)!=0;
  if(full!=a.fullscreen)
   SDL_SetWindowFullscreen(win,a.fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0);
  // Do not undo manual window resizing when applying only color/zoom.
  if(!a.fullscreen &&
     (a.windowWidth!=m_saved_settings.windowWidth ||
      a.windowHeight!=m_saved_settings.windowHeight))
   SDL_SetWindowSize(win,a.windowWidth,a.windowHeight);
 }
 m_saved_settings=a;
 m_settings_dirty=false;
 m_settings_status="Configuración aplicada y guardada.";
 if(a.diagnostics)g_message("Linthesia: ajustes aplicados; audio=%s",a.audioDriver.c_str());
 if(!fluidSelected && FluidEngine::IsRunning())FluidEngine::Shutdown();
 UiAppearance::Configure(a.uiScale,a.theme);
 if(close)CloseAdvancedSettings();
 return true;
}
void TitleState::ResetSettingsSection(){
 const AdvancedSettings def;
 auto& a=m_draft_settings;
 switch(m_section){
 case 0:a.fps=def.fps;a.windowWidth=def.windowWidth;
        a.windowHeight=def.windowHeight;a.fullscreen=def.fullscreen;a.uiScale=def.uiScale;a.theme=def.theme;
        m_draft_keyboard=KeyboardSize(KeyboardSize::Size88);break;
 case 1:a.audioDriver=def.audioDriver;a.audioDevice=def.audioDevice;
        a.sampleRate=def.sampleRate;a.bufferSize=def.bufferSize;
        a.masterVolume=def.masterVolume;RefreshDeviceChoices();break;
 case 2:a.midiInput.clear();a.midiOutput.clear();
        a.midiChannel=0;a.midiThru=false;
        m_draft_in_index=-1;m_draft_out_index=-1;break;
 case 3:a.soundfont=def.soundfont;a.polyphony=def.polyphony;a.gain=def.gain;
        a.reverb=def.reverb;a.chorus=def.chorus;break;
 case 4:a.libraryPath=UserSetting::Get(SONG_LIB_PATH_KEY,MUSICDIR);
        a.rememberFolder=true;a.alphabeticOrder=false;break;
 case 5:a.multipleSelect=true;a.dragHoverMs=800;a.doubleClickMs=400;break;
 case 6:a.diagnostics=false;break;
 case 8:a.scenePreset=def.scenePreset;a.sceneLightIntensity=def.sceneLightIntensity;
        a.sceneLightColor=def.sceneLightColor;
        a.sceneCameraMotion=def.sceneCameraMotion;
        a.scenePianistMotion=def.scenePianistMotion;break;
 case 7:a.effectSpeed=def.effectSpeed;a.effectCount=def.effectCount;
        a.effectSpread=def.effectSpread;a.effectSize=def.effectSize;
        a.effectPalette=def.effectPalette;a.effectBirth=def.effectBirth;break;
 }
 m_settings_dirty=true;
 m_settings_status="Valores restaurados en esta sección. Pulsa Aplicar.";
}
void TitleState::UpdateAdvancedSettings(const MouseInfo& mouse){
 PollSettingsFileDialog();
 if(m_test_note && SDL_GetTicks()>=m_test_note_until){
  FluidEngine::NoteOff();m_test_note=false;
 }
 // MIDI monitor reads only an explicitly selected input. No MIDI-Thru here.
 if(m_section==2 && m_state.midi_in){
  int count=0;
  while(count++<64 && m_state.midi_in->KeepReading())
   (void)m_state.midi_in->Read();
 }
 if(m_editing_path){
  std::string* text=m_editing_path==30?&m_draft_settings.soundfont:
                    &m_draft_settings.libraryPath;
  const int W=GetStateWidth(),H=GetStateHeight();
  const int width=std::min(W-32,1160),height=std::min(H-104,680);
  const int x=(W-width)/2,y=(H-height)/2;
  const int side=std::max(151,std::min(210,width/5));
  const int contentX=x+side+24;
  const int contentW=width-side-42;
  const int rowY=y+119,rowH=58;
  const int maxRows=std::max(2,(height-240)/rowH);
  const auto rows=SettingsRows();
  bool onEditedText=false;
  for(int local=0;local<maxRows;++local){
     const int index=m_settings_page*maxRows+local;
     if(index>=static_cast<int>(rows.size()))break;
     if(rows[index].id!=m_editing_path)continue;
     const int yy=rowY+local*rowH;
     onEditedText=mouse.x>=contentX+3 &&
        mouse.x<contentX+contentW-120 &&
        mouse.y>=yy && mouse.y<yy+rowH-3;
     break;
  }
  if(IsKeyPressed(KeyEscape)){
     *text=m_path_before_edit;
     m_editing_path=0;m_path_paste_latch=false;
     m_settings_status="Edición de ruta cancelada.";
     return;
  }
  if(IsKeyPressed(KeyEnter)){
     if((m_editing_path==30 && !fontExists(*text)) ||
        (m_editing_path==40 && !folderExists(*text))){
        m_settings_status="Ruta no válida. Selecciona un archivo o carpeta existente.";
        return;
     }
     if(m_editing_path==30){
        auto& registered=m_draft_settings.extraSoundfonts;
        if(std::find(registered.begin(),registered.end(),*text)==registered.end())
           registered.push_back(*text);
        EnumerateSoundfonts();
     }
     m_editing_path=0;m_path_paste_latch=false;
     m_settings_dirty=true;
     m_settings_status="Ruta editada. Pulsa Aplicar para guardar.";
     return;
  }
  // A click outside the text releases editing immediately, and is then
  // handled below as a normal click (Cancel, section, Examine, etc.).
  if(mouse.released.left && !onEditedText){
     *text=m_path_before_edit;
     m_editing_path=0;m_path_paste_latch=false;
     m_settings_status="Edición de ruta cancelada.";
  }else{
     const Uint8* keys=SDL_GetKeyboardState(nullptr);
     const bool paste=(SDL_GetModState() & KMOD_CTRL)!=0 &&
                       keys[SDL_SCANCODE_V];
     if(paste && !m_path_paste_latch){
       char* clipboardText=SDL_GetClipboardText();
       if(clipboardText){
          OnTextInput(clipboardText);
          SDL_free(clipboardText);
       }
     }
     m_path_paste_latch=paste;
     if(IsKeyPressed(KeyBackspace)&&!text->empty()){
        const gchar* begin=text->c_str();
        const gchar* previous=g_utf8_find_prev_char(begin,begin+text->size());
        if(previous)text->resize(previous-begin);
     }
     return;
  }
 }
 // A real choice panel intercepts all input. No cycling blind through
 // values, and no accidental application while a list is open.
 if(m_choice_open_id){
    const auto options=SettingsOptionsFor(m_choice_open_id);
    const int W=GetStateWidth(),H=GetStateHeight();
    const int width=std::min(W-38,710),height=std::min(H-44,560);
    const int x=(W-width)/2,y=(H-height)/2;
    const int rowH=35, visible=std::max(1,(height-145)/rowH);
    const int pages=std::max(1,(static_cast<int>(options.size())+
                               visible-1)/visible);
    m_choice_page=std::max(0,std::min(m_choice_page,pages-1));
    if(IsKeyPressed(KeyEscape)){m_choice_open_id=0;return;}
    if(IsKeyPressed(KeyLeft)&&m_choice_page>0){--m_choice_page;return;}
    if(IsKeyPressed(KeyRight)&&m_choice_page+1<pages){
        ++m_choice_page;return;
    }
    if(!mouse.released.left)return;
    auto inside=[&](int xx,int yy,int ww,int hh){
        return mouse.x>=xx && mouse.x<xx+ww &&
               mouse.y>=yy && mouse.y<yy+hh;
    };
    if(inside(x+width-102,y+12,88,30) ||
       !inside(x,y,width,height)){
        m_choice_open_id=0;return;
    }
    if(pages>1){
        if(inside(x+width-214,y+height-48,88,32)){
            if(m_choice_page>0)--m_choice_page;
            return;
        }
        if(inside(x+width-112,y+height-48,88,32)){
            if(m_choice_page+1<pages)++m_choice_page;
            return;
        }
    }
    if(inside(x+14,y+74,width-28,visible*rowH)){
        const int index=m_choice_page*visible+(mouse.y-y-74)/rowH;
        if(index>=0&&index<static_cast<int>(options.size())){
            const int id=m_choice_open_id;
            m_choice_open_id=0;m_choice_page=0;
            SelectSettingsChoice(id,index);
        }
    }
    return;
 }
 if(IsKeyPressed(KeyEscape)){CloseAdvancedSettings();return;}
 const int W=GetStateWidth(),H=GetStateHeight();
 const int width=std::min(W-32,1160);
 const int height=std::min(H-104,680);
 const int x=(W-width)/2, y=(H-height)/2;
 const int side=std::max(151,std::min(210,width/5));
 const int contentX=x+side+24;
 const int contentW=width-side-42;
 const int rowY=y+119,rowH=58;
 const int maxRows=std::max(2,(height-240)/rowH);
 const auto rows=SettingsRows();
 const int pages=std::max(1,((int)rows.size()+maxRows-1)/maxRows);
 m_settings_page=std::max(0,std::min(m_settings_page,pages-1));
 if(!mouse.released.left)return;
 const auto inside=[&](int xx,int yy,int ww,int hh){
  return mouse.x>=xx && mouse.x<xx+ww && mouse.y>=yy && mouse.y<yy+hh;
 };
 const int categoryStep=std::max(30,std::min(55,(height-135)/9));
 for(int i=0;i<9;i++){
  if(inside(x+12,y+105+i*categoryStep,side-10,categoryStep-5)){
   m_section=i;m_settings_page=0;m_settings_status.clear();
   m_reset_pending=false;
   if(i==2)RefreshDeviceChoices();
   return;
  }
 }
 if(inside(contentX+contentW-116,y+65,50,30) && m_settings_page>0){
  --m_settings_page;return;
 }
 if(inside(contentX+contentW-58,y+65,50,30) && m_settings_page+1<pages){
  ++m_settings_page;return;
 }
 const int buttonY=y+height-54;
 const int buttonW=std::max(85,std::min(115,(contentW-46)/4));
 for(int i=0;i<4;++i){
  const int bx=contentX+6+i*(buttonW+10);
  if(!inside(bx,buttonY,buttonW,39))continue;
  switch(i){
   case 0:
    if(!m_reset_pending){
     m_reset_pending=true;
     m_settings_status="Pulsa Restaurar otra vez para confirmar.";
    }else{
     ResetSettingsSection();m_reset_pending=false;
    }
    return;
   case 1:CloseAdvancedSettings();return;
   case 2:ApplyAdvancedSettings(false);return;
   case 3:ApplyAdvancedSettings(true);return;
  }
 }
 for(int local=0;local<maxRows;local++){
  const int index=m_settings_page*maxRows+local;
  if(index>=(int)rows.size())break;
  const auto& row=rows[index];
  const int ry=rowY+local*rowH;
  if(!inside(contentX+3,ry,contentW-5,rowH-3))continue;
  if(row.readOnly)return;
  m_reset_pending=false;
  if(row.id==30 || row.id==40){
   const int right=contentX+contentW;
   if(inside(right-110,ry+11,95,34)){
      BrowseSettingsPath(row.id);
   }else if(inside(right-192,ry+11,76,34)){
      m_path_before_edit=row.id==30?m_draft_settings.soundfont:
                                    m_draft_settings.libraryPath;
      m_editing_path=row.id;
      // Replace the full path, rather than forcing users to append to it.
      if(row.id==30)m_draft_settings.soundfont.clear();
      else m_draft_settings.libraryPath.clear();
      m_settings_status="Escribe o pega la ruta nueva · Intro confirma · Esc cancela.";
   }
   // Clicking a route elsewhere must not trap mouse and keyboard.
   return;
  }
  // Only action rows execute on click. Every enumerated row opens all
  // choices, with selection performed after the user has seen the list.
  if(row.id==15 || row.id==36 || row.id==26){
     ModifySetting(row.id,1);
     return;
  }
  if(row.id==37){BrowseSettingsPath(37);return;}
  if(row.id==38){
      auto& extra=m_draft_settings.extraSoundfonts;
      const std::string current=m_draft_settings.soundfont;
      auto it=std::find(extra.begin(),extra.end(),current);
      if(it==extra.end()){
        m_settings_status="Este banco no está registrado por el usuario.";
        return;
      }
      extra.erase(it);
      m_draft_settings.soundfont="/usr/share/sounds/sf2/default-GM.sf2";
      EnumerateSoundfonts();
      m_settings_dirty=true;
      m_settings_status="Registro quitado, archivo intacto. Pulsa Aplicar.";
      return;
  }
  const auto options=SettingsOptionsFor(row.id);
  if(options.empty()){
     m_settings_status="No hay opciones disponibles para este ajuste.";
     return;
  }
  m_choice_open_id=row.id;
  m_choice_page=0;
  return;
 }
}
void TitleState::DrawAdvancedSettings(Renderer& renderer)const{
 const int W=GetStateWidth(),H=GetStateHeight();
 const int width=std::min(W-32,1160);
 const int height=std::min(H-104,680);
 const int x=(W-width)/2,y=(H-height)/2;
 const int side=std::max(151,std::min(210,width/5));
 const int contentX=x+side+24;
 const int contentW=width-side-42;
 renderer.SetColor(21,29,32);
 renderer.DrawQuad(x,y,width,height);
 renderer.SetColor(87,149,148);
 renderer.DrawQuad(x,y,width,3);
 renderer.SetColor(35,48,51);
 renderer.DrawQuad(x+8,y+90,side,height-105);
 renderer.SetColor(255,255,255);
 TextWriter heading(x+25,y+18,renderer,false,28);
 heading << Text("Configuración de Linthesia",Renderer::ToColor(238,244,241));
 TextWriter tagline(x+26,y+54,renderer,false,14);
 tagline << Text("Pantalla · MIDI · Efectos 3D · Escenarios editables",
                 Renderer::ToColor(162,199,193));
 const char* categories[]={"Pantalla","Sonido","MIDI","FluidSynth",
                           "Biblioteca","Controles","Avanzado","Efectos 3D","Escenarios 3D"};
 const int categoryStep=std::max(30,std::min(55,(height-135)/9));
 for(int i=0;i<9;i++){
  const int cy=y+105+i*categoryStep;
  renderer.SetColor(i==m_section?62:41,i==m_section?101:61,
                    i==m_section?109:65);
  renderer.DrawQuad(x+12,cy,side-10,categoryStep-5);
  if(i==m_section){
   renderer.SetColor(110,195,191);
   renderer.DrawQuad(x+12,cy,4,categoryStep-5);
  }
  renderer.SetColor(255,255,255);
  TextWriter label(x+24,cy+std::max(7,(categoryStep-26)/2),
                   renderer,false,categoryStep<43?13:16);
  label << Text(categories[i],Renderer::ToColor(i==m_section?247:206,
                                                i==m_section?252:217,
                                                i==m_section?249:214));
 }
 TextWriter section(contentX+7,y+78,renderer,false,22);
 section << Text(categories[m_section],Renderer::ToColor(236,245,240));
 const auto rows=SettingsRows();
 const int rowH=58,rowY=y+119;
 const int maxRows=std::max(2,(height-240)/rowH);
 const int pages=std::max(1,((int)rows.size()+maxRows-1)/maxRows);
 if(pages>1){
  TextWriter page(contentX+contentW-148,y+70,renderer,false,13);
  page << Text(std::to_string(m_settings_page+1)+"/"+std::to_string(pages),
               Renderer::ToColor(195,217,209));
  for(int i=0;i<2;i++){
   const int xx=contentX+contentW-(i?58:116);
   renderer.SetColor(56,82,85);
   renderer.DrawQuad(xx,y+65,50,30);
   renderer.SetColor(255,255,255);
   TextWriter nav(xx+25,y+70,renderer,true,17);
   nav << Text(i?">":"<",Renderer::ToColor(221,235,230));
  }
 }
 for(int local=0;local<maxRows;local++){
  const int index=m_settings_page*maxRows+local;
  if(index>=(int)rows.size())break;
  const auto& row=rows[index];
  const int ry=rowY+local*rowH;
  renderer.SetColor(local%2?37:42,local%2?55:60,local%2?59:64);
  renderer.DrawQuad(contentX+3,ry,contentW-5,rowH-3);
  renderer.SetColor(255,255,255);
  TextWriter label(contentX+15,ry+7,renderer,false,15);
  label << Text(row.name,Renderer::ToColor(224,237,230));
  const int maxChars=std::max(8,(contentW-((row.id==30||row.id==40)?255:175))/8);
  const std::string value=clip(row.value,maxChars);
  TextWriter val(contentX+17,ry+30,renderer,false,14);
  val << Text(value+(m_editing_path==row.id?"_":""),
               Renderer::ToColor(row.readOnly?168:244,row.readOnly?191:251,
                                 row.readOnly?181:241));
  if(row.id==84){
    for(int swatch=0;swatch<8;++swatch){
      renderer.SetColor(Effect3DPalette::Color(
          Track::ColorNoteWhite[swatch],m_draft_settings.effectPalette,swatch));
      renderer.DrawQuad(contentX+280+swatch*23,ry+36,18,10);
    }
  }
  if(!row.readOnly){
    const bool path=row.id==30||row.id==40;
    const bool action=row.id==15||row.id==36||row.id==26||
                      row.id==37||row.id==38;
    const int ax=contentX+contentW-110;
    renderer.SetColor(71,111,112);
    renderer.DrawQuad(ax,ry+11,95,34);
    renderer.SetColor(255,255,255);
    TextWriter select(ax+47,ry+19,renderer,true,14);
    select << Text(path?"Examinar":row.id==37?"Añadir":row.id==38?
                    "Quitar":action?"Ejecutar":"Elegir opciones",
                   Renderer::ToColor(237,243,238));
    if(path){
       const int bx=contentX+contentW-192;
       renderer.SetColor(71,111,112);
       renderer.DrawQuad(bx,ry+11,76,34);
       renderer.SetColor(255,255,255);
       TextWriter edit(bx+38,ry+19,renderer,true,14);
       edit << Text("Editar",Renderer::ToColor(237,243,238));
    }
  }
 }
 if(m_section==2 && m_settings_page>0){
  const auto snapshot=MidiActivity::Now();
  const int barY=y+height-169;
  TextWriter monitor(contentX+7,barY-32,renderer,false,13);
  monitor << Text(clip(snapshot.last+" · Eventos: "+
              std::to_string(snapshot.events),std::max(30,contentW/8)),
              Renderer::ToColor(194,229,218));
  const int barWidth=std::max(9,(contentW-20)/16-3);
  for(int ch=0;ch<16;ch++){
   const int bx=contentX+7+ch*(barWidth+3);
   renderer.SetColor(38,58,62);
   renderer.DrawQuad(bx,barY,barWidth,45);
   const int high=std::max(0,45*snapshot.bars[ch]/127);
   renderer.SetColor(65+std::min(80,snapshot.bars[ch]),162,152);
   if(high)renderer.DrawQuad(bx,barY+45-high,barWidth,high);
   TextWriter chlabel(bx+barWidth/2,barY+48,renderer,true,10);
   chlabel << Text(std::to_string(ch+1),Renderer::ToColor(195,214,206));
  }
 }
 const int statusY=y+height-92;
 renderer.SetColor(255,255,255);
 TextWriter status(contentX+8,statusY,renderer,false,13);
 status << Text(clip(m_settings_status,std::max(22,(contentW-22)/8)),
                Renderer::ToColor(168,210,198));
 renderer.SetColor(77,102,102);
 renderer.DrawQuad(contentX+4,y+height-64,contentW-8,1);
 const char* buttonNames[]={"Restaurar","Cancelar","Aplicar","Aceptar"};
 const int bw=std::max(85,std::min(115,(contentW-46)/4));
 for(int i=0;i<4;i++){
  const int bx=contentX+6+i*(bw+10);
  const int by=y+height-54;
  renderer.SetColor(i==3?66:53,i==3?111:83,i==3?114:86);
  renderer.DrawQuad(bx,by,bw,39);
  renderer.SetColor(255,255,255);
  TextWriter button(bx+bw/2,by+11,renderer,true,15);
  button << Text(buttonNames[i],Renderer::ToColor(236,247,242));
 }
 if(m_choice_open_id){
    const auto options=SettingsOptionsFor(m_choice_open_id);
    const auto allRows=SettingsRows();
    std::string caption="Seleccionar opción";
    for(const auto& item:allRows)
      if(item.id==m_choice_open_id){caption=item.name;break;}
    const int width=std::min(W-38,710);
    const int h=std::min(H-44,560);
    const int left=(W-width)/2,top=(H-h)/2;
    const int rowHeight=35;
    const int count=std::max(1,(h-145)/rowHeight);
    const int pages=std::max(1,(static_cast<int>(options.size())+
                               count-1)/count);
    renderer.SetColor(23,33,37);
    renderer.DrawQuad(left,top,width,h);
    renderer.SetColor(111,163,159);
    renderer.DrawQuad(left,top,width,3);
    renderer.SetColor(255,255,255);
    TextWriter title(left+20,top+18,renderer,false,22);
    title << Text("Elegir: "+caption,Renderer::ToColor(239,248,242));
    renderer.SetColor(64,100,100);
    renderer.DrawQuad(left+width-102,top+12,88,30);
    renderer.SetColor(255,255,255);
    TextWriter close(left+width-58,top+20,renderer,true,14);
    close << Text("Cerrar",Renderer::ToColor(239,243,241));
    TextWriter subtitle(left+20,top+53,renderer,false,13);
    subtitle << Text(std::to_string(options.size())+
                     " opciones disponibles · Elige una",
                     Renderer::ToColor(184,206,205));
    const auto rowsHere=SettingsRows();
    std::string current;
    for(const auto& row:rowsHere)
      if(row.id==m_choice_open_id){current=row.value;break;}
    if(m_choice_open_id==31)current=m_draft_settings.soundfont;
    for(int i=0;i<count;++i){
      const int index=m_choice_page*count+i;
      if(index>=static_cast<int>(options.size()))break;
      const int ry=top+74+i*rowHeight;
      const bool selected=options[index]==current;
      renderer.SetColor(selected?64:(i%2?43:48),
                        selected?103:(i%2?59:65),
                        selected?113:(i%2?64:68));
      renderer.DrawQuad(left+14,ry,width-28,rowHeight-2);
      if(selected){
        renderer.SetColor(124,197,184);
        renderer.DrawQuad(left+14,ry,4,rowHeight-2);
      }
      renderer.SetColor(255,255,255);
      TextWriter option(left+27,ry+7,renderer,false,16);
      option << Text(clip(options[index],std::max(12,(width-65)/8)),
                     Renderer::ToColor(selected?250:219,
                                       selected?254:235,
                                       selected?252:234));
      if(m_choice_open_id==90 && index>=1 && index<=6){
        const int scene=index+5;
        if(!m_scene_previews[scene]&&!m_scene_previews_failed[scene]){
          try{
            m_scene_previews[scene]=Tga::Load(
                   std::string("scenarios/escena_")+
                   (scene<10?"0":"")+std::to_string(scene));
            m_scene_previews[scene]->SetSmooth(true);
          }catch(const std::exception& error){
            fprintf(stderr,"Linthesia: vista previa escenario: %s\n",
                    error.what());
            m_scene_previews_failed[scene]=true;
          }
        }
        if(const Tga* thumbnail=m_scene_previews[scene]){
          renderer.SetColor(255,255,255);
          renderer.DrawStretchedTga(thumbnail,left+width-104,ry+2,74,30);
        }
      }
      if(m_choice_open_id==84){
        const int previewLeft=left+width-186;
        const int paletteIndex=index;
        for(int swatch=0;swatch<8;++swatch){
          renderer.SetColor(Effect3DPalette::Color(
               Track::ColorNoteWhite[swatch],paletteIndex,swatch));
          renderer.DrawQuad(previewLeft+swatch*19,ry+11,15,12);
        }
      }
    }
    if(pages>1){
      renderer.SetColor(255,255,255);
      TextWriter progress(left+24,top+h-38,renderer,false,13);
      progress << Text("Página "+std::to_string(m_choice_page+1)+
                       " de "+std::to_string(pages),
                       Renderer::ToColor(199,215,209));
      for(int i=0;i<2;++i){
        const int bx=left+width-(i?112:214);
        renderer.SetColor(67,96,95);
        renderer.DrawQuad(bx,top+h-48,88,32);
        renderer.SetColor(255,255,255);
        TextWriter nav(bx+44,top+h-40,renderer,true,14);
        nav << Text(i?"Siguiente":"Anterior",
                    Renderer::ToColor(233,240,236));
      }
    }
 }

}
