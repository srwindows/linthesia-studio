// Licencia: GNU GPL versión 2, conforme al archivo COPYING de Linthesia.
// Linthesia: historial de reproducciones, sin duplicar ni alterar los MIDI.
// Se conserva la licencia original del proyecto.
#ifndef LINTHESIA_RECENT_SONGS_H
#define LINTHESIA_RECENT_SONGS_H
#include <string>
#include <vector>
namespace RecentSongs {
struct Entry{
 std::string path;
 long long lastPlayed=0;
 unsigned int plays=0;
 int bestPoints=0;
};
enum Sort { Alphabetical=0, PlaybackOrder=1, MostPlayed=2, HighestPoints=3 };
std::string FilePath();
// Última canción seleccionada: independiente del orden de Recientes.
std::string LastSelectedPath();
void RememberLastSelected(const std::string& path);
void RecordPlay(const std::string& path);
void RecordPoints(const std::string& path,int points);
std::vector<Entry> List(int limit,int sort);
}
#endif
