#ifndef HELPER_H
#define HELPER_H

#include "typedefs.h"

#include <string>
#include <vector>

Color makeColor(u8 r, u8 g, u8 b, u8 a);
u8 colorR(Color color);
u8 colorG(Color color);
u8 colorB(Color color);
u8 colorA(Color color);
Color getColor(const u8* rgbaData, i32 index);
void setColor(std::vector<u8>& rgbaData, i32 index, Color color);

f32 deg2rad(f32 deg);

namespace rnd
{
i32 mod(i32 a);
i32 between(i32 a, i32 b);
f32 between(f32 a, f32 b);
bool percent(f32 percentage);
}

namespace VecMath
{
f32 lengthSquared(f32 x, f32 y);
f32 length(f32 x, f32 y);
void norm(f32& x, f32& y);
void rotate(f32& x, f32& y, f32 rad);
}

template<typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) // emscripten somehow can't use std::clamp
{
  return (v < lo) ? lo : (hi < v) ? hi : v;
}

template<typename T>
const T& min_(const T& a, const T& b)
{
  return (b < a) ? b : a;
}

template<typename T>
const T& max_(const T& a, const T& b)
{
  return (a < b) ? b : a;
}

template<typename T>
bool sameSign(const T& a, const T& b)
{
  return a < 0 == b < 0;
}

namespace string
{
std::vector<std::string> split(std::string str, char delimeter);
bool endsWith(const std::string& str, const std::string& ending);
}

#endif // HELPER_H
