// SPDX-License-Identifier: GPL-2.0-or-later
#include "StaffNotation.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace StaffNotation;
int main(){
 using HandPractice::Right;
 using HandPractice::Left;
 auto c4=Spell(60,Right,0);
 assert(c4.letter==0 && c4.octave==4 && c4.offset==-2 && c4.symbol==0);
 auto e4=Spell(64,Right,0);
 assert(e4.letter==2 && e4.octave==4 && e4.offset==0);
 auto g2=Spell(43,Left,0);
 assert(g2.letter==4 && g2.octave==2 && g2.offset==0);
 auto cs=Spell(61,Right,0);
 assert(cs.letter==0 && cs.alteration==1 && cs.symbol=='#');
 auto db=Spell(61,Right,-5); // D-flat key signature
 assert(db.letter==1 && db.alteration==-1 && db.symbol==0);
 auto fs=Spell(66,Right,2); // F# in D major: belongs to key
 assert(fs.letter==3 && fs.symbol==0);
 auto fNatural=Spell(65,Right,1); // F natural in G major: cancel sharp
 assert(fNatural.letter==3 && fNatural.symbol=='n');
 auto bb=Spell(70,Right,-1); // Bb in F major, already in key
 assert(bb.letter==6 && bb.alteration==-1 && bb.symbol==0);
 auto bNatural=Spell(71,Right,-1);
 assert(bNatural.letter==6 && bNatural.alteration==0 && bNatural.symbol=='n');
 assert(SignatureAlteration(3,2)==1);
 assert(SignatureAlteration(6,-1)==-1);
 assert(SignatureAlteration(6,0)==0);
 assert(LedgerCount(10)==1 && LedgerCount(12)==2 && LedgerCount(-2)==1);
 assert(ValueFromDuration(2000000,120)==NoteValue::Whole);
 assert(ValueFromDuration(1000000,120)==NoteValue::Half);
 assert(ValueFromDuration(500000,120)==NoteValue::Quarter);
 assert(ValueFromDuration(250000,120)==NoteValue::Eighth);
 assert(ValueFromDuration(125000,120)==NoteValue::Sixteenth);
 assert(!Stem(NoteValue::Whole));
 assert(Hollow(NoteValue::Half));
 assert(Flags(NoteValue::Eighth)==1);
 assert(Flags(NoteValue::Sixteenth)==2);
 assert(RhythmFromDuration(1500000,120).value==NoteValue::Half);
 assert(RhythmFromDuration(1500000,120).dots==1);
 assert(RhythmFromDuration(750000,120).value==NoteValue::Quarter);
 assert(RhythmFromDuration(750000,120).dots==1);
 assert(RhythmFromDuration(375000,120).value==NoteValue::Eighth);
 assert(RhythmFromDuration(375000,120).dots==1);
 assert(RhythmFromDuration(875000,120).dots==2);
 assert(RhythmFromDuration(125000,120).dots==0);
 assert(RhythmFromDuration(166667,120).tuplet==3);
 assert(RhythmFromDuration(166667,120).value==NoteValue::Eighth);
 assert(RhythmFromDuration(200000,120).tuplet==5);
 std::cout<<"OK: key signatures, accidentals, treble/bass ledger position, musical duration and flags\n";
}
