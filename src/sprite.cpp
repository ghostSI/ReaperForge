#include "sprite.h"

#include "global.h"
#include "opengl.h"
#include "helper.h"
#include "shader.h"

#include <unordered_map>

static std::unordered_map<const GLvoid*, GLuint> textureCache;

void Sprite::init()
{
}

static void fixRawDims(Sprite::Info& spriteInfo)
{
  if (spriteInfo.textureType != Texture::Type::notfound)
  {
    if (spriteInfo.rawTextureWidth == 0)
      spriteInfo.rawTextureWidth = Texture::width(spriteInfo.textureType);

    if (spriteInfo.rawTextureHeight == 0)
    {
      if (spriteInfo.textureType != Texture::Type::notfound)
        spriteInfo.rawTextureHeight = Texture::height(spriteInfo.textureType);
      else
        spriteInfo.rawTextureHeight = Texture::height(spriteInfo.textureType);
    }

    ASSERT(spriteInfo.rawTextureWidth != 0);
    ASSERT(spriteInfo.rawTextureHeight != 0);
  }
}

static void fixScaleDims(Sprite::Info& spriteInfo)
{
  if (spriteInfo.scaleWidth == 0.0_f32)
    spriteInfo.scaleWidth = f32(spriteInfo.rawTextureWidth);

  if (spriteInfo.scaleHeight == 0.0_f32)
    spriteInfo.scaleHeight = f32(spriteInfo.rawTextureHeight);

  ASSERT(spriteInfo.scaleWidth != 0.0_f32);
  ASSERT(spriteInfo.scaleHeight != 0.0_f32);
}

static void fixTexture(Sprite::Info& spriteInfo)
{
  if (spriteInfo.rawTexture == nullptr && spriteInfo.textureType != Texture::Type::notfound)
    spriteInfo.rawTexture = Texture::texture(spriteInfo.textureType);

  //ASSERT(.rawTexture != nullptr);
}

void Sprite::render(Sprite::Info& spriteInfo)
{
  fixRawDims(spriteInfo);
  fixScaleDims(spriteInfo);
  fixTexture(spriteInfo);
  ASSERT(spriteInfo.zOrder != ZOrder::zOrderNotSpecified);

  const f32 left = spriteInfo.posX - 0.5_f32 * spriteInfo.scaleWidth;
  const f32 top = spriteInfo.posY - 0.5_f32 * spriteInfo.scaleHeight;
  const f32 right = spriteInfo.posX + 0.5_f32 * spriteInfo.scaleWidth;
  const f32 bottom = spriteInfo.posY + 0.5_f32 * spriteInfo.scaleHeight;;

  const f32 mx = 0.5_f32 * (left + right);
  const f32 my = 0.5_f32 * (top + bottom);

  f32 pmx1 = left - mx;
  f32 pmy1 = top - my;
  f32 pmx2 = right - mx;
  f32 pmy2 = pmy1;
  f32 pmx3 = pmx1;
  f32 pmy3 = bottom - my;
  f32 pmx4 = pmx2;
  f32 pmy4 = pmy3;

  VecMath::rotate(pmx1, pmy1, spriteInfo.angle);
  VecMath::rotate(pmx2, pmy2, spriteInfo.angle);
  VecMath::rotate(pmx3, pmy3, spriteInfo.angle);
  VecMath::rotate(pmx4, pmy4, spriteInfo.angle);

  pmx1 += mx;
  pmy1 += my;
  pmx2 += mx;
  pmy2 += my;
  pmx3 += mx;
  pmy3 += my;
  pmx4 += mx;
  pmy4 += my;

  switch (spriteInfo.space)
  {
  case Space::worldSpace:
    //pmx1 = x2GlWorld(pmx1);
    //pmx2 = x2GlWorld(pmx2);
    //pmx3 = x2GlWorld(pmx3);
    //pmx4 = x2GlWorld(pmx4);
    //pmy1 = y2GlWorld(pmy1);
    //pmy2 = y2GlWorld(pmy2);
    //pmy3 = y2GlWorld(pmy3);
    //pmy4 = y2GlWorld(pmy4);
    break;
  case Space::screenSpace:
    pmx1 = x2GlScreen(pmx1);
    pmx2 = x2GlScreen(pmx2);
    pmx3 = x2GlScreen(pmx3);
    pmx4 = x2GlScreen(pmx4);
    pmy1 = y2GlScreen(pmy1);
    pmy2 = y2GlScreen(pmy2);
    pmy3 = y2GlScreen(pmy3);
    pmy4 = y2GlScreen(pmy4);
    break;
  }

  switch (spriteInfo.flip)
  {
  case Sprite::Flip::Horizontal:
    std::swap(spriteInfo.uv.top, spriteInfo.uv.bottom);
    break;
  case Sprite::Flip::Vertical:
    std::swap(spriteInfo.uv.left, spriteInfo.uv.right);
    break;
  }

  const f32 z = zOrder(spriteInfo.zOrder);

  // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
  const GLfloat v[] = {
    pmx1, pmy1, z, spriteInfo.uv.left, spriteInfo.uv.top,
    pmx2, pmy2, z, spriteInfo.uv.right, spriteInfo.uv.top,
    pmx3, pmy3, z, spriteInfo.uv.left, spriteInfo.uv.bottom,
    pmx4, pmy4, z, spriteInfo.uv.right, spriteInfo.uv.bottom,
  };

  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

  GLuint shader = Shader::useShader(spriteInfo.shaderStem);

  vec4 colorVec = colorVec4(spriteInfo.color);
  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), colorVec.v0, colorVec.v1, colorVec.v2, colorVec.v3);

  if (spriteInfo.rawTexture != nullptr)
  { // Load texture
    if (!spriteInfo.skipTextureCache)
    {
      const auto it = textureCache.find(&reinterpret_cast<const u32*>(spriteInfo.rawTexture)[spriteInfo.pixelOffset]);
      if (it == textureCache.end())
      {
        GLuint tex;
        glGenTextures(1, &tex);
        textureCache.insert({ &reinterpret_cast<const u32*>(spriteInfo.rawTexture)[spriteInfo.pixelOffset], tex });

        glBindTexture(GL_TEXTURE_2D, tex);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, spriteInfo.rawTextureWidth, spriteInfo.rawTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &reinterpret_cast<const u32*>(spriteInfo.rawTexture)[spriteInfo.pixelOffset]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
      else
      {
        glBindTexture(GL_TEXTURE_2D, it->second);
      }
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    else
    {
      GLuint tex;
      glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_2D, tex);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, spriteInfo.rawTextureWidth, spriteInfo.rawTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &reinterpret_cast<const u32*>(spriteInfo.rawTexture)[spriteInfo.pixelOffset]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      glDeleteTextures(1, &tex);
    }
  }
  else
  {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
}
