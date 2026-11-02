#include "FullSongPlayback.h"
#include "libmidi/MidiEvent.h"
#include <cassert>
#include <iostream>
int main(){
 std::vector<Track::Properties> tracks(14);
 std::vector<bool> notes(14,true),drums(14,false);
 notes[0]=false;
 drums[9]=true;
 for(auto& t:tracks)t.mode=Track::ModeNotPlayed;
 tracks[2].mode=Track::ModePlayedAutomatically;
 const auto original=tracks;
 const auto before=FullSongPlayback::Count(tracks,notes,drums);
 assert(before.playable==1 && before.excluded==12);
 // The 12 muted tracks must stay muted until an EXPLICIT button click.
 for(size_t i=0;i<tracks.size();++i)assert(tracks[i].mode==original[i].mode);
 FullSongPlayback::EnableAll(tracks,notes,drums);
 const auto after=FullSongPlayback::Count(tracks,notes,drums);
 assert(after.playable==13 && after.excluded==0);
 assert(tracks[9].mode==Track::ModePlayedButHidden);
 assert(tracks[0].mode==Track::ModeNotPlayed);
 tracks=original;
 assert(FullSongPlayback::Count(tracks,notes,drums).excluded==12);
 assert(FullSongPlayback::ClampNoteVelocity(300)==127);
 assert(FullSongPlayback::ClampNoteVelocity(-1)==0);
 assert(FullSongPlayback::ClampNoteVelocity(76.0)==76);
 MidiEvent ev=MidiEvent::Build(MidiEventSimple(0x90,60,64));
 ev.SetVelocity(256);
 assert(ev.NoteVelocity()==127);
 ev.SetVelocity(-64);
 assert(ev.NoteVelocity()==0);
 ev.SetVelocity(96);
 assert(ev.NoteVelocity()==96);
 std::cout<<"OK Halo: piano-only stays selected, explicitly listening to all adds accompaniment, restoring choice keeps original modes and velocity\n";
}
