#ifndef HELPER_H
#define HELPER_H

#include "type.h"

#include <string>
#include <vector>

Color makeColor(u8 r, u8 g, u8 b, u8 a);

Color makeColor(const char* hexColor);

u8 colorR(Color color);

u8 colorG(Color color);

u8 colorB(Color color);

u8 colorA(Color color);

vec4 colorVec4(const Color& color);

vec4 colorVec4(const std::string& hexColor);

std::string hexColor(vec4 color);

Color getColor(const u8 *rgbaData, i32 index);

void setColor(std::vector<u8> &rgbaData, i32 index, Color color);

std::string n2hexStr(i32 value);

f32 x2GlScreen(f32 x);

f32 y2GlScreen(f32 y);

f32 deg2rad(f32 deg);

namespace VecMath {
    f32 lengthSquared(f32 x, f32 y);
    f32 lengthSquared(const vec3& x);

    f32 length(f32 x, f32 y);
    f32 length(const vec3& x);

    void norm(f32 &x, f32 &y);
    vec3 norm(const vec3& x);

    void rotate(f32 &x, f32 &y, f32 rad);

    vec3 multipicate(const vec3& x, const f32 scale);
    mat4 multipicate(const mat4& m0, const mat4& m1);

    vec3 truncate(const vec3& x, const f32 max);
}

template<typename T>
const T &min_(const T &a, const T &b) {
    return (b < a) ? b : a;
}

template<typename T>
const T &max_(const T &a, const T &b) {
    return (a < b) ? b : a;
}

template<typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) // emscripten somehow can't use std::clamp
{
  return (v < lo) ? lo : (hi < v) ? hi : v;
}

constexpr f32 map(f32 x, f32 in_min, f32 in_max, f32 out_min, f32 out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

template<typename T>
bool sameSign(const T &a, const T &b) {
    return a < 0 == b < 0;
}

inline u16 u16_le(const u8* bytes)
{
  return *reinterpret_cast<const u16*>(bytes);
}

inline i16 i16_le(const u8* bytes)
{
  return *reinterpret_cast<const i16*>(bytes);
}

inline u32 u32_le(const u8* bytes)
{
  return *reinterpret_cast<const u32*>(bytes);
}

inline i32 i32_le(const u8* bytes)
{
  return *reinterpret_cast<const i32*>(bytes);
}

inline f32 f32_le(const u8* bytes)
{
  return *reinterpret_cast<const f32*>(bytes);
}

inline f64 f64_le(const u8* bytes)
{
  return *reinterpret_cast<const f64*>(bytes);
}

inline u16 u16_be(const u8* bytes)
{
  return bytes[0] << 8 | bytes[1];
}

inline u32 u32_be(const u8* bytes)
{
  return bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3];
}

inline u64 u40_be(const u8* bytes)
{
  return u64(bytes[0]) << 32 | bytes[1] << 24 | bytes[2] << 16 | bytes[3] << 8 | bytes[4];
}

namespace string {
    std::vector<std::string> split(const std::string& str, char delimeter);

    bool endsWith(const std::string &str, const std::string &ending);
}

#ifdef __EMSCRIPTEN__
#define EMSC_PATH(x) "/"#x
#else
#define EMSC_PATH(x) #x
#endif // __EMSCRIPTEN__

#endif // HELPER_H
