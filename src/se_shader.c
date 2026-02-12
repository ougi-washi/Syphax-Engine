// Syphax Engine - Ougi Washi

#include "se_shader.h"
#include "render/se_gl.h"
#include "syphax/s_files.h"

#define SE_UNIFORMS_MAX 128

static GLuint se_shader_compile(const char *source, GLenum type);
static GLuint se_shader_create_program(const char *vertex_source, const char *fragment_source);
static void se_shader_cleanup(se_shader *shader);

static GLuint se_shader_compile(const char *source, GLenum type) {
	if (source == NULL || source[0] == '\0') {
		return 0;
	}

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512] = {0};
		glGetShaderInfoLog(shader, 512, NULL, info_log);
		fprintf(stderr, "se_shader_compile :: shader compilation failed: %s\n", info_log);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint se_shader_create_program(const char *vertex_source, const char *fragment_source) {
	GLuint vertex_shader = se_shader_compile(vertex_source, GL_VERTEX_SHADER);
	GLuint fragment_shader = se_shader_compile(fragment_source, GL_FRAGMENT_SHADER);
	if (vertex_shader == 0 || fragment_shader == 0) {
		if (vertex_shader != 0) {
			glDeleteShader(vertex_shader);
		}
		if (fragment_shader != 0) {
			glDeleteShader(fragment_shader);
		}
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char info_log[512] = {0};
		glGetProgramInfoLog(program, 512, NULL, info_log);
		fprintf(stderr, "se_shader_create_program :: shader program linking failed: %s\n", info_log);
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program;
}

b8 se_shader_load_internal(se_shader *shader) {

	assert(shader);

	se_shader_cleanup(shader);

	char *vertex_source = NULL;
	char *fragment_source = NULL;

	if (!s_file_read(shader->vertex_path, &vertex_source, NULL) ||
		!s_file_read(shader->fragment_path, &fragment_source, NULL)) {
		free(vertex_source);
		free(fragment_source);
		return false;
	}
	shader->program = se_shader_create_program(vertex_source, fragment_source);
	free(vertex_source);
	free(fragment_source);

	if (!shader->program) {
	return false;
	}

	shader->vertex_mtime = 0;
	shader->fragment_mtime = 0;
	s_file_mtime(shader->vertex_path, &shader->vertex_mtime);
	s_file_mtime(shader->fragment_path, &shader->fragment_mtime);
	s_array_init(&shader->uniforms, SE_UNIFORMS_MAX);
	printf("se_shader_load_internal :: created program: %d, from %s, %s\n", shader->program, shader->vertex_path, shader->fragment_path);
	return true;
}

se_shader *se_shader_load(se_context *ctx, const char *vertex_file_path, const char *fragment_file_path) {
	if (!ctx || !vertex_file_path || !fragment_file_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_capacity(&ctx->shaders) <= s_array_get_size(&ctx->shaders)) {
		const sz current_capacity = s_array_get_capacity(&ctx->shaders);
		const sz next_capacity = (current_capacity == 0) ? 2 : (current_capacity + 2);
		s_array_resize(&ctx->shaders, next_capacity);
	}
	se_shader *new_shader = s_array_increment(&ctx->shaders);
	memset(new_shader, 0, sizeof(*new_shader));

	// make path absolute
	if (strlen(vertex_file_path) > 0) {
		if (!se_paths_resolve_resource_path(new_shader->vertex_path, SE_MAX_PATH_LENGTH, vertex_file_path)) {
			s_array_remove_at(&ctx->shaders, s_array_get_size(&ctx->shaders) - 1);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return NULL;
		}
	} else {
		new_shader->vertex_path[0] = '\0';
	}

	if (strlen(fragment_file_path) > 0) {
		if (!se_paths_resolve_resource_path(new_shader->fragment_path, SE_MAX_PATH_LENGTH, fragment_file_path)) {
			s_array_remove_at(&ctx->shaders, s_array_get_size(&ctx->shaders) - 1);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return NULL;
		}
	} else {
		new_shader->fragment_path[0] = '\0';
	}

	if (se_shader_load_internal(new_shader)) {
		se_set_last_error(SE_RESULT_OK);
		return new_shader;
	}
	s_array_remove_at(&ctx->shaders, s_array_get_size(&ctx->shaders) - 1);
	se_set_last_error(SE_RESULT_IO);
	return NULL;
}

se_shader *se_shader_load_from_memory(se_context *ctx, const char *vertex_source, const char *fragment_source) {
	if (!ctx || !vertex_source || !fragment_source) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	if (s_array_get_capacity(&ctx->shaders) <= s_array_get_size(&ctx->shaders)) {
		const sz current_capacity = s_array_get_capacity(&ctx->shaders);
		const sz next_capacity = (current_capacity == 0) ? 2 : (current_capacity + 2);
		s_array_resize(&ctx->shaders, next_capacity);
	}

	se_shader *new_shader = s_array_increment(&ctx->shaders);

	memset(new_shader, 0, sizeof(*new_shader));
	new_shader->program = se_shader_create_program(vertex_source, fragment_source);
	if (new_shader->program == 0) {
		s_array_remove_at(&ctx->shaders, s_array_get_size(&ctx->shaders) - 1);
		se_set_last_error(SE_RESULT_IO);
		return NULL;
	}

	s_array_init(&new_shader->uniforms, SE_UNIFORMS_MAX);
	new_shader->vertex_path[0] = '\0';
	new_shader->fragment_path[0] = '\0';
	new_shader->vertex_mtime = 0;
	new_shader->fragment_mtime = 0;
	new_shader->needs_reload = false;
	se_set_last_error(SE_RESULT_OK);
	return new_shader;
}

void se_shader_destroy(se_context *ctx, se_shader *shader) {
	s_assertf(ctx, "se_shader_destroy :: ctx is null");
	s_assertf(shader, "se_shader_destroy :: shader is null");
	se_shader_cleanup(shader);
	for (sz i = 0; i < s_array_get_size(&ctx->shaders); i++) {
		se_shader *slot = s_array_get(&ctx->shaders, i);
		if (slot == shader) {
			s_array_remove_at(&ctx->shaders, i);
			break;
		}
	}
}

b8 se_shader_reload_if_changed(se_shader *shader) {
	if (shader == NULL) {
		return false;
	}
	if (strlen(shader->vertex_path) == 0 || strlen(shader->fragment_path) == 0) {
	return false;
	}

	time_t vertex_mtime = 0;
	time_t fragment_mtime = 0;
	s_file_mtime(shader->vertex_path, &vertex_mtime);
	s_file_mtime(shader->fragment_path, &fragment_mtime);

	if (vertex_mtime != shader->vertex_mtime ||
		fragment_mtime != shader->fragment_mtime) {
	printf("se_shader_reload_if_changed :: Reloading shader: %s, %s\n", shader->vertex_path, shader->fragment_path);
	return se_shader_load_internal(shader);
	}

	return false;
}

void se_shader_use(se_context *ctx, se_shader *shader, const b8 update_uniforms, const b8 update_global_uniforms) {
	glUseProgram(shader->program);
	if (update_uniforms) {
	se_uniform_apply(ctx, shader, update_global_uniforms);
	}
}

static void se_shader_cleanup(se_shader *shader) {
	s_array_clear(&shader->uniforms);
	printf("se_shader_cleanup :: shader: %p\n", shader);
	if (shader->program) {
	glDeleteProgram(shader->program);
	shader->program = 0;
	}
}

i32 se_shader_get_uniform_location(se_shader *shader, const char *name) {
	return (i32)glGetUniformLocation(shader->program, name);
}

f32 *se_shader_get_uniform_float(se_shader *shader, const char *name) {
	s_foreach(&shader->uniforms, i) {
		se_uniform *uniform = s_array_get(&shader->uniforms, i);
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.f;
		}
	}
	return NULL;
}

s_vec2 *se_shader_get_uniform_vec2(se_shader *shader, const char *name) {
	s_foreach(&shader->uniforms, i) {
		se_uniform *uniform = s_array_get(&shader->uniforms, i);
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.vec2;
		}
	}
	return NULL;
}

s_vec3 *se_shader_get_uniform_vec3(se_shader *shader, const char *name) {
	s_foreach(&shader->uniforms, i) {
		se_uniform *uniform = s_array_get(&shader->uniforms, i);
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.vec3;
		}
	}
	return NULL;
}

s_vec4 *se_shader_get_uniform_vec4(se_shader *shader, const char *name) {
	s_foreach(&shader->uniforms, i) {
		se_uniform *uniform = s_array_get(&shader->uniforms, i);
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.vec4;
		}
	}
	return NULL;
}

i32 *se_shader_get_uniform_int(se_shader *shader, const char *name) {
	s_foreach(&shader->uniforms, i) {
		se_uniform *uniform = s_array_get(&shader->uniforms, i);
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.i;
		}
	}
	return NULL;
}

s_mat4 *se_shader_get_uniform_mat4(se_shader *shader, const char *name) {
	s_foreach(&shader->uniforms, i) {
		se_uniform *uniform = s_array_get(&shader->uniforms, i);
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.mat4;
		}
	}
	return NULL;
}

u32 *se_shader_get_uniform_texture(se_shader *shader, const char *name) {
	s_foreach(&shader->uniforms, i) {
		se_uniform *uniform = s_array_get(&shader->uniforms, i);
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.texture;
		}
	}
	return NULL;
}

void se_shader_set_float(se_shader *shader, const char *name, f32 value) {
	se_uniform_set_float(&shader->uniforms, name, value);
}

void se_shader_set_vec2(se_shader *shader, const char *name, const s_vec2 *value) {
	se_uniform_set_vec2(&shader->uniforms, name, value);
}

void se_shader_set_vec3(se_shader *shader, const char *name, const s_vec3 *value) {
	se_uniform_set_vec3(&shader->uniforms, name, value);
}

void se_shader_set_vec4(se_shader *shader, const char *name, const s_vec4 *value) {
	se_uniform_set_vec4(&shader->uniforms, name, value);
}

void se_shader_set_int(se_shader *shader, const char *name, i32 value) {
	se_uniform_set_int(&shader->uniforms, name, value);
}

void se_shader_set_mat3(se_shader *shader, const char *name, const s_mat3 *value) {
	se_uniform_set_mat3(&shader->uniforms, name, value);
}

void se_shader_set_mat4(se_shader *shader, const char *name, const s_mat4 *value) {
	se_uniform_set_mat4(&shader->uniforms, name, value);
}

void se_shader_set_texture(se_shader *shader, const char *name, const u32 texture) {
	se_uniform_set_texture(&shader->uniforms, name, texture);
}

void se_context_reload_changed_shaders(se_context *ctx) {
	if (ctx == NULL) {
		return;
	}

	s_foreach(&ctx->shaders, i) {
		se_shader *shader = s_array_get(&ctx->shaders, i);
		if (shader == NULL) {
			continue;
		}
		(void)se_shader_reload_if_changed(shader);
	}
}

se_uniforms *se_context_get_global_uniforms(se_context *ctx) {
	if (ctx == NULL) {
		return NULL;
	}
	return (se_uniforms *)&ctx->global_uniforms;
}

//static void se_render_buffer_cleanup(se_render_buffer *buffer) {
//	printf("se_render_buffer_cleanup :: buffer: %p\n", buffer);
//	if (buffer->framebuffer) {
//	glDeleteFramebuffers(1, &buffer->framebuffer);
//	buffer->framebuffer = 0;
//	}
//	if (buffer->prev_framebuffer) {
//		glDeleteFramebuffers(1, &buffer->prev_framebuffer);
//		buffer->prev_framebuffer = 0;
//	}
//	if (buffer->texture) {
//	glDeleteTextures(1, &buffer->texture);
//	buffer->texture = 0;
//	}
//	if (buffer->prev_texture) {
//		glDeleteTextures(1, &buffer->prev_texture);
//		buffer->prev_texture = 0;
//	}
//	if (buffer->depth_buffer) {
//	glDeleteRenderbuffers(1, &buffer->depth_buffer);
//	buffer->depth_buffer = 0;
//	}
//}

// Uniform functions
void se_uniform_set_float(se_uniforms *uniforms, const char *name, f32 value) {
	s_foreach(uniforms, i) {
	se_uniform *found_uniform = s_array_get(uniforms, i);
	if (found_uniform && strcmp(found_uniform->name, name) == 0) {
		found_uniform->type = SE_UNIFORM_FLOAT;
		found_uniform->value.f = value;
		return;
	}
	}
	se_uniform *new_uniform = s_array_increment(uniforms);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_FLOAT;
	new_uniform->value.f = value;
}

void se_uniform_set_vec2(se_uniforms *uniforms, const char *name, const s_vec2 *value) {
	s_foreach(uniforms, i) {
	se_uniform *found_uniform = s_array_get(uniforms, i);
	if (found_uniform && strcmp(found_uniform->name, name) == 0) {
		found_uniform->type = SE_UNIFORM_VEC2;
		memcpy(&found_uniform->value.vec2, value, sizeof(s_vec2));
		return;
	}
	}
	se_uniform *new_uniform = s_array_increment(uniforms);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_VEC2;
	memcpy(&new_uniform->value.vec2, value, sizeof(s_vec2));
}

void se_uniform_set_vec3(se_uniforms *uniforms, const char *name, const s_vec3 *value) {
	s_assertf(uniforms, "se_uniform_set_vec3 :: uniforms is null");
	s_assertf(name, "se_uniform_set_vec3 :: name is null");
	s_assertf(value, "se_uniform_set_vec3 :: value is null");
	s_foreach(uniforms, i) {
	se_uniform *found_uniform = s_array_get(uniforms, i);
	if (found_uniform && strcmp(found_uniform->name, name) == 0) {
		found_uniform->type = SE_UNIFORM_VEC3;
		memcpy(&found_uniform->value.vec3, value, sizeof(s_vec3));
		return;
	}
	}
	se_uniform *new_uniform = s_array_increment(uniforms);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_VEC3;
	memcpy(&new_uniform->value.vec3, value, sizeof(s_vec3));
}

void se_uniform_set_vec4(se_uniforms *uniforms, const char *name, const s_vec4 *value) {
	s_foreach(uniforms, i) {
	se_uniform *found_uniform = s_array_get(uniforms, i);
	if (found_uniform && strcmp(found_uniform->name, name) == 0) {
		found_uniform->type = SE_UNIFORM_VEC4;
		memcpy(&found_uniform->value.vec4, value, sizeof(s_vec4));
		return;
	}
	}
	se_uniform *new_uniform = s_array_increment(uniforms);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_VEC4;
	memcpy(&new_uniform->value.vec4, value, sizeof(s_vec4));
}

void se_uniform_set_int(se_uniforms *uniforms, const char *name, i32 value) {
	s_foreach(uniforms, i) {
	se_uniform *found_uniform = s_array_get(uniforms, i);
	if (found_uniform && strcmp(found_uniform->name, name) == 0) {
		found_uniform->type = SE_UNIFORM_INT;
		found_uniform->value.i = value;
		return;
	}
	}
	se_uniform *new_uniform = s_array_increment(uniforms);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_INT;
	new_uniform->value.i = value;
}

void se_uniform_set_mat3(se_uniforms *uniforms, const char *name, const s_mat3 *value) {
	s_foreach(uniforms, i) {
		se_uniform *found_uniform = s_array_get(uniforms, i);
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_MAT3;
			memcpy(&found_uniform->value.mat3, value, sizeof(s_mat3));
			return;
		}
	}
	se_uniform *new_uniform = s_array_increment(uniforms);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_MAT3;
	memcpy(&new_uniform->value.mat3, value, sizeof(s_mat3));
}

void se_uniform_set_mat4(se_uniforms *uniforms, const char *name, const s_mat4 *value) {
	s_foreach(uniforms, i) {
		se_uniform *found_uniform = s_array_get(uniforms, i);
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_MAT4;
			memcpy(&found_uniform->value.mat4, value, sizeof(s_mat4));
			return;
		}
	}
	se_uniform *new_uniform = s_array_increment(uniforms);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_MAT4;
	memcpy(&new_uniform->value.mat4, value, sizeof(s_mat4));
}

void se_uniform_set_texture(se_uniforms *uniforms, const char *name, const u32 texture) {
	s_assertf(uniforms, "se_uniform_set_texture :: uniforms is null");
	s_assertf(name, "se_uniform_set_texture :: name is null");
	s_foreach(uniforms, i) {
	se_uniform *found_uniform = s_array_get(uniforms, i);
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_TEXTURE;
			found_uniform->value.texture = texture;
			return;
		}
	}
	se_uniform *new_uniform = s_array_increment(uniforms);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_TEXTURE;
	new_uniform->value.texture = texture;
	printf("se_uniform_set_texture :: added texture uniform: %s, id: %d, ptr: %p\n", name, texture, new_uniform);
}

void se_uniform_apply(se_context *ctx, se_shader *shader, const b8 update_global_uniforms) {
	glUseProgram(shader->program);
	u32 texture_unit = 0;
	s_foreach(&shader->uniforms, i) {
	se_uniform *uniform = s_array_get(&shader->uniforms, i);
	GLint location = glGetUniformLocation(shader->program, uniform->name);
	if (location == -1) {
		continue;
	};
	switch (uniform->type) {
		case SE_UNIFORM_FLOAT:
			glUniform1fv(location, 1, &uniform->value.f);
			break;
		case SE_UNIFORM_VEC2:
			glUniform2fv(location, 1, &uniform->value.vec2.x);
			break;
		case SE_UNIFORM_VEC3:
			glUniform3fv(location, 1, &uniform->value.vec3.x);
			break;
		case SE_UNIFORM_VEC4:
			glUniform4fv(location, 1, &uniform->value.vec4.x);
			break;
		case SE_UNIFORM_INT:
			glUniform1i(location, uniform->value.i);
			break;
		case SE_UNIFORM_MAT3:
			glUniformMatrix3fv(location, 1, GL_FALSE, uniform->value.mat3.m[0]);
			break;
		case SE_UNIFORM_MAT4:
			glUniformMatrix4fv(location, 1, GL_FALSE, uniform->value.mat4.m[0]);
			break;
		case SE_UNIFORM_TEXTURE:
			glActiveTexture(GL_TEXTURE0 + texture_unit);
			glBindTexture(GL_TEXTURE_2D, uniform->value.texture);
			glUniform1i(location, texture_unit);
			texture_unit++;
			break;
		}
	}

	if (!update_global_uniforms) {
		return;
	}

	// apply global uniforms
	s_foreach(&ctx->global_uniforms, i) {
	se_uniform *uniform = s_array_get(&ctx->global_uniforms, i);
	GLint location = glGetUniformLocation(shader->program, uniform->name);
	if (location == -1) {
		continue;
	};
	switch (uniform->type) {
		case SE_UNIFORM_FLOAT:
			glUniform1fv(location, 1, &uniform->value.f);
			break;
		case SE_UNIFORM_VEC2:
			glUniform2fv(location, 1, &uniform->value.vec2.x);
			break;
		case SE_UNIFORM_VEC3:
			glUniform3fv(location, 1, &uniform->value.vec3.x);
			break;
		case SE_UNIFORM_VEC4:
			glUniform4fv(location, 1, &uniform->value.vec4.x);
			break;
		case SE_UNIFORM_INT:
			glUniform1i(location, uniform->value.i);
			break;
		case SE_UNIFORM_MAT3:
			glUniformMatrix3fv(location, 1, GL_FALSE, uniform->value.mat3.m[0]);
			break;
		case SE_UNIFORM_MAT4:
			glUniformMatrix4fv(location, 1, GL_FALSE, uniform->value.mat4.m[0]);
			break;
		case SE_UNIFORM_TEXTURE:
			glActiveTexture(GL_TEXTURE0 + texture_unit);
			glBindTexture(GL_TEXTURE_2D, uniform->value.texture);
			glUniform1i(location, texture_unit);
			texture_unit++;
			break;
		}
	}
}
