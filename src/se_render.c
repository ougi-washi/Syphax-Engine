// Syphax-render_handle - Ougi Washi

#include "se_render.h"
#include "se_gl.h"
#include "syphax/s_files.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SE_UNIFORMS_MAX 128

static GLuint se_shader_compile(const char *source, GLenum type);
static GLuint se_shader_create_program(const char *vertex_source, const char *fragment_source);

static b8 se_ensure_capacity(void **data, u32 *capacity, u32 needed, sz elem_size, const char *label);

static b8 se_is_blending = false;
void se_enable_blending() {
	if (se_is_blending) {
	return;
	}
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Also disable depth testing if you have it on
	glDisable(GL_DEPTH_TEST);
	se_is_blending = true;
}

void se_disable_blending() {
	if (!se_is_blending) {
	return;
	}
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	se_is_blending = false;
}

void se_unbind_framebuffer() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void se_render_clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void se_render_set_background_color(const s_vec4 color) {
	glClearColor(color.x, color.y, color.z, color.w);
}

se_render_handle *se_render_handle_create(const se_render_handle_params *params) {
	se_render_handle_params resolved = SE_RENDER_HANDLE_PARAMS_DEFAULTS;
	if (params) {
		resolved = *params;
		if (resolved.framebuffers_count == 0) {
			resolved.framebuffers_count = SE_RENDER_HANDLE_PARAMS_DEFAULTS.framebuffers_count;
		}
		if (resolved.render_buffers_count == 0) {
			resolved.render_buffers_count = SE_RENDER_HANDLE_PARAMS_DEFAULTS.render_buffers_count;
		}
		if (resolved.textures_count == 0) {
			resolved.textures_count = SE_RENDER_HANDLE_PARAMS_DEFAULTS.textures_count;
		}
		if (resolved.shaders_count == 0) {
			resolved.shaders_count = SE_RENDER_HANDLE_PARAMS_DEFAULTS.shaders_count;
		}
		if (resolved.models_count == 0) {
			resolved.models_count = SE_RENDER_HANDLE_PARAMS_DEFAULTS.models_count;
		}
		if (resolved.cameras_count == 0) {
			resolved.cameras_count = SE_RENDER_HANDLE_PARAMS_DEFAULTS.cameras_count;
		}
	}
	se_render_handle *render_handle = malloc(sizeof(se_render_handle));
	memset(render_handle, 0, sizeof(se_render_handle));

	s_array_init(&render_handle->framebuffers, resolved.framebuffers_count);
	s_array_init(&render_handle->render_buffers, resolved.render_buffers_count);
	s_array_init(&render_handle->textures, resolved.textures_count);
	s_array_init(&render_handle->shaders, resolved.shaders_count);
	s_array_init(&render_handle->models, resolved.models_count);
	s_array_init(&render_handle->cameras, resolved.cameras_count);
	s_array_init(&render_handle->global_uniforms, SE_UNIFORMS_MAX);
	se_render_set_background_color(s_vec4(0.0f, 0.0f, 0.0f, 1.0f));

	printf("se_render_handle_create :: created render handle %p\n", render_handle);
	return render_handle;
}

void se_render_handle_cleanup(se_render_handle *render_handle) {
	s_assertf(render_handle, "se_render_handle_cleanup :: render_handle is null");

	printf("se_render_handle_cleanup :: render handle: %p\n", render_handle);
	s_foreach(&render_handle->models, i) {
	se_model *curr_model = s_array_get(&render_handle->models, i);
	se_model_cleanup(curr_model);
	}
	s_array_clear(&render_handle->models);

	s_foreach(&render_handle->textures, i) {
	se_texture *curr_texture = s_array_get(&render_handle->textures, i);
	se_texture_cleanup(curr_texture);
	}
	s_array_clear(&render_handle->textures);

	s_foreach(&render_handle->shaders, i) {
	se_shader *curr_shader = s_array_get(&render_handle->shaders, i);
	se_shader_cleanup(curr_shader);
	}
	s_array_clear(&render_handle->shaders);
	s_array_clear(&render_handle->cameras);
	s_array_clear(&render_handle->global_uniforms);

	s_foreach(&render_handle->framebuffers, i) {
	se_framebuffer *curr_framebuffer =
		s_array_get(&render_handle->framebuffers, i);
	se_framebuffer_cleanup(curr_framebuffer);
	}
	s_array_clear(&render_handle->framebuffers);

	s_foreach(&render_handle->render_buffers, i) {
	se_render_buffer *curr_buffer =
		s_array_get(&render_handle->render_buffers, i);
	se_render_buffer_cleanup(curr_buffer);
	}
	s_array_clear(&render_handle->render_buffers);

	free(render_handle);
	printf("se_render_handle_cleanup :: finished cleanup\n");
}

void se_render_handle_reload_changed_shaders(se_render_handle *render_handle) {
	s_foreach(&render_handle->shaders, i) {
	se_shader_reload_if_changed(s_array_get(&render_handle->shaders, i));
	}
}

se_uniforms *se_render_handle_get_global_uniforms(se_render_handle *render_handle) {
	return &render_handle->global_uniforms;
}

// Shader functions

se_texture *se_texture_load(se_render_handle *render_handle, const char *file_path, const se_texture_wrap wrap) {
	stbi_set_flip_vertically_on_load(1);

	se_texture *texture = s_array_increment(&render_handle->textures);

	char full_path[SE_MAX_PATH_LENGTH] = {0};
	if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, RESOURCES_DIR, file_path)) {
		fprintf(stderr, "se_texture_load :: path too long for %s\n", file_path);
		return NULL;
	}

	unsigned char *pixels = stbi_load(full_path, &texture->width, &texture->height, &texture->channels, 0);
	if (!pixels) {
		fprintf(stderr, "se_texture_load :: could not load image %s\n", file_path);
		return NULL;
	}

	glGenTextures(1, &texture->id);
	glActiveTexture(GL_TEXTURE0); // always bind to unit 0 by default
	glBindTexture(GL_TEXTURE_2D, texture->id);

	// Upload to GPU
	GLenum format = (texture->channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Set filtering/wrapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (wrap == SE_CLAMP) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else if (wrap == SE_REPEAT) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	stbi_image_free(pixels);
	return texture;
}

se_texture *se_texture_load_from_memory(se_render_handle *render_handle, const u8 *data, const sz size, const se_texture_wrap wrap) {
	stbi_set_flip_vertically_on_load(1);

	se_texture *texture = s_array_increment(&render_handle->textures);

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 0);
	if (!pixels) {
		fprintf(stderr, "se_texture_load_from_memory :: could not load image from memory\n");
		return NULL;
	}

	texture->width = width;
	texture->height = height;
	texture->channels = channels;
	texture->path[0] = '\0';

	glGenTextures(1, &texture->id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->id);

	GLenum format = (texture->channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (wrap == SE_CLAMP) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else if (wrap == SE_REPEAT) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	stbi_image_free(pixels);
	return texture;
}

void se_texture_cleanup(se_texture *texture) {
	glDeleteTextures(1, &texture->id);
	texture->id = 0;
	texture->width = 0;
	texture->height = 0;
	texture->channels = 0;
	texture->path[0] = '\0';
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

se_shader *se_shader_load(se_render_handle *render_handle, const char *vertex_file_path, const char *fragment_file_path) {
	s_assertf(render_handle, "se_shader_load :: render_handle is null");
	s_assertf(vertex_file_path, "se_shader_load :: vertex_file_path is null");
	s_assertf(fragment_file_path, "se_shader_load :: fragment_file_path is null");

	if (s_array_get_capacity(&render_handle->shaders) <= s_array_get_size(&render_handle->shaders)) {
	printf("se_shader_load :: error, render_handle->shaders is full, allocate more space\n");
	return NULL;
	}

	se_shader *new_shader = s_array_increment(&render_handle->shaders);

	// make path absolute
	if (strlen(vertex_file_path) > 0) {
		if (!s_path_join(new_shader->vertex_path, SE_MAX_PATH_LENGTH, RESOURCES_DIR, vertex_file_path)) {
			fprintf(stderr, "se_shader_load :: vertex path too long for %s\n", vertex_file_path);
			return NULL;
		}
	} else {
		new_shader->vertex_path[0] = '\0';
	}

	if (strlen(fragment_file_path) > 0) {
		if (!s_path_join(new_shader->fragment_path, SE_MAX_PATH_LENGTH, RESOURCES_DIR, fragment_file_path)) {
			fprintf(stderr, "se_shader_load :: fragment path too long for %s\n", fragment_file_path);
			return NULL;
		}
	} else {
		new_shader->fragment_path[0] = '\0';
	}

	if (se_shader_load_internal(new_shader)) {
	return new_shader;
	}
	return NULL;
}

b8 se_shader_reload_if_changed(se_shader *shader) {
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

void se_shader_use(se_render_handle *render_handle, se_shader *shader, const b8 update_uniforms, const b8 update_global_uniforms) {
	glUseProgram(shader->program);
	if (update_uniforms) {
	se_uniform_apply(render_handle, shader, update_global_uniforms);
	}
}

void se_shader_cleanup(se_shader *shader) {
	s_array_clear(&shader->uniforms);
	printf("se_shader_cleanup :: shader: %p\n", shader);
	if (shader->program) {
	glDeleteProgram(shader->program);
	shader->program = 0;
	}
}

GLuint se_shader_get_uniform_location(se_shader *shader, const char *name) {
	return glGetUniformLocation(shader->program, name);
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

void se_shader_set_texture(se_shader *shader, const char *name, GLuint texture) {
	se_uniform_set_texture(&shader->uniforms, name, texture);
}

void se_shader_set_buffer_texture(se_shader *shader, const char *name, se_render_buffer *buffer) {
	se_uniform_set_buffer_texture(&shader->uniforms, name, buffer);
}

// Mesh functions
void se_mesh_translate(se_mesh *mesh, const s_vec3 *v) {
    s_mat4_translate(&mesh->matrix, v);
}

void se_mesh_rotate(se_mesh *mesh, const s_vec3 *v) {
	//mesh->matrix = mat4_mul(mesh->matrix, mat4_rotate_x(mat4_identity(), v->x));
	//mesh->matrix = mat4_mul(mesh->matrix, mat4_rotate_y(mat4_identity(), v->y));
	//mesh->matrix = mat4_mul(mesh->matrix, mat4_rotate_z(mat4_identity(), v->z));
    s_mat4_rotate_x(&mesh->matrix, v->x);
    s_mat4_rotate_y(&mesh->matrix, v->y);
    s_mat4_rotate_z(&mesh->matrix, v->z);
}

void se_mesh_scale(se_mesh *mesh, const s_vec3 *v) {
    s_mat4_scale(&mesh->matrix, v);
}

static b8 se_ensure_capacity(void **data, u32 *capacity, u32 needed, sz elem_size, const char *label) {
	if (needed <= *capacity) {
		return true;
	}

	u32 new_capacity = *capacity ? *capacity : 64;
	while (new_capacity < needed) {
		u32 next_capacity = new_capacity * 2;
		if (next_capacity < new_capacity) {
			new_capacity = needed;
			break;
		}
		new_capacity = next_capacity;
	}

	void *new_data = realloc(*data, (sz)new_capacity * elem_size);
	if (!new_data) {
		fprintf(stderr, "se_model_load_obj :: out of memory for %s\n", label);
		return false;
	}

	*data = new_data;
	*capacity = new_capacity;
	return true;
}

// Helper function to finalize a mesh
static void se_mesh_finalize(se_mesh *mesh, se_vertex_3d *vertices, u32 *indices, u32 vertex_count, u32 index_count, se_shaders_ptr *shaders, u32 se_mesh_index) {
	// Allocate mesh data
	mesh->vertices = malloc(vertex_count * sizeof(se_vertex_3d));
	mesh->indices = malloc(index_count * sizeof(u32));
	memcpy(mesh->vertices, vertices, vertex_count * sizeof(se_vertex_3d));
	memcpy(mesh->indices, indices, index_count * sizeof(u32));
	mesh->vertex_count = vertex_count;
	mesh->index_count = index_count;
	mesh->matrix = s_mat4_identity;
	mesh->texture_id = 0;

	// Assign shader (cycle through available shaders)
	if (s_array_get_size(shaders) > 0) {
	mesh->shader = *s_array_get(shaders, se_mesh_index % s_array_get_size(shaders));
	} else {
	mesh->shader = NULL;
	fprintf(stderr, "No shaders provided for mesh %u\n", se_mesh_index);
	}

	// Create OpenGL objects
	glGenVertexArrays(1, &mesh->vao);
	glGenBuffers(1, &mesh->vbo);
	glGenBuffers(1, &mesh->ebo);

	glBindVertexArray(mesh->vao);

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(se_vertex_3d), mesh->vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u32), mesh->indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)offsetof(se_vertex_3d, normal));
	glEnableVertexAttribArray(1);

	// UV attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)offsetof(se_vertex_3d, uv));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

se_model *se_model_load_obj(se_render_handle *render_handle, const char *path, se_shaders_ptr *shaders) {
	se_model *model = s_array_increment(&render_handle->models);
	s_array_init(&model->meshes, 64);

	char full_path[SE_MAX_PATH_LENGTH] = {0};
	if (!s_path_join(full_path, SE_MAX_PATH_LENGTH, RESOURCES_DIR, path)) {
		fprintf(stderr, "se_model_load_obj :: path too long for %s\n", path);
		return NULL;
	}

	char *file_data = NULL;
	if (!s_file_read(full_path, &file_data, NULL)) {
		fprintf(stderr, "se_model_load_obj :: failed to open OBJ file: %s\n", path);
		return NULL;
	}

	// Arrays for temporary storage (shared across all meshes)
	s_vec3 *temp_vertices = NULL;
	s_vec3 *temp_normals = NULL;
	s_vec2 *temp_uvs = NULL;
	u32 temp_vertices_capacity = 0;
	u32 temp_normals_capacity = 0;
	u32 temp_uvs_capacity = 0;

	u32 vertex_count = 0;
	u32 normal_count = 0;
	u32 uv_count = 0;

	// Dynamic array for meshes

	// Current mesh data
	se_vertex_3d *current_vertices = NULL;
	u32 *current_indices = NULL;
	u32 current_vertices_capacity = 0;
	u32 current_indices_capacity = 0;
	u32 current_vertex_count = 0;
	u32 current_index_count = 0;

	char line[256];
	char current_object[256] = "default";
	b8 hse_faces = false;
	b8 ok = true;

	const char *cursor = file_data;
	while (cursor && *cursor) {
		sz line_len = 0;
		while (cursor[line_len] != '\0' && cursor[line_len] != '\n' && cursor[line_len] != '\r') {
			line_len++;
		}
		sz copy_len = line_len;
		if (copy_len >= sizeof(line)) {
			copy_len = sizeof(line) - 1;
		}
		memcpy(line, cursor, copy_len);
		line[copy_len] = '\0';
		cursor += line_len;
		if (*cursor == '\r') {
			cursor++;
		}
		if (*cursor == '\n') {
			cursor++;
		}
	if (strncmp(line, "v ", 2) == 0) {
		// Vertex position
		if (!se_ensure_capacity((void **)&temp_vertices, &temp_vertices_capacity, vertex_count + 1, sizeof(s_vec3), "vertex positions")) {
			ok = false;
			break;
		}
		sscanf(line, "v %f %f %f", &temp_vertices[vertex_count].x,
			 &temp_vertices[vertex_count].y, &temp_vertices[vertex_count].z);
		vertex_count++;
	} else if (strncmp(line, "vn ", 3) == 0) {
		// Vertex normal
		if (!se_ensure_capacity((void **)&temp_normals, &temp_normals_capacity, normal_count + 1, sizeof(s_vec3), "vertex normals")) {
			ok = false;
			break;
		}
		sscanf(line, "vn %f %f %f", &temp_normals[normal_count].x,
			 &temp_normals[normal_count].y, &temp_normals[normal_count].z);
		normal_count++;
	} else if (strncmp(line, "vt ", 3) == 0) {
		// Vertex texture coordinate
		if (!se_ensure_capacity((void **)&temp_uvs, &temp_uvs_capacity, uv_count + 1, sizeof(s_vec2), "vertex uvs")) {
			ok = false;
			break;
		}
		sscanf(line, "vt %f %f", &temp_uvs[uv_count].x, &temp_uvs[uv_count].y);
		uv_count++;
	} else if (strncmp(line, "o ", 2) == 0 || strncmp(line, "g ", 2) == 0) {
		// New object/group - finalize current mesh if it has faces
		if (hse_faces && current_vertex_count > 0) {
		se_mesh *new_mesh = s_array_increment(&model->meshes);
		const sz mesh_count = s_array_get_size(&model->meshes);
		if (mesh_count > 0) {
			se_mesh_finalize(new_mesh, current_vertices, current_indices, current_vertex_count, current_index_count, shaders, mesh_count - 1);
		}
		// Reset for next mesh
		current_vertex_count = 0;
		current_index_count = 0;
		hse_faces = false;
		}

		// Get new object name
		sscanf(line, "%*s %255s", current_object);
	} else if (strncmp(line, "f ", 2) == 0) {
		// Face
		hse_faces = true;
		u32 v1, v2, v3, n1, n2, n3, t1, t2, t3;
		i32 matches = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &t1, &n1,
					 &v2, &t2, &n2, &v3, &t3, &n3);

		if (matches == 9) {
			if (!se_ensure_capacity((void **)&current_vertices, &current_vertices_capacity, current_vertex_count + 3, sizeof(se_vertex_3d), "mesh vertices") ||
				!se_ensure_capacity((void **)&current_indices, &current_indices_capacity, current_index_count + 3, sizeof(u32), "mesh indices")) {
				ok = false;
				break;
			}
		// Create vertices for this face
		for (i32 i = 0; i < 3; i++) {
			u32 vi = (i == 0) ? v1 - 1 : (i == 1) ? v2 - 1 : v3 - 1;
			u32 ni = (i == 0) ? n1 - 1 : (i == 1) ? n2 - 1 : n3 - 1;
			u32 ti = (i == 0) ? t1 - 1 : (i == 1) ? t2 - 1 : t3 - 1;

			// Check bounds
			if (vi >= vertex_count || ni >= normal_count || ti >= uv_count) {
			fprintf(stderr, "OBJ file contains invalid face indices\n");
			continue;
			}

			current_vertices[current_vertex_count].position = temp_vertices[vi];
			current_vertices[current_vertex_count].normal = temp_normals[ni];
			current_vertices[current_vertex_count].uv = temp_uvs[ti];

			current_indices[current_index_count] = current_vertex_count;
			current_vertex_count++;
			current_index_count++;
		}
		} else {
		// Try to parse face without texture coordinates (v//n format)
		matches = sscanf(line, "f %d//%d %d//%d %d//%d", &v1, &n1, &v2, &n2, &v3, &n3);
		if (matches == 6) {
			if (!se_ensure_capacity((void **)&current_vertices, &current_vertices_capacity, current_vertex_count + 3, sizeof(se_vertex_3d), "mesh vertices") ||
				!se_ensure_capacity((void **)&current_indices, &current_indices_capacity, current_index_count + 3, sizeof(u32), "mesh indices")) {
				ok = false;
				break;
			}
			for (i32 i = 0; i < 3; i++) {
				u32 vi = (i == 0) ? v1 - 1 : (i == 1) ? v2 - 1 : v3 - 1;
				u32 ni = (i == 0) ? n1 - 1 : (i == 1) ? n2 - 1 : n3 - 1;

			if (vi >= vertex_count || ni >= normal_count) {
				fprintf(stderr, "OBJ file contains invalid face indices\n");
				continue;
			}

			current_vertices[current_vertex_count].position = temp_vertices[vi];
			current_vertices[current_vertex_count].normal = temp_normals[ni];
			current_vertices[current_vertex_count].uv = (s_vec2){0.0f, 0.0f}; // Default UV

			current_indices[current_index_count] = current_vertex_count;
			current_vertex_count++;
			current_index_count++;
			}
		}
		}
	}
	}

	// Finalize the last mesh
	if (ok && hse_faces && current_vertex_count > 0) {
	const sz mesh_count = s_array_get_size(&model->meshes);
	se_mesh *new_mesh = s_array_increment(&model->meshes);
	se_mesh_finalize(new_mesh, current_vertices, current_indices, current_vertex_count, current_index_count, shaders, mesh_count - 1);
	}

	free(file_data);

	// Cleanup temporary arrays
	free(temp_vertices);
	free(temp_normals);
	free(temp_uvs);
	free(current_vertices);
	free(current_indices);

	if (!ok) {
		se_model_cleanup(model);
		return NULL;
	}

	// If no meshes were created, create a default one
	if (s_array_get_size(&model->meshes) == 0) {
	fprintf(stderr, "No valid meshes found in OBJ file: %s\n", path);
	s_array_clear(&model->meshes);
	return NULL;
	}

	return model;
}

void se_model_render(se_render_handle *render_handle, se_model *model, se_camera *camera) {
	// set up global view/proj once per frame
	const s_mat4 proj = se_camera_get_projection_matrix(camera);
	const s_mat4 view = se_camera_get_view_matrix(camera);
	s_foreach(&model->meshes, i) {
	se_mesh *mesh = s_array_get(&model->meshes, i);
	se_shader *sh = mesh->shader;

	se_shader_use(render_handle, sh, true, true);

	s_mat4 vp = s_mat4_mul(&proj, &view);
	s_mat4 mvp = s_mat4_mul(&vp, &mesh->matrix);

	GLint loc_mvp = glGetUniformLocation(sh->program, "u_mvp");
	if (loc_mvp >= 0) {
		glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, mvp.m[0]);
	}

	GLint loc_model = glGetUniformLocation(sh->program, "u_model");
	if (loc_model >= 0) {
		glUniformMatrix4fv(loc_model, 1, GL_FALSE, mesh->matrix.m[0]);
	}

	// send to the GPU other uniforms (lights if forward rendering, etc)

	// draw
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glBindVertexArray(mesh->vao);
	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	}
	// unbind
	glBindVertexArray(0);
	glUseProgram(0);
}

void se_model_cleanup(se_model *model) {
	s_foreach(&model->meshes, i) {
	se_mesh *mesh = s_array_get(&model->meshes, i);
	glDeleteVertexArrays(1, &mesh->vao);
	glDeleteBuffers(1, &mesh->vbo);
	glDeleteBuffers(1, &mesh->ebo);
	free(mesh->vertices);
	free(mesh->indices);
	}
	s_array_clear(&model->meshes);
}

void se_model_translate(se_model *model, const s_vec3 *v) {
	s_foreach(&model->meshes, i) {
	se_mesh *mesh = s_array_get(&model->meshes, i);
	se_mesh_translate(mesh, v);
	}
}

void se_model_rotate(se_model *model, const s_vec3 *v) {
	s_foreach(&model->meshes, i) {
	se_mesh *mesh = s_array_get(&model->meshes, i);
	se_mesh_rotate(mesh, v);
	}
}

void se_model_scale(se_model *model, const s_vec3 *v) {
	s_foreach(&model->meshes, i) {
	se_mesh *mesh = s_array_get(&model->meshes, i);
	se_mesh_scale(mesh, v);
	}
}

// camera functions
se_camera *se_camera_create(se_render_handle *render_handle) {
	se_camera *camera = s_array_increment(&render_handle->cameras);
	camera->position = (s_vec3){0, 0, 5};
	camera->target = (s_vec3){0, 0, 0};
	camera->up = (s_vec3){0, 1, 0};
	camera->right = (s_vec3){1, 0, 0};
	camera->fov = 45.0f;
	camera->near = 0.1f;
	camera->far = 100.0f;
	camera->aspect = 1.78;
	printf("se_camera_create :: created camera %p\n", camera);
	return camera;
}

s_mat4 se_camera_get_view_matrix(const se_camera *camera) {
	return s_mat4_look_at(&camera->position, &camera->target, &camera->up);
}

s_mat4 se_camera_get_projection_matrix(const se_camera *camera) {
	return s_mat4_perspective(camera->fov * (PI / 180.0f), camera->aspect, camera->near, camera->far);
}

void se_camera_set_aspect(se_camera *camera, const f32 width, const f32 height) {
	camera->aspect = width / height;
}

void se_camera_destroy(se_render_handle *render_handle, se_camera *camera) {
	printf("se_camera_destroy :: camera: %p\n", camera);
	s_array_remove(&render_handle->cameras, camera);
}

// Framebuffer functions
se_framebuffer *se_framebuffer_create(se_render_handle *render_handle, const s_vec2 *size) {
	se_framebuffer *framebuffer = s_array_increment(&render_handle->framebuffers);
	framebuffer->size = *size;

	// Create framebuffer
	glGenFramebuffers(1, &framebuffer->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);

	// Create color texture
	glGenTextures(1, &framebuffer->texture);
	glBindTexture(GL_TEXTURE_2D, framebuffer->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size->x, size->y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->texture, 0);

	// Create depth buffer
	glGenRenderbuffers(1, &framebuffer->depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, framebuffer->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size->x, size->y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->depth_buffer);

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	fprintf(stderr, "se_framebuffer_create :: framebuffer not complete!\n");
	se_framebuffer_cleanup(framebuffer);
	return false;
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	printf("se_framebuffer_create :: created framebuffer %p\n", framebuffer);
	return framebuffer;
}

void se_framebuffer_set_size(se_framebuffer *framebuffer, const s_vec2 *size) {
	s_assertf(framebuffer, "se_framebuffer_set_size :: framebuffer is null");
	s_assertf(size, "se_framebuffer_set_size :: size is null");

	framebuffer->size = *size;

	glBindTexture(GL_TEXTURE_2D, framebuffer->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (i32)size->x, (i32)size->y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, framebuffer->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (i32)size->x, (i32)size->y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->depth_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void se_framebuffer_get_size(se_framebuffer *framebuffer, s_vec2 *out_size) {
	s_assertf(framebuffer, "se_framebuffer_get_size :: framebuffer is null");
	s_assertf(out_size, "se_framebuffer_get_size :: out_size is null");
	*out_size = framebuffer->size;
}

void se_framebuffer_bind(se_framebuffer *framebuffer) {
	glViewport(0, 0, framebuffer->size.x, framebuffer->size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);
}

void se_framebuffer_unbind(se_framebuffer *framebuffer) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void se_framebuffer_cleanup(se_framebuffer *framebuffer) {
	printf("se_framebuffer_cleanup :: framebuffer: %p\n", framebuffer);
	if (framebuffer->framebuffer) {
	glDeleteFramebuffers(1, &framebuffer->framebuffer);
	framebuffer->framebuffer = 0;
	}
	if (framebuffer->texture) {
	glDeleteTextures(1, &framebuffer->texture);
	framebuffer->texture = 0;
	}
	if (framebuffer->depth_buffer) {
	glDeleteRenderbuffers(1, &framebuffer->depth_buffer);
	framebuffer->depth_buffer = 0;
	}
}

// Render buffer functions
se_render_buffer *se_render_buffer_create(se_render_handle *render_handle, const u32 width, const u32 height, const c8 *fragment_shader_path) {
	se_render_buffer *buffer = s_array_increment(&render_handle->render_buffers);

	buffer->texture_size = s_vec2(width, height);
	buffer->scale = s_vec2(1., 1.);
	buffer->position = s_vec2(0., 0.);

	// Create framebuffer
	glGenFramebuffers(1, &buffer->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->framebuffer);

	// Create color texture
	glGenTextures(1, &buffer->texture);
	glBindTexture(GL_TEXTURE_2D, buffer->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer->texture, 0);

	// Create depth buffer
	glGenRenderbuffers(1, &buffer->depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, buffer->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer->depth_buffer);

	// Create previous texture
	glGenTextures(1, &buffer->prev_texture);
	glBindTexture(GL_TEXTURE_2D, buffer->prev_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create framebuffer for previous frame (for easy copying)
	glGenFramebuffers(1, &buffer->prev_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->prev_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer->prev_texture, 0);

	buffer->shader = se_shader_load(render_handle, "shaders/render_buffer_vert.glsl", fragment_shader_path);

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	fprintf(stderr, "se_render_buffer_create :: Framebuffer not complete!\n");
	se_render_buffer_cleanup(buffer);
	return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	printf("se_render_buffer_create :: created render buffer %p\n", buffer);
	return buffer;
}

void se_render_buffer_copy_to_previous(se_render_buffer *buffer) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, buffer->framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer->prev_framebuffer);

	glBlitFramebuffer(0, 0, buffer->texture_size.x,
					buffer->texture_size.y, // Source rectangle
					0, 0, buffer->texture_size.x,
					buffer->texture_size.y, // Destination rectangle
					GL_COLOR_BUFFER_BIT,	// Copy color buffer
					GL_NEAREST // Use nearest filtering for exact copy
	);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void se_render_buffer_set_shader(se_render_buffer *buffer, se_shader *shader) {
	s_assert(buffer && shader);
	buffer->shader = shader;
}

void se_render_buffer_unset_shader(se_render_buffer *buffer) {
	s_assert(buffer);
	buffer->shader = NULL;
}

void se_render_buffer_bind(se_render_buffer *buffer) {
	se_render_buffer_copy_to_previous(buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->framebuffer);
	glViewport(0, 0, buffer->texture_size.x, buffer->texture_size.y);
	se_shader_set_texture(buffer->shader, "u_prev", buffer->prev_texture);
	se_shader_set_vec2(buffer->shader, "u_scale", &buffer->scale);
	se_shader_set_vec2(buffer->shader, "u_position", &buffer->position);
	se_shader_set_vec2(buffer->shader, "u_texture_size", &buffer->texture_size);
}

void se_render_buffer_unbind(se_render_buffer *buf) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void se_render_buffer_set_scale(se_render_buffer *buffer, const s_vec2 *scale) {
	buffer->scale = *scale;
}

void se_render_buffer_set_position(se_render_buffer *buffer, const s_vec2 *position) {
	buffer->position = *position;
}

void se_render_buffer_cleanup(se_render_buffer *buffer) {
	printf("se_render_buffer_cleanup :: buffer: %p\n", buffer);
	if (buffer->framebuffer) {
	glDeleteFramebuffers(1, &buffer->framebuffer);
	buffer->framebuffer = 0;
	}
	if (buffer->prev_framebuffer) {
		glDeleteFramebuffers(1, &buffer->prev_framebuffer);
		buffer->prev_framebuffer = 0;
	}
	if (buffer->texture) {
	glDeleteTextures(1, &buffer->texture);
	buffer->texture = 0;
	}
	if (buffer->prev_texture) {
		glDeleteTextures(1, &buffer->prev_texture);
		buffer->prev_texture = 0;
	}
	if (buffer->depth_buffer) {
	glDeleteRenderbuffers(1, &buffer->depth_buffer);
	buffer->depth_buffer = 0;
	}
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

GLuint *se_shader_get_uniform_texture(se_shader *shader, const char *name) {
	s_foreach(&shader->uniforms, i) {
	se_uniform *uniform = s_array_get(&shader->uniforms, i);
	if (uniform && strcmp(uniform->name, name) == 0) {
		return &uniform->value.texture;
	}
	}
	return NULL;
}

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

void se_uniform_set_texture(se_uniforms *uniforms, const char *name, GLuint texture) {
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
	printf(
		"se_uniform_set_texture :: added texture uniform: %s, id: %d, ptr: %p\n",
		name, texture, new_uniform);
}

void se_uniform_set_buffer_texture(se_uniforms *uniforms, const char *name, se_render_buffer *buffer) {
	se_uniform_set_texture(uniforms, name, buffer->texture);
}

void se_uniform_apply(se_render_handle *render_handle, se_shader *shader, const b8 update_global_uniforms) {
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
	se_uniforms *global_uniforms =
		se_render_handle_get_global_uniforms(render_handle);
	s_foreach(global_uniforms, i) {
	se_uniform *uniform = s_array_get(global_uniforms, i);
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

void se_mesh_instance_create(se_mesh_instance *out_instance, const se_mesh *mesh, const u32 instance_count) {
	s_assertf(out_instance, "se_mesh_instance_create :: out_instance is null");
	s_assertf(mesh, "se_mesh_instance_create :: mesh is null");

	memset(out_instance, 0, sizeof(se_mesh_instance));
	glGenVertexArrays(1, &out_instance->vao);
	glBindVertexArray(out_instance->vao);

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, uv));

	glBindVertexArray(0);

	if (instance_count > 0) {
		s_array_init(&out_instance->instance_buffers, instance_count);
		out_instance->instance_buffers_dirty = true;
	}
}

void se_mesh_instance_add_buffer(se_mesh_instance *instance, const s_mat4 *buffer, const sz instance_count) {
	s_assertf(instance, "se_mesh_instance_add_buffer :: instance is null");
	s_assertf(buffer, "se_mesh_instance_add_buffer :: buffer is null");

	glBindVertexArray(instance->vao);

	se_instance_buffer *new_buffer = s_array_increment(&instance->instance_buffers);
	new_buffer->vbo = 0;
	new_buffer->buffer_ptr = buffer;
	new_buffer->buffer_size = sizeof(s_mat4) * instance_count;
	glGenBuffers(1, &new_buffer->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, new_buffer->vbo);
	glBufferData(GL_ARRAY_BUFFER, new_buffer->buffer_size, new_buffer->buffer_ptr, GL_DYNAMIC_DRAW);

	for (u32 i = 0; i < 4; i++) {
		const u32 attrib = 3 + i;
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 4, GL_FLOAT, GL_FALSE, sizeof(s_mat4), (void *)(sizeof(s_vec4) * i));
		glVertexAttribDivisor(attrib, 1);
	}
	instance->instance_buffers_dirty = true;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void se_mesh_instance_update(se_mesh_instance *instance) {
	s_assertf(instance, "se_mesh_instance_update :: instance is null");
	if (!instance->instance_buffers_dirty) {
		return;
	}

	s_foreach(&instance->instance_buffers, i) {
		se_instance_buffer *current_buffer = s_array_get(&instance->instance_buffers, i);
		glBindBuffer(GL_ARRAY_BUFFER, current_buffer->vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, current_buffer->buffer_size, current_buffer->buffer_ptr);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	instance->instance_buffers_dirty = false;
}

void se_mesh_instance_destroy(se_mesh_instance *instance) {
	s_assertf(instance, "se_mesh_instance_destroy :: instance is null");
	s_foreach(&instance->instance_buffers, i) {
		se_instance_buffer *current_buffer = s_array_get(&instance->instance_buffers, i);
		glDeleteBuffers(1, &current_buffer->vbo);
	}
	s_array_clear(&instance->instance_buffers);
	if (instance->vao) {
		glDeleteVertexArrays(1, &instance->vao);
		instance->vao = 0;
	}
}

void se_quad_2d_create(se_quad *out_quad, const u32 instance_count) {
	s_assertf(out_quad, "se_quad_create :: out_quad is null");

	out_quad->vao = 0;
	out_quad->vbo = 0;
	out_quad->ebo = 0;
	out_quad->last_attribute = 0;

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
	s_array_init(&out_quad->instance_buffers, instance_count);
	out_quad->instance_buffers_dirty = true;
	}
}

void se_quad_2d_add_instance_buffer(se_quad *quad, const s_mat4 *buffer, const sz instance_count) {
	s_assertf(quad, "se_quad_2d_add_instance_buffer :: quad is null");
	s_assertf(buffer, "se_quad_2d_add_instance_buffer :: buffer is null");

	glBindVertexArray(quad->vao);

	se_instance_buffer *new_buffer = s_array_increment(&quad->instance_buffers);
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

	printf("se_quad_2d_add_instance_buffer :: buffer: %p\n", buffer);
}

void se_quad_2d_add_instance_buffer_mat3(se_quad *quad, const s_mat3 *buffer, const sz instance_count) {
	s_assertf(quad, "se_quad_2d_add_instance_buffer_mat3 :: quad is null");
	s_assertf(buffer, "se_quad_2d_add_instance_buffer_mat3 :: buffer is null");

	glBindVertexArray(quad->vao);

	se_instance_buffer *new_buffer = s_array_increment(&quad->instance_buffers);
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

	printf("se_quad_2d_add_instance_buffer_mat3 :: buffer: %p\n", buffer);
}

void se_quad_render(se_quad *quad, const sz instance_count) {
	glBindVertexArray(quad->vao);
	if (instance_count > 0) {
		if (quad->instance_buffers_dirty) {
			s_foreach(&quad->instance_buffers, i) {
			se_instance_buffer *current_buffer = s_array_get(&quad->instance_buffers, i);
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
	s_foreach(&quad->instance_buffers, i) {
	se_instance_buffer *current_buffer = s_array_get(&quad->instance_buffers, i);
	glDeleteBuffers(1, &current_buffer->vbo);
	}
	s_array_clear(&quad->instance_buffers);
	glDeleteVertexArrays(1, &quad->vao);
	glDeleteBuffers(1, &quad->vbo);
	glDeleteBuffers(1, &quad->ebo);
}

static GLuint se_shader_compile(const char *source, GLenum type) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
	char info_log[512];
	glGetShaderInfoLog(shader, 512, NULL, info_log);
	fprintf(stderr, "se_shader_compile :: Shader compilation failed: %s\n", info_log);
	glDeleteShader(shader);
	return 0;
	}

	return shader;
}

static GLuint se_shader_create_program(const char *vertex_source, const char *fragment_source) {
	GLuint vertex_shader = se_shader_compile(vertex_source, GL_VERTEX_SHADER);
	GLuint fragment_shader = se_shader_compile(fragment_source, GL_FRAGMENT_SHADER);
	if (!vertex_shader || !fragment_shader) {
	if (vertex_shader) {
		glDeleteShader(vertex_shader);
	}
	if (fragment_shader) {
		glDeleteShader(fragment_shader);
	}
	printf("se_shader_create_program :: Failed to create shader program, vertex or fragment shaders are invalid\n");
	return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
	char info_log[512];
	glGetProgramInfoLog(program, 512, NULL, info_log);
	fprintf(stderr, "se_shader_create_program :: Shader program linking failed: %s\n", info_log);
	glDeleteProgram(program);
	program = 0;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}
