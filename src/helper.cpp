#include "helper.h"

#include "global.h"

#include <math.h>
#include <sstream>

Color makeColor(u8 r, u8 g, u8 b, u8 a) {
  return r << 24 | g << 16 | b << 8 | a;
}

Color makeColor(const char* hexColor) {
  return (Color)strtoul(hexColor, NULL, 16);
}

u8 colorR(Color color) {
  return color >> 24;
}

u8 colorG(Color color) {
  return color >> 16;
}

u8 colorB(Color color) {
  return color >> 8;
}

u8 colorA(Color color) {
  return color;
}

vec4 colorVec4(const Color& color) {
  const u8 r = colorR(color);
  const u8 g = colorG(color);
  const u8 b = colorB(color);
  const u8 a = colorA(color);

  return vec4{
    .v0 = r / 255.0f,
    .v1 = g / 255.0f,
    .v2 = b / 255.0f,
    .v3 = a / 255.0f
  };
}

vec4 colorVec4(const std::string& hexColor) {
  assert((hexColor.size() == sizeof("#BADA55") - 1) || (hexColor.size() == sizeof("#BA55BABE") - 1));

  if (hexColor.size() == sizeof("#B16B00B5") - 1)
    return colorVec4(makeColor((hexColor.substr(1)).c_str()));

  return colorVec4(makeColor((hexColor.substr(1) + n2hexStr(255)).c_str()));
}

std::string hexColor(vec4 color)
{
  std::string hexString = "#";

  const u8 r = color.v0 * 255.0f;
  const u8 g = color.v1 * 255.0f;
  const u8 b = color.v2 * 255.0f;
  const u8 a = color.v3 * 255.0f;

  char hex[3];
  sprintf(hex, "%02X", r);
  hexString += hex;
  sprintf(hex, "%02X", g);
  hexString += hex;
  sprintf(hex, "%02X", b);
  hexString += hex;

  if (a != 0xFF)
  {
    sprintf(hex, "%02X", a);
    hexString += hex;
  }

  return hexString;
}

Color getColor(const u8* rgbaData, i32 index) {
  return reinterpret_cast<const Color*>(rgbaData)[index];
}

void setColor(std::vector<u8>& rgbaData, i32 index, Color color) {
  reinterpret_cast<std::vector<Color>&>(rgbaData)[index] = color;
}

std::string n2hexStr(i32 value) {
  assert(value >= 0);
  assert(value <= 255);
  char hexString[4 * sizeof(int) + 1];
  sprintf(hexString, "%X", value);
  return std::string(hexString);
}

f32 x2GlScreen(f32 x)
{
  return 2.0f * x / f32(Global::settings.graphicsResolutionWidth) - 1.0_f32;
}

f32 y2GlScreen(f32 y)
{
  return -(2.0f * y / f32(Global::settings.graphicsResolutionHeight) - 1.0_f32);
}

f32 deg2rad(f32 deg) {
  return 0.0174533f * deg;
}

f32 VecMath::lengthSquared(f32 x, f32 y) {
  return x * x + y * y;
}

f32 VecMath::lengthSquared(const vec3& x) {
  return x.v0 * x.v0 + x.v1 * x.v1 + x.v2 * x.v2;
}

f32 VecMath::length(f32 x, f32 y) {
  return sqrtf(lengthSquared(x, y));
}

f32 VecMath::length(const vec3& x) {
  return sqrtf(lengthSquared(x));
}

void VecMath::rotate(f32& x, f32& y, f32 rad) {
  const f32 oldX = x;
  x = x * cosf(rad) - y * sinf(rad);
  y = oldX * sinf(rad) + y * cosf(rad);
}

void VecMath::norm(f32& x, f32& y) {
  const f32 len = length(x, y);
  x /= len;
  y /= len;
}

vec3 VecMath::norm(const vec3& x) {
  const f32 len = VecMath::length(x);

  if (len == 0.0f)
    return {};

  vec3 normX
  {
    .v0 = x.v0 / len,
    .v1 = x.v1 / len,
    .v2 = x.v2 / len
  };
  return normX;
}

vec3 VecMath::multipicate(const vec3& x, const f32 scale)
{
  return vec3{
    .v0 = x.v0 * scale,
    .v1 = x.v1 * scale,
    .v2 = x.v2 * scale
  };
}

mat4 VecMath::multipicate(const mat4& m0, const mat4& m1)
{
  mat4 res
  {
    .m00 = m0.m00 * m1.m00 + m0.m10 * m1.m01 + m0.m20 * m1.m02 + m0.m30 * m1.m03,
    .m01 = m0.m01 * m1.m00 + m0.m11 * m1.m01 + m0.m21 * m1.m02 + m0.m31 * m1.m03,
    .m02 = m0.m02 * m1.m00 + m0.m12 * m1.m01 + m0.m22 * m1.m02 + m0.m32 * m1.m03,
    .m03 = m0.m03 * m1.m00 + m0.m13 * m1.m01 + m0.m23 * m1.m02 + m0.m33 * m1.m03,
    .m10 = m0.m00 * m1.m10 + m0.m10 * m1.m11 + m0.m20 * m1.m12 + m0.m30 * m1.m13,
    .m11 = m0.m01 * m1.m10 + m0.m11 * m1.m11 + m0.m21 * m1.m12 + m0.m31 * m1.m13,
    .m12 = m0.m02 * m1.m10 + m0.m12 * m1.m11 + m0.m22 * m1.m12 + m0.m32 * m1.m13,
    .m13 = m0.m03 * m1.m10 + m0.m13 * m1.m11 + m0.m23 * m1.m12 + m0.m33 * m1.m13,
    .m20 = m0.m00 * m1.m20 + m0.m10 * m1.m21 + m0.m20 * m1.m22 + m0.m30 * m1.m23,
    .m21 = m0.m01 * m1.m20 + m0.m11 * m1.m21 + m0.m21 * m1.m22 + m0.m31 * m1.m23,
    .m22 = m0.m02 * m1.m20 + m0.m12 * m1.m21 + m0.m22 * m1.m22 + m0.m32 * m1.m23,
    .m23 = m0.m03 * m1.m20 + m0.m13 * m1.m21 + m0.m23 * m1.m22 + m0.m33 * m1.m23,
    .m30 = m0.m00 * m1.m30 + m0.m10 * m1.m31 + m0.m20 * m1.m32 + m0.m30 * m1.m33,
    .m31 = m0.m01 * m1.m30 + m0.m11 * m1.m31 + m0.m21 * m1.m32 + m0.m31 * m1.m33,
    .m32 = m0.m02 * m1.m30 + m0.m12 * m1.m31 + m0.m22 * m1.m32 + m0.m32 * m1.m33,
    .m33 = m0.m03 * m1.m30 + m0.m13 * m1.m31 + m0.m23 * m1.m32 + m0.m33 * m1.m33
  };

  return res;
}

vec3 VecMath::truncate(const vec3& x, const f32 max)
{
  if (VecMath::length(x) > max)
    return multipicate(VecMath::norm(x), max);

  return x;
}

std::vector<std::string> string::split(const std::string& str, const char delimeter) {
  std::stringstream ss(str);
  std::string item;
  std::vector<std::string> splittedStrings;
  while (std::getline(ss, item, delimeter)) {
    splittedStrings.push_back(item);
  }
  return splittedStrings;
}

bool string::endsWith(const std::string& str, const std::string& ending) {
  if (str.length() >= ending.length())
    return (0 == str.compare(str.length() - ending.length(), ending.length(), ending));
  return false;
}
