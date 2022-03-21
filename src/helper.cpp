#include "helper.h"

#include "global.h"

#include <math.h>
#include <sstream>

static i32 randomInt() {
    static const i32 lookup[] = {
#include "../res/random.txt"
    };

    static u8 index;

    return lookup[index++];
}

Color makeColor(u8 r, u8 g, u8 b, u8 a) {
    return r << 24 | g << 16 | b << 8 | a;
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

Color getColor(const u8 *rgbaData, i32 index) {
    return reinterpret_cast<const Color *>(rgbaData)[index];
}

void setColor(std::vector<u8> &rgbaData, i32 index, Color color) {
    reinterpret_cast<std::vector<Color> &>(rgbaData)[index] = color;
}

f32 x2GlScreen(f32 x)
{
  return 2.0f * x / f32(Global::windowWidth) - 1.0_f32;
}

f32 y2GlScreen(f32 y)
{
  return -(2.0f * y / f32(Global::windowHeight) - 1.0_f32);
}

f32 deg2rad(f32 deg) {
    return 0.0174533f * deg;
}

i32 rnd::mod(i32 a) {
    return randomInt() % a;
}

i32 rnd::between(i32 a, i32 b) {
    return randomInt() % (b - a + 1) + a;
}

f32 rnd::between(f32 a, f32 b) {
    return f32(randomInt()) / f32(Const::randomIntMax) * (b - a) + a;
}

bool rnd::percent(f32 p) {
    return f32(randomInt()) < (f32(Const::randomIntMax) / 100.0_f32) * p;
}

f32 VecMath::lengthSquared(f32 x, f32 y) {
    return x * x + y * y;
}

f32 VecMath::length(f32 x, f32 y) {
    return sqrtf(lengthSquared(x, y));
}

void VecMath::rotate(f32 &x, f32 &y, f32 rad) {
    const f32 oldX = x;
    x = x * cosf(rad) - y * sinf(rad);
    y = oldX * sinf(rad) + y * cosf(rad);
}

void VecMath::norm(f32 &x, f32 &y) {
    const f32 len = length(x, y);
    x /= len;
    y /= len;
}

std::vector<std::string> string::split(std::string str, char delimeter) {
    std::stringstream ss(str);
    std::string item;
    std::vector<std::string> splittedStrings;
    while (std::getline(ss, item, delimeter)) {
        splittedStrings.push_back(item);
    }
    return splittedStrings;
}

bool string::endsWith(const std::string &str, const std::string &ending) {
    if (str.length() >= ending.length())
        return (0 == str.compare(str.length() - ending.length(), ending.length(), ending));
    return false;
}
