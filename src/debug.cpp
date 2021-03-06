#include "debug.h"

#include "chords.h"
#include "opengl.h"
#include "helper.h"
#include "global.h"
#include "shader.h"
#include "font.h"
#include "type.h"

#include <string.h>

void Debug::render()
{
  if (Global::inputDebug.toggle)
  {
    char text[50];

    GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
    glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);


    const f32 scaleY = f32(Const::fontCharHeight) / f32(Global::resolutionHeight);

    {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
      sprintf(text, "Framerate %.0f", 1000.0_f32 / Global::frameDelta);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
      const i32 letters = i32(strlen(text));
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::resolutionWidth);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY, 0.0f, scaleX, scaleY);
    }

    {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
      sprintf(text, "Cursor X %d Y %d", Global::inputCursorPosX, Global::inputCursorPosY);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
      const i32 letters = i32(strlen(text));
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::resolutionWidth);
      const f32 offsetY = 2.0f * f32(Const::fontCharHeight) / f32(Global::resolutionHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
      sprintf(text, "ProfileName %s", Global::profileName);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
      const i32 letters = i32(strlen(text));
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::resolutionWidth);
      const f32 offsetY = 4.0f * f32(Const::fontCharHeight) / f32(Global::resolutionHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
      sprintf(text, "InstrumentVolume%6.2f", Global::instrumentVolume * 100.0f);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
      const i32 letters = i32(strlen(text));
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::resolutionWidth);
      const f32 offsetY = 6.0f * f32(Const::fontCharHeight) / f32(Global::resolutionHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
      sprintf(text, "Chord %s", Chords::chordDetectorName());
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
      const i32 letters = i32(strlen(text));
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::resolutionWidth);
      const f32 offsetY = 8.0f * f32(Const::fontCharHeight) / f32(Global::resolutionHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
      sprintf(text, "Camera %.2f %.2f %.2f", Global::cameraMat.m30, Global::cameraMat.m31, Global::cameraMat.m32);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
      const i32 letters = i32(strlen(text));
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::resolutionWidth);
      const f32 offsetY = 10.0f * f32(Const::fontCharHeight) / f32(Global::resolutionHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
      sprintf(text, "RecCallback %llu", Global::debugAudioCallbackRecording + 0);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
      const i32 letters = i32(strlen(text));
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::resolutionWidth);
      const f32 offsetY = 12.0f * f32(Const::fontCharHeight) / f32(Global::resolutionHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }

    {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
      sprintf(text, "PlayCallback %llu", Global::debugAudioCallbackPlayback + 0);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
      const i32 letters = i32(strlen(text));
      const f32 scaleX = f32(Const::fontCharWidth * letters) / f32(Global::resolutionWidth);
      const f32 offsetY = 14.0f * f32(Const::fontCharHeight) / f32(Global::resolutionHeight);
      Font::draw(text, scaleX - 1.00f, 1.00f - scaleY - offsetY, 0.0f, scaleX, scaleY);
    }
  }
}
