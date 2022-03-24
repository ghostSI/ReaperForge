#include "highway.h"

#include "data.h"
#include "type.h"
#include "opengl.h"
#include "shader.h"

static const f32 stringSpacing = 0.42f;

static const f32 frets[]
{
  0.0f,
  36.353f * 0.05f,
  70.665f * 0.05f,
  103.051f * 0.05f,
  133.620f * 0.05f,
  162.473f * 0.05f,
  189.707f * 0.05f,
  215.412f * 0.05f,
  239.675f * 0.05f,
  262.575f * 0.05f,
  284.191f * 0.05f,
  304.593f * 0.05f,
  323.850f * 0.05f,
  342.026f * 0.05f,
  359.182f * 0.05f,
  375.376f * 0.05f,
  390.660f * 0.05f,
  405.087f * 0.05f,
  418.703f * 0.05f,
  431.556f * 0.05f,
  443.687f * 0.05f,
  455.138f * 0.05f,
  465.945f * 0.05f,
  476.146f * 0.05f,
  485.775f * 0.05f
};

static void drawNote(GLuint shader, i32 fret, i32 string, f32 time)
{
  mat4 modelMat;
  modelMat.m30 = frets[fret] + 0.5f * (frets[fret + 1] - frets[fret]);
  modelMat.m31 = f32(string) * stringSpacing;
  modelMat.m32 = time;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::note), Data::Geometry::note, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::note) / (sizeof(float) * 5));
}

void Highway::render()
{
  GLuint shader = Shader::useShader(Shader::Stem::defaultWorld);

  // Draw Fret
  for (int i = 0; i < sizeof(frets); ++i)
  {
    mat4 modelMat;
    modelMat.m30 = frets[i];
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::fret), Data::Geometry::fret, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::fret) / (sizeof(float) * 5));
  }

  // Draw Strings
  for (int i = 0; i < 350; ++i)
  {
    mat4 modelMat;
    modelMat.m30 = 0.1f * f32(i);
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::e), Data::Geometry::String::e, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::e) / (sizeof(float) * 5));
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::B), Data::Geometry::String::B, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::B) / (sizeof(float) * 5));
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::G), Data::Geometry::String::G, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::G) / (sizeof(float) * 5));
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::D), Data::Geometry::String::D, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::D) / (sizeof(float) * 5));
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::A), Data::Geometry::String::A, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::A) / (sizeof(float) * 5));
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::E), Data::Geometry::String::E, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::E) / (sizeof(float) * 5));
  }

  for (f32 f = 0; f > -10.0f; f -= 2.0f)
    for (int y = 0; y < 6; ++y)
      for (int x = 0; x < 4; ++x)
        drawNote(shader, x, y, f);
}
