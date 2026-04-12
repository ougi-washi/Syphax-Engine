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
	se_context* context;
	se_window_handle window;
	se_camera_handle camera;
	se_sdf_handle scene;
	se_font_handle font;
	se_editor* editor;
	s_json* root;
	c8 json_path[SE_MAX_PATH_LENGTH];
	u32 selected_item;
	u32 selected_property;
	u32 selected_component;
	editor_sdf_pane active_pane;
	b8 editing;
	c8 edit_buffer[SE_MAX_PATH_LENGTH];
	c8 status[SE_MAX_PATH_LENGTH];
	s_vec3 camera_target;
	s_vec3 focus_target;
	s_handle focused_handle;
	f32 camera_yaw;
	f32 camera_pitch;
	f32 camera_distance;
	b8 focus_active;
	b8 focus_valid;
} editor_sdf_app;

static void editor_sdf_set_status(editor_sdf_app* app, const c8* format, ...) {
	va_list args;
	if (!app || !format) return;
	va_start(args, format);
	vsnprintf(app->status, sizeof(app->status), format, args);
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

static void editor_sdf_apply_camera(editor_sdf_app* app) {
	if (!app || app->camera == S_HANDLE_NULL) return;
	const s_vec3 rotation = s_vec3(app->camera_pitch, app->camera_yaw, 0.0f);
	se_camera_set_rotation(app->camera, &rotation);
	{
		const s_vec3 forward = se_camera_get_forward_vector(app->camera);
		const s_vec3 position = s_vec3_sub(&app->camera_target, &s_vec3_muls(&forward, app->camera_distance));
		se_camera_set_location(app->camera, &position);
	}
	se_camera_set_target(app->camera, &app->camera_target);
	se_camera_set_window_aspect(app->camera, app->window);
}

static b8 editor_sdf_json_number(const s_json* node, f32* out) {
	if (!node || node->type != S_JSON_NUMBER || !out) return false;
	*out = (f32)node->as.number;
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
	return editor_sdf_json_replace(object, key, s_json_num(NULL, value));
}

static b8 editor_sdf_set_string(s_json* object, const c8* key, const c8* value) {
	return editor_sdf_json_replace(object, key, s_json_str(NULL, value ? value : ""));
}

static b8 editor_sdf_set_vec2(s_json* object, const c8* key, s_vec2 value) {
	s_json* array = s_json_array_empty(NULL);
	if (!array) return false;
	if (!s_json_add(array, s_json_num(NULL, value.x)) ||
		!s_json_add(array, s_json_num(NULL, value.y)) ||
		!editor_sdf_json_replace(object, key, array)) {
		s_json_free(array);
		return false;
	}
	return true;
}

static b8 editor_sdf_set_vec3(s_json* object, const c8* key, s_vec3 value) {
	s_json* array = s_json_array_empty(NULL);
	if (!array) return false;
	if (!s_json_add(array, s_json_num(NULL, value.x)) ||
		!s_json_add(array, s_json_num(NULL, value.y)) ||
		!s_json_add(array, s_json_num(NULL, value.z)) ||
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
			if (!s_json_add(array, s_json_num(NULL, value->m[row][col]))) {
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

static void editor_sdf_add_value(se_editor* editor, const c8* name, se_editor_value value, b8 editable) {
	(void)se_editor_add_property_value(editor, name, &value, editable);
}

static void editor_sdf_add_vec3_child(se_editor* editor, s_json* object, const c8* object_name, const c8* value_name, b8 editable) {
	s_json* parent = object_name ? s_json_get(object, object_name) : object;
	s_vec3 value = s_vec3(0.0f, 0.0f, 0.0f);
	if (editor_sdf_json_vec3(s_json_get(parent, value_name), &value)) {
		editor_sdf_add_value(editor, value_name, se_editor_value_vec3(value), editable);
	}
}

static b8 editor_sdf_add_item_for_json(se_editor* editor, s_json* object, editor_sdf_item_kind kind, s_handle owner, u32* id) {
	c8 label[SE_MAX_NAME_LENGTH] = {0};
	const s_handle handle = (s_handle)(*id);
	editor_sdf_make_item_label(object, kind, handle, label, sizeof(label));
	(*id)++;
	return se_editor_add_item(editor, SE_EDITOR_CATEGORY_CUSTOM, handle, owner, object, NULL, (u32)kind, label);
}

static b8 editor_sdf_collect_node(se_editor* editor, s_json* node, s_handle parent, u32* id) {
	if (!node || node->type != S_JSON_OBJECT) return true;
	const s_handle node_handle = (s_handle)(*id);
	if (!editor_sdf_add_item_for_json(editor, node, EDITOR_SDF_NODE, parent, id)) return false;
	s_json* noises = s_json_get(node, "noises");
	for (sz i = 0u; noises && noises->type == S_JSON_ARRAY && i < noises->as.children.count; ++i) {
		if (!editor_sdf_add_item_for_json(editor, s_json_at(noises, i), EDITOR_SDF_NOISE, node_handle, id)) return false;
	}
	s_json* points = s_json_get(node, "point_lights");
	for (sz i = 0u; points && points->type == S_JSON_ARRAY && i < points->as.children.count; ++i) {
		if (!editor_sdf_add_item_for_json(editor, s_json_at(points, i), EDITOR_SDF_POINT_LIGHT, node_handle, id)) return false;
	}
	s_json* dirs = s_json_get(node, "directional_lights");
	for (sz i = 0u; dirs && dirs->type == S_JSON_ARRAY && i < dirs->as.children.count; ++i) {
		if (!editor_sdf_add_item_for_json(editor, s_json_at(dirs, i), EDITOR_SDF_DIRECTIONAL_LIGHT, node_handle, id)) return false;
	}
	s_json* children = s_json_get(node, "children");
	for (sz i = 0u; children && children->type == S_JSON_ARRAY && i < children->as.children.count; ++i) {
		if (!editor_sdf_collect_node(editor, s_json_at(children, i), node_handle, id)) return false;
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

static s_json* editor_sdf_item_json(editor_sdf_app* app, const se_editor_item* item, editor_sdf_item_kind* out_kind) {
	u32 id = 1u;
	editor_sdf_item_kind kind = 0;
	s_json* found = editor_sdf_find_node(app ? app->root : NULL, item ? item->handle : S_HANDLE_NULL, &id, &kind);
	if (out_kind) *out_kind = kind;
	return found;
}

static b8 editor_sdf_apply_json(editor_sdf_app* app) {
	return app && app->scene != SE_SDF_NULL && app->root && se_sdf_from_json(app->scene, app->root);
}

static b8 editor_sdf_collect_items(se_editor* editor, se_editor_category_mask mask, void* user_data) {
	editor_sdf_app* app = user_data;
	u32 id = 1u;
	if (!(mask & se_editor_category_to_mask(SE_EDITOR_CATEGORY_CUSTOM))) return true;
	return app && app->root && editor_sdf_collect_node(editor, app->root, S_HANDLE_NULL, &id);
}

static b8 editor_sdf_collect_properties(se_editor* editor, const se_editor_item* item, void* user_data) {
	editor_sdf_app* app = user_data;
	editor_sdf_item_kind kind = 0;
	s_json* object = editor_sdf_item_json(app, item, &kind);
	if (!object) return false;
	editor_sdf_add_value(editor, "kind", se_editor_value_uint((u32)kind), false);
	editor_sdf_add_value(editor, "json_path", se_editor_value_text(app->json_path), true);
	if (kind == EDITOR_SDF_NODE) {
		s_mat4 transform = s_mat4_identity;
		s_json* type = s_json_get(object, "type");
		s_json* operation = s_json_get(object, "operation");
		editor_sdf_json_mat4(s_json_get(object, "transform"), &transform);
		editor_sdf_add_value(editor, "transform", se_editor_value_mat4(transform), true);
		editor_sdf_add_value(editor, "type", se_editor_value_text(type && type->type == S_JSON_STRING ? type->as.string : "custom"), true);
		editor_sdf_add_value(editor, "operation", se_editor_value_text(operation && operation->type == S_JSON_STRING ? operation->as.string : "union"), true);
		f32 amount = 0.0f;
		if (editor_sdf_json_number(s_json_get(object, "operation_amount"), &amount)) editor_sdf_add_value(editor, "operation_amount", se_editor_value_float(amount), true);
		s_vec3 size = s_vec3(0.0f, 0.0f, 0.0f);
		if (editor_sdf_json_vec3(s_json_get(s_json_get(object, "box"), "size"), &size)) editor_sdf_add_value(editor, "size", se_editor_value_vec3(size), true);
		f32 radius = 0.0f;
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "sphere"), "radius"), &radius)) editor_sdf_add_value(editor, "radius", se_editor_value_float(radius), true);
		editor_sdf_add_vec3_child(editor, object, "shading", "ambient", true);
		editor_sdf_add_vec3_child(editor, object, "shading", "diffuse", true);
		editor_sdf_add_vec3_child(editor, object, "shading", "specular", true);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shading"), "roughness"), &radius)) editor_sdf_add_value(editor, "roughness", se_editor_value_float(radius), true);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shading"), "bias"), &radius)) editor_sdf_add_value(editor, "shading_bias", se_editor_value_float(radius), true);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shading"), "smoothness"), &radius)) editor_sdf_add_value(editor, "smoothness", se_editor_value_float(radius), true);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shadow"), "softness"), &radius)) editor_sdf_add_value(editor, "shadow_softness", se_editor_value_float(radius), true);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shadow"), "bias"), &radius)) editor_sdf_add_value(editor, "shadow_bias", se_editor_value_float(radius), true);
		if (editor_sdf_json_number(s_json_get(s_json_get(object, "shadow"), "samples"), &radius)) editor_sdf_add_value(editor, "shadow_samples", se_editor_value_uint((u32)radius), true);
		editor_sdf_add_value(editor, "child_count", se_editor_value_u64(s_json_get(object, "children") ? s_json_get(object, "children")->as.children.count : 0u), false);
		return true;
	}
	if (kind == EDITOR_SDF_NOISE) {
		s_json* descriptor = s_json_get(object, "descriptor");
		s_json* type = s_json_get(descriptor, "type");
		s_vec2 value = s_vec2(0.0f, 0.0f);
		f32 number = 0.0f;
		editor_sdf_add_value(editor, "type", se_editor_value_text(type && type->type == S_JSON_STRING ? type->as.string : ""), true);
		if (editor_sdf_json_number(s_json_get(descriptor, "frequency"), &number)) editor_sdf_add_value(editor, "noise_frequency", se_editor_value_float(number), true);
		if (editor_sdf_json_vec2(s_json_get(descriptor, "offset"), &value)) editor_sdf_add_value(editor, "noise_offset", se_editor_value_vec2(value), true);
		if (editor_sdf_json_vec2(s_json_get(descriptor, "scale"), &value)) editor_sdf_add_value(editor, "noise_scale", se_editor_value_vec2(value), true);
		if (editor_sdf_json_number(s_json_get(descriptor, "seed"), &number)) editor_sdf_add_value(editor, "seed", se_editor_value_uint((u32)number), true);
		return true;
	}
	if (kind == EDITOR_SDF_POINT_LIGHT) {
		s_vec3 value = s_vec3(0.0f, 0.0f, 0.0f);
		f32 radius = 0.0f;
		if (editor_sdf_json_vec3(s_json_get(object, "position"), &value)) editor_sdf_add_value(editor, "position", se_editor_value_vec3(value), true);
		if (editor_sdf_json_vec3(s_json_get(object, "color"), &value)) editor_sdf_add_value(editor, "color", se_editor_value_vec3(value), true);
		if (editor_sdf_json_number(s_json_get(object, "radius"), &radius)) editor_sdf_add_value(editor, "radius", se_editor_value_float(radius), true);
		return true;
	}
	if (kind == EDITOR_SDF_DIRECTIONAL_LIGHT) {
		s_vec3 value = s_vec3(0.0f, 0.0f, 0.0f);
		if (editor_sdf_json_vec3(s_json_get(object, "direction"), &value)) editor_sdf_add_value(editor, "direction", se_editor_value_vec3(value), true);
		if (editor_sdf_json_vec3(s_json_get(object, "color"), &value)) editor_sdf_add_value(editor, "color", se_editor_value_vec3(value), true);
		return true;
	}
	return false;
}

static b8 editor_sdf_load_file(editor_sdf_app* app, const c8* path) {
	s_json* root = NULL;
	c8 file_path[SE_MAX_PATH_LENGTH] = {0};
	if (!app || !se_paths_resolve_resource_path(file_path, sizeof(file_path), path) || !se_editor_json_read_file(file_path, &root)) return false;
	if (!se_sdf_from_json(app->scene, root)) {
		s_json_free(root);
		return false;
	}
	s_json_free(app->root);
	app->root = root;
	snprintf(app->json_path, sizeof(app->json_path), "%s", path);
	return true;
}

static b8 editor_sdf_save_file(editor_sdf_app* app, const c8* path) {
	s_json* fresh = NULL;
	c8 file_path[SE_MAX_PATH_LENGTH] = {0};
	if (!app || !se_paths_resolve_resource_path(file_path, sizeof(file_path), path)) return false;
	fresh = se_sdf_to_json(app->scene);
	if (!fresh) return false;
	if (!se_editor_json_write_file(file_path, fresh)) {
		s_json_free(fresh);
		return false;
	}
	s_json_free(app->root);
	app->root = fresh;
	snprintf(app->json_path, sizeof(app->json_path), "%s", path);
	return true;
}

static u32 editor_sdf_component_count(se_editor_value_type type) {
	switch (type) {
		case SE_EDITOR_VALUE_VEC2: return 2u;
		case SE_EDITOR_VALUE_VEC3: return 3u;
		case SE_EDITOR_VALUE_VEC4: return 4u;
		case SE_EDITOR_VALUE_MAT4: return 3u;
		default: return 1u;
	}
}

static const c8* editor_sdf_component_name(se_editor_value_type type, u32 component) {
	static const c8* vector_names[] = { "x", "y", "z", "w" };
	static const c8* transform_names[] = { "tx", "ty", "tz" };
	if (type == SE_EDITOR_VALUE_MAT4) return transform_names[component % 3u];
	return vector_names[component % 4u];
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
			snprintf(out, out_size, "%.4f", value->float_value);
			break;
		case SE_EDITOR_VALUE_DOUBLE:
			snprintf(out, out_size, "%.4f", value->double_value);
			break;
		case SE_EDITOR_VALUE_VEC2:
			snprintf(out, out_size, "%.4f %.4f", value->vec2_value.x, value->vec2_value.y);
			break;
		case SE_EDITOR_VALUE_VEC3:
			snprintf(out, out_size, "%.4f %.4f %.4f", value->vec3_value.x, value->vec3_value.y, value->vec3_value.z);
			break;
		case SE_EDITOR_VALUE_VEC4:
			snprintf(out, out_size, "%.4f %.4f %.4f %.4f", value->vec4_value.x, value->vec4_value.y, value->vec4_value.z, value->vec4_value.w);
			break;
		case SE_EDITOR_VALUE_MAT4:
			if (edit_text) {
				snprintf(out, out_size,
					"%.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f",
					value->mat4_value.m[0][0], value->mat4_value.m[0][1], value->mat4_value.m[0][2], value->mat4_value.m[0][3],
					value->mat4_value.m[1][0], value->mat4_value.m[1][1], value->mat4_value.m[1][2], value->mat4_value.m[1][3],
					value->mat4_value.m[2][0], value->mat4_value.m[2][1], value->mat4_value.m[2][2], value->mat4_value.m[2][3],
					value->mat4_value.m[3][0], value->mat4_value.m[3][1], value->mat4_value.m[3][2], value->mat4_value.m[3][3]);
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
			*out = se_editor_value_float(strtof(text, &end));
			return end != text;
		case SE_EDITOR_VALUE_DOUBLE:
			*out = se_editor_value_double(strtod(text, &end));
			return end != text;
		case SE_EDITOR_VALUE_VEC2:
			if (!editor_sdf_parse_floats(text, values, 2u)) return false;
			*out = se_editor_value_vec2(s_vec2(values[0], values[1]));
			return true;
		case SE_EDITOR_VALUE_VEC3:
			if (!editor_sdf_parse_floats(text, values, 3u)) return false;
			*out = se_editor_value_vec3(s_vec3(values[0], values[1], values[2]));
			return true;
		case SE_EDITOR_VALUE_VEC4:
			if (!editor_sdf_parse_floats(text, values, 4u)) return false;
			*out = se_editor_value_vec4(s_vec4(values[0], values[1], values[2], values[3]));
			return true;
		case SE_EDITOR_VALUE_MAT4:
			if (!editor_sdf_parse_floats(text, values, 16u)) return false;
			{
				s_mat4 matrix = s_mat4_identity;
				u32 index = 0u;
				for (u32 row = 0u; row < 4u; ++row) {
					for (u32 col = 0u; col < 4u; ++col) {
						matrix.m[row][col] = values[index++];
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

static b8 editor_sdf_collect_selection(editor_sdf_app* app, const se_editor_item** out_items, sz* out_item_count, se_editor_item* out_item, const se_editor_property** out_properties, sz* out_property_count) {
	const se_editor_item* items = NULL;
	const se_editor_property* properties = NULL;
	sz item_count = 0u;
	sz property_count = 0u;
	if (!app || !se_editor_collect_items(app->editor, se_editor_category_to_mask(SE_EDITOR_CATEGORY_CUSTOM), &items, &item_count) || item_count == 0u) {
		return false;
	}
	if (app->selected_item >= item_count) app->selected_item = (u32)item_count - 1u;
	if (!se_editor_collect_properties(app->editor, &items[app->selected_item], &properties, &property_count)) {
		property_count = 0u;
		properties = NULL;
	}
	if (property_count == 0u) {
		app->selected_property = 0u;
	} else if (app->selected_property >= property_count) {
		app->selected_property = (u32)property_count - 1u;
	}
	if (out_items) *out_items = items;
	if (out_item_count) *out_item_count = item_count;
	if (out_item) *out_item = items[app->selected_item];
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

static b8 editor_sdf_item_focus(editor_sdf_app* app, const se_editor_item* item, s_vec3* out_focus) {
	u32 id = 1u;
	if (!app || !item || !out_focus) return false;
	return editor_sdf_focus_walk(app->root, &s_mat4_identity, item->handle, &id, out_focus);
}

static void editor_sdf_focus_selection(editor_sdf_app* app, b8 show_status) {
	se_editor_item item = {0};
	s_vec3 target = s_vec3(0.0f, 0.0f, 0.0f);
	if (!app || !editor_sdf_collect_selection(app, NULL, NULL, &item, NULL, NULL)) return;
	if (!editor_sdf_item_focus(app, &item, &target)) return;
	app->focused_handle = item.handle;
	app->focus_target = target;
	app->focus_valid = true;
	app->focus_active = true;
	if (show_status) {
		editor_sdf_set_status(app, "focus %s @ %.2f %.2f %.2f", item.label, target.x, target.y, target.z);
	}
}

static void editor_sdf_mark_selection_seen(editor_sdf_app* app) {
	se_editor_item item = {0};
	if (!app || !editor_sdf_collect_selection(app, NULL, NULL, &item, NULL, NULL)) return;
	app->focused_handle = item.handle;
	app->focus_active = false;
}

static void editor_sdf_update_focus(editor_sdf_app* app, f32 dt) {
	se_editor_item item = {0};
	if (!app || app->editing) return;
	if (editor_sdf_collect_selection(app, NULL, NULL, &item, NULL, NULL) && item.handle != app->focused_handle) {
		editor_sdf_focus_selection(app, true);
	}
	if (app->focus_active && app->focus_valid) {
		const f32 t = s_min(1.0f, s_max(0.0f, dt * 25.f));
		app->camera_target = s_vec3_lerp(&app->camera_target, &app->focus_target, t);
		if (s_vec3_length(&s_vec3_sub(&app->focus_target, &app->camera_target)) < 0.03f) {
			app->camera_target = app->focus_target;
			app->focus_active = false;
		}
		editor_sdf_apply_camera(app);
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

static void editor_sdf_begin_edit(editor_sdf_app* app) {
	se_editor_item item = {0};
	const se_editor_property* properties = NULL;
	sz property_count = 0u;
	if (!editor_sdf_collect_selection(app, NULL, NULL, &item, &properties, &property_count) || property_count == 0u) return;
	app->active_pane = EDITOR_SDF_PANE_PROPERTIES;
	if (!properties[app->selected_property].editable) {
		editor_sdf_set_status(app, "%s read only", properties[app->selected_property].name);
		return;
	}
	editor_sdf_value_text(&properties[app->selected_property].value, app->edit_buffer, sizeof(app->edit_buffer), true);
	app->editing = true;
	se_editor_set_mode(app->editor, SE_EDITOR_MODE_INSERT);
	editor_sdf_set_status(app, "edit %s", properties[app->selected_property].name);
	(void)item;
}

static void editor_sdf_cancel_edit(editor_sdf_app* app) {
	if (!app) return;
	app->editing = false;
	app->edit_buffer[0] = '\0';
	se_editor_set_mode(app->editor, SE_EDITOR_MODE_NORMAL);
	editor_sdf_set_status(app, "edit canceled");
}

static void editor_sdf_commit_edit(editor_sdf_app* app) {
	se_editor_item item = {0};
	const se_editor_property* properties = NULL;
	sz property_count = 0u;
	se_editor_value value = {0};
	c8 property_name[SE_MAX_NAME_LENGTH] = {0};
	se_editor_value_type type = SE_EDITOR_VALUE_NONE;
	if (!editor_sdf_collect_selection(app, NULL, NULL, &item, &properties, &property_count) || property_count == 0u) return;
	type = properties[app->selected_property].value.type;
	snprintf(property_name, sizeof(property_name), "%s", properties[app->selected_property].name);
	if (!editor_sdf_parse_value(type, app->edit_buffer, &value)) {
		editor_sdf_set_status(app, "bad value for %s", property_name);
		return;
	}
	if (!se_editor_apply_set(app->editor, &item, property_name, &value)) {
		editor_sdf_set_status(app, "apply %s failed: %s", property_name, se_result_str(se_get_last_error()));
		return;
	}
	app->editing = false;
	app->edit_buffer[0] = '\0';
	se_editor_set_mode(app->editor, SE_EDITOR_MODE_NORMAL);
	editor_sdf_focus_selection(app, false);
	editor_sdf_set_status(app, "set %s", property_name);
}

static b8 editor_sdf_nudge_value(se_editor_value* value, u32 component, f32 amount) {
	if (!value) return false;
	switch (value->type) {
		case SE_EDITOR_VALUE_INT:
			value->int_value += (i32)(amount > 0.0f ? 1 : -1);
			return true;
		case SE_EDITOR_VALUE_UINT:
			if (amount < 0.0f && value->uint_value > 0u) value->uint_value--;
			if (amount > 0.0f) value->uint_value++;
			return true;
		case SE_EDITOR_VALUE_U64:
			if (amount < 0.0f && value->u64_value > 0u) value->u64_value--;
			if (amount > 0.0f) value->u64_value++;
			return true;
		case SE_EDITOR_VALUE_FLOAT:
			value->float_value += amount;
			return true;
		case SE_EDITOR_VALUE_DOUBLE:
			value->double_value += amount;
			return true;
		case SE_EDITOR_VALUE_VEC2:
			if (component % 2u == 0u) value->vec2_value.x += amount;
			else value->vec2_value.y += amount;
			return true;
		case SE_EDITOR_VALUE_VEC3:
			if (component % 3u == 0u) value->vec3_value.x += amount;
			else if (component % 3u == 1u) value->vec3_value.y += amount;
			else value->vec3_value.z += amount;
			return true;
		case SE_EDITOR_VALUE_VEC4:
			if (component % 4u == 0u) value->vec4_value.x += amount;
			else if (component % 4u == 1u) value->vec4_value.y += amount;
			else if (component % 4u == 2u) value->vec4_value.z += amount;
			else value->vec4_value.w += amount;
			return true;
		case SE_EDITOR_VALUE_MAT4: {
			s_vec3 position = s_mat4_get_translation(&value->mat4_value);
			if (component % 3u == 0u) position.x += amount;
			else if (component % 3u == 1u) position.y += amount;
			else position.z += amount;
			s_mat4_set_translation(&value->mat4_value, &position);
			return true;
		}
		default:
			return false;
	}
}

static void editor_sdf_nudge_property(editor_sdf_app* app, f32 amount) {
	se_editor_item item = {0};
	const se_editor_property* properties = NULL;
	sz property_count = 0u;
	se_editor_value value = {0};
	c8 property_name[SE_MAX_NAME_LENGTH] = {0};
	if (!editor_sdf_collect_selection(app, NULL, NULL, &item, &properties, &property_count) || property_count == 0u) return;
	app->active_pane = EDITOR_SDF_PANE_PROPERTIES;
	if (!properties[app->selected_property].editable) {
		editor_sdf_set_status(app, "%s read only", properties[app->selected_property].name);
		return;
	}
	value = properties[app->selected_property].value;
	snprintf(property_name, sizeof(property_name), "%s", properties[app->selected_property].name);
	if (!editor_sdf_nudge_value(&value, app->selected_component, amount)) {
		editor_sdf_set_status(app, "%s needs text edit", property_name);
		return;
	}
	if (se_editor_apply_set(app->editor, &item, property_name, &value)) {
		editor_sdf_focus_selection(app, false);
		editor_sdf_set_status(app, "%s %+.2f", property_name, amount);
	} else {
		editor_sdf_set_status(app, "apply %s failed: %s", property_name, se_result_str(se_get_last_error()));
	}
}

static void editor_sdf_text_input(se_window_handle window, const c8* utf8_text, void* user_data) {
	editor_sdf_app* app = user_data;
	sz length = 0u;
	sz add_length = 0u;
	(void)window;
	if (!app || !app->editing || !utf8_text) return;
	length = strlen(app->edit_buffer);
	add_length = strlen(utf8_text);
	if (length + add_length + 1u >= sizeof(app->edit_buffer)) return;
	memcpy(app->edit_buffer + length, utf8_text, add_length + 1u);
}

static void editor_sdf_update_edit_keys(editor_sdf_app* app) {
	sz length = 0u;
	if (!app || !app->editing) return;
	if (se_window_is_key_pressed(app->window, SE_KEY_ESCAPE)) {
		editor_sdf_cancel_edit(app);
		return;
	}
	if (se_window_is_key_pressed(app->window, SE_KEY_ENTER) || se_window_is_key_pressed(app->window, SE_KEY_KP_ENTER)) {
		editor_sdf_commit_edit(app);
		return;
	}
	if (se_window_is_key_pressed(app->window, SE_KEY_BACKSPACE)) {
		length = strlen(app->edit_buffer);
		if (length > 0u) app->edit_buffer[length - 1u] = '\0';
	}
}

static void editor_sdf_update_camera(editor_sdf_app* app, f32 dt) {
	s_vec3 move = s_vec3(0.0f, 0.0f, 0.0f);
	b8 control = false;
	b8 manual = false;
	if (!app || app->editing) return;
	control = se_window_is_key_down(app->window, SE_KEY_LEFT_CONTROL) || se_window_is_key_down(app->window, SE_KEY_RIGHT_CONTROL);
	if (control) return;
	{
		const s_vec3 forward_xz = s_vec3(sinf(app->camera_yaw), 0.0f, -cosf(app->camera_yaw));
		const s_vec3 right_xz = s_vec3(cosf(app->camera_yaw), 0.0f, sinf(app->camera_yaw));
		const f32 move_speed = (se_window_is_key_down(app->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(app->window, SE_KEY_RIGHT_SHIFT)) ? 8.0f : 4.0f;
		if (se_window_is_key_down(app->window, SE_KEY_W)) move = s_vec3_add(&move, &forward_xz);
		if (se_window_is_key_down(app->window, SE_KEY_S)) move = s_vec3_sub(&move, &forward_xz);
		if (se_window_is_key_down(app->window, SE_KEY_D)) move = s_vec3_add(&move, &right_xz);
		if (se_window_is_key_down(app->window, SE_KEY_A)) move = s_vec3_sub(&move, &right_xz);
		if (se_window_is_key_down(app->window, SE_KEY_E)) move.y += 1.0f;
		if (se_window_is_key_down(app->window, SE_KEY_Q)) move.y -= 1.0f;
		if (s_vec3_length(&move) > 0.0001f) {
			const s_vec3 direction = s_vec3_normalize(&move);
			move = s_vec3_muls(&direction, move_speed * s_max(dt, 0.0f));
			app->camera_target = s_vec3_add(&app->camera_target, &move);
			manual = true;
		}
	}
	if (se_window_is_key_down(app->window, SE_KEY_U)) { app->camera_yaw -= dt * 1.4f; manual = true; }
	if (se_window_is_key_down(app->window, SE_KEY_O)) { app->camera_yaw += dt * 1.4f; manual = true; }
	if (se_window_is_key_down(app->window, SE_KEY_Y)) { app->camera_pitch = s_min(1.35f, app->camera_pitch + dt * 1.0f); manual = true; }
	if (se_window_is_key_down(app->window, SE_KEY_P)) { app->camera_pitch = s_max(-1.35f, app->camera_pitch - dt * 1.0f); manual = true; }
	if (se_window_is_key_down(app->window, SE_KEY_Z)) { app->camera_distance = s_max(2.0f, app->camera_distance - dt * 6.0f); manual = true; }
	if (se_window_is_key_down(app->window, SE_KEY_X)) { app->camera_distance = s_min(40.0f, app->camera_distance + dt * 6.0f); manual = true; }
	if (manual) {
		app->focus_active = false;
		editor_sdf_mark_selection_seen(app);
	}
	editor_sdf_apply_camera(app);
}

static void editor_sdf_draw_text(editor_sdf_app* app, f32 x, f32 y, const c8* format, ...) {
	c8 line[256] = {0};
	va_list args;
	if (!app || app->font == S_HANDLE_NULL || !format) return;
	va_start(args, format);
	vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	se_text_draw(app->font, line, &s_vec2(x, y), &s_vec2(0.70f, 0.70f), 0.024f);
}

static void editor_sdf_draw_ui(editor_sdf_app* app) {
	const se_editor_item* items = NULL;
	const se_editor_property* properties = NULL;
	sz item_count = 0u;
	sz property_count = 0u;
	const u32 visible_items = 17u;
	const u32 visible_properties = 17u;
	u32 item_start = 0u;
	u32 property_start = 0u;
	const c8* mode = se_editor_mode_name(se_editor_get_mode(app->editor));
	if (!editor_sdf_collect_selection(app, &items, &item_count, NULL, &properties, &property_count)) {
		editor_sdf_draw_text(app, -0.96f, 0.86f, "SDF editor: no items");
		return;
	}
	if (app->selected_item > visible_items / 2u) item_start = app->selected_item - visible_items / 2u;
	if (item_start + visible_items > item_count) item_start = item_count > visible_items ? (u32)item_count - visible_items : 0u;
	if (app->selected_property > visible_properties / 2u) property_start = app->selected_property - visible_properties / 2u;
	if (property_start + visible_properties > property_count) property_start = property_count > visible_properties ? (u32)property_count - visible_properties : 0u;

	editor_sdf_draw_text(app, -0.96f, 0.90f, "SDF level editor | %s | %s", mode, app->json_path);
	editor_sdf_draw_text(app, -0.96f, 0.86f, "selected: %s @ %.2f %.2f %.2f%s",
		items[app->selected_item].label,
		app->focus_target.x, app->focus_target.y, app->focus_target.z,
		app->focus_active ? " -> camera" : "");
	editor_sdf_draw_text(app, -0.96f, 0.82f, "%s items (%zu)", app->active_pane == EDITOR_SDF_PANE_ITEMS ? ">" : " ", item_count);
	editor_sdf_draw_text(app, -0.10f, 0.82f, "%s properties (%zu)", app->active_pane == EDITOR_SDF_PANE_PROPERTIES ? ">" : " ", property_count);
	for (u32 row = 0u; row < visible_items && item_start + row < item_count; ++row) {
		const u32 index = item_start + row;
		const u32 depth = editor_sdf_item_depth(items, item_count, &items[index]);
		editor_sdf_draw_text(app, -0.96f, 0.76f - (f32)row * 0.045f, "%s%*s%s",
			index == app->selected_item ? ">" : " ",
			(i32)(depth * 2u), "",
			items[index].label);
	}
	for (u32 row = 0u; row < visible_properties && property_start + row < property_count; ++row) {
		c8 value_text[160] = {0};
		c8 component[16] = {0};
		const u32 index = property_start + row;
		editor_sdf_value_text(&properties[index].value, value_text, sizeof(value_text), false);
		if (index == app->selected_property && editor_sdf_component_count(properties[index].value.type) > 1u) {
			snprintf(component, sizeof(component), "[%s] ", editor_sdf_component_name(properties[index].value.type, app->selected_component));
		}
		editor_sdf_draw_text(app, -0.10f, 0.76f - (f32)row * 0.045f, "%s%s%s = %s%s",
			index == app->selected_property ? ">" : " ",
			properties[index].editable ? "" : "(ro) ",
			properties[index].name,
			component,
			value_text);
	}
	if (app->editing && property_count > 0u) {
		editor_sdf_draw_text(app, -0.96f, -0.74f, "edit %s: %s_", properties[app->selected_property].name, app->edit_buffer);
	} else {
		editor_sdf_draw_text(app, -0.96f, -0.74f, "%s", app->status[0] ? app->status : "ready");
	}
	editor_sdf_draw_text(app, -0.96f, -0.82f, "j/k select  h/l pane  Enter/i edit  +/- nudge  [/] component  Ctrl+S save  Ctrl+O load  Esc quit");
	editor_sdf_draw_text(app, -0.96f, -0.90f, "F focus selected  WASD/QE pan  U/O yaw  Y/P pitch  Z/X zoom  R reset camera");
}

static b8 editor_sdf_bind_shortcuts(editor_sdf_app* app) {
	b8 ok = true;
	if (!app || !app->editor) return false;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_ENTER, SE_EDITOR_SHORTCUT_MOD_NONE, "edit") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_KP_ENTER, SE_EDITOR_SHORTCUT_MOD_NONE, "edit") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_EQUAL, SE_EDITOR_SHORTCUT_MOD_NONE, "increase") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_KP_ADD, SE_EDITOR_SHORTCUT_MOD_NONE, "increase") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_MINUS, SE_EDITOR_SHORTCUT_MOD_NONE, "decrease") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_KP_SUBTRACT, SE_EDITOR_SHORTCUT_MOD_NONE, "decrease") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_LEFT_BRACKET, SE_EDITOR_SHORTCUT_MOD_NONE, "component_prev") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_RIGHT_BRACKET, SE_EDITOR_SHORTCUT_MOD_NONE, "component_next") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_F, SE_EDITOR_SHORTCUT_MOD_NONE, "focus") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_R, SE_EDITOR_SHORTCUT_MOD_NONE, "reset_camera") && ok;
	ok = se_editor_bind_key(app->editor, SE_EDITOR_MODE_NORMAL, SE_KEY_ESCAPE, SE_EDITOR_SHORTCUT_MOD_NONE, "quit") && ok;
	ok = se_editor_set_shortcut_repeat(app->editor, "increase", true) && ok;
	ok = se_editor_set_shortcut_repeat(app->editor, "decrease", true) && ok;
	return ok;
}

static b8 editor_sdf_apply_command(se_editor* editor, const se_editor_command* command, void* user_data) {
	(void)editor;
	editor_sdf_app* app = user_data;
	editor_sdf_item_kind kind = 0;
	if (!app || !command) return false;
	s_json* object = editor_sdf_item_json(app, &command->item, &kind);
	const c8* path = se_editor_value_as_text(&command->value);
	if (strcmp(command->name, "json_path") == 0) {
		if (!path || path[0] == '\0') return false;
		snprintf(app->json_path, sizeof(app->json_path), "%s", path);
		return true;
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, "json_save_file") == 0) {
		return editor_sdf_save_file(app, path && path[0] ? path : app->json_path);
	}
	if (command->type == SE_EDITOR_COMMAND_ACTION && strcmp(command->name, "json_load_file") == 0) {
		return editor_sdf_load_file(app, path && path[0] ? path : app->json_path);
	}
	if (!object || command->type == SE_EDITOR_COMMAND_ACTION) return false;
	if (kind == EDITOR_SDF_NODE) {
		s_json* shading = s_json_get(object, "shading");
		s_json* shadow = s_json_get(object, "shadow");
		s_mat4 mat4_value = s_mat4_identity;
		s_vec3 vec3_value = s_vec3(0.0f, 0.0f, 0.0f);
		f32 f32_value = 0.0f;
		u32 u32_value = 0u;
		if (strcmp(command->name, "type") == 0) {
			const c8* type = se_editor_value_as_text(&command->value);
			if (!type || !editor_sdf_set_string(object, "type", type)) return false;
			if (!editor_sdf_ensure_shape_defaults(object, type)) return false;
		} else if (strcmp(command->name, "operation") == 0) {
			const c8* operation = se_editor_value_as_text(&command->value);
			if (!operation || !editor_sdf_set_string(object, "operation", operation)) return false;
		} else if (strcmp(command->name, "operation_amount") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(object, "operation_amount", f32_value)) return false;
		} else if (strcmp(command->name, "transform") == 0 && se_editor_value_as_mat4(&command->value, &mat4_value)) {
			if (!editor_sdf_set_mat4(object, "transform", &mat4_value)) return false;
		} else if (strcmp(command->name, "radius") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(s_json_get(object, "sphere"), "radius", f32_value)) return false;
		} else if (strcmp(command->name, "size") == 0 && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			if (!editor_sdf_set_vec3(s_json_get(object, "box"), "size", vec3_value)) return false;
		} else if ((strcmp(command->name, "ambient") == 0 || strcmp(command->name, "diffuse") == 0 || strcmp(command->name, "specular") == 0) && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			if (!editor_sdf_set_vec3(shading, command->name, vec3_value)) return false;
		} else if (strcmp(command->name, "roughness") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shading, "roughness", f32_value)) return false;
		} else if (strcmp(command->name, "shading_bias") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shading, "bias", f32_value)) return false;
		} else if (strcmp(command->name, "smoothness") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shading, "smoothness", f32_value)) return false;
		} else if (strcmp(command->name, "shadow_softness") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shadow, "softness", f32_value)) return false;
		} else if (strcmp(command->name, "shadow_bias") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(shadow, "bias", f32_value)) return false;
		} else if (strcmp(command->name, "shadow_samples") == 0 && se_editor_value_as_u32(&command->value, &u32_value)) {
			if (!editor_sdf_set_number(shadow, "samples", (f32)u32_value)) return false;
		} else {
			return false;
		}
		return editor_sdf_apply_json(app);
	}
	if (kind == EDITOR_SDF_NOISE) {
		s_json* descriptor = s_json_get(object, "descriptor");
		s_vec2 vec2_value = s_vec2(0.0f, 0.0f);
		f32 f32_value = 0.0f;
		u32 u32_value = 0u;
		if (strcmp(command->name, "type") == 0) {
			const c8* type = se_editor_value_as_text(&command->value);
			if (!type || !editor_sdf_set_string(descriptor, "type", type)) return false;
		} else if (strcmp(command->name, "noise_frequency") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(descriptor, "frequency", f32_value)) return false;
		} else if (strcmp(command->name, "noise_offset") == 0 && se_editor_value_as_vec2(&command->value, &vec2_value)) {
			if (!editor_sdf_set_vec2(descriptor, "offset", vec2_value)) return false;
		} else if (strcmp(command->name, "noise_scale") == 0 && se_editor_value_as_vec2(&command->value, &vec2_value)) {
			if (!editor_sdf_set_vec2(descriptor, "scale", vec2_value)) return false;
		} else if (strcmp(command->name, "seed") == 0 && se_editor_value_as_u32(&command->value, &u32_value)) {
			if (!editor_sdf_set_number(descriptor, "seed", (f32)u32_value)) return false;
		} else {
			return false;
		}
		editor_sdf_json_remove(object, "texture");
		return editor_sdf_apply_json(app);
	}
	if (kind == EDITOR_SDF_POINT_LIGHT || kind == EDITOR_SDF_DIRECTIONAL_LIGHT) {
		s_vec3 vec3_value = s_vec3(0.0f, 0.0f, 0.0f);
		f32 f32_value = 0.0f;
		if ((strcmp(command->name, "position") == 0 || strcmp(command->name, "color") == 0 || strcmp(command->name, "direction") == 0) && se_editor_value_as_vec3(&command->value, &vec3_value)) {
			if (!editor_sdf_set_vec3(object, command->name, vec3_value)) return false;
		} else if (strcmp(command->name, "radius") == 0 && se_editor_value_as_f32(&command->value, &f32_value)) {
			if (!editor_sdf_set_number(object, "radius", f32_value)) return false;
		} else {
			return false;
		}
		return editor_sdf_apply_json(app);
	}
	return false;
}

static b8 editor_sdf_setup(editor_sdf_app* app) {
	se_editor_config config = SE_EDITOR_CONFIG_DEFAULTS;
	if (!app) return false;
	se_paths_set_resource_root(RESOURCES_DIR);
	app->context = se_context_create();
	app->window = se_window_create("Syphax - Editor SDF", 1280, 720);
	if (app->window == S_HANDLE_NULL) return false;
	se_window_set_target_fps(app->window, 60);
	se_render_set_background(s_vec4(0.04f, 0.05f, 0.07f, 1.0f));
	app->font = se_font_load(SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 22.0f);
	if (app->font == S_HANDLE_NULL) return false;
	app->camera = se_camera_create();
	se_camera_set_window_aspect(app->camera, app->window);
	se_camera_set_perspective(app->camera, 50.0f, 0.05f, 100.0f);
	app->camera_target = s_vec3(0.0f, 0.3f, 0.0f);
	app->camera_yaw = 0.54f;
	app->camera_pitch = 0.30f;
	app->camera_distance = 8.0f;
	app->focused_handle = S_HANDLE_NULL;
	editor_sdf_apply_camera(app);
	app->scene = se_sdf_create();
	if (!editor_sdf_load_file(app, EDITOR_SDF_PATH)) return false;
	config.context = app->context;
	config.window = app->window;
	config.custom_user_data = app;
	config.collect_custom_items = editor_sdf_collect_items;
	config.collect_custom_properties = editor_sdf_collect_properties;
	config.apply_custom_command = editor_sdf_apply_command;
	app->editor = se_editor_create(&config);
	se_window_set_text_callback(app->window, editor_sdf_text_input, app);
	editor_sdf_set_status(app, "ready");
	return app->editor != NULL;
}

static void editor_sdf_cleanup(editor_sdf_app* app) {
	if (!app) return;
	if (app->editor) se_editor_destroy(app->editor);
	if (app->scene != SE_SDF_NULL) se_sdf_destroy(app->scene);
	if (app->font != S_HANDLE_NULL) se_font_destroy(app->font);
	s_json_free(app->root);
	if (app->context) se_context_destroy(app->context);
}

static b8 editor_sdf_autotest(editor_sdf_app* app) {
	const se_editor_item* items = NULL;
	sz item_count = 0u;
	se_editor_shortcut_event event = {0};
	if (!se_editor_collect_items(app->editor, se_editor_category_to_mask(SE_EDITOR_CATEGORY_CUSTOM), &items, &item_count) || item_count == 0u) return false;
	se_window_clear_input_state(app->window);
	se_window_inject_key_state(app->window, SE_KEY_LEFT_CONTROL, true);
	se_window_inject_key_state(app->window, SE_KEY_S, true);
	if (!se_editor_update_shortcuts(app->editor) || !se_editor_poll_shortcut(app->editor, &event) || strcmp(event.action, "save") != 0) return false;
	se_window_clear_input_state(app->window);
	se_editor_item root_item = items[0];
	se_editor_value value = se_editor_value_float(0.5f);
	if (!se_editor_apply_set(app->editor, &root_item, "operation_amount", &value)) return false;
	if (!editor_sdf_save_file(app, "/tmp/editor_sdf_autotest.json")) return false;
	if (!editor_sdf_load_file(app, "/tmp/editor_sdf_autotest.json")) return false;
	printf("editor_sdf :: autotest custom_items=%zu ok\n", item_count);
	return true;
}

static void editor_sdf_handle_shortcut(editor_sdf_app* app, const se_editor_shortcut_event* event) {
	const se_editor_item* items = NULL;
	const se_editor_property* properties = NULL;
	sz item_count = 0u;
	sz property_count = 0u;
	if (!app || !event) {
		return;
	}
	if (strcmp(event->action, "normal_mode") == 0) {
		editor_sdf_cancel_edit(app);
		return;
	}
	if (app->editing) {
		return;
	}
	editor_sdf_collect_selection(app, &items, &item_count, NULL, &properties, &property_count);
	if (strcmp(event->action, "save") == 0) {
		const b8 ok = editor_sdf_save_file(app, app->json_path);
		editor_sdf_set_status(app, "save %s: %s", ok ? "ok" : "failed", ok ? app->json_path : se_result_str(se_get_last_error()));
		printf("editor_sdf :: %s\n", app->status);
	} else if (strcmp(event->action, "load") == 0) {
		const b8 ok = editor_sdf_load_file(app, app->json_path);
		if (ok) {
			app->selected_item = 0u;
			app->selected_property = 0u;
			app->focused_handle = S_HANDLE_NULL;
			app->focus_valid = false;
			app->focus_active = false;
		}
		editor_sdf_set_status(app, "load %s: %s", ok ? "ok" : "failed", ok ? app->json_path : se_result_str(se_get_last_error()));
		printf("editor_sdf :: %s\n", app->status);
	} else if (strcmp(event->action, "quit") == 0) {
		se_window_set_should_close(app->window, true);
	} else if (strcmp(event->action, "insert_mode") == 0 || strcmp(event->action, "edit") == 0) {
		editor_sdf_begin_edit(app);
	} else if (strcmp(event->action, "move_left") == 0) {
		app->active_pane = EDITOR_SDF_PANE_ITEMS;
	} else if (strcmp(event->action, "move_right") == 0) {
		app->active_pane = EDITOR_SDF_PANE_PROPERTIES;
	} else if (strcmp(event->action, "move_down") == 0) {
		if (app->active_pane == EDITOR_SDF_PANE_PROPERTIES && property_count > 0u) {
			app->selected_property = (u32)s_min(property_count - 1u, (sz)app->selected_property + 1u);
		} else if (item_count > 0u) {
			app->selected_item = (u32)s_min(item_count - 1u, (sz)app->selected_item + 1u);
			app->selected_property = 0u;
		}
	} else if (strcmp(event->action, "move_up") == 0) {
		if (app->active_pane == EDITOR_SDF_PANE_PROPERTIES && property_count > 0u) {
			app->selected_property = app->selected_property > 0u ? app->selected_property - 1u : 0u;
		} else if (item_count > 0u) {
			app->selected_item = app->selected_item > 0u ? app->selected_item - 1u : 0u;
			app->selected_property = 0u;
		}
	} else if (strcmp(event->action, "next") == 0 || strcmp(event->action, "previous") == 0) {
		app->active_pane = app->active_pane == EDITOR_SDF_PANE_ITEMS ? EDITOR_SDF_PANE_PROPERTIES : EDITOR_SDF_PANE_ITEMS;
	} else if (strcmp(event->action, "first") == 0) {
		if (app->active_pane == EDITOR_SDF_PANE_PROPERTIES) app->selected_property = 0u;
		else {
			app->selected_item = 0u;
			app->selected_property = 0u;
		}
	} else if (strcmp(event->action, "last") == 0) {
		if (app->active_pane == EDITOR_SDF_PANE_PROPERTIES && property_count > 0u) app->selected_property = (u32)property_count - 1u;
		else if (item_count > 0u) {
			app->selected_item = (u32)item_count - 1u;
			app->selected_property = 0u;
		}
	} else if (strcmp(event->action, "increase") == 0) {
		editor_sdf_nudge_property(app, se_window_is_key_down(app->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(app->window, SE_KEY_RIGHT_SHIFT) ? 1.0f : 0.1f);
	} else if (strcmp(event->action, "decrease") == 0) {
		editor_sdf_nudge_property(app, se_window_is_key_down(app->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(app->window, SE_KEY_RIGHT_SHIFT) ? -1.0f : -0.1f);
	} else if (strcmp(event->action, "component_prev") == 0) {
		u32 count = property_count > 0u ? editor_sdf_component_count(properties[app->selected_property].value.type) : 1u;
		app->selected_component = app->selected_component == 0u ? count - 1u : app->selected_component - 1u;
	} else if (strcmp(event->action, "component_next") == 0) {
		u32 count = property_count > 0u ? editor_sdf_component_count(properties[app->selected_property].value.type) : 1u;
		app->selected_component = (app->selected_component + 1u) % count;
	} else if (strcmp(event->action, "focus") == 0) {
		editor_sdf_focus_selection(app, true);
	} else if (strcmp(event->action, "reset_camera") == 0) {
		app->camera_target = s_vec3(0.0f, 0.3f, 0.0f);
		app->camera_yaw = 0.54f;
		app->camera_pitch = 0.30f;
		app->camera_distance = 8.0f;
		editor_sdf_mark_selection_seen(app);
		editor_sdf_apply_camera(app);
		editor_sdf_set_status(app, "camera reset");
	}
}

int main(int argc, char** argv) {
	editor_sdf_app app = {0};
	const b8 autotest = argc > 1 && strcmp(argv[1], "--autotest") == 0;
	if (!editor_sdf_setup(&app)) {
		printf("editor_sdf :: setup failed (%s)\n", se_result_str(se_get_last_error()));
		editor_sdf_cleanup(&app);
		return 1;
	}
	if (!se_editor_bind_default_shortcuts(app.editor)) {
		printf("editor_sdf :: shortcut bind failed (%s)\n", se_result_str(se_get_last_error()));
		editor_sdf_cleanup(&app);
		return 1;
	}
	if (!editor_sdf_bind_shortcuts(&app)) {
		printf("editor_sdf :: sdf shortcut bind failed (%s)\n", se_result_str(se_get_last_error()));
		editor_sdf_cleanup(&app);
		return 1;
	}
	if (autotest) {
		const b8 ok = editor_sdf_autotest(&app);
		editor_sdf_cleanup(&app);
		return ok ? 0 : 1;
	}
	printf("editor_sdf controls:\n");
	printf("  j/k select, h/l pane, Enter/i edit, +/- nudge, [/] component\n");
	printf("  F focus, WASD/QE pan, U/O yaw, Y/P pitch, Z/X zoom, R reset, Ctrl+S save, Ctrl+O load, Esc quit\n");
	while (!se_window_should_close(app.window)) {
		se_editor_shortcut_event event = {0};
		f32 dt = 0.0f;
		se_window_begin_frame(app.window);
		dt = (f32)se_window_get_delta_time(app.window);
		se_editor_update_shortcuts(app.editor);
		while (se_editor_poll_shortcut(app.editor, &event)) {
			editor_sdf_handle_shortcut(&app, &event);
		}
		editor_sdf_update_edit_keys(&app);
		editor_sdf_update_camera(&app, dt);
		editor_sdf_update_focus(&app, dt);
		se_render_clear();
		se_sdf_render_to_window(app.scene, app.camera, app.window, 0.1f);
		editor_sdf_draw_ui(&app);
		se_window_end_frame(app.window);
	}
	editor_sdf_cleanup(&app);
	return 0;
}
