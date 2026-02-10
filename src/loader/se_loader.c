// Syphax-Engine - Ougi Washi

#include "loader/se_loader.h"

#include "render/se_gl.h"
#include "syphax/s_files.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const u8 *data;
	u32 stride;
} se_gltf_accessor_view;

typedef struct {
	se_render_handle *render_handle;
	se_scene_handle *scene_handle;
	se_scene_3d *scene;
	const se_gltf_asset *asset;
	se_shader *mesh_shader;
	se_texture *default_texture;
	se_texture_wrap wrap;
	se_models_ptr *model_cache;
	se_textures_ptr *texture_cache;
	se_shaders_ptr *material_shader_cache;
	sz added_objects;
} se_gltf_scene_build_context;

static b8 se_loader_decode_base64(const char *in, u8 **out_data, sz *out_size) {
	if (in == NULL || out_data == NULL || out_size == NULL) return false;
	sz len = strlen(in);
	while (len > 0 && isspace((unsigned char)in[len - 1])) len--;
	if (len == 0) return false;
	if ((len % 4) != 0) return false;

	sz padding = 0;
	if (len >= 1 && in[len - 1] == '=') padding++;
	if (len >= 2 && in[len - 2] == '=') padding++;

	sz out_len = (len / 4) * 3;
	if (padding > 0) out_len -= padding;

	u8 *out = (u8 *)malloc(out_len ? out_len : 1);
	if (out == NULL) return false;

	sz out_pos = 0;
	for (sz i = 0; i < len; i += 4) {
		u32 val = 0;
		u8 quad[4] = {0, 0, 0, 0};
		for (u8 j = 0; j < 4; j++) {
			char c = in[i + j];
			u8 v = 0;
			if (c >= 'A' && c <= 'Z') v = (u8)(c - 'A');
			else if (c >= 'a' && c <= 'z') v = (u8)(c - 'a' + 26);
			else if (c >= '0' && c <= '9') v = (u8)(c - '0' + 52);
			else if (c == '+') v = 62;
			else if (c == '/') v = 63;
			else if (c == '=') v = 0;
			else {
				free(out);
				return false;
			}
			quad[j] = v;
		}
		val = ((u32)quad[0] << 18) | ((u32)quad[1] << 12) | ((u32)quad[2] << 6) | (u32)quad[3];
		out[out_pos++] = (u8)((val >> 16) & 0xFF);
		if (in[i + 2] != '=') out[out_pos++] = (u8)((val >> 8) & 0xFF);
		if (in[i + 3] != '=') out[out_pos++] = (u8)(val & 0xFF);
	}

	*out_data = out;
	*out_size = out_pos;
	return true;
}

static b8 se_loader_parse_data_uri(const char *uri, char **out_mime, u8 **out_data, sz *out_size) {
	if (uri == NULL || strncmp(uri, "data:", 5) != 0) return false;
	const char *comma = strchr(uri, ',');
	if (comma == NULL) return false;
	const char *meta = uri + 5;
	sz meta_len = (sz)(comma - meta);
	b8 is_base64 = false;
	const char *base64_tag = ";base64";
	if (meta_len >= 7 && strncmp(comma - 7, base64_tag, 7) == 0) is_base64 = true;
	if (!is_base64) return false;
	if (out_mime) {
		sz mime_len = meta_len;
		if (mime_len >= 7 && strncmp(meta + mime_len - 7, base64_tag, 7) == 0) mime_len -= 7;
		if (mime_len > 0) {
			char *mime = (char *)malloc(mime_len + 1);
			if (mime == NULL) return false;
			memcpy(mime, meta, mime_len);
			mime[mime_len] = '\0';
			*out_mime = mime;
		} else {
			*out_mime = NULL;
		}
	}
	return se_loader_decode_base64(comma + 1, out_data, out_size);
}

static b8 se_loader_path_is_absolute(const char *path) {
	if (path == NULL || path[0] == '\0') return false;
	if (s_path_is_sep(path[0])) return true;
#ifdef _WIN32
	if (path[0] && path[1] == ':' && s_path_is_sep(path[2])) return true;
#endif
	return false;
}

static b8 se_gltf_mesh_finalize(se_mesh *mesh, se_vertex_3d *vertices, u32 *indices, u32 vertex_count, u32 index_count) {
	if (mesh == NULL || vertices == NULL || indices == NULL || vertex_count == 0 || index_count == 0) {
		return false;
	}

	mesh->vertices = malloc(vertex_count * sizeof(se_vertex_3d));
	mesh->indices = malloc(index_count * sizeof(u32));
	if (mesh->vertices == NULL || mesh->indices == NULL) {
		free(mesh->vertices);
		free(mesh->indices);
		mesh->vertices = NULL;
		mesh->indices = NULL;
		return false;
	}

	memcpy(mesh->vertices, vertices, vertex_count * sizeof(se_vertex_3d));
	memcpy(mesh->indices, indices, index_count * sizeof(u32));
	mesh->vertex_count = vertex_count;
	mesh->index_count = index_count;
	mesh->matrix = s_mat4_identity;
	mesh->shader = NULL;
	mesh->texture_id = 0;

	glGenVertexArrays(1, &mesh->vao);
	glGenBuffers(1, &mesh->vbo);
	glGenBuffers(1, &mesh->ebo);
	glBindVertexArray(mesh->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(se_vertex_3d), mesh->vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u32), mesh->indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)offsetof(se_vertex_3d, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (void *)offsetof(se_vertex_3d, uv));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	return true;
}

static void se_gltf_mesh_release(se_mesh *mesh) {
	if (mesh == NULL) return;
	if (mesh->vao != 0) glDeleteVertexArrays(1, &mesh->vao);
	if (mesh->vbo != 0) glDeleteBuffers(1, &mesh->vbo);
	if (mesh->ebo != 0) glDeleteBuffers(1, &mesh->ebo);
	free(mesh->vertices);
	free(mesh->indices);
	memset(mesh, 0, sizeof(*mesh));
}

static void se_gltf_model_release(se_model *model) {
	if (model == NULL) return;
	s_foreach(&model->meshes, i) {
		se_mesh *mesh = s_array_get(&model->meshes, i);
		se_gltf_mesh_release(mesh);
	}
	s_array_clear(&model->meshes);
}

static void se_gltf_model_rollback(se_render_handle *render_handle, se_model *model) {
	if (render_handle == NULL || model == NULL) return;
	se_gltf_model_release(model);
	memset(model, 0, sizeof(*model));
	if (render_handle->models.size > 0) {
		se_model *last = s_array_get(&render_handle->models, render_handle->models.size - 1);
		if (last == model) {
			render_handle->models.size--;
		}
	}
}

static b8 se_gltf_get_accessor_view(const se_gltf_asset *asset, const se_gltf_accessor *accessor, const u32 element_size, const u32 default_stride, const u32 required_count, se_gltf_accessor_view *out_view) {
	if (asset == NULL || accessor == NULL || out_view == NULL || element_size == 0) return false;
	if (!accessor->has_buffer_view) return false;
	if (accessor->buffer_view < 0 || (sz)accessor->buffer_view >= asset->buffer_views.size) return false;

	se_gltf_buffer_view *view = s_array_get(&asset->buffer_views, accessor->buffer_view);
	if (view->buffer < 0 || (sz)view->buffer >= asset->buffers.size) return false;

	se_gltf_buffer *buffer = s_array_get(&asset->buffers, view->buffer);
	if (buffer->data == NULL || buffer->data_size == 0) return false;

	const u32 stride = view->has_byte_stride ? view->byte_stride : default_stride;
	if (stride < element_size) return false;

	const u32 count = required_count ? required_count : accessor->count;
	if (count == 0 || count > accessor->count) return false;

	const sz start = (sz)view->byte_offset + (sz)accessor->byte_offset;
	if (start >= buffer->data_size) return false;

	const sz total_size = (sz)(count - 1) * (sz)stride + element_size;
	if (total_size > (buffer->data_size - start)) return false;

	out_view->data = buffer->data + start;
	out_view->stride = stride;
	return true;
}

static se_texture *se_gltf_texture_from_cache(se_render_handle *render_handle,
									 const se_gltf_asset *asset,
									 se_textures_ptr *texture_cache,
									 const i32 texture_index,
									 const se_texture_wrap wrap) {
	if (render_handle == NULL || asset == NULL || texture_cache == NULL) return NULL;
	if (texture_index < 0 || (sz)texture_index >= asset->textures.size || (sz)texture_index >= texture_cache->size) {
		return NULL;
	}

	se_texture **cached = s_array_get(texture_cache, texture_index);
	if (cached == NULL) return NULL;
	if (*cached == NULL) {
		*cached = se_gltf_texture_load(render_handle, asset, texture_index, wrap);
	}
	return *cached;
}

static f32 se_gltf_clamp(const f32 value, const f32 min_value, const f32 max_value) {
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static i32 se_gltf_alpha_mode_to_uniform(const se_gltf_material *material) {
	if (material == NULL || !material->has_alpha_mode || material->alpha_mode == NULL) {
		return 0;
	}
	if (strcmp(material->alpha_mode, "MASK") == 0) {
		return 1;
	}
	if (strcmp(material->alpha_mode, "BLEND") == 0) {
		return 2;
	}
	return 0;
}

static void se_gltf_set_default_material_uniforms(se_shader *shader, se_texture *default_texture) {
	if (shader == NULL) return;

	const u32 default_texture_id = default_texture ? default_texture->id : 0;
	const s_vec4 base_color_factor = s_vec4(1.0f, 1.0f, 1.0f, 1.0f);
	const s_vec3 emissive_factor = s_vec3(0.0f, 0.0f, 0.0f);

	se_shader_set_texture(shader, "u_texture", default_texture_id);
	se_shader_set_texture(shader, "u_metallic_roughness_texture", default_texture_id);
	se_shader_set_texture(shader, "u_occlusion_texture", default_texture_id);
	se_shader_set_texture(shader, "u_emissive_texture", default_texture_id);
	se_shader_set_vec4(shader, "u_base_color_factor", &base_color_factor);
	se_shader_set_float(shader, "u_metallic_factor", 1.0f);
	se_shader_set_float(shader, "u_roughness_factor", 1.0f);
	se_shader_set_float(shader, "u_ao_factor", 1.0f);
	se_shader_set_vec3(shader, "u_emissive_factor", &emissive_factor);
	se_shader_set_int(shader, "u_has_metallic_roughness_texture", 0);
	se_shader_set_int(shader, "u_has_occlusion_texture", 0);
	se_shader_set_int(shader, "u_has_emissive_texture", 0);
	se_shader_set_int(shader, "u_alpha_mode", 0);
	se_shader_set_float(shader, "u_alpha_cutoff", 0.5f);
}

static se_shader *se_gltf_get_material_shader(se_render_handle *render_handle,
									   const se_gltf_asset *asset,
									   const i32 material_index,
									   se_shader *mesh_shader,
									   se_texture *default_texture,
									   const se_texture_wrap wrap,
									   se_textures_ptr *texture_cache,
									   se_shaders_ptr *material_shader_cache) {
	if (render_handle == NULL || asset == NULL || mesh_shader == NULL || texture_cache == NULL || material_shader_cache == NULL) {
		return mesh_shader;
	}
	if (material_index < 0 || (sz)material_index >= asset->materials.size || (sz)material_index >= material_shader_cache->size) {
		return mesh_shader;
	}

	se_shader **cached_shader = s_array_get(material_shader_cache, material_index);
	if (cached_shader == NULL) return mesh_shader;
	if (*cached_shader != NULL) return *cached_shader;

	se_shader *material_shader = se_shader_load(render_handle, mesh_shader->vertex_path, mesh_shader->fragment_path);
	if (material_shader == NULL) {
		return mesh_shader;
	}

	se_gltf_set_default_material_uniforms(material_shader, default_texture);

	se_gltf_material *material = s_array_get(&asset->materials, material_index);
	if (material != NULL) {
		s_vec4 base_color_factor = s_vec4(1.0f, 1.0f, 1.0f, 1.0f);
		f32 metallic_factor = 1.0f;
		f32 roughness_factor = 1.0f;
		f32 ao_factor = 1.0f;
		s_vec3 emissive_factor = s_vec3(0.0f, 0.0f, 0.0f);

		i32 has_metallic_roughness_texture = 0;
		i32 has_occlusion_texture = 0;
		i32 has_emissive_texture = 0;

		u32 metallic_roughness_texture_id = default_texture ? default_texture->id : 0;
		u32 occlusion_texture_id = default_texture ? default_texture->id : 0;
		u32 emissive_texture_id = default_texture ? default_texture->id : 0;

		if (material->has_pbr_metallic_roughness) {
			se_gltf_pbr_metallic_roughness *pbr = &material->pbr_metallic_roughness;
			if (pbr->has_base_color_factor) {
				base_color_factor = s_vec4(
					pbr->base_color_factor[0],
					pbr->base_color_factor[1],
					pbr->base_color_factor[2],
					pbr->base_color_factor[3]);
			}
			if (pbr->has_metallic_factor) {
				metallic_factor = pbr->metallic_factor;
			}
			if (pbr->has_roughness_factor) {
				roughness_factor = pbr->roughness_factor;
			}

			if (pbr->has_metallic_roughness_texture) {
				se_texture *tex = se_gltf_texture_from_cache(
					render_handle,
					asset,
					texture_cache,
					pbr->metallic_roughness_texture.index,
					wrap);
				if (tex != NULL) {
					metallic_roughness_texture_id = tex->id;
					has_metallic_roughness_texture = 1;
				}
			}
		}

		if (material->has_occlusion_texture) {
			if (material->occlusion_texture.has_strength) {
				ao_factor = material->occlusion_texture.strength;
			}
			se_texture *tex = se_gltf_texture_from_cache(
				render_handle,
				asset,
				texture_cache,
				material->occlusion_texture.info.index,
				wrap);
			if (tex != NULL) {
				occlusion_texture_id = tex->id;
				has_occlusion_texture = 1;
			}
		}

		if (material->has_emissive_factor) {
			emissive_factor = s_vec3(
				material->emissive_factor[0],
				material->emissive_factor[1],
				material->emissive_factor[2]);
		}

		if (material->has_emissive_texture) {
			se_texture *tex = se_gltf_texture_from_cache(
				render_handle,
				asset,
				texture_cache,
				material->emissive_texture.index,
				wrap);
			if (tex != NULL) {
				emissive_texture_id = tex->id;
				has_emissive_texture = 1;
			}
		}

		metallic_factor = se_gltf_clamp(metallic_factor, 0.0f, 1.0f);
		roughness_factor = se_gltf_clamp(roughness_factor, 0.04f, 1.0f);
		ao_factor = se_gltf_clamp(ao_factor, 0.0f, 1.0f);
		base_color_factor.x = se_gltf_clamp(base_color_factor.x, 0.0f, 1.0f);
		base_color_factor.y = se_gltf_clamp(base_color_factor.y, 0.0f, 1.0f);
		base_color_factor.z = se_gltf_clamp(base_color_factor.z, 0.0f, 1.0f);
		base_color_factor.w = se_gltf_clamp(base_color_factor.w, 0.0f, 1.0f);
		emissive_factor.x = se_gltf_clamp(emissive_factor.x, 0.0f, 32.0f);
		emissive_factor.y = se_gltf_clamp(emissive_factor.y, 0.0f, 32.0f);
		emissive_factor.z = se_gltf_clamp(emissive_factor.z, 0.0f, 32.0f);

		f32 alpha_cutoff = material->has_alpha_cutoff ? material->alpha_cutoff : 0.5f;
		alpha_cutoff = se_gltf_clamp(alpha_cutoff, 0.0f, 1.0f);

		se_shader_set_vec4(material_shader, "u_base_color_factor", &base_color_factor);
		se_shader_set_float(material_shader, "u_metallic_factor", metallic_factor);
		se_shader_set_float(material_shader, "u_roughness_factor", roughness_factor);
		se_shader_set_float(material_shader, "u_ao_factor", ao_factor);
		se_shader_set_vec3(material_shader, "u_emissive_factor", &emissive_factor);

		se_shader_set_texture(material_shader, "u_metallic_roughness_texture", metallic_roughness_texture_id);
		se_shader_set_texture(material_shader, "u_occlusion_texture", occlusion_texture_id);
		se_shader_set_texture(material_shader, "u_emissive_texture", emissive_texture_id);

		se_shader_set_int(material_shader, "u_has_metallic_roughness_texture", has_metallic_roughness_texture);
		se_shader_set_int(material_shader, "u_has_occlusion_texture", has_occlusion_texture);
		se_shader_set_int(material_shader, "u_has_emissive_texture", has_emissive_texture);
		se_shader_set_int(material_shader, "u_alpha_mode", se_gltf_alpha_mode_to_uniform(material));
		se_shader_set_float(material_shader, "u_alpha_cutoff", alpha_cutoff);
	}

	*cached_shader = material_shader;
	return material_shader;
}

static void se_gltf_apply_model_materials(se_render_handle *render_handle, const se_gltf_asset *asset, const se_gltf_mesh *gltf_mesh, se_model *model, se_shader *mesh_shader, se_texture *default_texture, const se_texture_wrap wrap, se_textures_ptr *texture_cache, se_shaders_ptr *material_shader_cache) {
	if (render_handle == NULL || asset == NULL || gltf_mesh == NULL || model == NULL || texture_cache == NULL) return;

	if (mesh_shader != NULL) {
		se_gltf_set_default_material_uniforms(mesh_shader, default_texture);
	}

	s_foreach(&model->meshes, prim_index) {
		se_mesh *mesh = s_array_get(&model->meshes, prim_index);
		if (mesh == NULL) {
			continue;
		}

		se_texture *mesh_texture = default_texture;
		se_shader *mesh_material_shader = mesh_shader;

		if (prim_index < gltf_mesh->primitives.size) {
			se_gltf_primitive *prim = s_array_get(&gltf_mesh->primitives, prim_index);
			if (prim != NULL && prim->has_material && prim->material >= 0 && (sz)prim->material < asset->materials.size) {
				se_gltf_material *material = s_array_get(&asset->materials, prim->material);
				if (material != NULL && material->has_pbr_metallic_roughness && material->pbr_metallic_roughness.has_base_color_texture) {
					se_texture *base_color_texture = se_gltf_texture_from_cache(
						render_handle,
						asset,
						texture_cache,
						material->pbr_metallic_roughness.base_color_texture.index,
						wrap);
					if (base_color_texture != NULL) {
						mesh_texture = base_color_texture;
					}
				}

				mesh_material_shader = se_gltf_get_material_shader(
					render_handle,
					asset,
					prim->material,
					mesh_shader,
					default_texture,
					wrap,
					texture_cache,
					material_shader_cache);
			}
		}

		if (mesh_material_shader != NULL) {
			mesh->shader = mesh_material_shader;
		}
		mesh->texture_id = mesh_texture ? mesh_texture->id : 0;
	}
}

static s_mat4 se_gltf_node_matrix_from_array(const f32 matrix_values[16]) {
	s_mat4 matrix = s_mat4_identity;
	for (u32 c = 0; c < 4; c++) {
		for (u32 r = 0; r < 4; r++) {
			matrix.m[c][r] = matrix_values[(c * 4) + r];
		}
	}
	return matrix;
}

static s_mat4 se_gltf_node_rotation_matrix(const f32 rotation[4]) {
	const f32 x = rotation[0];
	const f32 y = rotation[1];
	const f32 z = rotation[2];
	const f32 w = rotation[3];

	const f32 xx = x * x;
	const f32 yy = y * y;
	const f32 zz = z * z;
	const f32 xy = x * y;
	const f32 xz = x * z;
	const f32 yz = y * z;
	const f32 wx = w * x;
	const f32 wy = w * y;
	const f32 wz = w * z;

	return s_mat4(
		1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
		2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx), 0.0f,
		2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

static s_mat4 se_gltf_node_local_transform(const se_gltf_node *node) {
	if (node == NULL) return s_mat4_identity;
	if (node->has_matrix) {
		return se_gltf_node_matrix_from_array(node->matrix);
	}

	s_vec3 translation = s_vec3(0.0f, 0.0f, 0.0f);
	if (node->has_translation) {
		translation = s_vec3(node->translation[0], node->translation[1], node->translation[2]);
	}

	s_vec3 scale = s_vec3(1.0f, 1.0f, 1.0f);
	if (node->has_scale) {
		scale = s_vec3(node->scale[0], node->scale[1], node->scale[2]);
	}

	s_mat4 translation_matrix = s_mat4_identity;
	s_mat4_set_translation(&translation_matrix, &translation);

	s_mat4 rotation_matrix = s_mat4_identity;
	if (node->has_rotation) {
		rotation_matrix = se_gltf_node_rotation_matrix(node->rotation);
	}

	s_mat4 scale_matrix = s_mat4(
		scale.x, 0.0f, 0.0f, 0.0f,
		0.0f, scale.y, 0.0f, 0.0f,
		0.0f, 0.0f, scale.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	s_mat4 local = s_mat4_mul(&translation_matrix, &rotation_matrix);
	return s_mat4_mul(&local, &scale_matrix);
}

static void se_gltf_bounds_include_point(s_vec3 *min_bounds, s_vec3 *max_bounds, const s_vec3 *point, b8 *has_bounds) {
	if (min_bounds == NULL || max_bounds == NULL || point == NULL || has_bounds == NULL) return;
	if (!*has_bounds) {
		*min_bounds = *point;
		*max_bounds = *point;
		*has_bounds = true;
		return;
	}

	if (point->x < min_bounds->x) min_bounds->x = point->x;
	if (point->y < min_bounds->y) min_bounds->y = point->y;
	if (point->z < min_bounds->z) min_bounds->z = point->z;
	if (point->x > max_bounds->x) max_bounds->x = point->x;
	if (point->y > max_bounds->y) max_bounds->y = point->y;
	if (point->z > max_bounds->z) max_bounds->z = point->z;
}

static s_vec3 se_gltf_transform_point(const s_mat4 *transform, const s_vec3 *point) {
	if (transform == NULL || point == NULL) return s_vec3(0.0f, 0.0f, 0.0f);
	return s_vec3(
		(transform->m[0][0] * point->x) + (transform->m[1][0] * point->y) + (transform->m[2][0] * point->z) + transform->m[3][0],
		(transform->m[0][1] * point->x) + (transform->m[1][1] * point->y) + (transform->m[2][1] * point->z) + transform->m[3][1],
		(transform->m[0][2] * point->x) + (transform->m[1][2] * point->y) + (transform->m[2][2] * point->z) + transform->m[3][2]);
}

static void se_gltf_bounds_include_transformed_aabb(const s_vec3 *local_min, const s_vec3 *local_max, const s_mat4 *transform, s_vec3 *min_bounds, s_vec3 *max_bounds, b8 *has_bounds) {
	if (local_min == NULL || local_max == NULL || transform == NULL || min_bounds == NULL || max_bounds == NULL || has_bounds == NULL) {
		return;
	}

	for (u32 i = 0; i < 8; i++) {
		s_vec3 corner = s_vec3(
			(i & 1) ? local_max->x : local_min->x,
			(i & 2) ? local_max->y : local_min->y,
			(i & 4) ? local_max->z : local_min->z);
		s_vec3 world_corner = se_gltf_transform_point(transform, &corner);
		se_gltf_bounds_include_point(min_bounds, max_bounds, &world_corner, has_bounds);
	}
}

static b8 se_gltf_mesh_compute_local_bounds(const se_gltf_asset *asset, const i32 mesh_index, s_vec3 *out_min, s_vec3 *out_max) {
	if (asset == NULL || out_min == NULL || out_max == NULL) return false;
	if (mesh_index < 0 || (sz)mesh_index >= asset->meshes.size) return false;

	se_gltf_mesh *mesh = s_array_get(&asset->meshes, mesh_index);
	b8 has_bounds = false;
	s_vec3 min_bounds = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 max_bounds = s_vec3(0.0f, 0.0f, 0.0f);

	s_foreach(&mesh->primitives, prim_index) {
		se_gltf_primitive *prim = s_array_get(&mesh->primitives, prim_index);
		se_gltf_attribute *position_attr = NULL;

		s_foreach(&prim->attributes.attributes, attr_index) {
			se_gltf_attribute *attr = s_array_get(&prim->attributes.attributes, attr_index);
			if (strcmp(attr->name, "POSITION") == 0) {
				position_attr = attr;
				break;
			}
		}

		if (position_attr == NULL || position_attr->accessor < 0 || (sz)position_attr->accessor >= asset->accessors.size) {
			continue;
		}

		se_gltf_accessor *position_accessor = s_array_get(&asset->accessors, position_attr->accessor);
		if (!position_accessor->has_min || !position_accessor->has_max) {
			continue;
		}

		if (!isfinite(position_accessor->min_values[0]) || !isfinite(position_accessor->min_values[1]) || !isfinite(position_accessor->min_values[2]) ||
			!isfinite(position_accessor->max_values[0]) || !isfinite(position_accessor->max_values[1]) || !isfinite(position_accessor->max_values[2])) {
			continue;
		}

		s_vec3 local_min = s_vec3(position_accessor->min_values[0], position_accessor->min_values[1], position_accessor->min_values[2]);
		s_vec3 local_max = s_vec3(position_accessor->max_values[0], position_accessor->max_values[1], position_accessor->max_values[2]);

		se_gltf_bounds_include_point(&min_bounds, &max_bounds, &local_min, &has_bounds);
		se_gltf_bounds_include_point(&min_bounds, &max_bounds, &local_max, &has_bounds);
	}

	if (!has_bounds) return false;
	*out_min = min_bounds;
	*out_max = max_bounds;
	return true;
}

static void se_gltf_scene_compute_bounds_recursive(const se_gltf_asset *asset, const i32 node_index, const s_mat4 *parent_transform, s_vec3 *min_bounds, s_vec3 *max_bounds, b8 *has_bounds, const sz depth) {
	if (asset == NULL || parent_transform == NULL || min_bounds == NULL || max_bounds == NULL || has_bounds == NULL) return;
	if (depth > asset->nodes.size) return;
	if (node_index < 0 || (sz)node_index >= asset->nodes.size) return;

	se_gltf_node *node = s_array_get(&asset->nodes, node_index);
	s_mat4 local = se_gltf_node_local_transform(node);
	s_mat4 world = s_mat4_mul(parent_transform, &local);

	if (node->has_mesh && node->mesh >= 0 && (sz)node->mesh < asset->meshes.size) {
		s_vec3 mesh_min = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 mesh_max = s_vec3(0.0f, 0.0f, 0.0f);
		if (se_gltf_mesh_compute_local_bounds(asset, node->mesh, &mesh_min, &mesh_max)) {
			se_gltf_bounds_include_transformed_aabb(&mesh_min, &mesh_max, &world, min_bounds, max_bounds, has_bounds);
		}
	}

	s_foreach(&node->children, i) {
		i32 *child_index = s_array_get(&node->children, i);
		if (child_index == NULL) continue;
		se_gltf_scene_compute_bounds_recursive(asset, *child_index, &world, min_bounds, max_bounds, has_bounds, depth + 1);
	}
}

static b8 se_gltf_add_node_objects_recursive(se_gltf_scene_build_context *ctx, const i32 node_index, const s_mat4 *parent_transform, const sz depth) {
	if (ctx == NULL || parent_transform == NULL) return false;
	if (depth > ctx->asset->nodes.size) return false;
	if (node_index < 0 || (sz)node_index >= ctx->asset->nodes.size) return false;

	se_gltf_node *node = s_array_get(&ctx->asset->nodes, node_index);
	s_mat4 local = se_gltf_node_local_transform(node);
	s_mat4 world = s_mat4_mul(parent_transform, &local);

	if (node->has_mesh) {
		if (node->mesh < 0 || (sz)node->mesh >= ctx->asset->meshes.size) return false;

		se_model **cached_model = s_array_get(ctx->model_cache, node->mesh);
		if (*cached_model == NULL) {
			*cached_model = se_gltf_model_load(ctx->render_handle, ctx->asset, node->mesh);
			if (*cached_model == NULL) return false;
			se_gltf_mesh *gltf_mesh = s_array_get(&ctx->asset->meshes, node->mesh);
			se_gltf_apply_model_materials(ctx->render_handle, ctx->asset, gltf_mesh, *cached_model, ctx->mesh_shader, ctx->default_texture, ctx->wrap, ctx->texture_cache, ctx->material_shader_cache);
		}

		se_object_3d *object = se_object_3d_create(ctx->scene_handle, *cached_model, &world, 1);
		if (object == NULL) return false;

		s_mat4 identity = s_mat4_identity;
		se_object_3d_add_instance(object, &identity, &identity);
		se_scene_3d_add_object(ctx->scene, object);
		ctx->added_objects++;
	}

	s_foreach(&node->children, i) {
		i32 *child_index = s_array_get(&node->children, i);
		if (child_index == NULL) return false;
		if (!se_gltf_add_node_objects_recursive(ctx, *child_index, &world, depth + 1)) return false;
	}

	return true;
}

static b8 se_gltf_add_mesh_objects_fallback(se_gltf_scene_build_context *ctx) {
	if (ctx == NULL) return false;
	s_mat4 identity = s_mat4_identity;

	for (sz mesh_index = 0; mesh_index < ctx->asset->meshes.size; mesh_index++) {
		se_model **cached_model = s_array_get(ctx->model_cache, mesh_index);
		if (*cached_model == NULL) {
			*cached_model = se_gltf_model_load(ctx->render_handle, ctx->asset, (i32)mesh_index);
			if (*cached_model == NULL) return false;
			se_gltf_mesh *gltf_mesh = s_array_get(&ctx->asset->meshes, mesh_index);
			se_gltf_apply_model_materials(ctx->render_handle, ctx->asset, gltf_mesh, *cached_model, ctx->mesh_shader, ctx->default_texture, ctx->wrap, ctx->texture_cache, ctx->material_shader_cache);
		}

		se_object_3d *object = se_object_3d_create(ctx->scene_handle, *cached_model, &identity, 1);
		if (object == NULL) return false;
		se_object_3d_add_instance(object, &identity, &identity);
		se_scene_3d_add_object(ctx->scene, object);
		ctx->added_objects++;
	}

	return true;
}

se_model *se_gltf_model_load(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 mesh_index) {
	if (render_handle == NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (mesh_index < 0 || (sz)mesh_index >= asset->meshes.size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_capacity(&render_handle->models) == s_array_get_size(&render_handle->models)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}

	se_gltf_mesh *mesh = s_array_get(&asset->meshes, mesh_index);
	if (mesh->primitives.size == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}

	se_model *model = s_array_increment(&render_handle->models);
	memset(model, 0, sizeof(*model));
	s_array_init(&model->meshes, mesh->primitives.size);

	se_result error = SE_RESULT_IO;
	se_vertex_3d *tmp_vertices = NULL;
	u32 *tmp_indices = NULL;

	for (sz i = 0; i < mesh->primitives.size; i++) {
		se_gltf_primitive *prim = s_array_get(&mesh->primitives, i);
		if (prim->has_mode && prim->mode != 4) {
			error = SE_RESULT_UNSUPPORTED;
			goto fail;
		}

		se_gltf_attribute *pos_attr = NULL;
		se_gltf_attribute *norm_attr = NULL;
		se_gltf_attribute *uv_attr = NULL;
		for (sz a = 0; a < prim->attributes.attributes.size; a++) {
			se_gltf_attribute *attr = s_array_get(&prim->attributes.attributes, a);
			if (strcmp(attr->name, "POSITION") == 0) pos_attr = attr;
			else if (strcmp(attr->name, "NORMAL") == 0) norm_attr = attr;
			else if (strcmp(attr->name, "TEXCOORD_0") == 0) uv_attr = attr;
		}

		if (pos_attr == NULL || pos_attr->accessor < 0 || (sz)pos_attr->accessor >= asset->accessors.size) {
			error = SE_RESULT_IO;
			goto fail;
		}

		se_gltf_accessor *pos_acc = s_array_get(&asset->accessors, pos_attr->accessor);
		if (pos_acc->component_type != 5126 || pos_acc->type != SE_GLTF_ACCESSOR_VEC3 || pos_acc->count == 0) {
			error = SE_RESULT_UNSUPPORTED;
			goto fail;
		}

		const u32 vertex_count = pos_acc->count;
		se_gltf_accessor_view pos_view = {0};
		if (!se_gltf_get_accessor_view(asset, pos_acc, sizeof(f32) * 3, sizeof(f32) * 3, vertex_count, &pos_view)) {
			error = SE_RESULT_IO;
			goto fail;
		}

		tmp_vertices = (se_vertex_3d *)malloc(sizeof(se_vertex_3d) * vertex_count);
		if (tmp_vertices == NULL) {
			error = SE_RESULT_OUT_OF_MEMORY;
			goto fail;
		}

		for (u32 v = 0; v < vertex_count; v++) {
			const u8 *ptr = pos_view.data + ((sz)v * pos_view.stride);
			const f32 *fptr = (const f32 *)ptr;
			tmp_vertices[v].position = s_vec3(fptr[0], fptr[1], fptr[2]);
			tmp_vertices[v].normal = s_vec3(0.0f, 0.0f, 1.0f);
			tmp_vertices[v].uv = s_vec2(0.0f, 0.0f);
		}

		if (norm_attr != NULL) {
			if (norm_attr->accessor < 0 || (sz)norm_attr->accessor >= asset->accessors.size) {
				error = SE_RESULT_IO;
				goto fail;
			}
			se_gltf_accessor *nacc = s_array_get(&asset->accessors, norm_attr->accessor);
			if (nacc->component_type != 5126 || nacc->type != SE_GLTF_ACCESSOR_VEC3) {
				error = SE_RESULT_UNSUPPORTED;
				goto fail;
			}
			if (nacc->count < vertex_count) {
				error = SE_RESULT_IO;
				goto fail;
			}

			se_gltf_accessor_view normal_view = {0};
			if (!se_gltf_get_accessor_view(asset, nacc, sizeof(f32) * 3, sizeof(f32) * 3, vertex_count, &normal_view)) {
				error = SE_RESULT_IO;
				goto fail;
			}

			for (u32 v = 0; v < vertex_count; v++) {
				const u8 *ptr = normal_view.data + ((sz)v * normal_view.stride);
				const f32 *fptr = (const f32 *)ptr;
				tmp_vertices[v].normal = s_vec3(fptr[0], fptr[1], fptr[2]);
			}
		}

		if (uv_attr != NULL) {
			if (uv_attr->accessor < 0 || (sz)uv_attr->accessor >= asset->accessors.size) {
				error = SE_RESULT_IO;
				goto fail;
			}
			se_gltf_accessor *uacc = s_array_get(&asset->accessors, uv_attr->accessor);
			if (uacc->component_type != 5126 || uacc->type != SE_GLTF_ACCESSOR_VEC2) {
				error = SE_RESULT_UNSUPPORTED;
				goto fail;
			}
			if (uacc->count < vertex_count) {
				error = SE_RESULT_IO;
				goto fail;
			}

			se_gltf_accessor_view uv_view = {0};
			if (!se_gltf_get_accessor_view(asset, uacc, sizeof(f32) * 2, sizeof(f32) * 2, vertex_count, &uv_view)) {
				error = SE_RESULT_IO;
				goto fail;
			}

			for (u32 v = 0; v < vertex_count; v++) {
				const u8 *ptr = uv_view.data + ((sz)v * uv_view.stride);
				const f32 *fptr = (const f32 *)ptr;
				tmp_vertices[v].uv = s_vec2(fptr[0], fptr[1]);
			}
		}

		u32 index_count = vertex_count;
		if (prim->has_indices) {
			if (prim->indices < 0 || (sz)prim->indices >= asset->accessors.size) {
				error = SE_RESULT_IO;
				goto fail;
			}

			se_gltf_accessor *iacc = s_array_get(&asset->accessors, prim->indices);
			if (iacc->type != SE_GLTF_ACCESSOR_SCALAR || iacc->count == 0) {
				error = SE_RESULT_UNSUPPORTED;
				goto fail;
			}

			u32 component_size = 0;
			if (iacc->component_type == 5125) component_size = 4;
			else if (iacc->component_type == 5123) component_size = 2;
			else if (iacc->component_type == 5121) component_size = 1;
			else {
				error = SE_RESULT_UNSUPPORTED;
				goto fail;
			}

			index_count = iacc->count;
			se_gltf_accessor_view idx_view = {0};
			if (!se_gltf_get_accessor_view(asset, iacc, component_size, component_size, index_count, &idx_view)) {
				error = SE_RESULT_IO;
				goto fail;
			}

			tmp_indices = (u32 *)malloc(sizeof(u32) * index_count);
			if (tmp_indices == NULL) {
				error = SE_RESULT_OUT_OF_MEMORY;
				goto fail;
			}

			for (u32 idx = 0; idx < index_count; idx++) {
				const u8 *ptr = idx_view.data + ((sz)idx * idx_view.stride);
				u32 value = 0;
				if (iacc->component_type == 5125) value = *(const u32 *)ptr;
				else if (iacc->component_type == 5123) value = *(const u16 *)ptr;
				else value = *(const u8 *)ptr;
				if (value >= vertex_count) {
					error = SE_RESULT_IO;
					goto fail;
				}
				tmp_indices[idx] = value;
			}
		} else {
			tmp_indices = (u32 *)malloc(sizeof(u32) * index_count);
			if (tmp_indices == NULL) {
				error = SE_RESULT_OUT_OF_MEMORY;
				goto fail;
			}
			for (u32 idx = 0; idx < index_count; idx++) {
				tmp_indices[idx] = idx;
			}
		}

		se_mesh *out_mesh = s_array_increment(&model->meshes);
		memset(out_mesh, 0, sizeof(*out_mesh));
		if (!se_gltf_mesh_finalize(out_mesh, tmp_vertices, tmp_indices, vertex_count, index_count)) {
			error = SE_RESULT_OUT_OF_MEMORY;
			goto fail;
		}

		free(tmp_vertices);
		tmp_vertices = NULL;
		free(tmp_indices);
		tmp_indices = NULL;
	}

	model->is_valid = true;
	se_set_last_error(SE_RESULT_OK);
	return model;

fail:
	free(tmp_vertices);
	free(tmp_indices);
	se_gltf_model_rollback(render_handle, model);
	se_set_last_error(error);
	return NULL;
}

se_texture *se_gltf_image_load(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 image_index, const se_texture_wrap wrap) {
	if (render_handle == NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (image_index < 0 || (sz)image_index >= asset->images.size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_gltf_image *image = s_array_get(&asset->images, image_index);
	if (image->data && image->data_size > 0) {
		return se_texture_load_from_memory(render_handle, image->data, image->data_size, wrap);
	}

	if (image->uri && image->uri[0] != '\0') {
		if (strncmp(image->uri, "data:", 5) == 0) {
			u8 *decoded = NULL;
			sz decoded_size = 0;
			char *mime = NULL;
			if (se_loader_parse_data_uri(image->uri, &mime, &decoded, &decoded_size)) {
				se_texture *texture = se_texture_load_from_memory(render_handle, decoded, decoded_size, wrap);
				free(mime);
				free(decoded);
				return texture;
			}
			free(mime);
			free(decoded);
			se_set_last_error(SE_RESULT_IO);
			return NULL;
		}

		if (!se_loader_path_is_absolute(image->uri) && asset->base_dir && asset->base_dir[0] != '\0') {
			char full_path[SE_MAX_PATH_LENGTH] = {0};
			if (s_path_join(full_path, SE_MAX_PATH_LENGTH, asset->base_dir, image->uri)) {
				se_texture *texture = se_texture_load(render_handle, full_path, wrap);
				if (texture != NULL) {
					return texture;
				}
			}
		}

		return se_texture_load(render_handle, image->uri, wrap);
	}

	se_set_last_error(SE_RESULT_NOT_FOUND);
	return NULL;
}

se_texture *se_gltf_texture_load(se_render_handle *render_handle, const se_gltf_asset *asset, const i32 texture_index, const se_texture_wrap wrap) {
	if (render_handle == NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (texture_index < 0 || (sz)texture_index >= asset->textures.size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_gltf_texture *texture = s_array_get(&asset->textures, texture_index);
	if (!texture->has_source) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	if (texture->source < 0 || (sz)texture->source >= asset->images.size) {
		se_set_last_error(SE_RESULT_IO);
		return NULL;
	}

	return se_gltf_image_load(render_handle, asset, texture->source, wrap);
}

se_model *se_gltf_model_load_first(se_render_handle *render_handle, const char *path, const se_gltf_load_params *params) {
	if (render_handle == NULL || path == NULL || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_gltf_asset *asset = se_gltf_load(path, params);
	if (asset == NULL) {
		return NULL;
	}

	if (asset->meshes.size == 0) {
		se_gltf_free(asset);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}

	se_model *model = se_gltf_model_load(render_handle, asset, 0);
	se_result result = se_get_last_error();
	se_gltf_free(asset);
	se_set_last_error(result);
	return model;
}

sz se_gltf_scene_load_from_asset(se_render_handle *render_handle, se_scene_handle *scene_handle, se_scene_3d *scene, const se_gltf_asset *asset, se_shader *mesh_shader, se_texture *default_texture, const se_texture_wrap wrap) {
	if (render_handle == NULL || scene_handle == NULL || scene == NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (asset->meshes.size == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}

	se_models_ptr model_cache = {0};
	s_array_init(&model_cache, asset->meshes.size);
	for (sz i = 0; i < asset->meshes.size; i++) {
		se_model **slot = s_array_increment(&model_cache);
		*slot = NULL;
	}

	se_textures_ptr texture_cache = {0};
	s_array_init(&texture_cache, asset->textures.size);
	for (sz i = 0; i < asset->textures.size; i++) {
		se_texture **slot = s_array_increment(&texture_cache);
		*slot = NULL;
	}

	se_shaders_ptr material_shader_cache = {0};
	s_array_init(&material_shader_cache, asset->materials.size);
	for (sz i = 0; i < asset->materials.size; i++) {
		se_shader **slot = s_array_increment(&material_shader_cache);
		*slot = NULL;
	}

	se_gltf_scene_build_context ctx = {0};
	ctx.render_handle = render_handle;
	ctx.scene_handle = scene_handle;
	ctx.scene = scene;
	ctx.asset = asset;
	ctx.mesh_shader = mesh_shader;
	ctx.default_texture = default_texture;
	ctx.wrap = wrap;
	ctx.model_cache = &model_cache;
	ctx.texture_cache = &texture_cache;
	ctx.material_shader_cache = &material_shader_cache;

	b8 ok = true;
	if (asset->scenes.size > 0) {
		i32 scene_index = 0;
		if (asset->has_default_scene) {
			scene_index = asset->default_scene;
		}
		if (scene_index < 0 || (sz)scene_index >= asset->scenes.size) {
			ok = false;
			se_set_last_error(SE_RESULT_IO);
		} else {
			se_gltf_scene *default_scene = s_array_get(&asset->scenes, scene_index);
			s_mat4 identity = s_mat4_identity;
			s_foreach(&default_scene->nodes, i) {
				i32 *root_node = s_array_get(&default_scene->nodes, i);
				if (root_node == NULL || !se_gltf_add_node_objects_recursive(&ctx, *root_node, &identity, 0)) {
					ok = false;
					if (se_get_last_error() == SE_RESULT_OK) {
						se_set_last_error(SE_RESULT_IO);
					}
					break;
				}
			}
		}
	} else {
		ok = se_gltf_add_mesh_objects_fallback(&ctx);
		if (!ok && se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_IO);
		}
	}

	s_array_clear(&texture_cache);
	s_array_clear(&material_shader_cache);
	s_array_clear(&model_cache);

	if (!ok || ctx.added_objects == 0) {
		if (ok) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
		}
		return 0;
	}

	se_set_last_error(SE_RESULT_OK);
	return ctx.added_objects;
}

se_gltf_asset *se_gltf_scene_load(se_render_handle *render_handle, se_scene_handle *scene_handle, se_scene_3d *scene, const char *path, const se_gltf_load_params *load_params, se_shader *mesh_shader, se_texture *default_texture, const se_texture_wrap wrap, sz *out_objects_loaded) {
	if (out_objects_loaded) {
		*out_objects_loaded = 0;
	}
	if (render_handle == NULL || scene_handle == NULL || scene == NULL || path == NULL || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_gltf_asset *asset = se_gltf_load(path, load_params);
	if (asset == NULL) {
		return NULL;
	}

	sz objects_loaded = se_gltf_scene_load_from_asset(render_handle, scene_handle, scene, asset, mesh_shader, default_texture, wrap);
	if (objects_loaded == 0) {
		se_result result = se_get_last_error();
		se_gltf_free(asset);
		se_set_last_error(result);
		return NULL;
	}

	if (out_objects_loaded) {
		*out_objects_loaded = objects_loaded;
	}

	se_set_last_error(SE_RESULT_OK);
	return asset;
}

b8 se_gltf_scene_compute_bounds(const se_gltf_asset *asset, s_vec3 *out_center, f32 *out_radius) {
	if (asset == NULL || out_center == NULL || out_radius == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	b8 has_bounds = false;
	s_vec3 min_bounds = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 max_bounds = s_vec3(0.0f, 0.0f, 0.0f);

	i32 scene_index = 0;
	if (asset->has_default_scene) {
		scene_index = asset->default_scene;
	}

	if (asset->scenes.size > 0 && scene_index >= 0 && (sz)scene_index < asset->scenes.size) {
		se_gltf_scene *scene = s_array_get(&asset->scenes, scene_index);
		s_mat4 identity = s_mat4_identity;
		s_foreach(&scene->nodes, i) {
			i32 *root_node = s_array_get(&scene->nodes, i);
			if (root_node == NULL) continue;
			se_gltf_scene_compute_bounds_recursive(asset, *root_node, &identity, &min_bounds, &max_bounds, &has_bounds, 0);
		}
	}

	if (!has_bounds) {
		for (sz mesh_index = 0; mesh_index < asset->meshes.size; mesh_index++) {
			s_vec3 mesh_min = s_vec3(0.0f, 0.0f, 0.0f);
			s_vec3 mesh_max = s_vec3(0.0f, 0.0f, 0.0f);
			if (!se_gltf_mesh_compute_local_bounds(asset, (i32)mesh_index, &mesh_min, &mesh_max)) {
				continue;
			}
			se_gltf_bounds_include_point(&min_bounds, &max_bounds, &mesh_min, &has_bounds);
			se_gltf_bounds_include_point(&min_bounds, &max_bounds, &mesh_max, &has_bounds);
		}
	}

	if (!has_bounds) {
		*out_center = s_vec3(0.0f, 0.0f, 0.0f);
		*out_radius = 1.0f;
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	out_center->x = 0.5f * (min_bounds.x + max_bounds.x);
	out_center->y = 0.5f * (min_bounds.y + max_bounds.y);
	out_center->z = 0.5f * (min_bounds.z + max_bounds.z);

	f32 size_x = max_bounds.x - min_bounds.x;
	f32 size_y = max_bounds.y - min_bounds.y;
	f32 size_z = max_bounds.z - min_bounds.z;
	f32 radius = size_x;
	if (size_y > radius) radius = size_y;
	if (size_z > radius) radius = size_z;
	if (radius < 1.0f) radius = 1.0f;
	*out_radius = radius;

	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_gltf_scene_fit_camera(se_scene_3d *scene, const se_gltf_asset *asset) {
	if (scene == NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	if (scene->camera == NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	s_vec3 bounds_center = s_vec3(0.0f, 0.0f, 0.0f);
	f32 bounds_radius = 1.0f;
	b8 has_bounds = se_gltf_scene_compute_bounds(asset, &bounds_center, &bounds_radius);

	se_camera *camera = scene->camera;
	s_vec3 target = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 position = s_vec3(0.0f, 300.0f, 1500.0f);
	if (has_bounds) {
		target = bounds_center;
		position = s_vec3(bounds_center.x, bounds_center.y + bounds_radius * 0.35f, bounds_center.z + bounds_radius * 1.5f);
		camera->far = bounds_radius * 10.0f;
	} else {
		camera->far = 5000.0f;
	}

	camera->position = position;
	camera->target = target;
	camera->up = s_vec3(0.0f, 1.0f, 0.0f);
	camera->near = 0.1f;

	se_set_last_error(SE_RESULT_OK);
}

void se_gltf_scene_get_navigation_speeds(const se_gltf_asset *asset, f32 *out_base_speed, f32 *out_fast_speed) {
	if (out_base_speed) {
		*out_base_speed = 600.0f;
	}
	if (out_fast_speed) {
		*out_fast_speed = 1400.0f;
	}
	if (asset == NULL || out_base_speed == NULL || out_fast_speed == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}

	s_vec3 center = s_vec3(0.0f, 0.0f, 0.0f);
	f32 radius = 1.0f;
	b8 has_bounds = se_gltf_scene_compute_bounds(asset, &center, &radius);
	if (!has_bounds) {
		se_set_last_error(SE_RESULT_OK);
		return;
	}

	f32 base_speed = radius * 1.25f;
	if (base_speed < 60.0f) base_speed = 60.0f;
	if (base_speed > 3000.0f) base_speed = 3000.0f;
	*out_base_speed = base_speed;
	*out_fast_speed = base_speed * 2.5f;

	se_set_last_error(SE_RESULT_OK);
}
