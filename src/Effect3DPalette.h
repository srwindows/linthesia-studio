// Linthesia GPLv2. Original color families; no SeeMusic data/assets used.
#ifndef LINTHESIA_EFFECT3D_PALETTE_H
#define LINTHESIA_EFFECT3D_PALETTE_H
#include <SDL.h>
#include <algorithm>
namespace Effect3DPalette {
constexpr int Count=9;
inline const char* Name(int n){
  constexpr const char* names[]={"Original (colores de pista)","Arcoiris",
    "Neon electrico","Pastel","Fuego","Oceano","Aurora","Atardecer",
    "Monocromo"};
  return names[std::max(0,std::min(Count-1,n))];
}
// Respect each track's user-selected hue. A global palette modifies only
// the luminosity of its cinematic highlights, never replaces the track
// colour. The full palette swatches remain available in the UI.
inline SDL_Color TrackTint(SDL_Color selected,int preset,int track);
inline SDL_Color Color(SDL_Color original,int preset,int track){
  if(preset<=0||preset>=Count)return original;
  // Eight harmonised swatches per family; a 16-track set repeats tones.
  static constexpr unsigned char rgb[8][8][3]={
    {{240,71,86},{255,112,49},{255,162,50},{255,220,57},
     {99,234,101},{55,194,249},{90,103,254},{205,73,236}},
    {{255,54,191},{143,51,255},{55,130,255},{25,235,247},
     {39,255,177},{145,255,60},{255,231,45},{255,102,56}},
    {{252,179,192},{240,192,236},{188,206,251},{176,228,251},
     {182,242,222},{228,239,190},{255,218,186},{225,206,255}},
    {{255,42,37},{255,75,18},{255,114,15},{255,159,18},
     {251,200,39},{255,106,49},{241,60,57},{255,222,125}},
    {{21,170,243},{22,213,224},{41,225,195},{65,179,242},
     {33,130,219},{96,192,255},{19,221,218},{113,236,246}},
    {{50,231,195},{108,250,110},{35,197,255},{169,95,254},
     {240,73,208},{71,255,191},{155,239,66},{95,183,255}},
    {{255,111,93},{255,162,76},{255,206,102},{252,130,157},
     {218,114,221},{182,111,255},{255,94,138},{250,185,118}},
    {{245,245,246},{211,222,238},{188,199,214},{175,187,205},
     {225,232,240},{193,204,219},{235,242,250},{205,216,228}}
  };
  const int idx=(track%8+8)%8;
  const auto& c=rgb[preset-1][idx];
  return SDL_Color{c[0],c[1],c[2],original.a};
}
inline SDL_Color TrackTint(SDL_Color selected,int preset,int track){
  if(preset<=0 || preset>=Count)return selected;
  const auto swatch=Color(selected,preset,track);
  const float luminance=(.2126f*swatch.r+.7152f*swatch.g+.0722f*swatch.b)/255.0f;
  const float gain=.97f+.06f*luminance; // max 3% luminance correction
  const auto clamp=[](float v)->Uint8{
    return static_cast<Uint8>(std::max(0.0f,std::min(255.0f,v)));
  };
  return SDL_Color{clamp(selected.r*gain),clamp(selected.g*gain),
                   clamp(selected.b*gain),selected.a};
}
} // namespace Effect3DPalette
#endif
