// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_defines.h"
#include "se_editor.h"
#include "se_graphics.h"
#include "se_sdf.h"
#include "se_text.h"
#include "se_window.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"

#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_SDF_PATH SE_RESOURCE_EXAMPLE("editor_sdf/scene.json")
#define EDITOR_SDF_FONT_SIZE 42.0f
#define EDITOR_SDF_TEXT_SCALE 0.70f
#define EDITOR_SDF_TEXT_NEWLINE_OFFSET 0.024f
#define EDITOR_SDF_TEXT_TOP_Y 0.90f
#define EDITOR_SDF_TEXT_BOTTOM_Y -0.90f

typedef enum {
	EDITOR_SDF_NODE = 1,
	EDITOR_SDF_NOISE,
	EDITOR_SDF_POINT_LIGHT,
	EDITOR_SDF_DIRECTIONAL_LIGHT
} editor_sdf_item_kind;

typedef enum {
	EDITOR_SDF_PANE_ITEMS = 0,
	EDITOR_SDF_PANE_PROPERTIES
} editor_sdf_pane;

typedef struct {
	editor_sdf_item_kind kind;
	se_sdf_handle sdf;
	se_sdf_noise_handle noise;
	se_sdf_point_light_handle point_light;
	se_sdf_directional_light_handle directional_light;
} editor_sdf_runtime_item;

typedef struct {
	se_context* context;
	se_window_handle window;
	se_camera_handle camera;
	se_sdf_handle scene;
	se_font_handle font;
	se_editor* handle;
	s_json* root;
	c8 json_path[SE_MAX_PATH_LENGTH];
	u32 selected_item;
	u32 selected_property;
	u32 selected_component;
	editor_sdf_pane active_pane;
	b8 editing;
	b8 moving;
	c8 edit_buffer[SE_MAX_PATH_LENGTH];
	c8 status[SE_MAX_PATH_LENGTH];
	c8 move_property_name[SE_MAX_NAME_LENGTH];
	c8 move_property_label[SE_MAX_NAME_LENGTH];
	se_editor_value move_original_value;
	s_vec3 camera_target;
	s_vec3 focus_target;
	s_handle focused_handle;
	f32 camera_yaw;
	f32 camera_pitch;
	f32 camera_distance;
	b8 focus_active;
	b8 focus_valid;
} sdf_editor;

static void editor_sdf_handle_shortcut(sdf_editor* editor, const se_editor_shortcut_event* event);
static f32 editor_sdf_json_round_f32(f32 value);

static f32 editor_sdf_text_line_step(const sdf_editor* editor) {
	u32 framebuffer_width = 0u;
	u32 framebuffer_height = 0u;
	f32 line_advance = 0.0f;
	f32 line_height = 0.0f;
	f32 line_step = 0.045f;
	if (!editor || editor->window == S_HANDLE_NULL) {
		return line_step;
	}
	se_window_get_framebuffer_size(editor->window, &framebuffer_width, &framebuffer_height);
	(void)framebuffer_width;
	if (framebuffer_height == 0u) {
		return line_step;
	}
	line_advance = se_font_get_line_advance(editor->font);
	line_height = se_font_get_line_height(editor->font);
	line_height = s_max(line_height, s_max(line_advance, EDITOR_SDF_FONT_SIZE));
	line_step = (line_height * EDITOR_SDF_TEXT_SCALE * (2.0f / (f32)framebuffer_height)) + EDITOR_SDF_TEXT_NEWLINE_OFFSET;
	return s_max(line_step, 0.045f);
}

static void editor_sdf_set_status(sdf_editor* editor, const c8* format, ...) {
	va_list args;
	if (!editor || !format) return;
	va_start(args, format);
	vsnprintf(editor->status, sizeof(editor->status), format, args);
	va_end(args);
}

static const c8* editor_sdf_json_string(const s_json* object, const c8* key, const c8* fallback) {
	const s_json* child = s_json_get(object, key);
	return child && child->type == S_JSON_STRING ? child->as.string : fallback;
}

static void editor_sdf_make_item_label(s_json* object, editor_sdf_item_kind kind, s_handle handle, c8* out, sz out_size) {
	if (!out || out_size == 0u) return;
	if (kind == EDITOR_SDF_NODE) {
		snprintf(out, out_size, "node[%llu] %s", (unsigned long long)handle, editor_sdf_json_string(object, "type", "custom"));
	} else if (kind == EDITOR_SDF_NOISE) {
		snprintf(out, out_size, "noise[%llu] %s", (unsigned long long)handle, editor_sdf_json_string(s_json_get(object, "descriptor"), "type", "noise"));
	} else if (kind == EDITOR_SDF_POINT_LIGHT) {
		snprintf(out, out_size, "light[%llu] point", (unsigned long long)handle);
	} else if (kind == EDITOR_SDF_DIRECTIONAL_LIGHT) {
		snprintf(out, out_size, "light[%llu] directional", (unsigned long long)handle);
	} else {
		snprintf(out, out_size, "sdf[%llu]", (unsigned long long)handle);
	}
}

static void editor_sdf_apply_camera(sdf_editor* editor) {
	if (!editor || editor->camera == S_HANDLE_NULL) return;
	const s_vec3 rotation = s_vec3(editor->camera_pitch, editor->camera_yaw, 0.0f);
	se_camera_set_rotation(editor->camera, &rotation);
	{
		const s_vec3 forward = se_camera_get_forward_vector(editor->camera);
		const s_vec3 position = s_vec3_sub(&editor->camera_target, &s_vec3_muls(&forward, editor->camera_distance));
		se_camera_set_location(editor->camera, &position);
	}
	se_camera_set_target(editor->camera, &editor->camera_target);
	se_camera_set_window_aspect(editor->camera, editor->window);
}

static b8 editor_sdf_json_number(const s_json* node, f32* out) {
	if (!node || node->type != S_JSON_NUMBER || !out) return false;
	*out = editor_sdf_json_round_f32((f32)node->as.number);
	return true;
}

static b8 editor_sdf_json_vec2(const s_json* node, s_vec2* out) {
	if (!node || node->type != S_JSON_ARRAY || node->as.children.count != 2u || !out) return false;
	return editor_sdf_json_number(s_json_at(node, 0u), &out->x) &&
		editor_sdf_json_number(s_json_at(node, 1u), &out->y);
}

static b8 editor_sdf_json_vec3(const s_json* node, s_vec3* out) {
	if (!node || node->type != S_JSON_ARRAY || node->as.children.count != 3u || !out) return false;
	return editor_sdf_json_number(s_json_at(node, 0u), &out->x) &&
		editor_sdf_json_number(s_json_at(node, 1u), &out->y) &&
		editor_sdf_json_number(s_json_at(node, 2u), &out->z);
}

static b8 editor_sdf_json_mat4(const s_json* node, s_mat4* out) {
	if (!node || node->type != S_JSON_ARRAY || node->as.children.count != 16u || !out) return false;
	u32 index = 0u;
	*out = s_mat4_identity;
	for (u32 row = 0u; row < 4u; ++row) {
		for (u32 col = 0u; col < 4u; ++col) {
			if (!editor_sdf_json_number(s_json_at(node, index++), &out->m[row][col])) return false;
		}
	}
	return true;
}

static s_vec3 editor_sdf_mat4_point(const s_mat4* matrix, const s_vec3* point) {
	if (!matrix || !point) return s_vec3(0.0f, 0.0f, 0.0f);
	return s_vec3(
		matrix->m[0][0] * point->x + matrix->m[1][0] * point->y + matrix->m[2][0] * point->z + matrix->m[3][0],
		matrix->m[0][1] * point->x + matrix->m[1][1] * point->y + matrix->m[2][1] * point->z + matrix->m[3][1],
		matrix->m[0][2] * point->x + matrix->m[1][2] * point->y + matrix->m[2][2] * point->z + matrix->m[3][2]);
}

static const c8* editor_sdf_property_label(const se_editor_property* property) {
	if (!property) return "";
	return property->label[0] ? property->label : property->name;
}

static f32 editor_sdf_degrees_to_radians(const f32 degrees) {
	return degrees * ((f32)PI / 180.0f);
}

static f32 editor_sdf_radians_to_degrees(const f32 radians) {
	return radians * (180.0f / (f32)PI);
}

static s_vec3 editor_sdf_mat4_extract_scale(const s_mat4* transform) {
	if (!transform) {
		return s_vec3(1.0f, 1.0f, 1.0f);
	}
	return s_vec3(
		sqrtf((transform->m[0][0] * transform->m[0][0]) + (transform->m[0][1] * transform->m[0][1]) + (transform->m[0][2] * transform->m[0][2])),
		sqrtf((transform->m[1][0] * transform->m[1][0]) + (transform->m[1][1] * transform->m[1][1]) + (transform->m[1][2] * transform->m[1][2])),
		sqrtf((transform->m[2][0] * transform->m[2][0]) + (transform->m[2][1] * transform->m[2][1]) + (transform->m[2][2] * transform->m[2][2])));
}

static s_mat4 editor_sdf_mat4_extract_rotation_matrix(const s_mat4* transform, const s_vec3* scale) {
	const f32 epsilon = 0.000001f;
	s_mat4 rotation = s_mat4_identity;
	if (!transform || !scale) {
		return rotation;
	}
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

static s_mat4 editor_sdf_mat4_rotation_xyz(const s_vec3* rotation) {
	const f32 cx = rotation ? cosf(rotation->x) : 1.0f;
	const f32 sx = rotation ? sinf(rotation->x) : 0.0f;
	const f32 cy = rotation ? cosf(rotation->y) : 1.0f;
	const f32 sy = rotation ? sinf(rotation->y) : 0.0f;
	const f32 cz = rotation ? cosf(rotation->z) : 1.0f;
	const f32 sz = rotation ? sinf(rotation->z) : 0.0f;
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

static void editor_sdf_mat4_apply_scale(s_mat4* transform, const s_vec3* scale) {
	if (!transform || !scale) return;
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

static s_vec3 editor_sdf_mat4_extract_rotation_degrees(const s_mat4* transform) {
	const s_vec3 scale = editor_sdf_mat4_extract_scale(transform);
	const s_mat4 rotation = editor_sdf_mat4_extract_rotation_matrix(transform, &scale);
	const f32 sy = s_max(-1.0f, s_min(1.0f, rotation.m[2][0]));
	const f32 y = asinf(sy);
	const f32 cy = cosf(y);
	f32 x = 0.0f;
	f32 z = 0.0f;
	if (fabsf(cy) > 0.0001f) {
		x = atan2f(-rotation.m[2][1], rotation.m[2][2]);
		z = atan2f(-rotation.m[1][0], rotation.m[0][0]);
	} else {
		x = atan2f(rotation.m[0][1], rotation.m[1][1]);
	}
	return s_vec3(
		editor_sdf_radians_to_degrees(x),
		editor_sdf_radians_to_degrees(y),
		editor_sdf_radians_to_degrees(z));
}

static void editor_sdf_transform_decompose(const s_mat4* transform, s_vec3* out_position, s_vec3* out_rotation_degrees, s_vec3* out_scale) {
	if (out_position) *out_position = transform ? s_mat4_get_translation(transform) : s_vec3(0.0f, 0.0f, 0.0f);
	if (out_rotation_degrees) *out_rotation_degrees = editor_sdf_mat4_extract_rotation_degrees(transform);
	if (out_scale) *out_scale = editor_sdf_mat4_extract_scale(transform);
}

static s_mat4 editor_sdf_transform_compose(const s_vec3* position, const s_vec3* rotation_degrees, const s_vec3* scale) {
	const s_vec3 rotation_radians = rotation_degrees
		? s_vec3(
			editor_sdf_degrees_to_radians(rotation_degrees->x),
			editor_sdf_degrees_to_radians(rotation_degrees->y),
			editor_sdf_degrees_to_radians(rotation_degrees->z))
		: s_vec3(0.0f, 0.0f, 0.0f);
	const s_vec3 safe_scale = scale ? *scale : s_vec3(1.0f, 1.0f, 1.0f);
	const s_vec3 safe_position = position ? *position : s_vec3(0.0f, 0.0f, 0.0f);
	s_mat4 transform = editor_sdf_mat4_rotation_xyz(&rotation_radians);
	editor_sdf_mat4_apply_scale(&transform, &safe_scale);
	s_mat4_set_translation(&transform, &safe_position);
	return transform;
}

static b8 editor_sdf_json_replace(s_json* object, const c8* key, s_json* value) {
	if (!object || object->type != S_JSON_OBJECT || !key || !value) return false;
	s_json_set_name(value, key);
	for (sz i = 0u; i < object->as.children.count; ++i) {
		s_json* child = object->as.children.items[i];
		if (child && child->name && strcmp(child->name, key) == 0) {
			s_json_free(child);
			object->as.children.items[i] = value;
			return true;
		}
	}
	return s_json_add(object, value);
}

static f32 editor_sdf_json_round_f32(const f32 value) {
	const f32 scale = 1000.0f;
	if (!isfinite(value)) {
		return value;
	}
	const f32 rounded = roundf(value * scale) / scale;
	return fabsf(rounded) < (0.5f / scale) ? 0.0f : rounded;
}

static void editor_sdf_json_remove(s_json* object, const c8* key) {
	if (!object || object->type != S_JSON_OBJECT || !key) return;
	for (sz i = 0u; i < object->as.children.count; ++i) {
		s_json* child = object->as.children.items[i];
		if (!child || !child->name || strcmp(child->name, key) != 0) continue;
		s_json_free(child);
		for (sz j = i + 1u; j < object->as.children.count; ++j) {
			object->as.children.items[j - 1u] = object->as.children.items[j];
		}
		object->as.children.count--;
		return;
	}
}

static b8 editor_sdf_set_number(s_json* object, const c8* key, f32 value) {
	return editor_sdf_json_replace(object, key, s_json_num(NULL, editor_sdf_json_round_f32(value)));
}

static b8 editor_sdf_set_string(s_json* object, const c8* key, const c8* value) {
	return editor_sdf_json_replace(object, key, s_json_str(NULL, value ? value : ""));
}

static b8 editor_sdf_set_vec2(s_json* object, const c8* key, s_vec2 value) {
	s_json* array = s_json_array_empty(NULL);
	if (!array) return false;
	if (!s_json_add(array, s_json_num(NULL, editor_sdf_json_round_f32(value.x))) ||
		!s_json_add(array, s_json_num(NULL, editor_sdf_json_round_f32(value.y))) ||
		!editor_sdf_json_replace(object, key, array)) {
		s_json_free(array);
		return false;
	}
	return true;
}

static b8 editor_sdf_set_vec3(s_json* object, const c8* key, s_vec3 value) {
	s_json* array = s_json_array_empty(NULL);
	if (!array) return false;
	if (!s_json_add(array, s_json_num(NULL, editor_sdf_json_round_f32(value.x))) ||
		!s_json_add(array, s_json_num(NULL, editor_sdf_json_round_f32(value.y))) ||
		!s_json_add(array, s_json_num(NULL, editor_sdf_json_round_f32(value.z))) ||
		!editor_sdf_json_replace(object, key, array)) {
		s_json_free(array);
		return false;
	}
	return true;
}

static b8 editor_sdf_set_mat4(s_json* object, const c8* key, const s_mat4* value) {
	if (!value) return false;
	s_json* array = s_json_array_empty(NULL);
	if (!array) return false;
	for (u32 row = 0u; row < 4u; ++row) {
		for (u32 col = 0u; col < 4u; ++col) {
			if (!s_json_add(array, s_json_num(NULL, editor_sdf_json_round_f32(value->m[row][col])))) {
				s_json_free(array);
				return false;
			}
		}
	}
	if (!editor_sdf_json_replace(object, key, array)) {
		s_json_free(array);
		return false;
	}
	return true;
}

static b8 editor_sdf_ensure_shape_defaults(s_json* object, const c8* type) {
	if (!object || !type) return false;
	if (strcmp(type, "sphere") == 0 && !s_json_get(object, "sphere")) {
		s_json* sphere = s_json_object(NULL, s_json_num("radius", 1.0));
		if (!sphere) return false;
		if (!editor_sdf_json_replace(object, "sphere", sphere)) {
			s_json_free(sphere);
			return false;
		}
	}
	if (strcmp(type, "box") == 0 && !s_json_get(object, "box")) {
		s_json* box = s_json_object(NULL, s_json_array("size", s_json_num(NULL, 1.0), s_json_num(NULL, 1.0), s_json_num(NULL, 1.0)));
		if (!box) return false;
		if (!editor_sdf_json_replace(object, "box", box)) {
			s_json_free(box);
			return false;
		}
	}
	return true;
}

static void editor_sdf_add_property_value(se_editor* handle, const c8* name, const c8* label, se_editor_value value, b8 editable, se_editor_property_role role, f32 step, f32 large_step) {
	se_editor_property_desc desc = SE_EDITOR_PROPERTY_DESC_DEFAULTS;
	desc.name = name;
	desc.label = label;
	desc.value = value;
	desc.role = role;
	desc.step = step;
	desc.large_step = large_step;
	desc.editable = editable;
	(void)se_editor_add_property(handle, &desc);
}

static void editor_sdf_json_transform_values(s_json* object, s_mat4* out_matrix, s_vec3* out_position, s_vec3* out_rotation_degrees, s_vec3* out_scale) {
	s_mat4 transform = s_mat4_identity;
	if (!object) {
		if (out_matrix) *out_matrix = transform;
		if (out_position) *out_position = s_vec3(0.0f, 0.0f, 0.0f);
		if (out_rotation_degrees) *out_rotation_degrees = s_vec3(0.0f, 0.0f, 0.0f);
		if (out_scale) *out_scale = s_vec3(1.0f, 1.0f, 1.0f);
		return;
	}
	(void)editor_sdf_json_mat4(s_json_get(object, "transform"), &transform);
	if (out_matrix) *out_matrix = transform;
	editor_sdf_transform_decompose(&transform, out_position, out_rotation_degrees, out_scale);
}

static b8 editor_sdf_set_transform_values(s_json* object, const s_vec3* position, const s_vec3* rotation_degrees, const s_vec3* scale) {
	const s_mat4 transform = editor_sdf_transform_compose(position, rotation_degrees, scale);
	return editor_sdf_set_mat4(object, "transform", &transform);
}

static b8 editor_sdf_add_item_for_json(se_editor* handle, s_json* object, editor_sdf_item_kind kind, s_handle owner, u32* id) {
	c8 label[SE_MAX_NAME_LENGTH] = {0};
	const s_handle item_handle = (s_handle)(*id);
	editor_sdf_make_item_label(object, kind, item_handle, label, sizeof(label));
	(*id)++;
	return se_editor_add_item(handle, SE_EDITOR_CATEGORY_ITEM, item_handle, owner, object, NULL, (u32)kind, label);
}

static b8 editor_sdf_collect_node(se_editor* handle, s_json* node, s_handle parent, u32* id) {
	if (!node || node->type != S_JSON_OBJECT) return true;
	const s_handle node_handle = (s_handle)(*id);
	if (!editor_sdf_add_item_for_json(handle, node, EDITOR_SDF_NODE, parent, id)) return false;
	s_json* noises = s_json_get(node, "noises");
	for (sz i = 0u; noises && noises->type == S_JSON_ARRAY && i < noises->as.children.count; ++i) {
		if (!editor_sdf_add_item_for_json(handle, s_json_at(noises, i), EDITOR_SDF_NOISE, node_handle, id)) return false;
	}
	s_json* points = s_json_get(node, "point_lights");
	for (sz i = 0u; points && points->type == S_JSON_ARRAY && i < points->as.children.count; ++i) {
		if (!editor_sdf_add_item_for_json(handle, s_json_at(points, i), EDITOR_SDF_POINT_LIGHT, node_handle, id)) return false;
	}
	s_json* dirs = s_json_get(node, "directional_lights");
	for (sz i = 0u; dirs && dirs->type == S_JSON_ARRAY && i < dirs->as.children.count; ++i) {
		if (!editor_sdf_add_item_for_json(handle, s_json_at(dirs, i), EDITOR_SDF_DIRECTIONAL_LIGHT, node_handle, id)) return false;
	}
	s_json* children = s_json_get(node, "children");
	for (sz i = 0u; children && children->type == S_JSON_ARRAY && i < children->as.children.count; ++i) {
		if (!editor_sdf_collect_node(handle, s_json_at(children, i), node_handle, id)) return false;
	}
	return true;
}

static s_json* editor_sdf_find_node(s_json* node, s_handle target, u32* id, editor_sdf_item_kind* out_kind) {
	if (!node || node->type != S_JSON_OBJECT) return NULL;
	if ((s_handle)(*id) == target) {
		*out_kind = EDITOR_SDF_NODE;
		return node;
	}
	(*id)++;
	const c8* arrays[] = { "noises", "point_lights", "directional_lights" };
	const editor_sdf_item_kind kinds[] = { EDITOR_SDF_NOISE, EDITOR_SDF_POINT_LIGHT, EDITOR_SDF_DIRECTIONAL_LIGHT };
	for (u32 a = 0u; a < 3u; ++a) {
		s_json* array = s_json_get(node, arrays[a]);
		for (sz i = 0u; array && array->type == S_JSON_ARRAY && i < array->as.children.count; ++i) {
			if ((s_handle)(*id) == target) {
				*out_kind = kinds[a];
				return s_json_at(array, i);
			}
			(*id)++;
		}
	}
	s_json* children = s_json_get(node, "children");
	for (sz i = 0u; children && children->type == S_JSON_ARRAY && i < children->as.children.count; ++i) {
		s_json* found = editor_sdf_find_node(s_json_at(children, i), target, id, out_kind);
		if (found) return found;
	}
	return NULL;
}

static s_json* editor_sdf_item_json(sdf_editor* editor, const se_editor_item* item, editor_sdf_item_kind* out_kind) {
	u32 id = 1u;
	editor_sdf_item_kind kind = 0;
	s_json* found = editor_sdf_find_node(editor ? editor->root : NULL, item ? item->handle : S_HANDLE_NULL, &id, &kind);
	if (out_kind) *out_kind = kind;
	return found;
}

static b8 editor_sdf_find_runtime_item_recursive(se_sdf_handle sdf, s_handle target, u32* id, editor_sdf_runtime_item* out_item) {
	const se_sdf_noise_handle* noises = NULL;
	const se_sdf_point_light_handle* point_lights = NULL;
	const se_sdf_directional_light_handle* directional_lights = NULL;
	const se_sdf_handle* children = NULL;
	sz noise_count = 0u;
	sz point_light_count = 0u;
	sz directional_light_count = 0u;
	sz child_count = 0u;
	if (!id || !out_item || sdf == SE_SDF_NULL) return false;
	if ((s_handle)(*id) == target) {
		*out_item = (editor_sdf_runtime_item){
			.kind = EDITOR_SDF_NODE,
			.sdf = sdf
		};
		return true;
	}
	(*id)++;
	if (!se_sdf_get_noises(sdf, &noises, &noise_count) ||
		!se_sdf_get_point_lights(sdf, &point_lights, &point_light_count) ||
		!se_sdf_get_directional_lights(sdf, &directional_lights, &directional_light_count) ||
		!se_sdf_get_children(sdf, &children, &child_count)) {
		return false;
	}
	for (sz i = 0u; i < noise_count; ++i) {
		if ((s_handle)(*id) == target) {
			*out_item = (editor_sdf_runtime_item){
				.kind = EDITOR_SDF_NOISE,
				.sdf = sdf,
				.noise = noises[i]
			};
			return true;
		}
		(*id)++;
	}
	for (sz i = 0u; i < point_light_count; ++i) {
		if ((s_handle)(*id) == target) {
			*out_item = (editor_sdf_runtime_item){
				.kind = EDITOR_SDF_POINT_LIGHT,
				.sdf = sdf,
				.point_light = point_lights[i]
			};
			return true;
		}
		(*id)++;
	}
	for (sz i = 0u; i < directional_light_count; ++i) {
		if ((s_handle)(*id) == target) {
			*out_item = (editor_sdf_runtime_item){
				.kind = EDITOR_SDF_DIRECTIONAL_LIGHT,
				.sdf = sdf,
				.directional_light = directional_lights[i]
			};
			return true;
		}
		(*id)++;
	}
	for (sz i = 0u; i < child_count; ++i) {
		if (editor_sdf_find_runtime_item_recursive(children[i], target, id, out_item)) return true;
	}
	return false;
}

static b8 editor_sdf_item_runtime(sdf_editor* editor, const se_editor_item* item, editor_sdf_runtime_item* out_item) {
	u32 id = 1u;
	if (!editor || !item || !out_item || editor->scene == SE_SDF_NULL) return false;
	memset(out_item, 0, sizeof(*out_item));
	return editor_sdf_find_runtime_item_recursive(editor->scene, item->handle, &id, out_item);
}

static b8 editor_sdf_apply_json(sdf_editor* editor) {
	return editor && editor->scene != SE_SDF_NULL && editor->root && se_sdf_from_json(editor->scene, editor->root);
}

static se_shader_handle editor_sdf_scene_shader(const sdf_editor* editor) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = NULL;
	if (!editor || !ctx || editor->scene == SE_SDF_NULL) return S_HANDLE_NULL;
	sdf_ptr = s_array_get(&ctx->sdfs, editor->scene);
	return sdf_ptr ? sdf_ptr->shader : S_HANDLE_NULL;
}

static b8 editor_sdf_collect_items(se_editor* handle, void* user_data) {
	sdf_editor* editor = user_data;
	u32 id = 1u;
	return editor && editor->root && editor_sdf_collect_node(handle, editor->root, S_HANDLE_NULL, &id);
}

static b8 editor_sdf_collect_properties(se_editor* handle, const se_editor_item* item, void* user_data) {
	sdf_editor* editor = user_data;
	editor_sdf_item_kind kind = 0;
	s_json* object = editor_sdf_item_json(editor, item, &kind);
	if (!object) return false;
	editor_sdf_add_property_value(handle, "kind", "kind", se_editor_value_uint((u32)kind), false, SE_EDITOR_PROPERTY_ROLE_NONE, 0.0f, 0.0f);
	editor_sdf_add_property_value(handle, "json_path", "json path", se_editor_value_text(editor->json_path), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.0f, 0.0f);
	if (kind == EDITOR_SDF_NODE) {
		s_mat4 transform = s_mat4_identity;
		s_vec3 position = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 rotation = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 scale = s_vec3(1.0f, 1.0f, 1.0f);
		s_json* type = s_json_get(object, "type");
		s_json* operation = s_json_get(object, "operation");
		const c8* type_name = type && type->type == S_JSON_STRING ? type->as.string : "custom";
		editor_sdf_json_transform_values(object, &transform, &position, &rotation, &scale);
		editor_sdf_add_property_value(handle, "transform.position", "position", se_editor_value_vec3(position), true, SE_EDITOR_PROPERTY_ROLE_POSITION, 0.1f, 1.0f);
		editor_sdf_add_property_value(handle, "transform.rotation", "rotation", se_editor_value_vec3(rotation), true, SE_EDITOR_PROPERTY_ROLE_ROTATION, 5.0f, 15.0f);
		editor_sdf_add_property_value(handle, "transform.scale", "scale", se_editor_value_vec3(scale), true, SE_EDITOR_PROPERTY_ROLE_SCALE, 0.1f, 0.5f);
		editor_sdf_add_property_value(handle, "transform.matrix", "matrix", se_editor_value_mat4(transform), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.0f, 0.0f);
		editor_sdf_add_property_value(handle, "type", "type", se_editor_value_text(type_name), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.0f, 0.0f);
		editor_sdf_add_property_value(handle, "operation", "operation", se_editor_value_text(operation && operation->type == S_JSON_STRING ? operation->as.string : "union"), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.0f, 0.0f);
		f32 amount = 0.0f;
		if (editor_sdf_json_number(s_json_get(object, "operation_amount"), &amount)) editor_sdf_add_property_value(handle, "operation_amount", "operation amount", se_editor_value_float(amount), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.1f, 1.0f);
		s_vec3 size = s_vec3(0.0f, 0.0f, 0.0f);
		f32 radius = 0.0f;
		if (strcmp(type_name, "box") == 0 && editor_sdf_json_vec3(s_json_get(s_json_get(object, "box"), "size"), &size)) {
			editor_sdf_add_property_value(handle, "size", "size", se_editor_value_vec3(size), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.1f, 1.0f);
		}
		if (strcmp(type_name, "sphere") == 0 && editor_sdf_json_number(s_json_get(s_json_get(object, "sphere"), "radius"), &radius)) {
			editor_sdf_add_property_value(handle, "radius", "radius", se_editor_value_float(radius), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.1f, 1.0f);
		}
		if (editor_sdf_json_vec3(s_json_get(s_json_get(object, "shading"), "ambient"), &size)) editor_sdf_add_property_value(handle, "shading.ambient", "ambient", se_editor_value_vec3(size), true, SE_EDITOR_PROPERTY_ROLE_COLOR, 0.02f, 0.10f);
		if (editor_sdf_json_vec3(s_json_get(s_json_get(object, "shading"), "diffuse"), &size)) editor_sdf_add_property_value(handle, "shading.diffuse", "diffuse", se_editor_value_vec3(size), true, SE_EDITOR_PROPERTY_ROLE_COLOR, 0.02f, 0.10f);
		if (editor_sdf_json_vec3(s_json_get(s_json_get(object, "shading"), "specular"), &size)) editor_sdf_add_property_value(handle, "shading.specular", "specular", se_editor_value_vec3(size), true, SE_EDITOR_PROPERTY_ROLE_COLOR, 0.02f, 0.10f);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shading"), "roughness"), &radius)) editor_sdf_add_property_value(handle, "shading.roughness", "roughness", se_editor_value_float(radius), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.02f, 0.10f);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shading"), "bias"), &radius)) editor_sdf_add_property_value(handle, "shading.bias", "bias", se_editor_value_float(radius), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.01f, 0.05f);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shading"), "smoothness"), &radius)) editor_sdf_add_property_value(handle, "shading.smoothness", "smoothness", se_editor_value_float(radius), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.02f, 0.10f);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shadow"), "softness"), &radius)) editor_sdf_add_property_value(handle, "shadow.softness", "softness", se_editor_value_float(radius), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.25f, 1.0f);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shadow"), "bias"), &radius)) editor_sdf_add_property_value(handle, "shadow.bias", "bias", se_editor_value_float(radius), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.005f, 0.02f);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shadow"), "samples"), &radius)) editor_sdf_add_property_value(handle, "shadow.samples", "samples", se_editor_value_uint((u32)radius), true, SE_EDITOR_PROPERTY_ROLE_NONE, 1.0f, 8.0f);
		editor_sdf_add_property_value(handle, "child_count", "child count", se_editor_value_u64(s_json_get(object, "children") ? s_json_get(object, "children")->as.children.count : 0u), false, SE_EDITOR_PROPERTY_ROLE_NONE, 0.0f, 0.0f);
		return true;
	}
	if (kind == EDITOR_SDF_NOISE) {
		s_json* descriptor = s_json_get(object, "descriptor");
		s_json* type = s_json_get(descriptor, "type");
		s_vec2 value = s_vec2(0.0f, 0.0f);
		f32 number = 0.0f;
		editor_sdf_add_property_value(handle, "type", "type", se_editor_value_text(type && type->type == S_JSON_STRING ? type->as.string : ""), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.0f, 0.0f);
		if (editor_sdf_json_number(s_json_get(descriptor, "frequency"), &number)) editor_sdf_add_property_value(handle, "noise.frequency", "frequency", se_editor_value_float(number), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.1f, 1.0f);
		if (editor_sdf_json_vec2(s_json_get(descriptor, "offset"), &value)) editor_sdf_add_property_value(handle, "noise.offset", "offset", se_editor_value_vec2(value), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.1f, 1.0f);
		if (editor_sdf_json_vec2(s_json_get(descriptor, "scale"), &value)) editor_sdf_add_property_value(handle, "noise.scale", "scale", se_editor_value_vec2(value), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.1f, 1.0f);
		if (editor_sdf_json_number(s_json_get(descriptor, "seed"), &number)) editor_sdf_add_property_value(handle, "seed", "seed", se_editor_value_uint((u32)number), true, SE_EDITOR_PROPERTY_ROLE_NONE, 1.0f, 10.0f);
		return true;
	}
	if (kind == EDITOR_SDF_POINT_LIGHT) {
		s_vec3 value = s_vec3(0.0f, 0.0f, 0.0f);
		f32 radius = 0.0f;
		if (editor_sdf_json_vec3(s_json_get(object, "position"), &value)) editor_sdf_add_property_value(handle, "position", "position", se_editor_value_vec3(value), true, SE_EDITOR_PROPERTY_ROLE_POSITION, 0.1f, 1.0f);
		if (editor_sdf_json_vec3(s_json_get(object, "color"), &value)) editor_sdf_add_property_value(handle, "color", "color", se_editor_value_vec3(value), true, SE_EDITOR_PROPERTY_ROLE_COLOR, 0.02f, 0.10f);
		if (editor_sdf_json_number(s_json_get(object, "radius"), &radius)) editor_sdf_add_property_value(handle, "radius", "radius", se_editor_value_float(radius), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.1f, 1.0f);
		return true;
	}
	if (kind == EDITOR_SDF_DIRECTIONAL_LIGHT) {
		s_vec3 value = s_vec3(0.0f, 0.0f, 0.0f);
		if (editor_sdf_json_vec3(s_json_get(object, "direction"), &value)) editor_sdf_add_property_value(handle, "direction", "direction", se_editor_value_vec3(value), true, SE_EDITOR_PROPERTY_ROLE_NONE, 0.05f, 0.25f);
		if (editor_sdf_json_vec3(s_json_get(object, "color"), &value)) editor_sdf_add_property_value(handle, "color", "color", se_editor_value_vec3(value), true, SE_EDITOR_PROPERTY_ROLE_COLOR, 0.02f, 0.10f);
		return true;
	}
	return false;
}

static b8 editor_sdf_load_file(sdf_editor* editor, const c8* path) {
	s_json* root = NULL;
	c8 file_path[SE_MAX_PATH_LENGTH] = {0};
	if (!editor || !se_paths_resolve_resource_path(file_path, sizeof(file_path), path) || !se_editor_json_read_file(file_path, &root)) return false;
	if (!se_sdf_from_json(editor->scene, root)) {
		s_json_free(root);
		return false;
	}
	s_json_free(editor->root);
	editor->root = root;
	snprintf(editor->json_path, sizeof(editor->json_path), "%s", path);
	return true;
}

static b8 editor_sdf_save_file(sdf_editor* editor, const c8* path) {
	s_json* fresh = NULL;
	c8* text = NULL;
	c8 file_path[SE_MAX_PATH_LENGTH] = {0};
	if (!editor || !se_paths_resolve_resource_path(file_path, sizeof(file_path), path)) return false;
	fresh = se_sdf_to_json(editor->scene);
	if (!fresh) return false;
	text = s_json_stringify_precision(fresh, 3);
	if (!text || !s_file_write(file_path, text, strlen(text))) {
		free(text);
		s_json_free(fresh);
		return false;
	}
	free(text);
	s_json_free(editor->root);
	editor->root = fresh;
	snprintf(editor->json_path, sizeof(editor->json_path), "%s", path);
	return true;
}

static void editor_sdf_value_text(const se_editor_value* value, c8* out, sz out_size, b8 edit_text) {
	if (!out || out_size == 0u) return;
	out[0] = '\0';
	if (!value) return;
	switch (value->type) {
		case SE_EDITOR_VALUE_BOOL:
			snprintf(out, out_size, "%s", value->bool_value ? "true" : "false");
			break;
		case SE_EDITOR_VALUE_INT:
			snprintf(out, out_size, "%d", value->int_value);
			break;
		case SE_EDITOR_VALUE_UINT:
			snprintf(out, out_size, "%u", value->uint_value);
			break;
		case SE_EDITOR_VALUE_U64:
			snprintf(out, out_size, "%llu", (unsigned long long)value->u64_value);
			break;
		case SE_EDITOR_VALUE_FLOAT:
			snprintf(out, out_size, "%.3f", editor_sdf_json_round_f32(value->float_value));
			break;
		case SE_EDITOR_VALUE_DOUBLE:
			snprintf(out, out_size, "%.3f", (f64)editor_sdf_json_round_f32((f32)value->double_value));
			break;
		case SE_EDITOR_VALUE_VEC2:
			snprintf(out, out_size, "%.3f %.3f",
				editor_sdf_json_round_f32(value->vec2_value.x),
				editor_sdf_json_round_f32(value->vec2_value.y));
			break;
		case SE_EDITOR_VALUE_VEC3:
			snprintf(out, out_size, "%.3f %.3f %.3f",
				editor_sdf_json_round_f32(value->vec3_value.x),
				editor_sdf_json_round_f32(value->vec3_value.y),
				editor_sdf_json_round_f32(value->vec3_value.z));
			break;
		case SE_EDITOR_VALUE_VEC4:
			snprintf(out, out_size, "%.3f %.3f %.3f %.3f",
				editor_sdf_json_round_f32(value->vec4_value.x),
				editor_sdf_json_round_f32(value->vec4_value.y),
				editor_sdf_json_round_f32(value->vec4_value.z),
				editor_sdf_json_round_f32(value->vec4_value.w));
			break;
		case SE_EDITOR_VALUE_MAT4:
			if (edit_text) {
				snprintf(out, out_size,
					"%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f",
					editor_sdf_json_round_f32(value->mat4_value.m[0][0]), editor_sdf_json_round_f32(value->mat4_value.m[0][1]), editor_sdf_json_round_f32(value->mat4_value.m[0][2]), editor_sdf_json_round_f32(value->mat4_value.m[0][3]),
					editor_sdf_json_round_f32(value->mat4_value.m[1][0]), editor_sdf_json_round_f32(value->mat4_value.m[1][1]), editor_sdf_json_round_f32(value->mat4_value.m[1][2]), editor_sdf_json_round_f32(value->mat4_value.m[1][3]),
					editor_sdf_json_round_f32(value->mat4_value.m[2][0]), editor_sdf_json_round_f32(value->mat4_value.m[2][1]), editor_sdf_json_round_f32(value->mat4_value.m[2][2]), editor_sdf_json_round_f32(value->mat4_value.m[2][3]),
					editor_sdf_json_round_f32(value->mat4_value.m[3][0]), editor_sdf_json_round_f32(value->mat4_value.m[3][1]), editor_sdf_json_round_f32(value->mat4_value.m[3][2]), editor_sdf_json_round_f32(value->mat4_value.m[3][3]));
			} else {
				const s_vec3 position = s_mat4_get_translation(&value->mat4_value);
				snprintf(out, out_size, "pos %.3f %.3f %.3f", position.x, position.y, position.z);
			}
			break;
		case SE_EDITOR_VALUE_HANDLE:
			snprintf(out, out_size, "%llu", (unsigned long long)value->handle_value);
			break;
		case SE_EDITOR_VALUE_POINTER:
			snprintf(out, out_size, "%p", value->pointer_value);
			break;
		case SE_EDITOR_VALUE_TEXT:
			snprintf(out, out_size, "%s", value->text_value);
			break;
		default:
			snprintf(out, out_size, "<none>");
			break;
	}
}

static void editor_sdf_skip_number_separators(const c8** cursor) {
	while (cursor && *cursor && (**cursor == ' ' || **cursor == '\t' || **cursor == ',' ||
		**cursor == '(' || **cursor == ')' || **cursor == '[' || **cursor == ']' ||
		**cursor == '=' || **cursor == ':')) {
		(*cursor)++;
	}
}

static b8 editor_sdf_parse_floats(const c8* text, f32* values, u32 count) {
	const c8* cursor = text;
	if (!cursor || !values) return false;
	for (u32 i = 0u; i < count; ++i) {
		c8* end = NULL;
		editor_sdf_skip_number_separators(&cursor);
		values[i] = strtof(cursor, &end);
		if (end == cursor) return false;
		cursor = end;
	}
	return true;
}

static b8 editor_sdf_parse_value(se_editor_value_type type, const c8* text, se_editor_value* out) {
	c8* end = NULL;
	f32 values[16] = {0};
	if (!text || !out) return false;
	switch (type) {
		case SE_EDITOR_VALUE_BOOL:
			if (strcmp(text, "true") == 0 || strcmp(text, "1") == 0) {
				*out = se_editor_value_bool(true);
				return true;
			}
			if (strcmp(text, "false") == 0 || strcmp(text, "0") == 0) {
				*out = se_editor_value_bool(false);
				return true;
			}
			return false;
		case SE_EDITOR_VALUE_INT:
			*out = se_editor_value_int((i32)strtol(text, &end, 10));
			return end != text;
		case SE_EDITOR_VALUE_UINT:
			*out = se_editor_value_uint((u32)strtoul(text, &end, 10));
			return end != text;
		case SE_EDITOR_VALUE_U64:
			*out = se_editor_value_u64((u64)strtoull(text, &end, 10));
			return end != text;
		case SE_EDITOR_VALUE_FLOAT:
			*out = se_editor_value_float(editor_sdf_json_round_f32(strtof(text, &end)));
			return end != text;
		case SE_EDITOR_VALUE_DOUBLE:
			*out = se_editor_value_double((f64)editor_sdf_json_round_f32((f32)strtod(text, &end)));
			return end != text;
		case SE_EDITOR_VALUE_VEC2:
			if (!editor_sdf_parse_floats(text, values, 2u)) return false;
			*out = se_editor_value_vec2(s_vec2(editor_sdf_json_round_f32(values[0]), editor_sdf_json_round_f32(values[1])));
			return true;
		case SE_EDITOR_VALUE_VEC3:
			if (!editor_sdf_parse_floats(text, values, 3u)) return false;
			*out = se_editor_value_vec3(s_vec3(
				editor_sdf_json_round_f32(values[0]),
				editor_sdf_json_round_f32(values[1]),
				editor_sdf_json_round_f32(values[2])));
			return true;
		case SE_EDITOR_VALUE_VEC4:
			if (!editor_sdf_parse_floats(text, values, 4u)) return false;
			*out = se_editor_value_vec4(s_vec4(
				editor_sdf_json_round_f32(values[0]),
				editor_sdf_json_round_f32(values[1]),
				editor_sdf_json_round_f32(values[2]),
				editor_sdf_json_round_f32(values[3])));
			return true;
		case SE_EDITOR_VALUE_MAT4:
			if (!editor_sdf_parse_floats(text, values, 16u)) return false;
			{
				s_mat4 matrix = s_mat4_identity;
				u32 index = 0u;
				for (u32 row = 0u; row < 4u; ++row) {
					for (u32 col = 0u; col < 4u; ++col) {
						matrix.m[row][col] = editor_sdf_json_round_f32(values[index++]);
					}
				}
				*out = se_editor_value_mat4(matrix);
			}
			return true;
		case SE_EDITOR_VALUE_TEXT:
			*out = se_editor_value_text(text);
			return true;
		default:
			return false;
	}
}

static b8 editor_sdf_collect_selection(sdf_editor* editor, const se_editor_item** out_items, sz* out_item_count, se_editor_item* out_item, const se_editor_property** out_properties, sz* out_property_count) {
	const se_editor_item* items = NULL;
	const se_editor_property* properties = NULL;
	sz item_count = 0u;
	sz property_count = 0u;
	if (!editor || !se_editor_collect_items(editor->handle, &items, &item_count) || item_count == 0u) {
		return false;
	}
	if (editor->selected_item >= item_count) editor->selected_item = (u32)item_count - 1u;
	if (!se_editor_collect_properties(editor->handle, &items[editor->selected_item], &properties, &property_count)) {
		property_count = 0u;
		properties = NULL;
	}
	if (property_count == 0u) {
		editor->selected_property = 0u;
		editor->selected_component = 0u;
	} else if (editor->selected_property >= property_count) {
		editor->selected_property = (u32)property_count - 1u;
	}
	if (property_count > 0u) {
		const u32 component_count = se_editor_property_component_count(&properties[editor->selected_property]);
		if (component_count == 0u) {
			editor->selected_component = 0u;
		} else if (editor->selected_component >= component_count) {
			editor->selected_component = component_count - 1u;
		}
	}
	if (out_items) *out_items = items;
	if (out_item_count) *out_item_count = item_count;
	if (out_item) *out_item = items[editor->selected_item];
	if (out_properties) *out_properties = properties;
	if (out_property_count) *out_property_count = property_count;
	return true;
}

static b8 editor_sdf_focus_walk(s_json* node, const s_mat4* parent_transform, s_handle target, u32* id, s_vec3* out_focus) {
	s_mat4 local = s_mat4_identity;
	s_mat4 world = parent_transform ? *parent_transform : s_mat4_identity;
	if (!node || node->type != S_JSON_OBJECT || !id || !out_focus) return false;
	editor_sdf_json_mat4(s_json_get(node, "transform"), &local);
	world = s_mat4_mul(&world, &local);
	if ((s_handle)(*id) == target) {
		*out_focus = s_mat4_get_translation(&world);
		return true;
	}
	(*id)++;
	{
		const c8* arrays[] = { "noises", "point_lights", "directional_lights" };
		for (u32 a = 0u; a < 3u; ++a) {
			s_json* array = s_json_get(node, arrays[a]);
			for (sz i = 0u; array && array->type == S_JSON_ARRAY && i < array->as.children.count; ++i) {
				s_json* child = s_json_at(array, i);
				if ((s_handle)(*id) == target) {
					s_vec3 position = s_mat4_get_translation(&world);
					if (a == 1u) {
						s_vec3 local_position = s_vec3(0.0f, 0.0f, 0.0f);
						if (editor_sdf_json_vec3(s_json_get(child, "position"), &local_position)) {
							position = editor_sdf_mat4_point(&world, &local_position);
						}
					}
					*out_focus = position;
					return true;
				}
				(*id)++;
			}
		}
	}
	{
		s_json* children = s_json_get(node, "children");
		for (sz i = 0u; children && children->type == S_JSON_ARRAY && i < children->as.children.count; ++i) {
			if (editor_sdf_focus_walk(s_json_at(children, i), &world, target, id, out_focus)) return true;
		}
	}
	return false;
}

static b8 editor_sdf_item_focus(sdf_editor* editor, const se_editor_item* item, s_vec3* out_focus) {
	u32 id = 1u;
	if (!editor || !item || !out_focus) return false;
	return editor_sdf_focus_walk(editor->root, &s_mat4_identity, item->handle, &id, out_focus);
}

static void editor_sdf_focus_selection(sdf_editor* editor, b8 show_status) {
	se_editor_item item = {0};
	s_vec3 target = s_vec3(0.0f, 0.0f, 0.0f);
	if (!editor || !editor_sdf_collect_selection(editor, NULL, NULL, &item, NULL, NULL)) return;
	if (!editor_sdf_item_focus(editor, &item, &target)) return;
	editor->focused_handle = item.handle;
	editor->focus_target = target;
	editor->focus_valid = true;
	editor->focus_active = true;
	if (show_status) {
		editor_sdf_set_status(editor, "focus %s @ %.2f %.2f %.2f", item.label, target.x, target.y, target.z);
	}
}

static void editor_sdf_mark_selection_seen(sdf_editor* editor) {
	se_editor_item item = {0};
	if (!editor || !editor_sdf_collect_selection(editor, NULL, NULL, &item, NULL, NULL)) return;
	editor->focused_handle = item.handle;
	editor->focus_active = false;
}

static void editor_sdf_update_focus(sdf_editor* editor, f32 dt) {
	se_editor_item item = {0};
	if (!editor || editor->editing || editor->moving) return;
	if (editor_sdf_collect_selection(editor, NULL, NULL, &item, NULL, NULL) && item.handle != editor->focused_handle) {
		editor_sdf_focus_selection(editor, true);
	}
	if (editor->focus_active && editor->focus_valid) {
		const f32 t = s_min(1.0f, s_max(0.0f, dt * 25.f));
		editor->camera_target = s_vec3_lerp(&editor->camera_target, &editor->focus_target, t);
		if (s_vec3_length(&s_vec3_sub(&editor->focus_target, &editor->camera_target)) < 0.03f) {
			editor->camera_target = editor->focus_target;
			editor->focus_active = false;
		}
		editor_sdf_apply_camera(editor);
	}
}

static u32 editor_sdf_item_depth(const se_editor_item* items, sz item_count, const se_editor_item* item) {
	u32 depth = 0u;
	s_handle owner = item ? item->owner_handle : S_HANDLE_NULL;
	while (owner != S_HANDLE_NULL && depth < 8u) {
		b8 found = false;
		for (sz i = 0u; i < item_count; ++i) {
			if (items[i].handle == owner) {
				owner = items[i].owner_handle;
				depth++;
				found = true;
				break;
			}
		}
		if (!found) break;
	}
	return depth;
}

static void editor_sdf_begin_edit(sdf_editor* editor) {
	se_editor_item item = {0};
	const se_editor_property* properties = NULL;
	sz property_count = 0u;
	if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count) || property_count == 0u) return;
	editor->active_pane = EDITOR_SDF_PANE_PROPERTIES;
	if (!properties[editor->selected_property].editable) {
		editor_sdf_set_status(editor, "%s read only", editor_sdf_property_label(&properties[editor->selected_property]));
		return;
	}
	editor_sdf_value_text(&properties[editor->selected_property].value, editor->edit_buffer, sizeof(editor->edit_buffer), true);
	editor->editing = true;
	se_editor_set_mode(editor->handle, SE_EDITOR_MODE_INSERT);
	editor_sdf_set_status(editor, "edit %s", editor_sdf_property_label(&properties[editor->selected_property]));
	(void)item;
}

static void editor_sdf_cancel_edit(sdf_editor* editor) {
	if (!editor) return;
	editor->editing = false;
	editor->edit_buffer[0] = '\0';
	se_editor_set_mode(editor->handle, SE_EDITOR_MODE_NORMAL);
	editor_sdf_set_status(editor, "edit canceled");
}

static void editor_sdf_commit_edit(sdf_editor* editor) {
	se_editor_item item = {0};
	const se_editor_property* properties = NULL;
	sz property_count = 0u;
	se_editor_value value = {0};
	c8 property_name[SE_MAX_NAME_LENGTH] = {0};
	const c8* property_label = "";
	se_editor_value_type type = SE_EDITOR_VALUE_NONE;
	if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count) || property_count == 0u) return;
	type = properties[editor->selected_property].value.type;
	snprintf(property_name, sizeof(property_name), "%s", properties[editor->selected_property].name);
	property_label = editor_sdf_property_label(&properties[editor->selected_property]);
	if (!editor_sdf_parse_value(type, editor->edit_buffer, &value)) {
		editor_sdf_set_status(editor, "bad value for %s", property_label);
		return;
	}
	if (!se_editor_apply_set(editor->handle, &item, property_name, &value)) {
		editor_sdf_set_status(editor, "apply %s failed: %s", property_label, se_result_str(se_get_last_error()));
		return;
	}
	editor->editing = false;
	editor->edit_buffer[0] = '\0';
	se_editor_set_mode(editor->handle, SE_EDITOR_MODE_NORMAL);
	editor_sdf_focus_selection(editor, false);
	editor_sdf_set_status(editor, "set %s", property_label);
}

static b8 editor_sdf_shift_down(const sdf_editor* editor) {
	return editor && (se_window_is_key_down(editor->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(editor->window, SE_KEY_RIGHT_SHIFT));
}

static b8 editor_sdf_alt_down(const sdf_editor* editor) {
	return editor && (se_window_is_key_down(editor->window, SE_KEY_LEFT_ALT) || se_window_is_key_down(editor->window, SE_KEY_RIGHT_ALT));
}

static u32 editor_sdf_property_cycle_count(const se_editor_property* property) {
	const u32 count = se_editor_property_component_count(property);
	return count == 0u ? 1u : count;
}

static f32 editor_sdf_property_delta(const sdf_editor* editor, const se_editor_property* property, f32 direction) {
	f32 amount = property && property->step > 0.0f ? property->step : 0.1f;
	if (editor_sdf_alt_down(editor)) {
		amount *= 0.1f;
	} else if (editor_sdf_shift_down(editor)) {
		amount = property && property->large_step > 0.0f ? property->large_step : amount * 10.0f;
	}
	return direction < 0.0f ? -amount : amount;
}

static b8 editor_sdf_find_position_property(const se_editor_property* properties, sz property_count, u32* out_index) {
	if (!properties || !out_index) return false;
	for (u32 i = 0u; i < (u32)property_count; ++i) {
		if (properties[i].editable && properties[i].role == SE_EDITOR_PROPERTY_ROLE_POSITION) {
			*out_index = i;
			return true;
		}
	}
	return false;
}

static void editor_sdf_begin_move(sdf_editor* editor) {
	const se_editor_property* properties = NULL;
	sz property_count = 0u;
	u32 property_index = 0u;
	if (!editor || !editor_sdf_collect_selection(editor, NULL, NULL, NULL, &properties, &property_count) || property_count == 0u) return;
	if (!editor_sdf_find_position_property(properties, property_count, &property_index)) {
		editor_sdf_set_status(editor, "selection has no position property");
		return;
	}
	editor->active_pane = EDITOR_SDF_PANE_PROPERTIES;
	editor->selected_property = property_index;
	editor->selected_component = 0u;
	editor->moving = true;
	snprintf(editor->move_property_name, sizeof(editor->move_property_name), "%s", properties[property_index].name);
	snprintf(editor->move_property_label, sizeof(editor->move_property_label), "%s", editor_sdf_property_label(&properties[property_index]));
	editor->move_original_value = properties[property_index].value;
	editor_sdf_set_status(editor, "move %s [x]", editor->move_property_label);
}

static void editor_sdf_end_move(sdf_editor* editor, b8 keep_changes) {
	se_editor_item item = {0};
	const se_editor_property* properties = NULL;
	const se_editor_property* property = NULL;
	sz property_count = 0u;
	b8 restored = true;
	if (!editor || !editor->moving) return;
	if (!keep_changes) {
		if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count) ||
			!se_editor_find_property(properties, property_count, editor->move_property_name, &property) ||
			!se_editor_apply_set(editor->handle, &item, editor->move_property_name, &editor->move_original_value)) {
			restored = false;
		}
	}
	editor->moving = false;
	editor->move_property_name[0] = '\0';
	editor->move_property_label[0] = '\0';
	editor_sdf_focus_selection(editor, false);
	if (keep_changes) {
		editor_sdf_set_status(editor, "move kept");
	} else if (restored) {
		editor_sdf_set_status(editor, "move canceled");
	} else {
		editor_sdf_set_status(editor, "move cancel failed: %s", se_result_str(se_get_last_error()));
	}
}

static void editor_sdf_set_move_axis(sdf_editor* editor, u32 component) {
	const se_editor_property* properties = NULL;
	const se_editor_property* property = NULL;
	sz property_count = 0u;
	const c8* component_name = NULL;
	if (!editor || !editor->moving || !editor_sdf_collect_selection(editor, NULL, NULL, NULL, &properties, &property_count) ||
		!se_editor_find_property(properties, property_count, editor->move_property_name, &property)) {
		return;
	}
	if (component >= se_editor_property_component_count(property)) {
		return;
	}
	editor->selected_component = component;
	component_name = se_editor_property_component_name(property, editor->selected_component);
	editor_sdf_set_status(editor, "move %s [%s]", editor_sdf_property_label(property), component_name ? component_name : "?");
}

static void editor_sdf_nudge_property(sdf_editor* editor, f32 direction) {
	se_editor_item item = {0};
	const se_editor_property* properties = NULL;
	const se_editor_property* property = NULL;
	sz property_count = 0u;
	se_editor_value value = {0};
	f32 amount = 0.0f;
	if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count) || property_count == 0u) return;
	editor->active_pane = EDITOR_SDF_PANE_PROPERTIES;
	property = &properties[editor->selected_property];
	if (!property->editable) {
		editor_sdf_set_status(editor, "%s read only", editor_sdf_property_label(property));
		return;
	}
	value = property->value;
	amount = editor_sdf_property_delta(editor, property, direction);
	if (!se_editor_value_adjust_component(&value, editor->selected_component, amount)) {
		editor_sdf_set_status(editor, "%s needs text edit", editor_sdf_property_label(property));
		return;
	}
	if (se_editor_apply_set(editor->handle, &item, property->name, &value)) {
		editor_sdf_focus_selection(editor, false);
		editor_sdf_set_status(editor, "%s %+.3f", editor_sdf_property_label(property), amount);
	} else {
		editor_sdf_set_status(editor, "apply %s failed: %s", editor_sdf_property_label(property), se_result_str(se_get_last_error()));
	}
}

static void editor_sdf_nudge_move(sdf_editor* editor, f32 direction) {
	se_editor_item item = {0};
	const se_editor_property* properties = NULL;
	const se_editor_property* property = NULL;
	sz property_count = 0u;
	se_editor_value value = {0};
	f32 amount = 0.0f;
	const c8* component_name = NULL;
	if (!editor || !editor->moving) return;
	if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count) ||
		!se_editor_find_property(properties, property_count, editor->move_property_name, &property)) {
		editor_sdf_set_status(editor, "move target missing");
		return;
	}
	value = property->value;
	amount = editor_sdf_property_delta(editor, property, direction);
	if (!se_editor_value_adjust_component(&value, editor->selected_component, amount)) {
		editor_sdf_set_status(editor, "%s needs text edit", editor_sdf_property_label(property));
		return;
	}
	if (se_editor_apply_set(editor->handle, &item, property->name, &value)) {
		component_name = se_editor_property_component_name(property, editor->selected_component);
		editor_sdf_focus_selection(editor, false);
		editor_sdf_set_status(editor, "move %s [%s] %+.3f", editor_sdf_property_label(property), component_name ? component_name : "?", amount);
	} else {
		editor_sdf_set_status(editor, "apply %s failed: %s", editor_sdf_property_label(property), se_result_str(se_get_last_error()));
	}
}

static void editor_sdf_text_input(se_window_handle window, const c8* utf8_text, void* user_data) {
	sdf_editor* editor = user_data;
	sz length = 0u;
	sz add_length = 0u;
	(void)window;
	if (!editor || !editor->editing || !utf8_text) return;
	length = strlen(editor->edit_buffer);
	add_length = strlen(utf8_text);
	if (length + add_length + 1u >= sizeof(editor->edit_buffer)) return;
	memcpy(editor->edit_buffer + length, utf8_text, add_length + 1u);
}

static void editor_sdf_update_edit_keys(sdf_editor* editor) {
	sz length = 0u;
	if (!editor || !editor->editing) return;
	if (se_window_is_key_pressed(editor->window, SE_KEY_ESCAPE)) {
		editor_sdf_cancel_edit(editor);
		return;
	}
	if (se_window_is_key_pressed(editor->window, SE_KEY_ENTER) || se_window_is_key_pressed(editor->window, SE_KEY_KP_ENTER)) {
		editor_sdf_commit_edit(editor);
		return;
	}
	if (se_window_is_key_pressed(editor->window, SE_KEY_BACKSPACE)) {
		length = strlen(editor->edit_buffer);
		if (length > 0u) editor->edit_buffer[length - 1u] = '\0';
	}
}

static void editor_sdf_update_move_keys(sdf_editor* editor) {
	if (!editor || !editor->moving) return;
	if (se_window_is_key_pressed(editor->window, SE_KEY_ESCAPE)) {
		editor_sdf_end_move(editor, false);
	}
}

static void editor_sdf_update_camera(sdf_editor* editor, f32 dt) {
	s_vec3 move = s_vec3(0.0f, 0.0f, 0.0f);
	b8 control = false;
	b8 manual = false;
	if (!editor || editor->editing || editor->moving) return;
	control = se_window_is_key_down(editor->window, SE_KEY_LEFT_CONTROL) || se_window_is_key_down(editor->window, SE_KEY_RIGHT_CONTROL);
	if (control) return;
	{
		const s_vec3 forward_xz = s_vec3(sinf(editor->camera_yaw), 0.0f, -cosf(editor->camera_yaw));
		const s_vec3 right_xz = s_vec3(cosf(editor->camera_yaw), 0.0f, sinf(editor->camera_yaw));
		const f32 move_speed = (se_window_is_key_down(editor->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(editor->window, SE_KEY_RIGHT_SHIFT)) ? 8.0f : 4.0f;
		if (se_window_is_key_down(editor->window, SE_KEY_W)) move = s_vec3_add(&move, &forward_xz);
		if (se_window_is_key_down(editor->window, SE_KEY_S)) move = s_vec3_sub(&move, &forward_xz);
		if (se_window_is_key_down(editor->window, SE_KEY_D)) move = s_vec3_add(&move, &right_xz);
		if (se_window_is_key_down(editor->window, SE_KEY_A)) move = s_vec3_sub(&move, &right_xz);
		if (se_window_is_key_down(editor->window, SE_KEY_E)) move.y += 1.0f;
		if (se_window_is_key_down(editor->window, SE_KEY_Q)) move.y -= 1.0f;
		if (s_vec3_length(&move) > 0.0001f) {
			const s_vec3 direction = s_vec3_normalize(&move);
			move = s_vec3_muls(&direction, move_speed * s_max(dt, 0.0f));
			editor->camera_target = s_vec3_add(&editor->camera_target, &move);
			manual = true;
		}
	}
	if (se_window_is_key_down(editor->window, SE_KEY_U)) { editor->camera_yaw -= dt * 1.4f; manual = true; }
	if (se_window_is_key_down(editor->window, SE_KEY_O)) { editor->camera_yaw += dt * 1.4f; manual = true; }
	if (se_window_is_key_down(editor->window, SE_KEY_Y)) { editor->camera_pitch = s_min(1.35f, editor->camera_pitch + dt * 1.0f); manual = true; }
	if (se_window_is_key_down(editor->window, SE_KEY_P)) { editor->camera_pitch = s_max(-1.35f, editor->camera_pitch - dt * 1.0f); manual = true; }
	if (se_window_is_key_down(editor->window, SE_KEY_Z)) { editor->camera_distance = s_max(2.0f, editor->camera_distance - dt * 6.0f); manual = true; }
	if (se_window_is_key_down(editor->window, SE_KEY_X)) { editor->camera_distance = s_min(40.0f, editor->camera_distance + dt * 6.0f); manual = true; }
	if (manual) {
		editor->focus_active = false;
		editor_sdf_mark_selection_seen(editor);
	}
	editor_sdf_apply_camera(editor);
}

static void editor_sdf_draw_text(sdf_editor* editor, f32 x, f32 y, const c8* format, ...) {
	c8 line[256] = {0};
	va_list args;
	if (!editor || editor->font == S_HANDLE_NULL || !format) return;
	va_start(args, format);
	vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	se_text_draw(editor->font, line, &s_vec2(x, y), &s_vec2(EDITOR_SDF_TEXT_SCALE, EDITOR_SDF_TEXT_SCALE), EDITOR_SDF_TEXT_NEWLINE_OFFSET);
}

static void editor_sdf_draw_ui(sdf_editor* editor) {
	const se_editor_item* items = NULL;
	const se_editor_property* properties = NULL;
	sz item_count = 0u;
	sz property_count = 0u;
	u32 item_start = 0u;
	u32 property_start = 0u;
	u32 visible_rows = 0u;
	const f32 line_step = editor_sdf_text_line_step(editor);
	const f32 title_y = EDITOR_SDF_TEXT_TOP_Y;
	const f32 selected_y = title_y - line_step;
	const f32 pane_y = selected_y - line_step;
	const f32 list_y = pane_y - line_step;
	const f32 footer_y = EDITOR_SDF_TEXT_BOTTOM_Y;
	const f32 help_y = footer_y + line_step;
	const f32 status_y = help_y + line_step;
	const f32 list_bottom_y = status_y + line_step;
	const c8* mode = editor->moving ? "move" : se_editor_mode_name(se_editor_get_mode(editor->handle));
	if (line_step > 0.0f && list_y > list_bottom_y) {
		visible_rows = (u32)s_max(1.0f, floorf(((list_y - list_bottom_y) / line_step) + 1.0f));
	}
	if (!editor_sdf_collect_selection(editor, &items, &item_count, NULL, &properties, &property_count)) {
		editor_sdf_draw_text(editor, -0.96f, selected_y, "SDF editor: no items");
		return;
	}
	if (visible_rows > 0u) {
		if (editor->selected_item > visible_rows / 2u) item_start = editor->selected_item - visible_rows / 2u;
		if (item_start + visible_rows > item_count) item_start = item_count > visible_rows ? (u32)item_count - visible_rows : 0u;
		if (editor->selected_property > visible_rows / 2u) property_start = editor->selected_property - visible_rows / 2u;
		if (property_start + visible_rows > property_count) property_start = property_count > visible_rows ? (u32)property_count - visible_rows : 0u;
	}

	editor_sdf_draw_text(editor, -0.96f, title_y, "SDF level editor | %s | %s", mode, editor->json_path);
	editor_sdf_draw_text(editor, -0.96f, selected_y, "selected: %s @ %.2f %.2f %.2f%s",
		items[editor->selected_item].label,
		editor->focus_target.x, editor->focus_target.y, editor->focus_target.z,
		editor->focus_active ? " -> camera" : "");
	editor_sdf_draw_text(editor, -0.96f, pane_y, "%s items (%zu)", editor->active_pane == EDITOR_SDF_PANE_ITEMS ? ">" : " ", item_count);
	editor_sdf_draw_text(editor, -0.10f, pane_y, "%s properties (%zu)", editor->active_pane == EDITOR_SDF_PANE_PROPERTIES ? ">" : " ", property_count);
	for (u32 row = 0u; row < visible_rows && item_start + row < item_count; ++row) {
		const u32 index = item_start + row;
		const u32 depth = editor_sdf_item_depth(items, item_count, &items[index]);
		editor_sdf_draw_text(editor, -0.96f, list_y - (f32)row * line_step, "%s%*s%s",
			index == editor->selected_item ? ">" : " ",
			(i32)(depth * 2u), "",
			items[index].label);
	}
	for (u32 row = 0u; row < visible_rows && property_start + row < property_count; ++row) {
		c8 value_text[160] = {0};
		c8 component[16] = {0};
		const c8* component_name = NULL;
		const u32 index = property_start + row;
		editor_sdf_value_text(&properties[index].value, value_text, sizeof(value_text), false);
		component_name = se_editor_property_component_name(&properties[index], editor->selected_component);
		if (index == editor->selected_property && se_editor_property_component_count(&properties[index]) > 1u && component_name) {
			snprintf(component, sizeof(component), "[%s] ", component_name);
		}
		editor_sdf_draw_text(editor, -0.10f, list_y - (f32)row * line_step, "%s%s%s = %s%s",
			index == editor->selected_property ? ">" : " ",
			properties[index].editable ? "" : "(ro) ",
			editor_sdf_property_label(&properties[index]),
			component,
			value_text);
	}
	if (editor->editing && property_count > 0u) {
		editor_sdf_draw_text(editor, -0.96f, status_y, "edit %s: %s_", editor_sdf_property_label(&properties[editor->selected_property]), editor->edit_buffer);
	} else {
		editor_sdf_draw_text(editor, -0.96f, status_y, "%s", editor->status[0] ? editor->status : "ready");
	}
	editor_sdf_draw_text(editor, -0.96f, help_y, "j/k select  h/l pane  Enter/i edit  +/- nudge  [/] component  M move  Ctrl+S save  Ctrl+O load  Ctrl+Q quit");
	editor_sdf_draw_text(editor, -0.96f, footer_y, "move: X/Y/Z axis  Shift coarse  Alt fine  Enter keep  Esc cancel  |  camera: WASD/QE U/O Y/P Z/X R F");
}

static b8 editor_sdf_bind_shortcuts(sdf_editor* editor) {
	b8 ok = true;
	if (!editor || !editor->handle) return false;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_ENTER, SE_EDITOR_SHORTCUT_MOD_NONE, "edit") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_KP_ENTER, SE_EDITOR_SHORTCUT_MOD_NONE, "edit") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_EQUAL, SE_EDITOR_SHORTCUT_MOD_NONE, "increase") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_KP_ADD, SE_EDITOR_SHORTCUT_MOD_NONE, "increase") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_MINUS, SE_EDITOR_SHORTCUT_MOD_NONE, "decrease") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_KP_SUBTRACT, SE_EDITOR_SHORTCUT_MOD_NONE, "decrease") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_LEFT_BRACKET, SE_EDITOR_SHORTCUT_MOD_NONE, "component_prev") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_RIGHT_BRACKET, SE_EDITOR_SHORTCUT_MOD_NONE, "component_next") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_M, SE_EDITOR_SHORTCUT_MOD_NONE, "move_mode") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_X, SE_EDITOR_SHORTCUT_MOD_NONE, "move_axis_x") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_Y, SE_EDITOR_SHORTCUT_MOD_NONE, "move_axis_y") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_Z, SE_EDITOR_SHORTCUT_MOD_NONE, "move_axis_z") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_F, SE_EDITOR_SHORTCUT_MOD_NONE, "focus") && ok;
	ok = se_editor_bind_key(editor->handle, SE_EDITOR_MODE_NORMAL, SE_KEY_R, SE_EDITOR_SHORTCUT_MOD_NONE, "reset_camera") && ok;
	ok = se_editor_set_shortcut_repeat(editor->handle, "increase", true) && ok;
	ok = se_editor_set_shortcut_repeat(editor->handle, "decrease", true) && ok;
	return ok;
}

static b8 editor_sdf_apply_command(se_editor* handle, const se_editor_command* command, void* user_data) {
	(void)handle;
	sdf_editor* editor = user_data;
	editor_sdf_item_kind kind = 0;
	editor_sdf_runtime_item runtime = {0};
	if (!editor || !command) return false;
	s_json* object = editor_sdf_item_json(editor, &command->item, &kind);
	const c8* path = se_editor_value_as_text(&command->value);
	if (strcmp(command->name, "json_path") == 0) {
		if (!path || path[0] == '\0') return false;
		snprintf(editor->json_path, sizeof(editor->json_path), "%s", path);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, "json_save_file") == 0) {
		return editor_sdf_save_file(editor, path && path[0] ? path : editor->json_path);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, "json_load_file") == 0) {
		return editor_sdf_load_file(editor, path && path[0] ? path : editor->json_path);
	}
	if (!object || command->type == SE_EDITOR_COMMAND_ACTION) return false;
	if (!editor_sdf_item_runtime(editor, &command->item, &runtime) || runtime.kind != kind) return false;
	if (kind == EDITOR_SDF_NODE) {
		s_json* shading = s_json_get(object, "shading");
		s_json* shadow = s_json_get(object, "shadow");
		s_mat4 mat4_value = s_mat4_identity;
		s_vec3 vec3_value = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 position = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 rotation = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 scale = s_vec3(1.0f, 1.0f, 1.0f);
		f32 f32_value = 0.0f;
		u32 u32_value = 0u;
		if (strcmp(command->name, "type") == 0) {
			const c8* type = se_editor_value_as_text(&command->value);
			if (!type || !editor_sdf_set_string(object, "type", type)) return false;
			if (!editor_sdf_ensure_shape_defaults(object, type)) return false;
			return editor_sdf_apply_json(editor);
		} else if (strcmp(command->name, "operation") == 0) {
			const c8* operation = se_editor_value_as_text(&command->value);
			if (!operation || !editor_sdf_set_string(object, "operation", operation)) return false;
			return editor_sdf_apply_json(editor);
		} else if (strcmp(command->name, "operation_amount") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(object, "operation_amount", f32_value)) return false;
			se_sdf_set_operation_amount(runtime.sdf, f32_value);
			return true;
		} else if (strcmp(command->name, "transform.matrix") == 0 && se_editor_value_as_mat4(&command->value, &mat4_value)) {
			if (!editor_sdf_set_mat4(object, "transform", &mat4_value)) return false;
			se_sdf_set_transform(runtime.sdf, &mat4_value);
			return true;
		} else if (strcmp(command->name, "transform.position") == 0 && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			editor_sdf_json_transform_values(object, NULL, &position, &rotation, &scale);
			position = vec3_value;
			if (!editor_sdf_set_transform_values(object, &position, &rotation, &scale)) return false;
			mat4_value = editor_sdf_transform_compose(&position, &rotation, &scale);
			se_sdf_set_transform(runtime.sdf, &mat4_value);
			return true;
		} else if (strcmp(command->name, "transform.rotation") == 0 && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			editor_sdf_json_transform_values(object, NULL, &position, &rotation, &scale);
			rotation = vec3_value;
			if (!editor_sdf_set_transform_values(object, &position, &rotation, &scale)) return false;
			mat4_value = editor_sdf_transform_compose(&position, &rotation, &scale);
			se_sdf_set_transform(runtime.sdf, &mat4_value);
			return true;
		} else if (strcmp(command->name, "transform.scale") == 0 && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			editor_sdf_json_transform_values(object, NULL, &position, &rotation, &scale);
			scale = vec3_value;
			if (!editor_sdf_set_transform_values(object, &position, &rotation, &scale)) return false;
			mat4_value = editor_sdf_transform_compose(&position, &rotation, &scale);
			se_sdf_set_transform(runtime.sdf, &mat4_value);
			return true;
		} else if (strcmp(command->name, "radius") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(s_json_get(object, "sphere"), "radius", f32_value)) return false;
			return editor_sdf_apply_json(editor);
		} else if (strcmp(command->name, "size") == 0 && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			if (!editor_sdf_set_vec3(s_json_get(object, "box"), "size", vec3_value)) return false;
			return editor_sdf_apply_json(editor);
		} else if ((strcmp(command->name, "shading.ambient") == 0 || strcmp(command->name, "shading.diffuse") == 0 || strcmp(command->name, "shading.specular") == 0) && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			if (!editor_sdf_set_vec3(shading, command->name + 8, vec3_value)) return false;
			if (strcmp(command->name, "shading.ambient") == 0) se_sdf_set_shading_ambient(runtime.sdf, &vec3_value);
			else if (strcmp(command->name, "shading.diffuse") == 0) se_sdf_set_shading_diffuse(runtime.sdf, &vec3_value);
			else se_sdf_set_shading_specular(runtime.sdf, &vec3_value);
			return true;
		} else if (strcmp(command->name, "shading.roughness") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shading, "roughness", f32_value)) return false;
			se_sdf_set_shading_roughness(runtime.sdf, f32_value);
			return true;
		} else if (strcmp(command->name, "shading.bias") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shading, "bias", f32_value)) return false;
			se_sdf_set_shading_bias(runtime.sdf, f32_value);
			return true;
		} else if (strcmp(command->name, "shading.smoothness") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shading, "smoothness", f32_value)) return false;
			se_sdf_set_shading_smoothness(runtime.sdf, f32_value);
			return true;
		} else if (strcmp(command->name, "shadow.softness") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shadow, "softness", f32_value)) return false;
			se_sdf_set_shadow_softness(runtime.sdf, f32_value);
			return true;
		} else if (strcmp(command->name, "shadow.bias") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shadow, "bias", f32_value)) return false;
			se_sdf_set_shadow_bias(runtime.sdf, f32_value);
			return true;
		} else if (strcmp(command->name, "shadow.samples") == 0 && se_editor_value_as_u32(&command->value, &u32_value)) {
			if (!editor_sdf_set_number(shadow, "samples", (f32)u32_value)) return false;
			se_sdf_set_shadow_samples(runtime.sdf, (u16)u32_value);
			return true;
		} else {
			return false;
		}
	}
	if (kind == EDITOR_SDF_NOISE) {
		s_json* descriptor = s_json_get(object, "descriptor");
		s_vec2 vec2_value = s_vec2(0.0f, 0.0f);
		f32 f32_value = 0.0f;
		u32 u32_value = 0u;
		if (strcmp(command->name, "type") == 0) {
			const c8* type = se_editor_value_as_text(&command->value);
			if (!type || !editor_sdf_set_string(descriptor, "type", type)) return false;
			editor_sdf_json_remove(object, "texture");
			return editor_sdf_apply_json(editor);
		} else if (strcmp(command->name, "noise.frequency") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(descriptor, "frequency", f32_value)) return false;
			editor_sdf_json_remove(object, "texture");
			se_sdf_noise_set_frequency(runtime.noise, f32_value);
			return true;
		} else if (strcmp(command->name, "noise.offset") == 0 && se_editor_value_as_vec2(&command->value, &vec2_value)) {
			if (!editor_sdf_set_vec2(descriptor, "offset", vec2_value)) return false;
			editor_sdf_json_remove(object, "texture");
			se_sdf_noise_set_offset(runtime.noise, &s_vec3(vec2_value.x, vec2_value.y, 0.0f));
			return true;
		} else if (strcmp(command->name, "noise.scale") == 0 && se_editor_value_as_vec2(&command->value, &vec2_value)) {
			if (!editor_sdf_set_vec2(descriptor, "scale", vec2_value)) return false;
			editor_sdf_json_remove(object, "texture");
			se_sdf_noise_set_scale(runtime.noise, &s_vec3(vec2_value.x, vec2_value.y, 0.0f));
			return true;
		} else if (strcmp(command->name, "seed") == 0 && se_editor_value_as_u32(&command->value, &u32_value)) {
			if (!editor_sdf_set_number(descriptor, "seed", (f32)u32_value)) return false;
			editor_sdf_json_remove(object, "texture");
			se_sdf_noise_set_seed(runtime.noise, u32_value);
			return true;
		} else {
			return false;
		}
	}
	if (kind == EDITOR_SDF_POINT_LIGHT || kind == EDITOR_SDF_DIRECTIONAL_LIGHT) {
		s_vec3 vec3_value = s_vec3(0.0f, 0.0f, 0.0f);
		f32 f32_value = 0.0f;
		if ((strcmp(command->name, "position") == 0 || strcmp(command->name, "color") == 0 || strcmp(command->name, "direction") == 0) && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			if (!editor_sdf_set_vec3(object, command->name, vec3_value)) return false;
			if (kind == EDITOR_SDF_POINT_LIGHT) {
				if (strcmp(command->name, "position") == 0) se_sdf_point_light_set_position(runtime.point_light, &vec3_value);
				else se_sdf_point_light_set_color(runtime.point_light, &vec3_value);
			} else {
				if (strcmp(command->name, "direction") == 0) se_sdf_directional_light_set_direction(runtime.directional_light, &vec3_value);
				else se_sdf_directional_light_set_color(runtime.directional_light, &vec3_value);
			}
			return true;
		} else if (strcmp(command->name, "radius") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(object, "radius", f32_value)) return false;
			se_sdf_point_light_set_radius(runtime.point_light, f32_value);
			return true;
		} else {
			return false;
		}
	}
	return false;
}

static b8 editor_sdf_setup(sdf_editor* editor) {
	se_editor_config config = SE_EDITOR_CONFIG_DEFAULTS;
	if (!editor) return false;
	se_paths_set_resource_root(RESOURCES_DIR);
	editor->context = se_context_create();
	editor->window = se_window_create("Syphax - Editor SDF", 1280, 720);
	if (editor->window == S_HANDLE_NULL) return false;
	se_window_set_target_fps(editor->window, 60);
	se_render_set_background(s_vec4(0.04f, 0.05f, 0.07f, 1.0f));
	editor->font = se_font_load(SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), EDITOR_SDF_FONT_SIZE);
	if (editor->font == S_HANDLE_NULL) return false;
	editor->camera = se_camera_create();
	se_camera_set_window_aspect(editor->camera, editor->window);
	se_camera_set_perspective(editor->camera, 50.0f, 0.05f, 100.0f);
	editor->camera_target = s_vec3(0.0f, 0.3f, 0.0f);
	editor->camera_yaw = 0.54f;
	editor->camera_pitch = 0.30f;
	editor->camera_distance = 8.0f;
	editor->focused_handle = S_HANDLE_NULL;
	editor_sdf_apply_camera(editor);
	editor->scene = se_sdf_create();
	if (!editor_sdf_load_file(editor, EDITOR_SDF_PATH)) return false;
	config.window = editor->window;
	config.user_data = editor;
	config.collect_items = editor_sdf_collect_items;
	config.collect_properties = editor_sdf_collect_properties;
	config.apply_command = editor_sdf_apply_command;
	editor->handle = se_editor_create(&config);
	se_window_set_text_callback(editor->window, editor_sdf_text_input, editor);
	editor_sdf_set_status(editor, "ready");
	return editor->handle != NULL;
}

static void editor_sdf_cleanup(sdf_editor* editor) {
	if (!editor) return;
	if (editor->handle) se_editor_destroy(editor->handle);
	if (editor->scene != SE_SDF_NULL) se_sdf_destroy(editor->scene);
	if (editor->font != S_HANDLE_NULL) se_font_destroy(editor->font);
	s_json_free(editor->root);
	if (editor->context) se_context_destroy(editor->context);
}

static b8 editor_sdf_autotest(sdf_editor* editor) {
	const se_editor_item* items = NULL;
	const se_editor_property* properties = NULL;
	const se_editor_property* property = NULL;
	sz item_count = 0u;
	sz property_count = 0u;
	se_editor_shortcut_event event = {0};
	se_editor_item item = {0};
	se_editor_item float_item = {0};
	se_editor_item transform_item = {0};
	se_editor_item box_item = {0};
	se_editor_item sphere_item = {0};
	se_editor_item noise_item = {0};
	se_editor_item point_light_item = {0};
	se_editor_item directional_light_item = {0};
	se_editor_value value = se_editor_value_none();
	s_vec3 position = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 original_position = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 moved_position = s_vec3(0.0f, 0.0f, 0.0f);
	s_json* object = NULL;
	editor_sdf_item_kind kind = 0;
	se_shader_handle shader_before = S_HANDLE_NULL;
	const c8* float_property_name = NULL;
	u32 transform_item_index = 0u;
	u32 color_item_index = 0u;
	b8 found_float = false;
	b8 found_transform = false;
	b8 found_color = false;
	b8 found_box = false;
	b8 found_sphere = false;
	b8 found_noise = false;
	b8 found_point_light = false;
	b8 found_directional_light = false;
	if (!se_editor_collect_items(editor->handle, &items, &item_count) || item_count == 0u) return false;
	for (u32 i = 0u; i < (u32)item_count; ++i) {
		editor_sdf_item_kind item_kind = 0;
		s_json* item_object = NULL;
		const se_editor_property* item_properties = NULL;
		sz item_property_count = 0u;
		const c8* type_name = NULL;
		item_object = editor_sdf_item_json(editor, &items[i], &item_kind);
		if (!item_object) continue;
		if (!found_noise && item_kind == EDITOR_SDF_NOISE) {
			noise_item = items[i];
			found_noise = true;
		}
		if (!found_point_light && item_kind == EDITOR_SDF_POINT_LIGHT) {
			point_light_item = items[i];
			found_point_light = true;
		}
		if (!found_directional_light && item_kind == EDITOR_SDF_DIRECTIONAL_LIGHT) {
			directional_light_item = items[i];
			found_directional_light = true;
		}
		if (item_kind != EDITOR_SDF_NODE) continue;
		type_name = editor_sdf_json_string(item_object, "type", "custom");
		if (!found_box && strcmp(type_name, "box") == 0) {
			box_item = items[i];
			found_box = true;
		}
		if (!found_sphere && strcmp(type_name, "sphere") == 0) {
			sphere_item = items[i];
			found_sphere = true;
		}
		if (!se_editor_collect_properties(editor->handle, &items[i], &item_properties, &item_property_count)) continue;
		if (!found_float) {
			if (se_editor_find_property(item_properties, item_property_count, "operation_amount", &property)) {
				float_item = items[i];
				float_property_name = "operation_amount";
				found_float = true;
			} else if (se_editor_find_property(item_properties, item_property_count, "shading.roughness", &property)) {
				float_item = items[i];
				float_property_name = "shading.roughness";
				found_float = true;
			}
		}
		if (!found_transform && se_editor_find_property(item_properties, item_property_count, "transform.position", &property)) {
			transform_item = items[i];
			transform_item_index = i;
			found_transform = true;
		}
		if (!found_color && se_editor_find_property(item_properties, item_property_count, "shading.ambient", &property)) {
			color_item_index = i;
			found_color = true;
		}
	}
	if (!found_float || !found_transform || !found_directional_light) return false;
	if (found_box) {
		if (!se_editor_collect_properties(editor->handle, &box_item, &properties, &property_count)) return false;
		if (!se_editor_find_property(properties, property_count, "size", &property)) return false;
		if (se_editor_find_property(properties, property_count, "radius", &property)) return false;
	}
	if (found_sphere) {
		if (!se_editor_collect_properties(editor->handle, &sphere_item, &properties, &property_count)) return false;
		if (!se_editor_find_property(properties, property_count, "radius", &property)) return false;
		if (se_editor_find_property(properties, property_count, "size", &property)) return false;
	}
	se_window_begin_frame(editor->window);
	se_render_clear();
	se_sdf_render_to_window(editor->scene, editor->camera, editor->window, 0.1f);
	se_window_end_frame(editor->window);
	shader_before = editor_sdf_scene_shader(editor);
	if (shader_before == S_HANDLE_NULL) return false;
	snprintf(editor->json_path, sizeof(editor->json_path), "%s", "/tmp/editor_sdf_autotest_shortcut.json");
	se_window_clear_input_state(editor->window);
	se_window_inject_key_state(editor->window, SE_KEY_LEFT_CONTROL, true);
	se_window_inject_key_state(editor->window, SE_KEY_S, true);
	if (!se_editor_update_shortcuts(editor->handle) || !se_editor_poll_shortcut(editor->handle, &event) || strcmp(event.action, "save") != 0) return false;
	se_window_clear_input_state(editor->window);
	se_window_inject_key_state(editor->window, SE_KEY_LEFT_CONTROL, true);
	se_window_inject_key_state(editor->window, SE_KEY_Q, true);
	if (!se_editor_update_shortcuts(editor->handle) || !se_editor_poll_shortcut(editor->handle, &event) || strcmp(event.action, "quit") != 0) return false;
	se_window_clear_input_state(editor->window);
	se_window_inject_key_state(editor->window, SE_KEY_ESCAPE, true);
	if (!se_editor_update_shortcuts(editor->handle)) return false;
	if (se_editor_poll_shortcut(editor->handle, &event)) return false;
	se_window_clear_input_state(editor->window);
	value = se_editor_value_float(0.5f);
	if (!se_editor_apply_set(editor->handle, &float_item, float_property_name, &value)) return false;
	if (editor_sdf_scene_shader(editor) != shader_before) return false;
	editor->selected_item = transform_item_index;
	editor->selected_property = 0u;
	editor->selected_component = 0u;
	if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count)) return false;
	if (!se_editor_find_property(properties, property_count, "transform.position", &property)) return false;
	if (se_editor_property_component_count(property) != 3u) return false;
	if (strcmp(se_editor_property_component_name(property, 0u), "x") != 0) return false;
	if (strcmp(se_editor_property_component_name(property, 1u), "y") != 0) return false;
	if (strcmp(se_editor_property_component_name(property, 2u), "z") != 0) return false;
	if (!se_editor_find_property(properties, property_count, "transform.rotation", &property)) return false;
	if (!se_editor_find_property(properties, property_count, "transform.scale", &property)) return false;
	if (!se_editor_find_property(properties, property_count, "transform.matrix", &property)) return false;
	value = se_editor_value_vec3(s_vec3(2.0f, 1.0f, 0.5f));
	if (!se_editor_apply_set(editor->handle, &transform_item, "transform.position", &value)) return false;
	if (editor_sdf_scene_shader(editor) != shader_before) return false;
	object = editor_sdf_item_json(editor, &transform_item, &kind);
	if (!object || kind != EDITOR_SDF_NODE) return false;
	editor_sdf_json_transform_values(object, NULL, &position, NULL, NULL);
	if (fabsf(position.x - 2.0f) > 0.001f || fabsf(position.y - 1.0f) > 0.001f || fabsf(position.z - 0.5f) > 0.001f) return false;
	if (found_color) {
		editor->selected_item = color_item_index;
		if (!editor_sdf_collect_selection(editor, NULL, NULL, NULL, &properties, &property_count)) return false;
		if (!se_editor_find_property(properties, property_count, "shading.ambient", &property)) return false;
		if (strcmp(se_editor_property_component_name(property, 0u), "r") != 0) return false;
		if (strcmp(se_editor_property_component_name(property, 1u), "g") != 0) return false;
		if (strcmp(se_editor_property_component_name(property, 2u), "b") != 0) return false;
	}
	if (found_noise) {
		value = se_editor_value_float(2.5f);
		if (!se_editor_apply_set(editor->handle, &noise_item, "noise.frequency", &value)) return false;
		if (editor_sdf_scene_shader(editor) != shader_before) return false;
	}
	if (found_point_light) {
		value = se_editor_value_float(3.0f);
		if (!se_editor_apply_set(editor->handle, &point_light_item, "radius", &value)) return false;
		if (editor_sdf_scene_shader(editor) != shader_before) return false;
	}
	value = se_editor_value_vec3(s_vec3(0.93f, 0.95f, 1.0f));
	if (!se_editor_apply_set(editor->handle, &directional_light_item, "color", &value)) return false;
	if (editor_sdf_scene_shader(editor) != shader_before) return false;
	editor->selected_item = transform_item_index;
	if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count) ||
		!se_editor_find_property(properties, property_count, "transform.position", &property) ||
		!se_editor_value_as_vec3(&property->value, &original_position)) return false;
	editor_sdf_begin_move(editor);
	if (!editor->moving || strcmp(editor->move_property_name, "transform.position") != 0) return false;
	event = (se_editor_shortcut_event){ .mode = SE_EDITOR_MODE_NORMAL };
	snprintf(event.action, sizeof(event.action), "%s", "move_axis_x");
	editor_sdf_handle_shortcut(editor, &event);
	snprintf(event.action, sizeof(event.action), "%s", "increase");
	editor_sdf_handle_shortcut(editor, &event);
	snprintf(event.action, sizeof(event.action), "%s", "edit");
	editor_sdf_handle_shortcut(editor, &event);
	if (editor->moving) return false;
	if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count) ||
		!se_editor_find_property(properties, property_count, "transform.position", &property) ||
		!se_editor_value_as_vec3(&property->value, &moved_position)) return false;
	if (fabsf(moved_position.x - (original_position.x + 0.1f)) > 0.001f) return false;
	original_position = moved_position;
	editor_sdf_begin_move(editor);
	if (!editor->moving) return false;
	snprintf(event.action, sizeof(event.action), "%s", "move_axis_y");
	editor_sdf_handle_shortcut(editor, &event);
	snprintf(event.action, sizeof(event.action), "%s", "increase");
	editor_sdf_handle_shortcut(editor, &event);
	se_window_inject_key_state(editor->window, SE_KEY_ESCAPE, true);
	editor_sdf_update_move_keys(editor);
	se_window_clear_input_state(editor->window);
	if (editor->moving) return false;
	if (!editor_sdf_collect_selection(editor, NULL, NULL, &item, &properties, &property_count) ||
		!se_editor_find_property(properties, property_count, "transform.position", &property) ||
		!se_editor_value_as_vec3(&property->value, &position)) return false;
	if (fabsf(position.x - original_position.x) > 0.001f || fabsf(position.y - original_position.y) > 0.001f || fabsf(position.z - original_position.z) > 0.001f) return false;
	if (!editor_sdf_save_file(editor, "/tmp/editor_sdf_autotest.json")) return false;
	if (!editor_sdf_load_file(editor, "/tmp/editor_sdf_autotest.json")) return false;
	printf("editor_sdf :: autotest items=%zu ok\n", item_count);
	return true;
}

static void editor_sdf_handle_shortcut(sdf_editor* editor, const se_editor_shortcut_event* event) {
	const se_editor_item* items = NULL;
	const se_editor_property* properties = NULL;
	sz item_count = 0u;
	sz property_count = 0u;
	if (!editor || !event) {
		return;
	}
	if (strcmp(event->action, "normal_mode") == 0) {
		editor_sdf_cancel_edit(editor);
		return;
	}
	if (editor->editing) {
		return;
	}
	editor_sdf_collect_selection(editor, &items, &item_count, NULL, &properties, &property_count);
	if (editor->moving) {
		if (strcmp(event->action, "quit") == 0) {
			editor_sdf_end_move(editor, false);
		} else if (strcmp(event->action, "insert_mode") == 0 || strcmp(event->action, "edit") == 0) {
			editor_sdf_end_move(editor, true);
		} else if (strcmp(event->action, "increase") == 0) {
			editor_sdf_nudge_move(editor, 1.0f);
		} else if (strcmp(event->action, "decrease") == 0) {
			editor_sdf_nudge_move(editor, -1.0f);
		} else if (strcmp(event->action, "move_axis_x") == 0) {
			editor_sdf_set_move_axis(editor, 0u);
		} else if (strcmp(event->action, "move_axis_y") == 0) {
			editor_sdf_set_move_axis(editor, 1u);
		} else if (strcmp(event->action, "move_axis_z") == 0) {
			editor_sdf_set_move_axis(editor, 2u);
		}
		return;
	}
	if (strcmp(event->action, "save") == 0) {
		const b8 ok = editor_sdf_save_file(editor, editor->json_path);
		editor_sdf_set_status(editor, "save %s: %s", ok ? "ok" : "failed", ok ? editor->json_path : se_result_str(se_get_last_error()));
		printf("editor_sdf :: %s\n", editor->status);
	} else if (strcmp(event->action, "load") == 0) {
		const b8 ok = editor_sdf_load_file(editor, editor->json_path);
		if (ok) {
			editor->selected_item = 0u;
			editor->selected_property = 0u;
			editor->selected_component = 0u;
			editor->focused_handle = S_HANDLE_NULL;
			editor->focus_valid = false;
			editor->focus_active = false;
			editor->moving = false;
			editor->move_property_name[0] = '\0';
			editor->move_property_label[0] = '\0';
		}
		editor_sdf_set_status(editor, "load %s: %s", ok ? "ok" : "failed", ok ? editor->json_path : se_result_str(se_get_last_error()));
		printf("editor_sdf :: %s\n", editor->status);
	} else if (strcmp(event->action, "quit") == 0) {
		se_window_set_should_close(editor->window, true);
	} else if (strcmp(event->action, "move_mode") == 0) {
		editor_sdf_begin_move(editor);
	} else if (strcmp(event->action, "insert_mode") == 0 || strcmp(event->action, "edit") == 0) {
		editor_sdf_begin_edit(editor);
	} else if (strcmp(event->action, "move_left") == 0) {
		editor->active_pane = EDITOR_SDF_PANE_ITEMS;
	} else if (strcmp(event->action, "move_right") == 0) {
		editor->active_pane = EDITOR_SDF_PANE_PROPERTIES;
	} else if (strcmp(event->action, "move_down") == 0) {
		if (editor->active_pane == EDITOR_SDF_PANE_PROPERTIES && property_count > 0u) {
			editor->selected_property = (u32)s_min(property_count - 1u, (sz)editor->selected_property + 1u);
		} else if (item_count > 0u) {
			editor->selected_item = (u32)s_min(item_count - 1u, (sz)editor->selected_item + 1u);
			editor->selected_property = 0u;
		}
	} else if (strcmp(event->action, "move_up") == 0) {
		if (editor->active_pane == EDITOR_SDF_PANE_PROPERTIES && property_count > 0u) {
			editor->selected_property = editor->selected_property > 0u ? editor->selected_property - 1u : 0u;
		} else if (item_count > 0u) {
			editor->selected_item = editor->selected_item > 0u ? editor->selected_item - 1u : 0u;
			editor->selected_property = 0u;
		}
	} else if (strcmp(event->action, "next") == 0 || strcmp(event->action, "previous") == 0) {
		editor->active_pane = editor->active_pane == EDITOR_SDF_PANE_ITEMS ? EDITOR_SDF_PANE_PROPERTIES : EDITOR_SDF_PANE_ITEMS;
	} else if (strcmp(event->action, "first") == 0) {
		if (editor->active_pane == EDITOR_SDF_PANE_PROPERTIES) editor->selected_property = 0u;
		else {
			editor->selected_item = 0u;
			editor->selected_property = 0u;
		}
	} else if (strcmp(event->action, "last") == 0) {
		if (editor->active_pane == EDITOR_SDF_PANE_PROPERTIES && property_count > 0u) editor->selected_property = (u32)property_count - 1u;
		else if (item_count > 0u) {
			editor->selected_item = (u32)item_count - 1u;
			editor->selected_property = 0u;
		}
	} else if (strcmp(event->action, "increase") == 0) {
		editor_sdf_nudge_property(editor, 1.0f);
	} else if (strcmp(event->action, "decrease") == 0) {
		editor_sdf_nudge_property(editor, -1.0f);
	} else if (strcmp(event->action, "component_prev") == 0) {
		u32 count = property_count > 0u ? editor_sdf_property_cycle_count(&properties[editor->selected_property]) : 1u;
		editor->selected_component = editor->selected_component == 0u ? count - 1u : editor->selected_component - 1u;
	} else if (strcmp(event->action, "component_next") == 0) {
		u32 count = property_count > 0u ? editor_sdf_property_cycle_count(&properties[editor->selected_property]) : 1u;
		editor->selected_component = (editor->selected_component + 1u) % count;
	} else if (strcmp(event->action, "focus") == 0) {
		editor_sdf_focus_selection(editor, true);
	} else if (strcmp(event->action, "reset_camera") == 0) {
		editor->camera_target = s_vec3(0.0f, 0.3f, 0.0f);
		editor->camera_yaw = 0.54f;
		editor->camera_pitch = 0.30f;
		editor->camera_distance = 8.0f;
		editor_sdf_mark_selection_seen(editor);
		editor_sdf_apply_camera(editor);
		editor_sdf_set_status(editor, "camera reset");
	}
}

int main(int argc, char** argv) {
	sdf_editor editor = {0};
	const b8 autotest = argc > 1 && strcmp(argv[1], "--autotest") == 0;
	if (!editor_sdf_setup(&editor)) {
		printf("editor_sdf :: setup failed (%s)\n", se_result_str(se_get_last_error()));
		editor_sdf_cleanup(&editor);
		return 1;
	}
	if (!se_editor_bind_default_shortcuts(editor.handle)) {
		printf("editor_sdf :: shortcut bind failed (%s)\n", se_result_str(se_get_last_error()));
		editor_sdf_cleanup(&editor);
		return 1;
	}
	if (!editor_sdf_bind_shortcuts(&editor)) {
		printf("editor_sdf :: sdf shortcut bind failed (%s)\n", se_result_str(se_get_last_error()));
		editor_sdf_cleanup(&editor);
		return 1;
	}
	if (autotest) {
		const b8 ok = editor_sdf_autotest(&editor);
		editor_sdf_cleanup(&editor);
		return ok ? 0 : 1;
	}
	printf("editor_sdf controls:\n");
	printf("  j/k select, h/l pane, Enter/i edit, +/- nudge, [/] component, M move\n");
	printf("  move: X/Y/Z axis, Shift coarse, Alt fine, Enter keep, Esc cancel\n");
	printf("  camera: F focus, WASD/QE pan, U/O yaw, Y/P pitch, Z/X zoom, R reset, Ctrl+S save, Ctrl+O load, Ctrl+Q quit\n");
	while (!se_window_should_close(editor.window)) {
		se_editor_shortcut_event event = {0};
		f32 dt = 0.0f;
		se_window_begin_frame(editor.window);
		dt = (f32)se_window_get_delta_time(editor.window);
		se_editor_update_shortcuts(editor.handle);
		while (se_editor_poll_shortcut(editor.handle, &event)) {
			editor_sdf_handle_shortcut(&editor, &event);
		}
		editor_sdf_update_edit_keys(&editor);
		editor_sdf_update_move_keys(&editor);
		editor_sdf_update_camera(&editor, dt);
		editor_sdf_update_focus(&editor, dt);
		se_render_clear();
		se_sdf_render_to_window(editor.scene, editor.camera, editor.window, 0.1f);
		editor_sdf_draw_ui(&editor);
		se_window_end_frame(editor.window);
	}
	editor_sdf_cleanup(&editor);
	return 0;
}
