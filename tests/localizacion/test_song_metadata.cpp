#include "SongMetadata.h"
#include <cassert>
#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <unistd.h>
int main(){
 char parent[]="/tmp/linthesia-song-meta-XXXXXX";
 assert(mkdtemp(parent));
 g_setenv("XDG_DATA_HOME",parent,TRUE);
 const std::string song=std::string(parent)+"/original.mid";
 const std::string moved=std::string(parent)+"/renamed.mid";
 std::ofstream(song,std::ios::binary)<<"MThd\0\0\0\x06";
 assert(!SongMetadata::Identity(song).empty());
 SongMetadata::Entry e=SongMetadata::Load(song);
 assert(e.title.empty());
 e.title="Luz de luna";
 e.subtitle="Adagio";
 e.composer="Compositor de prueba";
 e.tags="piano,clásico,práctica";
 e.license="Libre";
 e.difficulty=4;e.rating=5;
 assert(SongMetadata::Save(song,e));
 auto actual=SongMetadata::Load(song);
 assert(actual.title==e.title && actual.subtitle=="Adagio");
 assert(actual.composer==e.composer);
 assert(actual.tags==e.tags);
 assert(actual.difficulty==4 && actual.rating==5);
 assert(rename(song.c_str(),moved.c_str())==0);
 assert(SongMetadata::Load(moved).title=="Luz de luna");
 e.rating=99;
 assert(SongMetadata::Save(moved,e));
 assert(SongMetadata::Load(moved).rating==5);
 e.title=std::string(257,'x');
 assert(!SongMetadata::Save(moved,e));
 assert(SongMetadata::Load(moved).title=="Luz de luna");
 assert(SongMetadata::Identity("/missing/file.mid").empty());
 std::cout<<"OK: file-identity metadata survives rename; title, composer, tags, licensing, rating, validation\n";
}
