// Syphax-Engine - Ougi Washi

#ifndef SE_GL_H
#define SE_GL_H

#if defined(SE_RENDER_BACKEND_GLES)
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>
#include <stddef.h>
#include "syphax/s_types.h"

typedef void(APIENTRY *PFNGLDELETEBUFFERS)(GLsizei n, const GLuint *buffers);
typedef void(APIENTRY *PFNGLGENBUFFERS)(GLsizei n, GLuint *buffers);
typedef void(APIENTRY *PFNGLBINDBUFFER)(GLenum target, GLuint buffer);
typedef void(APIENTRY *PFNGLBUFFERSUBDATA)(GLenum target, GLintptr offset,
											 GLsizeiptr size, const void *data);
typedef void(APIENTRY *PFNGLBUFFERDATA)(GLenum target, GLsizeiptr size,
										const void *data, GLenum usage);
typedef void(APIENTRY *PFNGLUSEPROGRAM)(GLuint program);
typedef GLuint(APIENTRY *PFNGLCREATESHADER)(GLenum type);
typedef void(APIENTRY *PFNGLSHADERSOURCE)(GLuint shader, GLsizei count,
											const GLchar **string,
											const GLint *length);
typedef void(APIENTRY *PFNGLCOMPILESHADER)(GLuint shader);
typedef GLuint(APIENTRY *PFNGLCREATEPROGRAM)(void);
typedef void(APIENTRY *PFNGLLINKPROGRAM)(GLuint program);
typedef void(APIENTRY *PFNGLATTACHSHADER)(GLuint program, GLuint shader);
typedef void(APIENTRY *PFNGLDELETEPROGRAM)(GLuint program);
typedef void(APIENTRY *PFNGLDELETESHADER)(GLuint shader);
typedef void(APIENTRY *PFNGLGENRENDERBUFFERS)(GLsizei n, GLuint *renderbuffers);
typedef void(APIENTRY *PFNGLBINDFRAMEBUFFER)(GLenum target, GLuint framebuffer);
typedef void(APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFER)(GLenum target,
													 GLenum attachment,
													 GLenum renderbuffertarget,
													 GLuint renderbuffer);
typedef void(APIENTRY *PFNGLFRAMEBUFFERTEXTURE)(GLenum target,
												GLenum attachment,
												GLuint texture, GLint level);
typedef void(APIENTRY *PFNGLBINDTEXTURE)(GLenum target, GLuint texture);
typedef void(APIENTRY *PFNGLDELETETEXTURES)(GLsizei n, const GLuint *textures);
typedef void(APIENTRY *PFNGLGENTEXTURES)(GLsizei n, GLuint *textures);
typedef void(APIENTRY *PFNGLTEXIMAGE2D)(GLenum target, GLint level,
										GLint internalFormat, GLsizei width,
										GLsizei height, GLint border,
										GLenum format, GLenum type,
										const void *pixels);
typedef void(APIENTRY *PFNGLTEXPARAMETERI)(GLenum target, GLenum pname,
											 GLint param);
typedef void(APIENTRY *PFNGLTEXPARAMETERF)(GLenum target, GLenum pname,
											 GLfloat param);
typedef void(APIENTRY *PFNGLACTIVETEXTURE)(GLenum texture);
typedef void(APIENTRY *PFNGLBINDVERTEXARRAY)(GLuint array);
typedef void(APIENTRY *PFNGLGENVERTEXARRAYS)(GLsizei n, GLuint *arrays);
typedef void(APIENTRY *PFNGLDELETEVERTEXARRAYS)(GLsizei n,
												const GLuint *arrays);
typedef void(APIENTRY *PFNGLVERTEXATTRIBPOINTER)(GLuint index, GLint size,
												 GLenum type,
												 GLboolean normalized,
												 GLsizei stride,
												 const void *pointer);
typedef void(APIENTRY *PFNGLENABLEVERTEXATTRIBARRAY)(GLuint index);
typedef void(APIENTRY *PFNGLDISABLEVERTEXATTRIBARRAY)(GLuint index);
typedef void(APIENTRY *PFNGLVERTEXATTRIBDIVISOR)(GLuint index, GLuint divisor);
typedef void(APIENTRY *PFNGLDRAWARRAYSINSTANCED)(GLenum mode, GLint first,
												 GLsizei count,
												 GLsizei instancecount);
typedef void(APIENTRY *PFNGLGENFRAMEBUFFERS)(GLsizei n, GLuint *framebuffers);
typedef void(APIENTRY *PFNGLFRAMEBUFFERTEXTURE2D)(GLenum target,
													GLenum attachment,
													GLenum textarget,
													GLuint texture, GLint level);
typedef void(APIENTRY *PFNGLGETSHADERIV)(GLuint shader, GLenum pname,
										 GLint *params);
typedef void(APIENTRY *PFNGLGETSHADERINFOLOG)(GLuint shader, GLsizei bufSize,
												GLsizei *length, GLchar *infoLog);
typedef void(APIENTRY *PFNGLGETPROGRAMIV)(GLuint program, GLenum pname,
											GLint *params);
typedef void(APIENTRY *PFNGLGETPROGRAMINFOLOG)(GLuint program, GLsizei bufSize,
												 GLsizei *length,
												 GLchar *infoLog);
typedef void(APIENTRY *PFNGLDRAWELEMENTSINSTANCED)(GLenum mode, GLsizei count,
													 GLenum type,
													 const void *indices,
													 GLsizei primcount);
typedef void *(APIENTRY *PFNGLMAPBUFFER)(GLenum target, GLenum access);
typedef GLboolean(APIENTRY *PFNGLUNMAPBUFFER)(GLenum target);
typedef GLint(APIENTRY *PFNGLGETUNIFORMLOCATION)(GLuint program,
												 const GLchar *name);
typedef void(APIENTRY *PFNGLUNIFORM1I)(GLint location, GLint v0);
typedef void(APIENTRY *PFNGLUNIFORM1F)(GLint location, GLfloat v0);
typedef void(APIENTRY *PFNGLUNIFORM1FV)(GLint location, GLsizei count,
										const GLfloat *value);
typedef void(APIENTRY *PFNGLUNIFORM2FV)(GLint location, GLsizei count,
										const GLfloat *value);
typedef void(APIENTRY *PFNGLUNIFORM3FV)(GLint location, GLsizei count,
										const GLfloat *value);
typedef void(APIENTRY *PFNGLUNIFORM4FV)(GLint location, GLsizei count,
										const GLfloat *value);
typedef void(APIENTRY *PFNGLUNIFORM1IV)(GLint location, GLsizei count,
										const GLint *value);
typedef void(APIENTRY *PFNGLUNIFORM2IV)(GLint location, GLsizei count,
										const GLint *value);
typedef void(APIENTRY *PFNGLUNIFORM3IV)(GLint location, GLsizei count,
										const GLint *value);
typedef void(APIENTRY *PFNGLUNIFORM4IV)(GLint location, GLsizei count,
										const GLint *value);
typedef void(APIENTRY *PFNGLUNIFORMMATRIX3FV)(GLint location, GLsizei count,
												GLboolean transpose,
												const GLfloat *value);
typedef void(APIENTRY *PFNGLUNIFORMMATRIX4FV)(GLint location, GLsizei count,
												GLboolean transpose,
												const GLfloat *value);
typedef void(APIENTRY *PFNGLBINDRENDERBUFFER)(GLenum target,
												GLuint renderbuffer);
typedef void(APIENTRY *PFNGLDELETERENDERBUFFERS)(GLsizei n,
												 const GLuint *renderbuffers);
typedef void(APIENTRY *PFNGLDELETEFRAMEBUFFERS)(GLsizei n,
												const GLuint *framebuffers);
typedef void(APIENTRY *PFNGLRENDERBUFFERSTORAGE)(GLenum target,
												 GLenum internalformat,
												 GLsizei width, GLsizei height);
typedef GLenum(APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUS)(GLenum target);
typedef void(APIENTRY *PFNGLGENERATEMIPMAP)(GLenum target);
typedef void(APIENTRY *PFNGLBLITFRAMEBUFFER)(GLint srcX0, GLint srcY0,
											 GLint srcX1, GLint srcY1,
											 GLint dstX0, GLint dstY0,
											 GLint dstX1, GLint dstY1,
											 GLbitfield mask, GLenum filter);

extern PFNGLDELETEBUFFERS se_glDeleteBuffers;
extern PFNGLGENBUFFERS se_glGenBuffers;
extern PFNGLBINDBUFFER se_glBindBuffer;
extern PFNGLBUFFERSUBDATA se_glBufferSubData;
extern PFNGLBUFFERDATA se_glBufferData;
extern PFNGLUSEPROGRAM se_glUseProgram;
extern PFNGLCREATESHADER se_glCreateShader;
extern PFNGLSHADERSOURCE se_glShaderSource;
extern PFNGLCOMPILESHADER se_glCompileShader;
extern PFNGLCREATEPROGRAM se_glCreateProgram;
extern PFNGLLINKPROGRAM se_glLinkProgram;
extern PFNGLATTACHSHADER se_glAttachShader;
extern PFNGLDELETEPROGRAM se_glDeleteProgram;
extern PFNGLDELETESHADER se_glDeleteShader;
extern PFNGLGENRENDERBUFFERS se_glGenRenderbuffers;
extern PFNGLBINDFRAMEBUFFER se_glBindFramebuffer;
extern PFNGLFRAMEBUFFERRENDERBUFFER se_glFramebufferRenderbuffer;
extern PFNGLFRAMEBUFFERTEXTURE se_glFramebufferTexture;
extern PFNGLBINDVERTEXARRAY se_glBindVertexArray;
extern PFNGLGENVERTEXARRAYS se_glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYS se_glDeleteVertexArrays;
extern PFNGLVERTEXATTRIBPOINTER se_glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAY se_glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAY se_glDisableVertexAttribArray;
extern PFNGLVERTEXATTRIBDIVISOR se_glVertexAttribDivisor;
extern PFNGLDRAWARRAYSINSTANCED se_glDrawArraysInstanced;
extern PFNGLGENFRAMEBUFFERS se_glGenFramebuffers;
extern PFNGLFRAMEBUFFERTEXTURE2D se_glFramebufferTexture2D;
extern PFNGLGETSHADERIV se_glGetShaderiv;
extern PFNGLGETSHADERINFOLOG se_glGetShaderInfoLog;
extern PFNGLGETPROGRAMIV se_glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOG se_glGetProgramInfoLog;
extern PFNGLDRAWELEMENTSINSTANCED se_glDrawElementsInstanced;
extern PFNGLMAPBUFFER se_glMapBuffer;
extern PFNGLUNMAPBUFFER se_glUnmapBuffer;
extern PFNGLGETUNIFORMLOCATION se_glGetUniformLocation;
extern PFNGLUNIFORM1I se_glUniform1i;
extern PFNGLUNIFORM1F se_glUniform1f;
extern PFNGLUNIFORM1FV se_glUniform1fv;
extern PFNGLUNIFORM2FV se_glUniform2fv;
extern PFNGLUNIFORM3FV se_glUniform3fv;
extern PFNGLUNIFORM4FV se_glUniform4fv;
extern PFNGLUNIFORM1IV se_glUniform1iv;
extern PFNGLUNIFORM2IV se_glUniform2iv;
extern PFNGLUNIFORM3IV se_glUniform3iv;
extern PFNGLUNIFORM4IV se_glUniform4iv;
extern PFNGLUNIFORMMATRIX3FV se_glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FV se_glUniformMatrix4fv;
extern PFNGLBINDRENDERBUFFER se_glBindRenderbuffer;
extern PFNGLDELETERENDERBUFFERS se_glDeleteRenderbuffers;
extern PFNGLDELETEFRAMEBUFFERS se_glDeleteFramebuffers;
extern PFNGLRENDERBUFFERSTORAGE se_glRenderbufferStorage;
extern PFNGLCHECKFRAMEBUFFERSTATUS se_glCheckFramebufferStatus;
extern PFNGLGENERATEMIPMAP se_glGenerateMipmap;
extern PFNGLBLITFRAMEBUFFER se_glBlitFramebuffer;

#if defined(SE_GL_NO_DISPATCH_WRAPPERS)

#define glDeleteBuffers se_glDeleteBuffers
#define glGenBuffers se_glGenBuffers
#define glBindBuffer se_glBindBuffer
#define glBufferSubData se_glBufferSubData
#define glBufferData se_glBufferData
#define glUseProgram se_glUseProgram
#define glCreateShader se_glCreateShader
#define glShaderSource se_glShaderSource
#define glCompileShader se_glCompileShader
#define glCreateProgram se_glCreateProgram
#define glLinkProgram se_glLinkProgram
#define glAttachShader se_glAttachShader
#define glDeleteProgram se_glDeleteProgram
#define glDeleteShader se_glDeleteShader
#define glGenRenderbuffers se_glGenRenderbuffers
#define glBindFramebuffer se_glBindFramebuffer
#define glFramebufferRenderbuffer se_glFramebufferRenderbuffer
#define glFramebufferTexture se_glFramebufferTexture
#define glBindVertexArray se_glBindVertexArray
#define glGenVertexArrays se_glGenVertexArrays
#define glDeleteVertexArrays se_glDeleteVertexArrays
#define glVertexAttribPointer se_glVertexAttribPointer
#define glEnableVertexAttribArray se_glEnableVertexAttribArray
#define glDisableVertexAttribArray se_glDisableVertexAttribArray
#define glVertexAttribDivisor se_glVertexAttribDivisor
#define glDrawArraysInstanced se_glDrawArraysInstanced
#define glGenFramebuffers se_glGenFramebuffers
#define glFramebufferTexture2D se_glFramebufferTexture2D
#define glGetShaderiv se_glGetShaderiv
#define glGetShaderInfoLog se_glGetShaderInfoLog
#define glGetProgramiv se_glGetProgramiv
#define glGetProgramInfoLog se_glGetProgramInfoLog
#define glDrawElementsInstanced se_glDrawElementsInstanced
#define glMapBuffer se_glMapBuffer
#define glUnmapBuffer se_glUnmapBuffer
#define glGetUniformLocation se_glGetUniformLocation
#define glUniform1i se_glUniform1i
#define glUniform1f se_glUniform1f
#define glUniform1fv se_glUniform1fv
#define glUniform2fv se_glUniform2fv
#define glUniform3fv se_glUniform3fv
#define glUniform4fv se_glUniform4fv
#define glUniform1iv se_glUniform1iv
#define glUniform2iv se_glUniform2iv
#define glUniform3iv se_glUniform3iv
#define glUniform4iv se_glUniform4iv
#define glUniformMatrix3fv se_glUniformMatrix3fv
#define glUniformMatrix4fv se_glUniformMatrix4fv
#define glBindRenderbuffer se_glBindRenderbuffer
#define glDeleteRenderbuffers se_glDeleteRenderbuffers
#define glDeleteFramebuffers se_glDeleteFramebuffers
#define glRenderbufferStorage se_glRenderbufferStorage
#define glCheckFramebufferStatus se_glCheckFramebufferStatus
#define glGenerateMipmap se_glGenerateMipmap
#define glBlitFramebuffer se_glBlitFramebuffer

#else

extern void se_gl_dispatchDeleteBuffers(GLsizei n, const GLuint *buffers);
extern void se_gl_dispatchGenBuffers(GLsizei n, GLuint *buffers);
extern void se_gl_dispatchBindBuffer(GLenum target, GLuint buffer);
extern void se_gl_dispatchBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
extern void se_gl_dispatchBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
extern void se_gl_dispatchUseProgram(GLuint program);
extern GLuint se_gl_dispatchCreateShader(GLenum type);
extern void se_gl_dispatchShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
extern void se_gl_dispatchCompileShader(GLuint shader);
extern GLuint se_gl_dispatchCreateProgram(void);
extern void se_gl_dispatchLinkProgram(GLuint program);
extern void se_gl_dispatchAttachShader(GLuint program, GLuint shader);
extern void se_gl_dispatchDeleteProgram(GLuint program);
extern void se_gl_dispatchDeleteShader(GLuint shader);
extern void se_gl_dispatchGenRenderbuffers(GLsizei n, GLuint *renderbuffers);
extern void se_gl_dispatchBindFramebuffer(GLenum target, GLuint framebuffer);
extern void se_gl_dispatchFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern void se_gl_dispatchBindVertexArray(GLuint array);
extern void se_gl_dispatchGenVertexArrays(GLsizei n, GLuint *arrays);
extern void se_gl_dispatchDeleteVertexArrays(GLsizei n, const GLuint *arrays);
extern void se_gl_dispatchVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
extern void se_gl_dispatchEnableVertexAttribArray(GLuint index);
extern void se_gl_dispatchVertexAttribDivisor(GLuint index, GLuint divisor);
extern void se_gl_dispatchDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
extern void se_gl_dispatchGenFramebuffers(GLsizei n, GLuint *framebuffers);
extern void se_gl_dispatchFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void se_gl_dispatchGetShaderiv(GLuint shader, GLenum pname, GLint *params);
extern void se_gl_dispatchGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
extern void se_gl_dispatchGetProgramiv(GLuint program, GLenum pname, GLint *params);
extern void se_gl_dispatchGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
extern void se_gl_dispatchDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
extern void *se_gl_dispatchMapBuffer(GLenum target, GLenum access);
extern GLboolean se_gl_dispatchUnmapBuffer(GLenum target);
extern GLint se_gl_dispatchGetUniformLocation(GLuint program, const GLchar *name);
extern void se_gl_dispatchUniform1i(GLint location, GLint v0);
extern void se_gl_dispatchUniform1f(GLint location, GLfloat v0);
extern void se_gl_dispatchUniform1fv(GLint location, GLsizei count, const GLfloat *value);
extern void se_gl_dispatchUniform2fv(GLint location, GLsizei count, const GLfloat *value);
extern void se_gl_dispatchUniform3fv(GLint location, GLsizei count, const GLfloat *value);
extern void se_gl_dispatchUniform4fv(GLint location, GLsizei count, const GLfloat *value);
extern void se_gl_dispatchUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void se_gl_dispatchUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void se_gl_dispatchBindRenderbuffer(GLenum target, GLuint renderbuffer);
extern void se_gl_dispatchDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers);
extern void se_gl_dispatchDeleteFramebuffers(GLsizei n, const GLuint *framebuffers);
extern void se_gl_dispatchRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern GLenum se_gl_dispatchCheckFramebufferStatus(GLenum target);
extern void se_gl_dispatchGenerateMipmap(GLenum target);
extern void se_gl_dispatchBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

extern void se_gl_dispatchEnable(GLenum cap);
extern void se_gl_dispatchDisable(GLenum cap);
extern void se_gl_dispatchBlendEquation(GLenum mode);
extern void se_gl_dispatchBlendFunc(GLenum sfactor, GLenum dfactor);
extern void se_gl_dispatchClear(GLbitfield mask);
extern void se_gl_dispatchClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void se_gl_dispatchColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void se_gl_dispatchDepthMask(GLboolean flag);
extern void se_gl_dispatchStencilMask(GLuint mask);
extern void se_gl_dispatchDepthFunc(GLenum func);
extern void se_gl_dispatchCullFace(GLenum mode);
extern void se_gl_dispatchFrontFace(GLenum mode);
extern void se_gl_dispatchDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices);
extern void se_gl_dispatchBindTexture(GLenum target, GLuint texture);
extern void se_gl_dispatchDeleteTextures(GLsizei n, const GLuint *textures);
extern void se_gl_dispatchGenTextures(GLsizei n, GLuint *textures);
extern void se_gl_dispatchTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
extern void se_gl_dispatchTexParameteri(GLenum target, GLenum pname, GLint param);
extern void se_gl_dispatchActiveTexture(GLenum texture);
extern void se_gl_dispatchViewport(GLint x, GLint y, GLsizei width, GLsizei height);
extern void se_gl_dispatchGetIntegerv(GLenum pname, GLint *data);
extern void se_gl_dispatchGetBooleanv(GLenum pname, GLboolean *data);
extern void se_gl_dispatchGetFloatv(GLenum pname, GLfloat *data);
extern GLboolean se_gl_dispatchIsEnabled(GLenum cap);
extern void se_gl_dispatchPixelStorei(GLenum pname, GLint param);
extern void se_gl_dispatchReadBuffer(GLenum src);
extern void se_gl_dispatchReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
extern const GLubyte* se_gl_dispatchGetString(GLenum name);

#define glDeleteBuffers se_gl_dispatchDeleteBuffers
#define glGenBuffers se_gl_dispatchGenBuffers
#define glBindBuffer se_gl_dispatchBindBuffer
#define glBufferSubData se_gl_dispatchBufferSubData
#define glBufferData se_gl_dispatchBufferData
#define glUseProgram se_gl_dispatchUseProgram
#define glCreateShader se_gl_dispatchCreateShader
#define glShaderSource se_gl_dispatchShaderSource
#define glCompileShader se_gl_dispatchCompileShader
#define glCreateProgram se_gl_dispatchCreateProgram
#define glLinkProgram se_gl_dispatchLinkProgram
#define glAttachShader se_gl_dispatchAttachShader
#define glDeleteProgram se_gl_dispatchDeleteProgram
#define glDeleteShader se_gl_dispatchDeleteShader
#define glGenRenderbuffers se_gl_dispatchGenRenderbuffers
#define glBindFramebuffer se_gl_dispatchBindFramebuffer
#define glFramebufferRenderbuffer se_gl_dispatchFramebufferRenderbuffer
#define glFramebufferTexture se_glFramebufferTexture
#define glBindVertexArray se_gl_dispatchBindVertexArray
#define glGenVertexArrays se_gl_dispatchGenVertexArrays
#define glDeleteVertexArrays se_gl_dispatchDeleteVertexArrays
#define glVertexAttribPointer se_gl_dispatchVertexAttribPointer
#define glEnableVertexAttribArray se_gl_dispatchEnableVertexAttribArray
#define glDisableVertexAttribArray se_glDisableVertexAttribArray
#define glVertexAttribDivisor se_gl_dispatchVertexAttribDivisor
#define glDrawArraysInstanced se_gl_dispatchDrawArraysInstanced
#define glGenFramebuffers se_gl_dispatchGenFramebuffers
#define glFramebufferTexture2D se_gl_dispatchFramebufferTexture2D
#define glGetShaderiv se_gl_dispatchGetShaderiv
#define glGetShaderInfoLog se_gl_dispatchGetShaderInfoLog
#define glGetProgramiv se_gl_dispatchGetProgramiv
#define glGetProgramInfoLog se_gl_dispatchGetProgramInfoLog
#define glDrawElementsInstanced se_gl_dispatchDrawElementsInstanced
#define glMapBuffer se_gl_dispatchMapBuffer
#define glUnmapBuffer se_gl_dispatchUnmapBuffer
#define glGetUniformLocation se_gl_dispatchGetUniformLocation
#define glUniform1i se_gl_dispatchUniform1i
#define glUniform1f se_gl_dispatchUniform1f
#define glUniform1fv se_gl_dispatchUniform1fv
#define glUniform2fv se_gl_dispatchUniform2fv
#define glUniform3fv se_gl_dispatchUniform3fv
#define glUniform4fv se_gl_dispatchUniform4fv
#define glUniform1iv se_glUniform1iv
#define glUniform2iv se_glUniform2iv
#define glUniform3iv se_glUniform3iv
#define glUniform4iv se_glUniform4iv
#define glUniformMatrix3fv se_gl_dispatchUniformMatrix3fv
#define glUniformMatrix4fv se_gl_dispatchUniformMatrix4fv
#define glBindRenderbuffer se_gl_dispatchBindRenderbuffer
#define glDeleteRenderbuffers se_gl_dispatchDeleteRenderbuffers
#define glDeleteFramebuffers se_gl_dispatchDeleteFramebuffers
#define glRenderbufferStorage se_gl_dispatchRenderbufferStorage
#define glCheckFramebufferStatus se_gl_dispatchCheckFramebufferStatus
#define glGenerateMipmap se_gl_dispatchGenerateMipmap
#define glBlitFramebuffer se_gl_dispatchBlitFramebuffer

#define glEnable se_gl_dispatchEnable
#define glDisable se_gl_dispatchDisable
#define glBlendEquation se_gl_dispatchBlendEquation
#define glBlendFunc se_gl_dispatchBlendFunc
#define glClear se_gl_dispatchClear
#define glClearColor se_gl_dispatchClearColor
#define glColorMask se_gl_dispatchColorMask
#define glDepthMask se_gl_dispatchDepthMask
#define glStencilMask se_gl_dispatchStencilMask
#define glDepthFunc se_gl_dispatchDepthFunc
#define glCullFace se_gl_dispatchCullFace
#define glFrontFace se_gl_dispatchFrontFace
#define glDrawElements se_gl_dispatchDrawElements
#define glBindTexture se_gl_dispatchBindTexture
#define glDeleteTextures se_gl_dispatchDeleteTextures
#define glGenTextures se_gl_dispatchGenTextures
#define glTexImage2D se_gl_dispatchTexImage2D
#define glTexParameteri se_gl_dispatchTexParameteri
#define glActiveTexture se_gl_dispatchActiveTexture
#define glViewport se_gl_dispatchViewport
#define glGetIntegerv se_gl_dispatchGetIntegerv
#define glGetBooleanv se_gl_dispatchGetBooleanv
#define glGetFloatv se_gl_dispatchGetFloatv
#define glIsEnabled se_gl_dispatchIsEnabled
#define glPixelStorei se_gl_dispatchPixelStorei
#define glReadBuffer se_gl_dispatchReadBuffer
#define glReadPixels se_gl_dispatchReadPixels
#define glGetString se_gl_dispatchGetString

#endif

extern b8 se_init_opengl(void);

#endif // SE_GL_H
