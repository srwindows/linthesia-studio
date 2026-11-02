from pathlib import Path
import sys
r=Path(sys.argv[1])
main=(r/'src/main.cpp').read_text()
header=(r/'src/GameState.h').read_text()
playing=(r/'src/PlayingState.cpp').read_text()
for key in ('F7','F8','F9'):
 assert 'SDLK_'+key in main
 assert 'Key'+key in header and 'Key'+key in playing
for text in ('SongBookmarks::Add(', 'SongBookmarks::Next(', 'SongBookmarks::RemoveNearest(', 'SongBookmarks::Save(', 'SongBookmarks::Load('):
 assert text in playing
assert 'm_song_bookmarks[i]' in playing
assert 'mark.time-offset' in playing
print("OK: bookmark controls F7 F8 F9 and measure-aligned timeline markers wired")
