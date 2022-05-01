#include "debug.h"

#include "chords.h"
#include "opengl.h"
#include "helper.h"
#include "global.h"
#include "shader.h"
#include "font.h"
#include "type.h"

void Debug::render()
{
  if (Global::inputDebug.toggle)
  {
    char text[50];

    GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
    glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);


    const f32 scaleY = f32(Const::fontCharHeight) / f32(Global::windowHeight);

    {
      sprintf(text, "Framerate %.0f", 1000.0_f32 / Global::frameDelta);
      const i32 letters = strlen(text);
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::windowWidth);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY, 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "Cursor X %d Y %d", Global::inputCursorPosX, Global::inputCursorPosY);
      const i32 letters = strlen(text);
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::windowWidth);
      const f32 offsetY = 2.0f * f32(Const::fontCharHeight) / f32(Global::windowHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "PlayerName %s", Global::playerName);
      const i32 letters = strlen(text);
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::windowWidth);
      const f32 offsetY = 4.0f * f32(Const::fontCharHeight) / f32(Global::windowHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "InstrumentVolume%6.2f", Global::instrumentVolume * 100.0f);
      const i32 letters = strlen(text);
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::windowWidth);
      const f32 offsetY = 6.0f * f32(Const::fontCharHeight) / f32(Global::windowHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "Chord %s", Chords::chordDetectorName());
      const i32 letters = strlen(text);
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::windowWidth);
      const f32 offsetY = 8.0f * f32(Const::fontCharHeight) / f32(Global::windowHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "Camera %.2f %.2f %.2f", Global::cameraMat.m30, Global::cameraMat.m31, Global::cameraMat.m32);
      const i32 letters = strlen(text);
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::windowWidth);
      const f32 offsetY = 10.0f * f32(Const::fontCharHeight) / f32(Global::windowHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "RecCallback %llu", Global::debugAudioCallbackRecording + 0);
      const i32 letters = strlen(text);
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::windowWidth);
      const f32 offsetY = 12.0f * f32(Const::fontCharHeight) / f32(Global::windowHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "PlayCallback %llu", Global::debugAudioCallbackPlayback + 0);
      const i32 letters = strlen(text);
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::windowWidth);
      const f32 offsetY = 14.0f * f32(Const::fontCharHeight) / f32(Global::windowHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }
  }
}
