#include "shader.h"

#include "file.h"
#include "global.h"
#include "opengl.h"

#include <filesystem>
#include <unordered_map>

struct ShaderFilePaths {
    std::string vert = "res/defaultWorld.vert";
    std::string frag = "res/defaultWorld.frag";
    std::string geom;
};

static std::unordered_map<std::string, ShaderFilePaths> existingShaders;
static std::unordered_map<std::string, GLuint> compiledShaderPool;
static GLuint currentShaderProgram = -1;

static GLuint compileShader(const ShaderFilePaths &shaderFilePaths) {
    GLint programId = 0;

    GLint vertexShader = 0;
    GLint geometryShader = 0;
    GLint fragmentShader = 0;

    { // compile the shaders
        std::string buffer;

        File::load(shaderFilePaths.vert.c_str(), buffer);
        GLchar const *vertexSource = buffer.c_str();
        vertexShader = OpenGl::glCreateShader(GL_VERTEX_SHADER);
        OpenGl::glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        OpenGl::glCompileShader(vertexShader);

        if (!shaderFilePaths.geom.empty()) {
            File::load(shaderFilePaths.geom.c_str(), buffer);
            GLchar const *geometrySource = buffer.c_str();
            geometryShader = OpenGl::glCreateShader(GL_GEOMETRY_SHADER);
            OpenGl::glShaderSource(geometryShader, 1, &geometrySource, nullptr);
            OpenGl::glCompileShader(geometryShader);
        }

        File::load(shaderFilePaths.frag.c_str(), buffer);
        GLchar const *fragSource = buffer.c_str();
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

        if (!shaderFilePaths.geom.empty()) {
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

void Shader::init() {
    auto shaderDir = std::filesystem::directory_iterator("res/");

    for (auto &p: shaderDir) {
        const std::string ext = p.path().extension().string();

        if (ext == ".vert" || ext == ".geom" || ext == ".frag") {
            const std::string relFilepath = p.path().relative_path().string();
            const std::string stem = p.path().stem().string();

            auto it = existingShaders.find(stem);
            if (it == existingShaders.end()) {
                existingShaders.insert({stem, ShaderFilePaths()});
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

    for (auto &it: existingShaders) {
        const GLuint programId = compileShader(it.second);
        const std::string stem = std::filesystem::path(it.first).stem().string();

        compiledShaderPool.insert({stem, programId});
    }
}

static GLuint getShader_(const char *name) {
    const auto it = compiledShaderPool.find(name);

    if (it != compiledShaderPool.end())
        return it->second;

    return 0;
}

static GLuint useShader_(const char *name) {
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
    OpenGl::glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                                  (void *) (3 * sizeof(GLfloat)));

    OpenGl::glUniform1i(OpenGl::glGetUniformLocation(shaderProgram, "texture0"), to_underlying(OpenGl::Type::texture0));

    mat4 modelMat;
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMat.m00);
    mat4 viewMat;

    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &viewMat.m00);
    mat4 projectionMat;
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE,
                               &projectionMat.m00);

    OpenGl::glUniform2f(OpenGl::glGetUniformLocation(shaderProgram, "resolution"), f32(Global::windowWidth),
                        f32(Global::windowHeight));

    OpenGl::glUniform1ui(OpenGl::glGetUniformLocation(shaderProgram, "renderOptions"),
                         to_underlying(Global::renderOptions));

    OpenGl::glUniform1f(OpenGl::glGetUniformLocation(shaderProgram, "time"), Global::time);

    return shaderProgram;
}

GLuint Shader::useShader(Shader::Stem shaderStem) {
    switch (shaderStem) {
        case Shader::Stem::defaultWorld:
            return useShader_("defaultWorld");
        case Shader::Stem::defaultScreen:
            return useShader_("defaultScreen");
    }

    ASSERT(false);
    return 0;
}
