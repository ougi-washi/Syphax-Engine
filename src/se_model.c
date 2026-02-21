// Syphax Engine - Ougi Washi

#include "se_model.h"
#include "se_debug.h"
#include "render/se_gl.h"
#include "syphax/s_files.h"

static void se_model_cleanup(se_model *model);

static se_model *se_model_from_handle(se_context *ctx, const se_model_handle model) {
	return s_array_get(&ctx->models, model);
}

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

static b8 se_mesh_data_flags_are_valid(const se_mesh_data_flags mesh_data_flags) {
	const se_mesh_data_flags supported_flags = (se_mesh_data_flags)(SE_MESH_DATA_CPU | SE_MESH_DATA_GPU);
	if ((mesh_data_flags & supported_flags) == 0) {
		return false;
	}
	if ((mesh_data_flags & ~supported_flags) != 0) {
		return false;
	}
	return true;
}

b8 se_mesh_has_cpu_data(const se_mesh *mesh) {
	if (mesh == NULL) {
		return false;
	}
	return (mesh->data_flags & SE_MESH_DATA_CPU) != 0;
}

b8 se_mesh_has_gpu_data(const se_mesh *mesh) {
	if (mesh == NULL) {
		return false;
	}
	if ((mesh->data_flags & SE_MESH_DATA_GPU) == 0) {
		return false;
	}
	return mesh->gpu.vao != 0 && mesh->gpu.vbo != 0 && mesh->gpu.ebo != 0;
}

void se_mesh_discard_cpu_data(se_mesh *mesh) {
	s_assertf(mesh, "se_mesh_discard_cpu_data :: mesh is null");
	s_array_clear(&mesh->cpu.file_path);
	s_array_clear(&mesh->cpu.vertices);
	s_array_clear(&mesh->cpu.indices);
	mesh->data_flags = (se_mesh_data_flags)(mesh->data_flags & ~SE_MESH_DATA_CPU);
}

void se_model_discard_cpu_data(se_model *model) {
	s_assertf(model, "se_model_discard_cpu_data :: model is null");
	se_mesh *mesh = NULL;
	s_foreach(&model->meshes, mesh) {
		se_mesh_discard_cpu_data(mesh);
	}
}

static void se_mesh_release_gpu_data(se_mesh *mesh) {
	if (mesh == NULL) {
		return;
	}
	if (mesh->gpu.vao != 0) {
		glDeleteVertexArrays(1, &mesh->gpu.vao);
		mesh->gpu.vao = 0;
	}
	if (mesh->gpu.vbo != 0) {
		glDeleteBuffers(1, &mesh->gpu.vbo);
		mesh->gpu.vbo = 0;
	}
	if (mesh->gpu.ebo != 0) {
		glDeleteBuffers(1, &mesh->gpu.ebo);
		mesh->gpu.ebo = 0;
	}
	mesh->gpu.vertex_count = 0;
	mesh->gpu.index_count = 0;
	mesh->data_flags = (se_mesh_data_flags)(mesh->data_flags & ~SE_MESH_DATA_GPU);
}

static b8 se_mesh_cpu_reserve_vertices(se_mesh_cpu_data *cpu, const u32 needed) {
	if (cpu == NULL) {
		return false;
	}
	if ((sz)needed <= s_array_get_capacity(&cpu->vertices)) {
		return true;
	}

	u32 new_capacity = s_array_get_capacity(&cpu->vertices) > 0 ? (u32)s_array_get_capacity(&cpu->vertices) : 64;
	while (new_capacity < needed) {
		u32 next_capacity = new_capacity * 2;
		if (next_capacity < new_capacity) {
			new_capacity = needed;
			break;
		}
		new_capacity = next_capacity;
	}

	s_array(se_vertex_3d, grown_vertices) = {0};
	s_array_init(&grown_vertices);
	s_array_reserve(&grown_vertices, new_capacity);
	se_vertex_3d *vertex = NULL;
	s_foreach(&cpu->vertices, vertex) {
		s_handle vertex_handle = s_array_increment(&grown_vertices);
		se_vertex_3d *dst = s_array_get(&grown_vertices, vertex_handle);
		*dst = *vertex;
	}
	s_array_clear(&cpu->vertices);
	s_array_init(&cpu->vertices);
	s_array_reserve(&cpu->vertices, new_capacity);
	vertex = NULL;
	s_foreach(&grown_vertices, vertex) {
		s_handle vertex_handle = s_array_increment(&cpu->vertices);
		se_vertex_3d *dst = s_array_get(&cpu->vertices, vertex_handle);
		*dst = *vertex;
	}
	s_array_clear(&grown_vertices);
	return true;
}

static b8 se_mesh_cpu_reserve_indices(se_mesh_cpu_data *cpu, const u32 needed) {
	if (cpu == NULL) {
		return false;
	}
	if ((sz)needed <= s_array_get_capacity(&cpu->indices)) {
		return true;
	}

	u32 new_capacity = s_array_get_capacity(&cpu->indices) > 0 ? (u32)s_array_get_capacity(&cpu->indices) : 64;
	while (new_capacity < needed) {
		u32 next_capacity = new_capacity * 2;
		if (next_capacity < new_capacity) {
			new_capacity = needed;
			break;
		}
		new_capacity = next_capacity;
	}

	s_array(u32, grown_indices) = {0};
	s_array_init(&grown_indices);
	s_array_reserve(&grown_indices, new_capacity);
	u32 *index = NULL;
	s_foreach(&cpu->indices, index) {
		s_handle index_handle = s_array_increment(&grown_indices);
		u32 *dst = s_array_get(&grown_indices, index_handle);
		*dst = *index;
	}
	s_array_clear(&cpu->indices);
	s_array_init(&cpu->indices);
	s_array_reserve(&cpu->indices, new_capacity);
	index = NULL;
	s_foreach(&grown_indices, index) {
		s_handle index_handle = s_array_increment(&cpu->indices);
		u32 *dst = s_array_get(&cpu->indices, index_handle);
		*dst = *index;
	}
	s_array_clear(&grown_indices);
	return true;
}

static b8 se_mesh_cpu_set_file_path(se_mesh_cpu_data *cpu, const char *source_path) {
	if (cpu == NULL) {
		return false;
	}

	s_array_clear(&cpu->file_path);
	if (source_path == NULL || source_path[0] == '\0') {
		return true;
	}

	const sz source_path_size = strlen(source_path) + 1;
	s_array_init(&cpu->file_path);
	s_array_reserve(&cpu->file_path, source_path_size);
	for (sz i = 0; i < source_path_size; i++) {
		s_handle char_handle = s_array_increment(&cpu->file_path);
		c8 *dst = s_array_get(&cpu->file_path, char_handle);
		*dst = source_path[i];
	}

	return true;
}

static b8 se_mesh_create_gpu_data(se_mesh *mesh,
							const se_vertex_3d *vertices,
							const u32 *indices,
							const u32 vertex_count,
							const u32 index_count) {
	s_assertf(mesh, "se_mesh_create_gpu_data :: mesh is null");

	if (vertex_count == 0 || index_count == 0 || vertices == NULL || indices == NULL) {
		return false;
	}

	glGenVertexArrays(1, &mesh->gpu.vao);
	glGenBuffers(1, &mesh->gpu.vbo);
	glGenBuffers(1, &mesh->gpu.ebo);

	if (mesh->gpu.vao == 0 || mesh->gpu.vbo == 0 || mesh->gpu.ebo == 0) {
		se_mesh_release_gpu_data(mesh);
		return false;
	}

	glBindVertexArray(mesh->gpu.vao);

	glBindBuffer(GL_ARRAY_BUFFER, mesh->gpu.vbo);
	glBufferData(GL_ARRAY_BUFFER, (sz)vertex_count * sizeof(se_vertex_3d), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpu.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (sz)index_count * sizeof(u32), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)offsetof(se_vertex_3d, normal));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)offsetof(se_vertex_3d, uv));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	mesh->gpu.vertex_count = vertex_count;
	mesh->gpu.index_count = index_count;
	mesh->data_flags = (se_mesh_data_flags)(mesh->data_flags | SE_MESH_DATA_GPU);
	return true;
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
		se_log("se_model_load_obj :: out of memory for %s", label ? label : "unknown");
		return false;
	}

	*data = new_data;
	*capacity = new_capacity;
	return true;
}

// Helper function to finalize a mesh
static b8 se_mesh_finalize(se_mesh *mesh,
						   const char *source_path,
						   const se_shaders_ptr *shaders,
						   const u32 se_mesh_index,
						   const se_mesh_data_flags mesh_data_flags) {
	if (mesh == NULL) {
		return false;
	}
	if (!se_mesh_data_flags_are_valid(mesh_data_flags)) {
		return false;
	}

	if (s_array_get_size(&mesh->cpu.vertices) == 0 || s_array_get_size(&mesh->cpu.indices) == 0) {
		return false;
	}

	mesh->matrix = s_mat4_identity;
	mesh->gpu.vertex_count = (u32)s_array_get_size(&mesh->cpu.vertices);
	mesh->gpu.index_count = (u32)s_array_get_size(&mesh->cpu.indices);

	if (shaders != NULL && s_array_get_size(shaders) > 0) {
		se_shaders_ptr *shader_array = (se_shaders_ptr *)shaders;
		sz shader_count = s_array_get_size(shader_array);
		s_handle shader_entry = s_array_handle(shader_array, (u32)(se_mesh_index % shader_count));
		se_shader_handle *shader_ptr = s_array_get(shader_array, shader_entry);
		mesh->shader = shader_ptr ? *shader_ptr : S_HANDLE_NULL;
	} else {
		mesh->shader = S_HANDLE_NULL;
	}

	const se_vertex_3d *vertices = s_array_get_data(&mesh->cpu.vertices);
	const u32 *indices = s_array_get_data(&mesh->cpu.indices);
	if ((mesh_data_flags & SE_MESH_DATA_GPU) != 0 && !se_mesh_create_gpu_data(mesh, vertices, indices, mesh->gpu.vertex_count, mesh->gpu.index_count)) {
		se_mesh_release_gpu_data(mesh);
		se_mesh_discard_cpu_data(mesh);
		return false;
	}

	if ((mesh_data_flags & SE_MESH_DATA_CPU) != 0) {
		if (!se_mesh_cpu_set_file_path(&mesh->cpu, source_path)) {
			se_mesh_release_gpu_data(mesh);
			se_mesh_discard_cpu_data(mesh);
			return false;
		}
		mesh->data_flags = (se_mesh_data_flags)(mesh->data_flags | SE_MESH_DATA_CPU);
	} else {
		se_mesh_discard_cpu_data(mesh);
	}

	return true;
}

se_model_handle se_model_load_obj_ex(const char *path, const se_shaders_ptr *shaders, const se_mesh_data_flags mesh_data_flags) {
	se_context *ctx = se_current_context();
	if (!ctx || !path || !se_mesh_data_flags_are_valid(mesh_data_flags)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_model_handle model_handle = s_array_increment(&ctx->models);
	se_model *model = s_array_get(&ctx->models, model_handle);
	memset(model, 0, sizeof(*model));
	s_array_init(&model->meshes);
	s_array_reserve(&model->meshes, 64);

	char full_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_paths_resolve_resource_path(full_path, SE_MAX_PATH_LENGTH, path)) {
		se_model_cleanup(model);
		s_array_remove(&ctx->models, model_handle);
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	char *file_data = NULL;
	if (!s_file_read(full_path, &file_data, NULL)) {
		se_model_cleanup(model);
		s_array_remove(&ctx->models, model_handle);
		se_set_last_error(SE_RESULT_IO);
		return S_HANDLE_NULL;
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
	se_mesh *current_mesh = NULL;
	u32 current_mesh_index = 0;

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
		if (hse_faces && current_mesh != NULL && s_array_get_size(&current_mesh->cpu.vertices) > 0) {
			if (!se_mesh_finalize(current_mesh, full_path, shaders, current_mesh_index, mesh_data_flags)) {
				ok = false;
				break;
			}
			current_mesh = NULL;
			hse_faces = false;
		}

		// Get new object name
		sscanf(line, "%*s %255s", current_object);
	} else if (strncmp(line, "f ", 2) == 0) {
		// Face
		hse_faces = true;
		if (current_mesh == NULL) {
			s_handle mesh_handle = s_array_increment(&model->meshes);
			current_mesh = s_array_get(&model->meshes, mesh_handle);
			memset(current_mesh, 0, sizeof(*current_mesh));
			current_mesh_index = (u32)(s_array_get_size(&model->meshes) - 1);
		}

		u32 v1, v2, v3, n1, n2, n3, t1, t2, t3;
		i32 matches = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &t1, &n1,
					 &v2, &t2, &n2, &v3, &t3, &n3);

		if (matches == 9) {
			const u32 needed_vertices = (u32)s_array_get_size(&current_mesh->cpu.vertices) + 3;
			const u32 needed_indices = (u32)s_array_get_size(&current_mesh->cpu.indices) + 3;
			if (!se_mesh_cpu_reserve_vertices(&current_mesh->cpu, needed_vertices) ||
				!se_mesh_cpu_reserve_indices(&current_mesh->cpu, needed_indices)) {
				ok = false;
				break;
			}
			for (i32 i = 0; i < 3; i++) {
				u32 vi = (i == 0) ? v1 - 1 : (i == 1) ? v2 - 1 : v3 - 1;
				u32 ni = (i == 0) ? n1 - 1 : (i == 1) ? n2 - 1 : n3 - 1;
				u32 ti = (i == 0) ? t1 - 1 : (i == 1) ? t2 - 1 : t3 - 1;

				if (vi >= vertex_count || ni >= normal_count || ti >= uv_count) {
					se_log("se_model_load_obj :: OBJ file contains invalid face indices");
					continue;
				}

				const u32 next_index = (u32)s_array_get_size(&current_mesh->cpu.vertices);
				s_handle vertex_handle = s_array_increment(&current_mesh->cpu.vertices);
				se_vertex_3d *new_vertex = s_array_get(&current_mesh->cpu.vertices, vertex_handle);
				new_vertex->position = temp_vertices[vi];
				new_vertex->normal = temp_normals[ni];
				new_vertex->uv = temp_uvs[ti];

				s_handle index_handle = s_array_increment(&current_mesh->cpu.indices);
				u32 *new_index = s_array_get(&current_mesh->cpu.indices, index_handle);
				*new_index = next_index;
			}
		} else {
			matches = sscanf(line, "f %d//%d %d//%d %d//%d", &v1, &n1, &v2, &n2, &v3, &n3);
			if (matches == 6) {
				const u32 needed_vertices = (u32)s_array_get_size(&current_mesh->cpu.vertices) + 3;
				const u32 needed_indices = (u32)s_array_get_size(&current_mesh->cpu.indices) + 3;
				if (!se_mesh_cpu_reserve_vertices(&current_mesh->cpu, needed_vertices) ||
					!se_mesh_cpu_reserve_indices(&current_mesh->cpu, needed_indices)) {
					ok = false;
					break;
				}
				for (i32 i = 0; i < 3; i++) {
					u32 vi = (i == 0) ? v1 - 1 : (i == 1) ? v2 - 1 : v3 - 1;
					u32 ni = (i == 0) ? n1 - 1 : (i == 1) ? n2 - 1 : n3 - 1;

					if (vi >= vertex_count || ni >= normal_count) {
						se_log("se_model_load_obj :: OBJ file contains invalid face indices");
						continue;
					}

				const u32 next_index = (u32)s_array_get_size(&current_mesh->cpu.vertices);
				s_handle vertex_handle = s_array_increment(&current_mesh->cpu.vertices);
				se_vertex_3d *new_vertex = s_array_get(&current_mesh->cpu.vertices, vertex_handle);
				new_vertex->position = temp_vertices[vi];
				new_vertex->normal = temp_normals[ni];
				new_vertex->uv = (s_vec2){0.0f, 0.0f};

				s_handle index_handle = s_array_increment(&current_mesh->cpu.indices);
				u32 *new_index = s_array_get(&current_mesh->cpu.indices, index_handle);
				*new_index = next_index;
			}
		}
	}
	}
	}

	// Finalize the last mesh
	if (ok && hse_faces && current_mesh != NULL && s_array_get_size(&current_mesh->cpu.vertices) > 0) {
		if (!se_mesh_finalize(current_mesh, full_path, shaders, current_mesh_index, mesh_data_flags)) {
			ok = false;
		}
		current_mesh = NULL;
	}

	free(file_data);

	// Cleanup temporary arrays
	free(temp_vertices);
	free(temp_normals);
	free(temp_uvs);

	if (!ok) {
		se_model_cleanup(model);
		s_array_remove(&ctx->models, model_handle);
		se_set_last_error(SE_RESULT_IO);
		return S_HANDLE_NULL;
	}

	// If no meshes were created, create a default one
	if (s_array_get_size(&model->meshes) == 0) {
		s_array_clear(&model->meshes);
		s_array_remove(&ctx->models, model_handle);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}

	se_set_last_error(SE_RESULT_OK);
	return model_handle;
}

se_model_handle se_model_load_obj(const char *path, const se_shaders_ptr *shaders) {
	return se_model_load_obj_ex(path, shaders, SE_MESH_DATA_GPU);
}

se_model_handle se_model_load_obj_simple_ex(const char *obj_path, const char *vertex_shader_path, const char *fragment_shader_path, const se_mesh_data_flags mesh_data_flags) {
	se_context *ctx = se_current_context();
	if (!ctx || !obj_path || !vertex_shader_path || !fragment_shader_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	se_shader_handle shader = se_shader_load(vertex_shader_path, fragment_shader_path);
	if (shader == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}

	se_shaders_ptr shaders = {0};
	s_array_init(&shaders);
	s_array_reserve(&shaders, 1);
	{
		s_handle shader_handle = s_array_increment(&shaders);
		se_shader_handle *shader_slot = s_array_get(&shaders, shader_handle);
		*shader_slot = shader;
	}

	se_model_handle model = se_model_load_obj_ex(obj_path, &shaders, mesh_data_flags);
	s_array_clear(&shaders);
	if (model == S_HANDLE_NULL) {
		se_shader_destroy(shader);
		return S_HANDLE_NULL;
	}

	se_set_last_error(SE_RESULT_OK);
	se_log("se_model_load_obj_simple_ex :: loaded model: %s", obj_path);
	return model;
}

se_model_handle se_model_load_obj_simple(const char *obj_path, const char *vertex_shader_path, const char *fragment_shader_path) {
	return se_model_load_obj_simple_ex(obj_path, vertex_shader_path, fragment_shader_path, SE_MESH_DATA_GPU);
}

void se_model_destroy(const se_model_handle model) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_model_destroy :: ctx is null");
	se_model *model_ptr = se_model_from_handle(ctx, model);
	s_assertf(model_ptr, "se_model_destroy :: model is null");
	se_model_cleanup(model_ptr);
	s_array_remove(&ctx->models, model);
}

void se_model_render(const se_model_handle model, const se_camera_handle camera) {
	se_context *ctx = se_current_context();
	se_model *model_ptr = se_model_from_handle(ctx, model);
	s_assertf(model_ptr, "se_model_render :: model is null");
	// set up global view/proj once per frame
	const s_mat4 proj = se_camera_get_projection_matrix(camera);
	const s_mat4 view = se_camera_get_view_matrix(camera);
	se_mesh *mesh = NULL;
	s_foreach(&model_ptr->meshes, mesh) {
		if (!se_mesh_has_gpu_data(mesh) || mesh->gpu.index_count == 0 || mesh->shader == S_HANDLE_NULL) {
			continue;
		}
		se_shader *sh = s_array_get(&ctx->shaders, mesh->shader);
		if (!sh) {
			continue;
		}

		se_shader_use(mesh->shader, true, true);

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
		glBindVertexArray(mesh->gpu.vao);
		glDrawElements(GL_TRIANGLES, mesh->gpu.index_count, GL_UNSIGNED_INT, 0);
	}
	// unbind
	glBindVertexArray(0);
	glUseProgram(0);
}

static void se_model_cleanup(se_model *model) {
	se_mesh *mesh = NULL;
	s_foreach(&model->meshes, mesh) {
		se_mesh_release_gpu_data(mesh);
		se_mesh_discard_cpu_data(mesh);
	}
	s_array_clear(&model->meshes);
}

void se_model_translate(const se_model_handle model, const s_vec3 *v) {
	se_context *ctx = se_current_context();
	se_model *model_ptr = se_model_from_handle(ctx, model);
	s_assertf(model_ptr, "se_model_translate :: model is null");
	se_mesh *mesh = NULL;
	s_foreach(&model_ptr->meshes, mesh) {
		se_mesh_translate(mesh, v);
	}
}

void se_model_rotate(const se_model_handle model, const s_vec3 *v) {
	se_context *ctx = se_current_context();
	se_model *model_ptr = se_model_from_handle(ctx, model);
	s_assertf(model_ptr, "se_model_rotate :: model is null");
	se_mesh *mesh = NULL;
	s_foreach(&model_ptr->meshes, mesh) {
		se_mesh_rotate(mesh, v);
	}
}

void se_model_scale(const se_model_handle model, const s_vec3 *v) {
	se_context *ctx = se_current_context();
	se_model *model_ptr = se_model_from_handle(ctx, model);
	s_assertf(model_ptr, "se_model_scale :: model is null");
	se_mesh *mesh = NULL;
	s_foreach(&model_ptr->meshes, mesh) {
		se_mesh_scale(mesh, v);
	}
}

sz se_model_get_mesh_count(const se_model_handle model) {
	se_context *ctx = se_current_context();
	se_model *model_ptr = se_model_from_handle(ctx, model);
	if (!model_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}
	se_set_last_error(SE_RESULT_OK);
	return s_array_get_size(&model_ptr->meshes);
}

se_shader_handle se_model_get_mesh_shader(const se_model_handle model, const sz mesh_index) {
	se_context *ctx = se_current_context();
	se_model *model_ptr = se_model_from_handle(ctx, model);
	if (!model_ptr || mesh_index >= s_array_get_size(&model_ptr->meshes)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}
	se_mesh *mesh = s_array_get(&model_ptr->meshes, s_array_handle(&model_ptr->meshes, (u32)mesh_index));
	if (!mesh) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return mesh->shader;
}


void se_mesh_instance_create(se_mesh_instance *out_instance, const se_mesh *mesh, const u32 instance_count) {
	s_assertf(out_instance, "se_mesh_instance_create :: out_instance is null");
	s_assertf(mesh, "se_mesh_instance_create :: mesh is null");

	memset(out_instance, 0, sizeof(se_mesh_instance));
	if (!se_mesh_has_gpu_data(mesh)) {
		return;
	}

	glGenVertexArrays(1, &out_instance->vao);
	glBindVertexArray(out_instance->vao);

	glBindBuffer(GL_ARRAY_BUFFER, mesh->gpu.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpu.ebo);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void *)offsetof(se_vertex_3d, uv));

	glBindVertexArray(0);

	if (instance_count > 0) {
		s_array_init(&out_instance->instance_buffers);
		s_array_reserve(&out_instance->instance_buffers, instance_count);
		out_instance->instance_buffers_dirty = true;
	}
}

void se_mesh_instance_add_buffer(se_mesh_instance *instance, const s_mat4 *buffer, const sz instance_count) {
	s_assertf(instance, "se_mesh_instance_add_buffer :: instance is null");
	s_assertf(buffer, "se_mesh_instance_add_buffer :: buffer is null");
	if (instance->vao == 0) {
		return;
	}

	glBindVertexArray(instance->vao);

	s_handle buffer_handle = s_array_increment(&instance->instance_buffers);
	se_instance_buffer *new_buffer = s_array_get(&instance->instance_buffers, buffer_handle);
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
	if (instance->vao == 0) {
		return;
	}
	if (!instance->instance_buffers_dirty) {
		return;
	}

	se_instance_buffer *current_buffer = NULL;
	s_foreach(&instance->instance_buffers, current_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, current_buffer->vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, current_buffer->buffer_size, current_buffer->buffer_ptr);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	instance->instance_buffers_dirty = false;
}

void se_mesh_instance_destroy(se_mesh_instance *instance) {
	s_assertf(instance, "se_mesh_instance_destroy :: instance is null");
	se_instance_buffer *current_buffer = NULL;
	s_foreach(&instance->instance_buffers, current_buffer) {
		glDeleteBuffers(1, &current_buffer->vbo);
	}
	s_array_clear(&instance->instance_buffers);
	if (instance->vao) {
		glDeleteVertexArrays(1, &instance->vao);
		instance->vao = 0;
	}
}
