#ifndef TYPE_H
#define TYPE_H

#include "typedefs.h"

enum struct RenderOptions : u32 {
    blockLit = 1_u32 << 0,
    actorLit = 1_u32 << 1,
    lit = blockLit | actorLit,
    postprocessing = 1_u32 << 2,
    generateDecor = 1_u32 << 5,
    nightVision = 1_u32 << 8,
}BIT_FLAGS(RenderOptions);

enum struct Space {
    worldSpace,
    screenSpace,
};

enum struct InputDebugMode : u16 {
    none = 0,
    f1 = 1_u16 << 1, // wireframe
    f2 = 1_u16 << 2, // box2D
    f3 = 1_u16 << 3, // show stats in top left corner
    f4 = 1_u16 << 4, // unused
    f5 = 1_u16 << 5, // overlay blurred lightMap
    f6 = 1_u16 << 6, // show lightMap
    f7 = 1_u16 << 7, // show LightMap
    f8 = 1_u16 << 8, // show decorMap
    f9 = 1_u16 << 9, // show LiquidMap
    f10 = 1_u16 << 10, // show blockColorMap
    f11 = 1_u16 << 11, // unused
    f12 = 1_u16 << 12, // unused
}BIT_FLAGS(InputDebugMode);

enum struct DisplayMode : u8 {
    windowed,
    fullscreen,
    borderless,
    COUNT,
};

enum struct Instrument {
    none,
    LeadGuitar = 1 << 0,
    RhythmGuitar = 1 << 1,
    BassGuitar = 1 << 2,
    All = LeadGuitar | RhythmGuitar | BassGuitar
}BIT_FLAGS(Instrument);

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

struct Rect {
    i32 left;
    i32 right;
    i32 top;
    i32 bottom;
};

#pragma pack(push, 1)
struct KeyInput {
    bool pressed: 1 = false;
    bool pressedLastFrame: 1 = false;
    bool down: 1 = false;
    bool up: 1 = false;
    bool toggle: 1 = false;
};
#pragma pack(pop)

#endif // TYPE_H
