#include "debug.h"

#include "opengl.h"
#include "helper.h"
#include "global.h"
#include "shader.h"
#include "zorder.h"
#include "font2.h"
#include "type.h"

void Debug::render()
{
  if (Global::inputDebug.toggle)
  {
    char text[50];
    i32 y = 1.0f;

    GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
    OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

    const f32 aspectRatio = f32(Global::windowWidth) / f32(Global::windowHeight);


    sprintf(text, "Framerate %.0f", 1000.0_f32 / Global::frameDelta);
    {
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) * aspectRatio / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, 1.0f - 0.5f * scaleY, 0.0f, scaleX, scaleY);
    }

    sprintf(text, "Cursor X %d Y %d", Global::inputCursorPosX, Global::inputCursorPosY);
    {
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) * aspectRatio / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, 0.9f - 0.5f * scaleY, 0.0f, scaleX, scaleY);
    }

    sprintf(text, "PlayerName %s", Global::playerName);
    {
      const i32 letters = strlen(text);
      const f32 scaleX = (1000.0f / Const::fontCharWidth) * letters / Global::windowWidth;
      const f32 scaleY = (1000.0f / Const::fontCharHeight) * aspectRatio / Global::windowHeight;
      Font::draw(text, -1.00f + 0.5f * scaleX, 0.8f - 0.5f * scaleY, 0.0f, scaleX, scaleY);
    }
  }
}
