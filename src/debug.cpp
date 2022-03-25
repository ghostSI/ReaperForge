#include "debug.h"

#include "opengl.h"
#include "helper.h"
#include "global.h"
#include "shader.h"
#include "zorder.h"
#include "font.h"
#include "type.h"

void Debug::tick()
{
  static Font::Handle frameTimeId;
  static Font::Handle cursorPosId;
  static Font::Handle playerNameId;

  if (Global::inputDebug.toggle)
  {
    char text[50];
    i32 y = 10;

    sprintf(text, "Framerate %.0f", 1000.0_f32 / Global::frameDelta);
    Font::Info fontInfo{
      .text = text,
      .fontHandle = frameTimeId,
      .posX = 0,
      .posY = f32(y),
      .space = Space::screenSpace,
    };
    frameTimeId = Font::print(fontInfo);

    y += 20;

    sprintf(text, "Cursor X %d Y %d", Global::inputCursorPosX, Global::inputCursorPosY);
    fontInfo.posY = f32(y);
    fontInfo.text = text;
    fontInfo.fontHandle = cursorPosId;
    cursorPosId = Font::print(fontInfo);

    y += 20;

    sprintf(text, "PlayerName %s", Global::playerName);
    fontInfo.posY = f32(y);
    fontInfo.text = text;
    fontInfo.fontHandle = playerNameId;
    playerNameId = Font::print(fontInfo);
  }
  else if (Global::inputDebug.pressed)
  {
    Font::remove(frameTimeId);
    Font::remove(cursorPosId);
    Font::remove(playerNameId);
  }
}