#include "ChordDetector.h"
#include <cassert>
#include <iostream>
#include <vector>
using ChordDetector::Detect;
int main(){
 assert(Detect({60,64,67})=="C");
 assert(Detect({64,67,72})=="C/E");
 assert(Detect({60,63,67})=="Cm");
 assert(Detect({55,59,62,65})=="G7");
 assert(Detect({57,60,64,67})=="Am7");
 assert(Detect({60,65,67})=="Csus4");
 assert(Detect({60,62,64,67})=="Cadd9");
 assert(Detect({60,64})=="—");
 assert(Detect({60,61,62,63})=="—");
 assert(Detect({})=="—");
 std::cout<<"OK: acorde mayor, menor, inversión, séptima, sus4, add9 y silencio\n";
}
