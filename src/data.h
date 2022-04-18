#ifndef DATA_H
#define DATA_H

#include "typedefs.h"

namespace Data {
  namespace Shader
  {
    extern const char* defaultScreenVert;
    extern const char* defaultWorldVert;
    extern const char* defaultWorldFrag;
    extern const char* groundFrag;
    extern const char* fontFrag;
    extern const char* anchorWorldFrag;
    extern const char* chordBoxWorldFrag;
    extern const char* phrasesScreenFrag;
    extern const char* arpeggioBoxWorldFrag;
    extern const char* handShapeAnchorWorldFrag;
  }
  namespace Texture
  {
    extern const u8 font[82944];
    extern const u8 test[384];
    extern const u8 texture[1048704];
  }
  namespace Geometry
  {
    extern const f32 fret[15060];
    extern const f32 ground[30];
    extern const f32 string[180];


    extern const f32 arpeggio[180];
    extern const f32 arpeggioZeroLeft[120];
    extern const f32 arpeggioZeroRight[120];

    extern const f32 fretMute[540];
    extern const f32 hammerOn[105];
    extern const f32 harmonic[8610];
    extern const f32 harmonicPinch[8610];
    extern const f32 note[150];
    extern const f32 noteFretboard[2460];
    extern const f32 noteStand[180];
    extern const f32 palmMute[690];
    extern const f32 pop[1860];
    extern const f32 pullOff[75];
    extern const f32 slap[1860];
    extern const f32 tap[330];
    extern const f32 zeroLeft[135];
    extern const f32 zeroMiddle[270];
    extern const f32 zeroRight[135];

  }
}

#endif // DATA_H