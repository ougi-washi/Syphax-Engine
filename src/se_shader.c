// Syphax Engine - Ougi Washi

#include "se_shader.h"
#include "se_debug.h"
#include "se_render.h"
#include "render/se_gl.h"
#include "syphax/s_files.h"

#define SE_UNIFORMS_MAX 128

static GLuint se_shader_compile(const char *source, GLenum type);
static GLuint se_shader_create_program(const char *vertex_source, const char *fragment_source);
static void se_shader_cleanup(se_shader *shader);
static char *se_shader_prepare_source(const char *source, GLenum type);
static const char *se_shader_skip_bom(const char *source);
static const char *se_shader_get_line_end(const char *line_start);
static const char *se_shader_get_body_after_version(const char *source);
static b8 se_shader_line_is_precision(const char *line_start, const char *line_end);
#if defined(SE_RENDER_BACKEND_GLES)
static b8 se_shader_has_precision_decl(const char *source);
static char *se_shader_translate_gl330_to_es300(const char *source, const GLenum type);
#else
static char *se_shader_translate_es300_to_gl330(const char *source);
#endif
static se_shader* se_shader_from_handle(se_context *ctx, const se_shader_handle shader) {
	return s_array_get(&ctx->shaders, shader);
}

static const char *se_shader_skip_bom(const char *source) {
	if (!source) {
		return source;
	}
	if ((u8)source[0] == 0xEF && (u8)source[1] == 0xBB && (u8)source[2] == 0xBF) {
		return source + 3;
	}
	return source;
}

static const char *se_shader_get_line_end(const char *line_start) {
	const char *line_end = strchr(line_start, '\n');
	if (line_end) {
		return line_end;
	}
	return line_start + strlen(line_start);
}

static const char *se_shader_get_body_after_version(const char *source) {
	const char *line_end = se_shader_get_line_end(source);
	if (*line_end == '\n') {
		return line_end + 1;
	}
	return line_end;
}

static b8 se_shader_line_is_precision(const char *line_start, const char *line_end) {
	const char *cursor = line_start;
	while (cursor < line_end && (*cursor == ' ' || *cursor == '\t' || *cursor == '\r')) {
		cursor++;
	}
	return (sz)(line_end - cursor) >= 9 && strncmp(cursor, "precision", 9) == 0;
}

#if defined(SE_RENDER_BACKEND_GLES)
static b8 se_shader_has_precision_decl(const char *source) {
	const char *cursor = source;
	while (cursor && *cursor) {
		const char *line_end = se_shader_get_line_end(cursor);
		if (se_shader_line_is_precision(cursor, line_end)) {
			return true;
		}
		if (*line_end == '\n') {
			cursor = line_end + 1;
		} else {
			break;
		}
	}
	return false;
}
#endif

#if !defined(SE_RENDER_BACKEND_GLES)
static char *se_shader_translate_es300_to_gl330(const char *source) {
	static const char *gl_header = "#version 330 core\n";
	const char *body = se_shader_get_body_after_version(source);
	const sz source_size = strlen(source);
	const sz header_size = strlen(gl_header);
	char *translated = malloc(source_size + header_size + 1);
	if (!translated) {
		return NULL;
	}
	sz write = 0;
	memcpy(translated + write, gl_header, header_size);
	write += header_size;

	const char *cursor = body;
	while (cursor && *cursor) {
		const char *line_end = se_shader_get_line_end(cursor);
		const sz line_size = (sz)(line_end - cursor);
		if (!se_shader_line_is_precision(cursor, line_end)) {
			memcpy(translated + write, cursor, line_size);
			write += line_size;
			if (*line_end == '\n') {
				translated[write++] = '\n';
			}
		}
		if (*line_end == '\n') {
			cursor = line_end + 1;
		} else {
			break;
		}
	}
	translated[write] = '\0';
	return translated;
}
#endif

#if defined(SE_RENDER_BACKEND_GLES)
static char *se_shader_translate_gl330_to_es300(const char *source, const GLenum type) {
	static const char *es_header = "#version 300 es\n";
	static const char *frag_precision = "precision mediump float;\nprecision mediump int;\n";
	const char *body = se_shader_get_body_after_version(source);
	const b8 needs_precision =
		(type == GL_FRAGMENT_SHADER) && !se_shader_has_precision_decl(body);
	const sz source_size = strlen(source);
	const sz header_size = strlen(es_header);
	const sz precision_size = needs_precision ? strlen(frag_precision) : 0;
	char *translated = malloc(source_size + header_size + precision_size + 1);
	if (!translated) {
		return NULL;
	}
	sz write = 0;
	memcpy(translated + write, es_header, header_size);
	write += header_size;
	if (needs_precision) {
		memcpy(translated + write, frag_precision, precision_size);
		write += precision_size;
	}
	memcpy(translated + write, body, strlen(body));
	write += strlen(body);
	translated[write] = '\0';
	return translated;
}
#endif

static char *se_shader_prepare_source(const char *source, GLenum type) {
	const char *clean_source = se_shader_skip_bom(source);
	if (!clean_source || clean_source[0] == '\0') {
		return NULL;
	}
#if defined(SE_RENDER_BACKEND_GLES)
	if (strncmp(clean_source, "#version 330 core", 17) == 0) {
		return se_shader_translate_gl330_to_es300(clean_source, type);
	}
	if (type == GL_FRAGMENT_SHADER && strncmp(clean_source, "#version 300 es", 15) == 0) {
		const char *body = se_shader_get_body_after_version(clean_source);
		if (!se_shader_has_precision_decl(body)) {
			return se_shader_translate_gl330_to_es300(clean_source, type);
		}
	}
#else
	if (strncmp(clean_source, "#version 300 es", 15) == 0) {
		return se_shader_translate_es300_to_gl330(clean_source);
	}
#endif
	return NULL;
}

static GLuint se_shader_compile(const char *source, GLenum type) {
	if (source == NULL || source[0] == '\0') {
		return 0;
	}

	char *translated_source = se_shader_prepare_source(source, type);
	const char *compile_source = translated_source ? translated_source : source;

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &compile_source, NULL);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512] = {0};
		glGetShaderInfoLog(shader, 512, NULL, info_log);
		se_log("se_shader_compile :: shader compilation failed: %s", info_log);
		glDeleteShader(shader);
		free(translated_source);
		return 0;
	}
	free(translated_source);

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
		se_log("se_shader_create_program :: shader program linking failed: %s", info_log);
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program;
}

b8 se_shader_load_internal(se_shader *shader) {

	assert(shader);
	if (!se_render_has_context()) {
		return false;
	}

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
	s_array_init(&shader->uniforms);
	se_log("se_shader_load_internal :: created program: %d, from %s, %s", shader->program, shader->vertex_path, shader->fragment_path);
	return true;
}

se_shader_handle se_shader_load(const char *vertex_file_path, const char *fragment_file_path) {
	se_context *ctx = se_current_context();
	if (!ctx || !vertex_file_path || !fragment_file_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (!se_render_has_context()) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->shaders) == 0) {
		s_array_init(&ctx->shaders);
	}
	se_shader_handle shader_handle = s_array_increment(&ctx->shaders);
	se_shader *new_shader = s_array_get(&ctx->shaders, shader_handle);
	memset(new_shader, 0, sizeof(*new_shader));

	// make path absolute
	if (strlen(vertex_file_path) > 0) {
		if (!se_paths_resolve_resource_path(new_shader->vertex_path, SE_MAX_PATH_LENGTH, vertex_file_path)) {
			s_array_remove(&ctx->shaders, shader_handle);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return S_HANDLE_NULL;
		}
	} else {
		new_shader->vertex_path[0] = '\0';
	}

	if (strlen(fragment_file_path) > 0) {
		if (!se_paths_resolve_resource_path(new_shader->fragment_path, SE_MAX_PATH_LENGTH, fragment_file_path)) {
			s_array_remove(&ctx->shaders, shader_handle);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return S_HANDLE_NULL;
		}
	} else {
		new_shader->fragment_path[0] = '\0';
	}

	if (se_shader_load_internal(new_shader)) {
		se_set_last_error(SE_RESULT_OK);
		return shader_handle;
	}
	s_array_remove(&ctx->shaders, shader_handle);
	se_set_last_error(SE_RESULT_IO);
	return S_HANDLE_NULL;
}

se_shader_handle se_shader_load_from_memory(const char *vertex_source, const char *fragment_source) {
	se_context *ctx = se_current_context();
	if (!ctx || !vertex_source || !fragment_source) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (!se_render_has_context()) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->shaders) == 0) {
		s_array_init(&ctx->shaders);
	}
	se_shader_handle shader_handle = s_array_increment(&ctx->shaders);
	se_shader *new_shader = s_array_get(&ctx->shaders, shader_handle);

	memset(new_shader, 0, sizeof(*new_shader));
	new_shader->program = se_shader_create_program(vertex_source, fragment_source);
	if (new_shader->program == 0) {
		s_array_remove(&ctx->shaders, shader_handle);
		se_set_last_error(SE_RESULT_IO);
		return S_HANDLE_NULL;
	}

	s_array_init(&new_shader->uniforms);
	new_shader->vertex_path[0] = '\0';
	new_shader->fragment_path[0] = '\0';
	new_shader->vertex_mtime = 0;
	new_shader->fragment_mtime = 0;
	new_shader->needs_reload = false;
	se_set_last_error(SE_RESULT_OK);
	return shader_handle;
}

void se_shader_destroy(const se_shader_handle shader) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_shader_destroy :: ctx is null");
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	s_assertf(shader_ptr, "se_shader_destroy :: shader is null");
	se_shader_cleanup(shader_ptr);
	s_array_remove(&ctx->shaders, shader);
}

b8 se_shader_reload_if_changed(const se_shader_handle shader) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	if (shader_ptr == NULL) {
		return false;
	}
	if (strlen(shader_ptr->vertex_path) == 0 || strlen(shader_ptr->fragment_path) == 0) {
	return false;
	}

	time_t vertex_mtime = 0;
	time_t fragment_mtime = 0;
	s_file_mtime(shader_ptr->vertex_path, &vertex_mtime);
	s_file_mtime(shader_ptr->fragment_path, &fragment_mtime);

	if (vertex_mtime != shader_ptr->vertex_mtime ||
		fragment_mtime != shader_ptr->fragment_mtime) {
	se_log("se_shader_reload_if_changed :: reloading shader: %s, %s", shader_ptr->vertex_path, shader_ptr->fragment_path);
	return se_shader_load_internal(shader_ptr);
	}

	return false;
}

void se_shader_use(const se_shader_handle shader, const b8 update_uniforms, const b8 update_global_uniforms) {
	se_context *ctx = se_current_context();
	if (!ctx || !se_render_has_context()) {
		return;
	}
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	if (!shader_ptr || shader_ptr->program == 0) {
		return;
	}
	glUseProgram(shader_ptr->program);
	if (update_uniforms) {
		se_uniform_apply(shader, update_global_uniforms);
	}
}

static void se_shader_cleanup(se_shader *shader) {
	s_array_clear(&shader->uniforms);
	se_log("se_shader_cleanup :: shader: %p", (void*)shader);
	if (shader->program) {
	glDeleteProgram(shader->program);
	shader->program = 0;
	}
}

i32 se_shader_get_uniform_location(const se_shader_handle shader, const char *name) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	return (i32)glGetUniformLocation(shader_ptr->program, name);
}

f32 *se_shader_get_uniform_float(const se_shader_handle shader, const char *name) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	for (sz i = 0; i < s_array_get_size(&shader_ptr->uniforms); ++i) {
		se_uniform *uniform = s_array_get(&shader_ptr->uniforms, s_array_handle(&shader_ptr->uniforms, (u32)i));
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.f;
		}
	}
	return NULL;
}

s_vec2 *se_shader_get_uniform_vec2(const se_shader_handle shader, const char *name) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	for (sz i = 0; i < s_array_get_size(&shader_ptr->uniforms); ++i) {
		se_uniform *uniform = s_array_get(&shader_ptr->uniforms, s_array_handle(&shader_ptr->uniforms, (u32)i));
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.vec2;
		}
	}
	return NULL;
}

s_vec3 *se_shader_get_uniform_vec3(const se_shader_handle shader, const char *name) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	for (sz i = 0; i < s_array_get_size(&shader_ptr->uniforms); ++i) {
		se_uniform *uniform = s_array_get(&shader_ptr->uniforms, s_array_handle(&shader_ptr->uniforms, (u32)i));
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.vec3;
		}
	}
	return NULL;
}

s_vec4 *se_shader_get_uniform_vec4(const se_shader_handle shader, const char *name) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	for (sz i = 0; i < s_array_get_size(&shader_ptr->uniforms); ++i) {
		se_uniform *uniform = s_array_get(&shader_ptr->uniforms, s_array_handle(&shader_ptr->uniforms, (u32)i));
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.vec4;
		}
	}
	return NULL;
}

i32 *se_shader_get_uniform_int(const se_shader_handle shader, const char *name) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	for (sz i = 0; i < s_array_get_size(&shader_ptr->uniforms); ++i) {
		se_uniform *uniform = s_array_get(&shader_ptr->uniforms, s_array_handle(&shader_ptr->uniforms, (u32)i));
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.i;
		}
	}
	return NULL;
}

s_mat4 *se_shader_get_uniform_mat4(const se_shader_handle shader, const char *name) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	for (sz i = 0; i < s_array_get_size(&shader_ptr->uniforms); ++i) {
		se_uniform *uniform = s_array_get(&shader_ptr->uniforms, s_array_handle(&shader_ptr->uniforms, (u32)i));
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.mat4;
		}
	}
	return NULL;
}

u32 *se_shader_get_uniform_texture(const se_shader_handle shader, const char *name) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	for (sz i = 0; i < s_array_get_size(&shader_ptr->uniforms); ++i) {
		se_uniform *uniform = s_array_get(&shader_ptr->uniforms, s_array_handle(&shader_ptr->uniforms, (u32)i));
		if (uniform && strcmp(uniform->name, name) == 0) {
			return &uniform->value.texture;
		}
	}
	return NULL;
}

void se_shader_set_float(const se_shader_handle shader, const char *name, f32 value) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	se_uniform_set_float(&shader_ptr->uniforms, name, value);
}

void se_shader_set_vec2(const se_shader_handle shader, const char *name, const s_vec2 *value) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	se_uniform_set_vec2(&shader_ptr->uniforms, name, value);
}

void se_shader_set_vec3(const se_shader_handle shader, const char *name, const s_vec3 *value) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	se_uniform_set_vec3(&shader_ptr->uniforms, name, value);
}

void se_shader_set_vec4(const se_shader_handle shader, const char *name, const s_vec4 *value) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	se_uniform_set_vec4(&shader_ptr->uniforms, name, value);
}

void se_shader_set_int(const se_shader_handle shader, const char *name, i32 value) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	se_uniform_set_int(&shader_ptr->uniforms, name, value);
}

void se_shader_set_mat3(const se_shader_handle shader, const char *name, const s_mat3 *value) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	se_uniform_set_mat3(&shader_ptr->uniforms, name, value);
}

void se_shader_set_mat4(const se_shader_handle shader, const char *name, const s_mat4 *value) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	se_uniform_set_mat4(&shader_ptr->uniforms, name, value);
}

void se_shader_set_texture(const se_shader_handle shader, const char *name, const u32 texture) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	se_uniform_set_texture(&shader_ptr->uniforms, name, texture);
}

void se_context_reload_changed_shaders(void) {
	se_context *ctx = se_current_context();
	if (ctx == NULL) {
		return;
	}

	for (sz i = 0; i < s_array_get_size(&ctx->shaders); ++i) {
		se_shader_handle shader_handle = s_array_handle(&ctx->shaders, (u32)i);
		(void)se_shader_reload_if_changed(shader_handle);
	}
}

se_uniforms *se_context_get_global_uniforms(void) {
	se_context *ctx = se_current_context();
	if (ctx == NULL) {
		return NULL;
	}
	return (se_uniforms *)&ctx->global_uniforms;
}

//static void se_render_buffer_cleanup(se_render_buffer *buffer) {
//	se_log("se_render_buffer_cleanup :: buffer: %p", (void*)buffer);
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
	for (sz i = 0; i < s_array_get_size(uniforms); ++i) {
		se_uniform *found_uniform = s_array_get(uniforms, s_array_handle(uniforms, (u32)i));
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_FLOAT;
			found_uniform->value.f = value;
			return;
		}
	}
	s_handle uniform_handle = s_array_increment(uniforms);
	se_uniform *new_uniform = s_array_get(uniforms, uniform_handle);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_FLOAT;
	new_uniform->value.f = value;
}

void se_uniform_set_vec2(se_uniforms *uniforms, const char *name, const s_vec2 *value) {
	for (sz i = 0; i < s_array_get_size(uniforms); ++i) {
		se_uniform *found_uniform = s_array_get(uniforms, s_array_handle(uniforms, (u32)i));
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_VEC2;
			memcpy(&found_uniform->value.vec2, value, sizeof(s_vec2));
			return;
		}
	}
	s_handle uniform_handle = s_array_increment(uniforms);
	se_uniform *new_uniform = s_array_get(uniforms, uniform_handle);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_VEC2;
	memcpy(&new_uniform->value.vec2, value, sizeof(s_vec2));
}

void se_uniform_set_vec3(se_uniforms *uniforms, const char *name, const s_vec3 *value) {
	s_assertf(uniforms, "se_uniform_set_vec3 :: uniforms is null");
	s_assertf(name, "se_uniform_set_vec3 :: name is null");
	s_assertf(value, "se_uniform_set_vec3 :: value is null");
	for (sz i = 0; i < s_array_get_size(uniforms); ++i) {
		se_uniform *found_uniform = s_array_get(uniforms, s_array_handle(uniforms, (u32)i));
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_VEC3;
			memcpy(&found_uniform->value.vec3, value, sizeof(s_vec3));
			return;
		}
	}
	s_handle uniform_handle = s_array_increment(uniforms);
	se_uniform *new_uniform = s_array_get(uniforms, uniform_handle);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_VEC3;
	memcpy(&new_uniform->value.vec3, value, sizeof(s_vec3));
}

void se_uniform_set_vec4(se_uniforms *uniforms, const char *name, const s_vec4 *value) {
	for (sz i = 0; i < s_array_get_size(uniforms); ++i) {
		se_uniform *found_uniform = s_array_get(uniforms, s_array_handle(uniforms, (u32)i));
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_VEC4;
			memcpy(&found_uniform->value.vec4, value, sizeof(s_vec4));
			return;
		}
	}
	s_handle uniform_handle = s_array_increment(uniforms);
	se_uniform *new_uniform = s_array_get(uniforms, uniform_handle);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_VEC4;
	memcpy(&new_uniform->value.vec4, value, sizeof(s_vec4));
}

void se_uniform_set_int(se_uniforms *uniforms, const char *name, i32 value) {
	for (sz i = 0; i < s_array_get_size(uniforms); ++i) {
		se_uniform *found_uniform = s_array_get(uniforms, s_array_handle(uniforms, (u32)i));
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_INT;
			found_uniform->value.i = value;
			return;
		}
	}
	s_handle uniform_handle = s_array_increment(uniforms);
	se_uniform *new_uniform = s_array_get(uniforms, uniform_handle);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_INT;
	new_uniform->value.i = value;
}

void se_uniform_set_mat3(se_uniforms *uniforms, const char *name, const s_mat3 *value) {
	for (sz i = 0; i < s_array_get_size(uniforms); ++i) {
		se_uniform *found_uniform = s_array_get(uniforms, s_array_handle(uniforms, (u32)i));
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_MAT3;
			memcpy(&found_uniform->value.mat3, value, sizeof(s_mat3));
			return;
		}
	}
	s_handle uniform_handle = s_array_increment(uniforms);
	se_uniform *new_uniform = s_array_get(uniforms, uniform_handle);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_MAT3;
	memcpy(&new_uniform->value.mat3, value, sizeof(s_mat3));
}

void se_uniform_set_mat4(se_uniforms *uniforms, const char *name, const s_mat4 *value) {
	for (sz i = 0; i < s_array_get_size(uniforms); ++i) {
		se_uniform *found_uniform = s_array_get(uniforms, s_array_handle(uniforms, (u32)i));
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_MAT4;
			memcpy(&found_uniform->value.mat4, value, sizeof(s_mat4));
			return;
		}
	}
	s_handle uniform_handle = s_array_increment(uniforms);
	se_uniform *new_uniform = s_array_get(uniforms, uniform_handle);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_MAT4;
	memcpy(&new_uniform->value.mat4, value, sizeof(s_mat4));
}

void se_uniform_set_texture(se_uniforms *uniforms, const char *name, const u32 texture) {
	s_assertf(uniforms, "se_uniform_set_texture :: uniforms is null");
	s_assertf(name, "se_uniform_set_texture :: name is null");
	for (sz i = 0; i < s_array_get_size(uniforms); ++i) {
		se_uniform *found_uniform = s_array_get(uniforms, s_array_handle(uniforms, (u32)i));
		if (found_uniform && strcmp(found_uniform->name, name) == 0) {
			found_uniform->type = SE_UNIFORM_TEXTURE;
			found_uniform->value.texture = texture;
			return;
		}
	}
	s_handle uniform_handle = s_array_increment(uniforms);
	se_uniform *new_uniform = s_array_get(uniforms, uniform_handle);
	strncpy(new_uniform->name, name, sizeof(new_uniform->name) - 1);
	new_uniform->type = SE_UNIFORM_TEXTURE;
	new_uniform->value.texture = texture;
	se_log("se_uniform_set_texture :: added texture uniform: %s, id: %u, ptr: %p", name, texture, (void*)new_uniform);
}

void se_uniform_apply(const se_shader_handle shader, const b8 update_global_uniforms) {
	se_context *ctx = se_current_context();
	se_shader *shader_ptr = se_shader_from_handle(ctx, shader);
	glUseProgram(shader_ptr->program);
	u32 texture_unit = 0;
	for (sz i = 0; i < s_array_get_size(&shader_ptr->uniforms); ++i) {
		se_uniform *uniform = s_array_get(&shader_ptr->uniforms, s_array_handle(&shader_ptr->uniforms, (u32)i));
		GLint location = glGetUniformLocation(shader_ptr->program, uniform->name);
		if (location == -1) {
			continue;
		}
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
	for (sz i = 0; i < s_array_get_size(&ctx->global_uniforms); ++i) {
		se_uniform *uniform = s_array_get(&ctx->global_uniforms, s_array_handle(&ctx->global_uniforms, (u32)i));
		GLint location = glGetUniformLocation(shader_ptr->program, uniform->name);
		if (location == -1) {
			continue;
		}
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
