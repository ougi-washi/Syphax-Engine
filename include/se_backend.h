// Syphax-Engine - Ougi Washi

#ifndef SE_BACKEND_H
#define SE_BACKEND_H

#include "se_defines.h"

typedef enum {
	SE_BACKEND_RENDER_UNKNOWN = 0,
	SE_BACKEND_RENDER_OPENGL,
	SE_BACKEND_RENDER_GLES,
	SE_BACKEND_RENDER_WEBGL,
	SE_BACKEND_RENDER_METAL,
	SE_BACKEND_RENDER_VULKAN,
	SE_BACKEND_RENDER_SOFTWARE
} se_backend_render;

typedef enum {
	SE_BACKEND_PLATFORM_UNKNOWN = 0,
	SE_BACKEND_PLATFORM_DESKTOP_GLFW,
	SE_BACKEND_PLATFORM_ANDROID,
	SE_BACKEND_PLATFORM_IOS,
	SE_BACKEND_PLATFORM_WEB,
	SE_BACKEND_PLATFORM_TERMINAL
} se_backend_platform;

typedef enum {
	SE_SHADER_PROFILE_UNKNOWN = 0,
	SE_SHADER_PROFILE_GLSL_330,
	SE_SHADER_PROFILE_GLSL_ES_300,
	SE_SHADER_PROFILE_WGSL,
	SE_SHADER_PROFILE_METAL,
	SE_SHADER_PROFILE_SPIRV
} se_shader_profile;

typedef enum {
	SE_PROFILE_NONE = 0,
	SE_PROFILE_GLES3 = 1 << 0,
	SE_PROFILE_WEBGL2 = 1 << 1,
	SE_PROFILE_METAL = 1 << 2,
	SE_PROFILE_VULKAN = 1 << 3,
	SE_PROFILE_SOFTWARE = 1 << 4
} se_portability_profile;

typedef struct {
	se_backend_render render_backend;
	se_backend_platform platform_backend;
	se_shader_profile shader_profile;
	se_portability_profile portability_profile;
	b8 extension_apis_supported;
} se_backend_info;

typedef struct {
	b8 valid;
	b8 instancing;
	u32 max_mrt_count;
	b8 float_render_targets;
	b8 compute_available;
	u32 max_texture_size;
	u32 max_texture_units;
} se_capabilities;

extern se_backend_info se_get_backend_info(void);
extern se_portability_profile se_get_portability_profile(void);
extern se_capabilities se_capabilities_get(void);

#endif // SE_BACKEND_H
