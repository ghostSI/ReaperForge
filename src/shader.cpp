#include "shader.h"

#include "data.h"
#include "file.h"
#include "global.h"
#include "opengl.h"

static std::vector<GLuint> shaderPrograms;

static GLuint compileShader(const char* vertexSource, const char* fragSource)
{
  GLint vertexShader = 0;
  GLint fragmentShader = 0;

  { // compile the shaders
    vertexShader = OpenGl::glCreateShader(GL_VERTEX_SHADER);
    OpenGl::glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    OpenGl::glCompileShader(vertexShader);

    fragmentShader = OpenGl::glCreateShader(GL_FRAGMENT_SHADER);
    OpenGl::glShaderSource(fragmentShader, 1, &fragSource, nullptr);
    OpenGl::glCompileShader(fragmentShader);
  }

  { // handle compilation errors
    GLint status;
    char buffer[512];

    OpenGl::glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      OpenGl::glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
      ASSERT(false);
    }

    OpenGl::glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      OpenGl::glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
      ASSERT(false);
    }
  }

  GLuint programId = OpenGl::glCreateProgram();
  OpenGl::glAttachShader(programId, vertexShader);
  OpenGl::glAttachShader(programId, fragmentShader);

  OpenGl::glBindFragDataLocation(programId, 0, "outColor");

  OpenGl::glLinkProgram(programId);

  if (vertexShader != 0)
    OpenGl::glDeleteShader(vertexShader);
  if (fragmentShader != 0)
    OpenGl::glDeleteShader(fragmentShader);

  return programId;
}

void Shader::init()
{
  shaderPrograms.push_back(compileShader(Data::Shader::defaultScreenVert, Data::Shader::defaultWorldFrag));
  shaderPrograms.push_back(compileShader(Data::Shader::defaultWorldVert, Data::Shader::defaultWorldFrag));
}

GLuint Shader::useShader(Shader::Stem shaderStem)
{
  GLuint shaderProgram = shaderPrograms[to_underlying(shaderStem)];

  OpenGl::glUseProgram(shaderProgram);

  GLint posAttrib = OpenGl::glGetAttribLocation(shaderProgram, "position");
  OpenGl::glEnableVertexAttribArray(posAttrib);
  OpenGl::glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);

  GLint texAttrib = OpenGl::glGetAttribLocation(shaderProgram, "texcoord");
  OpenGl::glEnableVertexAttribArray(texAttrib);
  OpenGl::glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

  OpenGl::glUniform1i(OpenGl::glGetUniformLocation(shaderProgram, "texture0"), to_underlying(OpenGl::Type::texture0));

  mat4 modelMat;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMat.m00);


  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &Global::cameraMat.m00);

  mat4 projectionMat;
  const f32 angleOfView = 90;
  const f32 scale = 1 / tan(angleOfView * 0.5 * PI / 180);
  const f32 near_ = 0.1f;
  const f32 far_ = 100.0f;
  projectionMat.m00 = scale;
  projectionMat.m11 = scale;
  projectionMat.m22 = -far_ / (far_ - near_);
  projectionMat.m32 = -far_ * near_ / (far_ - near_);
  projectionMat.m23 = -1.0f;
  projectionMat.m33 = 0.0f;

  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projectionMat.m00);


  return shaderProgram;
}
