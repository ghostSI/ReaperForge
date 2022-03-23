#include "shader.h"

#include "file.h"
#include "global.h"
#include "opengl.h"

#include <filesystem>
#include <unordered_map>

struct ShaderFilePaths
{
  std::string vert = "res/defaultWorld.vert";
  std::string frag = "res/defaultWorld.frag";
  std::string geom;
};

static std::unordered_map<std::string, ShaderFilePaths> existingShaders;
static std::unordered_map<std::string, GLuint> compiledShaderPool;
static GLuint currentShaderProgram = -1;

static GLuint compileShader(const ShaderFilePaths& shaderFilePaths)
{
  GLint programId = 0;

  GLint vertexShader = 0;
  GLint geometryShader = 0;
  GLint fragmentShader = 0;

  { // compile the shaders
    std::string buffer;

    File::load(shaderFilePaths.vert.c_str(), buffer);
    GLchar const* vertexSource = buffer.c_str();
    vertexShader = OpenGl::glCreateShader(GL_VERTEX_SHADER);
    OpenGl::glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    OpenGl::glCompileShader(vertexShader);

    if (!shaderFilePaths.geom.empty())
    {
      File::load(shaderFilePaths.geom.c_str(), buffer);
      GLchar const* geometrySource = buffer.c_str();
      geometryShader = OpenGl::glCreateShader(GL_GEOMETRY_SHADER);
      OpenGl::glShaderSource(geometryShader, 1, &geometrySource, nullptr);
      OpenGl::glCompileShader(geometryShader);
    }

    File::load(shaderFilePaths.frag.c_str(), buffer);
    GLchar const* fragSource = buffer.c_str();
    fragmentShader = OpenGl::glCreateShader(GL_FRAGMENT_SHADER);
    OpenGl::glShaderSource(fragmentShader, 1, &fragSource, nullptr);
    OpenGl::glCompileShader(fragmentShader);
  }

  { // print shader compilation errors
    GLint status;
    char buffer[512];

    OpenGl::glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      OpenGl::glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
      printf("%s\n", buffer);
    }

    if (!shaderFilePaths.geom.empty())
    {
      OpenGl::glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &status);
      if (status != GL_TRUE) {
        OpenGl::glGetShaderInfoLog(geometryShader, 512, NULL, buffer);
        printf("%s\n", buffer);
      }
    }

    OpenGl::glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      OpenGl::glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
      printf("%s\n", buffer);
    }
  }

  programId = OpenGl::glCreateProgram();
  OpenGl::glAttachShader(programId, vertexShader);
  if (geometryShader != 0)
    OpenGl::glAttachShader(programId, geometryShader);
  OpenGl::glAttachShader(programId, fragmentShader);

  OpenGl::glBindFragDataLocation(programId, 0, "outColor");

  OpenGl::glLinkProgram(programId);

  if (vertexShader != 0)
    OpenGl::glDeleteShader(vertexShader);
  if (vertexShader != 0)
    OpenGl::glDeleteShader(geometryShader);
  if (vertexShader != 0)
    OpenGl::glDeleteShader(fragmentShader);

  return programId;
}

void Shader::init()
{
  auto shaderDir = std::filesystem::directory_iterator("res/");

  for (auto& p : shaderDir)
  {
    const std::string ext = p.path().extension().string();

    if (ext == ".vert" || ext == ".geom" || ext == ".frag")
    {
      const std::string relFilepath = p.path().relative_path().string();
      const std::string stem = p.path().stem().string();

      auto it = existingShaders.find(stem);
      if (it == existingShaders.end())
      {
        existingShaders.insert({ stem, ShaderFilePaths() });
      }

      it = existingShaders.find(stem);

      if (ext == ".vert")
        it->second.vert = relFilepath;
      if (ext == ".frag")
        it->second.frag = relFilepath;
      if (ext == ".geom")
        it->second.geom = relFilepath;
    }
  }

  for (auto& it : existingShaders)
  {
    const GLuint programId = compileShader(it.second);
    const std::string stem = std::filesystem::path(it.first).stem().string();

    compiledShaderPool.insert({ stem, programId });
  }
}

static GLuint getShader_(const char* name)
{
#define debugRecompile 0
#if debugRecompile
  // recompile shader every 5 sec.
  static i32 timer;
  if (i32(Global::time) != timer)
  {
    for (auto it : compiledShaderPool)
      OpenGl::glDeleteShader(it.second);
    compiledShaderPool.clear();
    Shader::init();
  }
  timer = i32(Global::time);
#endif

  const auto it = compiledShaderPool.find(name);

  if (it != compiledShaderPool.end())
    return it->second;

  return 0;
}

static GLuint useShader_(const char* name)
{
  const GLuint shaderProgram = getShader_(name);

  if (currentShaderProgram == shaderProgram)
    return 0;

  currentShaderProgram = shaderProgram;

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

  //if (Global::inputZoom != Const::zoomStetch)
  //{
  //  const f32 zoomFactor = Zoom::zoom2ZoomFactor(Global::inputZoom);
  //  const f32 aspectFixX = f32(Global::worldWidth) / f32(Global::windowWidth);
  //  const f32 aspectFixY = f32(Global::worldHeight) / f32(Global::windowHeight);

  //  viewMat.m00 = zoomFactor * aspectFixX;
  //  viewMat.m11 = zoomFactor * aspectFixY;
  //  viewMat.m03 = (-2.0f * Global::cameraMidX / Global::worldWidth + 1.0_f32) * zoomFactor * aspectFixX;
  //  viewMat.m13 = (2.0f * Global::cameraMidY / Global::worldHeight - 1.0_f32) * zoomFactor * aspectFixY;
  //}

  
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &Global::cameraMat.m00);

  mat4 projectionMat;
  const f32 angleOfView = 90;
  const f32 scale = 1 / tan(angleOfView * 0.5 * PI / 180);
  const f32 near_ = 0.1f;
  const f32 far_ = 100.0f;
  projectionMat.m00 = scale;
  projectionMat.m11 = scale;
  projectionMat.m22 = -far_ / (far_ - near_);
  projectionMat.m23 = -far_ * near_ / (far_ - near_);
  projectionMat.m32 = -1.0f;
  projectionMat.m33 = 0.0f;


  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projectionMat.m00);

  OpenGl::glUniform2f(OpenGl::glGetUniformLocation(shaderProgram, "resolution"), f32(Global::windowWidth), f32(Global::windowHeight));

  OpenGl::glUniform1ui(OpenGl::glGetUniformLocation(shaderProgram, "renderOptions"), to_underlying(Global::renderOptions));

  OpenGl::glUniform1f(OpenGl::glGetUniformLocation(shaderProgram, "time"), Global::time);


  /*OpenGl::glUniform2f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[1].position"), 0.2f, 0.2f);
  OpenGl::glUniform3f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[1].color"), 0.2f, 0.8f, 0.2f);
  OpenGl::glUniform1f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[1].radius"), 0.1f);
  OpenGl::glUniform1f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[1].falloff"), 0.6f);

  OpenGl::glUniform2f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[2].position"), 0.2f, 0.4f);
  OpenGl::glUniform3f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[2].color"), 0.2f, 0.2f, 0.8f);
  OpenGl::glUniform1f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[2].radius"), 0.1f);
  OpenGl::glUniform1f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[2].falloff"), 0.6f);

  OpenGl::glUniform2f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[3].position"), 0.4f, 0.2f);
  OpenGl::glUniform3f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[3].color"), 0.8f, 0.2f, 0.8f);
  OpenGl::glUniform1f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[3].radius"), 0.1f);
  OpenGl::glUniform1f(OpenGl::glGetUniformLocation(shaderProgram, "pointLights[3].falloff"), 0.6f);*/

  return shaderProgram;
}

GLuint Shader::useShader(Shader::Stem shaderStem)
{
  switch (shaderStem)
  {
  case Shader::Stem::defaultWorld:
    return useShader_("defaultWorld");
  case Shader::Stem::defaultScreen:
    return useShader_("defaultScreen");
  }

  ASSERT(false);
  return 0;
}
