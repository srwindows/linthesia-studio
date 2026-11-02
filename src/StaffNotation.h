// SPDX-License-Identifier: GPL-2.0-or-later
// Original Linthesia grand-staff pitch spelling and rhythmic presentation.
// Render-only: song timing, MIDI notes and evaluated durations stay unchanged.
#ifndef LINTHESIA_STAFF_NOTATION_H
#define LINTHESIA_STAFF_NOTATION_H
#include "HandPractice.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace StaffNotation {
enum class NoteValue { Whole, Half, Quarter, Eighth, Sixteenth };
struct Spelling {
  int octave=4;
  int letter=0;      // 0=C,1=D,2=E,3=F,4=G,5=A,6=B
  int alteration=0;  // -1 flat, 0 natural, +1 sharp
  char symbol=0;     // 'b', '#', 'n' or none (already in key signature)
  int offset=0;      // diatonic distance from staff bottom line
};
inline int SignatureAlteration(int letter,int fifths){
  static const int sharps[7]={3,0,4,1,5,2,6}; // F C G D A E B
  static const int flats[7]={6,2,5,1,4,0,3};  // B E A D G C F
  fifths=std::max(-7,std::min(7,fifths));
  if(fifths>0)
    for(int i=0;i<fifths;++i)if(letter==sharps[i])return 1;
  if(fifths<0)
    for(int i=0;i< -fifths;++i)if(letter==flats[i])return -1;
  return 0;
}
inline Spelling Spell(int midi,HandPractice::Hand hand,int fifths=0){
  static const int natural[7]={0,2,4,5,7,9,11};
  if(midi<0||midi>127)return {};
  const int o=midi/12-1;
  float best=10000.f;
  Spelling result{};
  for(int octave=o-1;octave<=o+1;++octave){
    for(int letter=0;letter<7;++letter){
      const int offset=midi-(12*(octave+1)+natural[letter]);
      if(offset < -1 || offset > 1)continue;
      const int inKey=SignatureAlteration(letter,fifths);
      float score=(offset==inKey?0.f:10.f);
      // In C major prefer C# over Db; in flat keys prefer Db over C#.
      if(offset>0 && fifths<0)score+=2.f;
      if(offset<0 && fifths>=0)score+=2.f;
      // Avoid E#, B#, Cb and Fb unless explicitly part of the key.
      if((letter==2||letter==6)&&offset>0&&inKey!=1)score+=4.f;
      if((letter==0||letter==3)&&offset<0&&inKey!=-1)score+=4.f;
      // Naturals are the preferred spelling for white keys unless
      // doing so conflicts with the current key signature.
      if(offset!=0 && midi%12==natural[letter])score+=1.f;
      if(score<best){
        best=score;
        result.letter=letter;result.octave=octave;
        result.alteration=offset;
        result.symbol=offset==inKey?0:(offset==0?'n':(offset>0?'#':'b'));
        const int step=(octave+1)*7+letter;
        const int baseline=hand==HandPractice::Right?
           (4+1)*7+2 : (2+1)*7+4; // E4 treble; G2 bass
        result.offset=step-baseline;
      }
    }
  }
  return result;
}
inline NoteValue ValueFromDuration(std::int64_t durationUs,int bpm){
  if(bpm<=0)return NoteValue::Quarter;
  const double beats=std::max<std::int64_t>(0,durationUs)*
                     double(bpm)/60000000.0;
  if(beats>=3.55)return NoteValue::Whole;
  if(beats>=1.7)return NoteValue::Half;
  if(beats>=.78)return NoteValue::Quarter;
  if(beats>=.38)return NoteValue::Eighth;
  return NoteValue::Sixteenth;
}
struct Rhythm {
  NoteValue value=NoteValue::Quarter;
  int dots=0; // Supports one or two dots; notation only
  int tuplet=0; // n:2 or n:4 irregular notation
};
inline Rhythm RhythmFromDuration(std::int64_t microseconds,int bpm){
  if(microseconds<=0||bpm<=0)return {};
  const double beats=double(microseconds)*bpm/60000000.0;
  const NoteValue values[]={NoteValue::Whole,NoteValue::Half,
    NoteValue::Quarter,NoteValue::Eighth,NoteValue::Sixteenth};
  const double lengths[]={4.,2.,1.,.5,.25};
  double best=1.e100;
  Rhythm result;
  for(int i=0;i<5;++i){
    for(int dots=0;dots<=2;++dots){
      const double duration=lengths[i]*
                       (dots==0?1.:(dots==1?1.5:1.75));
      for(int tuplet:{0,3,5,7}){
        const double factor=tuplet==0?1.:(tuplet==3?2./3.:
                                     (tuplet==5?4./5.:4./7.));
        const double error=std::abs(beats-duration*factor);
        const double score=error+0.008*dots+(tuplet?0.012:0.);
        if(score<best){
          best=score;result.value=values[i];result.dots=dots;
          result.tuplet=tuplet;
        }
      }
    }
  }
  return result;
}
inline bool Hollow(NoteValue n){
 return n==NoteValue::Whole||n==NoteValue::Half;
}
inline int Flags(NoteValue n){
 return n==NoteValue::Eighth?1:(n==NoteValue::Sixteenth?2:0);
}
inline bool Stem(NoteValue n){return n!=NoteValue::Whole;}
// Only visible ledger lines within the staff, not unrelated decor.
inline int LedgerCount(int offset){
 if(offset>8)return std::max(0,(offset-8)/2);
 if(offset<0)return std::max(0,(-offset)/2);
 return 0;
}
}
#endif
