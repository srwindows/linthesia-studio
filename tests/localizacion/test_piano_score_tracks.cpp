#include "PianoScoreTracks.h"
#include <cassert>
#include <iostream>
using PianoScoreTracks::Candidate;
int main(){
 std::vector<Candidate> halo={
   {0,true,false,false,false,0},
   {1,true,false,false,false,65},
   {2,true,false,true,false,801},
   {3,true,false,false,false,525},
   {4,true,false,false,false,476},
   {9,true,false,false,true,772},
   {11,true,false,false,false,814}};
 auto p=PianoScoreTracks::Choose(halo);
 assert(p.size()==1 && *p.begin()==2);
 halo[6].userPlayable=true;
 p=PianoScoreTracks::Choose(halo);
 assert(p.size()==1 && *p.begin()==11); // explicit user wins
 halo[2].userPlayable=true;
 p=PianoScoreTracks::Choose(halo);
 assert(p.size()==1 && *p.begin()==2); // selected piano preferred
 halo[2].visible=false;
 p=PianoScoreTracks::Choose(halo);
 assert(p.size()==1 && *p.begin()==11);
 halo[6].visible=false;
 p=PianoScoreTracks::Choose(halo);
 assert(p.size()==1 && *p.begin()==3);
 std::cout<<"OK: separate Halo piano from 11 accompaniment tracks, honour explicit selection, exclude drums\n";
}
