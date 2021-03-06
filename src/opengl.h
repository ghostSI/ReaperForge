#ifndef OPENGL_H
#define OPENGL_H

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

#include <GL/gl.h>

#ifdef _WIN32
#include <SDL2/SDL_opengl_glext.h>
#endif // _WIN32

#include "typedefs.h"

namespace OpenGl {
  void init();
  GLuint loadDDSTexture(const u8* in_dds, i32 in_size);
}

void glUseProgram(GLuint program);
void glGenVertexArrays(GLsizei n, GLuint* arrays);
void glBindVertexArray(GLuint array);
void glGenBuffers(GLsizei n, GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glDetachShader(GLuint program, GLuint shader);
void glDeleteProgram(GLuint program);
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
GLint glGetUniformLocation(GLuint program, const GLchar* name);
void glUniform1f(GLint location, GLfloat v0);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glUniform1i(GLint location, GLint v0);
void glUniform1ui(GLint location, GLuint v0);
GLint glGetAttribLocation(GLuint program, const GLchar* name);
void glEnableVertexAttribArray(GLuint index);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
void glGenerateMipmap(GLenum target);
GLuint glCreateShader(GLenum shaderType);
void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
void glCompileShader(GLuint shader);
GLuint glCreateProgram();
void glAttachShader(GLuint program, GLuint shader);
void glBindFragDataLocation(GLuint program, GLuint colorNumber, const char* name);
void glLinkProgram(GLuint program);
void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void glDeleteShader(GLuint shader);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glActiveTexture(GLenum texture);
void glGenFramebuffers(GLsizei n, GLuint* ids);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLenum glCheckFramebufferStatus(GLenum target);
void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void glDrawBuffers(GLsizei n, const GLenum* bufs);
//void glBindTexture(GLenum target, GLuint texture);
//void glGenTextures(GLsizei n, GLuint* textures);
void* glMapBuffer(GLenum target, GLenum access);
//void glDeleteTextures(GLsizei n, const GLuint* textures);
GLboolean glUnmapBuffer(GLenum target);
void glBlendEquation(GLenum mode);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);

#endif // OPENGL_H
