#ifndef TYPE_H
#define TYPE_H

#include "typedefs.h"

enum struct FullscreenMode : u8
{
  windowed,
  borderless,
  //fullscreen // not supported. vst plugin ui does not show.
};

enum struct InstrumentFlags : u16
{
  none,
  LeadGuitar = 1 << 0,
  RhythmGuitar = 1 << 1,
  BassGuitar = 1 << 2,
  Second = 1 << 5,
  Third = 1 << 6,
  //Alternative = 1 << 7,
  //Bonus = 1 << 8,
  //Vocals = 1 << 10,
  //Keyboard = 1 << 11,
}BIT_FLAGS(InstrumentFlags);

enum struct SaveMode : u8
{
  none,
  statsOnly,
  wholeManifest,
};

enum struct SongFormat : u8
{
  sng,
  xml,
};

enum struct SignalChain : i8
{
  soundBank,
  plugin,
  COUNT
};

enum struct PsarcGear
{
  none = -1,
  pedal,
  amp,
  cabinet,
  rack
};

namespace Chords
{
  enum struct Note : i32
  {
    A,
    Bb,
    B,
    C,
    Db,
    D,
    Eb,
    E,
    F,
    Gb,
    G,
    Ab
  };

  enum struct Quality : i32
  {
    Minor,
    Major,
    Suspended,
    Dominant,
    Dimished5th,
    Augmented5th
  };
}


struct vec2 {
  f32 v0 = 0.0_f32;
  f32 v1 = 0.0_f32;
};

struct vec3 {
  f32 v0 = 0.0_f32;
  f32 v1 = 0.0_f32;
  f32 v2 = 0.0_f32;
};

struct vec4 {
  f32 v0 = 0.0_f32;
  f32 v1 = 0.0_f32;
  f32 v2 = 0.0_f32;
  f32 v3 = 0.0_f32;
};

struct mat4 {
  f32 m00 = 1.0_f32;
  f32 m01 = 0.0_f32;
  f32 m02 = 0.0_f32;
  f32 m03 = 0.0_f32;
  f32 m10 = 0.0_f32;
  f32 m11 = 1.0_f32;
  f32 m12 = 0.0_f32;
  f32 m13 = 0.0_f32;
  f32 m20 = 0.0_f32;
  f32 m21 = 0.0_f32;
  f32 m22 = 1.0_f32;
  f32 m23 = 0.0_f32;
  f32 m30 = 0.0_f32;
  f32 m31 = 0.0_f32;
  f32 m32 = 0.0_f32;
  f32 m33 = 1.0_f32;
};

struct BoundingBox {
  f32 minX;
  f32 minY;
  f32 maxX;
  f32 maxY;
};

struct Rect
{
  i16 top;
  i16 left;
  i16 bottom;
  i16 right;
};

struct Tuning
{
  i32 string[6];
};

#pragma pack(push, 1)
struct KeyInput {
  bool pressed : 1 = false;
  bool pressedLastFrame : 1 = false;
  bool down : 1 = false;
  bool up : 1 = false;
  bool toggle : 1 = false;
};
#pragma pack(pop)

#endif // TYPE_H
