#include "highway.h"

#include "data.h"
#include "type.h"
#include "opengl.h"
#include "shader.h"

void Highway::render()
{
  // Draw Fret
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::fretGeometry), Data::fretGeometry, GL_STATIC_DRAW);
  Shader::useShader(Shader::Stem::defaultScreen);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::fretGeometry) / (sizeof(float) * 5));
}
