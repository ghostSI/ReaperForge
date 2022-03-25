#ifndef DATA_H
#define DATA_H

#include "typedefs.h"

namespace Data {
  namespace Shader
  {
    extern const char* defaultScreenVert;
    extern const char* defaultWorldVert;
    extern const char* defaultWorldFrag;
  }
  namespace Texture
  {
    extern const u8 font[82944];
  }
  namespace Geometry
  {
    extern const f32 fret[15060];
    extern const f32 note[180];
    extern const f32 noteFretboard[480];
    extern const f32 noteStand[240];
    extern const f32 ground[30];

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