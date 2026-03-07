// Syphax-Engine - Ougi Washi

#include "se_scene.h"
#include "se_debug.h"
#include "se_graphics.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"
#include "render/se_gl.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Scene/object memory is owned by the se_context arrays.

#define SE_OBJECT_2D_VERTEX_SHADER_PATH SE_RESOURCE_INTERNAL("shaders/object_2d_vertex.glsl")
#define SE_SCENE_2D_JSON_FORMAT "se_scene_2d_json"
#define SE_SCENE_3D_JSON_FORMAT "se_scene_3d_json"
#define SE_OBJECT_2D_JSON_FORMAT "se_object_2d_json"
#define SE_OBJECT_3D_JSON_FORMAT "se_object_3d_json"
#define SE_SCENE_JSON_MAX_SAFE_INTEGER_U64 9007199254740991ULL

enum {
	SE_SCENE_2D_JSON_VERSION = 1u,
	SE_SCENE_3D_JSON_VERSION = 1u,
	SE_OBJECT_2D_JSON_VERSION = 1u,
	SE_OBJECT_3D_JSON_VERSION = 1u
};

static void se_scene_json_serialize_path(const c8* input_path, c8* out_path, const sz out_path_size) {
	if (!out_path || out_path_size == 0) {
		return;
	}
	out_path[0] = '\0';
	if (!input_path || input_path[0] == '\0') {
		return;
	}

	const c8* path = input_path;
	const c8* root = se_paths_get_resource_root();
	if (root && root[0] != '\0') {
		const sz root_len = strlen(root);
		if (root_len > 0 && strncmp(path, root, root_len) == 0) {
			const c8* relative = path + root_len;
			if (relative[0] != '\0') {
				path = relative;
			}
		}
	}

	const sz path_len = strlen(path);
	if (path_len + 1 > out_path_size) {
		return;
	}
	memcpy(out_path, path, path_len + 1);
}

static b8 se_scene_json_write_file(const c8* path, s_json* root) {
	c8* text = NULL;
	b8 ok = false;
	if (!path || path[0] == '\0' || !root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	text = s_json_stringify(root);
	if (!text) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}
	ok = s_file_write(path, text, strlen(text));
	free(text);
	if (!ok) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_scene_json_read_file(const c8* path, s_json** out_root) {
	c8* text = NULL;
	s_json* root = NULL;
	if (!path || path[0] == '\0' || !out_root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_root = NULL;
	if (!s_file_read(path, &text, NULL)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	root = s_json_parse(text);
	free(text);
	if (!root) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	*out_root = root;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static b8 se_scene_2d_json_add_child(s_json* parent, s_json* child) {
	if (!parent || !child || !s_json_add(parent, child)) {
		if (child) {
			s_json_free(child);
		}
		return false;
	}
	return true;
}

static s_json* se_scene_2d_json_add_object(s_json* parent, const c8* name) {
	s_json* object = s_json_object_empty(name);
	if (!object) {
		return NULL;
	}
	if (!se_scene_2d_json_add_child(parent, object)) {
		return NULL;
	}
	return object;
}

static s_json* se_scene_2d_json_add_array(s_json* parent, const c8* name) {
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return NULL;
	}
	if (!se_scene_2d_json_add_child(parent, array)) {
		return NULL;
	}
	return array;
}

static b8 se_scene_2d_json_add_u32(s_json* parent, const c8* name, const u32 value) {
	s_json* node = s_json_num(name, (f64)value);
	return se_scene_2d_json_add_child(parent, node);
}

static b8 se_scene_2d_json_add_i32(s_json* parent, const c8* name, const i32 value) {
	s_json* node = s_json_num(name, (f64)value);
	return se_scene_2d_json_add_child(parent, node);
}

static b8 se_scene_2d_json_add_f32(s_json* parent, const c8* name, const f32 value) {
	if (!isfinite((f64)value)) {
		return false;
	}
	s_json* node = s_json_num(name, (f64)value);
	return se_scene_2d_json_add_child(parent, node);
}

static b8 se_scene_2d_json_add_bool(s_json* parent, const c8* name, const b8 value) {
	s_json* node = s_json_bool(name, value);
	return se_scene_2d_json_add_child(parent, node);
}

static b8 se_scene_2d_json_add_string(s_json* parent, const c8* name, const c8* value) {
	s_json* node = s_json_str(name, value ? value : "");
	return se_scene_2d_json_add_child(parent, node);
}

static b8 se_scene_2d_json_add_vec2(s_json* parent, const c8* name, const s_vec2* value) {
	if (!parent || !value) {
		return false;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return false;
	}
	if (!se_scene_2d_json_add_f32(array, NULL, value->x) ||
		!se_scene_2d_json_add_f32(array, NULL, value->y)) {
		s_json_free(array);
		return false;
	}
	if (!se_scene_2d_json_add_child(parent, array)) {
		return false;
	}
	return true;
}

static b8 se_scene_json_add_mat3(s_json* parent, const c8* name, const s_mat3* value) {
	if (!parent || !value) {
		return false;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return false;
	}
	for (u32 row = 0; row < 3u; ++row) {
		for (u32 col = 0; col < 3u; ++col) {
			if (!se_scene_2d_json_add_f32(array, NULL, value->m[row][col])) {
				s_json_free(array);
				return false;
			}
		}
	}
	if (!se_scene_2d_json_add_child(parent, array)) {
		return false;
	}
	return true;
}

static b8 se_scene_json_add_mat4(s_json* parent, const c8* name, const s_mat4* value) {
	if (!parent || !value) {
		return false;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return false;
	}
	for (u32 row = 0; row < 4u; ++row) {
		for (u32 col = 0; col < 4u; ++col) {
			if (!se_scene_2d_json_add_f32(array, NULL, value->m[row][col])) {
				s_json_free(array);
				return false;
			}
		}
	}
	if (!se_scene_2d_json_add_child(parent, array)) {
		return false;
	}
	return true;
}

static b8 se_scene_json_number_to_u64(const s_json* node, u64* out_value) {
	if (!node || node->type != S_JSON_NUMBER || !out_value) {
		return false;
	}
	const f64 value = node->as.number;
	if (!isfinite(value) || value < 0.0 || value > (f64)SE_SCENE_JSON_MAX_SAFE_INTEGER_U64) {
		return false;
	}
	const u64 integer = (u64)value;
	if ((f64)integer != value) {
		return false;
	}
	*out_value = integer;
	return true;
}

static b8 se_scene_json_number_to_i64(const s_json* node, i64* out_value) {
	if (!node || node->type != S_JSON_NUMBER || !out_value) {
		return false;
	}
	const f64 value = node->as.number;
	const f64 min_value = -(f64)SE_SCENE_JSON_MAX_SAFE_INTEGER_U64;
	const f64 max_value = (f64)SE_SCENE_JSON_MAX_SAFE_INTEGER_U64;
	if (!isfinite(value) || value < min_value || value > max_value) {
		return false;
	}
	const i64 integer = (i64)value;
	if ((f64)integer != value) {
		return false;
	}
	*out_value = integer;
	return true;
}

static b8 se_scene_json_number_to_f64(const s_json* node, f64* out_value) {
	if (!node || node->type != S_JSON_NUMBER || !out_value) {
		return false;
	}
	if (!isfinite(node->as.number)) {
		return false;
	}
	*out_value = node->as.number;
	return true;
}

static b8 se_scene_json_number_to_u32(const s_json* node, u32* out_value) {
	u64 value = 0u;
	if (!se_scene_json_number_to_u64(node, &value) || value > (u64)UINT32_MAX) {
		return false;
	}
	*out_value = (u32)value;
	return true;
}

static b8 se_scene_json_number_to_i32(const s_json* node, i32* out_value) {
	i64 value = 0;
	if (!se_scene_json_number_to_i64(node, &value) || value < (i64)INT32_MIN || value > (i64)INT32_MAX) {
		return false;
	}
	*out_value = (i32)value;
	return true;
}

static b8 se_scene_json_number_to_f32(const s_json* node, f32* out_value) {
	f64 value = 0.0;
	if (!se_scene_json_number_to_f64(node, &value) || value < -(f64)FLT_MAX || value > (f64)FLT_MAX) {
		return false;
	}
	*out_value = (f32)value;
	return true;
}

static b8 se_scene_json_bool_to_b8(const s_json* node, b8* out_value) {
	if (!node || node->type != S_JSON_BOOL || !out_value) {
		return false;
	}
	*out_value = node->as.boolean;
	return true;
}

static b8 se_scene_json_string_to_c8(const s_json* node, const c8** out_value) {
	if (!node || node->type != S_JSON_STRING || !out_value) {
		return false;
	}
	*out_value = node->as.string ? node->as.string : "";
	return true;
}

static const s_json* se_scene_json_get_required(const s_json* object, const c8* key, const s_json_type type) {
	if (!object || object->type != S_JSON_OBJECT || !key) {
		return NULL;
	}
	const s_json* child = s_json_get(object, key);
	if (!child || child->type != type) {
		return NULL;
	}
	return child;
}

static b8 se_scene_2d_json_read_vec2_node(const s_json* node, s_vec2* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 2u) {
		return false;
	}
	f32 values[2] = {0.0f, 0.0f};
	for (u32 i = 0; i < 2u; ++i) {
		const s_json* item = s_json_at(node, i);
		if (!se_scene_json_number_to_f32(item, &values[i])) {
			return false;
		}
	}
	*out_value = s_vec2(values[0], values[1]);
	return true;
}

static b8 se_scene_json_read_mat3(const s_json* node, s_mat3* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 9u) {
		return false;
	}
	s_mat3 parsed = s_mat3_identity;
	u32 index = 0u;
	for (u32 row = 0; row < 3u; ++row) {
		for (u32 col = 0; col < 3u; ++col) {
			const s_json* item = s_json_at(node, index++);
			if (!se_scene_json_number_to_f32(item, &parsed.m[row][col])) {
				return false;
			}
		}
	}
	*out_value = parsed;
	return true;
}

static b8 se_scene_json_read_mat4(const s_json* node, s_mat4* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 16u) {
		return false;
	}
	s_mat4 parsed = s_mat4_identity;
	u32 index = 0u;
	for (u32 row = 0; row < 4u; ++row) {
		for (u32 col = 0; col < 4u; ++col) {
			const s_json* item = s_json_at(node, index++);
			if (!se_scene_json_number_to_f32(item, &parsed.m[row][col])) {
				return false;
			}
		}
	}
	*out_value = parsed;
	return true;
}

static b8 se_object_2d_json_read_slot(
	const s_json* slot_json,
	i32* out_instance_id,
	b8* out_active,
	s_mat3* out_transform,
	s_mat4* out_buffer,
	s_mat4* out_metadata) {
	if (!slot_json || slot_json->type != S_JSON_OBJECT ||
		!out_instance_id || !out_active || !out_transform || !out_buffer || !out_metadata) {
		return false;
	}

	const s_json* transform_json = se_scene_json_get_required(slot_json, "transform", S_JSON_ARRAY);
	const s_json* buffer_json = se_scene_json_get_required(slot_json, "buffer", S_JSON_ARRAY);
	const s_json* metadata_json = se_scene_json_get_required(slot_json, "metadata", S_JSON_ARRAY);
	if (!se_scene_json_number_to_i32(se_scene_json_get_required(slot_json, "id", S_JSON_NUMBER), out_instance_id) ||
		!se_scene_json_bool_to_b8(se_scene_json_get_required(slot_json, "active", S_JSON_BOOL), out_active) ||
		!transform_json || !buffer_json || !metadata_json ||
		!se_scene_json_read_mat3(transform_json, out_transform) ||
		!se_scene_json_read_mat4(buffer_json, out_buffer) ||
		!se_scene_json_read_mat4(metadata_json, out_metadata)) {
		return false;
	}
	return true;
}

static b8 se_object_3d_json_read_slot(
	const s_json* slot_json,
	i32* out_instance_id,
	b8* out_active,
	s_mat4* out_transform,
	s_mat4* out_buffer,
	s_mat4* out_metadata) {
	if (!slot_json || slot_json->type != S_JSON_OBJECT ||
		!out_instance_id || !out_active || !out_transform || !out_buffer || !out_metadata) {
		return false;
	}

	const s_json* transform_json = se_scene_json_get_required(slot_json, "transform", S_JSON_ARRAY);
	const s_json* buffer_json = se_scene_json_get_required(slot_json, "buffer", S_JSON_ARRAY);
	const s_json* metadata_json = se_scene_json_get_required(slot_json, "metadata", S_JSON_ARRAY);
	if (!se_scene_json_number_to_i32(se_scene_json_get_required(slot_json, "id", S_JSON_NUMBER), out_instance_id) ||
		!se_scene_json_bool_to_b8(se_scene_json_get_required(slot_json, "active", S_JSON_BOOL), out_active) ||
		!transform_json || !buffer_json || !metadata_json ||
		!se_scene_json_read_mat4(transform_json, out_transform) ||
		!se_scene_json_read_mat4(buffer_json, out_buffer) ||
		!se_scene_json_read_mat4(metadata_json, out_metadata)) {
		return false;
	}
	return true;
}

static se_object_2d* se_object_2d_from_handle(se_context *ctx, const se_object_2d_handle object) {
	return s_array_get(&ctx->objects_2d, object);
}

static se_object_3d* se_object_3d_from_handle(se_context *ctx, const se_object_3d_handle object) {
	return s_array_get(&ctx->objects_3d, object);
}

static se_scene_2d* se_scene_2d_from_handle(se_context *ctx, const se_scene_2d_handle scene) {
	return s_array_get(&ctx->scenes_2d, scene);
}

static se_scene_3d* se_scene_3d_from_handle(se_context *ctx, const se_scene_3d_handle scene) {
	return s_array_get(&ctx->scenes_3d, scene);
}

static se_model* se_model_from_handle(se_context *ctx, const se_model_handle model) {
	return s_array_get(&ctx->models, model);
}

static s_vec3 se_mat4_extract_basis_scale(const s_mat4 *transform) {
	if (!transform) {
		return s_vec3(1.0f, 1.0f, 1.0f);
	}

	const f32 sx = sqrtf((transform->m[0][0] * transform->m[0][0]) +
		(transform->m[0][1] * transform->m[0][1]) +
		(transform->m[0][2] * transform->m[0][2]));
	const f32 sy = sqrtf((transform->m[1][0] * transform->m[1][0]) +
		(transform->m[1][1] * transform->m[1][1]) +
		(transform->m[1][2] * transform->m[1][2]));
	const f32 sz = sqrtf((transform->m[2][0] * transform->m[2][0]) +
		(transform->m[2][1] * transform->m[2][1]) +
		(transform->m[2][2] * transform->m[2][2]));
	return s_vec3(sx, sy, sz);
}

static s_mat4 se_mat4_extract_rotation(const s_mat4 *transform, const s_vec3 *scale) {
	s_mat4 rotation = s_mat4_identity;
	if (!transform || !scale) {
		return rotation;
	}

	const f32 epsilon = 0.000001f;

	if (fabsf(scale->x) > epsilon) {
		rotation.m[0][0] = transform->m[0][0] / scale->x;
		rotation.m[0][1] = transform->m[0][1] / scale->x;
		rotation.m[0][2] = transform->m[0][2] / scale->x;
	}
	if (fabsf(scale->y) > epsilon) {
		rotation.m[1][0] = transform->m[1][0] / scale->y;
		rotation.m[1][1] = transform->m[1][1] / scale->y;
		rotation.m[1][2] = transform->m[1][2] / scale->y;
	}
	if (fabsf(scale->z) > epsilon) {
		rotation.m[2][0] = transform->m[2][0] / scale->z;
		rotation.m[2][1] = transform->m[2][1] / scale->z;
		rotation.m[2][2] = transform->m[2][2] / scale->z;
	}
	return rotation;
}

static s_mat4 se_mat4_rotation_xyz(const s_vec3 *rotation) {
	if (!rotation) {
		return s_mat4_identity;
	}

	const f32 cx = cosf(rotation->x);
	const f32 sx = sinf(rotation->x);
	const f32 cy = cosf(rotation->y);
	const f32 sy = sinf(rotation->y);
	const f32 cz = cosf(rotation->z);
	const f32 sz = sinf(rotation->z);

	return s_mat4(
		cy * cz,
		(sx * sy * cz) + (cx * sz),
		(-cx * sy * cz) + (sx * sz),
		0.0f,

		-cy * sz,
		(-sx * sy * sz) + (cx * cz),
		(cx * sy * sz) + (sx * cz),
		0.0f,

		sy,
		-sx * cy,
		cx * cy,
		0.0f,

		0.0f, 0.0f, 0.0f, 1.0f);
}

static void se_mat4_apply_basis_scale(s_mat4 *transform, const s_vec3 *scale) {
	if (!transform || !scale) {
		return;
	}

	transform->m[0][0] *= scale->x;
	transform->m[0][1] *= scale->x;
	transform->m[0][2] *= scale->x;
	transform->m[1][0] *= scale->y;
	transform->m[1][1] *= scale->y;
	transform->m[1][2] *= scale->y;
	transform->m[2][0] *= scale->z;
	transform->m[2][1] *= scale->z;
	transform->m[2][2] *= scale->z;
}

static b8 se_object_2d_handle_exists(const se_context* ctx, const se_object_2d_handle handle) {
	return ctx && handle != S_HANDLE_NULL && s_array_get((se_objects_2d*)&ctx->objects_2d, handle) != NULL;
}

static b8 se_object_3d_handle_exists(const se_context* ctx, const se_object_3d_handle handle) {
	return ctx && handle != S_HANDLE_NULL && s_array_get((se_objects_3d*)&ctx->objects_3d, handle) != NULL;
}

static b8 se_model_handle_exists(const se_context* ctx, const se_model_handle handle) {
	return ctx && handle != S_HANDLE_NULL && s_array_get((se_models*)&ctx->models, handle) != NULL;
}

static b8 se_shader_handle_exists(const se_context* ctx, const se_shader_handle handle) {
	return ctx && handle != S_HANDLE_NULL && s_array_get((se_shaders*)&ctx->shaders, handle) != NULL;
}

static b8 se_object_2d_list_contains(const se_objects_2d_ptr* list, const se_object_2d_handle handle) {
	if (!list || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size((se_objects_2d_ptr*)list); ++i) {
		se_object_2d_handle* current = s_array_get((se_objects_2d_ptr*)list, s_array_handle((se_objects_2d_ptr*)list, (u32)i));
		if (current && *current == handle) {
			return true;
		}
	}
	return false;
}

static b8 se_object_3d_list_contains(const se_objects_3d_ptr* list, const se_object_3d_handle handle) {
	if (!list || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size((se_objects_3d_ptr*)list); ++i) {
		se_object_3d_handle* current = s_array_get((se_objects_3d_ptr*)list, s_array_handle((se_objects_3d_ptr*)list, (u32)i));
		if (current && *current == handle) {
			return true;
		}
	}
	return false;
}

static b8 se_model_list_contains(const se_models_ptr* list, const se_model_handle handle) {
	if (!list || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size((se_models_ptr*)list); ++i) {
		se_model_handle* current = s_array_get((se_models_ptr*)list, s_array_handle((se_models_ptr*)list, (u32)i));
		if (current && *current == handle) {
			return true;
		}
	}
	return false;
}

static b8 se_shader_list_contains(const se_shaders_ptr* list, const se_shader_handle handle) {
	if (!list || handle == S_HANDLE_NULL) {
		return false;
	}
	for (sz i = 0; i < s_array_get_size((se_shaders_ptr*)list); ++i) {
		se_shader_handle* current = s_array_get((se_shaders_ptr*)list, s_array_handle((se_shaders_ptr*)list, (u32)i));
		if (current && *current == handle) {
			return true;
		}
	}
	return false;
}

static void se_object_2d_list_add_unique(se_objects_2d_ptr* list, const se_object_2d_handle handle) {
	if (!list || handle == S_HANDLE_NULL || se_object_2d_list_contains(list, handle)) {
		return;
	}
	s_array_add(list, handle);
}

static void se_object_3d_list_add_unique(se_objects_3d_ptr* list, const se_object_3d_handle handle) {
	if (!list || handle == S_HANDLE_NULL || se_object_3d_list_contains(list, handle)) {
		return;
	}
	s_array_add(list, handle);
}

static void se_model_list_add_unique(se_models_ptr* list, const se_model_handle handle) {
	if (!list || handle == S_HANDLE_NULL || se_model_list_contains(list, handle)) {
		return;
	}
	s_array_add(list, handle);
}

static void se_shader_list_add_unique(se_shaders_ptr* list, const se_shader_handle handle) {
	if (!list || handle == S_HANDLE_NULL || se_shader_list_contains(list, handle)) {
		return;
	}
	s_array_add(list, handle);
}

static void se_scene_2d_collect_existing_shaders(const se_context* ctx, se_shaders_ptr* out_handles) {
	if (!ctx || !out_handles) {
		return;
	}
	s_array_init(out_handles);
	s_array_reserve(out_handles, s_array_get_size((se_shaders*)&ctx->shaders));
	for (sz i = 0; i < s_array_get_size((se_shaders*)&ctx->shaders); ++i) {
		const se_shader_handle shader_handle = s_array_handle((se_shaders*)&ctx->shaders, (u32)i);
		if (!se_shader_handle_exists(ctx, shader_handle)) {
			continue;
		}
		se_shader_list_add_unique(out_handles, shader_handle);
	}
}

static void se_scene_2d_cleanup_loaded_objects(
	se_context* ctx,
	se_objects_2d_ptr* loaded_objects,
	const se_shaders_ptr* pre_existing_shaders,
	se_shaders_ptr* loaded_shaders) {
	if (!ctx || !loaded_objects || !loaded_shaders) {
		return;
	}

	for (sz i = 0; i < s_array_get_size(loaded_objects); ++i) {
		se_object_2d_handle* handle = s_array_get(loaded_objects, s_array_handle(loaded_objects, (u32)i));
		if (!handle || !se_object_2d_handle_exists(ctx, *handle)) {
			continue;
		}
		se_object_2d_destroy(*handle);
	}

	for (sz i = 0; i < s_array_get_size(loaded_shaders); ++i) {
		se_shader_handle* shader_handle = s_array_get(loaded_shaders, s_array_handle(loaded_shaders, (u32)i));
		if (!shader_handle || !se_shader_handle_exists(ctx, *shader_handle)) {
			continue;
		}
		if (pre_existing_shaders && se_shader_list_contains(pre_existing_shaders, *shader_handle)) {
			continue;
		}
		se_shader_destroy(*shader_handle);
	}

	s_array_clear(loaded_shaders);
	s_array_clear(loaded_objects);
}

static b8 se_scene_3d_find_shader_by_paths(
	const se_context* ctx,
	const c8* vertex_path,
	const c8* fragment_path,
	se_shader_handle* out_handle) {
	if (!ctx || !vertex_path || !fragment_path || !out_handle) {
		return false;
	}
	c8 resolved_vertex_path[SE_MAX_PATH_LENGTH] = {0};
	c8 resolved_fragment_path[SE_MAX_PATH_LENGTH] = {0};
	const c8* expected_vertex_path = vertex_path;
	const c8* expected_fragment_path = fragment_path;
	if (se_paths_resolve_resource_path(resolved_vertex_path, sizeof(resolved_vertex_path), vertex_path)) {
		expected_vertex_path = resolved_vertex_path;
	}
	if (se_paths_resolve_resource_path(resolved_fragment_path, sizeof(resolved_fragment_path), fragment_path)) {
		expected_fragment_path = resolved_fragment_path;
	}
	for (sz i = 0; i < s_array_get_size((se_shaders*)&ctx->shaders); ++i) {
		const se_shader_handle handle = s_array_handle((se_shaders*)&ctx->shaders, (u32)i);
		const se_shader* shader = s_array_get((se_shaders*)&ctx->shaders, handle);
		if (!shader) {
			continue;
		}
		if (strcmp(shader->vertex_path, expected_vertex_path) == 0 &&
			strcmp(shader->fragment_path, expected_fragment_path) == 0) {
			*out_handle = handle;
			return true;
		}
	}
	return false;
}

static b8 se_scene_3d_model_get_source_path(const se_model* model, const c8** out_path) {
	if (!model || !out_path) {
		return false;
	}
	*out_path = NULL;
	const sz mesh_count = s_array_get_size((se_meshes*)&model->meshes);
	if (mesh_count == 0) {
		return false;
	}
	for (sz i = 0; i < mesh_count; ++i) {
		se_mesh* mesh = s_array_get((se_meshes*)&model->meshes, s_array_handle((se_meshes*)&model->meshes, (u32)i));
		if (!mesh) {
			return false;
		}
		const c8* mesh_source_path = s_array_get_data(&mesh->cpu.file_path);
		if (!mesh_source_path || mesh_source_path[0] == '\0') {
			return false;
		}
		if (*out_path == NULL) {
			*out_path = mesh_source_path;
			continue;
		}
		if (strcmp(*out_path, mesh_source_path) != 0) {
			return false;
		}
	}
	return *out_path != NULL;
}

static b8 se_scene_3d_model_index_of(const se_models_ptr* models, const se_model_handle model, u32* out_index) {
	if (!models || !out_index) {
		return false;
	}
	for (u32 i = 0; i < (u32)s_array_get_size((se_models_ptr*)models); ++i) {
		const se_model_handle* current = s_array_get((se_models_ptr*)models, s_array_handle((se_models_ptr*)models, i));
		if (current && *current == model) {
			*out_index = i;
			return true;
		}
	}
	return false;
}

static void se_scene_3d_cleanup_loaded_data(
	se_context* ctx,
	se_objects_3d_ptr* loaded_objects,
	se_models_ptr* loaded_models,
	const se_shaders_ptr* pre_existing_shaders,
	se_shaders_ptr* loaded_shaders) {
	if (!ctx) {
		return;
	}

	if (loaded_objects) {
		for (sz i = 0; i < s_array_get_size(loaded_objects); ++i) {
			se_object_3d_handle* object_handle = s_array_get(loaded_objects, s_array_handle(loaded_objects, (u32)i));
			if (!object_handle || !se_object_3d_handle_exists(ctx, *object_handle)) {
				continue;
			}
			se_object_3d_destroy(*object_handle);
		}
	}

	if (loaded_models) {
		for (sz i = 0; i < s_array_get_size(loaded_models); ++i) {
			se_model_handle* model_handle = s_array_get(loaded_models, s_array_handle(loaded_models, (u32)i));
			if (!model_handle || !se_model_handle_exists(ctx, *model_handle)) {
				continue;
			}
			se_model_destroy(*model_handle);
		}
	}

	if (loaded_shaders) {
		for (sz i = 0; i < s_array_get_size(loaded_shaders); ++i) {
			se_shader_handle* shader_handle = s_array_get(loaded_shaders, s_array_handle(loaded_shaders, (u32)i));
			if (!shader_handle || !se_shader_handle_exists(ctx, *shader_handle)) {
				continue;
			}
			if (pre_existing_shaders && se_shader_list_contains(pre_existing_shaders, *shader_handle)) {
				continue;
			}
			se_shader_destroy(*shader_handle);
		}
	}

	if (loaded_shaders) {
		s_array_clear(loaded_shaders);
	}
	if (loaded_models) {
		s_array_clear(loaded_models);
	}
	if (loaded_objects) {
		s_array_clear(loaded_objects);
	}
}

static void se_instance_ids_clear_keep_capacity(se_instance_ids* ids) {
	if (!ids) {
		return;
	}
	while (s_array_get_size(ids) > 0) {
		s_array_remove(ids, s_array_handle(ids, (u32)(s_array_get_size(ids) - 1)));
	}
}

static void se_instance_actives_clear_keep_capacity(se_instance_actives* actives) {
	if (!actives) {
		return;
	}
	while (s_array_get_size(actives) > 0) {
		s_array_remove(actives, s_array_handle(actives, (u32)(s_array_get_size(actives) - 1)));
	}
}

static void se_transforms_2d_clear_keep_capacity(se_transforms_2d* transforms) {
	if (!transforms) {
		return;
	}
	while (s_array_get_size(transforms) > 0) {
		s_array_remove(transforms, s_array_handle(transforms, (u32)(s_array_get_size(transforms) - 1)));
	}
}

static void se_transforms_clear_keep_capacity(se_transforms* transforms) {
	if (!transforms) {
		return;
	}
	while (s_array_get_size(transforms) > 0) {
		s_array_remove(transforms, s_array_handle(transforms, (u32)(s_array_get_size(transforms) - 1)));
	}
}

static void se_buffers_clear_keep_capacity(se_buffers* buffers) {
	if (!buffers) {
		return;
	}
	while (s_array_get_size(buffers) > 0) {
		s_array_remove(buffers, s_array_handle(buffers, (u32)(s_array_get_size(buffers) - 1)));
	}
}

static void se_scene_3d_invoke_custom_renders(const se_scene_3d_handle scene, se_scene_3d* scene_ptr) {
	if (!scene_ptr || s_array_get_size(&scene_ptr->custom_renders) == 0) {
		return;
	}

	typedef s_array(se_scene_3d_custom_render_handle, se_scene_3d_custom_render_handles);
	se_scene_3d_custom_render_handles callback_handles = {0};
	s_array_init(&callback_handles);
	s_array_reserve(&callback_handles, s_array_get_size(&scene_ptr->custom_renders));
	for (sz i = 0; i < s_array_get_size(&scene_ptr->custom_renders); ++i) {
		const se_scene_3d_custom_render_handle callback_handle = s_array_handle(&scene_ptr->custom_renders, (u32)i);
		s_array_add(&callback_handles, callback_handle);
	}

	for (sz i = 0; i < s_array_get_size(&callback_handles); ++i) {
		se_scene_3d_custom_render_handle* callback_handle = s_array_get(&callback_handles, s_array_handle(&callback_handles, (u32)i));
		if (!callback_handle) {
			continue;
		}
		se_scene_3d_custom_render_entry* entry = s_array_get(&scene_ptr->custom_renders, *callback_handle);
		if (!entry || !entry->callback) {
			continue;
		}
		entry->callback(scene, entry->user_data);
	}
	s_array_clear(&callback_handles);
}

static sz se_instances_active_count(const se_instance_actives* actives) {
	if (!actives) {
		return 0;
	}
	sz count = 0;
	for (sz i = 0; i < s_array_get_size(actives); ++i) {
		b8* active = s_array_get((se_instance_actives*)actives, s_array_handle((se_instance_actives*)actives, (u32)i));
		if (active && *active) {
			count++;
		}
	}
	return count;
}

static b8 se_mat4_near_equal(const s_mat4* a, const s_mat4* b, const f32 epsilon) {
	if (!a || !b) {
		return false;
	}
	for (u32 row = 0; row < 4; ++row) {
		for (u32 col = 0; col < 4; ++col) {
			if (fabsf(a->m[row][col] - b->m[row][col]) > epsilon) {
				return false;
			}
		}
	}
	return true;
}

static void se_instances_push_free_index(se_instance_ids* free_indices, const sz index) {
	if (!free_indices) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(free_indices); ++i) {
		se_instance_id* existing = s_array_get(free_indices, s_array_handle(free_indices, (u32)i));
		if (existing && *existing == (se_instance_id)index) {
			return;
		}
	}
	se_instance_id value = (se_instance_id)index;
	s_array_add(free_indices, value);
}

static b8 se_instances_pop_free_index(se_instance_ids* free_indices, sz* out_index) {
	if (!free_indices || !out_index || s_array_get_size(free_indices) == 0) {
		return false;
	}
	s_handle handle = s_array_handle(free_indices, (u32)(s_array_get_size(free_indices) - 1));
	se_instance_id* idx = s_array_get(free_indices, handle);
	if (!idx) {
		s_array_remove(free_indices, handle);
		return false;
	}
	*out_index = (sz)s_max(*idx, 0);
	s_array_remove(free_indices, handle);
	return true;
}

static void se_instances_remove_free_index(se_instance_ids* free_indices, const sz index) {
	if (!free_indices) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(free_indices); ++i) {
		se_instance_id* value = s_array_get(free_indices, s_array_handle(free_indices, (u32)i));
		if (value && *value == (se_instance_id)index) {
			s_array_remove(free_indices, s_array_handle(free_indices, (u32)i));
			return;
		}
	}
}

static void se_object_2d_sync_render_instances(se_object_2d* object_ptr) {
	if (!object_ptr || object_ptr->is_custom) {
		return;
	}
	se_transforms_2d_clear_keep_capacity(&object_ptr->render_transforms);
	se_buffers_clear_keep_capacity(&object_ptr->render_buffers);
	const sz logical_count = s_array_get_size(&object_ptr->instances.transforms);
	for (sz i = 0; i < logical_count; ++i) {
		b8* active = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)i));
		if (!active || !*active) {
			continue;
		}
		s_mat3* transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)i));
		s_mat4* buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)i));
		if (!transform || !buffer) {
			continue;
		}
		s_mat3 render_transform = s_mat3_mul(&object_ptr->transform, transform);
		s_array_add(&object_ptr->render_transforms, render_transform);
		s_array_add(&object_ptr->render_buffers, *buffer);
	}
	object_ptr->quad.instance_buffers_dirty = true;
}

static void se_object_3d_sync_render_instances(se_object_3d* object_ptr) {
	if (!object_ptr || object_ptr->is_custom) {
		return;
	}
	se_transforms_clear_keep_capacity(&object_ptr->render_transforms);
	const sz logical_count = s_array_get_size(&object_ptr->instances.transforms);
	for (sz i = 0; i < logical_count; ++i) {
		b8* active = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)i));
		if (!active || !*active) {
			continue;
		}
		s_mat4 transform = s_mat4_identity;
		s_array_add(&object_ptr->render_transforms, transform);
	}
}

se_object_2d_handle se_object_2d_create(const c8 *fragment_shader_path, const s_mat3 *transform, const sz max_instances_count) {
	se_context *ctx = se_current_context();
	if (!ctx || !fragment_shader_path || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->objects_2d) == 0) {
		s_array_init(&ctx->objects_2d);
	}
	if (s_array_get_size(&ctx->objects_2d) >= SE_MAX_2D_OBJECTS) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_object_2d_handle object_handle = s_array_increment(&ctx->objects_2d);
	se_object_2d *new_object = s_array_get(&ctx->objects_2d, object_handle);
	memset(new_object, 0, sizeof(*new_object));
	const sz instance_capacity = (max_instances_count > 0) ? max_instances_count : 1;
	se_quad_2d_create(&new_object->quad, (u32)(instance_capacity * 2));
	new_object->transform = *transform;
	new_object->is_custom = false;
	new_object->is_visible = true;
	new_object->shader = S_HANDLE_NULL;
	for (sz i = 0; i < s_array_get_size(&ctx->shaders); ++i) {
		se_shader_handle shader_handle = s_array_handle(&ctx->shaders, (u32)i);
		se_shader *curr_shader = s_array_get(&ctx->shaders, shader_handle);
		if (curr_shader && strcmp(curr_shader->fragment_path, fragment_shader_path) == 0) {
			new_object->shader = shader_handle;
			break;
		}
	}
	if (new_object->shader == S_HANDLE_NULL) {
		se_shader_handle shader = se_shader_load(
			SE_OBJECT_2D_VERTEX_SHADER_PATH,
			fragment_shader_path);
		if (shader == S_HANDLE_NULL) {
			se_quad_destroy(&new_object->quad);
			memset(new_object, 0, sizeof(*new_object));
			s_array_remove(&ctx->objects_2d, object_handle);
			return S_HANDLE_NULL;
		}
		new_object->shader = shader;
	}

	s_array_init(&new_object->instances.ids);
	s_array_init(&new_object->instances.transforms);
	s_array_init(&new_object->instances.buffers);
	s_array_init(&new_object->instances.actives);
	s_array_init(&new_object->instances.free_indices);
	s_array_init(&new_object->instances.metadata);
	s_array_init(&new_object->render_transforms);
	s_array_init(&new_object->render_buffers);
	s_array_reserve(&new_object->instances.ids, instance_capacity);
	s_array_reserve(&new_object->instances.transforms, instance_capacity);
	s_array_reserve(&new_object->instances.buffers, instance_capacity);
	s_array_reserve(&new_object->instances.actives, instance_capacity);
	s_array_reserve(&new_object->instances.metadata, instance_capacity);
	s_array_reserve(&new_object->instances.free_indices, instance_capacity);
	s_array_reserve(&new_object->render_transforms, instance_capacity);
	s_array_reserve(&new_object->render_buffers, instance_capacity);
	new_object->instances.next_id = 0;
	se_quad_2d_add_instance_buffer_mat3(&new_object->quad,
							 s_array_get_data(&new_object->render_transforms),
							 instance_capacity);
	se_quad_2d_add_instance_buffer(&new_object->quad,
							 s_array_get_data(&new_object->render_buffers),
							 instance_capacity);
	if (max_instances_count == 0) {
		s_handle id_handle = s_array_increment(&new_object->instances.ids);
		s_handle transform_handle = s_array_increment(&new_object->instances.transforms);
		s_handle buffer_handle = s_array_increment(&new_object->instances.buffers);
		s_handle active_handle = s_array_increment(&new_object->instances.actives);
		s_handle metadata_handle = s_array_increment(&new_object->instances.metadata);
		se_instance_id *new_instance_id = s_array_get(&new_object->instances.ids, id_handle);
		s_mat3 *new_transform = s_array_get(&new_object->instances.transforms, transform_handle);
		s_mat4 *new_buffer = s_array_get(&new_object->instances.buffers, buffer_handle);
		b8 *new_active = s_array_get(&new_object->instances.actives, active_handle);
		s_mat4 *new_metadata = s_array_get(&new_object->instances.metadata, metadata_handle);
		*new_instance_id = 0;
		*new_transform = s_mat3_identity;
		*new_buffer = s_mat4_identity;
		*new_active = true;
		*new_metadata = s_mat4_identity;
		new_object->instances.next_id = 1;
		se_object_2d_sync_render_instances(new_object);
		se_object_2d_set_instances_dirty(object_handle, true);
	}
	se_set_last_error(SE_RESULT_OK);
	return object_handle;
}

se_object_2d_handle se_object_2d_create_custom(se_object_custom *custom, const s_mat3 *transform) {
	se_context *ctx = se_current_context();
	if (!ctx || !custom || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	s_assertf(custom->data_size <= SE_OBJECT_CUSTOM_DATA_SIZE, "se_object_2d_create_custom :: data_size exceeds SE_OBJECT_CUSTOM_DATA_SIZE");
	if (s_array_get_capacity(&ctx->objects_2d) == 0) {
		s_array_init(&ctx->objects_2d);
	}
	if (s_array_get_size(&ctx->objects_2d) >= SE_MAX_2D_OBJECTS) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_object_2d_handle object_handle = s_array_increment(&ctx->objects_2d);
	se_object_2d *new_object = s_array_get(&ctx->objects_2d, object_handle);
	memset(new_object, 0, sizeof(*new_object));
	new_object->transform = *transform;
	new_object->is_custom = true;
	new_object->is_visible = true;
	memcpy(&new_object->custom, custom, sizeof(se_object_custom));
	se_set_last_error(SE_RESULT_OK);
	return object_handle;
}

void se_object_custom_set_data(se_object_custom *custom, const void *data, const sz size) {
	s_assertf(custom, "se_object_custom_set_data :: custom is null");
	s_assertf(data, "se_object_custom_set_data :: data is null");
	s_assertf(size <= SE_OBJECT_CUSTOM_DATA_SIZE, "se_object_custom_set_data :: size exceeds SE_OBJECT_CUSTOM_DATA_SIZE");
	memcpy(custom->data, data, size);
	custom->data_size = size;
}

void se_object_2d_destroy(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_object_2d_destroy :: ctx is null");
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_destroy :: object is null");
	if (object_ptr->is_custom) {
		object_ptr->custom.render = NULL;
		object_ptr->custom.data_size = 0;
		object_ptr->is_visible = false;
		s_array_remove(&ctx->objects_2d, object);
		return;
	}

	se_quad_destroy(&object_ptr->quad);
	object_ptr->quad.vao = 0;
	object_ptr->quad.vbo = 0;
	object_ptr->quad.ebo = 0;
	object_ptr->shader = S_HANDLE_NULL;

	s_array_clear(&object_ptr->render_transforms);
	s_array_clear(&object_ptr->render_buffers);
	s_array_clear(&object_ptr->instances.ids);
	s_array_clear(&object_ptr->instances.transforms);
	s_array_clear(&object_ptr->instances.buffers);
	s_array_clear(&object_ptr->instances.actives);
	s_array_clear(&object_ptr->instances.free_indices);
	s_array_clear(&object_ptr->instances.metadata);
	object_ptr->is_visible = false;

	s_array_remove(&ctx->objects_2d, object);
}

void se_object_2d_set_transform(const se_object_2d_handle object, const s_mat3 *transform) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_transform :: object is null");
	s_assertf(transform, "se_object_2d_set_transform :: transform is null");
	object_ptr->transform = *transform;
	se_object_2d_set_instances_dirty(object, true);
}

s_mat3 se_object_2d_get_transform(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_transform :: object is null");
	return object_ptr->transform;
}

void se_object_2d_set_position(const se_object_2d_handle object, const s_vec2 *position) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_position :: object is null");
	s_assertf(position, "se_object_2d_set_position :: position is null");
	s_mat3_set_translation(&object_ptr->transform, position);
	se_object_2d_set_instances_dirty(object, true);
}

s_vec2 se_object_2d_get_position(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_position :: object is null");
	s_vec2 out_pos = s_mat3_get_translation(&object_ptr->transform);
	return out_pos;
}

void se_object_2d_set_scale(const se_object_2d_handle object, const s_vec2 *scale) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_scale :: object is null");
	s_assertf(scale, "se_object_2d_set_scale :: scale is null");
	s_mat3_set_scale(&object_ptr->transform, scale);
	se_object_2d_set_instances_dirty(object, true);
}

s_vec2 se_object_2d_get_scale(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_scale :: object is null");
	s_vec2 out_scale = s_mat3_get_scale(&object_ptr->transform);
	return out_scale;
}

void se_object_2d_get_box_2d(const se_object_2d_handle object, se_box_2d *out_box) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_box_2d :: object is null");
	s_assertf(out_box, "se_object_2d_get_box_2d :: out_box is null");
	se_box_2d_make(out_box, &object_ptr->transform);
}

void se_object_2d_set_shader(const se_object_2d_handle object, const se_shader_handle shader) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	object_ptr->shader = shader;
}

se_shader_handle se_object_2d_get_shader(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_shader :: object is null");
	if (object_ptr->is_custom) {
		return S_HANDLE_NULL;
	}
	return object_ptr->shader;
}

void se_object_2d_update_uniforms(const se_object_2d_handle object) {
	(void)object;
}

se_instance_id se_object_2d_add_instance(const se_object_2d_handle object, const s_mat3 *transform, const s_mat4 *buffer) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instance_add :: object is null");
	s_assertf(transform, "se_object_2d_set_instance_add :: transform is null");
	s_assertf(buffer, "se_object_2d_set_instance_add :: buffer is null");
	const sz max_capacity = s_array_get_capacity(&object_ptr->instances.ids);
	sz reuse_index = 0;
	const b8 has_free_slot = se_instances_pop_free_index(&object_ptr->instances.free_indices, &reuse_index);
	if (!has_free_slot && max_capacity > 0 && s_array_get_size(&object_ptr->instances.ids) >= max_capacity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return -1;
	}
	const se_instance_id new_id = object_ptr->instances.next_id++;
	if (has_free_slot && reuse_index < s_array_get_size(&object_ptr->instances.ids)) {
		se_instance_id* id = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)reuse_index));
		s_mat3* dst_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)reuse_index));
		s_mat4* dst_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)reuse_index));
		b8* dst_active = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)reuse_index));
		s_mat4* dst_metadata = s_array_get(&object_ptr->instances.metadata, s_array_handle(&object_ptr->instances.metadata, (u32)reuse_index));
		if (id && dst_transform && dst_buffer && dst_active && dst_metadata) {
			*id = new_id;
			*dst_transform = *transform;
			*dst_buffer = *buffer;
			*dst_active = true;
			*dst_metadata = s_mat4_identity;
		}
	} else {
		s_mat4 default_metadata = s_mat4_identity;
		b8 active = true;
		s_array_add(&object_ptr->instances.ids, new_id);
		s_array_add(&object_ptr->instances.transforms, *transform);
		s_array_add(&object_ptr->instances.buffers, *buffer);
		s_array_add(&object_ptr->instances.actives, active);
		s_array_add(&object_ptr->instances.metadata, default_metadata);
	}
	se_object_2d_sync_render_instances(object_ptr);
	se_object_2d_set_instances_dirty(object, true);

	return new_id;
}

b8 se_object_2d_remove_instance(const se_object_2d_handle object, const se_instance_id instance_id) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_remove_instance :: object is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index < 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	b8* active = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)index));
	if (active && *active) {
		*active = false;
		se_instances_push_free_index(&object_ptr->instances.free_indices, (sz)index);
		se_object_2d_set_instances_dirty(object, true);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

i32 se_object_2d_get_instance_index(const se_object_2d_handle object, const se_instance_id instance_id) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_instance_index :: object is null");
	const sz count = s_array_get_size(&object_ptr->instances.ids);
	if (instance_id >= 0 && (sz)instance_id < count) {
		se_instance_id *quick = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)instance_id));
		if (quick && *quick == instance_id) {
			return instance_id;
		}
	}
	for (sz i = 0; i < s_array_get_size(&object_ptr->instances.ids); ++i) {
		se_instance_id *found_instance = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)i));
		if (*found_instance == instance_id) {
			return (i32)i;
		}
	}
	return -1;
}

void se_object_2d_set_transform_by_id(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat3 *transform) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_transform_by_id :: object is null");
	s_assertf(transform, "se_object_2d_set_transform_by_id :: transform is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat3 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)index));
		*current_transform = *transform;
		se_object_2d_set_instances_dirty(object, true);
		se_set_last_error(SE_RESULT_OK);
	} else {
		se_set_last_error(SE_RESULT_NOT_FOUND);
	}
}

void se_object_2d_set_buffer_by_id(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat4 *buffer) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_buffer_by_id :: object is null");
	s_assertf(buffer, "se_object_2d_set_buffer_by_id :: buffer is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)index));
		*current_buffer = *buffer;
		se_object_2d_set_instances_dirty(object, true);
		se_set_last_error(SE_RESULT_OK);
	} else {
		se_set_last_error(SE_RESULT_NOT_FOUND);
	}
}

void se_object_2d_set_instances_transforms(const se_object_2d_handle object, const se_transforms_2d *transforms) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instances_transforms :: object is null");
	s_assertf(transforms, "se_object_2d_set_instances_transforms :: transforms is null");

	const sz count = s_array_get_size(transforms);
	const sz max_capacity = s_array_get_capacity(&object_ptr->instances.ids);
	if (max_capacity > 0 && count > max_capacity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return;
	}
	se_instance_ids_clear_keep_capacity(&object_ptr->instances.ids);
	se_transforms_2d_clear_keep_capacity(&object_ptr->instances.transforms);
	se_buffers_clear_keep_capacity(&object_ptr->instances.buffers);
	se_instance_actives_clear_keep_capacity(&object_ptr->instances.actives);
	se_instance_ids_clear_keep_capacity(&object_ptr->instances.free_indices);
	se_buffers_clear_keep_capacity(&object_ptr->instances.metadata);
	for (sz i = 0; i < count; ++i) {
		const s_mat3 *src_transform = s_array_get((se_transforms_2d *)transforms, s_array_handle((se_transforms_2d *)transforms, (u32)i));
		if (!src_transform) {
			continue;
		}
		se_instance_id id = (se_instance_id)i;
		s_mat4 default_buffer = s_mat4_identity;
		s_mat4 default_metadata = s_mat4_identity;
		b8 active = true;
		s_array_add(&object_ptr->instances.ids, id);
		s_array_add(&object_ptr->instances.transforms, *src_transform);
		s_array_add(&object_ptr->instances.buffers, default_buffer);
		s_array_add(&object_ptr->instances.actives, active);
		s_array_add(&object_ptr->instances.metadata, default_metadata);
	}
	object_ptr->instances.next_id = (se_instance_id)count;
	se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_instances_buffers(const se_object_2d_handle object, const se_buffers *buffers) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instances_buffers :: object is null");
	s_assertf(buffers, "se_object_2d_set_instances_buffers :: buffers is null");

	const sz count = s_array_get_size(buffers);
	const sz logical_count = s_array_get_size(&object_ptr->instances.buffers);
	const sz write_count = s_min(count, logical_count);
	for (sz i = 0; i < write_count; ++i) {
		const s_mat4 *src_buffer = s_array_get((se_buffers *)buffers, s_array_handle((se_buffers *)buffers, (u32)i));
		s_mat4 *dst_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)i));
		if (src_buffer && dst_buffer) {
			*dst_buffer = *src_buffer;
		}
	}
	se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_transforms_by_id(const se_object_2d_handle object, const se_instance_id* instance_ids, const s_mat3* transforms, const sz count) {
	if (!instance_ids || !transforms) {
		return;
	}
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_transforms_by_id :: object is null");
	for (sz i = 0; i < count; ++i) {
		const i32 index = se_object_2d_get_instance_index(object, instance_ids[i]);
		if (index < 0) {
			continue;
		}
		s_mat3 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)index));
		if (current_transform) {
			*current_transform = transforms[i];
		}
	}
	se_object_2d_set_instances_dirty(object, true);
}

void se_object_2d_set_buffers_by_id(const se_object_2d_handle object, const se_instance_id* instance_ids, const s_mat4* buffers, const sz count) {
	if (!instance_ids || !buffers) {
		return;
	}
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_buffers_by_id :: object is null");
	for (sz i = 0; i < count; ++i) {
		const i32 index = se_object_2d_get_instance_index(object, instance_ids[i]);
		if (index < 0) {
			continue;
		}
		s_mat4 *current_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)index));
		if (current_buffer) {
			*current_buffer = buffers[i];
		}
	}
	se_object_2d_set_instances_dirty(object, true);
}

b8 se_object_2d_set_active_by_id(const se_object_2d_handle object, const se_instance_id instance_id, const b8 active) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_active_by_id :: object is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index < 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	b8 *entry = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)index));
	if (!entry) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*entry = active;
	if (active) {
		se_instances_remove_free_index(&object_ptr->instances.free_indices, (sz)index);
	} else {
		se_instances_push_free_index(&object_ptr->instances.free_indices, (sz)index);
	}
	se_object_2d_set_instances_dirty(object, true);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_object_2d_is_active_by_id(const se_object_2d_handle object, const se_instance_id instance_id) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_is_active_by_id :: object is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index < 0) {
		return false;
	}
	b8 *entry = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)index));
	return entry ? *entry : false;
}

sz se_object_2d_get_inactive_slot_count(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_inactive_slot_count :: object is null");
	return s_array_get_size(&object_ptr->instances.free_indices);
}

b8 se_object_2d_set_metadata_by_id(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat4* metadata) {
	if (!metadata) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_metadata_by_id :: object is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index < 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	s_mat4* entry = s_array_get(&object_ptr->instances.metadata, s_array_handle(&object_ptr->instances.metadata, (u32)index));
	if (!entry) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*entry = *metadata;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_object_2d_get_metadata_by_id(const se_object_2d_handle object, const se_instance_id instance_id, s_mat4* out_metadata) {
	if (!out_metadata) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_metadata_by_id :: object is null");
	const i32 index = se_object_2d_get_instance_index(object, instance_id);
	if (index < 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	s_mat4* entry = s_array_get(&object_ptr->instances.metadata, s_array_handle(&object_ptr->instances.metadata, (u32)index));
	if (!entry) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_metadata = *entry;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_object_2d_set_instances_dirty(const se_object_2d_handle object, const b8 dirty) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_set_instances_dirty :: object is null");
	object_ptr->quad.instance_buffers_dirty = dirty;
}

b8 se_object_2d_are_instances_dirty(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_are_instances_dirty :: object is null");
	return object_ptr->quad.instance_buffers_dirty;
}

sz se_object_2d_get_instance_count(const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_object_2d *object_ptr = se_object_2d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_2d_get_instance_count :: object is null");
	return se_instances_active_count(&object_ptr->instances.actives);
}

s_json* se_object_2d_to_json(const se_object_2d_handle object) {
	se_context* ctx = se_current_context();
	if (!ctx || object == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_object_2d* object_ptr = se_object_2d_from_handle(ctx, object);
	if (!object_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	if (object_ptr->is_custom) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return NULL;
	}
	if (!se_shader_handle_exists(ctx, object_ptr->shader)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	se_shader* shader = s_array_get(&ctx->shaders, object_ptr->shader);
	if (!shader || shader->fragment_path[0] == '\0') {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return NULL;
	}

	const sz slot_count = s_array_get_size(&object_ptr->instances.ids);
	if (slot_count != s_array_get_size(&object_ptr->instances.transforms) ||
		slot_count != s_array_get_size(&object_ptr->instances.buffers) ||
		slot_count != s_array_get_size(&object_ptr->instances.actives) ||
		slot_count != s_array_get_size(&object_ptr->instances.metadata)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	s_json* root = s_json_object_empty(NULL);
	if (!root) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	c8 fragment_path_json[SE_MAX_PATH_LENGTH] = {0};
	se_scene_json_serialize_path(shader->fragment_path, fragment_path_json, sizeof(fragment_path_json));
	const c8* fragment_path = fragment_path_json[0] != '\0' ? fragment_path_json : shader->fragment_path;
	if (!se_scene_2d_json_add_string(root, "format", SE_OBJECT_2D_JSON_FORMAT) ||
		!se_scene_2d_json_add_u32(root, "version", SE_OBJECT_2D_JSON_VERSION) ||
		!se_scene_2d_json_add_string(root, "fragment_shader_path", fragment_path) ||
		!se_scene_json_add_mat3(root, "transform", &object_ptr->transform) ||
		!se_scene_2d_json_add_bool(root, "visible", object_ptr->is_visible) ||
		!se_scene_2d_json_add_u32(root, "instance_capacity", (u32)s_array_get_capacity(&object_ptr->instances.ids)) ||
		!se_scene_2d_json_add_i32(root, "next_instance_id", object_ptr->instances.next_id)) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	s_json* instances_json = se_scene_2d_json_add_array(root, "instances");
	if (!instances_json) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	for (sz slot_index = 0; slot_index < slot_count; ++slot_index) {
		se_instance_id* id = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)slot_index));
		s_mat3* transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)slot_index));
		s_mat4* buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)slot_index));
		b8* active = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)slot_index));
		s_mat4* metadata = s_array_get(&object_ptr->instances.metadata, s_array_handle(&object_ptr->instances.metadata, (u32)slot_index));
		if (!id || !transform || !buffer || !active || !metadata) {
			s_json_free(root);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return NULL;
		}

		s_json* slot_json = se_scene_2d_json_add_object(instances_json, NULL);
		if (!slot_json ||
			!se_scene_2d_json_add_i32(slot_json, "id", *id) ||
			!se_scene_2d_json_add_bool(slot_json, "active", *active) ||
			!se_scene_json_add_mat3(slot_json, "transform", transform) ||
			!se_scene_json_add_mat4(slot_json, "buffer", buffer) ||
			!se_scene_json_add_mat4(slot_json, "metadata", metadata)) {
			s_json_free(root);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}
	}

	se_set_last_error(SE_RESULT_OK);
	return root;
}

b8 se_object_2d_to_json_file(const se_object_2d_handle object, const c8* path) {
	if (!path || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_json* root = se_object_2d_to_json(object);
	if (!root) {
		return false;
	}

	const b8 ok = se_scene_json_write_file(path, root);
	s_json_free(root);
	return ok;
}

b8 se_object_2d_from_json(const se_object_2d_handle object, const s_json* root) {
	se_context* ctx = se_current_context();
	if (!ctx || object == S_HANDLE_NULL || !root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_object_2d* object_ptr = se_object_2d_from_handle(ctx, object);
	if (!object_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (object_ptr->is_custom) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}
	if (root->type != S_JSON_OBJECT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const c8* format = NULL;
	u32 version = 0u;
	if (!se_scene_json_string_to_c8(se_scene_json_get_required(root, "format", S_JSON_STRING), &format) ||
		!se_scene_json_number_to_u32(se_scene_json_get_required(root, "version", S_JSON_NUMBER), &version)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (strcmp(format, SE_OBJECT_2D_JSON_FORMAT) != 0 || version != SE_OBJECT_2D_JSON_VERSION) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}

	const s_json* transform_json = se_scene_json_get_required(root, "transform", S_JSON_ARRAY);
	const s_json* instances_json = se_scene_json_get_required(root, "instances", S_JSON_ARRAY);
	const c8* fragment_shader_path = NULL;
	b8 visible = true;
	u32 instance_capacity = 0u;
	i32 next_instance_id = 0;
	s_mat3 object_transform = s_mat3_identity;
	if (!se_scene_json_string_to_c8(se_scene_json_get_required(root, "fragment_shader_path", S_JSON_STRING), &fragment_shader_path) ||
		!fragment_shader_path || fragment_shader_path[0] == '\0' ||
		!se_scene_json_bool_to_b8(se_scene_json_get_required(root, "visible", S_JSON_BOOL), &visible) ||
		!se_scene_json_number_to_u32(se_scene_json_get_required(root, "instance_capacity", S_JSON_NUMBER), &instance_capacity) ||
		!se_scene_json_number_to_i32(se_scene_json_get_required(root, "next_instance_id", S_JSON_NUMBER), &next_instance_id) ||
		!transform_json || !instances_json ||
		!se_scene_json_read_mat3(transform_json, &object_transform) ||
		next_instance_id < 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const sz slot_count = instances_json->as.children.count;
	const sz max_capacity = s_array_get_capacity(&object_ptr->instances.ids);
	const sz required_capacity = s_max((sz)s_max(instance_capacity, 1u), slot_count);
	if (max_capacity > 0 && required_capacity > max_capacity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	i32 max_instance_id = -1;
	for (sz slot_index = 0; slot_index < slot_count; ++slot_index) {
		const s_json* slot_json = s_json_at(instances_json, slot_index);
		i32 instance_id = 0;
		b8 active = true;
		s_mat3 instance_transform = s_mat3_identity;
		s_mat4 instance_buffer = s_mat4_identity;
		s_mat4 instance_metadata = s_mat4_identity;
		if (!se_object_2d_json_read_slot(
			slot_json,
			&instance_id,
			&active,
			&instance_transform,
			&instance_buffer,
			&instance_metadata)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		max_instance_id = s_max(max_instance_id, instance_id);
	}

	if (slot_count > 0 && next_instance_id <= max_instance_id) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_shader_handle shader_handle = S_HANDLE_NULL;
	for (sz i = 0; i < s_array_get_size(&ctx->shaders); ++i) {
		se_shader_handle shader = s_array_handle(&ctx->shaders, (u32)i);
		se_shader* current_shader = s_array_get(&ctx->shaders, shader);
		if (current_shader && strcmp(current_shader->fragment_path, fragment_shader_path) == 0) {
			shader_handle = shader;
			break;
		}
	}
	if (shader_handle == S_HANDLE_NULL) {
		shader_handle = se_shader_load(SE_OBJECT_2D_VERTEX_SHADER_PATH, fragment_shader_path);
		if (shader_handle == S_HANDLE_NULL) {
			return false;
		}
	}

	object_ptr->transform = object_transform;
	object_ptr->is_visible = visible;
	object_ptr->shader = shader_handle;
	se_instance_ids_clear_keep_capacity(&object_ptr->instances.ids);
	se_transforms_2d_clear_keep_capacity(&object_ptr->instances.transforms);
	se_buffers_clear_keep_capacity(&object_ptr->instances.buffers);
	se_instance_actives_clear_keep_capacity(&object_ptr->instances.actives);
	se_instance_ids_clear_keep_capacity(&object_ptr->instances.free_indices);
	se_buffers_clear_keep_capacity(&object_ptr->instances.metadata);
	se_transforms_2d_clear_keep_capacity(&object_ptr->render_transforms);
	se_buffers_clear_keep_capacity(&object_ptr->render_buffers);

	for (sz slot_index = 0; slot_index < slot_count; ++slot_index) {
		const s_json* slot_json = s_json_at(instances_json, slot_index);
		i32 instance_id = 0;
		b8 active = true;
		s_mat3 instance_transform = s_mat3_identity;
		s_mat4 instance_buffer = s_mat4_identity;
		s_mat4 instance_metadata = s_mat4_identity;
		if (!se_object_2d_json_read_slot(
			slot_json,
			&instance_id,
			&active,
			&instance_transform,
			&instance_buffer,
			&instance_metadata)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}

		se_instance_id instance_id_value = (se_instance_id)instance_id;
		s_array_add(&object_ptr->instances.ids, instance_id_value);
		s_array_add(&object_ptr->instances.transforms, instance_transform);
		s_array_add(&object_ptr->instances.buffers, instance_buffer);
		s_array_add(&object_ptr->instances.actives, active);
		s_array_add(&object_ptr->instances.metadata, instance_metadata);

		if (!active) {
			se_instances_push_free_index(&object_ptr->instances.free_indices, slot_index);
		}
	}

	object_ptr->instances.next_id = (se_instance_id)next_instance_id;
	se_object_2d_set_instances_dirty(object, true);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_object_2d_from_json_file(const se_object_2d_handle object, const c8* path) {
	s_json* root = NULL;
	if (!se_scene_json_read_file(path, &root)) {
		return false;
	}
	const b8 ok = se_object_2d_from_json(object, root);
	s_json_free(root);
	return ok;
}

se_scene_2d_handle se_scene_2d_create(const s_vec2 *size, const u16 object_count) {
	se_context *ctx = se_current_context();
	if (!ctx || !size || object_count == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->scenes_2d) == 0) {
		s_array_init(&ctx->scenes_2d);
	}
	if (s_array_get_size(&ctx->scenes_2d) >= SE_MAX_SCENES) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_scene_2d_handle scene_handle = s_array_increment(&ctx->scenes_2d);
	se_scene_2d *new_scene = s_array_get(&ctx->scenes_2d, scene_handle);
	memset(new_scene, 0, sizeof(*new_scene));
	se_framebuffer_handle framebuffer = se_framebuffer_create(size);
	if (framebuffer == S_HANDLE_NULL) {
		s_array_remove(&ctx->scenes_2d, scene_handle);
		return S_HANDLE_NULL;
	}
	new_scene->output = framebuffer;
	s_array_init(&new_scene->objects);
	s_array_reserve(&new_scene->objects, object_count);
	se_set_last_error(SE_RESULT_OK);
	return scene_handle;
}

static void se_scene_2d_resize_callback(const se_window_handle window, void *scene_data) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_scene_2d_resize_callback :: ctx is null");
	s_assertf(scene_data, "se_scene_2d_resize_callback :: scene_data is null");

	se_scene_2d_handle scene = (se_scene_2d_handle)(uintptr_t)scene_data;
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	se_scene_2d *scene_ptr = s_array_get(&ctx->scenes_2d, scene);
	s_assertf(window_ptr, "se_scene_2d_resize_callback :: window_ptr is null");
	s_assertf(scene_ptr, "se_scene_2d_resize_callback :: scene_ptr is null");

	se_framebuffer_handle framebuffer = scene_ptr->output;
	s_assertf(framebuffer != S_HANDLE_NULL, "se_scene_2d_resize_callback :: framebuffer is null");
	s_vec2 old_size = {0};
	se_framebuffer_get_size(framebuffer, &old_size);
	s_vec2 new_size = {0};
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, framebuffer);
	new_size = s_vec2(framebuffer_ptr->ratio.x * window_ptr->width,
						framebuffer_ptr->ratio.y * window_ptr->height);

	// TODO: update object positions and scales
	// s_foreach(&scene_ptr->objects, i) {
	//	se_object_2d_ptr* current_object_ptr = s_array_get(&scene_ptr->objects,
	//	i); if (current_object_ptr == NULL || *current_object_ptr == NULL) {
	//		continue;
	//	}
	//	se_object_2d* current_object = *current_object_ptr;
	//	s_vec2* current_position = &current_object->position;
	//	s_vec2* current_scale = &current_object->scale;
	//	se_object_2d_set_position(current_object, &s_vec2(current_position->x *
	//	new_size.x / old_size.x, current_position->y * new_size.y /
	//	old_size.y)); se_object_2d_set_scale(current_object,
	//	&s_vec2(current_scale->x * new_size.x / old_size.x, current_scale->y *
	//	new_size.y / old_size.y));
	//}

	se_framebuffer_set_size(framebuffer, &new_size);
	se_scene_2d_render_to_buffer(scene);
}

void se_scene_2d_set_fit_to_window(const se_scene_2d_handle scene, const se_window_handle window, const s_vec2 *ratio) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_set_fit_to_window :: scene is null");
	s_assertf(ratio, "se_scene_2d_set_fit_to_window :: ratio is null");

	s_assertf(ratio->x == 1.0 && ratio->y == 1.0,
				"se_scene_2d_set_fit_to_window :: ratio can only be 1 for now, "
				"please wait for the next update");
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, scene_ptr->output);
	framebuffer_ptr->auto_resize = true;
	framebuffer_ptr->ratio = *ratio;
	se_window_register_resize_event(window, se_scene_2d_resize_callback, (void *)(uintptr_t)scene);
}

void se_scene_2d_destroy(const se_scene_2d_handle scene) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_scene_2d_destroy :: ctx is null");
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_destroy :: scene is null");
	se_framebuffer_destroy(scene_ptr->output);
	scene_ptr->output = S_HANDLE_NULL;
	s_array_clear(&scene_ptr->objects);
	s_array_remove(&ctx->scenes_2d, scene);
}

void se_scene_2d_destroy_full(const se_scene_2d_handle scene, const b8 destroy_object_shaders) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_scene_2d_destroy_full :: ctx is null");
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_destroy_full :: scene is null");

	se_objects_2d_ptr object_handles = {0};
	se_shaders_ptr shader_handles = {0};
	s_array_init(&object_handles);
	s_array_init(&shader_handles);

	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_2d_handle* handle_ptr = s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (!handle_ptr || *handle_ptr == S_HANDLE_NULL || !se_object_2d_handle_exists(ctx, *handle_ptr)) {
			continue;
		}
		se_object_2d_list_add_unique(&object_handles, *handle_ptr);
		if (!destroy_object_shaders) {
			continue;
		}
		se_object_2d* object_ptr = se_object_2d_from_handle(ctx, *handle_ptr);
		if (!object_ptr || object_ptr->is_custom || !se_shader_handle_exists(ctx, object_ptr->shader)) {
			continue;
		}
		se_shader_list_add_unique(&shader_handles, object_ptr->shader);
	}

	for (sz i = 0; i < s_array_get_size(&object_handles); ++i) {
		se_object_2d_handle* object_handle = s_array_get(&object_handles, s_array_handle(&object_handles, (u32)i));
		if (!object_handle || !se_object_2d_handle_exists(ctx, *object_handle)) {
			continue;
		}
		se_object_2d_destroy(*object_handle);
	}

	if (destroy_object_shaders) {
		for (sz i = 0; i < s_array_get_size(&shader_handles); ++i) {
			se_shader_handle* shader_handle = s_array_get(&shader_handles, s_array_handle(&shader_handles, (u32)i));
			if (!shader_handle || !se_shader_handle_exists(ctx, *shader_handle)) {
				continue;
			}
			se_shader_destroy(*shader_handle);
		}
	}

	s_array_clear(&shader_handles);
	s_array_clear(&object_handles);
	se_scene_2d_destroy(scene);
}

void se_scene_2d_bind(const se_scene_2d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_bind :: scene is null");
	se_framebuffer_bind(scene_ptr->output);
	se_render_set_blending(true);
}

void se_scene_2d_unbind(const se_scene_2d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_unbind :: scene is null");
	se_framebuffer_unbind(scene_ptr->output);
	se_render_set_blending(false);
}

void se_scene_2d_render_raw(const se_scene_2d_handle scene) {
	se_debug_trace_begin("scene2d_render");
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_render_raw :: scene is null");
	s_assertf(ctx, "se_scene_2d_render_raw :: ctx is null");

	se_render_clear();
	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_2d_handle object_handle = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (object_handle == S_HANDLE_NULL) {
			continue;
		}
		se_object_2d *current_object_2d = se_object_2d_from_handle(ctx, object_handle);
		if (!current_object_2d || !current_object_2d->is_visible) {
			continue;
		}
		if (current_object_2d->is_custom) {
			if (current_object_2d->custom.render) {
				current_object_2d->custom.render(current_object_2d->custom.data);
			}
		} else {
			if (se_object_2d_are_instances_dirty(object_handle)) {
				se_object_2d_sync_render_instances(current_object_2d);
			}
			se_uniform_apply(current_object_2d->shader, true);
			const sz instance_count = se_object_2d_get_instance_count(object_handle);
			se_quad_render(&current_object_2d->quad, instance_count);
		}
	}
	se_debug_trace_end("scene2d_render");
}

void se_scene_2d_render_to_buffer(const se_scene_2d_handle scene) {
	se_scene_2d_bind(scene);
	se_scene_2d_render_raw(scene);
	se_scene_2d_unbind(scene);
}

void se_scene_2d_render_to_screen(const se_scene_2d_handle scene, const se_window_handle window) {
	se_context *ctx = se_current_context();
	if (ctx == NULL) {
		return;
	}

	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, scene_ptr->output);
	se_render_unbind_framebuffer();
	se_render_set_blending(true);
	se_shader_set_texture(window_ptr->shader, "u_texture", framebuffer_ptr->texture);
	se_window_render_quad(window);
	se_render_set_blending(false);
}

void se_scene_2d_draw(const se_scene_2d_handle scene, const se_window_handle window) {
	se_scene_2d_render_to_buffer(scene);
	se_render_clear();
	se_scene_2d_render_to_screen(scene, window);
}

void se_scene_2d_add_object(const se_scene_2d_handle scene, const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_add_object :: scene is null");
	s_array_add(&scene_ptr->objects, object);
}

void se_scene_2d_remove_object(const se_scene_2d_handle scene, const se_object_2d_handle object) {
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_remove_object :: scene is null");
	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_2d_handle current = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (current == object) {
			s_array_remove(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
			break;
		}
	}
}

b8 se_scene_2d_pick_object(const se_scene_2d_handle scene, const s_vec2* point_ndc, se_scene_pick_filter_2d filter, void* user_data, se_object_2d_handle* out_object) {
	if (!point_ndc || !out_object) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_context *ctx = se_current_context();
	se_scene_2d *scene_ptr = se_scene_2d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_2d_pick_object :: scene is null");
	*out_object = S_HANDLE_NULL;
	for (sz i = s_array_get_size(&scene_ptr->objects); i > 0; --i) {
		se_object_2d_handle object_handle = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)(i - 1)));
		if (object_handle == S_HANDLE_NULL) {
			continue;
		}
		if (filter && !filter(object_handle, user_data)) {
			continue;
		}
		se_object_2d *object = se_object_2d_from_handle(ctx, object_handle);
		if (!object || !object->is_visible) {
			continue;
		}
		se_box_2d box = {0};
		se_object_2d_get_box_2d(object_handle, &box);
		if (point_ndc->x >= box.min.x && point_ndc->x <= box.max.x &&
			point_ndc->y >= box.min.y && point_ndc->y <= box.max.y) {
			*out_object = object_handle;
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

s_json* se_scene_2d_to_json(const se_scene_2d_handle scene) {
	se_context* ctx = se_current_context();
	if (!ctx || scene == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_scene_2d* scene_ptr = se_scene_2d_from_handle(ctx, scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	if (scene_ptr->output == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}

	s_json* root = s_json_object_empty(NULL);
	if (!root) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	if (!se_scene_2d_json_add_string(root, "format", SE_SCENE_2D_JSON_FORMAT) ||
		!se_scene_2d_json_add_u32(root, "version", SE_SCENE_2D_JSON_VERSION)) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	s_vec2 output_size = s_vec2(0.0f, 0.0f);
	se_framebuffer_get_size(scene_ptr->output, &output_size);
	if (!se_scene_2d_json_add_vec2(root, "output_size", &output_size)) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	s_json* objects = se_scene_2d_json_add_array(root, "objects");
	if (!objects) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_2d_handle* object_handle = s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (!object_handle || !se_object_2d_handle_exists(ctx, *object_handle)) {
			continue;
		}
		s_json* object_json = se_object_2d_to_json(*object_handle);
		if (!object_json) {
			s_json_free(root);
			return NULL;
		}
		if (!se_scene_2d_json_add_child(objects, object_json)) {
			s_json_free(root);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}
	}

	se_set_last_error(SE_RESULT_OK);
	return root;
}

b8 se_scene_2d_from_json(const se_scene_2d_handle scene, const s_json* root) {
	se_context* ctx = se_current_context();
	if (!ctx || scene == S_HANDLE_NULL || !root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_scene_2d* scene_ptr = se_scene_2d_from_handle(ctx, scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (scene_ptr->output == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (root->type != S_JSON_OBJECT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const c8* format = NULL;
	u32 version = 0u;
	if (!se_scene_json_string_to_c8(se_scene_json_get_required(root, "format", S_JSON_STRING), &format) ||
		!se_scene_json_number_to_u32(se_scene_json_get_required(root, "version", S_JSON_NUMBER), &version)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (strcmp(format, SE_SCENE_2D_JSON_FORMAT) != 0 || version != SE_SCENE_2D_JSON_VERSION) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}

	const s_json* output_size_json = se_scene_json_get_required(root, "output_size", S_JSON_ARRAY);
	const s_json* objects_json = se_scene_json_get_required(root, "objects", S_JSON_ARRAY);
	if (!output_size_json || !objects_json) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_vec2 output_size = s_vec2(0.0f, 0.0f);
	if (!se_scene_2d_json_read_vec2_node(output_size_json, &output_size) ||
		output_size.x <= 0.0f ||
		output_size.y <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_objects_2d_ptr loaded_objects = {0};
	se_shaders_ptr loaded_shaders = {0};
	se_shaders_ptr existing_shaders = {0};
	s_array_init(&loaded_objects);
	s_array_init(&loaded_shaders);
	se_scene_2d_collect_existing_shaders(ctx, &existing_shaders);
	s_array_reserve(&loaded_objects, objects_json->as.children.count);
	s_array_reserve(&loaded_shaders, objects_json->as.children.count);

	for (sz object_index = 0; object_index < objects_json->as.children.count; ++object_index) {
		const s_json* object_json = s_json_at(objects_json, object_index);
		if (!object_json || object_json->type != S_JSON_OBJECT) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		const c8* fragment_shader_path = NULL;
		u32 instance_capacity = 0u;
		const s_json* instances_json = se_scene_json_get_required(object_json, "instances", S_JSON_ARRAY);
		if (!se_scene_json_string_to_c8(se_scene_json_get_required(object_json, "fragment_shader_path", S_JSON_STRING), &fragment_shader_path) ||
			!fragment_shader_path || fragment_shader_path[0] == '\0' ||
			!se_scene_json_number_to_u32(se_scene_json_get_required(object_json, "instance_capacity", S_JSON_NUMBER), &instance_capacity) ||
			!instances_json) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		const sz slot_count = instances_json->as.children.count;
		const sz create_capacity = s_max((sz)s_max(instance_capacity, 1u), slot_count);
		const s_mat3 create_transform = s_mat3_identity;
		const se_object_2d_handle object_handle = se_object_2d_create(fragment_shader_path, &create_transform, create_capacity);
		if (object_handle == S_HANDLE_NULL) {
			goto fail;
		}
		s_array_add(&loaded_objects, object_handle);

		se_object_2d* object = se_object_2d_from_handle(ctx, object_handle);
		if (!object) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			goto fail;
		}
		se_shader_list_add_unique(&loaded_shaders, object->shader);
		if (!se_object_2d_from_json(object_handle, object_json)) {
			object = se_object_2d_from_handle(ctx, object_handle);
			if (object) {
				se_shader_list_add_unique(&loaded_shaders, object->shader);
			}
			goto fail;
		}
		object = se_object_2d_from_handle(ctx, object_handle);
		if (!object) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			goto fail;
		}
		se_shader_list_add_unique(&loaded_shaders, object->shader);
	}

	se_framebuffer_set_size(scene_ptr->output, &output_size);
	while (s_array_get_size(&scene_ptr->objects) > 0) {
		s_array_remove(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)(s_array_get_size(&scene_ptr->objects) - 1)));
	}
	s_array_reserve(&scene_ptr->objects, s_array_get_size(&loaded_objects));
	for (sz i = 0; i < s_array_get_size(&loaded_objects); ++i) {
		se_object_2d_handle* object_handle = s_array_get(&loaded_objects, s_array_handle(&loaded_objects, (u32)i));
		if (!object_handle) {
			continue;
		}
		s_array_add(&scene_ptr->objects, *object_handle);
	}

	s_array_clear(&existing_shaders);
	s_array_clear(&loaded_shaders);
	s_array_clear(&loaded_objects);
	se_set_last_error(SE_RESULT_OK);
	return true;

fail:
	se_scene_2d_cleanup_loaded_objects(ctx, &loaded_objects, &existing_shaders, &loaded_shaders);
	s_array_clear(&existing_shaders);
	return false;
}

s_json* se_scene_3d_to_json(const se_scene_3d_handle scene) {
	se_context* ctx = se_current_context();
	if (!ctx || scene == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_scene_3d* scene_ptr = se_scene_3d_from_handle(ctx, scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	if (scene_ptr->output == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}

	s_json* root = s_json_object_empty(NULL);
	if (!root) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	if (!se_scene_2d_json_add_string(root, "format", SE_SCENE_3D_JSON_FORMAT) ||
		!se_scene_2d_json_add_u32(root, "version", SE_SCENE_3D_JSON_VERSION)) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	s_vec2 output_size = s_vec2(0.0f, 0.0f);
	se_framebuffer_get_size(scene_ptr->output, &output_size);
	if (!se_scene_2d_json_add_vec2(root, "output_size", &output_size) ||
		!se_scene_2d_json_add_bool(root, "culling", scene_ptr->enable_culling)) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	se_models_ptr model_handles = {0};
	s_array_init(&model_handles);
	s_array_reserve(&model_handles, s_array_get_size(&scene_ptr->objects));
	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_3d_handle* object_handle = s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (!object_handle || !se_object_3d_handle_exists(ctx, *object_handle)) {
			continue;
		}
		se_object_3d* object = se_object_3d_from_handle(ctx, *object_handle);
		if (object && object->is_custom) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			return NULL;
		}
		if (!object || !se_model_handle_exists(ctx, object->model)) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return NULL;
		}
		se_model_list_add_unique(&model_handles, object->model);
	}

	s_json* models_json = se_scene_2d_json_add_array(root, "models");
	if (!models_json) {
		s_array_clear(&model_handles);
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	for (sz model_index = 0; model_index < s_array_get_size(&model_handles); ++model_index) {
		se_model_handle* model_handle = s_array_get(&model_handles, s_array_handle(&model_handles, (u32)model_index));
		if (!model_handle || !se_model_handle_exists(ctx, *model_handle)) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return NULL;
		}
		se_model* model = se_model_from_handle(ctx, *model_handle);
		if (!model) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return NULL;
		}

		const c8* source_path = NULL;
		if (!se_scene_3d_model_get_source_path(model, &source_path)) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			return NULL;
		}
		c8 source_path_json[SE_MAX_PATH_LENGTH] = {0};
		se_scene_json_serialize_path(source_path, source_path_json, sizeof(source_path_json));
		const c8* source_path_serialized = source_path_json[0] != '\0' ? source_path_json : source_path;

		s_json* model_json = se_scene_2d_json_add_object(models_json, NULL);
		s_json* shaders_json = se_scene_2d_json_add_array(model_json, "shaders");
		s_json* mesh_matrices_json = se_scene_2d_json_add_array(model_json, "mesh_matrices");
		if (!model_json || !shaders_json || !mesh_matrices_json ||
			!se_scene_2d_json_add_string(model_json, "source_path", source_path_serialized)) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}

		for (sz mesh_index = 0; mesh_index < s_array_get_size(&model->meshes); ++mesh_index) {
			se_mesh* mesh = s_array_get(&model->meshes, s_array_handle(&model->meshes, (u32)mesh_index));
			if (!mesh || !se_shader_handle_exists(ctx, mesh->shader)) {
				s_array_clear(&model_handles);
				s_json_free(root);
				se_set_last_error(SE_RESULT_NOT_FOUND);
				return NULL;
			}
			se_shader* shader = s_array_get(&ctx->shaders, mesh->shader);
			if (!shader || shader->vertex_path[0] == '\0' || shader->fragment_path[0] == '\0') {
				s_array_clear(&model_handles);
				s_json_free(root);
				se_set_last_error(SE_RESULT_UNSUPPORTED);
				return NULL;
			}
			c8 vertex_path_json[SE_MAX_PATH_LENGTH] = {0};
			c8 fragment_path_json[SE_MAX_PATH_LENGTH] = {0};
			se_scene_json_serialize_path(shader->vertex_path, vertex_path_json, sizeof(vertex_path_json));
			se_scene_json_serialize_path(shader->fragment_path, fragment_path_json, sizeof(fragment_path_json));
			const c8* vertex_path = vertex_path_json[0] != '\0' ? vertex_path_json : shader->vertex_path;
			const c8* fragment_path = fragment_path_json[0] != '\0' ? fragment_path_json : shader->fragment_path;
			s_json* shader_json = se_scene_2d_json_add_object(shaders_json, NULL);
			if (!shader_json ||
				!se_scene_2d_json_add_string(shader_json, "vertex_path", vertex_path) ||
				!se_scene_2d_json_add_string(shader_json, "fragment_path", fragment_path) ||
				!se_scene_json_add_mat4(mesh_matrices_json, NULL, &mesh->matrix)) {
				s_array_clear(&model_handles);
				s_json_free(root);
				se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
				return NULL;
			}
		}
	}

	s_json* objects_json = se_scene_2d_json_add_array(root, "objects");
	if (!objects_json) {
		s_array_clear(&model_handles);
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_3d_handle* object_handle = s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (!object_handle || !se_object_3d_handle_exists(ctx, *object_handle)) {
			continue;
		}
		se_object_3d* object = se_object_3d_from_handle(ctx, *object_handle);
		if (!object) {
			continue;
		}
		if (object->is_custom) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			return NULL;
		}
		u32 model_index = 0u;
		if (!se_scene_3d_model_index_of(&model_handles, object->model, &model_index)) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return NULL;
		}

		s_json* object_json = se_object_3d_to_json(*object_handle);
		if (!object_json) {
			s_array_clear(&model_handles);
			s_json_free(root);
			return NULL;
		}
		if (!se_scene_2d_json_add_u32(object_json, "model_index", model_index)) {
			s_json_free(object_json);
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}
		if (!se_scene_2d_json_add_child(objects_json, object_json)) {
			s_array_clear(&model_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}
	}

	s_array_clear(&model_handles);
	se_set_last_error(SE_RESULT_OK);
	return root;
}

b8 se_scene_3d_from_json(const se_scene_3d_handle scene, const s_json* root) {
	se_context* ctx = se_current_context();
	if (!ctx || scene == S_HANDLE_NULL || !root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_scene_3d* scene_ptr = se_scene_3d_from_handle(ctx, scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (scene_ptr->output == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (root->type != S_JSON_OBJECT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const c8* format = NULL;
	u32 version = 0u;
	if (!se_scene_json_string_to_c8(se_scene_json_get_required(root, "format", S_JSON_STRING), &format) ||
		!se_scene_json_number_to_u32(se_scene_json_get_required(root, "version", S_JSON_NUMBER), &version)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (strcmp(format, SE_SCENE_3D_JSON_FORMAT) != 0 || version != SE_SCENE_3D_JSON_VERSION) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}

	const s_json* output_size_json = se_scene_json_get_required(root, "output_size", S_JSON_ARRAY);
	const s_json* culling_json = se_scene_json_get_required(root, "culling", S_JSON_BOOL);
	const s_json* models_json = se_scene_json_get_required(root, "models", S_JSON_ARRAY);
	const s_json* objects_json = se_scene_json_get_required(root, "objects", S_JSON_ARRAY);
	if (!output_size_json || !culling_json || !models_json || !objects_json) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_vec2 output_size = s_vec2(0.0f, 0.0f);
	if (!se_scene_2d_json_read_vec2_node(output_size_json, &output_size) ||
		output_size.x <= 0.0f ||
		output_size.y <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	b8 culling = true;
	if (!se_scene_json_bool_to_b8(culling_json, &culling)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_models_ptr loaded_models = {0};
	se_objects_3d_ptr loaded_objects = {0};
	se_shaders_ptr loaded_shaders = {0};
	se_shaders_ptr existing_shaders = {0};
	s_array_init(&loaded_models);
	s_array_init(&loaded_objects);
	s_array_init(&loaded_shaders);
	se_scene_2d_collect_existing_shaders(ctx, &existing_shaders);
	s_array_reserve(&loaded_models, models_json->as.children.count);
	s_array_reserve(&loaded_objects, objects_json->as.children.count);
	s_array_reserve(&loaded_shaders, models_json->as.children.count);

	for (sz model_index = 0; model_index < models_json->as.children.count; ++model_index) {
		const s_json* model_json = s_json_at(models_json, model_index);
		if (!model_json || model_json->type != S_JSON_OBJECT) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		const c8* source_path = NULL;
		const s_json* shaders_json = se_scene_json_get_required(model_json, "shaders", S_JSON_ARRAY);
		const s_json* mesh_matrices_json = se_scene_json_get_required(model_json, "mesh_matrices", S_JSON_ARRAY);
		if (!se_scene_json_string_to_c8(se_scene_json_get_required(model_json, "source_path", S_JSON_STRING), &source_path) ||
			!source_path || source_path[0] == '\0' ||
			!shaders_json || !mesh_matrices_json ||
			shaders_json->as.children.count == 0 ||
			shaders_json->as.children.count != mesh_matrices_json->as.children.count) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		se_shaders_ptr shader_handles = {0};
		s_array_init(&shader_handles);
		s_array_reserve(&shader_handles, shaders_json->as.children.count);
		for (sz shader_index = 0; shader_index < shaders_json->as.children.count; ++shader_index) {
			const s_json* shader_json = s_json_at(shaders_json, shader_index);
			if (!shader_json || shader_json->type != S_JSON_OBJECT) {
				s_array_clear(&shader_handles);
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
			const c8* vertex_path = NULL;
			const c8* fragment_path = NULL;
			if (!se_scene_json_string_to_c8(se_scene_json_get_required(shader_json, "vertex_path", S_JSON_STRING), &vertex_path) ||
				!se_scene_json_string_to_c8(se_scene_json_get_required(shader_json, "fragment_path", S_JSON_STRING), &fragment_path) ||
				!vertex_path || !fragment_path ||
				vertex_path[0] == '\0' || fragment_path[0] == '\0') {
				s_array_clear(&shader_handles);
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}

			se_shader_handle shader_handle = S_HANDLE_NULL;
			if (!se_scene_3d_find_shader_by_paths(ctx, vertex_path, fragment_path, &shader_handle)) {
				shader_handle = se_shader_load(vertex_path, fragment_path);
				if (shader_handle == S_HANDLE_NULL) {
					s_array_clear(&shader_handles);
					goto fail;
				}
				se_shader_list_add_unique(&loaded_shaders, shader_handle);
			}
			s_array_add(&shader_handles, shader_handle);
		}

		const se_model_handle model_handle = se_model_load_obj_with_flags(source_path, &shader_handles, SE_MESH_DATA_CPU_GPU);
		s_array_clear(&shader_handles);
		if (model_handle == S_HANDLE_NULL) {
			goto fail;
		}
		s_array_add(&loaded_models, model_handle);

		se_model* model = se_model_from_handle(ctx, model_handle);
		if (!model) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			goto fail;
		}
		const sz mesh_count = s_array_get_size(&model->meshes);
		if (mesh_count != mesh_matrices_json->as.children.count ||
			mesh_count != shaders_json->as.children.count) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		for (sz mesh_index = 0; mesh_index < mesh_count; ++mesh_index) {
			se_mesh* mesh = s_array_get(&model->meshes, s_array_handle(&model->meshes, (u32)mesh_index));
			const s_json* matrix_json = s_json_at(mesh_matrices_json, mesh_index);
			s_mat4 matrix = s_mat4_identity;
			if (!mesh || !matrix_json || !se_scene_json_read_mat4(matrix_json, &matrix)) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
			mesh->matrix = matrix;
		}
	}

	for (sz object_index = 0; object_index < objects_json->as.children.count; ++object_index) {
		const s_json* object_json = s_json_at(objects_json, object_index);
		if (!object_json || object_json->type != S_JSON_OBJECT) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		u32 model_index = 0u;
		u32 instance_capacity = 0u;
		const s_json* instances_json = se_scene_json_get_required(object_json, "instances", S_JSON_ARRAY);
		if (!se_scene_json_number_to_u32(se_scene_json_get_required(object_json, "model_index", S_JSON_NUMBER), &model_index) ||
			!se_scene_json_number_to_u32(se_scene_json_get_required(object_json, "instance_capacity", S_JSON_NUMBER), &instance_capacity) ||
			!instances_json ||
			model_index >= s_array_get_size(&loaded_models)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		se_model_handle* model_handle = s_array_get(&loaded_models, s_array_handle(&loaded_models, model_index));
		if (!model_handle || !se_model_handle_exists(ctx, *model_handle)) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			goto fail;
		}

		const sz slot_count = instances_json->as.children.count;
		const sz create_capacity = s_max((sz)s_max(instance_capacity, 1u), slot_count);
		const s_mat4 create_transform = s_mat4_identity;
		const se_object_3d_handle object_handle = se_object_3d_create(*model_handle, &create_transform, create_capacity);
		if (object_handle == S_HANDLE_NULL) {
			goto fail;
		}
		s_array_add(&loaded_objects, object_handle);

		if (!se_object_3d_from_json(object_handle, object_json)) {
			goto fail;
		}
	}

	se_framebuffer_set_size(scene_ptr->output, &output_size);
	if (scene_ptr->camera != S_HANDLE_NULL) {
		se_camera_set_aspect(scene_ptr->camera, output_size.x, output_size.y);
	}
	scene_ptr->enable_culling = culling;
	while (s_array_get_size(&scene_ptr->objects) > 0) {
		s_array_remove(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)(s_array_get_size(&scene_ptr->objects) - 1)));
	}
	s_array_reserve(&scene_ptr->objects, s_array_get_size(&loaded_objects));
	for (sz i = 0; i < s_array_get_size(&loaded_objects); ++i) {
		se_object_3d_handle* object_handle = s_array_get(&loaded_objects, s_array_handle(&loaded_objects, (u32)i));
		if (!object_handle) {
			continue;
		}
		s_array_add(&scene_ptr->objects, *object_handle);
	}

	s_array_clear(&existing_shaders);
	s_array_clear(&loaded_shaders);
	s_array_clear(&loaded_models);
	s_array_clear(&loaded_objects);
	se_set_last_error(SE_RESULT_OK);
	return true;

fail:
	se_scene_3d_cleanup_loaded_data(ctx, &loaded_objects, &loaded_models, &existing_shaders, &loaded_shaders);
	s_array_clear(&existing_shaders);
	return false;
}

se_scene_3d_handle se_scene_3d_create(const s_vec2 *size, const u16 object_count) {
	se_context *ctx = se_current_context();
	if (!ctx || !size || object_count == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->scenes_3d) == 0) {
		s_array_init(&ctx->scenes_3d);
	}
	if (s_array_get_size(&ctx->scenes_3d) >= SE_MAX_SCENES) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_scene_3d_handle scene_handle = s_array_increment(&ctx->scenes_3d);
	se_scene_3d *new_scene = s_array_get(&ctx->scenes_3d, scene_handle);
	memset(new_scene, 0, sizeof(*new_scene));
	se_framebuffer_handle framebuffer = se_framebuffer_create(size);
	if (framebuffer == S_HANDLE_NULL) {
		s_array_remove(&ctx->scenes_3d, scene_handle);
		return S_HANDLE_NULL;
	}
	new_scene->output = framebuffer;
	s_array_init(&new_scene->objects);
	s_array_init(&new_scene->post_process);
	s_array_init(&new_scene->custom_renders);
	s_array_reserve(&new_scene->objects, object_count);
	s_array_reserve(&new_scene->post_process, object_count);
	s_array_reserve(&new_scene->custom_renders, 8);
	new_scene->output_shader = S_HANDLE_NULL;
	se_camera_handle camera = se_camera_create();
	if (camera == S_HANDLE_NULL) {
		se_framebuffer_destroy(new_scene->output);
		s_array_remove(&ctx->scenes_3d, scene_handle);
		return S_HANDLE_NULL;
	}
	new_scene->camera = camera;
	new_scene->enable_culling = true;
	new_scene->last_vp = s_mat4_identity;
	new_scene->has_last_vp = false;
	if (new_scene->camera != S_HANDLE_NULL) {
		se_camera_set_aspect(new_scene->camera, size->x, size->y);
	}
	se_set_last_error(SE_RESULT_OK);
	return scene_handle;
}

se_scene_3d_handle se_scene_3d_create_for_window(const se_window_handle window, const u16 object_count) {
	se_context *ctx = se_current_context();
	if (!ctx || window == S_HANDLE_NULL || object_count == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	const s_vec2 size = s_vec2((f32)window_ptr->width, (f32)window_ptr->height);
	se_scene_3d_handle scene = se_scene_3d_create(&size, object_count);
	if (scene == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}

	const s_vec2 ratio = s_vec2(1.0f, 1.0f);
	se_scene_3d_set_fit_to_window(scene, window, &ratio);
	se_set_last_error(SE_RESULT_OK);
	return scene;
}

static void se_scene_3d_resize_callback(const se_window_handle window, void *scene_data) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_scene_3d_resize_callback :: ctx is null");
	s_assertf(scene_data, "se_scene_3d_resize_callback :: scene is null");

	se_scene_3d_handle scene = (se_scene_3d_handle)(uintptr_t)scene_data;
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	se_scene_3d *scene_ptr = s_array_get(&ctx->scenes_3d, scene);
	s_assertf(window_ptr, "se_scene_3d_resize_callback :: window_ptr is null");
	s_assertf(scene_ptr, "se_scene_3d_resize_callback :: scene_ptr is null");

	se_framebuffer_handle framebuffer = scene_ptr->output;
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, framebuffer);
	s_vec2 new_size = {framebuffer_ptr->ratio.x * window_ptr->width,
						framebuffer_ptr->ratio.y * window_ptr->height};

	se_framebuffer_set_size(framebuffer, &new_size);
	if (scene_ptr->camera != S_HANDLE_NULL) {
		se_camera_set_aspect(scene_ptr->camera, new_size.x, new_size.y);
	}
	se_scene_3d_render_to_buffer(scene);
}

void se_scene_3d_set_fit_to_window(const se_scene_3d_handle scene, const se_window_handle window, const s_vec2 *ratio) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_set_fit_to_window :: scene is null");
	s_assertf(ratio, "se_scene_3d_set_fit_to_window :: ratio is null");

	s_assertf(ratio->x == 1.0 && ratio->y == 1.0,
			"se_scene_3d_set_fit_to_window :: ratio can only be 1 for now, "
			"please wait for the next update");
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, scene_ptr->output);
	framebuffer_ptr->auto_resize = true;
	framebuffer_ptr->ratio = *ratio;
	se_window_register_resize_event(window, se_scene_3d_resize_callback, (void *)(uintptr_t)scene);
}

void se_scene_3d_destroy(const se_scene_3d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	if (scene_ptr->camera != S_HANDLE_NULL && ctx) {
		se_camera_destroy(scene_ptr->camera);
		scene_ptr->camera = S_HANDLE_NULL;
	}
	if (scene_ptr->output != S_HANDLE_NULL) {
		se_framebuffer_destroy(scene_ptr->output);
		scene_ptr->output = S_HANDLE_NULL;
	}
	s_array_clear(&scene_ptr->post_process);
	s_array_clear(&scene_ptr->custom_renders);
	s_array_clear(&scene_ptr->objects);
	s_array_remove(&ctx->scenes_3d, scene);
}

void se_scene_3d_destroy_full(const se_scene_3d_handle scene, const b8 destroy_models, const b8 destroy_model_shaders) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_scene_3d_destroy_full :: ctx is null");
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_destroy_full :: scene is null");

	const b8 destroy_shaders = destroy_models && destroy_model_shaders;
	if (destroy_model_shaders && !destroy_models) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_SCENE,
			"se_scene_3d_destroy_full ignored destroy_model_shaders because destroy_models is false");
	}

	se_objects_3d_ptr object_handles = {0};
	se_models_ptr model_handles = {0};
	se_shaders_ptr shader_handles = {0};
	s_array_init(&object_handles);
	s_array_init(&model_handles);
	s_array_init(&shader_handles);

	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_3d_handle* handle_ptr = s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (!handle_ptr || *handle_ptr == S_HANDLE_NULL || !se_object_3d_handle_exists(ctx, *handle_ptr)) {
			continue;
		}
		se_object_3d_list_add_unique(&object_handles, *handle_ptr);
		if (!destroy_models) {
			continue;
		}
		se_object_3d* object_ptr = se_object_3d_from_handle(ctx, *handle_ptr);
		if (!object_ptr || object_ptr->is_custom || !se_model_handle_exists(ctx, object_ptr->model)) {
			continue;
		}
		se_model_list_add_unique(&model_handles, object_ptr->model);
	}

	if (destroy_shaders) {
		for (sz i = 0; i < s_array_get_size(&model_handles); ++i) {
			se_model_handle* model_handle = s_array_get(&model_handles, s_array_handle(&model_handles, (u32)i));
			if (!model_handle || !se_model_handle_exists(ctx, *model_handle)) {
				continue;
			}
			se_model* model_ptr = se_model_from_handle(ctx, *model_handle);
			if (!model_ptr) {
				continue;
			}
			for (sz mesh_index = 0; mesh_index < s_array_get_size(&model_ptr->meshes); ++mesh_index) {
				se_mesh* mesh = s_array_get(&model_ptr->meshes, s_array_handle(&model_ptr->meshes, (u32)mesh_index));
				if (!mesh || !se_shader_handle_exists(ctx, mesh->shader)) {
					continue;
				}
				se_shader_list_add_unique(&shader_handles, mesh->shader);
			}
		}
	}

	for (sz i = 0; i < s_array_get_size(&object_handles); ++i) {
		se_object_3d_handle* object_handle = s_array_get(&object_handles, s_array_handle(&object_handles, (u32)i));
		if (!object_handle || !se_object_3d_handle_exists(ctx, *object_handle)) {
			continue;
		}
		se_object_3d_destroy(*object_handle);
	}

	if (destroy_models) {
		for (sz i = 0; i < s_array_get_size(&model_handles); ++i) {
			se_model_handle* model_handle = s_array_get(&model_handles, s_array_handle(&model_handles, (u32)i));
			if (!model_handle || !se_model_handle_exists(ctx, *model_handle)) {
				continue;
			}
			se_model_destroy(*model_handle);
		}
	}

	if (destroy_shaders) {
		for (sz i = 0; i < s_array_get_size(&shader_handles); ++i) {
			se_shader_handle* shader_handle = s_array_get(&shader_handles, s_array_handle(&shader_handles, (u32)i));
			if (!shader_handle || !se_shader_handle_exists(ctx, *shader_handle)) {
				continue;
			}
			se_shader_destroy(*shader_handle);
		}
	}

	s_array_clear(&shader_handles);
	s_array_clear(&model_handles);
	s_array_clear(&object_handles);
	se_scene_3d_destroy(scene);
}

void se_scene_3d_render_to_buffer(const se_scene_3d_handle scene) {
	se_debug_trace_begin("scene3d_render");
	se_context *ctx = se_current_context();
	if (ctx == NULL) {
		se_debug_trace_end("scene3d_render");
		return;
	}
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_render_to_buffer :: scene is null");
	s_assertf(scene_ptr->output != S_HANDLE_NULL, "se_scene_3d_render_to_buffer :: scene output is null");

	se_framebuffer_bind(scene_ptr->output);
	se_render_clear();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	if (scene_ptr->enable_culling) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
	} else {
		glDisable(GL_CULL_FACE);
	}

	s_mat4 view = s_mat4_identity;
	s_mat4 proj = s_mat4_identity;
	if (scene_ptr->camera != S_HANDLE_NULL) {
		view = se_camera_get_view_matrix(scene_ptr->camera);
		proj = se_camera_get_projection_matrix(scene_ptr->camera);
	}
	s_mat4 vp = s_mat4_mul(&proj, &view);
	b8 camera_changed = true;
	if (scene_ptr->has_last_vp) {
		camera_changed = !se_mat4_near_equal(&scene_ptr->last_vp, &vp, 0.00001f);
	}
	scene_ptr->last_vp = vp;
	scene_ptr->has_last_vp = true;

	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_3d_handle object_handle = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (object_handle == S_HANDLE_NULL) {
			continue;
		}
		se_object_3d *object = se_object_3d_from_handle(ctx, object_handle);
		if (!object || !object->is_visible) {
			continue;
		}
		if (object->is_custom) {
			if (object->custom.render) {
				object->custom.render(object->custom.data);
			}
			continue;
		}
		if (object->model == S_HANDLE_NULL) {
			continue;
		}
		se_model *model = se_model_from_handle(ctx, object->model);
		if (!model) {
			continue;
		}
		const sz active_count = se_instances_active_count(&object->instances.actives);
		if (active_count != s_array_get_size(&object->render_transforms)) {
			se_object_3d_sync_render_instances(object);
		}
		const sz instance_count = s_array_get_size(&object->render_transforms);
		if (instance_count == 0) {
			continue;
		}
		const b8 object_requires_upload = camera_changed || se_object_3d_are_instances_dirty(object_handle);

		const sz mesh_count = s_array_get_size(&object->mesh_instances);
		sz mesh_index = 0;
		for (sz m = 0; m < s_array_get_size(&model->meshes); ++m) {
			se_mesh *mesh = s_array_get(&model->meshes, s_array_handle(&model->meshes, (u32)m));
			if (mesh_index >= mesh_count) {
				break;
			}
			se_mesh_instance *mesh_instance = s_array_get(&object->mesh_instances, s_array_handle(&object->mesh_instances, (u32)mesh_index));
			mesh_index++;
			if (mesh == NULL || !se_mesh_has_gpu_data(mesh) || mesh->gpu.index_count == 0) {
				continue;
			}
			if (mesh_instance == NULL || mesh_instance->vao == 0) {
				continue;
			}
			if (object_requires_upload) {
				sz active_index = 0;
				for (sz j = 0; j < s_array_get_size(&object->instances.transforms); ++j) {
					b8* active = s_array_get(&object->instances.actives, s_array_handle(&object->instances.actives, (u32)j));
					if (!active || !*active) {
						continue;
					}
					s_mat4 *instance_transform = s_array_get(&object->instances.transforms, s_array_handle(&object->instances.transforms, (u32)j));
					s_mat4 *out_buffer = s_array_get(&object->render_transforms, s_array_handle(&object->render_transforms, (u32)active_index));
					if (!instance_transform || !out_buffer) {
						continue;
					}
					s_mat4 model_matrix = s_mat4_mul(&object->transform, instance_transform);
					model_matrix = s_mat4_mul(&model_matrix, &mesh->matrix);
					*out_buffer = s_mat4_mul(&vp, &model_matrix);
					active_index++;
				}
				mesh_instance->instance_buffers_dirty = true;
				se_mesh_instance_update(mesh_instance);
			}

			se_shader_handle shader = mesh->shader;
			if (shader == S_HANDLE_NULL) {
				se_log("se_scene_3d_render_to_buffer :: mesh->shader is null");
				continue;
			}

			se_shader_use(shader, true, true);
			glBindVertexArray(mesh_instance->vao);
			glDrawElementsInstanced(GL_TRIANGLES, mesh->gpu.index_count, GL_UNSIGNED_INT, 0, (GLsizei)instance_count);
		}
	}

	se_scene_3d_invoke_custom_renders(scene, scene_ptr);
	se_framebuffer_unbind(scene_ptr->output);
	se_debug_trace_end("scene3d_render");
}

void se_scene_3d_render_to_screen(const se_scene_3d_handle scene, const se_window_handle window) {
	se_context *ctx = se_current_context();
	if (ctx == NULL) {
		return;
	}
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	se_window *window_ptr = s_array_get(&ctx->windows, window);
	se_framebuffer *framebuffer_ptr = s_array_get(&ctx->framebuffers, scene_ptr->output);

	se_render_unbind_framebuffer();
	se_render_clear();
	se_render_set_blending(true);
	GLboolean previous_depth_mask = GL_TRUE;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &previous_depth_mask);
	glDepthMask(GL_FALSE);
	se_shader_set_texture(window_ptr->shader, "u_texture", framebuffer_ptr->texture);
	se_window_render_quad(window);
	glDepthMask(previous_depth_mask);
	se_render_set_blending(false);
}

void se_scene_3d_draw(const se_scene_3d_handle scene, const se_window_handle window) {
	se_scene_3d_render_to_buffer(scene);
	se_scene_3d_render_to_screen(scene, window);
	//se_window_render_screen(window); // TODO: Check if this is causing some flickering
}

void se_scene_3d_add_object(const se_scene_3d_handle scene, const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_add_object :: scene is null");
	s_array_add(&scene_ptr->objects, object);
}

se_object_3d_handle se_scene_3d_add_model(const se_scene_3d_handle scene, const se_model_handle model, const s_mat4 *transform) {
	se_context *ctx = se_current_context();
	if (!ctx || model == S_HANDLE_NULL || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_object_3d_handle object = se_object_3d_create(model, transform, 0);
	if (object == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	se_scene_3d_add_object(scene, object);
	se_set_last_error(SE_RESULT_OK);
	return object;
}

void se_scene_3d_remove_object(const se_scene_3d_handle scene, const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_remove_object :: scene is null");
	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_3d_handle current = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (current == object) {
			s_array_remove(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
			break;
		}
	}
}

b8 se_scene_3d_pick_object_screen(const se_scene_3d_handle scene, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, const f32 pick_radius, se_scene_pick_filter_3d filter, void* user_data, se_object_3d_handle* out_object, f32* out_distance) {
	if (!out_object || viewport_width <= 1.0f || viewport_height <= 1.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_pick_object_screen :: scene is null");
	if (scene_ptr->camera == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	s_vec3 ray_origin = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 ray_direction = s_vec3(0.0f, 0.0f, -1.0f);
	if (!se_camera_screen_to_ray(scene_ptr->camera, screen_x, screen_y, viewport_width, viewport_height, &ray_origin, &ray_direction)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const f32 radius = s_max(pick_radius, 0.0001f);
	f32 best_distance = FLT_MAX;
	se_object_3d_handle best_object = S_HANDLE_NULL;
	for (sz i = 0; i < s_array_get_size(&scene_ptr->objects); ++i) {
		se_object_3d_handle object_handle = *s_array_get(&scene_ptr->objects, s_array_handle(&scene_ptr->objects, (u32)i));
		if (object_handle == S_HANDLE_NULL) {
			continue;
		}
		if (filter && !filter(object_handle, user_data)) {
			continue;
		}
		se_object_3d *object = se_object_3d_from_handle(ctx, object_handle);
		if (!object || !object->is_visible || object->is_custom) {
			continue;
		}
		for (sz j = 0; j < s_array_get_size(&object->instances.transforms); ++j) {
			b8* active = s_array_get(&object->instances.actives, s_array_handle(&object->instances.actives, (u32)j));
			if (!active || !*active) {
				continue;
			}
			s_mat4 *instance_transform = s_array_get(&object->instances.transforms, s_array_handle(&object->instances.transforms, (u32)j));
			if (!instance_transform) {
				continue;
			}
			s_mat4 world_transform = s_mat4_mul(&object->transform, instance_transform);
			const s_vec3 center = s_mat4_get_translation(&world_transform);
			const s_vec3 origin_to_center = s_vec3_sub(&center, &ray_origin);
			const f32 t = s_vec3_dot(&origin_to_center, &ray_direction);
			if (t < 0.0f) {
				continue;
			}
			const s_vec3 projected = s_vec3_add(&ray_origin, &s_vec3_muls(&ray_direction, t));
			const s_vec3 delta = s_vec3_sub(&center, &projected);
			const f32 distance = s_vec3_length(&delta);
			if (distance <= radius && t < best_distance) {
				best_distance = t;
				best_object = object_handle;
			}
		}
	}
	*out_object = best_object;
	if (out_distance) {
		*out_distance = best_distance;
	}
	if (best_object == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_scene_3d_set_camera(const se_scene_3d_handle scene, const se_camera_handle camera) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_set_camera :: scene is null");
	scene_ptr->camera = camera;
}

se_camera_handle se_scene_3d_get_camera(const se_scene_3d_handle scene) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return scene_ptr->camera;
}

b8 se_scene_3d_get_output_depth_texture(const se_scene_3d_handle scene, u32* out_depth_texture) {
	if (!out_depth_texture) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	if (!scene_ptr || scene_ptr->output == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	return se_framebuffer_get_depth_texture_id(scene_ptr->output, out_depth_texture);
}

void se_scene_3d_set_culling(const se_scene_3d_handle scene, const b8 enabled) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_assertf(scene_ptr, "se_scene_3d_set_culling :: scene is null");
	scene_ptr->enable_culling = enabled;
}

void se_scene_3d_add_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	s_array_add(&scene_ptr->post_process, buffer);
}

void se_scene_3d_remove_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer) {
	se_context *ctx = se_current_context();
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	for (sz i = 0; i < s_array_get_size(&scene_ptr->post_process); ++i) {
		se_render_buffer_handle current = *s_array_get(&scene_ptr->post_process, s_array_handle(&scene_ptr->post_process, (u32)i));
		if (current == buffer) {
			s_array_remove(&scene_ptr->post_process, s_array_handle(&scene_ptr->post_process, (u32)i));
			break;
		}
	}
}

se_scene_3d_custom_render_handle se_scene_3d_register_custom_render(const se_scene_3d_handle scene, se_scene_3d_custom_render_callback callback, void* user_data) {
	if (!callback || scene == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_context *ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}
	se_scene_3d_custom_render_handle callback_handle = s_array_increment(&scene_ptr->custom_renders);
	se_scene_3d_custom_render_entry* new_entry = s_array_get(&scene_ptr->custom_renders, callback_handle);
	if (!new_entry) {
		s_array_remove(&scene_ptr->custom_renders, callback_handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	memset(new_entry, 0, sizeof(*new_entry));
	new_entry->callback = callback;
	new_entry->user_data = user_data;
	se_set_last_error(SE_RESULT_OK);
	return callback_handle;
}

b8 se_scene_3d_unregister_custom_render(const se_scene_3d_handle scene, const se_scene_3d_custom_render_handle callback_handle) {
	if (scene == S_HANDLE_NULL || callback_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_context *ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_scene_3d *scene_ptr = se_scene_3d_from_handle(ctx, scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (!s_array_get(&scene_ptr->custom_renders, callback_handle)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	s_array_remove(&scene_ptr->custom_renders, callback_handle);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

se_object_3d_handle se_object_3d_create(const se_model_handle model, const s_mat4 *transform, const sz max_instances_count) {
	se_context *ctx = se_current_context();
	if (!ctx || model == S_HANDLE_NULL || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->objects_3d) == 0) {
		s_array_init(&ctx->objects_3d);
	}
	if (s_array_get_size(&ctx->objects_3d) >= SE_MAX_3D_OBJECTS) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_object_3d_handle object_handle = s_array_increment(&ctx->objects_3d);
	se_object_3d *new_object = s_array_get(&ctx->objects_3d, object_handle);
	memset(new_object, 0, sizeof(*new_object));
	new_object->model = model;
	new_object->transform = *transform;
	new_object->is_custom = false;
	new_object->is_visible = true;
	const sz instance_capacity = (max_instances_count > 0) ? max_instances_count : 1;
	s_array_init(&new_object->instances.ids);
	s_array_init(&new_object->instances.transforms);
	s_array_init(&new_object->instances.buffers);
	s_array_init(&new_object->instances.actives);
	s_array_init(&new_object->instances.free_indices);
	s_array_init(&new_object->instances.metadata);
	s_array_init(&new_object->render_transforms);
	s_array_reserve(&new_object->instances.ids, instance_capacity);
	s_array_reserve(&new_object->instances.transforms, instance_capacity);
	s_array_reserve(&new_object->instances.buffers, instance_capacity);
	s_array_reserve(&new_object->instances.actives, instance_capacity);
	s_array_reserve(&new_object->instances.free_indices, instance_capacity);
	s_array_reserve(&new_object->instances.metadata, instance_capacity);
	s_array_reserve(&new_object->render_transforms, instance_capacity);
	new_object->instances.next_id = 0;

	se_model *model_ptr = se_model_from_handle(ctx, model);
	const sz mesh_count = s_array_get_size(&model_ptr->meshes);
	if (mesh_count > 0) {
		s_array_init(&new_object->mesh_instances);
		s_array_reserve(&new_object->mesh_instances, mesh_count);
		for (sz i = 0; i < mesh_count; ++i) {
			se_mesh *mesh = s_array_get(&model_ptr->meshes, s_array_handle(&model_ptr->meshes, (u32)i));
			s_handle mesh_inst_handle = s_array_increment(&new_object->mesh_instances);
			se_mesh_instance *mesh_instance = s_array_get(&new_object->mesh_instances, mesh_inst_handle);
			memset(mesh_instance, 0, sizeof(*mesh_instance));
			if (!se_mesh_has_gpu_data(mesh)) {
				continue;
			}
			se_mesh_instance_create(mesh_instance, mesh, (u32)instance_capacity);
			se_mesh_instance_add_buffer(mesh_instance, s_array_get_data(&new_object->render_transforms), instance_capacity);
		}
	}

	if (max_instances_count == 0) {
		s_handle id_handle = s_array_increment(&new_object->instances.ids);
		s_handle transform_handle = s_array_increment(&new_object->instances.transforms);
		s_handle buffer_handle = s_array_increment(&new_object->instances.buffers);
		s_handle active_handle = s_array_increment(&new_object->instances.actives);
		s_handle metadata_handle = s_array_increment(&new_object->instances.metadata);
		s_handle render_handle = s_array_increment(&new_object->render_transforms);
		se_instance_id *new_instance_id = s_array_get(&new_object->instances.ids, id_handle);
		s_mat4 *new_transform = s_array_get(&new_object->instances.transforms, transform_handle);
		s_mat4 *new_buffer = s_array_get(&new_object->instances.buffers, buffer_handle);
		b8 *new_active = s_array_get(&new_object->instances.actives, active_handle);
		s_mat4 *new_metadata = s_array_get(&new_object->instances.metadata, metadata_handle);
		s_mat4 *new_render_transform = s_array_get(&new_object->render_transforms, render_handle);
		*new_instance_id = 0;
		*new_transform = s_mat4_identity;
		*new_buffer = s_mat4_identity;
		*new_active = true;
		*new_metadata = s_mat4_identity;
		*new_render_transform = s_mat4_identity;
		new_object->instances.next_id = 1;
		se_object_3d_set_instances_dirty(object_handle, true);
	}

	se_set_last_error(SE_RESULT_OK);
	return object_handle;
}

se_object_3d_handle se_object_3d_create_custom(se_object_custom *custom, const s_mat4 *transform) {
	se_context *ctx = se_current_context();
	if (!ctx || !custom || !transform) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	s_assertf(custom->data_size <= SE_OBJECT_CUSTOM_DATA_SIZE, "se_object_3d_create_custom :: data_size exceeds SE_OBJECT_CUSTOM_DATA_SIZE");
	if (s_array_get_capacity(&ctx->objects_3d) == 0) {
		s_array_init(&ctx->objects_3d);
	}
	if (s_array_get_size(&ctx->objects_3d) >= SE_MAX_3D_OBJECTS) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	se_object_3d_handle object_handle = s_array_increment(&ctx->objects_3d);
	se_object_3d *new_object = s_array_get(&ctx->objects_3d, object_handle);
	memset(new_object, 0, sizeof(*new_object));
	new_object->transform = *transform;
	new_object->is_custom = true;
	new_object->is_visible = true;
	memcpy(&new_object->custom, custom, sizeof(se_object_custom));
	se_set_last_error(SE_RESULT_OK);
	return object_handle;
}

void se_object_3d_destroy(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_object_3d_destroy :: ctx is null");
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_destroy :: object is null");
	if (object_ptr->is_custom) {
		object_ptr->custom.render = NULL;
		object_ptr->custom.data_size = 0;
		object_ptr->is_visible = false;
		s_array_remove(&ctx->objects_3d, object);
		return;
	}
	for (sz i = 0; i < s_array_get_size(&object_ptr->mesh_instances); ++i) {
		se_mesh_instance *mesh_instance = s_array_get(&object_ptr->mesh_instances, s_array_handle(&object_ptr->mesh_instances, (u32)i));
		se_mesh_instance_destroy(mesh_instance);
	}
	s_array_clear(&object_ptr->mesh_instances);
	s_array_clear(&object_ptr->render_transforms);
	s_array_clear(&object_ptr->instances.ids);
	s_array_clear(&object_ptr->instances.transforms);
	s_array_clear(&object_ptr->instances.buffers);
	s_array_clear(&object_ptr->instances.actives);
	s_array_clear(&object_ptr->instances.free_indices);
	s_array_clear(&object_ptr->instances.metadata);
	object_ptr->model = S_HANDLE_NULL;
	object_ptr->is_visible = false;
	s_array_remove(&ctx->objects_3d, object);
}

void se_object_3d_set_transform(const se_object_3d_handle object, const s_mat4 *transform) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_transform :: object is null");
	s_assertf(transform, "se_object_3d_set_transform :: transform is null");
	object_ptr->transform = *transform;
	se_object_3d_set_instances_dirty(object, true);
}

void se_object_3d_set_rotation(const se_object_3d_handle object, const s_vec3 *rotation) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_rotation :: object is null");
	s_assertf(rotation, "se_object_3d_set_rotation :: rotation is null");

	const s_vec3 location = s_mat4_get_translation(&object_ptr->transform);
	const s_vec3 current_scale = se_mat4_extract_basis_scale(&object_ptr->transform);
	s_mat4 transform = se_mat4_rotation_xyz(rotation);
	se_mat4_apply_basis_scale(&transform, &current_scale);
	s_mat4_set_translation(&transform, &location);

	object_ptr->transform = transform;
	se_object_3d_set_instances_dirty(object, true);
}

void se_object_3d_set_scale(const se_object_3d_handle object, const s_vec3 *scale) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_scale :: object is null");
	s_assertf(scale, "se_object_3d_set_scale :: scale is null");

	const s_vec3 location = s_mat4_get_translation(&object_ptr->transform);
	const s_vec3 current_scale = se_mat4_extract_basis_scale(&object_ptr->transform);
	s_mat4 transform = se_mat4_extract_rotation(&object_ptr->transform, &current_scale);
	se_mat4_apply_basis_scale(&transform, scale);
	s_mat4_set_translation(&transform, &location);

	object_ptr->transform = transform;
	se_object_3d_set_instances_dirty(object, true);
}

s_mat4 se_object_3d_get_transform(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_get_transform :: object is null");
	return object_ptr->transform;
}

se_instance_id se_object_3d_add_instance(const se_object_3d_handle object, const s_mat4 *transform, const s_mat4 *buffer) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_add_instance :: object is null");
	s_assertf(transform, "se_object_3d_add_instance :: transform is null");
	s_assertf(buffer, "se_object_3d_add_instance :: buffer is null");
	const sz max_capacity = s_array_get_capacity(&object_ptr->instances.ids);
	sz reuse_index = 0;
	const b8 has_free_slot = se_instances_pop_free_index(&object_ptr->instances.free_indices, &reuse_index);
	if (!has_free_slot && max_capacity > 0 && s_array_get_size(&object_ptr->instances.ids) >= max_capacity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return -1;
	}

	const se_instance_id new_id = object_ptr->instances.next_id++;
	if (has_free_slot && reuse_index < s_array_get_size(&object_ptr->instances.ids)) {
		se_instance_id* id = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)reuse_index));
		s_mat4* dst_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)reuse_index));
		s_mat4* dst_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)reuse_index));
		b8* dst_active = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)reuse_index));
		s_mat4* dst_metadata = s_array_get(&object_ptr->instances.metadata, s_array_handle(&object_ptr->instances.metadata, (u32)reuse_index));
		if (id && dst_transform && dst_buffer && dst_active && dst_metadata) {
			*id = new_id;
			*dst_transform = *transform;
			*dst_buffer = *buffer;
			*dst_active = true;
			*dst_metadata = s_mat4_identity;
		}
	} else {
		b8 active = true;
		s_mat4 metadata = s_mat4_identity;
		s_array_add(&object_ptr->instances.ids, new_id);
		s_array_add(&object_ptr->instances.transforms, *transform);
		s_array_add(&object_ptr->instances.buffers, *buffer);
		s_array_add(&object_ptr->instances.actives, active);
		s_array_add(&object_ptr->instances.metadata, metadata);
	}
	se_object_3d_set_instances_dirty(object, true);

	return new_id;
}

b8 se_object_3d_remove_instance(const se_object_3d_handle object, const se_instance_id instance_id) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_remove_instance :: object is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index < 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	b8* active = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)index));
	if (active && *active) {
		*active = false;
		se_instances_push_free_index(&object_ptr->instances.free_indices, (sz)index);
		se_object_3d_set_instances_dirty(object, true);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

i32 se_object_3d_get_instance_index(const se_object_3d_handle object, const se_instance_id instance_id) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_get_instance_index :: object is null");
	const sz count = s_array_get_size(&object_ptr->instances.ids);
	if (instance_id >= 0 && (sz)instance_id < count) {
		se_instance_id *quick = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)instance_id));
		if (quick && *quick == instance_id) {
			return instance_id;
		}
	}
	for (sz i = 0; i < s_array_get_size(&object_ptr->instances.ids); ++i) {
		se_instance_id *found_instance = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)i));
		if (*found_instance == instance_id) {
			return (i32)i;
		}
	}
	return -1;
}

void se_object_3d_set_transform_by_id(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *transform) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_transform_by_id :: object is null");
	s_assertf(transform, "se_object_3d_set_transform_by_id :: transform is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)index));
		*current_transform = *transform;
		se_object_3d_set_instances_dirty(object, true);
		se_set_last_error(SE_RESULT_OK);
	} else {
		se_set_last_error(SE_RESULT_NOT_FOUND);
	}
}

void se_object_3d_set_buffer_by_id(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *buffer) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_buffer_by_id :: object is null");
	s_assertf(buffer, "se_object_3d_set_buffer_by_id :: buffer is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index >= 0) {
		s_mat4 *current_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)index));
		*current_buffer = *buffer;
		se_object_3d_set_instances_dirty(object, true);
		se_set_last_error(SE_RESULT_OK);
	} else {
		se_set_last_error(SE_RESULT_NOT_FOUND);
	}
}

void se_object_3d_set_transforms_by_id(const se_object_3d_handle object, const se_instance_id* instance_ids, const s_mat4* transforms, const sz count) {
	if (!instance_ids || !transforms) {
		return;
	}
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_transforms_by_id :: object is null");
	for (sz i = 0; i < count; ++i) {
		const i32 index = se_object_3d_get_instance_index(object, instance_ids[i]);
		if (index < 0) {
			continue;
		}
		s_mat4 *current_transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)index));
		if (current_transform) {
			*current_transform = transforms[i];
		}
	}
	se_object_3d_set_instances_dirty(object, true);
}

void se_object_3d_set_buffers_by_id(const se_object_3d_handle object, const se_instance_id* instance_ids, const s_mat4* buffers, const sz count) {
	if (!instance_ids || !buffers) {
		return;
	}
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_buffers_by_id :: object is null");
	for (sz i = 0; i < count; ++i) {
		const i32 index = se_object_3d_get_instance_index(object, instance_ids[i]);
		if (index < 0) {
			continue;
		}
		s_mat4 *current_buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)index));
		if (current_buffer) {
			*current_buffer = buffers[i];
		}
	}
	se_object_3d_set_instances_dirty(object, true);
}

b8 se_object_3d_set_active_by_id(const se_object_3d_handle object, const se_instance_id instance_id, const b8 active) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_active_by_id :: object is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index < 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	b8 *entry = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)index));
	if (!entry) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*entry = active;
	if (active) {
		se_instances_remove_free_index(&object_ptr->instances.free_indices, (sz)index);
	} else {
		se_instances_push_free_index(&object_ptr->instances.free_indices, (sz)index);
	}
	se_object_3d_set_instances_dirty(object, true);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_object_3d_is_active_by_id(const se_object_3d_handle object, const se_instance_id instance_id) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_is_active_by_id :: object is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index < 0) {
		return false;
	}
	b8 *entry = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)index));
	return entry ? *entry : false;
}

sz se_object_3d_get_inactive_slot_count(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_get_inactive_slot_count :: object is null");
	return s_array_get_size(&object_ptr->instances.free_indices);
}

b8 se_object_3d_set_metadata_by_id(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4* metadata) {
	if (!metadata) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_metadata_by_id :: object is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index < 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	s_mat4* entry = s_array_get(&object_ptr->instances.metadata, s_array_handle(&object_ptr->instances.metadata, (u32)index));
	if (!entry) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*entry = *metadata;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_object_3d_get_metadata_by_id(const se_object_3d_handle object, const se_instance_id instance_id, s_mat4* out_metadata) {
	if (!out_metadata) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_get_metadata_by_id :: object is null");
	const i32 index = se_object_3d_get_instance_index(object, instance_id);
	if (index < 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	s_mat4* entry = s_array_get(&object_ptr->instances.metadata, s_array_handle(&object_ptr->instances.metadata, (u32)index));
	if (!entry) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_metadata = *entry;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_object_3d_set_instances_dirty(const se_object_3d_handle object, const b8 dirty) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_set_instances_dirty :: object is null");
	if (object_ptr->is_custom) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&object_ptr->mesh_instances); ++i) {
		se_mesh_instance *mesh_instance = s_array_get(&object_ptr->mesh_instances, s_array_handle(&object_ptr->mesh_instances, (u32)i));
		mesh_instance->instance_buffers_dirty = dirty;
	}
}

b8 se_object_3d_are_instances_dirty(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_are_instances_dirty :: object is null");
	if (object_ptr->is_custom) {
		return false;
	}
	if (s_array_get_size(&object_ptr->mesh_instances) == 0) {
		return false;
	}
	se_mesh_instance *mesh_instance = s_array_get(&object_ptr->mesh_instances, s_array_handle(&object_ptr->mesh_instances, 0));
	return mesh_instance->instance_buffers_dirty;
}

sz se_object_3d_get_instance_count(const se_object_3d_handle object) {
	se_context *ctx = se_current_context();
	se_object_3d *object_ptr = se_object_3d_from_handle(ctx, object);
	s_assertf(object_ptr, "se_object_3d_get_instance_count :: object is null");
	return se_instances_active_count(&object_ptr->instances.actives);
}

s_json* se_object_3d_to_json(const se_object_3d_handle object) {
	se_context* ctx = se_current_context();
	if (!ctx || object == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_object_3d* object_ptr = se_object_3d_from_handle(ctx, object);
	if (!object_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}
	if (object_ptr->is_custom) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return NULL;
	}
	if (!se_model_handle_exists(ctx, object_ptr->model)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return NULL;
	}

	const sz slot_count = s_array_get_size(&object_ptr->instances.ids);
	if (slot_count != s_array_get_size(&object_ptr->instances.transforms) ||
		slot_count != s_array_get_size(&object_ptr->instances.buffers) ||
		slot_count != s_array_get_size(&object_ptr->instances.actives) ||
		slot_count != s_array_get_size(&object_ptr->instances.metadata)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	s_json* root = s_json_object_empty(NULL);
	if (!root) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	if (!se_scene_2d_json_add_string(root, "format", SE_OBJECT_3D_JSON_FORMAT) ||
		!se_scene_2d_json_add_u32(root, "version", SE_OBJECT_3D_JSON_VERSION) ||
		!se_scene_json_add_mat4(root, "transform", &object_ptr->transform) ||
		!se_scene_2d_json_add_bool(root, "visible", object_ptr->is_visible) ||
		!se_scene_2d_json_add_u32(root, "instance_capacity", (u32)s_array_get_capacity(&object_ptr->instances.ids)) ||
		!se_scene_2d_json_add_i32(root, "next_instance_id", object_ptr->instances.next_id)) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	s_json* instances_json = se_scene_2d_json_add_array(root, "instances");
	if (!instances_json) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	for (sz slot_index = 0; slot_index < slot_count; ++slot_index) {
		se_instance_id* id = s_array_get(&object_ptr->instances.ids, s_array_handle(&object_ptr->instances.ids, (u32)slot_index));
		s_mat4* transform = s_array_get(&object_ptr->instances.transforms, s_array_handle(&object_ptr->instances.transforms, (u32)slot_index));
		s_mat4* buffer = s_array_get(&object_ptr->instances.buffers, s_array_handle(&object_ptr->instances.buffers, (u32)slot_index));
		b8* active = s_array_get(&object_ptr->instances.actives, s_array_handle(&object_ptr->instances.actives, (u32)slot_index));
		s_mat4* metadata = s_array_get(&object_ptr->instances.metadata, s_array_handle(&object_ptr->instances.metadata, (u32)slot_index));
		if (!id || !transform || !buffer || !active || !metadata) {
			s_json_free(root);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return NULL;
		}

		s_json* slot_json = se_scene_2d_json_add_object(instances_json, NULL);
		if (!slot_json ||
			!se_scene_2d_json_add_i32(slot_json, "id", *id) ||
			!se_scene_2d_json_add_bool(slot_json, "active", *active) ||
				!se_scene_json_add_mat4(slot_json, "transform", transform) ||
				!se_scene_json_add_mat4(slot_json, "buffer", buffer) ||
				!se_scene_json_add_mat4(slot_json, "metadata", metadata)) {
			s_json_free(root);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}
	}

	se_set_last_error(SE_RESULT_OK);
	return root;
}

b8 se_object_3d_to_json_file(const se_object_3d_handle object, const c8* path) {
	if (!path || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_json* root = se_object_3d_to_json(object);
	if (!root) {
		return false;
	}

	const b8 ok = se_scene_json_write_file(path, root);
	s_json_free(root);
	return ok;
}

b8 se_object_3d_from_json(const se_object_3d_handle object, const s_json* root) {
	se_context* ctx = se_current_context();
	if (!ctx || object == S_HANDLE_NULL || !root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_object_3d* object_ptr = se_object_3d_from_handle(ctx, object);
	if (!object_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (object_ptr->is_custom) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}
	if (!se_model_handle_exists(ctx, object_ptr->model)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (root->type != S_JSON_OBJECT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const s_json* format_node = s_json_get(root, "format");
	const s_json* version_node = s_json_get(root, "version");
	if ((format_node && !version_node) || (!format_node && version_node)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (format_node && version_node) {
		const c8* format = NULL;
		u32 version = 0u;
		if (!se_scene_json_string_to_c8(se_scene_json_get_required(root, "format", S_JSON_STRING), &format) ||
			!se_scene_json_number_to_u32(se_scene_json_get_required(root, "version", S_JSON_NUMBER), &version)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		if (strcmp(format, SE_OBJECT_3D_JSON_FORMAT) != 0 || version != SE_OBJECT_3D_JSON_VERSION) {
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			return false;
		}
	}

	const s_json* transform_json = se_scene_json_get_required(root, "transform", S_JSON_ARRAY);
	const s_json* instances_json = se_scene_json_get_required(root, "instances", S_JSON_ARRAY);
	b8 visible = true;
	u32 instance_capacity = 0u;
	i32 next_instance_id = 0;
	s_mat4 object_transform = s_mat4_identity;
	if (!se_scene_json_bool_to_b8(se_scene_json_get_required(root, "visible", S_JSON_BOOL), &visible) ||
		!se_scene_json_number_to_u32(se_scene_json_get_required(root, "instance_capacity", S_JSON_NUMBER), &instance_capacity) ||
		!se_scene_json_number_to_i32(se_scene_json_get_required(root, "next_instance_id", S_JSON_NUMBER), &next_instance_id) ||
		!transform_json || !instances_json ||
		!se_scene_json_read_mat4(transform_json, &object_transform) ||
		next_instance_id < 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const sz slot_count = instances_json->as.children.count;
	const sz max_capacity = s_array_get_capacity(&object_ptr->instances.ids);
	const sz required_capacity = s_max((sz)s_max(instance_capacity, 1u), slot_count);
	if (max_capacity > 0 && required_capacity > max_capacity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	i32 max_instance_id = -1;
	for (sz slot_index = 0; slot_index < slot_count; ++slot_index) {
		const s_json* slot_json = s_json_at(instances_json, slot_index);
		i32 instance_id = 0;
		b8 active = true;
		s_mat4 instance_transform = s_mat4_identity;
		s_mat4 instance_buffer = s_mat4_identity;
		s_mat4 instance_metadata = s_mat4_identity;
		if (!se_object_3d_json_read_slot(
			slot_json,
			&instance_id,
			&active,
			&instance_transform,
			&instance_buffer,
			&instance_metadata)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		max_instance_id = s_max(max_instance_id, instance_id);
	}

	if (slot_count > 0 && next_instance_id <= max_instance_id) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	object_ptr->transform = object_transform;
	object_ptr->is_visible = visible;
	se_instance_ids_clear_keep_capacity(&object_ptr->instances.ids);
	se_transforms_clear_keep_capacity(&object_ptr->instances.transforms);
	se_buffers_clear_keep_capacity(&object_ptr->instances.buffers);
	se_instance_actives_clear_keep_capacity(&object_ptr->instances.actives);
	se_instance_ids_clear_keep_capacity(&object_ptr->instances.free_indices);
	se_buffers_clear_keep_capacity(&object_ptr->instances.metadata);
	se_transforms_clear_keep_capacity(&object_ptr->render_transforms);

	for (sz slot_index = 0; slot_index < slot_count; ++slot_index) {
		const s_json* slot_json = s_json_at(instances_json, slot_index);
		i32 instance_id = 0;
		b8 active = true;
		s_mat4 instance_transform = s_mat4_identity;
		s_mat4 instance_buffer = s_mat4_identity;
		s_mat4 instance_metadata = s_mat4_identity;
		if (!se_object_3d_json_read_slot(
			slot_json,
			&instance_id,
			&active,
			&instance_transform,
			&instance_buffer,
			&instance_metadata)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}

		se_instance_id instance_id_value = (se_instance_id)instance_id;
		s_array_add(&object_ptr->instances.ids, instance_id_value);
		s_array_add(&object_ptr->instances.transforms, instance_transform);
		s_array_add(&object_ptr->instances.buffers, instance_buffer);
		s_array_add(&object_ptr->instances.actives, active);
		s_array_add(&object_ptr->instances.metadata, instance_metadata);

		if (!active) {
			se_instances_push_free_index(&object_ptr->instances.free_indices, slot_index);
		}
	}

	object_ptr->instances.next_id = (se_instance_id)next_instance_id;
	se_object_3d_set_instances_dirty(object, true);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_object_3d_from_json_file(const se_object_3d_handle object, const c8* path) {
	s_json* root = NULL;
	if (!se_scene_json_read_file(path, &root)) {
		return false;
	}
	const b8 ok = se_object_3d_from_json(object, root);
	s_json_free(root);
	return ok;
}
