
#include "opengl.h"

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_opengl_glext.h>

static PFNGLCREATESHADERPROC glCreateShaderProc;
static PFNGLSHADERSOURCEPROC glShaderSourceProc;
static PFNGLCOMPILESHADERPROC glCompileShaderProc;
static PFNGLGETSHADERIVPROC glGetShaderivProc;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLogProc;
static PFNGLDELETESHADERPROC glDeleteShaderProc;
static PFNGLATTACHSHADERPROC glAttachShaderProc;
static PFNGLCREATEPROGRAMPROC glCreateProgramProc;
static PFNGLLINKPROGRAMPROC glLinkProgramProc;
static PFNGLVALIDATEPROGRAMPROC glValidateProgramProc;
static PFNGLGETPROGRAMIVPROC glGetProgramivProc;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLogProc;
static PFNGLUSEPROGRAMPROC glUseProgramProc;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArraysProc;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArrayProc;
static PFNGLGENBUFFERSPROC glGenBuffersProc;
static PFNGLBINDBUFFERPROC glBindBufferProc;
static PFNGLDETACHSHADERPROC glDetachShaderProc;
static PFNGLDELETEPROGRAMPROC glDeleteProgramProc;
static PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocationProc;
static PFNGLGETATTRIBLOCATIONPROC glGetAttribLocationProc;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArrayProc;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointerProc;
static PFNGLBUFFERDATAPROC glBufferDataProc;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocationProc;
static PFNGLUNIFORM1FPROC glUniform1fProc;
static PFNGLUNIFORM2FPROC glUniform2fProc;
static PFNGLUNIFORM3FPROC glUniform3fProc;
static PFNGLUNIFORM4FPROC glUniform4fProc;
static PFNGLUNIFORM1IPROC glUniform1iProc;
static PFNGLUNIFORM1UIPROC glUniform1uiProc;
static PFNGLGENERATEMIPMAPPROC glGenerateMipmapProc;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fvProc;
static PFNGLACTIVETEXTUREPROC glActiveTextureProc;
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffersProc;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebufferProc;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2DProc;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatusProc;
static PFNGLGENRENDERBUFFERSPROC glGenRenderbuffersProc;
static PFNGLBINDRENDERBUFFERPROC glBindRenderbufferProc;
static PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorageProc;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbufferProc;
static PFNGLDRAWBUFFERSPROC glDrawBuffersProc;
static PFNGLBINDTEXTUREEXTPROC glBindTextureProc;
static PFNGLGENTEXTURESEXTPROC glGenTexturesProc;
static PFNGLMAPBUFFERPROC glMapBufferProc;
static PFNGLDELETETEXTURESEXTPROC glDeleteTexturesProc;
static PFNGLUNMAPBUFFERPROC glUnmapBufferProc;
static PFNGLBLENDEQUATIONPROC glBlendEquationProc;
static PFNGLDELETEBUFFERSPROC glDeleteBuffersProc;

void OpenGl::init() {
    glCreateShaderProc = static_cast<PFNGLCREATESHADERPROC>(SDL_GL_GetProcAddress("glCreateShader"));
    glShaderSourceProc = static_cast<PFNGLSHADERSOURCEPROC>(SDL_GL_GetProcAddress("glShaderSource"));
    glCompileShaderProc = static_cast<PFNGLCOMPILESHADERPROC>(SDL_GL_GetProcAddress("glCompileShader"));
    glGetShaderivProc = static_cast<PFNGLGETSHADERIVPROC>(SDL_GL_GetProcAddress("glGetShaderiv"));
    glGetShaderInfoLogProc = static_cast<PFNGLGETSHADERINFOLOGPROC>(SDL_GL_GetProcAddress("glGetShaderInfoLog"));
    glDeleteShaderProc = static_cast<PFNGLDELETESHADERPROC>(SDL_GL_GetProcAddress("glDeleteShader"));
    glAttachShaderProc = static_cast<PFNGLATTACHSHADERPROC>(SDL_GL_GetProcAddress("glAttachShader"));
    glCreateProgramProc = static_cast<PFNGLCREATEPROGRAMPROC>(SDL_GL_GetProcAddress("glCreateProgram"));
    glLinkProgramProc = static_cast<PFNGLLINKPROGRAMPROC>(SDL_GL_GetProcAddress("glLinkProgram"));
    glValidateProgramProc = static_cast<PFNGLVALIDATEPROGRAMPROC>(SDL_GL_GetProcAddress("glValidateProgram"));
    glGetProgramivProc = static_cast<PFNGLGETPROGRAMIVPROC>(SDL_GL_GetProcAddress("glGetProgramiv"));
    glGetProgramInfoLogProc = static_cast<PFNGLGETPROGRAMINFOLOGPROC>(SDL_GL_GetProcAddress("glGetProgramInfoLog"));
    glUseProgramProc = static_cast<PFNGLUSEPROGRAMPROC>(SDL_GL_GetProcAddress("glUseProgram"));
    glGenVertexArraysProc = static_cast<PFNGLGENVERTEXARRAYSPROC>(SDL_GL_GetProcAddress("glGenVertexArrays"));
    glBindVertexArrayProc = static_cast<PFNGLBINDVERTEXARRAYPROC>(SDL_GL_GetProcAddress("glBindVertexArray"));
    glGenBuffersProc = static_cast<PFNGLGENBUFFERSPROC>(SDL_GL_GetProcAddress("glGenBuffers"));
    glBindBufferProc = static_cast<PFNGLBINDBUFFERPROC>(SDL_GL_GetProcAddress("glBindBuffer"));
    glDetachShaderProc = static_cast<PFNGLDETACHSHADERPROC>(SDL_GL_GetProcAddress("glDetachShader"));
    glDeleteProgramProc = static_cast<PFNGLDELETEPROGRAMPROC>(SDL_GL_GetProcAddress("glDeleteProgram"));
    glBindFragDataLocationProc = static_cast<PFNGLBINDFRAGDATALOCATIONPROC>(SDL_GL_GetProcAddress(
            "glBindFragDataLocation"));
    glGetAttribLocationProc = static_cast<PFNGLGETATTRIBLOCATIONPROC>(SDL_GL_GetProcAddress("glGetAttribLocation"));
    glEnableVertexAttribArrayProc = static_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(SDL_GL_GetProcAddress(
            "glEnableVertexAttribArray"));
    glVertexAttribPointerProc = static_cast<PFNGLVERTEXATTRIBPOINTERPROC>(SDL_GL_GetProcAddress(
            "glVertexAttribPointer"));
    glBufferDataProc = static_cast<PFNGLBUFFERDATAPROC>(SDL_GL_GetProcAddress("glBufferData"));
    glGetUniformLocationProc = static_cast<PFNGLGETUNIFORMLOCATIONPROC>(SDL_GL_GetProcAddress("glGetUniformLocation"));
    glUniform1fProc = static_cast<PFNGLUNIFORM1FPROC>(SDL_GL_GetProcAddress("glUniform1f"));
    glUniform2fProc = static_cast<PFNGLUNIFORM2FPROC>(SDL_GL_GetProcAddress("glUniform2f"));
    glUniform3fProc = static_cast<PFNGLUNIFORM3FPROC>(SDL_GL_GetProcAddress("glUniform3f"));
    glUniform4fProc = static_cast<PFNGLUNIFORM4FPROC>(SDL_GL_GetProcAddress("glUniform4f"));
    glUniform1iProc = static_cast<PFNGLUNIFORM1IPROC>(SDL_GL_GetProcAddress("glUniform1i"));
    glUniform1uiProc = static_cast<PFNGLUNIFORM1UIPROC>(SDL_GL_GetProcAddress("glUniform1ui"));
    glGenerateMipmapProc = static_cast<PFNGLGENERATEMIPMAPPROC>(SDL_GL_GetProcAddress("glGenerateMipmap"));
    glUniformMatrix4fvProc = static_cast<PFNGLUNIFORMMATRIX4FVPROC>(SDL_GL_GetProcAddress("glUniformMatrix4fv"));
    glActiveTextureProc = static_cast<PFNGLACTIVETEXTUREPROC>(SDL_GL_GetProcAddress("glActiveTexture"));
    glGenFramebuffersProc = static_cast<PFNGLGENFRAMEBUFFERSPROC>(SDL_GL_GetProcAddress("glGenFramebuffers"));
    glBindFramebufferProc = static_cast<PFNGLBINDFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glBindFramebuffer"));
    glFramebufferTexture2DProc = static_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(SDL_GL_GetProcAddress(
            "glFramebufferTexture2D"));
    glCheckFramebufferStatusProc = static_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(SDL_GL_GetProcAddress(
            "glCheckFramebufferStatus"));
    glGenRenderbuffersProc = static_cast<PFNGLGENRENDERBUFFERSPROC>(SDL_GL_GetProcAddress("glGenRenderbuffers"));
    glBindRenderbufferProc = static_cast<PFNGLBINDRENDERBUFFERPROC>(SDL_GL_GetProcAddress("glBindRenderbuffer"));
    glRenderbufferStorageProc = static_cast<PFNGLRENDERBUFFERSTORAGEPROC>(SDL_GL_GetProcAddress(
            "glRenderbufferStorage"));
    glFramebufferRenderbufferProc = static_cast<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(SDL_GL_GetProcAddress(
            "glFramebufferRenderbuffer"));
    glDrawBuffersProc = static_cast<PFNGLDRAWBUFFERSPROC>(SDL_GL_GetProcAddress("glDrawBuffers"));
    glBindTextureProc = static_cast<PFNGLBINDTEXTUREEXTPROC>(SDL_GL_GetProcAddress("glBindTexture"));
    glGenTexturesProc = static_cast<PFNGLGENTEXTURESEXTPROC>(SDL_GL_GetProcAddress("glGenTextures"));
    glMapBufferProc = static_cast<PFNGLMAPBUFFERPROC>(SDL_GL_GetProcAddress("glMapBuffer"));
    glDeleteTexturesProc = static_cast<PFNGLDELETETEXTURESEXTPROC>(SDL_GL_GetProcAddress("glDeleteTextures"));
    glUnmapBufferProc = static_cast<PFNGLUNMAPBUFFERPROC>(SDL_GL_GetProcAddress("glUnmapBuffer"));
    glBlendEquationProc = static_cast<PFNGLBLENDEQUATIONPROC>(SDL_GL_GetProcAddress("glBlendEquation"));
    glDeleteBuffersProc = static_cast<PFNGLDELETEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteBuffers"));
}

void OpenGl::glUseProgram(GLuint program) {
    glUseProgramProc(program);
}

void OpenGl::glGenVertexArrays(GLsizei n, GLuint *arrays) {
    glGenVertexArraysProc(n, arrays);
}

void OpenGl::glBindVertexArray(GLuint array) {
    glBindVertexArrayProc(array);
}

void OpenGl::glGenBuffers(GLsizei n, GLuint *buffers) {
    glGenBuffersProc(n, buffers);
}

void OpenGl::glBindBuffer(GLenum target, GLuint buffer) {
    glBindBufferProc(target, buffer);
}

void OpenGl::glDetachShader(GLuint program, GLuint shader) {
    glDetachShaderProc(program, shader);
}

void OpenGl::glDeleteProgram(GLuint program) {
    glDeleteProgramProc(program);
}

void OpenGl::glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    glBufferDataProc(target, size, data, usage);
}

GLint OpenGl::glGetUniformLocation(GLuint program, const GLchar *name) {
    return glGetUniformLocationProc(program, name);
}

void OpenGl::glUniform1f(GLint location, GLfloat v0) {
    glUniform1fProc(location, v0);
}

void OpenGl::glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    glUniform2fProc(location, v0, v1);
}

void OpenGl::glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    glUniform3fProc(location, v0, v1, v2);
}

void OpenGl::glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    glUniform4fProc(location, v0, v1, v2, v3);
}

void OpenGl::glUniform1i(GLint location, GLint v0) {
    glUniform1iProc(location, v0);
}

void OpenGl::glUniform1ui(GLint location, GLuint v0) {
    glUniform1uiProc(location, v0);
}

GLint OpenGl::glGetAttribLocation(GLuint program, const GLchar *name) {
    return glGetAttribLocationProc(program, name);
}

void OpenGl::glEnableVertexAttribArray(GLuint index) {
    glEnableVertexAttribArrayProc(index);
}

void OpenGl::glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
                                   const void *pointer) {
    glVertexAttribPointerProc(index, size, type, normalized, stride, pointer);
}

void OpenGl::glGenerateMipmap(GLenum target) {
    glGenerateMipmapProc(target);
}

GLuint OpenGl::glCreateShader(GLenum shaderType) {
    return glCreateShaderProc(shaderType);
}

void OpenGl::glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
    glShaderSourceProc(shader, count, string, length);
}

void OpenGl::glCompileShader(GLuint shader) {
    glCompileShaderProc(shader);
}

GLuint OpenGl::glCreateProgram() {
    return glCreateProgramProc();
}

void OpenGl::glAttachShader(GLuint program, GLuint shader) {
    glAttachShaderProc(program, shader);
}

void OpenGl::glBindFragDataLocation(GLuint program, GLuint colorNumber, const char *name) {
    glBindFragDataLocationProc(program, colorNumber, name);
}

void OpenGl::glLinkProgram(GLuint program) {
    glLinkProgramProc(program);
}

void OpenGl::glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
    glGetProgramivProc(program, pname, params);
}

void OpenGl::glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
    glGetShaderivProc(shader, pname, params);
}

void OpenGl::glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
    glGetShaderInfoLogProc(shader, maxLength, length, infoLog);
}

void OpenGl::glDeleteShader(GLuint shader) {
    glDeleteShaderProc(shader);
}

void OpenGl::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    glUniformMatrix4fvProc(location, count, transpose, value);
}

void OpenGl::glActiveTexture(GLenum texture) {
    glActiveTextureProc(texture);
}

void OpenGl::glGenFramebuffers(GLsizei n, GLuint *ids) {
    glGenFramebuffersProc(n, ids);
}

void OpenGl::glBindFramebuffer(GLenum target, GLuint framebuffer) {
    glBindFramebufferProc(target, framebuffer);
}

void OpenGl::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    glFramebufferTexture2DProc(target, attachment, textarget, texture, level);
}

GLenum OpenGl::glCheckFramebufferStatus(GLenum target) {
    return glCheckFramebufferStatusProc(target);
}

void OpenGl::glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    glGenRenderbuffersProc(n, renderbuffers);
}

void OpenGl::glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    glBindRenderbufferProc(target, renderbuffer);
}

void OpenGl::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    glRenderbufferStorageProc(target, internalformat, width, height);
}

void
OpenGl::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    glFramebufferRenderbufferProc(target, attachment, renderbuffertarget, renderbuffer);
}

void OpenGl::glDrawBuffers(GLsizei n, const GLenum *bufs) {
    glDrawBuffersProc(n, bufs);
}

void OpenGl::glBindTexture(GLenum target, GLuint texture) {
    glBindTextureProc(target, texture);
}

void OpenGl::glGenTextures(GLsizei n, GLuint *textures) {
    glGenTexturesProc(n, textures);
}

void *OpenGl::glMapBuffer(GLenum target, GLenum access) {
    return glMapBufferProc(target, access);
}

void OpenGl::glDeleteTextures(GLsizei n, const GLuint *textures) {
    glDeleteTexturesProc(n, textures);
}

GLboolean OpenGl::glUnmapBuffer(GLenum target) {
    return glUnmapBufferProc(target);
}

void OpenGl::glBlendEquation(GLenum mode) {
    glBlendEquationProc(mode);
}

void OpenGl::glDeleteBuffers(GLsizei n, const GLuint *buffers) {
    glDeleteBuffersProc(n, buffers);
}