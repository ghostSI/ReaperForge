#include "font2.h"
#include "texture.h"
#include "opengl.h"
#include "shader.h"
#include "zorder.h"

#include <vector>

static const i32 charWidth = 12;
static const i32 charHeight = 18;

static std::vector<u32> createTextBitmap(const char* text, u64 letters)
{
  std::vector<u32> textBitmap;

  const u32* texture = reinterpret_cast<const u32*>(Texture::texture(Texture::Type::font));
  const i32 textureWidth_ = Texture::width(Texture::Type::font);
  const i32 rowOffset = (letters - 1) * charWidth;

  textBitmap.resize(i64(letters) * charWidth * charHeight);

  for (int c = 0; c < letters; ++c) {
    char letter = text[c];

    if (letter < ' ' || letter > '~')
      letter = '~' + 1;

    i32 offsetY = ((letter - ' ') / 16) * charHeight;
    i32 offsetX = (letter % 16) * charWidth;

    i32 i = charWidth * c;
    for (i32 y = 0; y < charHeight; ++y) {
      for (i32 x = 0; x < charWidth; ++x) {
        textBitmap[i] = texture[(y + offsetY) * textureWidth_ + (x + offsetX)];

        ++i;

        if (i % charWidth == 0)
          i += rowOffset;
      }
    }
  }

  return textBitmap;
}

void Font::draw(const char* text, f32 posX, f32 posY, f32 posZ, f32 scale)
{
  const u64 letters = strlen(text);

  const std::vector<u32> textBitmap = createTextBitmap(text, letters);

  const f32 left = posX - 0.5_f32 * scale;
  const f32 top = posY - 0.5_f32 * scale;
  const f32 right = posX + 0.5_f32 * scale;
  const f32 bottom = posY + 0.5_f32 * scale;

  // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
  const GLfloat v[] = {
    left , top, posZ, 0.0f, 1.0f,
    right, top, posZ, 1.0f, 1.0f,
    left, bottom, posZ, 0.0f, 0.0f,
    right, bottom, posZ, 1.0f, 0.0f,
  };


  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, charWidth * letters, charHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textBitmap.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDeleteTextures(1, &tex);
}
