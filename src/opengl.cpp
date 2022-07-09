
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
//static PFNGLBINDTEXTUREEXTPROC glBindTextureProc;
//static PFNGLGENTEXTURESEXTPROC glGenTexturesProc;
static PFNGLMAPBUFFERPROC glMapBufferProc;
//static PFNGLDELETETEXTURESEXTPROC glDeleteTexturesProc;
static PFNGLUNMAPBUFFERPROC glUnmapBufferProc;
static PFNGLBLENDEQUATIONPROC glBlendEquationProc;
static PFNGLDELETEBUFFERSPROC glDeleteBuffersProc;
static PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2DProc;

void OpenGl::init() {
  glCreateShaderProc = reinterpret_cast<PFNGLCREATESHADERPROC>(SDL_GL_GetProcAddress("glCreateShader"));
  glShaderSourceProc = reinterpret_cast<PFNGLSHADERSOURCEPROC>(SDL_GL_GetProcAddress("glShaderSource"));
  glCompileShaderProc = reinterpret_cast<PFNGLCOMPILESHADERPROC>(SDL_GL_GetProcAddress("glCompileShader"));
  glGetShaderivProc = reinterpret_cast<PFNGLGETSHADERIVPROC>(SDL_GL_GetProcAddress("glGetShaderiv"));
  glGetShaderInfoLogProc = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(SDL_GL_GetProcAddress("glGetShaderInfoLog"));
  glDeleteShaderProc = reinterpret_cast<PFNGLDELETESHADERPROC>(SDL_GL_GetProcAddress("glDeleteShader"));
  glAttachShaderProc = reinterpret_cast<PFNGLATTACHSHADERPROC>(SDL_GL_GetProcAddress("glAttachShader"));
  glCreateProgramProc = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(SDL_GL_GetProcAddress("glCreateProgram"));
  glLinkProgramProc = reinterpret_cast<PFNGLLINKPROGRAMPROC>(SDL_GL_GetProcAddress("glLinkProgram"));
  glValidateProgramProc = reinterpret_cast<PFNGLVALIDATEPROGRAMPROC>(SDL_GL_GetProcAddress("glValidateProgram"));
  glGetProgramivProc = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(SDL_GL_GetProcAddress("glGetProgramiv"));
  glGetProgramInfoLogProc = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(SDL_GL_GetProcAddress("glGetProgramInfoLog"));
  glUseProgramProc = reinterpret_cast<PFNGLUSEPROGRAMPROC>(SDL_GL_GetProcAddress("glUseProgram"));
  glGenVertexArraysProc = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(SDL_GL_GetProcAddress("glGenVertexArrays"));
  glBindVertexArrayProc = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(SDL_GL_GetProcAddress("glBindVertexArray"));
  glGenBuffersProc = reinterpret_cast<PFNGLGENBUFFERSPROC>(SDL_GL_GetProcAddress("glGenBuffers"));
  glBindBufferProc = reinterpret_cast<PFNGLBINDBUFFERPROC>(SDL_GL_GetProcAddress("glBindBuffer"));
  glDetachShaderProc = reinterpret_cast<PFNGLDETACHSHADERPROC>(SDL_GL_GetProcAddress("glDetachShader"));
  glDeleteProgramProc = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(SDL_GL_GetProcAddress("glDeleteProgram"));
  glBindFragDataLocationProc = reinterpret_cast<PFNGLBINDFRAGDATALOCATIONPROC>(SDL_GL_GetProcAddress("glBindFragDataLocation"));
  glGetAttribLocationProc = reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(SDL_GL_GetProcAddress("glGetAttribLocation"));
  glEnableVertexAttribArrayProc = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(SDL_GL_GetProcAddress("glEnableVertexAttribArray"));
  glVertexAttribPointerProc = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(SDL_GL_GetProcAddress("glVertexAttribPointer"));
  glBufferDataProc = reinterpret_cast<PFNGLBUFFERDATAPROC>(SDL_GL_GetProcAddress("glBufferData"));
  glGetUniformLocationProc = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(SDL_GL_GetProcAddress("glGetUniformLocation"));
  glUniform1fProc = reinterpret_cast<PFNGLUNIFORM1FPROC>(SDL_GL_GetProcAddress("glUniform1f"));
  glUniform2fProc = reinterpret_cast<PFNGLUNIFORM2FPROC>(SDL_GL_GetProcAddress("glUniform2f"));
  glUniform3fProc = reinterpret_cast<PFNGLUNIFORM3FPROC>(SDL_GL_GetProcAddress("glUniform3f"));
  glUniform4fProc = reinterpret_cast<PFNGLUNIFORM4FPROC>(SDL_GL_GetProcAddress("glUniform4f"));
  glUniform1iProc = reinterpret_cast<PFNGLUNIFORM1IPROC>(SDL_GL_GetProcAddress("glUniform1i"));
  glUniform1uiProc = reinterpret_cast<PFNGLUNIFORM1UIPROC>(SDL_GL_GetProcAddress("glUniform1ui"));
  glGenerateMipmapProc = reinterpret_cast<PFNGLGENERATEMIPMAPPROC>(SDL_GL_GetProcAddress("glGenerateMipmap"));
  glUniformMatrix4fvProc = reinterpret_cast<PFNGLUNIFORMMATRIX4FVPROC>(SDL_GL_GetProcAddress("glUniformMatrix4fv"));
  glActiveTextureProc = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(SDL_GL_GetProcAddress("glActiveTexture"));
  glGenFramebuffersProc = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(SDL_GL_GetProcAddress("glGenFramebuffers"));
  glBindFramebufferProc = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glBindFramebuffer"));
  glFramebufferTexture2DProc = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(SDL_GL_GetProcAddress("glFramebufferTexture2D"));
  glCheckFramebufferStatusProc = reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(SDL_GL_GetProcAddress("glCheckFramebufferStatus"));
  glGenRenderbuffersProc = reinterpret_cast<PFNGLGENRENDERBUFFERSPROC>(SDL_GL_GetProcAddress("glGenRenderbuffers"));
  glBindRenderbufferProc = reinterpret_cast<PFNGLBINDRENDERBUFFERPROC>(SDL_GL_GetProcAddress("glBindRenderbuffer"));
  glRenderbufferStorageProc = reinterpret_cast<PFNGLRENDERBUFFERSTORAGEPROC>(SDL_GL_GetProcAddress("glRenderbufferStorage"));
  glFramebufferRenderbufferProc = reinterpret_cast<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(SDL_GL_GetProcAddress("glFramebufferRenderbuffer"));
  glDrawBuffersProc = reinterpret_cast<PFNGLDRAWBUFFERSPROC>(SDL_GL_GetProcAddress("glDrawBuffers"));
  //glBindTextureProc = reinterpret_cast<PFNGLBINDTEXTUREEXTPROC>(SDL_GL_GetProcAddress("glBindTexture"));
  //glGenTexturesProc = reinterpret_cast<PFNGLGENTEXTURESEXTPROC>(SDL_GL_GetProcAddress("glGenTextures"));
  glMapBufferProc = reinterpret_cast<PFNGLMAPBUFFERPROC>(SDL_GL_GetProcAddress("glMapBuffer"));
  //glDeleteTexturesProc = reinterpret_cast<PFNGLDELETETEXTURESEXTPROC>(SDL_GL_GetProcAddress("glDeleteTextures"));
  glUnmapBufferProc = reinterpret_cast<PFNGLUNMAPBUFFERPROC>(SDL_GL_GetProcAddress("glUnmapBuffer"));
  glBlendEquationProc = reinterpret_cast<PFNGLBLENDEQUATIONPROC>(SDL_GL_GetProcAddress("glBlendEquation"));
  glDeleteBuffersProc = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteBuffers"));
  glCompressedTexImage2DProc = reinterpret_cast<PFNGLCOMPRESSEDTEXIMAGE2DPROC>(SDL_GL_GetProcAddress("glCompressedTexImage2D"));
}

GLuint OpenGl::loadDDSTexture(const u8* in_dds, i32 in_size) {
  assert(memcmp(in_dds, "DDS ", 4) == 0);

  u32 height = (in_dds[12]) | (in_dds[13] << 8) | (in_dds[14] << 16) | (in_dds[15] << 24);
  u32 width = (in_dds[16]) | (in_dds[17] << 8) | (in_dds[18] << 16) | (in_dds[19] << 24);

  u32 format;

  if (in_dds[84] == 'D') {
    switch (in_dds[87]) {
    case '1': // DXT1
      format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      break;
    case '3': // DXT3
      format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
      break;
    case '5': // DXT5
      format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      break;
    case '0': // DX10
    default:
      assert(false);
    }
  }
  else {
    assert(false);
  }

  GLuint texture = 0;
  glGenTextures(1, &texture);
  assert(texture != 0);

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, in_size - 128, &in_dds[128]);

  glBindTexture(GL_TEXTURE_2D, 0);

  return texture;
}

void glUseProgram(GLuint program) {
  glUseProgramProc(program);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glGenVertexArrays(GLsizei n, GLuint* arrays) {
  glGenVertexArraysProc(n, arrays);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glBindVertexArray(GLuint array) {
  glBindVertexArrayProc(array);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glGenBuffers(GLsizei n, GLuint* buffers) {
  glGenBuffersProc(n, buffers);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glBindBuffer(GLenum target, GLuint buffer) {
  glBindBufferProc(target, buffer);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glDetachShader(GLuint program, GLuint shader) {
  glDetachShaderProc(program, shader);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glDeleteProgram(GLuint program) {
  glDeleteProgramProc(program);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
  glBufferDataProc(target, size, data, usage);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

GLint glGetUniformLocation(GLuint program, const GLchar* name) {
  return glGetUniformLocationProc(program, name);
}

void glUniform1f(GLint location, GLfloat v0) {
  glUniform1fProc(location, v0);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
  glUniform2fProc(location, v0, v1);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
  glUniform3fProc(location, v0, v1, v2);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
  glUniform4fProc(location, v0, v1, v2, v3);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glUniform1i(GLint location, GLint v0) {
  glUniform1iProc(location, v0);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glUniform1ui(GLint location, GLuint v0) {
  glUniform1uiProc(location, v0);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

GLint glGetAttribLocation(GLuint program, const GLchar* name) {
  return glGetAttribLocationProc(program, name);
}

void glEnableVertexAttribArray(GLuint index) {
  glEnableVertexAttribArrayProc(index);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
  const void* pointer) {
  glVertexAttribPointerProc(index, size, type, normalized, stride, pointer);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glGenerateMipmap(GLenum target) {
  glGenerateMipmapProc(target);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

GLuint glCreateShader(GLenum shaderType) {
  return glCreateShaderProc(shaderType);
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length) {
  glShaderSourceProc(shader, count, string, length);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glCompileShader(GLuint shader) {
  glCompileShaderProc(shader);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

GLuint glCreateProgram() {
  return glCreateProgramProc();
}

void glAttachShader(GLuint program, GLuint shader) {
  glAttachShaderProc(program, shader);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glBindFragDataLocation(GLuint program, GLuint colorNumber, const char* name) {
  glBindFragDataLocationProc(program, colorNumber, name);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glLinkProgram(GLuint program) {
  glLinkProgramProc(program);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
  glGetProgramivProc(program, pname, params);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {
  glGetShaderivProc(shader, pname, params);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) {
  glGetShaderInfoLogProc(shader, maxLength, length, infoLog);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glDeleteShader(GLuint shader) {
  glDeleteShaderProc(shader);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
  glUniformMatrix4fvProc(location, count, transpose, value);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glActiveTexture(GLenum texture) {
  glActiveTextureProc(texture);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glGenFramebuffers(GLsizei n, GLuint* ids) {
  glGenFramebuffersProc(n, ids);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
  glBindFramebufferProc(target, framebuffer);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
  glFramebufferTexture2DProc(target, attachment, textarget, texture, level);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

GLenum glCheckFramebufferStatus(GLenum target) {
  return glCheckFramebufferStatusProc(target);
}

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
  glGenRenderbuffersProc(n, renderbuffers);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
  glBindRenderbufferProc(target, renderbuffer);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
  glRenderbufferStorageProc(target, internalformat, width, height);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void
glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
  glFramebufferRenderbufferProc(target, attachment, renderbuffertarget, renderbuffer);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glDrawBuffers(GLsizei n, const GLenum* bufs) {
  glDrawBuffersProc(n, bufs);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

//void glBindTexture(GLenum target, GLuint texture) {
//  glBindTextureProc(target, texture);
//}
//
//void glGenTextures(GLsizei n, GLuint* textures) {
//  glGenTexturesProc(n, textures);
//#ifdef OPENGL_ERROR_CHECK
//  assert(glGetError() == 0);
//#endif // OPENGL_ERROR_CHECK
//}

void* glMapBuffer(GLenum target, GLenum access) {
  return glMapBufferProc(target, access);
}

//void glDeleteTextures(GLsizei n, const GLuint* textures) {
//  glDeleteTexturesProc(n, textures);
//#ifdef OPENGL_ERROR_CHECK
//  assert(glGetError() == 0);
//#endif // OPENGL_ERROR_CHECK
//}

GLboolean glUnmapBuffer(GLenum target) {
  return glUnmapBufferProc(target);
}

void glBlendEquation(GLenum mode) {
  glBlendEquationProc(mode);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
  glDeleteBuffersProc(n, buffers);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
  GLint border, GLsizei imageSize, const GLvoid* data) {
  glCompressedTexImage2DProc(target, level, internalformat, width, height, border, imageSize, data);
#ifdef OPENGL_ERROR_CHECK
  assert(glGetError() == 0);
#endif // OPENGL_ERROR_CHECK
}
