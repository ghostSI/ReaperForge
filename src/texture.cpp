#include "texture.h"

#include "file.h"

static std::vector<std::vector<u8>> texturePool;
static std::vector<Texture::Info> textureInfo;
static std::vector<u32> animationTiles;

static void initTexture(const char* filePath, bool convertRGBA = true)
{
  Texture::Info info;

  if (File::exists(filePath))
  {
    texturePool.push_back(File::loadPng(filePath, info.width, info.height, convertRGBA));
    textureInfo.push_back(info);
  }
}

void Texture::init()
{
}

Texture::Type Texture::addTexture(const char* filepath)
{
  // TODO: it is currently possible to add a texture multiple times
  initTexture(filepath);
  return Texture::Type(texturePool.size() - 1);
}

Texture::Type Texture::findTexture(const u8* addr)
{
  for (i32 i = 0; i < texturePool.size(); ++i)
  {
    if (&texturePool[i][0] == addr)
      return Texture::Type(i);
  }
  return Texture::Type::notfound;
}

const u8* Texture::texture(Texture::Type textureType)
{
  ASSERT(textureType != Texture::Type::notfound);

  return texturePool[to_underlying(textureType)].data();
}

i32 Texture::width(Texture::Type textureType)
{
  ASSERT(textureType != Texture::Type::notfound);

  return textureInfo[to_underlying(textureType)].width;
}

i32 Texture::height(Texture::Type textureType)
{
  ASSERT(textureType != Texture::Type::notfound);

  return textureInfo[to_underlying(textureType)].height;
}
