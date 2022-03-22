#include "highway.h"

#include "data.h"
#include "type.h"
#include "opengl.h"
#include "shader.h"

void Highway::render()
{
  Shader::useShader(Shader::Stem::defaultScreen);

  // Draw Fret
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::fret), Data::Geometry::fret, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::fret) / (sizeof(float) * 5));

  // Draw Strings
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::e), Data::Geometry::String::e, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::e) / (sizeof(float) * 5));
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::B), Data::Geometry::String::B, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::B) / (sizeof(float) * 5));
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::G), Data::Geometry::String::G, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::G) / (sizeof(float) * 5));
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::D), Data::Geometry::String::D, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::D) / (sizeof(float) * 5));
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::String::E), Data::Geometry::String::E, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::String::E) / (sizeof(float) * 5));
}
