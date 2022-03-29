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
  }
  namespace Texture
  {
    extern const u8 font[82944];
  }
  namespace Geometry
  {
    extern const f32 fret[15060];
    extern const f32 note[150];
    extern const f32 noteFretboard[360];
    extern const f32 noteStand[240];
    extern const f32 palmmute[720];
    extern const f32 hammerOn[105];
    extern const f32 pullOff[75];
    extern const f32 pinchHarmonic[1590];
    extern const f32 zero[270];
    extern const f32 zeroLeft[135];
    extern const f32 zeroRight[135];
    extern const f32 ground[30];
    extern const f32 fretmute[540];
    extern const f32 harmonic[1590];

    namespace String
    {
      extern const f32 e[240];
      extern const f32 B[240];
      extern const f32 G[240];
      extern const f32 D[240];
      extern const f32 A[240];
      extern const f32 E[240];
    }
  }
}

#endif // DATA_H