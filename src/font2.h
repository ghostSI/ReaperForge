#ifndef FONT_H
#define FONT_H

#include "type.h"

namespace Font
{
  void init2();
  void draw(const char* text, f32 posX, f32 posY, f32 posZ, f32 scale);
  void drawFretNumber(i32 fretNumber, f32 posX, f32 posY, f32 posZ, f32 scale);
  void drawNoteNameFlat(i32 note, f32 posX, f32 posY, f32 posZ, f32 scale);
}

#endif // FONT_H