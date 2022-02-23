#ifndef SPRITE_H
#define SPRITE_H

#include "texture.h"
#include "zorder.h"
#include "shader.h"
#include "type.h"

namespace Sprite
{
enum struct Flip
{
  none,
  Vertical,
  Horizontal
};

struct Uv
{
  f32 left = 0.0_f32;
  f32 right = 1.0_f32;
  f32 top = 0.0_f32;
  f32 bottom = 1.0_f32;
};

struct Info
{
  Texture::Type textureType = Texture::Type::notfound;
  Shader::Stem shaderStem = Shader::Stem::defaultWorld;
  ZOrder zOrder = ZOrder::zOrderNotSpecified;
  f32 posX = 0.0_f32;
  f32 posY = 0.0_f32;
  f32 angle = 0.0_f32;
  Sprite::Flip flip = Flip::none;
  Sprite::Uv uv;
  f32 scaleWidth = 0.0_f32;
  f32 scaleHeight = 0.0_f32;
  Space space = Space::worldSpace;
  i32 pixelOffset = 0;
  bool skipTextureCache = false;

  // either use textureType or these 3
  const u8* rawTexture = nullptr;
  i32 rawTextureWidth = 0;
  i32 rawTextureHeight = 0;
};

void init();
void render(Info& spriteInfo);
}

#endif // SPRITE_H
