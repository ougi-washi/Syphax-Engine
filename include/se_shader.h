// Syphax Engine - Ougi Washi

#ifndef SE_SHADER_H
#define SE_SHADER_H

#include "se.h"
#include "se_defines.h"
#include <time.h>

typedef enum {
	SE_UNIFORM_FLOAT,
	SE_UNIFORM_VEC2,
	SE_UNIFORM_VEC3,
	SE_UNIFORM_VEC4,
	SE_UNIFORM_INT,
	SE_UNIFORM_MAT3,
	SE_UNIFORM_MAT4,
	SE_UNIFORM_TEXTURE
} se_uniform_type;

typedef struct se_uniform {
	char name[SE_MAX_NAME_LENGTH];
	se_uniform_type type;
	union {
		f32 f;
		s_vec2 vec2;
		s_vec3 vec3;
		s_vec4 vec4;
		i32 i;
		s_mat3 mat3;
		s_mat4 mat4;
		u32 texture;
	} value;
} se_uniform;
typedef s_array(se_uniform, se_uniforms);

typedef struct se_shader {
	u32 program;
	u32 vertex_shader;
	u32 fragment_shader;
	c8 vertex_path[SE_MAX_PATH_LENGTH];
	c8 fragment_path[SE_MAX_PATH_LENGTH];
	time_t vertex_mtime;
	time_t fragment_mtime;
	se_uniforms uniforms;
	b8 needs_reload;
} se_shader;
typedef s_array(se_shader, se_shaders);
typedef se_shader *se_shader_ptr;
typedef s_array(se_shader_ptr, se_shaders_ptr);

extern se_shader *se_shader_load(se_context *ctx, const char *vs_path, const char *fs_path);
extern se_shader *se_shader_load_from_memory(se_context *ctx, const char *vs_data, const char *fs_data);
extern void se_shader_destroy(se_context *ctx, se_shader *shader);
extern b8 se_shader_reload_if_changed(se_shader *shader);
extern void se_shader_use(se_context *ctx, se_shader *shader, const b8 update_uniforms, const b8 update_global_uniforms);
extern i32 se_shader_get_uniform_location(se_shader *shader, const char *name);
extern f32 *se_shader_get_uniform_float(se_shader *shader, const char *name);
extern s_vec2 *se_shader_get_uniform_vec2(se_shader *shader, const char *name);
extern s_vec3 *se_shader_get_uniform_vec3(se_shader *shader, const char *name);
extern s_vec4 *se_shader_get_uniform_vec4(se_shader *shader, const char *name);
extern i32 *se_shader_get_uniform_int(se_shader *shader, const char *name);
extern s_mat4 *se_shader_get_uniform_mat4(se_shader *shader, const char *name);
extern u32 *se_shader_get_uniform_texture(se_shader *shader, const char *name);
extern void se_shader_set_float(se_shader *shader, const char *name, f32 value);
extern void se_shader_set_vec2(se_shader *shader, const char *name, const s_vec2 *value);
extern void se_shader_set_vec3(se_shader *shader, const char *name, const s_vec3 *value);
extern void se_shader_set_vec4(se_shader *shader, const char *name, const s_vec4 *value);
extern void se_shader_set_int(se_shader *shader, const char *name, i32 value);
extern void se_shader_set_mat3(se_shader *shader, const char *name, const s_mat3 *value);
extern void se_shader_set_mat4(se_shader *shader, const char *name, const s_mat4 *value);
extern void se_shader_set_texture(se_shader *shader, const char *name, const u32 texture);
extern void se_context_reload_changed_shaders(se_context *ctx);
extern se_uniforms *se_context_get_global_uniforms(se_context *ctx);

// Uniform functions
extern void se_uniform_set_float(se_uniforms *uniforms, const char *name, f32 value);
extern void se_uniform_set_vec2(se_uniforms *uniforms, const char *name, const s_vec2 *value);
extern void se_uniform_set_vec3(se_uniforms *uniforms, const char *name, const s_vec3 *value);
extern void se_uniform_set_vec4(se_uniforms *uniforms, const char *name, const s_vec4 *value);
extern void se_uniform_set_int(se_uniforms *uniforms, const char *name, i32 value);
extern void se_uniform_set_mat3(se_uniforms *uniforms, const char *name, const s_mat3 *value);
extern void se_uniform_set_mat4(se_uniforms *uniforms, const char *name, const s_mat4 *value);
extern void se_uniform_set_texture(se_uniforms *uniforms, const char *name, const u32 texture);
extern void se_uniform_apply(se_context *ctx, se_shader *shader, const b8 update_global_uniforms);

#endif // SE_SHADER_H
