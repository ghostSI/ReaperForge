#include "highway.h"

#include "data.h"
#include "type.h"
#include "opengl.h"
#include "shader.h"

static f32 frets_25_5[]
{
  0.0f,
  36.353f,
  70.665f,
  103.051f,
  133.620f,
  162.473f,
  189.707f,
  215.412f,
  239.675f,
  262.575f,
  284.191f,
  304.593f,
  323.850f,
  342.026f,
  359.182f,
  375.376f,
  390.660f,
  405.087f,
  418.703f,
  431.556f,
  443.687f,
  455.138f,
  465.945f,
  476.146f,
  485.775f
};

void Highway::render()
{
  GLuint shader = Shader::useShader(Shader::Stem::defaultWorld);

  // Draw Fret
  for (int i = 0; i < sizeof(frets_25_5); ++i)
  {
    mat4 modelMat;
    modelMat.m03 = frets_25_5[i] * 0.075f;
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::fret), Data::Geometry::fret, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::fret) / (sizeof(float) * 5));
  }

  // Draw Strings
  for (int i = 0; i < 350; ++i)
  {
    mat4 modelMat;
    modelMat.m03 = 0.1f * f32(i);
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
}
