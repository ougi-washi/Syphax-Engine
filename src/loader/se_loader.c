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
	se_context *ctx;
	se_scene_3d_handle scene;
	const se_gltf_asset *asset;
	se_shader_handle mesh_shader;
	se_texture_handle default_texture;
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

static b8 se_loader_mesh_data_flags_are_valid(const se_mesh_data_flags mesh_data_flags) {
	const se_mesh_data_flags supported_flags = (se_mesh_data_flags)(SE_MESH_DATA_CPU | SE_MESH_DATA_GPU);
	if ((mesh_data_flags & supported_flags) == 0) {
		return false;
	}
	if ((mesh_data_flags & ~supported_flags) != 0) {
		return false;
	}
	return true;
}

static b8 se_gltf_mesh_set_source_path(se_mesh *mesh, const char *source_path) {
	if (mesh == NULL) {
		return false;
	}

	s_array_clear(&mesh->cpu.file_path);
	if (source_path == NULL || source_path[0] == '\0') {
		return true;
	}

	const sz source_path_size = strlen(source_path) + 1;
	s_array_init(&mesh->cpu.file_path);
	s_array_reserve(&mesh->cpu.file_path, source_path_size);
	for (sz i = 0; i < source_path_size; i++) {
		s_handle char_handle = s_array_increment(&mesh->cpu.file_path);
		c8 *dst = s_array_get(&mesh->cpu.file_path, char_handle);
		*dst = source_path[i];
	}

	return true;
}

static b8 se_gltf_mesh_create_gpu_data(se_mesh *mesh) {
	if (mesh == NULL || s_array_get_size(&mesh->cpu.vertices) == 0 || s_array_get_size(&mesh->cpu.indices) == 0) {
		return false;
	}

	const se_vertex_3d *vertices = s_array_get_data(&mesh->cpu.vertices);
	const u32 *indices = s_array_get_data(&mesh->cpu.indices);
	const u32 vertex_count = (u32)s_array_get_size(&mesh->cpu.vertices);
	const u32 index_count = (u32)s_array_get_size(&mesh->cpu.indices);

	glGenVertexArrays(1, &mesh->gpu.vao);
	glGenBuffers(1, &mesh->gpu.vbo);
	glGenBuffers(1, &mesh->gpu.ebo);
	if (mesh->gpu.vao == 0 || mesh->gpu.vbo == 0 || mesh->gpu.ebo == 0) {
		if (mesh->gpu.vao != 0) glDeleteVertexArrays(1, &mesh->gpu.vao);
		if (mesh->gpu.vbo != 0) glDeleteBuffers(1, &mesh->gpu.vbo);
		if (mesh->gpu.ebo != 0) glDeleteBuffers(1, &mesh->gpu.ebo);
		mesh->gpu.vao = 0;
		mesh->gpu.vbo = 0;
		mesh->gpu.ebo = 0;
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

static b8 se_gltf_mesh_finalize(se_mesh *mesh, const char *source_path, const se_mesh_data_flags mesh_data_flags) {
	if (mesh == NULL || !se_loader_mesh_data_flags_are_valid(mesh_data_flags)) {
		return false;
	}
	if (s_array_get_size(&mesh->cpu.vertices) == 0 || s_array_get_size(&mesh->cpu.indices) == 0) {
		return false;
	}

	mesh->matrix = s_mat4_identity;
	mesh->shader = S_HANDLE_NULL;
	mesh->gpu.vertex_count = (u32)s_array_get_size(&mesh->cpu.vertices);
	mesh->gpu.index_count = (u32)s_array_get_size(&mesh->cpu.indices);

	if ((mesh_data_flags & SE_MESH_DATA_GPU) != 0 && !se_gltf_mesh_create_gpu_data(mesh)) {
		se_mesh_discard_cpu_data(mesh);
		return false;
	}

	if ((mesh_data_flags & SE_MESH_DATA_CPU) != 0) {
		if (!se_gltf_mesh_set_source_path(mesh, source_path)) {
			if (mesh->gpu.vao != 0) glDeleteVertexArrays(1, &mesh->gpu.vao);
			if (mesh->gpu.vbo != 0) glDeleteBuffers(1, &mesh->gpu.vbo);
			if (mesh->gpu.ebo != 0) glDeleteBuffers(1, &mesh->gpu.ebo);
			mesh->gpu = (se_mesh_gpu_data){0};
			mesh->data_flags = (se_mesh_data_flags)(mesh->data_flags & ~SE_MESH_DATA_GPU);
			se_mesh_discard_cpu_data(mesh);
			return false;
		}
		mesh->data_flags = (se_mesh_data_flags)(mesh->data_flags | SE_MESH_DATA_CPU);
	} else {
		se_mesh_discard_cpu_data(mesh);
	}

	return true;
}

static void se_gltf_mesh_release(se_mesh *mesh) {
	if (mesh == NULL) return;
	if (mesh->gpu.vao != 0) glDeleteVertexArrays(1, &mesh->gpu.vao);
	if (mesh->gpu.vbo != 0) glDeleteBuffers(1, &mesh->gpu.vbo);
	if (mesh->gpu.ebo != 0) glDeleteBuffers(1, &mesh->gpu.ebo);
	se_mesh_discard_cpu_data(mesh);
	mesh->gpu = (se_mesh_gpu_data){0};
	mesh->data_flags = (se_mesh_data_flags)(mesh->data_flags & ~SE_MESH_DATA_GPU);
	memset(mesh, 0, sizeof(*mesh));
}

static void se_gltf_model_release(se_model *model) {
	if (model == NULL) return;
	se_mesh *mesh = NULL;
	s_foreach(&model->meshes, mesh) {
		se_gltf_mesh_release(mesh);
	}
	s_array_clear(&model->meshes);
}

static void se_gltf_model_rollback(se_context *ctx, const se_model_handle model_handle) {
	if (ctx == NULL || model_handle == S_HANDLE_NULL) return;
	se_model *model = s_array_get(&ctx->models, model_handle);
	if (model == NULL) return;
	se_gltf_model_release(model);
	memset(model, 0, sizeof(*model));
	s_array_remove(&ctx->models, model_handle);
}

static b8 se_gltf_get_accessor_view(const se_gltf_asset *asset, const se_gltf_accessor *accessor, const u32 element_size, const u32 default_stride, const u32 required_count, se_gltf_accessor_view *out_view) {
	if (asset == NULL || accessor == NULL || out_view == NULL || element_size == 0) return false;
	if (!accessor->has_buffer_view) return false;
	if (accessor->buffer_view < 0 || (sz)accessor->buffer_view >= s_array_get_size(&asset->buffer_views)) return false;

	se_gltf_buffer_views *buffer_views = (se_gltf_buffer_views *)&asset->buffer_views;
	s_handle view_handle = s_array_handle(buffer_views, (u32)accessor->buffer_view);
	se_gltf_buffer_view *view = s_array_get(buffer_views, view_handle);
	if (view == NULL) return false;
	if (view->buffer < 0 || (sz)view->buffer >= s_array_get_size(&asset->buffers)) return false;

	se_gltf_buffers *buffers = (se_gltf_buffers *)&asset->buffers;
	s_handle buffer_handle = s_array_handle(buffers, (u32)view->buffer);
	se_gltf_buffer *buffer = s_array_get(buffers, buffer_handle);
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

static se_texture_handle se_gltf_texture_from_cache(se_context *ctx,
								 const se_gltf_asset *asset,
								 se_textures_ptr *texture_cache,
								 const i32 texture_index,
								 const se_texture_wrap wrap) {
	if (ctx == NULL || asset == NULL || texture_cache == NULL) return S_HANDLE_NULL;
	if (texture_index < 0 || (sz)texture_index >= s_array_get_size(&asset->textures) || (sz)texture_index >= s_array_get_size(texture_cache)) {
		return S_HANDLE_NULL;
	}

	s_handle entry = s_array_handle(texture_cache, (u32)texture_index);
	se_texture_handle *cached = s_array_get(texture_cache, entry);
	if (cached == NULL) return S_HANDLE_NULL;
	if (*cached == S_HANDLE_NULL) {
		*cached = se_gltf_texture_load(asset, texture_index, wrap);
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

static void se_gltf_set_default_material_uniforms(se_context *ctx, const se_shader_handle shader, const se_texture_handle default_texture) {
	if (ctx == NULL || shader == S_HANDLE_NULL) return;

	se_texture *texture = s_array_get(&ctx->textures, default_texture);
	const u32 default_texture_id = texture ? texture->id : 0;
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

static se_shader_handle se_gltf_get_material_shader(se_context *ctx,
										   const se_gltf_asset *asset,
										   const i32 material_index,
										   const se_shader_handle mesh_shader,
										   const se_texture_handle default_texture,
										   const se_texture_wrap wrap,
										   se_textures_ptr *texture_cache,
										   se_shaders_ptr *material_shader_cache) {
	if (ctx == NULL || asset == NULL || mesh_shader == S_HANDLE_NULL || texture_cache == NULL || material_shader_cache == NULL) {
		return mesh_shader;
	}
	if (material_index < 0 || (sz)material_index >= s_array_get_size(&asset->materials) || (sz)material_index >= s_array_get_size(material_shader_cache)) {
		return mesh_shader;
	}

	s_handle cache_entry = s_array_handle(material_shader_cache, (u32)material_index);
	se_shader_handle *cached_shader = s_array_get(material_shader_cache, cache_entry);
	if (cached_shader == NULL) return mesh_shader;
	if (*cached_shader != S_HANDLE_NULL) return *cached_shader;

	se_shader *mesh_shader_ptr = s_array_get(&ctx->shaders, mesh_shader);
	if (mesh_shader_ptr == NULL) {
		return mesh_shader;
	}

	se_shader_handle material_shader = se_shader_load(mesh_shader_ptr->vertex_path, mesh_shader_ptr->fragment_path);
	if (material_shader == S_HANDLE_NULL) {
		return mesh_shader;
	}

	se_gltf_set_default_material_uniforms(ctx, material_shader, default_texture);

	se_gltf_materials *materials = (se_gltf_materials *)&asset->materials;
	s_handle material_handle = s_array_handle(materials, (u32)material_index);
	se_gltf_material *material = s_array_get(materials, material_handle);
	if (material != NULL) {
		s_vec4 base_color_factor = s_vec4(1.0f, 1.0f, 1.0f, 1.0f);
		f32 metallic_factor = 1.0f;
		f32 roughness_factor = 1.0f;
		f32 ao_factor = 1.0f;
		s_vec3 emissive_factor = s_vec3(0.0f, 0.0f, 0.0f);

		i32 has_metallic_roughness_texture = 0;
		i32 has_occlusion_texture = 0;
		i32 has_emissive_texture = 0;

		se_texture *default_texture_ptr = s_array_get(&ctx->textures, default_texture);
		u32 base_color_texture_id = default_texture_ptr ? default_texture_ptr->id : 0;
		u32 metallic_roughness_texture_id = base_color_texture_id;
		u32 occlusion_texture_id = base_color_texture_id;
		u32 emissive_texture_id = base_color_texture_id;

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
			if (pbr->has_base_color_texture) {
			se_texture_handle tex = se_gltf_texture_from_cache(
							ctx,
				asset,
				texture_cache,
				pbr->base_color_texture.index,
				wrap);
			se_texture *tex_ptr = s_array_get(&ctx->textures, tex);
			if (tex_ptr != NULL) {
				base_color_texture_id = tex_ptr->id;
			}
			}

			if (pbr->has_metallic_roughness_texture) {
			se_texture_handle tex = se_gltf_texture_from_cache(
							ctx,
				asset,
				texture_cache,
				pbr->metallic_roughness_texture.index,
				wrap);
			se_texture *tex_ptr = s_array_get(&ctx->textures, tex);
			if (tex_ptr != NULL) {
				metallic_roughness_texture_id = tex_ptr->id;
				has_metallic_roughness_texture = 1;
			}
			}
		}

		if (material->has_occlusion_texture) {
			if (material->occlusion_texture.has_strength) {
				ao_factor = material->occlusion_texture.strength;
			}
		se_texture_handle tex = se_gltf_texture_from_cache(
			ctx,
			asset,
			texture_cache,
			material->occlusion_texture.info.index,
			wrap);
		se_texture *tex_ptr = s_array_get(&ctx->textures, tex);
		if (tex_ptr != NULL) {
			occlusion_texture_id = tex_ptr->id;
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
		se_texture_handle tex = se_gltf_texture_from_cache(
			ctx,
			asset,
			texture_cache,
			material->emissive_texture.index,
			wrap);
		se_texture *tex_ptr = s_array_get(&ctx->textures, tex);
		if (tex_ptr != NULL) {
			emissive_texture_id = tex_ptr->id;
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

		se_shader_set_texture(material_shader, "u_texture", base_color_texture_id);
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

static void se_gltf_apply_model_materials(se_context *ctx, const se_gltf_asset *asset, const se_gltf_mesh *gltf_mesh, const se_model_handle model, const se_shader_handle mesh_shader, const se_texture_handle default_texture, const se_texture_wrap wrap, se_textures_ptr *texture_cache, se_shaders_ptr *material_shader_cache) {
	if (ctx == NULL || asset == NULL || gltf_mesh == NULL || model == S_HANDLE_NULL || texture_cache == NULL) return;
	se_model *model_ptr = s_array_get(&ctx->models, model);
	if (model_ptr == NULL) return;

	if (mesh_shader != S_HANDLE_NULL) {
		se_gltf_set_default_material_uniforms(ctx, mesh_shader, default_texture);
	}

	for (sz prim_index = 0; prim_index < s_array_get_size(&model_ptr->meshes); prim_index++) {
		s_handle mesh_handle = s_array_handle(&model_ptr->meshes, (u32)prim_index);
		se_mesh *mesh = s_array_get(&model_ptr->meshes, mesh_handle);
		if (mesh == NULL) {
			continue;
		}

		se_shader_handle mesh_material_shader = mesh_shader;

		if (prim_index < s_array_get_size(&gltf_mesh->primitives)) {
			se_gltf_primitives *primitives = (se_gltf_primitives *)&gltf_mesh->primitives;
			s_handle prim_handle = s_array_handle(primitives, (u32)prim_index);
			se_gltf_primitive *prim = s_array_get(primitives, prim_handle);
			if (prim != NULL && prim->has_material && prim->material >= 0 && (sz)prim->material < s_array_get_size(&asset->materials)) {
				mesh_material_shader = se_gltf_get_material_shader(
					ctx,
					asset,
					prim->material,
					mesh_shader,
					default_texture,
					wrap,
					texture_cache,
					material_shader_cache);
			}
		}

		if (mesh_material_shader != S_HANDLE_NULL) {
			mesh->shader = mesh_material_shader;
		}
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
	if (mesh_index < 0 || (sz)mesh_index >= s_array_get_size(&asset->meshes)) return false;

	se_gltf_meshes *meshes = (se_gltf_meshes *)&asset->meshes;
	s_handle mesh_handle = s_array_handle(meshes, (u32)mesh_index);
	se_gltf_mesh *mesh = s_array_get(meshes, mesh_handle);
	if (mesh == NULL) return false;
	b8 has_bounds = false;
	s_vec3 min_bounds = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 max_bounds = s_vec3(0.0f, 0.0f, 0.0f);

	for (sz prim_index = 0; prim_index < s_array_get_size(&mesh->primitives); prim_index++) {
		s_handle prim_handle = s_array_handle(&mesh->primitives, (u32)prim_index);
		se_gltf_primitive *prim = s_array_get(&mesh->primitives, prim_handle);
		if (prim == NULL) {
			continue;
		}
		se_gltf_attribute *position_attr = NULL;

		for (sz attr_index = 0; attr_index < s_array_get_size(&prim->attributes.attributes); attr_index++) {
			s_handle attr_handle = s_array_handle(&prim->attributes.attributes, (u32)attr_index);
			se_gltf_attribute *attr = s_array_get(&prim->attributes.attributes, attr_handle);
			if (attr == NULL) {
				continue;
			}
			if (strcmp(attr->name, "POSITION") == 0) {
				position_attr = attr;
				break;
			}
		}

		if (position_attr == NULL || position_attr->accessor < 0 || (sz)position_attr->accessor >= s_array_get_size(&asset->accessors)) {
			continue;
		}

		se_gltf_accessors *accessors = (se_gltf_accessors *)&asset->accessors;
		s_handle accessor_handle = s_array_handle(accessors, (u32)position_attr->accessor);
		se_gltf_accessor *position_accessor = s_array_get(accessors, accessor_handle);
		if (position_accessor == NULL) {
			continue;
		}
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
	if (depth > s_array_get_size(&asset->nodes)) return;
	if (node_index < 0 || (sz)node_index >= s_array_get_size(&asset->nodes)) return;

	se_gltf_nodes *nodes = (se_gltf_nodes *)&asset->nodes;
	s_handle node_handle = s_array_handle(nodes, (u32)node_index);
	se_gltf_node *node = s_array_get(nodes, node_handle);
	if (node == NULL) return;
	s_mat4 local = se_gltf_node_local_transform(node);
	s_mat4 world = s_mat4_mul(parent_transform, &local);

	if (node->has_mesh && node->mesh >= 0 && (sz)node->mesh < s_array_get_size(&asset->meshes)) {
		s_vec3 mesh_min = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 mesh_max = s_vec3(0.0f, 0.0f, 0.0f);
		if (se_gltf_mesh_compute_local_bounds(asset, node->mesh, &mesh_min, &mesh_max)) {
			se_gltf_bounds_include_transformed_aabb(&mesh_min, &mesh_max, &world, min_bounds, max_bounds, has_bounds);
		}
	}

	for (sz i = 0; i < s_array_get_size(&node->children); i++) {
		s_handle child_handle = s_array_handle(&node->children, (u32)i);
		i32 *child_index = s_array_get(&node->children, child_handle);
		if (child_index == NULL) continue;
		se_gltf_scene_compute_bounds_recursive(asset, *child_index, &world, min_bounds, max_bounds, has_bounds, depth + 1);
	}
}

static b8 se_gltf_add_node_objects_recursive(se_gltf_scene_build_context *ctx, const i32 node_index, const s_mat4 *parent_transform, const sz depth) {
	if (ctx == NULL || parent_transform == NULL) return false;
	if (depth > s_array_get_size(&ctx->asset->nodes)) return false;
	if (node_index < 0 || (sz)node_index >= s_array_get_size(&ctx->asset->nodes)) return false;

	se_gltf_nodes *nodes = (se_gltf_nodes *)&ctx->asset->nodes;
	s_handle node_handle = s_array_handle(nodes, (u32)node_index);
	se_gltf_node *node = s_array_get(nodes, node_handle);
	if (node == NULL) return false;
	s_mat4 local = se_gltf_node_local_transform(node);
	s_mat4 world = s_mat4_mul(parent_transform, &local);

	if (node->has_mesh) {
		if (node->mesh < 0 || (sz)node->mesh >= s_array_get_size(&ctx->asset->meshes)) return false;

		s_handle cache_entry = s_array_handle(ctx->model_cache, (u32)node->mesh);
		se_model_handle *cached_model = s_array_get(ctx->model_cache, cache_entry);
		if (cached_model == NULL) return false;
		if (*cached_model == S_HANDLE_NULL) {
			*cached_model = se_gltf_model_load(ctx->asset, node->mesh);
			if (*cached_model == S_HANDLE_NULL) return false;
			se_gltf_meshes *meshes = (se_gltf_meshes *)&ctx->asset->meshes;
			s_handle mesh_handle = s_array_handle(meshes, (u32)node->mesh);
			se_gltf_mesh *gltf_mesh = s_array_get(meshes, mesh_handle);
			if (gltf_mesh == NULL) return false;
			se_gltf_apply_model_materials(ctx->ctx, ctx->asset, gltf_mesh, *cached_model, ctx->mesh_shader, ctx->default_texture, ctx->wrap, ctx->texture_cache, ctx->material_shader_cache);
		}

		se_object_3d_handle object = se_object_3d_create(*cached_model, &world, 1);
		if (object == S_HANDLE_NULL) return false;

		s_mat4 identity = s_mat4_identity;
		se_object_3d_add_instance(object, &identity, &identity);
		se_scene_3d_add_object(ctx->scene, object);
		ctx->added_objects++;
	}

	for (sz i = 0; i < s_array_get_size(&node->children); i++) {
		s_handle child_handle = s_array_handle(&node->children, (u32)i);
		i32 *child_index = s_array_get(&node->children, child_handle);
		if (child_index == NULL) return false;
		if (!se_gltf_add_node_objects_recursive(ctx, *child_index, &world, depth + 1)) return false;
	}

	return true;
}

static b8 se_gltf_add_mesh_objects_fallback(se_gltf_scene_build_context *ctx) {
	if (ctx == NULL) return false;
	s_mat4 identity = s_mat4_identity;

	for (sz mesh_index = 0; mesh_index < s_array_get_size(&ctx->asset->meshes); mesh_index++) {
		s_handle cache_entry = s_array_handle(ctx->model_cache, (u32)mesh_index);
		se_model_handle *cached_model = s_array_get(ctx->model_cache, cache_entry);
		if (cached_model == NULL) return false;
		if (*cached_model == S_HANDLE_NULL) {
			*cached_model = se_gltf_model_load(ctx->asset, (i32)mesh_index);
			if (*cached_model == S_HANDLE_NULL) return false;
			se_gltf_meshes *meshes = (se_gltf_meshes *)&ctx->asset->meshes;
			s_handle mesh_handle = s_array_handle(meshes, (u32)mesh_index);
			se_gltf_mesh *gltf_mesh = s_array_get(meshes, mesh_handle);
			if (gltf_mesh == NULL) return false;
			se_gltf_apply_model_materials(ctx->ctx, ctx->asset, gltf_mesh, *cached_model, ctx->mesh_shader, ctx->default_texture, ctx->wrap, ctx->texture_cache, ctx->material_shader_cache);
		}

		se_object_3d_handle object = se_object_3d_create(*cached_model, &identity, 1);
		if (object == S_HANDLE_NULL) return false;
		se_object_3d_add_instance(object, &identity, &identity);
		se_scene_3d_add_object(ctx->scene, object);
		ctx->added_objects++;
	}

	return true;
}

se_model_handle se_gltf_model_load_ex(const se_gltf_asset *asset, const i32 mesh_index, const se_mesh_data_flags mesh_data_flags) {
	se_context *ctx = se_current_context();
	if (ctx == NULL || asset == NULL || !se_loader_mesh_data_flags_are_valid(mesh_data_flags)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (mesh_index < 0 || (sz)mesh_index >= s_array_get_size(&asset->meshes)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	se_gltf_meshes *meshes = (se_gltf_meshes *)&asset->meshes;
	s_handle mesh_handle = s_array_handle(meshes, (u32)mesh_index);
	se_gltf_mesh *mesh = s_array_get(meshes, mesh_handle);
	if (mesh == NULL || s_array_get_size(&mesh->primitives) == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}

	se_model_handle model_handle = s_array_increment(&ctx->models);
	se_model *model = s_array_get(&ctx->models, model_handle);
	memset(model, 0, sizeof(*model));
	s_array_init(&model->meshes);
	s_array_reserve(&model->meshes, s_array_get_size(&mesh->primitives));

	se_result error = SE_RESULT_IO;
	se_gltf_accessors *accessors = (se_gltf_accessors *)&asset->accessors;

	for (sz i = 0; i < s_array_get_size(&mesh->primitives); i++) {
		s_handle prim_handle = s_array_handle(&mesh->primitives, (u32)i);
		se_gltf_primitive *prim = s_array_get(&mesh->primitives, prim_handle);
		if (prim == NULL) {
			error = SE_RESULT_IO;
			goto fail;
		}
		if (prim->has_mode && prim->mode != 4) {
			error = SE_RESULT_UNSUPPORTED;
			goto fail;
		}

		se_gltf_attribute *pos_attr = NULL;
		se_gltf_attribute *norm_attr = NULL;
		se_gltf_attribute *uv_attr = NULL;
		for (sz a = 0; a < s_array_get_size(&prim->attributes.attributes); a++) {
			s_handle attr_handle = s_array_handle(&prim->attributes.attributes, (u32)a);
			se_gltf_attribute *attr = s_array_get(&prim->attributes.attributes, attr_handle);
			if (attr == NULL) continue;
			if (strcmp(attr->name, "POSITION") == 0) pos_attr = attr;
			else if (strcmp(attr->name, "NORMAL") == 0) norm_attr = attr;
			else if (strcmp(attr->name, "TEXCOORD_0") == 0) uv_attr = attr;
		}

		if (pos_attr == NULL || pos_attr->accessor < 0 || (sz)pos_attr->accessor >= s_array_get_size(&asset->accessors)) {
			error = SE_RESULT_IO;
			goto fail;
		}

		s_handle pos_handle = s_array_handle(accessors, (u32)pos_attr->accessor);
		se_gltf_accessor *pos_acc = s_array_get(accessors, pos_handle);
		if (pos_acc == NULL || pos_acc->component_type != 5126 || pos_acc->type != SE_GLTF_ACCESSOR_VEC3 || pos_acc->count == 0) {
			error = SE_RESULT_UNSUPPORTED;
			goto fail;
		}

		const u32 vertex_count = pos_acc->count;
		se_gltf_accessor_view pos_view = {0};
		if (!se_gltf_get_accessor_view(asset, pos_acc, sizeof(f32) * 3, sizeof(f32) * 3, vertex_count, &pos_view)) {
			error = SE_RESULT_IO;
			goto fail;
		}

		s_handle out_mesh_handle = s_array_increment(&model->meshes);
		se_mesh *out_mesh = s_array_get(&model->meshes, out_mesh_handle);
		memset(out_mesh, 0, sizeof(*out_mesh));
		s_array_init(&out_mesh->cpu.vertices);
		s_array_reserve(&out_mesh->cpu.vertices, vertex_count);

		for (u32 v = 0; v < vertex_count; v++) {
			const u8 *ptr = pos_view.data + ((sz)v * pos_view.stride);
			const f32 *fptr = (const f32 *)ptr;
			s_handle vertex_handle = s_array_increment(&out_mesh->cpu.vertices);
			se_vertex_3d *new_vertex = s_array_get(&out_mesh->cpu.vertices, vertex_handle);
			new_vertex->position = s_vec3(fptr[0], fptr[1], fptr[2]);
			new_vertex->normal = s_vec3(0.0f, 0.0f, 1.0f);
			new_vertex->uv = s_vec2(0.0f, 0.0f);
		}

		if (norm_attr != NULL) {
			if (norm_attr->accessor < 0 || (sz)norm_attr->accessor >= s_array_get_size(&asset->accessors)) {
				error = SE_RESULT_IO;
				goto fail;
			}
			s_handle norm_handle = s_array_handle(accessors, (u32)norm_attr->accessor);
			se_gltf_accessor *nacc = s_array_get(accessors, norm_handle);
			if (nacc == NULL || nacc->component_type != 5126 || nacc->type != SE_GLTF_ACCESSOR_VEC3) {
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
				s_handle vertex_handle = s_array_handle(&out_mesh->cpu.vertices, v);
				se_vertex_3d *current_vertex = s_array_get(&out_mesh->cpu.vertices, vertex_handle);
				current_vertex->normal = s_vec3(fptr[0], fptr[1], fptr[2]);
			}
		}

		if (uv_attr != NULL) {
			if (uv_attr->accessor < 0 || (sz)uv_attr->accessor >= s_array_get_size(&asset->accessors)) {
				error = SE_RESULT_IO;
				goto fail;
			}
			s_handle uv_handle = s_array_handle(accessors, (u32)uv_attr->accessor);
			se_gltf_accessor *uacc = s_array_get(accessors, uv_handle);
			if (uacc == NULL || uacc->component_type != 5126 || uacc->type != SE_GLTF_ACCESSOR_VEC2) {
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
				s_handle vertex_handle = s_array_handle(&out_mesh->cpu.vertices, v);
				se_vertex_3d *current_vertex = s_array_get(&out_mesh->cpu.vertices, vertex_handle);
				current_vertex->uv = s_vec2(fptr[0], fptr[1]);
			}
		}

		u32 index_count = vertex_count;
		if (prim->has_indices) {
			if (prim->indices < 0 || (sz)prim->indices >= s_array_get_size(&asset->accessors)) {
				error = SE_RESULT_IO;
				goto fail;
			}

			s_handle index_handle = s_array_handle(accessors, (u32)prim->indices);
			se_gltf_accessor *iacc = s_array_get(accessors, index_handle);
			if (iacc == NULL || iacc->type != SE_GLTF_ACCESSOR_SCALAR || iacc->count == 0) {
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

			s_array_init(&out_mesh->cpu.indices);
			s_array_reserve(&out_mesh->cpu.indices, index_count);

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
				s_handle idx_handle = s_array_increment(&out_mesh->cpu.indices);
				u32 *index_ptr = s_array_get(&out_mesh->cpu.indices, idx_handle);
				*index_ptr = value;
			}
		} else {
			s_array_init(&out_mesh->cpu.indices);
			s_array_reserve(&out_mesh->cpu.indices, index_count);
			for (u32 idx = 0; idx < index_count; idx++) {
				s_handle idx_handle = s_array_increment(&out_mesh->cpu.indices);
				u32 *index_ptr = s_array_get(&out_mesh->cpu.indices, idx_handle);
				*index_ptr = idx;
			}
		}

		if (!se_gltf_mesh_finalize(out_mesh, asset->source_path, mesh_data_flags)) {
			error = SE_RESULT_OUT_OF_MEMORY;
			goto fail;
		}
	}

	se_set_last_error(SE_RESULT_OK);
	return model_handle;

fail:
	se_gltf_model_rollback(ctx, model_handle);
	se_set_last_error(error);
	return S_HANDLE_NULL;
}

se_model_handle se_gltf_model_load(const se_gltf_asset *asset, const i32 mesh_index) {
	return se_gltf_model_load_ex(asset, mesh_index, SE_MESH_DATA_GPU);
}

se_texture_handle se_gltf_image_load(const se_gltf_asset *asset, const i32 image_index, const se_texture_wrap wrap) {
	se_context *ctx = se_current_context();
	if (ctx == NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (image_index < 0 || (sz)image_index >= s_array_get_size(&asset->images)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_gltf_images *images = (se_gltf_images *)&asset->images;
	s_handle image_handle = s_array_handle(images, (u32)image_index);
	se_gltf_image *image = s_array_get(images, image_handle);
	if (image == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (image->data && image->data_size > 0) {
		return se_texture_load_from_memory(image->data, image->data_size, wrap);
	}

	if (image->uri && image->uri[0] != '\0') {
		if (strncmp(image->uri, "data:", 5) == 0) {
			u8 *decoded = NULL;
			sz decoded_size = 0;
			char *mime = NULL;
			if (se_loader_parse_data_uri(image->uri, &mime, &decoded, &decoded_size)) {
				se_texture_handle texture = se_texture_load_from_memory(decoded, decoded_size, wrap);
				free(mime);
				free(decoded);
				return texture;
			}
			free(mime);
			free(decoded);
			se_set_last_error(SE_RESULT_IO);
			return S_HANDLE_NULL;
		}

		if (!se_loader_path_is_absolute(image->uri) && asset->base_dir && asset->base_dir[0] != '\0') {
			char full_path[SE_MAX_PATH_LENGTH] = {0};
			if (s_path_join(full_path, SE_MAX_PATH_LENGTH, asset->base_dir, image->uri)) {
				se_texture_handle texture = se_texture_load(full_path, wrap);
				if (texture != S_HANDLE_NULL) {
					return texture;
				}
			}
		}

		return se_texture_load(image->uri, wrap);
	}

	se_set_last_error(SE_RESULT_NOT_FOUND);
	return S_HANDLE_NULL;
}

se_texture_handle se_gltf_texture_load(const se_gltf_asset *asset, const i32 texture_index, const se_texture_wrap wrap) {
	se_context *ctx = se_current_context();
	if (ctx == NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (texture_index < 0 || (sz)texture_index >= s_array_get_size(&asset->textures)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_gltf_textures *textures = (se_gltf_textures *)&asset->textures;
	s_handle texture_handle = s_array_handle(textures, (u32)texture_index);
	se_gltf_texture *texture = s_array_get(textures, texture_handle);
	if (texture == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (!texture->has_source) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}
	if (texture->source < 0 || (sz)texture->source >= s_array_get_size(&asset->images)) {
		se_set_last_error(SE_RESULT_IO);
		return S_HANDLE_NULL;
	}

	return se_gltf_image_load(asset, texture->source, wrap);
}

se_model_handle se_gltf_model_load_first(const char *path, const se_gltf_load_params *params) {
	se_context *ctx = se_current_context();
	if (ctx == NULL || path == NULL || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	se_gltf_asset *asset = se_gltf_load(path, params);
	if (asset == NULL) {
		return S_HANDLE_NULL;
	}

	if (s_array_get_size(&asset->meshes) == 0) {
		se_gltf_free(asset);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}

	se_model_handle model = se_gltf_model_load(asset, 0);
	se_result result = se_get_last_error();
	se_gltf_free(asset);
	se_set_last_error(result);
	return model;
}

sz se_gltf_scene_load_from_asset(const se_scene_3d_handle scene, const se_gltf_asset *asset, const se_shader_handle mesh_shader, const se_texture_handle default_texture, const se_texture_wrap wrap) {
	se_context *context = se_current_context();
	if (context == NULL || scene == S_HANDLE_NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (s_array_get_size(&asset->meshes) == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}

	se_models_ptr model_cache = {0};
	s_array_init(&model_cache);
	s_array_reserve(&model_cache, s_array_get_size(&asset->meshes));
	for (sz i = 0; i < s_array_get_size(&asset->meshes); i++) {
		s_handle entry = s_array_increment(&model_cache);
		se_model_handle *slot = s_array_get(&model_cache, entry);
		*slot = S_HANDLE_NULL;
	}

	se_textures_ptr texture_cache = {0};
	s_array_init(&texture_cache);
	s_array_reserve(&texture_cache, s_array_get_size(&asset->textures));
	for (sz i = 0; i < s_array_get_size(&asset->textures); i++) {
		s_handle entry = s_array_increment(&texture_cache);
		se_texture_handle *slot = s_array_get(&texture_cache, entry);
		*slot = S_HANDLE_NULL;
	}

	se_shaders_ptr material_shader_cache = {0};
	s_array_init(&material_shader_cache);
	s_array_reserve(&material_shader_cache, s_array_get_size(&asset->materials));
	for (sz i = 0; i < s_array_get_size(&asset->materials); i++) {
		s_handle entry = s_array_increment(&material_shader_cache);
		se_shader_handle *slot = s_array_get(&material_shader_cache, entry);
		*slot = S_HANDLE_NULL;
	}

	se_gltf_scene_build_context build_ctx = {0};
	build_ctx.ctx = context;
	build_ctx.scene = scene;
	build_ctx.asset = asset;
	build_ctx.mesh_shader = mesh_shader;
	build_ctx.default_texture = default_texture;
	build_ctx.wrap = wrap;
	build_ctx.model_cache = &model_cache;
	build_ctx.texture_cache = &texture_cache;
	build_ctx.material_shader_cache = &material_shader_cache;

	b8 ok = true;
	if (s_array_get_size(&asset->scenes) > 0) {
		i32 scene_index = 0;
		if (asset->has_default_scene) {
			scene_index = asset->default_scene;
		}
		if (scene_index < 0 || (sz)scene_index >= s_array_get_size(&asset->scenes)) {
			ok = false;
			se_set_last_error(SE_RESULT_IO);
		} else {
			se_gltf_scenes *scenes = (se_gltf_scenes *)&asset->scenes;
			s_handle scene_handle = s_array_handle(scenes, (u32)scene_index);
			se_gltf_scene *default_scene = s_array_get(scenes, scene_handle);
			if (default_scene == NULL) {
				ok = false;
				se_set_last_error(SE_RESULT_IO);
			} else {
			s_mat4 identity = s_mat4_identity;
			for (sz i = 0; i < s_array_get_size(&default_scene->nodes); i++) {
				s_handle node_handle = s_array_handle(&default_scene->nodes, (u32)i);
				i32 *root_node = s_array_get(&default_scene->nodes, node_handle);
				if (root_node == NULL || !se_gltf_add_node_objects_recursive(&build_ctx, *root_node, &identity, 0)) {
					ok = false;
					if (se_get_last_error() == SE_RESULT_OK) {
						se_set_last_error(SE_RESULT_IO);
					}
					break;
				}
			}
		}
		}
	} else {
		ok = se_gltf_add_mesh_objects_fallback(&build_ctx);
		if (!ok && se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_IO);
		}
	}

	s_array_clear(&texture_cache);
	s_array_clear(&material_shader_cache);
	s_array_clear(&model_cache);

	if (!ok || build_ctx.added_objects == 0) {
		if (ok) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
		}
		return 0;
	}

	se_set_last_error(SE_RESULT_OK);
	return build_ctx.added_objects;
}

se_gltf_asset *se_gltf_scene_load(const se_scene_3d_handle scene, const char *path, const se_gltf_load_params *load_params, const se_shader_handle mesh_shader, const se_texture_handle default_texture, const se_texture_wrap wrap, sz *out_objects_loaded) {
	se_context *ctx = se_current_context();
	if (out_objects_loaded) {
		*out_objects_loaded = 0;
	}
	if (ctx == NULL || scene == S_HANDLE_NULL || path == NULL || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_gltf_asset *asset = se_gltf_load(path, load_params);
	if (asset == NULL) {
		return NULL;
	}

	sz objects_loaded = se_gltf_scene_load_from_asset(scene, asset, mesh_shader, default_texture, wrap);
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

	if (s_array_get_size(&asset->scenes) > 0 && scene_index >= 0 && (sz)scene_index < s_array_get_size(&asset->scenes)) {
		se_gltf_scenes *scenes = (se_gltf_scenes *)&asset->scenes;
		s_handle scene_handle = s_array_handle(scenes, (u32)scene_index);
		se_gltf_scene *scene = s_array_get(scenes, scene_handle);
		if (scene == NULL) {
			se_set_last_error(SE_RESULT_IO);
			return false;
		}
		s_mat4 identity = s_mat4_identity;
		for (sz i = 0; i < s_array_get_size(&scene->nodes); i++) {
			s_handle node_handle = s_array_handle(&scene->nodes, (u32)i);
			i32 *root_node = s_array_get(&scene->nodes, node_handle);
			if (root_node == NULL) continue;
			se_gltf_scene_compute_bounds_recursive(asset, *root_node, &identity, &min_bounds, &max_bounds, &has_bounds, 0);
		}
	}

	if (!has_bounds) {
		for (sz mesh_index = 0; mesh_index < s_array_get_size(&asset->meshes); mesh_index++) {
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

void se_gltf_scene_fit_camera(const se_scene_3d_handle scene, const se_gltf_asset *asset) {
	se_context *ctx = se_current_context();
	if (ctx == NULL || scene == S_HANDLE_NULL || asset == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	se_scene_3d *scene_ptr = s_array_get(&ctx->scenes_3d, scene);
	if (scene_ptr == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	if (scene_ptr->camera == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	s_vec3 bounds_center = s_vec3(0.0f, 0.0f, 0.0f);
	f32 bounds_radius = 1.0f;
	b8 has_bounds = se_gltf_scene_compute_bounds(asset, &bounds_center, &bounds_radius);

	se_camera *camera = s_array_get(&ctx->cameras, scene_ptr->camera);
	if (camera == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
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
