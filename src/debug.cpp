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
    i32 y = 1.0f;

    GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
    glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

    const f32 aspectRatio = 1.0f;
    // const f32 aspectRatio = f32(Global::windowWidth) / f32(Global::windowHeight);


    {
      sprintf(text, "Framerate %.0f", 1000.0_f32 / Global::frameDelta);
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, (1.0f - 0.5f * scaleY), 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "Cursor X %d Y %d", Global::inputCursorPosX, Global::inputCursorPosY);
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, (1.0f - 0.1f * aspectRatio - 0.5f * scaleY), 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "PlayerName %s", Global::playerName);
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, (1.0f - 0.2f * aspectRatio - 0.5f * scaleY), 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "InstrumentVolume%6.2f", Global::instrumentVolume * 100.0f);
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, (1.0f - 0.3f * aspectRatio - 0.5f * scaleY), 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "Chord %s", Chords::chordDetectorName());
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, (1.0f - 0.4f * aspectRatio - 0.5f * scaleY), 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "RecCallback %llu", Global::debugAudioCallbackRecording + 0);
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, (1.0f - 0.5f * aspectRatio - 0.5f * scaleY), 0.0f, scaleX, scaleY);
    }

    {
      sprintf(text, "PlayCallback %llu", Global::debugAudioCallbackPlayback + 0);
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, (1.0f - 0.6f * aspectRatio - 0.5f * scaleY), 0.0f, scaleX, scaleY);
    }
  }
}
