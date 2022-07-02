#include "shader.h"

#include "data.h"
#include "file.h"
#include "global.h"
#include "opengl.h"

static GLuint shaderPrograms[to_underlying(Shader::Stem::COUNT)];

static GLuint compileShader(const char* vertexSource, const char* fragSource)
{
  GLint vertexShader = 0;
  GLint fragmentShader = 0;

  { // compile the shaders
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragSource, nullptr);
    glCompileShader(fragmentShader);
  }

  { // handle compilation errors
    GLint status;
    char buffer[512];

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
      ASSERT(false);
    }

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
      ASSERT(false);
    }
  }

  GLuint programId = glCreateProgram();
  glAttachShader(programId, vertexShader);
  glAttachShader(programId, fragmentShader);

  //  glBindFragDataLocation(programId, 0, "outColor");

  glLinkProgram(programId);

  if (vertexShader != 0)
    glDeleteShader(vertexShader);
  if (fragmentShader != 0)
    glDeleteShader(fragmentShader);

  return programId;
}

void Shader::init()
{
  shaderPrograms[0] = compileShader(Data::Shader::defaultScreenVert, Data::Shader::defaultFrag);
  shaderPrograms[1] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::defaultFrag);
  shaderPrograms[2] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::detectedChordFrag);
  shaderPrograms[3] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::detectedFretFrag);
  shaderPrograms[4] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::detectedNoteFrag);
  shaderPrograms[5] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::dotInlayFrag);
  shaderPrograms[6] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::groundFrag);
  shaderPrograms[7] = compileShader(Data::Shader::defaultScreenVert, Data::Shader::fontFrag);
  shaderPrograms[8] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::fontFrag);
  shaderPrograms[9] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::anchorFrag);
  shaderPrograms[10] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::ebeatFrag);
  shaderPrograms[11] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::chordBoxFrag);
  shaderPrograms[12] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::chordBoxConsecutiveFrag);
  shaderPrograms[13] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::chordBoxFretMuteFrag);
  shaderPrograms[14] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::chordBoxPalmMuteFrag);
  shaderPrograms[15] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::chordBoxArpeggioFrag);
  shaderPrograms[16] = compileShader(Data::Shader::defaultScreenVert, Data::Shader::phrasesScreenFrag);
  shaderPrograms[17] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::handShapeAnchorFrag);
  shaderPrograms[18] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::fretGoldFrag);
  shaderPrograms[19] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::fretSilverFrag);
  shaderPrograms[20] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::fretBronzeFrag);
  shaderPrograms[21] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::stringFrag);
  shaderPrograms[22] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::sustainFrag);
  shaderPrograms[23] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::noteStandFrag);
  shaderPrograms[24] = compileShader(Data::Shader::defaultWorldVert, Data::Shader::noteStandZeroFrag);
  shaderPrograms[25] = compileShader(Data::Shader::uiVert, Data::Shader::uiFrag);
}

GLuint Shader::useShader(Shader::Stem shaderStem)
{
  GLuint shaderProgram = shaderPrograms[to_underlying(shaderStem)];

  glUseProgram(shaderProgram);

  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);

  GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
  glEnableVertexAttribArray(texAttrib);
  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

  glUniform1i(glGetUniformLocation(shaderProgram, "texture0"), 0);

  mat4 modelMat;
  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMat.m00);


  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &Global::cameraMat.m00);

  mat4 projectionMat;
  const f32 scale = 1.0f / tanf((Global::settings.cameraFieldOfView * 0.5f * PI) / 180.0f);
  const f32 near_ = 0.1f;
  const f32 far_ = 100.0f;
  projectionMat.m00 = scale;
  projectionMat.m11 = scale;
  projectionMat.m22 = -far_ / (far_ - near_);
  projectionMat.m32 = -far_ * near_ / (far_ - near_);
  projectionMat.m23 = -1.0f;
  projectionMat.m33 = 0.0f;

  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projectionMat.m00);

  glUniform4f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

  return shaderProgram;
}
