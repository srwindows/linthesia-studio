// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LINTHESIA_MUSICXML_IMPORT_H
#define LINTHESIA_MUSICXML_IMPORT_H
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <string>
#include <stdexcept>
#include <memory>
namespace MusicXmlImport {
inline bool IsXml(const std::string& path) {
  const auto dot=path.find_last_of('.');
  if(dot==std::string::npos)return false;
  const auto ext=path.substr(dot);
  return g_ascii_strcasecmp(ext.c_str(),".musicxml")==0 ||
         g_ascii_strcasecmp(ext.c_str(),".mxl")==0 ||
         g_ascii_strcasecmp(ext.c_str(),".xml")==0;
}
inline std::string Converter() {
  // This is the data directory already installed by the project Meson build;
  // no shell interpolation or dynamic remote downloads are involved.
  const std::string graphics=GRAPHDIR;
  auto end=graphics.rfind("/graphics");
  if(end==std::string::npos)throw std::runtime_error("Ruta de MusicXML no configurada");
  const std::string script=graphics.substr(0,end)+"/scripts/musicxml_to_midi.py";
  if(!g_file_test(script.c_str(),G_FILE_TEST_IS_REGULAR))
    throw std::runtime_error("No se ha instalado el convertidor MusicXML: "+script);
  return script;
}
inline std::string Convert(const std::string& path) {
  struct stat st{};
  if(lstat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode)||st.st_size>16*1024*1024)
    throw std::runtime_error("MusicXML ausente, no es archivo regular o supera 16 MiB");
  auto raw=path+"|"+std::to_string(st.st_size)+"|"+
           std::to_string(static_cast<long long>(st.st_mtime));
  gchar* hash=g_compute_checksum_for_string(G_CHECKSUM_SHA256,
                                           raw.c_str(),-1);
  if(!hash)throw std::runtime_error("No se pudo calcular la identidad MusicXML");
  const std::string cache=std::string(g_get_user_cache_dir())+
                           "/linthesia/musicxml";
  if(g_mkdir_with_parents(cache.c_str(),0700)!=0){
    g_free(hash);
    throw std::runtime_error("No se puede crear el caché de MusicXML");
  }
  const std::string out=cache+"/"+std::string(hash)+".mid";
  g_free(hash);
  if(g_file_test(out.c_str(),G_FILE_TEST_IS_REGULAR) &&
     g_file_test((out+".score.tsv").c_str(),G_FILE_TEST_IS_REGULAR))
    return out;
  const std::string script=Converter();
  gchar* argv[]={
      const_cast<char*>("python3"),const_cast<char*>(script.c_str()),
      const_cast<char*>(path.c_str()),const_cast<char*>(out.c_str()),nullptr
  };
  gchar* stdout_text=nullptr;
  gchar* stderr_text=nullptr;
  GError* error=nullptr;
  gint code=-1;
  const gboolean ok=g_spawn_sync(nullptr,argv,nullptr,
      G_SPAWN_SEARCH_PATH,nullptr,nullptr,&stdout_text,&stderr_text,&code,
      &error);
  const std::string detail=stderr_text?stderr_text:"";
  g_free(stdout_text);g_free(stderr_text);
  if(!ok){
    const std::string message=error?error->message:"Error de ejecución";
    if(error)g_error_free(error);
    throw std::runtime_error("No se pudo importar MusicXML: "+message);
  }
  if(!g_spawn_check_wait_status(code,&error)){
    std::string message=detail.empty()?"MusicXML incompatible":detail;
    if(error)g_error_free(error);
    throw std::runtime_error(message);
  }
  if(!g_file_test(out.c_str(),G_FILE_TEST_IS_REGULAR))
    throw std::runtime_error("No se creó el archivo MIDI convertido");
  return out;
}
}
#endif
