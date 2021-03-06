#include "font.h"
#include "opengl.h"
#include "data.h"
#include "global.h"
#include "shader.h"

#include <string.h>
#include <vector>

static std::vector<u32> fretNumberBitmap[25];
static std::vector<u32> noteNameFlatBitmap[12];

static std::vector<u32> createTextBitmap(const char* text, i32 letters)
{
  std::vector<u32> textBitmap;

  const u32* texture = reinterpret_cast<const u32*>(Data::Texture::font);
  const i32 rowOffset = (letters - 1) * Const::fontCharWidth;

  textBitmap.resize(i64(letters) * Const::fontCharWidth * Const::fontCharHeight);

  for (int c = 0; c < letters; ++c) {
    char letter = text[c];

    if (letter < ' ' || letter > '~')
      letter = '~' + 1;

    i32 offsetY = ((letter - ' ') / 16) * Const::fontCharHeight;
    i32 offsetX = (letter % 16) * Const::fontCharWidth;

    i32 i = Const::fontCharWidth * c;
    for (i32 y = 0; y < Const::fontCharHeight; ++y) {
      for (i32 x = 0; x < Const::fontCharWidth; ++x) {
        textBitmap[i] = texture[(y + offsetY) * Const::fontTextureWidth + (x + offsetX)];

        ++i;

        if (i % Const::fontCharWidth == 0)
          i += rowOffset;
      }
    }
  }

  return textBitmap;
}

void Font::init()
{
  for (i32 i = 0; i <= 24; ++i)
  {
    char fretNumber[3];
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    sprintf(fretNumber, "%d", i);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

    fretNumberBitmap[i] = createTextBitmap(fretNumber, i >= 10 ? 2 : 1);
  }

  for (i32 i = 0; i < 12; ++i)
  {
    noteNameFlatBitmap[i] = createTextBitmap(Const::notesFlat[i], i32(strlen(Const::notesFlat[i])));
  }
}

static void drawBitmap(const std::vector<u32>& textBitmap, i32 letters, f32 posX, f32 posY, f32 posZ, f32 scaleX, f32 scaleY)
{
  const f32 left = posX - scaleX;
  const f32 top = posY - scaleY;
  const f32 right = posX + scaleX;
  const f32 bottom = posY + scaleY;

  // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
  const GLfloat v[] = {
    left , top, posZ, 0.0f, 1.0f,
    right, top, posZ, 1.0f, 1.0f,
    left, bottom, posZ, 0.0f, 0.0f,
    right, bottom, posZ, 1.0f, 0.0f,
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Const::fontCharWidth * letters, Const::fontCharHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textBitmap.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDeleteTextures(1, &tex);

  glBindTexture(GL_TEXTURE_2D, Global::texture);
}

void Font::draw(const char* text, f32 posX, f32 posY, f32 posZ, f32 scaleX, f32 scaleY)
{
  const i32 letters = i32(strlen(text));

  const std::vector<u32> textBitmap = createTextBitmap(text, letters);

  drawBitmap(textBitmap, letters, posX, posY, posZ, scaleX, scaleY);
}

void Font::drawFretNumber(i32 fretNumber, f32 posX, f32 posY, f32 posZ, f32 scaleX, f32 scaleY)
{
  assert(fretNumber >= 0);
  assert(fretNumber < 25);

  const i32 letters = fretNumber >= 10 ? 2 : 1;

  drawBitmap(fretNumberBitmap[fretNumber], letters, posX, posY, posZ, scaleX, scaleY);
}

void Font::drawNoteNameFlat(i32 note, f32 posX, f32 posY, f32 posZ, f32 scaleX, f32 scaleY)
{
  assert(note >= 0);
  assert(note < 12);

  const i32 letters = i32(strlen(Const::notesFlat[note]));

  drawBitmap(noteNameFlatBitmap[note], letters, posX, posY, posZ, scaleX, scaleY);
}
