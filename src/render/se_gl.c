// Syphax-Engine - Ougi Washi

#if defined(SE_RENDER_BACKEND_OPENGL) || defined(SE_RENDER_BACKEND_GLES)

#include "se_gl.h"
#include "se_debug.h"
#include "se_render.h"
#include "syphax/s_files.h"
#if defined(SE_RENDER_BACKEND_GLES)
#include <EGL/egl.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

static void *se_gl_get_proc_address(const char *name) {
#if defined(SE_RENDER_BACKEND_GLES)
	return (void *)eglGetProcAddress(name);
#else
	return (void *)glfwGetProcAddress(name);
#endif
}

#define SE_UNIFORMS_MAX 128

static b8 se_render_initialized = false;
static b8 se_is_blending = false;
static u64 se_render_generation = 0;

static b8 se_render_ready_for_gl_calls(void) {
	if (se_render_initialized) {
		return true;
	}
	if (!se_render_has_context()) {
		return false;
	}
	return se_render_init();
}

PFNGLDELETEBUFFERS se_glDeleteBuffers = NULL;
PFNGLGENBUFFERS se_glGenBuffers = NULL;
PFNGLBINDBUFFER se_glBindBuffer = NULL;
PFNGLBUFFERSUBDATA se_glBufferSubData = NULL;
PFNGLBUFFERDATA se_glBufferData = NULL;
PFNGLUSEPROGRAM se_glUseProgram = NULL;
PFNGLCREATESHADER se_glCreateShader = NULL;
PFNGLSHADERSOURCE se_glShaderSource = NULL;
PFNGLCOMPILESHADER se_glCompileShader = NULL;
PFNGLCREATEPROGRAM se_glCreateProgram = NULL;
PFNGLLINKPROGRAM se_glLinkProgram = NULL;
PFNGLATTACHSHADER se_glAttachShader = NULL;
PFNGLDELETEPROGRAM se_glDeleteProgram = NULL;
PFNGLDELETESHADER se_glDeleteShader = NULL;
PFNGLGENRENDERBUFFERS se_glGenRenderbuffers = NULL;
PFNGLBINDFRAMEBUFFER se_glBindFramebuffer = NULL;
PFNGLFRAMEBUFFERRENDERBUFFER se_glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTURE se_glFramebufferTexture = NULL;
PFNGLBINDVERTEXARRAY se_glBindVertexArray = NULL;
PFNGLGENVERTEXARRAYS se_glGenVertexArrays = NULL;
PFNGLDELETEVERTEXARRAYS se_glDeleteVertexArrays = NULL;
PFNGLVERTEXATTRIBPOINTER se_glVertexAttribPointer = NULL;
PFNGLENABLEVERTEXATTRIBARRAY se_glEnableVertexAttribArray = NULL;
PFNGLDISABLEVERTEXATTRIBARRAY se_glDisableVertexAttribArray = NULL;
PFNGLVERTEXATTRIBDIVISOR se_glVertexAttribDivisor = NULL;
PFNGLDRAWARRAYSINSTANCED se_glDrawArraysInstanced = NULL;
PFNGLGENFRAMEBUFFERS se_glGenFramebuffers = NULL;
PFNGLFRAMEBUFFERTEXTURE2D se_glFramebufferTexture2D = NULL;
PFNGLGETSHADERIV se_glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOG se_glGetShaderInfoLog = NULL;
PFNGLGETPROGRAMIV se_glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOG se_glGetProgramInfoLog = NULL;
PFNGLDRAWELEMENTSINSTANCED se_glDrawElementsInstanced = NULL;
PFNGLMAPBUFFER se_glMapBuffer = NULL;
PFNGLUNMAPBUFFER se_glUnmapBuffer = NULL;
PFNGLGETUNIFORMLOCATION se_glGetUniformLocation = NULL;
PFNGLUNIFORM1I se_glUniform1i = NULL;
PFNGLUNIFORM1F se_glUniform1f = NULL;
PFNGLUNIFORM1FV se_glUniform1fv = NULL;
PFNGLUNIFORM2FV se_glUniform2fv = NULL;
PFNGLUNIFORM3FV se_glUniform3fv = NULL;
PFNGLUNIFORM4FV se_glUniform4fv = NULL;
PFNGLUNIFORM1IV se_glUniform1iv = NULL;
PFNGLUNIFORM2IV se_glUniform2iv = NULL;
PFNGLUNIFORM3IV se_glUniform3iv = NULL;
PFNGLUNIFORM4IV se_glUniform4iv = NULL;
PFNGLUNIFORMMATRIX3FV se_glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX4FV se_glUniformMatrix4fv = NULL;
PFNGLBINDRENDERBUFFER se_glBindRenderbuffer = NULL;
PFNGLDELETERENDERBUFFERS se_glDeleteRenderbuffers = NULL;
PFNGLDELETEFRAMEBUFFERS se_glDeleteFramebuffers = NULL;
PFNGLRENDERBUFFERSTORAGE se_glRenderbufferStorage = NULL;
PFNGLCHECKFRAMEBUFFERSTATUS se_glCheckFramebufferStatus = NULL;
PFNGLGENERATEMIPMAP se_glGenerateMipmap = NULL;
PFNGLBLITFRAMEBUFFER se_glBlitFramebuffer = NULL;

#define SE_GL_ASSIGN(func, func_type, name, fallback) \
	func = (func_type)se_gl_get_proc_address(name); \
	if (!func) { \
		func = (func_type)(fallback); \
	} \
	if (!func) { \
		se_log("se_init_opengl :: failed to load OpenGL function: %s", name); \
		return false; \
	}

b8 se_init_opengl(void) {
#if defined(SE_RENDER_BACKEND_GLES)
	SE_GL_ASSIGN(se_glDeleteBuffers, PFNGLDELETEBUFFERS, "glDeleteBuffers", glDeleteBuffers);
	SE_GL_ASSIGN(se_glGenBuffers, PFNGLGENBUFFERS, "glGenBuffers", glGenBuffers);
	SE_GL_ASSIGN(se_glBindBuffer, PFNGLBINDBUFFER, "glBindBuffer", glBindBuffer);
	SE_GL_ASSIGN(se_glBufferSubData, PFNGLBUFFERSUBDATA, "glBufferSubData", glBufferSubData);
	SE_GL_ASSIGN(se_glBufferData, PFNGLBUFFERDATA, "glBufferData", glBufferData);
	SE_GL_ASSIGN(se_glUseProgram, PFNGLUSEPROGRAM, "glUseProgram", glUseProgram);
	SE_GL_ASSIGN(se_glCreateShader, PFNGLCREATESHADER, "glCreateShader", glCreateShader);
	SE_GL_ASSIGN(se_glShaderSource, PFNGLSHADERSOURCE, "glShaderSource", glShaderSource);
	SE_GL_ASSIGN(se_glCompileShader, PFNGLCOMPILESHADER, "glCompileShader", glCompileShader);
	SE_GL_ASSIGN(se_glCreateProgram, PFNGLCREATEPROGRAM, "glCreateProgram", glCreateProgram);
	SE_GL_ASSIGN(se_glLinkProgram, PFNGLLINKPROGRAM, "glLinkProgram", glLinkProgram);
	SE_GL_ASSIGN(se_glAttachShader, PFNGLATTACHSHADER, "glAttachShader", glAttachShader);
	SE_GL_ASSIGN(se_glDeleteProgram, PFNGLDELETEPROGRAM, "glDeleteProgram", glDeleteProgram);
	SE_GL_ASSIGN(se_glDeleteShader, PFNGLDELETESHADER, "glDeleteShader", glDeleteShader);
	SE_GL_ASSIGN(se_glGenRenderbuffers, PFNGLGENRENDERBUFFERS, "glGenRenderbuffers", glGenRenderbuffers);
	SE_GL_ASSIGN(se_glBindFramebuffer, PFNGLBINDFRAMEBUFFER, "glBindFramebuffer", glBindFramebuffer);
	SE_GL_ASSIGN(se_glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFER, "glFramebufferRenderbuffer", glFramebufferRenderbuffer);
	SE_GL_ASSIGN(se_glFramebufferTexture, PFNGLFRAMEBUFFERTEXTURE, "glFramebufferTexture", glFramebufferTexture);
	SE_GL_ASSIGN(se_glBindVertexArray, PFNGLBINDVERTEXARRAY, "glBindVertexArray", glBindVertexArray);
	SE_GL_ASSIGN(se_glGenVertexArrays, PFNGLGENVERTEXARRAYS, "glGenVertexArrays", glGenVertexArrays);
	SE_GL_ASSIGN(se_glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYS, "glDeleteVertexArrays", glDeleteVertexArrays);
	SE_GL_ASSIGN(se_glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTER, "glVertexAttribPointer", glVertexAttribPointer);
	SE_GL_ASSIGN(se_glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAY, "glEnableVertexAttribArray", glEnableVertexAttribArray);
	SE_GL_ASSIGN(se_glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAY, "glDisableVertexAttribArray", glDisableVertexAttribArray);
	SE_GL_ASSIGN(se_glVertexAttribDivisor, PFNGLVERTEXATTRIBDIVISOR, "glVertexAttribDivisor", glVertexAttribDivisor);
	SE_GL_ASSIGN(se_glDrawArraysInstanced, PFNGLDRAWARRAYSINSTANCED, "glDrawArraysInstanced", glDrawArraysInstanced);
	SE_GL_ASSIGN(se_glGenFramebuffers, PFNGLGENFRAMEBUFFERS, "glGenFramebuffers", glGenFramebuffers);
	SE_GL_ASSIGN(se_glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2D, "glFramebufferTexture2D", glFramebufferTexture2D);
	SE_GL_ASSIGN(se_glGetShaderiv, PFNGLGETSHADERIV, "glGetShaderiv", glGetShaderiv);
	SE_GL_ASSIGN(se_glGetShaderInfoLog, PFNGLGETSHADERINFOLOG, "glGetShaderInfoLog", glGetShaderInfoLog);
	SE_GL_ASSIGN(se_glGetProgramiv, PFNGLGETPROGRAMIV, "glGetProgramiv", glGetProgramiv);
	SE_GL_ASSIGN(se_glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOG, "glGetProgramInfoLog", glGetProgramInfoLog);
	SE_GL_ASSIGN(se_glDrawElementsInstanced, PFNGLDRAWELEMENTSINSTANCED, "glDrawElementsInstanced", glDrawElementsInstanced);
	SE_GL_ASSIGN(se_glMapBuffer, PFNGLMAPBUFFER, "glMapBuffer", glMapBuffer);
	SE_GL_ASSIGN(se_glUnmapBuffer, PFNGLUNMAPBUFFER, "glUnmapBuffer", glUnmapBuffer);
	SE_GL_ASSIGN(se_glGetUniformLocation, PFNGLGETUNIFORMLOCATION, "glGetUniformLocation", glGetUniformLocation);
	SE_GL_ASSIGN(se_glUniform1i, PFNGLUNIFORM1I, "glUniform1i", glUniform1i);
	SE_GL_ASSIGN(se_glUniform1f, PFNGLUNIFORM1F, "glUniform1f", glUniform1f);
	SE_GL_ASSIGN(se_glUniform1fv, PFNGLUNIFORM1FV, "glUniform1fv", glUniform1fv);
	SE_GL_ASSIGN(se_glUniform2fv, PFNGLUNIFORM2FV, "glUniform2fv", glUniform2fv);
	SE_GL_ASSIGN(se_glUniform3fv, PFNGLUNIFORM3FV, "glUniform3fv", glUniform3fv);
	SE_GL_ASSIGN(se_glUniform4fv, PFNGLUNIFORM4FV, "glUniform4fv", glUniform4fv);
	SE_GL_ASSIGN(se_glUniform1iv, PFNGLUNIFORM1IV, "glUniform1iv", glUniform1iv);
	SE_GL_ASSIGN(se_glUniform2iv, PFNGLUNIFORM2IV, "glUniform2iv", glUniform2iv);
	SE_GL_ASSIGN(se_glUniform3iv, PFNGLUNIFORM3IV, "glUniform3iv", glUniform3iv);
	SE_GL_ASSIGN(se_glUniform4iv, PFNGLUNIFORM4IV, "glUniform4iv", glUniform4iv);
	SE_GL_ASSIGN(se_glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FV, "glUniformMatrix3fv", glUniformMatrix3fv);
	SE_GL_ASSIGN(se_glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FV, "glUniformMatrix4fv", glUniformMatrix4fv);
	SE_GL_ASSIGN(se_glBindRenderbuffer, PFNGLBINDRENDERBUFFER, "glBindRenderbuffer", glBindRenderbuffer);
	SE_GL_ASSIGN(se_glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERS, "glDeleteRenderbuffers", glDeleteRenderbuffers);
	SE_GL_ASSIGN(se_glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERS, "glDeleteFramebuffers", glDeleteFramebuffers);
	SE_GL_ASSIGN(se_glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGE, "glRenderbufferStorage", glRenderbufferStorage);
	SE_GL_ASSIGN(se_glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUS, "glCheckFramebufferStatus", glCheckFramebufferStatus);
	SE_GL_ASSIGN(se_glGenerateMipmap, PFNGLGENERATEMIPMAP, "glGenerateMipmap", glGenerateMipmap);
	SE_GL_ASSIGN(se_glBlitFramebuffer, PFNGLBLITFRAMEBUFFER, "glBlitFramebuffer", glBlitFramebuffer);
#else
	SE_GL_ASSIGN(se_glDeleteBuffers, PFNGLDELETEBUFFERS, "glDeleteBuffers", NULL);
	SE_GL_ASSIGN(se_glGenBuffers, PFNGLGENBUFFERS, "glGenBuffers", NULL);
	SE_GL_ASSIGN(se_glBindBuffer, PFNGLBINDBUFFER, "glBindBuffer", NULL);
	SE_GL_ASSIGN(se_glBufferSubData, PFNGLBUFFERSUBDATA, "glBufferSubData", NULL);
	SE_GL_ASSIGN(se_glBufferData, PFNGLBUFFERDATA, "glBufferData", NULL);
	SE_GL_ASSIGN(se_glUseProgram, PFNGLUSEPROGRAM, "glUseProgram", NULL);
	SE_GL_ASSIGN(se_glCreateShader, PFNGLCREATESHADER, "glCreateShader", NULL);
	SE_GL_ASSIGN(se_glShaderSource, PFNGLSHADERSOURCE, "glShaderSource", NULL);
	SE_GL_ASSIGN(se_glCompileShader, PFNGLCOMPILESHADER, "glCompileShader", NULL);
	SE_GL_ASSIGN(se_glCreateProgram, PFNGLCREATEPROGRAM, "glCreateProgram", NULL);
	SE_GL_ASSIGN(se_glLinkProgram, PFNGLLINKPROGRAM, "glLinkProgram", NULL);
	SE_GL_ASSIGN(se_glAttachShader, PFNGLATTACHSHADER, "glAttachShader", NULL);
	SE_GL_ASSIGN(se_glDeleteProgram, PFNGLDELETEPROGRAM, "glDeleteProgram", NULL);
	SE_GL_ASSIGN(se_glDeleteShader, PFNGLDELETESHADER, "glDeleteShader", NULL);
	SE_GL_ASSIGN(se_glGenRenderbuffers, PFNGLGENRENDERBUFFERS, "glGenRenderbuffers", NULL);
	SE_GL_ASSIGN(se_glBindFramebuffer, PFNGLBINDFRAMEBUFFER, "glBindFramebuffer", NULL);
	SE_GL_ASSIGN(se_glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFER, "glFramebufferRenderbuffer", NULL);
	SE_GL_ASSIGN(se_glFramebufferTexture, PFNGLFRAMEBUFFERTEXTURE, "glFramebufferTexture", NULL);
	SE_GL_ASSIGN(se_glBindVertexArray, PFNGLBINDVERTEXARRAY, "glBindVertexArray", NULL);
	SE_GL_ASSIGN(se_glGenVertexArrays, PFNGLGENVERTEXARRAYS, "glGenVertexArrays", NULL);
	SE_GL_ASSIGN(se_glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYS, "glDeleteVertexArrays", NULL);
	SE_GL_ASSIGN(se_glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTER, "glVertexAttribPointer", NULL);
	SE_GL_ASSIGN(se_glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAY, "glEnableVertexAttribArray", NULL);
	SE_GL_ASSIGN(se_glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAY, "glDisableVertexAttribArray", NULL);
	SE_GL_ASSIGN(se_glVertexAttribDivisor, PFNGLVERTEXATTRIBDIVISOR, "glVertexAttribDivisor", NULL);
	SE_GL_ASSIGN(se_glDrawArraysInstanced, PFNGLDRAWARRAYSINSTANCED, "glDrawArraysInstanced", NULL);
	SE_GL_ASSIGN(se_glGenFramebuffers, PFNGLGENFRAMEBUFFERS, "glGenFramebuffers", NULL);
	SE_GL_ASSIGN(se_glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2D, "glFramebufferTexture2D", NULL);
	SE_GL_ASSIGN(se_glGetShaderiv, PFNGLGETSHADERIV, "glGetShaderiv", NULL);
	SE_GL_ASSIGN(se_glGetShaderInfoLog, PFNGLGETSHADERINFOLOG, "glGetShaderInfoLog", NULL);
	SE_GL_ASSIGN(se_glGetProgramiv, PFNGLGETPROGRAMIV, "glGetProgramiv", NULL);
	SE_GL_ASSIGN(se_glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOG, "glGetProgramInfoLog", NULL);
	SE_GL_ASSIGN(se_glDrawElementsInstanced, PFNGLDRAWELEMENTSINSTANCED, "glDrawElementsInstanced", NULL);
	SE_GL_ASSIGN(se_glMapBuffer, PFNGLMAPBUFFER, "glMapBuffer", NULL);
	SE_GL_ASSIGN(se_glUnmapBuffer, PFNGLUNMAPBUFFER, "glUnmapBuffer", NULL);
	SE_GL_ASSIGN(se_glGetUniformLocation, PFNGLGETUNIFORMLOCATION, "glGetUniformLocation", NULL);
	SE_GL_ASSIGN(se_glUniform1i, PFNGLUNIFORM1I, "glUniform1i", NULL);
	SE_GL_ASSIGN(se_glUniform1f, PFNGLUNIFORM1F, "glUniform1f", NULL);
	SE_GL_ASSIGN(se_glUniform1fv, PFNGLUNIFORM1FV, "glUniform1fv", NULL);
	SE_GL_ASSIGN(se_glUniform2fv, PFNGLUNIFORM2FV, "glUniform2fv", NULL);
	SE_GL_ASSIGN(se_glUniform3fv, PFNGLUNIFORM3FV, "glUniform3fv", NULL);
	SE_GL_ASSIGN(se_glUniform4fv, PFNGLUNIFORM4FV, "glUniform4fv", NULL);
	SE_GL_ASSIGN(se_glUniform1iv, PFNGLUNIFORM1IV, "glUniform1iv", NULL);
	SE_GL_ASSIGN(se_glUniform2iv, PFNGLUNIFORM2IV, "glUniform2iv", NULL);
	SE_GL_ASSIGN(se_glUniform3iv, PFNGLUNIFORM3IV, "glUniform3iv", NULL);
	SE_GL_ASSIGN(se_glUniform4iv, PFNGLUNIFORM4IV, "glUniform4iv", NULL);
	SE_GL_ASSIGN(se_glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FV, "glUniformMatrix3fv", NULL);
	SE_GL_ASSIGN(se_glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FV, "glUniformMatrix4fv", NULL);
	SE_GL_ASSIGN(se_glBindRenderbuffer, PFNGLBINDRENDERBUFFER, "glBindRenderbuffer", NULL);
	SE_GL_ASSIGN(se_glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERS, "glDeleteRenderbuffers", NULL);
	SE_GL_ASSIGN(se_glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERS, "glDeleteFramebuffers", NULL);
	SE_GL_ASSIGN(se_glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGE, "glRenderbufferStorage", NULL);
	SE_GL_ASSIGN(se_glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUS, "glCheckFramebufferStatus", NULL);
	SE_GL_ASSIGN(se_glGenerateMipmap, PFNGLGENERATEMIPMAP, "glGenerateMipmap", NULL);
	SE_GL_ASSIGN(se_glBlitFramebuffer, PFNGLBLITFRAMEBUFFER, "glBlitFramebuffer", NULL);
#endif
	return true;
}

b8 se_render_init(void) {
#if defined(SE_WINDOW_BACKEND_GLFW)
	if (!glfwGetCurrentContext()) {
		se_log("se_render_init :: no active OpenGL context");
		se_render_initialized = false;
		return false;
	}
#endif
	if (se_render_initialized) {
		return true;
	}
	if (!se_init_opengl()) {
		se_log("se_render_init :: failed to load required OpenGL symbols");
		se_render_initialized = false;
		return false;
	}
	se_render_initialized = true;
	se_render_generation++;
	return true;
}

void se_render_shutdown(void) {
	se_render_initialized = false;
	se_is_blending = false;
}

b8 se_render_has_context(void) {
#if defined(SE_WINDOW_BACKEND_GLFW)
	return glfwGetCurrentContext() != NULL;
#else
	return se_render_initialized;
#endif
}

u64 se_render_get_generation(void) {
	return se_render_generation;
}

void se_render_set_blending(const b8 active) {
	if (active) {
		if (se_is_blending) {
			return;
		}
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		se_is_blending = true;
		return;
	}
	if (!se_is_blending) {
		return;
	}
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	se_is_blending = false;
}

void se_render_unbind_framebuffer(void) { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void se_render_clear(void) {
	if (!se_render_initialized) {
		return;
	}

	GLboolean color_mask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
	GLboolean depth_mask = GL_TRUE;
	GLint stencil_mask = 0xFF;
	const GLboolean scissor_enabled = glIsEnabled(GL_SCISSOR_TEST);

	glGetBooleanv(GL_COLOR_WRITEMASK, color_mask);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_mask);
	glGetIntegerv(GL_STENCIL_WRITEMASK, &stencil_mask);

	if (scissor_enabled) {
		glDisable(GL_SCISSOR_TEST);
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilMask(0xFF);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glColorMask(color_mask[0], color_mask[1], color_mask[2], color_mask[3]);
	glDepthMask(depth_mask);
	glStencilMask((GLuint)stencil_mask);

	if (scissor_enabled) {
		glEnable(GL_SCISSOR_TEST);
	}
}

void se_render_set_background_color(const s_vec4 color) {
	glClearColor(color.x, color.y, color.z, color.w);
}

void se_quad_add_vertex_attribute(se_quad *out_quad, const sz size,
									const GLenum type, const GLboolean normalized,
									const GLsizei stride, const void *pointer,
									const b8 per_instance) {
	s_assertf(out_quad, "se_quad_add_attribute :: out_quad is null");
	s_assertf(size > 0, "se_quad_add_attribute :: size is 0");

	glVertexAttribPointer(out_quad->last_attribute, size, type, normalized, stride, pointer);
	glEnableVertexAttribArray(out_quad->last_attribute);
	glVertexAttribDivisor(out_quad->last_attribute, per_instance ? 1 : 0);
	out_quad->last_attribute++;
}

void se_quad_3d_create(se_quad *out_quad) {
	s_assertf(out_quad, "se_quad_create :: out_quad is null");

	out_quad->vao = 0;
	out_quad->vbo = 0;
	out_quad->ebo = 0;
	out_quad->last_attribute = 0;
	s_array_init(&out_quad->instance_buffers);
	out_quad->instance_buffers_dirty = false;
	if (!se_render_ready_for_gl_calls()) {
		return;
	}

	glGenVertexArrays(1, &out_quad->vao);
	glGenBuffers(1, &out_quad->vbo);
	glGenBuffers(1, &out_quad->ebo);

	glBindVertexArray(out_quad->vao);

	glBindBuffer(GL_ARRAY_BUFFER, out_quad->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(se_quad_3d_vertices), se_quad_3d_vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_quad->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(se_quad_indices), se_quad_indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, uv));

	glBindVertexArray(0);
}
void se_quad_2d_create(se_quad *out_quad, const u32 instance_count) {
	s_assertf(out_quad, "se_quad_create :: out_quad is null");

	out_quad->vao = 0;
	out_quad->vbo = 0;
	out_quad->ebo = 0;
	out_quad->last_attribute = 0;
	s_array_init(&out_quad->instance_buffers);
	out_quad->instance_buffers_dirty = false;
	if (!se_render_ready_for_gl_calls()) {
		return;
	}

	glGenVertexArrays(1, &out_quad->vao);
	glGenBuffers(1, &out_quad->vbo);
	glGenBuffers(1, &out_quad->ebo);

	glBindVertexArray(out_quad->vao);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_quad->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(se_quad_indices), se_quad_indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, out_quad->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(se_quad_2d_vertices), se_quad_2d_vertices, GL_STATIC_DRAW);

	se_quad_add_vertex_attribute(out_quad, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_2d), (const void *)offsetof(se_vertex_2d, position), false);
	se_quad_add_vertex_attribute(out_quad, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_2d), (const void *)offsetof(se_vertex_2d, uv), false);

	glBindVertexArray(0);

	if (instance_count > 0) {
		s_array_reserve(&out_quad->instance_buffers, instance_count);
		out_quad->instance_buffers_dirty = true;
	}
}

void se_quad_2d_add_instance_buffer(se_quad *quad, const s_mat4 *buffer, const sz instance_count) {
	s_assertf(quad, "se_quad_2d_add_instance_buffer :: quad is null");
	s_assertf(buffer, "se_quad_2d_add_instance_buffer :: buffer is null");
	if (!se_render_ready_for_gl_calls() || quad->vao == 0) {
		return;
	}

	glBindVertexArray(quad->vao);

	s_handle new_buffer_handle = s_array_increment(&quad->instance_buffers);
	se_instance_buffer *new_buffer = s_array_get(&quad->instance_buffers, new_buffer_handle);
	s_assertf(new_buffer, "se_quad_2d_add_instance_buffer :: out of memory");
	new_buffer->vbo = 0;
	new_buffer->buffer_ptr = buffer;
	new_buffer->buffer_size = sizeof(s_mat4) * instance_count;
	glGenBuffers(1, &new_buffer->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, new_buffer->vbo);
	glBufferData(GL_ARRAY_BUFFER, new_buffer->buffer_size, new_buffer->buffer_ptr, GL_DYNAMIC_DRAW);

	se_quad_add_vertex_attribute(quad, 4, GL_FLOAT, GL_FALSE, sizeof(s_mat4), (void *)0, true);
	se_quad_add_vertex_attribute(quad, 4, GL_FLOAT, GL_FALSE, sizeof(s_mat4), (void *)(sizeof(s_vec4) * 1), true);
	se_quad_add_vertex_attribute(quad, 4, GL_FLOAT, GL_FALSE, sizeof(s_mat4), (void *)(sizeof(s_vec4) * 2), true);
	se_quad_add_vertex_attribute(quad, 4, GL_FLOAT, GL_FALSE, sizeof(s_mat4), (void *)(sizeof(s_vec4) * 3), true);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	quad->instance_buffers_dirty = true;

	se_log("se_quad_2d_add_instance_buffer :: buffer: %p", (const void*)buffer);
}

void se_quad_2d_add_instance_buffer_mat3(se_quad *quad, const s_mat3 *buffer, const sz instance_count) {
	s_assertf(quad, "se_quad_2d_add_instance_buffer_mat3 :: quad is null");
	s_assertf(buffer, "se_quad_2d_add_instance_buffer_mat3 :: buffer is null");
	if (!se_render_ready_for_gl_calls() || quad->vao == 0) {
		return;
	}

	glBindVertexArray(quad->vao);

	s_handle new_buffer_handle = s_array_increment(&quad->instance_buffers);
	se_instance_buffer *new_buffer = s_array_get(&quad->instance_buffers, new_buffer_handle);
	s_assertf(new_buffer, "se_quad_2d_add_instance_buffer_mat3 :: out of memory");
	new_buffer->vbo = 0;
	new_buffer->buffer_ptr = buffer;
	new_buffer->buffer_size = sizeof(s_mat3) * instance_count;
	glGenBuffers(1, &new_buffer->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, new_buffer->vbo);
	glBufferData(GL_ARRAY_BUFFER, new_buffer->buffer_size, new_buffer->buffer_ptr, GL_DYNAMIC_DRAW);

	se_quad_add_vertex_attribute(quad, 3, GL_FLOAT, GL_FALSE, sizeof(s_mat3), (void *)0, true);
	se_quad_add_vertex_attribute(quad, 3, GL_FLOAT, GL_FALSE, sizeof(s_mat3), (void *)(sizeof(s_vec3) * 1), true);
	se_quad_add_vertex_attribute(quad, 3, GL_FLOAT, GL_FALSE, sizeof(s_mat3), (void *)(sizeof(s_vec3) * 2), true);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	quad->instance_buffers_dirty = true;

	se_log("se_quad_2d_add_instance_buffer_mat3 :: buffer: %p", (const void*)buffer);
}

void se_quad_render(se_quad *quad, const sz instance_count) {
	if (!quad || !se_render_ready_for_gl_calls() || quad->vao == 0) {
		return;
	}
	glBindVertexArray(quad->vao);
	if (instance_count > 0) {
		if (quad->instance_buffers_dirty) {
			se_instance_buffer *current_buffer = NULL;
			s_foreach(&quad->instance_buffers, current_buffer) {
				glBindBuffer(GL_ARRAY_BUFFER, current_buffer->vbo);
				glBufferSubData(GL_ARRAY_BUFFER, 0, current_buffer->buffer_size, current_buffer->buffer_ptr);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			quad->instance_buffers_dirty = false;
		}
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, instance_count);
	} else {
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
	glBindVertexArray(0);
}

void se_quad_destroy(se_quad *quad) {
	if (!quad) {
		return;
	}
	if (!se_render_ready_for_gl_calls()) {
		s_array_clear(&quad->instance_buffers);
		quad->vao = 0;
		quad->vbo = 0;
		quad->ebo = 0;
		quad->last_attribute = 0;
		quad->instance_buffers_dirty = false;
		return;
	}
	se_instance_buffer *current_buffer = NULL;
	s_foreach(&quad->instance_buffers, current_buffer) {
		glDeleteBuffers(1, &current_buffer->vbo);
	}
	s_array_clear(&quad->instance_buffers);
	glDeleteVertexArrays(1, &quad->vao);
	glDeleteBuffers(1, &quad->vbo);
	glDeleteBuffers(1, &quad->ebo);
	quad->vao = 0;
	quad->vbo = 0;
	quad->ebo = 0;
	quad->last_attribute = 0;
	quad->instance_buffers_dirty = false;
}

#endif // SE_RENDER_BACKEND_OPENGL || SE_RENDER_BACKEND_GLES
