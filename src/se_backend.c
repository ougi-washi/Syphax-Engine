// Syphax-Engine - Ougi Washi

#include "se_backend.h"
#include "se_render.h"

#if defined(SE_RENDER_BACKEND_OPENGL) || defined(SE_RENDER_BACKEND_GLES)
#include "render/se_gl.h"
#include <stdio.h>
#include <string.h>
#endif

static se_backend_render se_backend_render_current(void) {
#if defined(SE_RENDER_BACKEND_OPENGL)
	return SE_BACKEND_RENDER_OPENGL;
#elif defined(SE_RENDER_BACKEND_GLES)
	return SE_BACKEND_RENDER_GLES;
#elif defined(SE_RENDER_BACKEND_VULKAN)
	return SE_BACKEND_RENDER_VULKAN;
#else
	return SE_BACKEND_RENDER_UNKNOWN;
#endif
}

static se_backend_platform se_backend_platform_current(void) {
#if defined(SE_WINDOW_BACKEND_GLFW)
	return SE_BACKEND_PLATFORM_DESKTOP_GLFW;
#elif defined(SE_WINDOW_BACKEND_TERMINAL)
	return SE_BACKEND_PLATFORM_TERMINAL;
#elif defined(SE_WINDOW_BACKEND_WEB)
	return SE_BACKEND_PLATFORM_WEB;
#else
	return SE_BACKEND_PLATFORM_UNKNOWN;
#endif
}

static se_shader_profile se_backend_shader_profile_current(void) {
#if defined(SE_RENDER_BACKEND_OPENGL)
	return SE_SHADER_PROFILE_GLSL_330;
#elif defined(SE_RENDER_BACKEND_GLES)
	return SE_SHADER_PROFILE_GLSL_ES_300;
#else
	return SE_SHADER_PROFILE_UNKNOWN;
#endif
}

static se_portability_profile se_backend_portability_profile_current(void) {
#if defined(SE_RENDER_BACKEND_OPENGL)
	return SE_PROFILE_GLES3;
#elif defined(SE_RENDER_BACKEND_GLES)
	return (se_portability_profile)(SE_PROFILE_GLES3 | SE_PROFILE_WEBGL2);
#else
	return SE_PROFILE_NONE;
#endif
}

se_backend_info se_get_backend_info(void) {
	se_backend_info info = {0};
	info.render_backend = se_backend_render_current();
	info.platform_backend = se_backend_platform_current();
	info.shader_profile = se_backend_shader_profile_current();
	info.portability_profile = se_backend_portability_profile_current();
	info.extension_apis_supported = true;
	return info;
}

se_portability_profile se_get_portability_profile(void) {
	return se_backend_portability_profile_current();
}

#if defined(SE_RENDER_BACKEND_OPENGL) || defined(SE_RENDER_BACKEND_GLES)
static void se_parse_gl_version(const c8 *version, i32 *out_major, i32 *out_minor) {
	s_assertf(out_major, "se_parse_gl_version :: out_major is null");
	s_assertf(out_minor, "se_parse_gl_version :: out_minor is null");
	*out_major = 0;
	*out_minor = 0;
	if (!version) {
		return;
	}
#if defined(SE_RENDER_BACKEND_GLES)
	if (sscanf(version, "OpenGL ES %d.%d", out_major, out_minor) == 2) {
		return;
	}
#endif
	(void)sscanf(version, "%d.%d", out_major, out_minor);
}
#endif

se_capabilities se_capabilities_get(void) {
	se_capabilities caps = {0};
#if defined(SE_RENDER_BACKEND_OPENGL) || defined(SE_RENDER_BACKEND_GLES)
	if (!se_render_has_context()) {
		return caps;
	}
	caps.valid = true;
	caps.instancing = true;

	GLint max_color_attachments = 1;
	GLint max_texture_size = 0;
	GLint max_texture_units = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units);
	if (max_color_attachments < 1) {
		max_color_attachments = 1;
	}
	if (max_texture_size < 0) {
		max_texture_size = 0;
	}
	if (max_texture_units < 0) {
		max_texture_units = 0;
	}
	caps.max_mrt_count = (u32)max_color_attachments;
	caps.max_texture_size = (u32)max_texture_size;
	caps.max_texture_units = (u32)max_texture_units;

#if defined(SE_RENDER_BACKEND_OPENGL)
	caps.float_render_targets = true;
#else
	const c8 *extensions = (const c8 *)glGetString(GL_EXTENSIONS);
	caps.float_render_targets =
		extensions &&
		(strstr(extensions, "EXT_color_buffer_float") ||
		 strstr(extensions, "OES_texture_float"));
#endif

	const c8 *version = (const c8 *)glGetString(GL_VERSION);
	i32 major = 0;
	i32 minor = 0;
	se_parse_gl_version(version, &major, &minor);
#if defined(SE_RENDER_BACKEND_GLES)
	caps.compute_available = major > 3 || (major == 3 && minor >= 1);
#else
	caps.compute_available = major > 4 || (major == 4 && minor >= 3);
#endif
#else
	caps.valid = false;
#endif
	return caps;
}
