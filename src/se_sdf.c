// Syphax-Engine - Ougi Washi

#include "se_sdf.h"
#include "se_camera.h"
#include "se_graphics.h"
#include "se_model.h"
#include "se_quad.h"
#include "se_shader.h"
#include "se_texture.h"
#include "se_scene.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"
#include "render/se_gl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>

static const char* se_sdf_fullscreen_vertex_shader =
	"#version 330 core\n"
	"layout(location = 0) in vec3 a_position;\n"
	"layout(location = 1) in vec3 a_normal;\n"
	"layout(location = 2) in vec2 a_uv;\n"
	"out vec2 v_uv;\n"
	"void main(void) {\n"
	"	v_uv = a_uv;\n"
	"	gl_Position = vec4(a_position.xy, 0.0, 1.0);\n"
	"}\n";

#define SE_SDF_JSON_FORMAT "se_sdf_json"

enum {
	SE_SDF_JSON_VERSION = 1u
};

typedef struct {
	se_sdf_scene_handle owner_scene;
	s_mat4 transform;
	se_sdf_operation operation;
	f32 operation_amount;
	se_sdf_primitive_desc primitive;
	se_sdf_node_handle parent;
	b8 is_group;
	s_array(se_sdf_node_handle, children);
	s_vec3 control_translation;
	s_vec3 control_rotation;
	s_vec3 control_scale;
	b8 control_trs_initialized;
} se_sdf_node;
typedef s_array(se_sdf_node, se_sdf_nodes);

typedef struct se_sdf_scene {
	se_sdf_nodes nodes;
	se_sdf_node_handle root;
	se_sdf_renderer_handle implicit_renderer;
	se_camera_handle implicit_camera;
} se_sdf_scene;
typedef s_array(se_sdf_scene, se_sdf_scenes);

typedef struct {
	se_sdf_scene_handle scene;
} se_sdf_scene_object_2d_data;

typedef struct {
	se_sdf_scene_handle scene;
} se_sdf_scene_object_3d_data;

typedef enum {
	SE_SDF_NODE_BIND_TRANSLATION,
	SE_SDF_NODE_BIND_ROTATION,
	SE_SDF_NODE_BIND_SCALE
} se_sdf_node_bind_target;

typedef struct {
	se_sdf_scene_handle scene;
	se_sdf_node_handle node;
	se_sdf_control_handle control;
	se_sdf_node_bind_target target;
} se_sdf_node_binding;
typedef s_array(se_sdf_node_binding, se_sdf_node_bindings);

typedef struct {
	se_sdf_scene_handle scene;
	se_sdf_node_handle node;
	se_sdf_primitive_param param;
	se_sdf_control_handle control;
} se_sdf_primitive_binding;
typedef s_array(se_sdf_primitive_binding, se_sdf_primitive_bindings);

typedef union {
	f32 f;
	s_vec2 vec2;
	s_vec3 vec3;
	s_vec4 vec4;
	i32 i;
	s_mat4 mat4;
} se_sdf_control_value;

typedef struct {
	char name[64];
	char uniform_name[96];
	se_sdf_control_type type;
	se_sdf_control_value value;
	se_sdf_control_value last_uploaded_value;
	union {
		const f32* f;
		const s_vec2* vec2;
		const s_vec3* vec3;
		const s_vec4* vec4;
		const i32* i;
		const s_mat4* mat4;
	} binding;
	i32 cached_uniform_location;
	b8 has_binding;
	b8 has_last_uploaded_value;
	b8 dirty;
} se_sdf_control;
typedef s_array(se_sdf_control, se_sdf_controls);

typedef struct se_sdf_renderer {
	se_sdf_renderer_desc desc;
	se_sdf_raymarch_quality quality;
	se_sdf_renderer_debug debug;
	se_sdf_scene_handle scene;
	se_sdf_controls controls;
	se_sdf_node_bindings node_bindings;
	se_sdf_primitive_bindings primitive_bindings;
	se_sdf_string generated_fragment_source;
	sz last_uniform_write_count;
	sz total_uniform_write_count;
	se_shader_handle shader;
	se_quad quad;
	b8 quad_ready;
	se_sdf_material_desc material;
	se_sdf_stylized_desc stylized;
	s_vec3 lighting_direction;
	s_vec3 lighting_color;
	s_vec3 fog_color;
	f32 fog_density;
	se_sdf_control_handle material_base_color_control;
	se_sdf_control_handle lighting_direction_control;
	se_sdf_control_handle lighting_color_control;
	se_sdf_control_handle fog_color_control;
	se_sdf_control_handle fog_density_control;
	se_sdf_control_handle stylized_band_levels_control;
	se_sdf_control_handle stylized_rim_power_control;
	se_sdf_control_handle stylized_rim_strength_control;
	se_sdf_control_handle stylized_fill_strength_control;
	se_sdf_control_handle stylized_back_strength_control;
	se_sdf_control_handle stylized_checker_scale_control;
	se_sdf_control_handle stylized_checker_strength_control;
	se_sdf_control_handle stylized_ground_blend_control;
	se_sdf_control_handle stylized_desaturate_control;
	se_sdf_control_handle stylized_gamma_control;
	se_sdf_build_diagnostics diagnostics;
	u64 render_generation;
	b8 built;
} se_sdf_renderer;
typedef s_array(se_sdf_renderer, se_sdf_renderers);

static se_sdf_renderer* se_sdf_codegen_active_renderer = NULL;

static void se_sdf_control_clear_binding(se_sdf_control* control);
static b8 se_sdf_control_value_equals(
	se_sdf_control_type type,
	const se_sdf_control_value* a,
	const se_sdf_control_value* b
);
static b8 se_sdf_is_valid_primitive_type(const se_sdf_primitive_type type);
static b8 se_sdf_validate_primitive_desc(const se_sdf_primitive_desc* primitive);
static void se_sdf_sync_control_bindings(se_sdf_renderer* renderer);
static b8 se_sdf_apply_node_bindings(se_sdf_renderer* renderer);
static b8 se_sdf_apply_primitive_bindings(se_sdf_renderer* renderer);
static void se_sdf_apply_renderer_shading_bindings(se_sdf_renderer* renderer);
static b8 se_sdf_emit_control_uniform_declarations(
	se_sdf_renderer* renderer,
	se_sdf_string* out
);
static const char* se_sdf_find_node_binding_uniform_name(
	se_sdf_renderer* renderer,
	se_sdf_scene_handle scene,
	se_sdf_node_handle node,
	se_sdf_node_bind_target target
);
static const char* se_sdf_find_primitive_binding_uniform_name(
	se_sdf_renderer* renderer,
	se_sdf_scene_handle scene,
	se_sdf_node_handle node,
	se_sdf_primitive_param param
);
static b8 se_sdf_renderer_has_live_shader(const se_sdf_renderer* renderer);
static void se_sdf_renderer_release_shader(se_sdf_renderer* renderer);
static void se_sdf_renderer_release_quad(se_sdf_renderer* renderer);
static void se_sdf_renderer_invalidate_gpu_state(se_sdf_renderer* renderer);
static void se_sdf_renderer_refresh_context_state(se_sdf_renderer* renderer);
static s_vec3 se_sdf_mul_mat4_point(const s_mat4* mat, const s_vec3* point);
static void se_sdf_scene_bounds_expand_point(se_sdf_scene_bounds* bounds, const s_vec3* point);
static void se_sdf_scene_bounds_expand_transformed_aabb(
	se_sdf_scene_bounds* bounds,
	const s_mat4* transform,
	const s_vec3* local_min,
	const s_vec3* local_max
);
static b8 se_sdf_get_primitive_local_bounds(
	const se_sdf_primitive_desc* primitive,
	s_vec3* out_min,
	s_vec3* out_max,
	b8* out_unbounded
);
static void se_sdf_collect_scene_bounds_recursive(
	se_sdf_scene* scene_ptr,
	se_sdf_scene_handle scene,
	se_sdf_node_handle node_handle,
	const s_mat4* parent_transform,
	se_sdf_scene_bounds* bounds,
	sz depth,
	sz max_depth
);
static s_mat4 se_sdf_transform_trs(
	f32 tx,
	f32 ty,
	f32 tz,
	f32 rx,
	f32 ry,
	f32 rz,
	f32 sx,
	f32 sy,
	f32 sz
);
static s_mat4 se_sdf_transform_grid_cell(
	i32 index,
	i32 columns,
	i32 rows,
	f32 spacing,
	f32 y,
	f32 yaw,
	f32 sx,
	f32 sy,
	f32 sz
);

typedef s_array(se_sdf_node_handle, se_sdf_node_handles);

static b8 se_sdf_json_add_child(s_json* parent, s_json* child) {
	if (!parent || !child || !s_json_add(parent, child)) {
		if (child) {
			s_json_free(child);
		}
		return 0;
	}
	return 1;
}

static s_json* se_sdf_json_add_object(s_json* parent, const c8* name) {
	s_json* object = s_json_object_empty(name);
	if (!object) {
		return NULL;
	}
	if (!se_sdf_json_add_child(parent, object)) {
		return NULL;
	}
	return object;
}

static s_json* se_sdf_json_add_array(s_json* parent, const c8* name) {
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return NULL;
	}
	if (!se_sdf_json_add_child(parent, array)) {
		return NULL;
	}
	return array;
}

static b8 se_sdf_json_add_u32(s_json* parent, const c8* name, const u32 value) {
	s_json* node = s_json_num(name, (f64)value);
	return se_sdf_json_add_child(parent, node);
}

static b8 se_sdf_json_add_i32(s_json* parent, const c8* name, const i32 value) {
	s_json* node = s_json_num(name, (f64)value);
	return se_sdf_json_add_child(parent, node);
}

static b8 se_sdf_json_add_f32(s_json* parent, const c8* name, const f32 value) {
	if (!isfinite((f64)value)) {
		return 0;
	}
	s_json* node = s_json_num(name, (f64)value);
	return se_sdf_json_add_child(parent, node);
}

static b8 se_sdf_json_add_bool(s_json* parent, const c8* name, const b8 value) {
	s_json* node = s_json_bool(name, value);
	return se_sdf_json_add_child(parent, node);
}

static b8 se_sdf_json_add_string(s_json* parent, const c8* name, const c8* value) {
	s_json* node = s_json_str(name, value ? value : "");
	return se_sdf_json_add_child(parent, node);
}

static b8 se_sdf_json_add_vec2(s_json* parent, const c8* name, const s_vec2* value) {
	if (!parent || !value) {
		return 0;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return 0;
	}
	if (!se_sdf_json_add_f32(array, NULL, value->x) ||
		!se_sdf_json_add_f32(array, NULL, value->y)) {
		s_json_free(array);
		return 0;
	}
	if (!se_sdf_json_add_child(parent, array)) {
		return 0;
	}
	return 1;
}

static b8 se_sdf_json_add_vec3(s_json* parent, const c8* name, const s_vec3* value) {
	if (!parent || !value) {
		return 0;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return 0;
	}
	if (!se_sdf_json_add_f32(array, NULL, value->x) ||
		!se_sdf_json_add_f32(array, NULL, value->y) ||
		!se_sdf_json_add_f32(array, NULL, value->z)) {
		s_json_free(array);
		return 0;
	}
	if (!se_sdf_json_add_child(parent, array)) {
		return 0;
	}
	return 1;
}

static b8 se_sdf_json_add_mat4(s_json* parent, const c8* name, const s_mat4* value) {
	if (!parent || !value) {
		return 0;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return 0;
	}
	for (u32 row = 0; row < 4u; ++row) {
		for (u32 col = 0; col < 4u; ++col) {
			if (!se_sdf_json_add_f32(array, NULL, value->m[row][col])) {
				s_json_free(array);
				return 0;
			}
		}
	}
	if (!se_sdf_json_add_child(parent, array)) {
		return 0;
	}
	return 1;
}

static b8 se_sdf_json_number_to_u32(const s_json* node, u32* out_value) {
	if (!node || node->type != S_JSON_NUMBER || !out_value) {
		return 0;
	}
	if (!isfinite(node->as.number) || node->as.number < 0.0 || node->as.number > 4294967295.0) {
		return 0;
	}
	const f64 rounded = floor(node->as.number + 0.5);
	if (fabs(node->as.number - rounded) > 0.0001) {
		return 0;
	}
	*out_value = (u32)rounded;
	return 1;
}

static b8 se_sdf_json_number_to_i32(const s_json* node, i32* out_value) {
	if (!node || node->type != S_JSON_NUMBER || !out_value) {
		return 0;
	}
	if (!isfinite(node->as.number) || node->as.number < (f64)INT_MIN || node->as.number > (f64)INT_MAX) {
		return 0;
	}
	const f64 rounded = floor(node->as.number + ((node->as.number >= 0.0) ? 0.5 : -0.5));
	if (fabs(node->as.number - rounded) > 0.0001) {
		return 0;
	}
	*out_value = (i32)rounded;
	return 1;
}

static b8 se_sdf_json_number_to_f32(const s_json* node, f32* out_value) {
	if (!node || node->type != S_JSON_NUMBER || !out_value) {
		return 0;
	}
	if (!isfinite(node->as.number)) {
		return 0;
	}
	*out_value = (f32)node->as.number;
	return 1;
}

static const s_json* se_sdf_json_get_required(const s_json* object, const c8* key, const s_json_type type) {
	if (!object || object->type != S_JSON_OBJECT || !key) {
		return NULL;
	}
	const s_json* child = s_json_get(object, key);
	if (!child || child->type != type) {
		return NULL;
	}
	return child;
}

static b8 se_sdf_json_get_u32(const s_json* object, const c8* key, u32* out_value) {
	const s_json* node = se_sdf_json_get_required(object, key, S_JSON_NUMBER);
	return se_sdf_json_number_to_u32(node, out_value);
}

static b8 se_sdf_json_get_i32(const s_json* object, const c8* key, i32* out_value) {
	const s_json* node = se_sdf_json_get_required(object, key, S_JSON_NUMBER);
	return se_sdf_json_number_to_i32(node, out_value);
}

static b8 se_sdf_json_get_f32(const s_json* object, const c8* key, f32* out_value) {
	const s_json* node = se_sdf_json_get_required(object, key, S_JSON_NUMBER);
	return se_sdf_json_number_to_f32(node, out_value);
}

static b8 se_sdf_json_get_bool(const s_json* object, const c8* key, b8* out_value) {
	const s_json* node = se_sdf_json_get_required(object, key, S_JSON_BOOL);
	if (!node || !out_value) {
		return 0;
	}
	*out_value = node->as.boolean;
	return 1;
}

static b8 se_sdf_json_get_string(const s_json* object, const c8* key, const c8** out_value) {
	const s_json* node = se_sdf_json_get_required(object, key, S_JSON_STRING);
	if (!node || !out_value) {
		return 0;
	}
	*out_value = node->as.string;
	return 1;
}

static b8 se_sdf_json_read_vec2_node(const s_json* node, s_vec2* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 2u) {
		return 0;
	}
	f32 values[2] = {0.0f, 0.0f};
	for (u32 i = 0; i < 2u; ++i) {
		const s_json* item = s_json_at(node, i);
		if (!se_sdf_json_number_to_f32(item, &values[i])) {
			return 0;
		}
	}
	*out_value = s_vec2(values[0], values[1]);
	return 1;
}

static b8 se_sdf_json_read_vec3_node(const s_json* node, s_vec3* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 3u) {
		return 0;
	}
	f32 values[3] = {0.0f, 0.0f, 0.0f};
	for (u32 i = 0; i < 3u; ++i) {
		const s_json* item = s_json_at(node, i);
		if (!se_sdf_json_number_to_f32(item, &values[i])) {
			return 0;
		}
	}
	*out_value = s_vec3(values[0], values[1], values[2]);
	return 1;
}

static b8 se_sdf_json_read_mat4_node(const s_json* node, s_mat4* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 16u) {
		return 0;
	}
	s_mat4 parsed = s_mat4_identity;
	u32 index = 0u;
	for (u32 row = 0; row < 4u; ++row) {
		for (u32 col = 0; col < 4u; ++col) {
			const s_json* item = s_json_at(node, index++);
			if (!se_sdf_json_number_to_f32(item, &parsed.m[row][col])) {
				return 0;
			}
		}
	}
	*out_value = parsed;
	return 1;
}

static b8 se_sdf_json_read_vec2_field(const s_json* object, const c8* key, s_vec2* out_value) {
	const s_json* node = se_sdf_json_get_required(object, key, S_JSON_ARRAY);
	return se_sdf_json_read_vec2_node(node, out_value);
}

static b8 se_sdf_json_read_vec3_field(const s_json* object, const c8* key, s_vec3* out_value) {
	const s_json* node = se_sdf_json_get_required(object, key, S_JSON_ARRAY);
	return se_sdf_json_read_vec3_node(node, out_value);
}

static b8 se_sdf_json_read_mat4_field(const s_json* object, const c8* key, s_mat4* out_value) {
	const s_json* node = se_sdf_json_get_required(object, key, S_JSON_ARRAY);
	return se_sdf_json_read_mat4_node(node, out_value);
}

static b8 se_sdf_json_add_primitive_desc(s_json* parent, const c8* name, const se_sdf_primitive_desc* primitive) {
	if (!parent || !primitive) {
		return 0;
	}
	s_json* primitive_json = se_sdf_json_add_object(parent, name);
	if (!primitive_json || !se_sdf_json_add_u32(primitive_json, "type", (u32)primitive->type)) {
		return 0;
	}

	switch (primitive->type) {
		case SE_SDF_PRIMITIVE_SPHERE:
			return se_sdf_json_add_f32(primitive_json, "radius", primitive->sphere.radius);
		case SE_SDF_PRIMITIVE_BOX:
			return se_sdf_json_add_vec3(primitive_json, "size", &primitive->box.size);
		case SE_SDF_PRIMITIVE_ROUND_BOX:
			return se_sdf_json_add_vec3(primitive_json, "size", &primitive->round_box.size) &&
				se_sdf_json_add_f32(primitive_json, "roundness", primitive->round_box.roundness);
		case SE_SDF_PRIMITIVE_BOX_FRAME:
			return se_sdf_json_add_vec3(primitive_json, "size", &primitive->box_frame.size) &&
				se_sdf_json_add_f32(primitive_json, "thickness", primitive->box_frame.thickness);
		case SE_SDF_PRIMITIVE_TORUS:
			return se_sdf_json_add_vec2(primitive_json, "radii", &primitive->torus.radii);
		case SE_SDF_PRIMITIVE_CAPPED_TORUS:
			return se_sdf_json_add_vec2(primitive_json, "axis_sin_cos", &primitive->capped_torus.axis_sin_cos) &&
				se_sdf_json_add_f32(primitive_json, "major_radius", primitive->capped_torus.major_radius) &&
				se_sdf_json_add_f32(primitive_json, "minor_radius", primitive->capped_torus.minor_radius);
		case SE_SDF_PRIMITIVE_LINK:
			return se_sdf_json_add_f32(primitive_json, "half_length", primitive->link.half_length) &&
				se_sdf_json_add_f32(primitive_json, "outer_radius", primitive->link.outer_radius) &&
				se_sdf_json_add_f32(primitive_json, "inner_radius", primitive->link.inner_radius);
		case SE_SDF_PRIMITIVE_CYLINDER:
			return se_sdf_json_add_vec3(primitive_json, "axis_and_radius", &primitive->cylinder.axis_and_radius);
		case SE_SDF_PRIMITIVE_CONE:
			return se_sdf_json_add_vec2(primitive_json, "angle_sin_cos", &primitive->cone.angle_sin_cos) &&
				se_sdf_json_add_f32(primitive_json, "height", primitive->cone.height);
		case SE_SDF_PRIMITIVE_CONE_INFINITE:
			return se_sdf_json_add_vec2(primitive_json, "angle_sin_cos", &primitive->cone_infinite.angle_sin_cos);
		case SE_SDF_PRIMITIVE_PLANE:
			return se_sdf_json_add_vec3(primitive_json, "normal", &primitive->plane.normal) &&
				se_sdf_json_add_f32(primitive_json, "offset", primitive->plane.offset);
		case SE_SDF_PRIMITIVE_HEX_PRISM:
			return se_sdf_json_add_vec2(primitive_json, "half_size", &primitive->hex_prism.half_size);
		case SE_SDF_PRIMITIVE_CAPSULE:
			return se_sdf_json_add_vec3(primitive_json, "a", &primitive->capsule.a) &&
				se_sdf_json_add_vec3(primitive_json, "b", &primitive->capsule.b) &&
				se_sdf_json_add_f32(primitive_json, "radius", primitive->capsule.radius);
		case SE_SDF_PRIMITIVE_VERTICAL_CAPSULE:
			return se_sdf_json_add_f32(primitive_json, "height", primitive->vertical_capsule.height) &&
				se_sdf_json_add_f32(primitive_json, "radius", primitive->vertical_capsule.radius);
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER:
			return se_sdf_json_add_f32(primitive_json, "radius", primitive->capped_cylinder.radius) &&
				se_sdf_json_add_f32(primitive_json, "half_height", primitive->capped_cylinder.half_height);
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY:
			return se_sdf_json_add_vec3(primitive_json, "a", &primitive->capped_cylinder_arbitrary.a) &&
				se_sdf_json_add_vec3(primitive_json, "b", &primitive->capped_cylinder_arbitrary.b) &&
				se_sdf_json_add_f32(primitive_json, "radius", primitive->capped_cylinder_arbitrary.radius);
		case SE_SDF_PRIMITIVE_ROUNDED_CYLINDER:
			return se_sdf_json_add_f32(primitive_json, "outer_radius", primitive->rounded_cylinder.outer_radius) &&
				se_sdf_json_add_f32(primitive_json, "corner_radius", primitive->rounded_cylinder.corner_radius) &&
				se_sdf_json_add_f32(primitive_json, "half_height", primitive->rounded_cylinder.half_height);
		case SE_SDF_PRIMITIVE_CAPPED_CONE:
			return se_sdf_json_add_f32(primitive_json, "height", primitive->capped_cone.height) &&
				se_sdf_json_add_f32(primitive_json, "radius_base", primitive->capped_cone.radius_base) &&
				se_sdf_json_add_f32(primitive_json, "radius_top", primitive->capped_cone.radius_top);
		case SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY:
			return se_sdf_json_add_vec3(primitive_json, "a", &primitive->capped_cone_arbitrary.a) &&
				se_sdf_json_add_vec3(primitive_json, "b", &primitive->capped_cone_arbitrary.b) &&
				se_sdf_json_add_f32(primitive_json, "radius_a", primitive->capped_cone_arbitrary.radius_a) &&
				se_sdf_json_add_f32(primitive_json, "radius_b", primitive->capped_cone_arbitrary.radius_b);
		case SE_SDF_PRIMITIVE_SOLID_ANGLE:
			return se_sdf_json_add_vec2(primitive_json, "angle_sin_cos", &primitive->solid_angle.angle_sin_cos) &&
				se_sdf_json_add_f32(primitive_json, "radius", primitive->solid_angle.radius);
		case SE_SDF_PRIMITIVE_CUT_SPHERE:
			return se_sdf_json_add_f32(primitive_json, "radius", primitive->cut_sphere.radius) &&
				se_sdf_json_add_f32(primitive_json, "cut_height", primitive->cut_sphere.cut_height);
		case SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE:
			return se_sdf_json_add_f32(primitive_json, "radius", primitive->cut_hollow_sphere.radius) &&
				se_sdf_json_add_f32(primitive_json, "cut_height", primitive->cut_hollow_sphere.cut_height) &&
				se_sdf_json_add_f32(primitive_json, "thickness", primitive->cut_hollow_sphere.thickness);
		case SE_SDF_PRIMITIVE_DEATH_STAR:
			return se_sdf_json_add_f32(primitive_json, "radius_a", primitive->death_star.radius_a) &&
				se_sdf_json_add_f32(primitive_json, "radius_b", primitive->death_star.radius_b) &&
				se_sdf_json_add_f32(primitive_json, "separation", primitive->death_star.separation);
		case SE_SDF_PRIMITIVE_ROUND_CONE:
			return se_sdf_json_add_f32(primitive_json, "radius_base", primitive->round_cone.radius_base) &&
				se_sdf_json_add_f32(primitive_json, "radius_top", primitive->round_cone.radius_top) &&
				se_sdf_json_add_f32(primitive_json, "height", primitive->round_cone.height);
		case SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY:
			return se_sdf_json_add_vec3(primitive_json, "a", &primitive->round_cone_arbitrary.a) &&
				se_sdf_json_add_vec3(primitive_json, "b", &primitive->round_cone_arbitrary.b) &&
				se_sdf_json_add_f32(primitive_json, "radius_a", primitive->round_cone_arbitrary.radius_a) &&
				se_sdf_json_add_f32(primitive_json, "radius_b", primitive->round_cone_arbitrary.radius_b);
		case SE_SDF_PRIMITIVE_VESICA_SEGMENT:
			return se_sdf_json_add_vec3(primitive_json, "a", &primitive->vesica_segment.a) &&
				se_sdf_json_add_vec3(primitive_json, "b", &primitive->vesica_segment.b) &&
				se_sdf_json_add_f32(primitive_json, "width", primitive->vesica_segment.width);
		case SE_SDF_PRIMITIVE_RHOMBUS:
			return se_sdf_json_add_f32(primitive_json, "length_a", primitive->rhombus.length_a) &&
				se_sdf_json_add_f32(primitive_json, "length_b", primitive->rhombus.length_b) &&
				se_sdf_json_add_f32(primitive_json, "height", primitive->rhombus.height) &&
				se_sdf_json_add_f32(primitive_json, "corner_radius", primitive->rhombus.corner_radius);
		case SE_SDF_PRIMITIVE_OCTAHEDRON:
			return se_sdf_json_add_f32(primitive_json, "size", primitive->octahedron.size);
		case SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND:
			return se_sdf_json_add_f32(primitive_json, "size", primitive->octahedron_bound.size);
		case SE_SDF_PRIMITIVE_PYRAMID:
			return se_sdf_json_add_f32(primitive_json, "height", primitive->pyramid.height);
		case SE_SDF_PRIMITIVE_TRIANGLE:
			return se_sdf_json_add_vec3(primitive_json, "a", &primitive->triangle.a) &&
				se_sdf_json_add_vec3(primitive_json, "b", &primitive->triangle.b) &&
				se_sdf_json_add_vec3(primitive_json, "c", &primitive->triangle.c);
		case SE_SDF_PRIMITIVE_QUAD:
			return se_sdf_json_add_vec3(primitive_json, "a", &primitive->quad.a) &&
				se_sdf_json_add_vec3(primitive_json, "b", &primitive->quad.b) &&
				se_sdf_json_add_vec3(primitive_json, "c", &primitive->quad.c) &&
				se_sdf_json_add_vec3(primitive_json, "d", &primitive->quad.d);
		default:
			return 0;
	}
}

static b8 se_sdf_json_read_primitive_desc(const s_json* primitive_json, se_sdf_primitive_desc* out_primitive) {
	if (!primitive_json || primitive_json->type != S_JSON_OBJECT || !out_primitive) {
		return 0;
	}

	u32 type_u32 = 0u;
	if (!se_sdf_json_get_u32(primitive_json, "type", &type_u32)) {
		return 0;
	}
	se_sdf_primitive_desc primitive = {0};
	primitive.type = (se_sdf_primitive_type)type_u32;
	if (!se_sdf_is_valid_primitive_type(primitive.type)) {
		return 0;
	}

	switch (primitive.type) {
		case SE_SDF_PRIMITIVE_SPHERE:
			if (!se_sdf_json_get_f32(primitive_json, "radius", &primitive.sphere.radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_BOX:
			if (!se_sdf_json_read_vec3_field(primitive_json, "size", &primitive.box.size)) return 0;
			break;
		case SE_SDF_PRIMITIVE_ROUND_BOX:
			if (!se_sdf_json_read_vec3_field(primitive_json, "size", &primitive.round_box.size) ||
				!se_sdf_json_get_f32(primitive_json, "roundness", &primitive.round_box.roundness)) return 0;
			break;
		case SE_SDF_PRIMITIVE_BOX_FRAME:
			if (!se_sdf_json_read_vec3_field(primitive_json, "size", &primitive.box_frame.size) ||
				!se_sdf_json_get_f32(primitive_json, "thickness", &primitive.box_frame.thickness)) return 0;
			break;
		case SE_SDF_PRIMITIVE_TORUS:
			if (!se_sdf_json_read_vec2_field(primitive_json, "radii", &primitive.torus.radii)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_TORUS:
			if (!se_sdf_json_read_vec2_field(primitive_json, "axis_sin_cos", &primitive.capped_torus.axis_sin_cos) ||
				!se_sdf_json_get_f32(primitive_json, "major_radius", &primitive.capped_torus.major_radius) ||
				!se_sdf_json_get_f32(primitive_json, "minor_radius", &primitive.capped_torus.minor_radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_LINK:
			if (!se_sdf_json_get_f32(primitive_json, "half_length", &primitive.link.half_length) ||
				!se_sdf_json_get_f32(primitive_json, "outer_radius", &primitive.link.outer_radius) ||
				!se_sdf_json_get_f32(primitive_json, "inner_radius", &primitive.link.inner_radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CYLINDER:
			if (!se_sdf_json_read_vec3_field(primitive_json, "axis_and_radius", &primitive.cylinder.axis_and_radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CONE:
			if (!se_sdf_json_read_vec2_field(primitive_json, "angle_sin_cos", &primitive.cone.angle_sin_cos) ||
				!se_sdf_json_get_f32(primitive_json, "height", &primitive.cone.height)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CONE_INFINITE:
			if (!se_sdf_json_read_vec2_field(primitive_json, "angle_sin_cos", &primitive.cone_infinite.angle_sin_cos)) return 0;
			break;
		case SE_SDF_PRIMITIVE_PLANE:
			if (!se_sdf_json_read_vec3_field(primitive_json, "normal", &primitive.plane.normal) ||
				!se_sdf_json_get_f32(primitive_json, "offset", &primitive.plane.offset)) return 0;
			break;
		case SE_SDF_PRIMITIVE_HEX_PRISM:
			if (!se_sdf_json_read_vec2_field(primitive_json, "half_size", &primitive.hex_prism.half_size)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CAPSULE:
			if (!se_sdf_json_read_vec3_field(primitive_json, "a", &primitive.capsule.a) ||
				!se_sdf_json_read_vec3_field(primitive_json, "b", &primitive.capsule.b) ||
				!se_sdf_json_get_f32(primitive_json, "radius", &primitive.capsule.radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_VERTICAL_CAPSULE:
			if (!se_sdf_json_get_f32(primitive_json, "height", &primitive.vertical_capsule.height) ||
				!se_sdf_json_get_f32(primitive_json, "radius", &primitive.vertical_capsule.radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER:
			if (!se_sdf_json_get_f32(primitive_json, "radius", &primitive.capped_cylinder.radius) ||
				!se_sdf_json_get_f32(primitive_json, "half_height", &primitive.capped_cylinder.half_height)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY:
			if (!se_sdf_json_read_vec3_field(primitive_json, "a", &primitive.capped_cylinder_arbitrary.a) ||
				!se_sdf_json_read_vec3_field(primitive_json, "b", &primitive.capped_cylinder_arbitrary.b) ||
				!se_sdf_json_get_f32(primitive_json, "radius", &primitive.capped_cylinder_arbitrary.radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_ROUNDED_CYLINDER:
			if (!se_sdf_json_get_f32(primitive_json, "outer_radius", &primitive.rounded_cylinder.outer_radius) ||
				!se_sdf_json_get_f32(primitive_json, "corner_radius", &primitive.rounded_cylinder.corner_radius) ||
				!se_sdf_json_get_f32(primitive_json, "half_height", &primitive.rounded_cylinder.half_height)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CONE:
			if (!se_sdf_json_get_f32(primitive_json, "height", &primitive.capped_cone.height) ||
				!se_sdf_json_get_f32(primitive_json, "radius_base", &primitive.capped_cone.radius_base) ||
				!se_sdf_json_get_f32(primitive_json, "radius_top", &primitive.capped_cone.radius_top)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY:
			if (!se_sdf_json_read_vec3_field(primitive_json, "a", &primitive.capped_cone_arbitrary.a) ||
				!se_sdf_json_read_vec3_field(primitive_json, "b", &primitive.capped_cone_arbitrary.b) ||
				!se_sdf_json_get_f32(primitive_json, "radius_a", &primitive.capped_cone_arbitrary.radius_a) ||
				!se_sdf_json_get_f32(primitive_json, "radius_b", &primitive.capped_cone_arbitrary.radius_b)) return 0;
			break;
		case SE_SDF_PRIMITIVE_SOLID_ANGLE:
			if (!se_sdf_json_read_vec2_field(primitive_json, "angle_sin_cos", &primitive.solid_angle.angle_sin_cos) ||
				!se_sdf_json_get_f32(primitive_json, "radius", &primitive.solid_angle.radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CUT_SPHERE:
			if (!se_sdf_json_get_f32(primitive_json, "radius", &primitive.cut_sphere.radius) ||
				!se_sdf_json_get_f32(primitive_json, "cut_height", &primitive.cut_sphere.cut_height)) return 0;
			break;
		case SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE:
			if (!se_sdf_json_get_f32(primitive_json, "radius", &primitive.cut_hollow_sphere.radius) ||
				!se_sdf_json_get_f32(primitive_json, "cut_height", &primitive.cut_hollow_sphere.cut_height) ||
				!se_sdf_json_get_f32(primitive_json, "thickness", &primitive.cut_hollow_sphere.thickness)) return 0;
			break;
		case SE_SDF_PRIMITIVE_DEATH_STAR:
			if (!se_sdf_json_get_f32(primitive_json, "radius_a", &primitive.death_star.radius_a) ||
				!se_sdf_json_get_f32(primitive_json, "radius_b", &primitive.death_star.radius_b) ||
				!se_sdf_json_get_f32(primitive_json, "separation", &primitive.death_star.separation)) return 0;
			break;
		case SE_SDF_PRIMITIVE_ROUND_CONE:
			if (!se_sdf_json_get_f32(primitive_json, "radius_base", &primitive.round_cone.radius_base) ||
				!se_sdf_json_get_f32(primitive_json, "radius_top", &primitive.round_cone.radius_top) ||
				!se_sdf_json_get_f32(primitive_json, "height", &primitive.round_cone.height)) return 0;
			break;
		case SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY:
			if (!se_sdf_json_read_vec3_field(primitive_json, "a", &primitive.round_cone_arbitrary.a) ||
				!se_sdf_json_read_vec3_field(primitive_json, "b", &primitive.round_cone_arbitrary.b) ||
				!se_sdf_json_get_f32(primitive_json, "radius_a", &primitive.round_cone_arbitrary.radius_a) ||
				!se_sdf_json_get_f32(primitive_json, "radius_b", &primitive.round_cone_arbitrary.radius_b)) return 0;
			break;
		case SE_SDF_PRIMITIVE_VESICA_SEGMENT:
			if (!se_sdf_json_read_vec3_field(primitive_json, "a", &primitive.vesica_segment.a) ||
				!se_sdf_json_read_vec3_field(primitive_json, "b", &primitive.vesica_segment.b) ||
				!se_sdf_json_get_f32(primitive_json, "width", &primitive.vesica_segment.width)) return 0;
			break;
		case SE_SDF_PRIMITIVE_RHOMBUS:
			if (!se_sdf_json_get_f32(primitive_json, "length_a", &primitive.rhombus.length_a) ||
				!se_sdf_json_get_f32(primitive_json, "length_b", &primitive.rhombus.length_b) ||
				!se_sdf_json_get_f32(primitive_json, "height", &primitive.rhombus.height) ||
				!se_sdf_json_get_f32(primitive_json, "corner_radius", &primitive.rhombus.corner_radius)) return 0;
			break;
		case SE_SDF_PRIMITIVE_OCTAHEDRON:
			if (!se_sdf_json_get_f32(primitive_json, "size", &primitive.octahedron.size)) return 0;
			break;
		case SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND:
			if (!se_sdf_json_get_f32(primitive_json, "size", &primitive.octahedron_bound.size)) return 0;
			break;
		case SE_SDF_PRIMITIVE_PYRAMID:
			if (!se_sdf_json_get_f32(primitive_json, "height", &primitive.pyramid.height)) return 0;
			break;
		case SE_SDF_PRIMITIVE_TRIANGLE:
			if (!se_sdf_json_read_vec3_field(primitive_json, "a", &primitive.triangle.a) ||
				!se_sdf_json_read_vec3_field(primitive_json, "b", &primitive.triangle.b) ||
				!se_sdf_json_read_vec3_field(primitive_json, "c", &primitive.triangle.c)) return 0;
			break;
		case SE_SDF_PRIMITIVE_QUAD:
			if (!se_sdf_json_read_vec3_field(primitive_json, "a", &primitive.quad.a) ||
				!se_sdf_json_read_vec3_field(primitive_json, "b", &primitive.quad.b) ||
				!se_sdf_json_read_vec3_field(primitive_json, "c", &primitive.quad.c) ||
				!se_sdf_json_read_vec3_field(primitive_json, "d", &primitive.quad.d)) return 0;
			break;
		default:
			return 0;
	}

	if (!se_sdf_validate_primitive_desc(&primitive)) {
		return 0;
	}
	*out_primitive = primitive;
	return 1;
}

static b8 se_sdf_json_node_index_of(
	const se_sdf_node_handles* node_handles,
	const se_sdf_node_handle node,
	u32* out_index
) {
	if (!node_handles || !out_index || node == SE_SDF_NODE_NULL) {
		return 0;
	}
	for (u32 i = 0u; i < (u32)s_array_get_size((se_sdf_node_handles*)node_handles); ++i) {
		const se_sdf_node_handle* handle = s_array_get((se_sdf_node_handles*)node_handles, s_array_handle((se_sdf_node_handles*)node_handles, i));
		if (handle && *handle == node) {
			*out_index = i;
			return 1;
		}
	}
	return 0;
}

static b8 se_sdf_get_context(se_context** out_context) {
	se_context* ctx = se_current_context();
	if (!ctx) {
		return 0;
	}
	if (s_array_get_capacity(&ctx->sdf_scenes) == 0) {
		s_array_init(&ctx->sdf_scenes);
	}
	if (s_array_get_capacity(&ctx->sdf_renderers) == 0) {
		s_array_init(&ctx->sdf_renderers);
	}
	if (out_context) {
		*out_context = ctx;
	}
	return 1;
}

static se_sdf_renderer* se_sdf_renderer_from_handle(const se_sdf_renderer_handle renderer) {
	if (renderer == SE_SDF_RENDERER_NULL) {
		return NULL;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return NULL;
	}
	return s_array_get(&ctx->sdf_renderers, renderer);
}

static void se_sdf_set_diagnostics(
	se_sdf_renderer* renderer,
	const b8 success,
	const char* stage,
	const char* fmt,
	...
) {
	if (!renderer) {
		return;
	}
	memset(&renderer->diagnostics, 0, sizeof(renderer->diagnostics));
	renderer->diagnostics.success = success;
	if (stage) {
		strncpy(renderer->diagnostics.stage, stage, sizeof(renderer->diagnostics.stage) - 1);
	}
	if (!fmt) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	vsnprintf(renderer->diagnostics.message, sizeof(renderer->diagnostics.message), fmt, args);
	va_end(args);
}

static b8 se_sdf_renderer_has_live_shader(const se_sdf_renderer* renderer) {
	if (!renderer || renderer->shader == S_HANDLE_NULL) {
		return 0;
	}
	se_context* ctx = se_current_context();
	if (!ctx) {
		return 0;
	}
	se_shader* shader_ptr = s_array_get(&ctx->shaders, renderer->shader);
	return shader_ptr != NULL && shader_ptr->program != 0;
}

static void se_sdf_renderer_release_shader(se_sdf_renderer* renderer) {
	if (!renderer || renderer->shader == S_HANDLE_NULL) {
		return;
	}
	se_context* ctx = se_current_context();
	if (ctx && s_array_get(&ctx->shaders, renderer->shader) != NULL) {
		se_shader_destroy(renderer->shader);
	}
	renderer->shader = S_HANDLE_NULL;
}

static void se_sdf_renderer_release_quad(se_sdf_renderer* renderer) {
	if (!renderer || !renderer->quad_ready) {
		return;
	}
	if (se_render_has_context()) {
		se_quad_destroy(&renderer->quad);
	}
	memset(&renderer->quad, 0, sizeof(renderer->quad));
	renderer->quad_ready = 0;
}

static void se_sdf_renderer_invalidate_gpu_state(se_sdf_renderer* renderer) {
	if (!renderer) {
		return;
	}
	se_sdf_renderer_release_shader(renderer);
	se_sdf_renderer_release_quad(renderer);
	renderer->built = 0;
	for (sz i = 0; i < s_array_get_size(&renderer->controls); ++i) {
		se_sdf_control* control = s_array_get(&renderer->controls, s_array_handle(&renderer->controls, (u32)i));
		if (!control) {
			continue;
		}
		control->cached_uniform_location = -1;
		control->has_last_uploaded_value = 0;
		control->dirty = 1;
	}
}

static void se_sdf_renderer_refresh_context_state(se_sdf_renderer* renderer) {
	if (!renderer) {
		return;
	}
	const u64 current_generation = se_render_get_generation();
	if (renderer->render_generation != 0 && current_generation != 0 &&
		renderer->render_generation != current_generation) {
		se_sdf_renderer_invalidate_gpu_state(renderer);
	}
	renderer->render_generation = current_generation;

	if (renderer->shader != S_HANDLE_NULL && !se_sdf_renderer_has_live_shader(renderer)) {
		renderer->shader = S_HANDLE_NULL;
		renderer->built = 0;
	}
}

static se_sdf_scene* se_sdf_scene_from_handle(const se_sdf_scene_handle scene) {
	if (scene == SE_SDF_SCENE_NULL) {
		return NULL;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return NULL;
	}
	return s_array_get(&ctx->sdf_scenes, scene);
}

static void se_sdf_scene_release_implicit_resources(se_sdf_scene* scene_ptr) {
	if (!scene_ptr) {
		return;
	}
	if (scene_ptr->implicit_renderer != SE_SDF_RENDERER_NULL) {
		se_sdf_renderer_destroy(scene_ptr->implicit_renderer);
		scene_ptr->implicit_renderer = SE_SDF_RENDERER_NULL;
	}
	if (scene_ptr->implicit_camera != S_HANDLE_NULL) {
		if (se_camera_get(scene_ptr->implicit_camera)) {
			se_camera_destroy(scene_ptr->implicit_camera);
		}
		scene_ptr->implicit_camera = S_HANDLE_NULL;
	}
}

static b8 se_sdf_scene_prepare_implicit_render(
	const se_sdf_scene_handle scene,
	se_sdf_scene* scene_ptr
) {
	if (!scene_ptr) {
		return 0;
	}

	b8 created_renderer = 0;
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(scene_ptr->implicit_renderer);
	if (!renderer_ptr) {
		scene_ptr->implicit_renderer = se_sdf_renderer_create(NULL);
		if (scene_ptr->implicit_renderer == SE_SDF_RENDERER_NULL) {
			return 0;
		}
		created_renderer = 1;
		if (!se_sdf_renderer_set_scene(scene_ptr->implicit_renderer, scene)) {
			se_sdf_renderer_destroy(scene_ptr->implicit_renderer);
			scene_ptr->implicit_renderer = SE_SDF_RENDERER_NULL;
			return 0;
		}
	}

	if (scene_ptr->implicit_camera == S_HANDLE_NULL || !se_camera_get(scene_ptr->implicit_camera)) {
		scene_ptr->implicit_camera = se_camera_create();
		if (scene_ptr->implicit_camera == S_HANDLE_NULL) {
			if (created_renderer && scene_ptr->implicit_renderer != SE_SDF_RENDERER_NULL) {
				se_sdf_renderer_destroy(scene_ptr->implicit_renderer);
				scene_ptr->implicit_renderer = SE_SDF_RENDERER_NULL;
			}
			return 0;
		}
		se_camera_set_target_mode(scene_ptr->implicit_camera, 1);
		(void)se_sdf_scene_align_camera(scene, scene_ptr->implicit_camera, NULL, NULL);
	}
	return 1;
}

static se_window_handle se_sdf_first_window_handle(void) {
	se_context* ctx = se_current_context();
	if (!ctx) {
		return S_HANDLE_NULL;
	}
	for (u32 i = 0; i < (u32)s_array_get_size(&ctx->windows); ++i) {
		se_window_handle handle = s_array_handle(&ctx->windows, i);
		if (s_array_get(&ctx->windows, handle)) {
			return handle;
		}
	}
	return S_HANDLE_NULL;
}

static void se_sdf_scene_object_render(const se_sdf_scene_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	if (!se_sdf_scene_prepare_implicit_render(scene, scene_ptr)) {
		return;
	}

	GLint viewport[4] = {0, 0, 1, 1};
	glGetIntegerv(GL_VIEWPORT, viewport);
	const f32 width = viewport[2] > 0 ? (f32)viewport[2] : 1.0f;
	const f32 height = viewport[3] > 0 ? (f32)viewport[3] : 1.0f;

	se_sdf_frame_desc frame = SE_SDF_FRAME_DESC_DEFAULTS;
	frame.resolution = s_vec2(width, height);
	if (scene_ptr->implicit_camera != S_HANDLE_NULL) {
		se_camera_set_aspect(scene_ptr->implicit_camera, width, height);
		(void)se_sdf_frame_set_camera(&frame, scene_ptr->implicit_camera);
	}

	se_window_handle window = se_sdf_first_window_handle();
	if (window != S_HANDLE_NULL) {
		frame.time_seconds = (f32)se_window_get_time(window);
	}

	(void)se_sdf_renderer_render(scene_ptr->implicit_renderer, &frame);
}

static void se_sdf_scene_object_2d_render(void* data) {
	se_sdf_scene_object_2d_data* render_data = (se_sdf_scene_object_2d_data*)data;
	if (!render_data) {
		return;
	}
	se_sdf_scene_object_render(render_data->scene);
}

static void se_sdf_scene_object_3d_render(void* data) {
	se_sdf_scene_object_3d_data* render_data = (se_sdf_scene_object_3d_data*)data;
	if (!render_data) {
		return;
	}
	se_sdf_scene_object_render(render_data->scene);
}

static se_sdf_node* se_sdf_node_from_handle(
	se_sdf_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node
) {
	if (!scene_ptr || node == SE_SDF_NODE_NULL) {
		return NULL;
	}
	se_sdf_node* node_ptr = s_array_get(&scene_ptr->nodes, node);
	if (!node_ptr || node_ptr->owner_scene != scene) {
		return NULL;
	}
	return node_ptr;
}

static void se_sdf_scene_release_nodes(se_sdf_scene* scene_ptr) {
	if (!scene_ptr) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		se_sdf_node* node = s_array_get(&scene_ptr->nodes, s_array_handle(&scene_ptr->nodes, (u32)i));
		if (node) {
			s_array_clear(&node->children);
		}
	}
	s_array_clear(&scene_ptr->nodes);
	scene_ptr->root = SE_SDF_NODE_NULL;
}

static b8 se_sdf_remove_child_entry(se_sdf_node* parent_node, const se_sdf_node_handle child) {
	if (!parent_node || child == SE_SDF_NODE_NULL) {
		return 0;
	}
	for (sz i = 0; i < s_array_get_size(&parent_node->children); ++i) {
		se_sdf_node_handle* current = s_array_get(&parent_node->children, s_array_handle(&parent_node->children, (u32)i));
		if (!current || *current != child) {
			continue;
		}
		s_array_remove_ordered(&parent_node->children, s_array_handle(&parent_node->children, (u32)i));
		return 1;
	}
	return 0;
}

static b8 se_sdf_has_child_entry(se_sdf_node* parent_node, const se_sdf_node_handle child) {
	if (!parent_node || child == SE_SDF_NODE_NULL) {
		return 0;
	}
	for (sz i = 0; i < s_array_get_size(&parent_node->children); ++i) {
		se_sdf_node_handle* current = s_array_get(&parent_node->children, s_array_handle(&parent_node->children, (u32)i));
		if (current && *current == child) {
			return 1;
		}
	}
	return 0;
}

static b8 se_sdf_is_valid_primitive_type(const se_sdf_primitive_type type) {
	switch (type) {
		case SE_SDF_PRIMITIVE_SPHERE:
		case SE_SDF_PRIMITIVE_BOX:
		case SE_SDF_PRIMITIVE_ROUND_BOX:
		case SE_SDF_PRIMITIVE_BOX_FRAME:
		case SE_SDF_PRIMITIVE_TORUS:
		case SE_SDF_PRIMITIVE_CAPPED_TORUS:
		case SE_SDF_PRIMITIVE_LINK:
		case SE_SDF_PRIMITIVE_CYLINDER:
		case SE_SDF_PRIMITIVE_CONE:
		case SE_SDF_PRIMITIVE_CONE_INFINITE:
		case SE_SDF_PRIMITIVE_PLANE:
		case SE_SDF_PRIMITIVE_HEX_PRISM:
		case SE_SDF_PRIMITIVE_CAPSULE:
		case SE_SDF_PRIMITIVE_VERTICAL_CAPSULE:
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER:
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY:
		case SE_SDF_PRIMITIVE_ROUNDED_CYLINDER:
		case SE_SDF_PRIMITIVE_CAPPED_CONE:
		case SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY:
		case SE_SDF_PRIMITIVE_SOLID_ANGLE:
		case SE_SDF_PRIMITIVE_CUT_SPHERE:
		case SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE:
		case SE_SDF_PRIMITIVE_DEATH_STAR:
		case SE_SDF_PRIMITIVE_ROUND_CONE:
		case SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY:
		case SE_SDF_PRIMITIVE_VESICA_SEGMENT:
		case SE_SDF_PRIMITIVE_RHOMBUS:
		case SE_SDF_PRIMITIVE_OCTAHEDRON:
		case SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND:
		case SE_SDF_PRIMITIVE_PYRAMID:
		case SE_SDF_PRIMITIVE_TRIANGLE:
		case SE_SDF_PRIMITIVE_QUAD:
			return 1;
		default:
			return 0;
	}
}

static b8 se_sdf_vec3_all_positive(const s_vec3 v) {
	return v.x > 0.0f && v.y > 0.0f && v.z > 0.0f;
}

static b8 se_sdf_validate_primitive_desc(const se_sdf_primitive_desc* primitive) {
	if (!primitive || !se_sdf_is_valid_primitive_type(primitive->type)) {
		return 0;
	}

	switch (primitive->type) {
		case SE_SDF_PRIMITIVE_SPHERE:
			return primitive->sphere.radius > 0.0f;
		case SE_SDF_PRIMITIVE_BOX:
			return se_sdf_vec3_all_positive(primitive->box.size);
		case SE_SDF_PRIMITIVE_ROUND_BOX:
			return se_sdf_vec3_all_positive(primitive->round_box.size) &&
				primitive->round_box.roundness >= 0.0f;
		case SE_SDF_PRIMITIVE_BOX_FRAME:
			return se_sdf_vec3_all_positive(primitive->box_frame.size) &&
				primitive->box_frame.thickness >= 0.0f;
		case SE_SDF_PRIMITIVE_TORUS:
			return primitive->torus.radii.x > 0.0f && primitive->torus.radii.y > 0.0f;
		case SE_SDF_PRIMITIVE_CAPSULE:
			return primitive->capsule.radius >= 0.0f;
		case SE_SDF_PRIMITIVE_VERTICAL_CAPSULE:
			return primitive->vertical_capsule.height > 0.0f && primitive->vertical_capsule.radius >= 0.0f;
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER:
			return primitive->capped_cylinder.radius >= 0.0f && primitive->capped_cylinder.half_height > 0.0f;
		case SE_SDF_PRIMITIVE_CONE:
			return primitive->cone.height > 0.0f;
		case SE_SDF_PRIMITIVE_CUT_SPHERE:
			return primitive->cut_sphere.radius > 0.0f;
		case SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE:
			return primitive->cut_hollow_sphere.radius > 0.0f &&
				primitive->cut_hollow_sphere.thickness >= 0.0f;
		case SE_SDF_PRIMITIVE_ROUND_CONE:
			return primitive->round_cone.height > 0.0f &&
				primitive->round_cone.radius_base >= 0.0f &&
				primitive->round_cone.radius_top >= 0.0f;
		case SE_SDF_PRIMITIVE_OCTAHEDRON:
			return primitive->octahedron.size > 0.0f;
		case SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND:
			return primitive->octahedron_bound.size > 0.0f;
		case SE_SDF_PRIMITIVE_PYRAMID:
			return primitive->pyramid.height > 0.0f;
		default:
			return 1;
	}
}

static b8 se_sdf_operation_child_count_is_legal(
	const se_sdf_operation operation,
	const sz child_count
) {
	if (operation == SE_SDF_OP_SUBTRACTION || operation == SE_SDF_OP_SMOOTH_SUBTRACTION) {
		return child_count != 1;
	}
	return 1;
}

static b8 se_sdf_operation_is_smooth(const se_sdf_operation operation) {
	switch (operation) {
		case SE_SDF_OP_SMOOTH_UNION:
		case SE_SDF_OP_SMOOTH_INTERSECTION:
		case SE_SDF_OP_SMOOTH_SUBTRACTION:
			return 1;
		default:
			return 0;
	}
}

static f32 se_sdf_sanitize_operation_amount(
	const se_sdf_operation operation,
	const f32 operation_amount
) {
	if (!se_sdf_operation_is_smooth(operation)) {
		return operation_amount;
	}
	if (!isfinite(operation_amount) || operation_amount <= 0.000001f) {
		return SE_SDF_OPERATION_AMOUNT_DEFAULT;
	}
	return operation_amount;
}

static b8 se_sdf_is_ancestor(
	se_sdf_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle ancestor,
	se_sdf_node_handle node
) {
	if (!scene_ptr || ancestor == SE_SDF_NODE_NULL || node == SE_SDF_NODE_NULL) {
		return 0;
	}
	const sz max_depth = s_array_get_size(&scene_ptr->nodes) + 1;
	sz depth = 0;
	while (node != SE_SDF_NODE_NULL && depth++ < max_depth) {
		if (node == ancestor) {
			return 1;
		}
		se_sdf_node* current = se_sdf_node_from_handle(scene_ptr, scene, node);
		if (!current) {
			return 0;
		}
		node = current->parent;
	}
	return 0;
}

static void se_sdf_detach_from_parent(
	se_sdf_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node_handle
) {
	se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, node_handle);
	if (!node || node->parent == SE_SDF_NODE_NULL) {
		return;
	}
	se_sdf_node* parent = se_sdf_node_from_handle(scene_ptr, scene, node->parent);
	if (parent) {
		se_sdf_remove_child_entry(parent, node_handle);
	}
	node->parent = SE_SDF_NODE_NULL;
}

static void se_sdf_destroy_node_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node_handle
) {
	se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, node_handle);
	if (!node) {
		return;
	}

	s_array(se_sdf_node_handle, children_copy);
	s_array_init(&children_copy);
	for (sz i = 0; i < s_array_get_size(&node->children); ++i) {
		se_sdf_node_handle* child = s_array_get(&node->children, s_array_handle(&node->children, (u32)i));
		if (child) {
			s_array_add(&children_copy, *child);
		}
	}

	for (sz i = 0; i < s_array_get_size(&children_copy); ++i) {
		se_sdf_node_handle* child = s_array_get(&children_copy, s_array_handle(&children_copy, (u32)i));
		if (child) {
			se_sdf_destroy_node_recursive(scene_ptr, scene, *child);
		}
	}
	s_array_clear(&children_copy);

	node = se_sdf_node_from_handle(scene_ptr, scene, node_handle);
	if (!node) {
		return;
	}

	se_sdf_detach_from_parent(scene_ptr, scene, node_handle);
	if (scene_ptr->root == node_handle) {
		scene_ptr->root = SE_SDF_NODE_NULL;
	}

	s_array_clear(&node->children);
	s_array_remove(&scene_ptr->nodes, node_handle);
}

se_sdf_scene_handle se_sdf_scene_create(const se_sdf_scene_desc* desc) {
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return SE_SDF_SCENE_NULL;
	}
	se_sdf_scene_desc scene_desc = SE_SDF_SCENE_DESC_DEFAULTS;
	if (desc) {
		scene_desc = *desc;
	}

	se_sdf_scene_handle scene_handle = s_array_increment(&ctx->sdf_scenes);
	se_sdf_scene* scene = s_array_get(&ctx->sdf_scenes, scene_handle);
	if (!scene) {
		return SE_SDF_SCENE_NULL;
	}
	memset(scene, 0, sizeof(*scene));
	s_array_init(&scene->nodes);
	scene->root = SE_SDF_NODE_NULL;
	scene->implicit_renderer = SE_SDF_RENDERER_NULL;
	scene->implicit_camera = S_HANDLE_NULL;

	if (scene_desc.initial_node_capacity > 0) {
		s_array_reserve(&scene->nodes, scene_desc.initial_node_capacity);
	}

	return scene_handle;
}

se_object_2d_handle se_sdf_scene_to_object_2d(se_sdf_scene_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	const b8 had_renderer = se_sdf_renderer_from_handle(scene_ptr->implicit_renderer) != NULL;
	const b8 had_camera = (scene_ptr->implicit_camera != S_HANDLE_NULL) &&
		(se_camera_get(scene_ptr->implicit_camera) != NULL);
	if (!se_sdf_scene_prepare_implicit_render(scene, scene_ptr)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		}
		return S_HANDLE_NULL;
	}

	se_object_custom custom = {0};
	custom.render = se_sdf_scene_object_2d_render;
	const se_sdf_scene_object_2d_data custom_data = {
		.scene = scene
	};
	se_object_custom_set_data(&custom, &custom_data, sizeof(custom_data));

	se_object_2d_handle object = se_object_2d_create_custom(&custom, &s_mat3_identity);
	if (object == S_HANDLE_NULL) {
		if (!had_renderer && scene_ptr->implicit_renderer != SE_SDF_RENDERER_NULL) {
			se_sdf_renderer_destroy(scene_ptr->implicit_renderer);
			scene_ptr->implicit_renderer = SE_SDF_RENDERER_NULL;
		}
		if (!had_camera && scene_ptr->implicit_camera != S_HANDLE_NULL && se_camera_get(scene_ptr->implicit_camera)) {
			se_camera_destroy(scene_ptr->implicit_camera);
			scene_ptr->implicit_camera = S_HANDLE_NULL;
		}
		return S_HANDLE_NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return object;
}

se_object_3d_handle se_sdf_scene_to_object_3d(se_sdf_scene_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	const b8 had_renderer = se_sdf_renderer_from_handle(scene_ptr->implicit_renderer) != NULL;
	const b8 had_camera = (scene_ptr->implicit_camera != S_HANDLE_NULL) &&
		(se_camera_get(scene_ptr->implicit_camera) != NULL);
	if (!se_sdf_scene_prepare_implicit_render(scene, scene_ptr)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		}
		return S_HANDLE_NULL;
	}

	se_object_custom custom = {0};
	custom.render = se_sdf_scene_object_3d_render;
	const se_sdf_scene_object_3d_data custom_data = {
		.scene = scene
	};
	se_object_custom_set_data(&custom, &custom_data, sizeof(custom_data));

	se_object_3d_handle object = se_object_3d_create_custom(&custom, &s_mat4_identity);
	if (object == S_HANDLE_NULL) {
		if (!had_renderer && scene_ptr->implicit_renderer != SE_SDF_RENDERER_NULL) {
			se_sdf_renderer_destroy(scene_ptr->implicit_renderer);
			scene_ptr->implicit_renderer = SE_SDF_RENDERER_NULL;
		}
		if (!had_camera && scene_ptr->implicit_camera != S_HANDLE_NULL && se_camera_get(scene_ptr->implicit_camera)) {
			se_camera_destroy(scene_ptr->implicit_camera);
			scene_ptr->implicit_camera = S_HANDLE_NULL;
		}
		return S_HANDLE_NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return object;
}

void se_sdf_scene_destroy(const se_sdf_scene_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return;
	}
	se_sdf_scene_release_implicit_resources(scene_ptr);
	se_sdf_scene_release_nodes(scene_ptr);
	s_array_remove(&ctx->sdf_scenes, scene);
}

void se_sdf_scene_clear(const se_sdf_scene_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	se_sdf_scene_release_nodes(scene_ptr);
}

b8 se_sdf_scene_set_root(const se_sdf_scene_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	if (node == SE_SDF_NODE_NULL) {
		scene_ptr->root = SE_SDF_NODE_NULL;
		return 1;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	scene_ptr->root = node;
	return 1;
}

se_sdf_node_handle se_sdf_scene_get_root(const se_sdf_scene_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return SE_SDF_NODE_NULL;
	}
	return scene_ptr->root;
}

static void se_sdf_write_scene_error(
	char* error_message,
	const sz error_message_size,
	const char* fmt,
	...
) {
	if (!error_message || error_message_size == 0 || !fmt) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	vsnprintf(error_message, error_message_size, fmt, args);
	va_end(args);
}

b8 se_sdf_scene_validate(
	const se_sdf_scene_handle scene,
	char* error_message,
	const sz error_message_size
) {
	if (error_message && error_message_size > 0) {
		error_message[0] = '\0';
	}

	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		se_sdf_write_scene_error(error_message, error_message_size, "invalid scene handle");
		return 0;
	}

	if (scene_ptr->root != SE_SDF_NODE_NULL &&
		!se_sdf_node_from_handle(scene_ptr, scene, scene_ptr->root)) {
		se_sdf_write_scene_error(
			error_message, error_message_size, "root handle is invalid (slot=%u)", s_handle_slot(scene_ptr->root));
		return 0;
	}

	const sz node_count = s_array_get_size(&scene_ptr->nodes);
	for (sz i = 0; i < node_count; ++i) {
		se_sdf_node_handle handle = s_array_handle(&scene_ptr->nodes, (u32)i);
		se_sdf_node* node = s_array_get(&scene_ptr->nodes, handle);
		if (!node) {
			continue;
		}

		if (node->owner_scene != scene) {
			se_sdf_write_scene_error(
				error_message, error_message_size,
				"node owner mismatch (node_slot=%u)", s_handle_slot(handle));
			return 0;
		}

		if (!node->is_group && !se_sdf_validate_primitive_desc(&node->primitive)) {
			se_sdf_write_scene_error(
				error_message, error_message_size,
				"invalid primitive parameters (node_slot=%u, type=%d)",
				s_handle_slot(handle), (int)node->primitive.type);
			return 0;
		}

		if (!se_sdf_operation_child_count_is_legal(
				node->operation, s_array_get_size(&node->children))) {
			se_sdf_write_scene_error(
				error_message, error_message_size,
				"illegal operation for child count (node_slot=%u, op=%d, child_count=%zu)",
				s_handle_slot(handle), (int)node->operation, s_array_get_size(&node->children));
			return 0;
		}
		if (se_sdf_operation_is_smooth(node->operation) &&
			(!isfinite(node->operation_amount) || node->operation_amount <= 0.000001f)) {
			se_sdf_write_scene_error(
				error_message, error_message_size,
				"invalid smooth operation amount (node_slot=%u, amount=%.6f)",
				s_handle_slot(handle), (double)node->operation_amount);
			return 0;
		}

		if (node->parent != SE_SDF_NODE_NULL) {
			se_sdf_node* parent = se_sdf_node_from_handle(scene_ptr, scene, node->parent);
			if (!parent) {
				se_sdf_write_scene_error(
					error_message, error_message_size,
					"invalid parent handle (node_slot=%u, parent_slot=%u)",
					s_handle_slot(handle), s_handle_slot(node->parent));
				return 0;
			}
			if (!se_sdf_has_child_entry(parent, handle)) {
				se_sdf_write_scene_error(
					error_message, error_message_size,
					"parent link missing child entry (node_slot=%u, parent_slot=%u)",
					s_handle_slot(handle), s_handle_slot(node->parent));
				return 0;
			}
		}

		for (sz child_i = 0; child_i < s_array_get_size(&node->children); ++child_i) {
			se_sdf_node_handle* child_handle = s_array_get(&node->children, s_array_handle(&node->children, (u32)child_i));
			if (!child_handle || *child_handle == SE_SDF_NODE_NULL || *child_handle == handle) {
				se_sdf_write_scene_error(
					error_message, error_message_size,
					"invalid child link (parent_slot=%u)", s_handle_slot(handle));
				return 0;
			}
			se_sdf_node* child = se_sdf_node_from_handle(scene_ptr, scene, *child_handle);
			if (!child) {
				se_sdf_write_scene_error(
					error_message, error_message_size,
					"dangling child handle (parent_slot=%u, child_slot=%u)",
					s_handle_slot(handle), s_handle_slot(*child_handle));
				return 0;
			}
			if (child->parent != handle) {
				se_sdf_write_scene_error(
					error_message, error_message_size,
					"child parent mismatch (parent_slot=%u, child_slot=%u, child_parent_slot=%u)",
					s_handle_slot(handle),
					s_handle_slot(*child_handle),
					s_handle_slot(child->parent));
				return 0;
			}
		}

		se_sdf_node_handle walker = handle;
		sz depth = 0;
		const sz max_depth = node_count + 1;
		while (walker != SE_SDF_NODE_NULL && depth++ < max_depth) {
			se_sdf_node* current = se_sdf_node_from_handle(scene_ptr, scene, walker);
			if (!current) {
				se_sdf_write_scene_error(
					error_message, error_message_size,
					"invalid ancestor chain (node_slot=%u)", s_handle_slot(handle));
				return 0;
			}
			walker = current->parent;
		}
		if (walker != SE_SDF_NODE_NULL) {
			se_sdf_write_scene_error(
				error_message, error_message_size,
				"cycle detected in parent chain (node_slot=%u)", s_handle_slot(handle));
			return 0;
		}
	}

	return 1;
}

s_json* se_sdf_to_json(const se_sdf_scene_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	s_json* root = s_json_object_empty(NULL);
	if (!root) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	if (!se_sdf_json_add_string(root, "format", SE_SDF_JSON_FORMAT) ||
		!se_sdf_json_add_u32(root, "version", SE_SDF_JSON_VERSION)) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	se_sdf_node_handles node_handles = {0};
	s_array_init(&node_handles);
	s_array_reserve(&node_handles, s_array_get_size(&scene_ptr->nodes));
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		const se_sdf_node_handle handle = s_array_handle(&scene_ptr->nodes, (u32)i);
		se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, handle);
		if (!node) {
			continue;
		}
		s_array_add(&node_handles, handle);
	}

	i32 root_index = -1;
	if (scene_ptr->root != SE_SDF_NODE_NULL) {
		u32 root_u32 = 0u;
		if (!se_sdf_json_node_index_of(&node_handles, scene_ptr->root, &root_u32)) {
			s_array_clear(&node_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return NULL;
		}
		root_index = (i32)root_u32;
	}
	if (!se_sdf_json_add_i32(root, "root_index", root_index)) {
		s_array_clear(&node_handles);
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	s_json* nodes_json = se_sdf_json_add_array(root, "nodes");
	if (!nodes_json) {
		s_array_clear(&node_handles);
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	for (sz i = 0; i < s_array_get_size(&node_handles); ++i) {
		se_sdf_node_handle* handle = s_array_get(&node_handles, s_array_handle(&node_handles, (u32)i));
		if (!handle) {
			continue;
		}
		se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, *handle);
		if (!node) {
			continue;
		}

		s_json* node_json = se_sdf_json_add_object(nodes_json, NULL);
		s_json* children_json = se_sdf_json_add_array(node_json, "children");
		if (!node_json ||
			!children_json ||
			!se_sdf_json_add_bool(node_json, "is_group", node->is_group) ||
			!se_sdf_json_add_mat4(node_json, "transform", &node->transform) ||
			!se_sdf_json_add_i32(node_json, "operation", (i32)node->operation) ||
			!se_sdf_json_add_f32(node_json, "operation_amount", node->operation_amount) ||
			!se_sdf_json_add_vec3(node_json, "control_translation", &node->control_translation) ||
			!se_sdf_json_add_vec3(node_json, "control_rotation", &node->control_rotation) ||
			!se_sdf_json_add_vec3(node_json, "control_scale", &node->control_scale) ||
			!se_sdf_json_add_bool(node_json, "control_trs_initialized", node->control_trs_initialized)) {
			s_array_clear(&node_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}

		if (!node->is_group && !se_sdf_json_add_primitive_desc(node_json, "primitive", &node->primitive)) {
			s_array_clear(&node_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}

		for (sz child_i = 0; child_i < s_array_get_size(&node->children); ++child_i) {
			se_sdf_node_handle* child_handle = s_array_get(&node->children, s_array_handle(&node->children, (u32)child_i));
			if (!child_handle || *child_handle == SE_SDF_NODE_NULL) {
				s_array_clear(&node_handles);
				s_json_free(root);
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				return NULL;
			}
			u32 child_index = 0u;
			if (!se_sdf_json_node_index_of(&node_handles, *child_handle, &child_index) ||
				!se_sdf_json_add_u32(children_json, NULL, child_index)) {
				s_array_clear(&node_handles);
				s_json_free(root);
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				return NULL;
			}
		}
	}

	s_array_clear(&node_handles);
	se_set_last_error(SE_RESULT_OK);
	return root;
}

b8 se_sdf_from_json(const se_sdf_scene_handle scene, const s_json* root) {
	if (!root || root->type != S_JSON_OBJECT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (!se_sdf_scene_from_handle(scene)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	const c8* format = NULL;
	u32 version = 0u;
	i32 root_index = -1;
	const s_json* nodes_json = se_sdf_json_get_required(root, "nodes", S_JSON_ARRAY);
	if (!se_sdf_json_get_string(root, "format", &format) ||
		!se_sdf_json_get_u32(root, "version", &version) ||
		!se_sdf_json_get_i32(root, "root_index", &root_index) ||
		!nodes_json) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (strcmp(format, SE_SDF_JSON_FORMAT) != 0 || version != SE_SDF_JSON_VERSION) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return 0;
	}
	if (root_index < -1 || (root_index >= 0 && (sz)root_index >= nodes_json->as.children.count)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	se_sdf_scene_desc temp_desc = SE_SDF_SCENE_DESC_DEFAULTS;
	temp_desc.initial_node_capacity = nodes_json->as.children.count;
	const se_sdf_scene_handle temp_scene = se_sdf_scene_create(&temp_desc);
	if (temp_scene == SE_SDF_SCENE_NULL) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return 0;
	}
	se_sdf_scene* temp_scene_ptr = se_sdf_scene_from_handle(temp_scene);
	if (!temp_scene_ptr) {
		se_sdf_scene_destroy(temp_scene);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}

	se_sdf_node_handles created_handles = {0};
	s_array_init(&created_handles);
	s_array_reserve(&created_handles, nodes_json->as.children.count);

	for (sz i = 0; i < nodes_json->as.children.count; ++i) {
		const s_json* node_json = s_json_at(nodes_json, i);
		if (!node_json || node_json->type != S_JSON_OBJECT) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		b8 is_group = 0;
		i32 operation_i32 = 0;
		f32 operation_amount = SE_SDF_OPERATION_AMOUNT_DEFAULT;
		s_mat4 transform = s_mat4_identity;
		s_vec3 control_translation = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 control_rotation = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 control_scale = s_vec3(1.0f, 1.0f, 1.0f);
		b8 control_trs_initialized = 0;
		if (!se_sdf_json_get_bool(node_json, "is_group", &is_group) ||
			!se_sdf_json_get_i32(node_json, "operation", &operation_i32) ||
			!se_sdf_json_get_f32(node_json, "operation_amount", &operation_amount) ||
			!se_sdf_json_read_mat4_field(node_json, "transform", &transform)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (operation_i32 < (i32)SE_SDF_OP_NONE || operation_i32 > (i32)SE_SDF_OP_SMOOTH_SUBTRACTION) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		const s_json* control_translation_json = s_json_get(node_json, "control_translation");
		const s_json* control_rotation_json = s_json_get(node_json, "control_rotation");
		const s_json* control_scale_json = s_json_get(node_json, "control_scale");
		const s_json* control_initialized_json = s_json_get(node_json, "control_trs_initialized");
		if (control_translation_json && !se_sdf_json_read_vec3_node(control_translation_json, &control_translation)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (control_rotation_json && !se_sdf_json_read_vec3_node(control_rotation_json, &control_rotation)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (control_scale_json && !se_sdf_json_read_vec3_node(control_scale_json, &control_scale)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (control_initialized_json) {
			if (control_initialized_json->type != S_JSON_BOOL) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
			control_trs_initialized = control_initialized_json->as.boolean;
		}

		se_sdf_node_handle node_handle = SE_SDF_NODE_NULL;
		if (is_group) {
			se_sdf_node_group_desc group_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
			group_desc.transform = transform;
			group_desc.operation = (se_sdf_operation)operation_i32;
			group_desc.operation_amount = operation_amount;
			node_handle = se_sdf_node_create_group(temp_scene, &group_desc);
		} else {
			const s_json* primitive_json = se_sdf_json_get_required(node_json, "primitive", S_JSON_OBJECT);
			se_sdf_primitive_desc primitive = {0};
			if (!primitive_json || !se_sdf_json_read_primitive_desc(primitive_json, &primitive)) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}

			se_sdf_node_primitive_desc primitive_desc = {0};
			primitive_desc.transform = transform;
			primitive_desc.operation = (se_sdf_operation)operation_i32;
			primitive_desc.operation_amount = operation_amount;
			primitive_desc.primitive = primitive;
			node_handle = se_sdf_node_create_primitive(temp_scene, &primitive_desc);
		}

		if (node_handle == SE_SDF_NODE_NULL) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		s_array_add(&created_handles, node_handle);

		se_sdf_node* node_ptr = se_sdf_node_from_handle(temp_scene_ptr, temp_scene, node_handle);
		if (!node_ptr) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			goto fail;
		}
		node_ptr->control_translation = control_translation;
		node_ptr->control_rotation = control_rotation;
		node_ptr->control_scale = control_scale;
		node_ptr->control_trs_initialized = control_trs_initialized;
	}

	for (sz i = 0; i < nodes_json->as.children.count; ++i) {
		const s_json* node_json = s_json_at(nodes_json, i);
		const s_json* children_json = se_sdf_json_get_required(node_json, "children", S_JSON_ARRAY);
		if (!children_json) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		se_sdf_node_handle* parent_handle = s_array_get(&created_handles, s_array_handle(&created_handles, (u32)i));
		if (!parent_handle || *parent_handle == SE_SDF_NODE_NULL) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			goto fail;
		}

		for (sz child_i = 0; child_i < children_json->as.children.count; ++child_i) {
			const s_json* child_index_json = s_json_at(children_json, child_i);
			u32 child_index = 0u;
			if (!se_sdf_json_number_to_u32(child_index_json, &child_index) ||
				(sz)child_index >= s_array_get_size(&created_handles)) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
			se_sdf_node_handle* child_handle = s_array_get(&created_handles, s_array_handle(&created_handles, child_index));
			if (!child_handle || *child_handle == SE_SDF_NODE_NULL ||
				!se_sdf_node_add_child(temp_scene, *parent_handle, *child_handle)) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
		}
	}

	se_sdf_node_handle resolved_root = SE_SDF_NODE_NULL;
	if (root_index >= 0) {
		se_sdf_node_handle* root_handle = s_array_get(&created_handles, s_array_handle(&created_handles, (u32)root_index));
		if (!root_handle) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		resolved_root = *root_handle;
	}
	if (!se_sdf_scene_set_root(temp_scene, resolved_root)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		goto fail;
	}

	char validation_error[256] = {0};
	if (!se_sdf_scene_validate(temp_scene, validation_error, sizeof(validation_error))) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		goto fail;
	}

	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	temp_scene_ptr = se_sdf_scene_from_handle(temp_scene);
	if (!scene_ptr || !temp_scene_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		goto fail;
	}

	se_sdf_scene_release_nodes(scene_ptr);
	scene_ptr->nodes = temp_scene_ptr->nodes;
	scene_ptr->root = temp_scene_ptr->root;
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		se_sdf_node* node = s_array_get(&scene_ptr->nodes, s_array_handle(&scene_ptr->nodes, (u32)i));
		if (!node) {
			continue;
		}
		node->owner_scene = scene;
	}

	s_array_init(&temp_scene_ptr->nodes);
	temp_scene_ptr->root = SE_SDF_NODE_NULL;
	se_sdf_scene_destroy(temp_scene);
	s_array_clear(&created_handles);
	se_set_last_error(SE_RESULT_OK);
	return 1;

fail:
	s_array_clear(&created_handles);
	se_sdf_scene_destroy(temp_scene);
	return 0;
}

b8 se_sdf_from_json_file(const se_sdf_scene_handle scene, const c8* path) {
	if (!path || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	c8* text = NULL;
	if (!s_file_read(path, &text, NULL)) {
		se_set_last_error(SE_RESULT_IO);
		return 0;
	}

	s_json* root = s_json_parse(text);
	free(text);
	if (!root) {
		se_set_last_error(SE_RESULT_IO);
		return 0;
	}

	const b8 ok = se_sdf_from_json(scene, root);
	s_json_free(root);
	return ok;
}

se_sdf_node_handle se_sdf_node_create_primitive(
	const se_sdf_scene_handle scene,
	const se_sdf_node_primitive_desc* desc
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr || !desc || !se_sdf_validate_primitive_desc(&desc->primitive)) {
		return SE_SDF_NODE_NULL;
	}

	se_sdf_node_handle node_handle = s_array_increment(&scene_ptr->nodes);
	se_sdf_node* node = s_array_get(&scene_ptr->nodes, node_handle);
	if (!node) {
		return SE_SDF_NODE_NULL;
	}
	memset(node, 0, sizeof(*node));
	node->owner_scene = scene;
	node->transform = desc->transform;
	node->operation = desc->operation;
	node->operation_amount = se_sdf_sanitize_operation_amount(desc->operation, desc->operation_amount);
	node->primitive = desc->primitive;
	node->parent = SE_SDF_NODE_NULL;
	node->is_group = 0;
	node->control_translation = s_vec3(0.0f, 0.0f, 0.0f);
	node->control_rotation = s_vec3(0.0f, 0.0f, 0.0f);
	node->control_scale = s_vec3(1.0f, 1.0f, 1.0f);
	node->control_trs_initialized = 0;
	s_array_init(&node->children);
	return node_handle;
}

se_sdf_node_handle se_sdf_node_create_group(
	const se_sdf_scene_handle scene,
	const se_sdf_node_group_desc* desc
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return SE_SDF_NODE_NULL;
	}

	se_sdf_node_group_desc group_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	if (desc) {
		group_desc = *desc;
	}

	se_sdf_node_handle node_handle = s_array_increment(&scene_ptr->nodes);
	se_sdf_node* node = s_array_get(&scene_ptr->nodes, node_handle);
	if (!node) {
		return SE_SDF_NODE_NULL;
	}
	memset(node, 0, sizeof(*node));
	node->owner_scene = scene;
	node->transform = group_desc.transform;
	node->operation = group_desc.operation;
	node->operation_amount = se_sdf_sanitize_operation_amount(group_desc.operation, group_desc.operation_amount);
	node->parent = SE_SDF_NODE_NULL;
	node->is_group = 1;
	node->control_translation = s_vec3(0.0f, 0.0f, 0.0f);
	node->control_rotation = s_vec3(0.0f, 0.0f, 0.0f);
	node->control_scale = s_vec3(1.0f, 1.0f, 1.0f);
	node->control_trs_initialized = 0;
	s_array_init(&node->children);
	return node_handle;
}

void se_sdf_node_destroy(const se_sdf_scene_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	if (!se_sdf_node_from_handle(scene_ptr, scene, node)) {
		return;
	}
	se_sdf_destroy_node_recursive(scene_ptr, scene, node);
}

b8 se_sdf_node_add_child(
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle parent,
	const se_sdf_node_handle child
) {
	if (parent == child || parent == SE_SDF_NODE_NULL || child == SE_SDF_NODE_NULL) {
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}

	se_sdf_node* parent_node = se_sdf_node_from_handle(scene_ptr, scene, parent);
	se_sdf_node* child_node = se_sdf_node_from_handle(scene_ptr, scene, child);
	if (!parent_node || !child_node) {
		return 0;
	}

	if (se_sdf_is_ancestor(scene_ptr, scene, child, parent)) {
		return 0;
	}

	if (child_node->parent != SE_SDF_NODE_NULL && child_node->parent != parent) {
		se_sdf_node* old_parent = se_sdf_node_from_handle(scene_ptr, scene, child_node->parent);
		if (old_parent) {
			se_sdf_remove_child_entry(old_parent, child);
		}
	}

	if (!se_sdf_has_child_entry(parent_node, child)) {
		s_array_add(&parent_node->children, child);
	}
	child_node->parent = parent;
	return 1;
}

b8 se_sdf_node_remove_child(
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle parent,
	const se_sdf_node_handle child
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_node* parent_node = se_sdf_node_from_handle(scene_ptr, scene, parent);
	if (!parent_node) {
		return 0;
	}

	b8 removed = se_sdf_remove_child_entry(parent_node, child);
	if (!removed) {
		return 0;
	}

	se_sdf_node* child_node = se_sdf_node_from_handle(scene_ptr, scene, child);
	if (child_node && child_node->parent == parent) {
		child_node->parent = SE_SDF_NODE_NULL;
	}
	return 1;
}

b8 se_sdf_node_set_operation(
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_operation operation
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	if (!se_sdf_operation_child_count_is_legal(operation, s_array_get_size(&node_ptr->children))) {
		return 0;
	}
	node_ptr->operation = operation;
	node_ptr->operation_amount = se_sdf_sanitize_operation_amount(operation, node_ptr->operation_amount);
	return 1;
}

b8 se_sdf_node_set_transform(
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const s_mat4* transform
) {
	if (!transform) {
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	node_ptr->transform = *transform;
	node_ptr->control_trs_initialized = 0;
	return 1;
}

s_mat4 se_sdf_node_get_transform(const se_sdf_scene_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return s_mat4_identity;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return s_mat4_identity;
	}
	return node_ptr->transform;
}

static s_mat4 se_sdf_transform_trs(
	const f32 tx,
	const f32 ty,
	const f32 tz,
	const f32 rx,
	const f32 ry,
	const f32 rz,
	const f32 sx,
	const f32 sy,
	const f32 sz
) {
	s_mat4 transform = s_mat4_identity;
	const s_vec3 translation = s_vec3(tx, ty, tz);
	const s_vec3 scale = s_vec3(sx, sy, sz);

	transform = s_mat4_translate(&transform, &translation);
	if (rx != 0.0f) transform = s_mat4_rotate_x(&transform, rx);
	if (ry != 0.0f) transform = s_mat4_rotate_y(&transform, ry);
	if (rz != 0.0f) transform = s_mat4_rotate_z(&transform, rz);
	transform = s_mat4_scale(&transform, &scale);
	return transform;
}

static s_mat4 se_sdf_transform_grid_cell(
	const i32 index,
	const i32 columns,
	const i32 rows,
	const f32 spacing,
	const f32 y,
	const f32 yaw,
	const f32 sx,
	const f32 sy,
	const f32 sz
) {
	i32 safe_columns = columns > 0 ? columns : 1;
	i32 safe_rows = rows > 0 ? rows : 1;
	const f32 half_width = ((f32)safe_columns - 1.0f) * 0.5f;
	const f32 half_depth = ((f32)safe_rows - 1.0f) * 0.5f;

	const i32 row = index / safe_columns;
	const i32 col = index % safe_columns;
	const f32 x = ((f32)col - half_width) * spacing;
	const f32 z = ((f32)row - half_depth) * spacing;
	return se_sdf_transform_trs(x, y, z, 0.0f, yaw, 0.0f, sx, sy, sz);
}

static se_sdf_primitive_desc se_sdf_default_sphere_primitive(void) {
	se_sdf_primitive_desc primitive = {0};
	primitive.type = SE_SDF_PRIMITIVE_SPHERE;
	primitive.sphere.radius = 0.82f;
	return primitive;
}

static se_sdf_primitive_desc se_sdf_default_box_primitive(void) {
	se_sdf_primitive_desc primitive = {0};
	primitive.type = SE_SDF_PRIMITIVE_BOX;
	primitive.box.size = s_vec3(0.60f, 0.60f, 0.60f);
	return primitive;
}

static se_sdf_primitive_desc se_sdf_gallery_primitive(const i32 index) {
	se_sdf_primitive_desc primitive = {0};
	switch (index % 12) {
		case 0:
			primitive.type = SE_SDF_PRIMITIVE_SPHERE;
			primitive.sphere.radius = 0.82f;
			break;
		case 1:
			primitive.type = SE_SDF_PRIMITIVE_BOX;
			primitive.box.size = s_vec3(0.62f, 0.62f, 0.62f);
			break;
		case 2:
			primitive.type = SE_SDF_PRIMITIVE_ROUND_BOX;
			primitive.round_box.size = s_vec3(0.60f, 0.58f, 0.58f);
			primitive.round_box.roundness = 0.16f;
			break;
		case 3:
			primitive.type = SE_SDF_PRIMITIVE_BOX_FRAME;
			primitive.box_frame.size = s_vec3(0.72f, 0.72f, 0.72f);
			primitive.box_frame.thickness = 0.13f;
			break;
		case 4:
			primitive.type = SE_SDF_PRIMITIVE_TORUS;
			primitive.torus.radii = s_vec2(0.67f, 0.20f);
			break;
		case 5:
			primitive.type = SE_SDF_PRIMITIVE_CONE;
			primitive.cone.angle_sin_cos = s_vec2(0.50f, 0.86f);
			primitive.cone.height = 0.96f;
			break;
		case 6:
			primitive.type = SE_SDF_PRIMITIVE_CAPPED_CYLINDER;
			primitive.capped_cylinder.radius = 0.43f;
			primitive.capped_cylinder.half_height = 0.58f;
			break;
		case 7:
			primitive.type = SE_SDF_PRIMITIVE_CAPSULE;
			primitive.capsule.a = s_vec3(0.0f, -0.62f, 0.0f);
			primitive.capsule.b = s_vec3(0.0f, 0.62f, 0.0f);
			primitive.capsule.radius = 0.28f;
			break;
		case 8:
			primitive.type = SE_SDF_PRIMITIVE_OCTAHEDRON;
			primitive.octahedron.size = 0.96f;
			break;
		case 9:
			primitive.type = SE_SDF_PRIMITIVE_PYRAMID;
			primitive.pyramid.height = 0.95f;
			break;
		case 10:
			primitive.type = SE_SDF_PRIMITIVE_CUT_SPHERE;
			primitive.cut_sphere.radius = 0.78f;
			primitive.cut_sphere.cut_height = 0.26f;
			break;
		default:
			primitive.type = SE_SDF_PRIMITIVE_ROUND_CONE;
			primitive.round_cone.radius_base = 0.56f;
			primitive.round_cone.radius_top = 0.19f;
			primitive.round_cone.height = 1.12f;
			break;
	}
	return primitive;
}

se_sdf_node_handle se_sdf_node_spawn_primitive(
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle parent,
	const se_sdf_primitive_desc* primitive,
	const s_mat4* transform,
	const se_sdf_operation operation
) {
	if (!primitive) {
		return SE_SDF_NODE_NULL;
	}
	se_sdf_node_primitive_desc node_desc = {0};
	node_desc.transform = transform ? *transform : s_mat4_identity;
	node_desc.operation = operation;
	node_desc.primitive = *primitive;

	se_sdf_node_handle node = se_sdf_node_create_primitive(scene, &node_desc);
	if (node == SE_SDF_NODE_NULL) {
		return SE_SDF_NODE_NULL;
	}

	if (parent != SE_SDF_NODE_NULL) {
		if (!se_sdf_node_add_child(scene, parent, node)) {
			se_sdf_node_destroy(scene, node);
			return SE_SDF_NODE_NULL;
		}
		return node;
	}

	if (se_sdf_scene_get_root(scene) == SE_SDF_NODE_NULL) {
		(void)se_sdf_scene_set_root(scene, node);
	}
	return node;
}

b8 se_sdf_scene_build_single_object_preset(
	const se_sdf_scene_handle scene,
	const se_sdf_primitive_desc* primitive,
	const s_mat4* transform,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_object
) {
	if (scene == SE_SDF_SCENE_NULL) {
		return 0;
	}
	if (out_root) *out_root = SE_SDF_NODE_NULL;
	if (out_object) *out_object = SE_SDF_NODE_NULL;

	se_sdf_scene_clear(scene);

	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.transform = s_mat4_identity;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_scene_set_root(scene, root)) {
		return 0;
	}

	se_sdf_primitive_desc fallback = se_sdf_default_sphere_primitive();
	const se_sdf_primitive_desc* selected = primitive ? primitive : &fallback;
	const s_mat4 object_transform = transform ? *transform : s_mat4_identity;
	se_sdf_node_handle object = se_sdf_node_spawn_primitive(scene, root, selected, &object_transform, SE_SDF_OP_UNION);
	if (object == SE_SDF_NODE_NULL) {
		return 0;
	}

	if (out_root) *out_root = root;
	if (out_object) *out_object = object;
	return 1;
}

b8 se_sdf_scene_build_grid_preset(
	const se_sdf_scene_handle scene,
	const se_sdf_primitive_desc* primitive,
	const i32 columns,
	const i32 rows,
	const f32 spacing,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_first
) {
	if (scene == SE_SDF_SCENE_NULL || columns <= 0 || rows <= 0) {
		return 0;
	}
	if (out_root) *out_root = SE_SDF_NODE_NULL;
	if (out_first) *out_first = SE_SDF_NODE_NULL;

	se_sdf_scene_clear(scene);

	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.transform = s_mat4_identity;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_scene_set_root(scene, root)) {
		return 0;
	}

	se_sdf_primitive_desc fallback = se_sdf_default_sphere_primitive();
	const se_sdf_primitive_desc* selected = primitive ? primitive : &fallback;
	const i32 total_cells = columns * rows;
	for (i32 i = 0; i < total_cells; ++i) {
		const s_mat4 node_transform = se_sdf_transform_grid_cell(
			i, columns, rows, spacing, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
		se_sdf_node_handle node = se_sdf_node_spawn_primitive(scene, root, selected, &node_transform, SE_SDF_OP_UNION);
		if (node == SE_SDF_NODE_NULL) {
			return 0;
		}
		if (i == 0 && out_first) {
			*out_first = node;
		}
	}

	if (out_root) *out_root = root;
	return 1;
}

b8 se_sdf_scene_build_primitive_gallery_preset(
	const se_sdf_scene_handle scene,
	const i32 columns,
	const i32 rows,
	const f32 spacing,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_focus
) {
	if (scene == SE_SDF_SCENE_NULL || columns <= 0 || rows <= 0) {
		return 0;
	}
	if (out_root) *out_root = SE_SDF_NODE_NULL;
	if (out_focus) *out_focus = SE_SDF_NODE_NULL;

	se_sdf_scene_clear(scene);

	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.transform = s_mat4_identity;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_scene_set_root(scene, root)) {
		return 0;
	}

	const i32 total_cells = columns * rows;
	for (i32 i = 0; i < total_cells; ++i) {
		const s_mat4 node_transform = se_sdf_transform_grid_cell(
			i, columns, rows, spacing, 0.08f, 0.17f * (f32)i, 1.0f, 1.0f, 1.0f);
		se_sdf_primitive_desc primitive = se_sdf_gallery_primitive(i);
		se_sdf_node_handle node = se_sdf_node_spawn_primitive(scene, root, &primitive, &node_transform, SE_SDF_OP_UNION);
		if (node == SE_SDF_NODE_NULL) {
			return 0;
		}
		if (i == 0 && out_focus) {
			*out_focus = node;
		}
	}

	if (out_root) *out_root = root;
	return 1;
}

b8 se_sdf_scene_build_orbit_showcase_preset(
	const se_sdf_scene_handle scene,
	const se_sdf_primitive_desc* center_primitive,
	const se_sdf_primitive_desc* orbit_primitive,
	const i32 orbit_count,
	const f32 orbit_radius,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_center
) {
	if (scene == SE_SDF_SCENE_NULL || orbit_count <= 0 || orbit_radius <= 0.0f) {
		return 0;
	}
	if (out_root) *out_root = SE_SDF_NODE_NULL;
	if (out_center) *out_center = SE_SDF_NODE_NULL;

	se_sdf_scene_clear(scene);

	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.transform = s_mat4_identity;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_scene_set_root(scene, root)) {
		return 0;
	}

	se_sdf_primitive_desc center_fallback = se_sdf_default_sphere_primitive();
	se_sdf_primitive_desc orbit_fallback = se_sdf_default_box_primitive();
	const se_sdf_primitive_desc* center = center_primitive ? center_primitive : &center_fallback;
	const se_sdf_primitive_desc* orbit = orbit_primitive ? orbit_primitive : &orbit_fallback;

	const s_mat4 center_transform = s_mat4_identity;
	se_sdf_node_handle center_node = se_sdf_node_spawn_primitive(scene, root, center, &center_transform, SE_SDF_OP_UNION);
	if (center_node == SE_SDF_NODE_NULL) {
		return 0;
	}

	for (i32 i = 0; i < orbit_count; ++i) {
		const f32 t = ((f32)i / (f32)orbit_count) * 6.28318530718f;
		const f32 x = cosf(t) * orbit_radius;
		const f32 z = sinf(t) * orbit_radius;
		const f32 yaw = -t;
		const s_mat4 orbit_transform = se_sdf_transform_trs(
			x, 0.35f, z, 0.0f, yaw, 0.0f, 0.72f, 0.72f, 0.72f
		);
		se_sdf_node_handle orbit_node = se_sdf_node_spawn_primitive(scene, root, orbit, &orbit_transform, SE_SDF_OP_UNION);
		if (orbit_node == SE_SDF_NODE_NULL) {
			return 0;
		}
	}

	if (out_root) *out_root = root;
	if (out_center) *out_center = center_node;
	return 1;
}

static s_vec3 se_sdf_mul_mat4_point(const s_mat4* mat, const s_vec3* point) {
	if (!mat || !point) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	const f32 x = mat->m[0][0] * point->x + mat->m[1][0] * point->y + mat->m[2][0] * point->z + mat->m[3][0];
	const f32 y = mat->m[0][1] * point->x + mat->m[1][1] * point->y + mat->m[2][1] * point->z + mat->m[3][1];
	const f32 z = mat->m[0][2] * point->x + mat->m[1][2] * point->y + mat->m[2][2] * point->z + mat->m[3][2];
	const f32 w = mat->m[0][3] * point->x + mat->m[1][3] * point->y + mat->m[2][3] * point->z + mat->m[3][3];
	if (fabsf(w) > 0.00001f) {
		const f32 inv_w = 1.0f / w;
		return s_vec3(x * inv_w, y * inv_w, z * inv_w);
	}
	return s_vec3(x, y, z);
}

static void se_sdf_scene_bounds_expand_point(se_sdf_scene_bounds* bounds, const s_vec3* point) {
	if (!bounds || !point) {
		return;
	}
	if (!bounds->valid) {
		bounds->min = *point;
		bounds->max = *point;
		bounds->valid = 1;
		return;
	}
	if (point->x < bounds->min.x) bounds->min.x = point->x;
	if (point->y < bounds->min.y) bounds->min.y = point->y;
	if (point->z < bounds->min.z) bounds->min.z = point->z;
	if (point->x > bounds->max.x) bounds->max.x = point->x;
	if (point->y > bounds->max.y) bounds->max.y = point->y;
	if (point->z > bounds->max.z) bounds->max.z = point->z;
}

static void se_sdf_scene_bounds_expand_transformed_aabb(
	se_sdf_scene_bounds* bounds,
	const s_mat4* transform,
	const s_vec3* local_min,
	const s_vec3* local_max
) {
	if (!bounds || !transform || !local_min || !local_max) {
		return;
	}
	for (u8 ix = 0; ix < 2; ++ix) {
		for (u8 iy = 0; iy < 2; ++iy) {
			for (u8 iz = 0; iz < 2; ++iz) {
				const s_vec3 local_point = s_vec3(
					ix ? local_max->x : local_min->x,
					iy ? local_max->y : local_min->y,
					iz ? local_max->z : local_min->z
				);
				const s_vec3 world_point = se_sdf_mul_mat4_point(transform, &local_point);
				se_sdf_scene_bounds_expand_point(bounds, &world_point);
			}
		}
	}
}

static void se_sdf_bounds_from_points(
	const s_vec3* points,
	const sz count,
	const f32 inflate,
	s_vec3* out_min,
	s_vec3* out_max
) {
	if (!points || count == 0 || !out_min || !out_max) {
		return;
	}
	s_vec3 min_v = points[0];
	s_vec3 max_v = points[0];
	for (sz i = 1; i < count; ++i) {
		const s_vec3* point = &points[i];
		if (point->x < min_v.x) min_v.x = point->x;
		if (point->y < min_v.y) min_v.y = point->y;
		if (point->z < min_v.z) min_v.z = point->z;
		if (point->x > max_v.x) max_v.x = point->x;
		if (point->y > max_v.y) max_v.y = point->y;
		if (point->z > max_v.z) max_v.z = point->z;
	}
	if (inflate > 0.0f) {
		min_v.x -= inflate;
		min_v.y -= inflate;
		min_v.z -= inflate;
		max_v.x += inflate;
		max_v.y += inflate;
		max_v.z += inflate;
	}
	*out_min = min_v;
	*out_max = max_v;
}

static b8 se_sdf_get_primitive_local_bounds(
	const se_sdf_primitive_desc* primitive,
	s_vec3* out_min,
	s_vec3* out_max,
	b8* out_unbounded
) {
	if (!primitive || !out_min || !out_max || !out_unbounded) {
		return 0;
	}

	*out_unbounded = 0;
	switch (primitive->type) {
		case SE_SDF_PRIMITIVE_PLANE:
		case SE_SDF_PRIMITIVE_CONE_INFINITE:
		case SE_SDF_PRIMITIVE_CYLINDER:
			*out_unbounded = 1;
			return 0;

		case SE_SDF_PRIMITIVE_SPHERE: {
			const f32 r = fabsf(primitive->sphere.radius);
			*out_min = s_vec3(-r, -r, -r);
			*out_max = s_vec3(r, r, r);
			return 1;
		}
		case SE_SDF_PRIMITIVE_BOX: {
			const s_vec3 e = s_vec3(fabsf(primitive->box.size.x), fabsf(primitive->box.size.y), fabsf(primitive->box.size.z));
			*out_min = s_vec3(-e.x, -e.y, -e.z);
			*out_max = e;
			return 1;
		}
		case SE_SDF_PRIMITIVE_ROUND_BOX: {
			const f32 r = fabsf(primitive->round_box.roundness);
			const s_vec3 e = s_vec3(
				fabsf(primitive->round_box.size.x) + r,
				fabsf(primitive->round_box.size.y) + r,
				fabsf(primitive->round_box.size.z) + r
			);
			*out_min = s_vec3(-e.x, -e.y, -e.z);
			*out_max = e;
			return 1;
		}
		case SE_SDF_PRIMITIVE_BOX_FRAME: {
			const f32 t = fabsf(primitive->box_frame.thickness);
			const s_vec3 e = s_vec3(
				fabsf(primitive->box_frame.size.x) + t,
				fabsf(primitive->box_frame.size.y) + t,
				fabsf(primitive->box_frame.size.z) + t
			);
			*out_min = s_vec3(-e.x, -e.y, -e.z);
			*out_max = e;
			return 1;
		}
		case SE_SDF_PRIMITIVE_TORUS: {
			const f32 major = fabsf(primitive->torus.radii.x);
			const f32 minor = fabsf(primitive->torus.radii.y);
			*out_min = s_vec3(-(major + minor), -minor, -(major + minor));
			*out_max = s_vec3((major + minor), minor, (major + minor));
			return 1;
		}
		case SE_SDF_PRIMITIVE_CAPPED_TORUS: {
			const f32 major = fabsf(primitive->capped_torus.major_radius);
			const f32 minor = fabsf(primitive->capped_torus.minor_radius);
			const f32 e = major + minor;
			*out_min = s_vec3(-e, -e, -e);
			*out_max = s_vec3(e, e, e);
			return 1;
		}
		case SE_SDF_PRIMITIVE_LINK: {
			const f32 half_length = fabsf(primitive->link.half_length);
			const f32 radial = fabsf(primitive->link.outer_radius) + fabsf(primitive->link.inner_radius);
			*out_min = s_vec3(-radial, -(half_length + radial), -radial);
			*out_max = s_vec3(radial, (half_length + radial), radial);
			return 1;
		}
		case SE_SDF_PRIMITIVE_CONE: {
			const f32 h = fabsf(primitive->cone.height);
			const f32 s = fabsf(primitive->cone.angle_sin_cos.x);
			const f32 c = fabsf(primitive->cone.angle_sin_cos.y);
			const f32 radial = c > 0.0001f ? (h * s / c) : h;
			*out_min = s_vec3(-radial, -h, -radial);
			*out_max = s_vec3(radial, h, radial);
			return 1;
		}
		case SE_SDF_PRIMITIVE_HEX_PRISM: {
			const f32 ex = fabsf(primitive->hex_prism.half_size.x);
			const f32 ez = fabsf(primitive->hex_prism.half_size.y);
			*out_min = s_vec3(-ex, -ex, -ez);
			*out_max = s_vec3(ex, ex, ez);
			return 1;
		}
		case SE_SDF_PRIMITIVE_CAPSULE: {
			const s_vec3 points[2] = { primitive->capsule.a, primitive->capsule.b };
			se_sdf_bounds_from_points(points, 2, fabsf(primitive->capsule.radius), out_min, out_max);
			return 1;
		}
		case SE_SDF_PRIMITIVE_VERTICAL_CAPSULE: {
			const f32 h = fabsf(primitive->vertical_capsule.height);
			const f32 r = fabsf(primitive->vertical_capsule.radius);
			*out_min = s_vec3(-r, -r, -r);
			*out_max = s_vec3(r, h + r, r);
			return 1;
		}
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER: {
			const f32 r = fabsf(primitive->capped_cylinder.radius);
			const f32 h = fabsf(primitive->capped_cylinder.half_height);
			*out_min = s_vec3(-r, -h, -r);
			*out_max = s_vec3(r, h, r);
			return 1;
		}
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY: {
			const s_vec3 points[2] = { primitive->capped_cylinder_arbitrary.a, primitive->capped_cylinder_arbitrary.b };
			se_sdf_bounds_from_points(points, 2, fabsf(primitive->capped_cylinder_arbitrary.radius), out_min, out_max);
			return 1;
		}
		case SE_SDF_PRIMITIVE_ROUNDED_CYLINDER: {
			const f32 radial = fmaxf(fabsf(primitive->rounded_cylinder.outer_radius), fabsf(primitive->rounded_cylinder.corner_radius));
			const f32 h = fabsf(primitive->rounded_cylinder.half_height);
			*out_min = s_vec3(-radial, -h, -radial);
			*out_max = s_vec3(radial, h, radial);
			return 1;
		}
		case SE_SDF_PRIMITIVE_CAPPED_CONE: {
			const f32 h = fabsf(primitive->capped_cone.height);
			const f32 radial = fmaxf(fabsf(primitive->capped_cone.radius_base), fabsf(primitive->capped_cone.radius_top));
			*out_min = s_vec3(-radial, -h, -radial);
			*out_max = s_vec3(radial, h, radial);
			return 1;
		}
		case SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY: {
			const s_vec3 points[2] = { primitive->capped_cone_arbitrary.a, primitive->capped_cone_arbitrary.b };
			const f32 radial = fmaxf(fabsf(primitive->capped_cone_arbitrary.radius_a), fabsf(primitive->capped_cone_arbitrary.radius_b));
			se_sdf_bounds_from_points(points, 2, radial, out_min, out_max);
			return 1;
		}
		case SE_SDF_PRIMITIVE_SOLID_ANGLE: {
			const f32 r = fabsf(primitive->solid_angle.radius);
			*out_min = s_vec3(-r, -r, -r);
			*out_max = s_vec3(r, r, r);
			return 1;
		}
		case SE_SDF_PRIMITIVE_CUT_SPHERE: {
			const f32 r = fabsf(primitive->cut_sphere.radius);
			*out_min = s_vec3(-r, -r, -r);
			*out_max = s_vec3(r, r, r);
			return 1;
		}
		case SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE: {
			const f32 r = fabsf(primitive->cut_hollow_sphere.radius) + fabsf(primitive->cut_hollow_sphere.thickness);
			*out_min = s_vec3(-r, -r, -r);
			*out_max = s_vec3(r, r, r);
			return 1;
		}
		case SE_SDF_PRIMITIVE_DEATH_STAR: {
			const f32 ra = fabsf(primitive->death_star.radius_a);
			const f32 rb = fabsf(primitive->death_star.radius_b);
			const f32 d = fabsf(primitive->death_star.separation);
			const f32 ex = fmaxf(ra, d + rb);
			const f32 eyz = fmaxf(ra, rb);
			*out_min = s_vec3(-ex, -eyz, -eyz);
			*out_max = s_vec3(ex, eyz, eyz);
			return 1;
		}
		case SE_SDF_PRIMITIVE_ROUND_CONE: {
			const f32 h = fabsf(primitive->round_cone.height);
			const f32 r1 = fabsf(primitive->round_cone.radius_base);
			const f32 r2 = fabsf(primitive->round_cone.radius_top);
			const f32 radial = fmaxf(r1, r2);
			*out_min = s_vec3(-radial, -r1, -radial);
			*out_max = s_vec3(radial, h + r2, radial);
			return 1;
		}
		case SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY: {
			const s_vec3 points[2] = { primitive->round_cone_arbitrary.a, primitive->round_cone_arbitrary.b };
			const f32 radial = fmaxf(fabsf(primitive->round_cone_arbitrary.radius_a), fabsf(primitive->round_cone_arbitrary.radius_b));
			se_sdf_bounds_from_points(points, 2, radial, out_min, out_max);
			return 1;
		}
		case SE_SDF_PRIMITIVE_VESICA_SEGMENT: {
			const s_vec3 points[2] = { primitive->vesica_segment.a, primitive->vesica_segment.b };
			se_sdf_bounds_from_points(points, 2, fabsf(primitive->vesica_segment.width), out_min, out_max);
			return 1;
		}
		case SE_SDF_PRIMITIVE_RHOMBUS: {
			const f32 ex = fabsf(primitive->rhombus.length_a) + fabsf(primitive->rhombus.corner_radius);
			const f32 ey = fabsf(primitive->rhombus.height) + fabsf(primitive->rhombus.corner_radius);
			const f32 ez = fabsf(primitive->rhombus.length_b) + fabsf(primitive->rhombus.corner_radius);
			*out_min = s_vec3(-ex, -ey, -ez);
			*out_max = s_vec3(ex, ey, ez);
			return 1;
		}
		case SE_SDF_PRIMITIVE_OCTAHEDRON: {
			const f32 s = fabsf(primitive->octahedron.size);
			*out_min = s_vec3(-s, -s, -s);
			*out_max = s_vec3(s, s, s);
			return 1;
		}
		case SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND: {
			const f32 s = fabsf(primitive->octahedron_bound.size);
			*out_min = s_vec3(-s, -s, -s);
			*out_max = s_vec3(s, s, s);
			return 1;
		}
		case SE_SDF_PRIMITIVE_PYRAMID: {
			const f32 h = fabsf(primitive->pyramid.height);
			const f32 ex = 0.55f;
			const f32 ey = h + 0.55f;
			*out_min = s_vec3(-ex, -ey, -ex);
			*out_max = s_vec3(ex, ey, ex);
			return 1;
		}
		case SE_SDF_PRIMITIVE_TRIANGLE: {
			const s_vec3 points[3] = { primitive->triangle.a, primitive->triangle.b, primitive->triangle.c };
			se_sdf_bounds_from_points(points, 3, 0.0f, out_min, out_max);
			return 1;
		}
		case SE_SDF_PRIMITIVE_QUAD: {
			const s_vec3 points[4] = { primitive->quad.a, primitive->quad.b, primitive->quad.c, primitive->quad.d };
			se_sdf_bounds_from_points(points, 4, 0.0f, out_min, out_max);
			return 1;
		}
		default:
			break;
	}

	return 0;
}

static void se_sdf_collect_scene_bounds_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node_handle,
	const s_mat4* parent_transform,
	se_sdf_scene_bounds* bounds,
	const sz depth,
	const sz max_depth
) {
	if (!scene_ptr || !bounds || node_handle == SE_SDF_NODE_NULL || depth > max_depth) {
		return;
	}
	se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, node_handle);
	if (!node) {
		return;
	}

	s_mat4 world_transform = node->transform;
	if (parent_transform) {
		world_transform = s_mat4_mul(parent_transform, &node->transform);
	}

	if (!node->is_group) {
		s_vec3 local_min = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 local_max = s_vec3(0.0f, 0.0f, 0.0f);
		b8 is_unbounded = 0;
		if (se_sdf_get_primitive_local_bounds(&node->primitive, &local_min, &local_max, &is_unbounded)) {
			se_sdf_scene_bounds_expand_transformed_aabb(bounds, &world_transform, &local_min, &local_max);
		}
		if (is_unbounded) {
			bounds->has_unbounded_primitives = 1;
		}
	}

	for (sz i = 0; i < s_array_get_size(&node->children); ++i) {
		se_sdf_node_handle* child = s_array_get(&node->children, s_array_handle(&node->children, (u32)i));
		if (!child) {
			continue;
		}
		se_sdf_collect_scene_bounds_recursive(
			scene_ptr,
			scene,
			*child,
			&world_transform,
			bounds,
			depth + 1,
			max_depth
		);
	}
}

b8 se_sdf_scene_calculate_bounds(const se_sdf_scene_handle scene, se_sdf_scene_bounds* out_bounds) {
	if (!out_bounds || scene == SE_SDF_SCENE_NULL) {
		return 0;
	}

	*out_bounds = SE_SDF_SCENE_BOUNDS_DEFAULTS;

	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr || scene_ptr->root == SE_SDF_NODE_NULL) {
		return 0;
	}

	const sz max_depth = s_array_get_size(&scene_ptr->nodes) + 1;
	se_sdf_collect_scene_bounds_recursive(
		scene_ptr,
		scene,
		scene_ptr->root,
		NULL,
		out_bounds,
		0,
		max_depth
	);

	if (!out_bounds->valid) {
		return 0;
	}

	out_bounds->center = s_vec3(
		(out_bounds->min.x + out_bounds->max.x) * 0.5f,
		(out_bounds->min.y + out_bounds->max.y) * 0.5f,
		(out_bounds->min.z + out_bounds->max.z) * 0.5f
	);
	const s_vec3 span = s_vec3(
		out_bounds->max.x - out_bounds->min.x,
		out_bounds->max.y - out_bounds->min.y,
		out_bounds->max.z - out_bounds->min.z
	);
	out_bounds->radius = 0.5f * sqrtf(span.x * span.x + span.y * span.y + span.z * span.z);
	return 1;
}

b8 se_sdf_scene_align_camera(
	const se_sdf_scene_handle scene,
	const se_camera_handle camera,
	const se_sdf_camera_align_desc* desc,
	se_sdf_scene_bounds* out_bounds
) {
	if (camera == S_HANDLE_NULL) {
		return 0;
	}
	se_camera* camera_ptr = se_camera_get(camera);
	if (!camera_ptr) {
		return 0;
	}

	se_sdf_scene_bounds bounds = SE_SDF_SCENE_BOUNDS_DEFAULTS;
	if (!se_sdf_scene_calculate_bounds(scene, &bounds)) {
		if (out_bounds) {
			*out_bounds = bounds;
		}
		return 0;
	}
	if (out_bounds) {
		*out_bounds = bounds;
	}

	se_sdf_camera_align_desc align = SE_SDF_CAMERA_ALIGN_DESC_DEFAULTS;
	if (desc) {
		align = *desc;
	}

	f32 padding = align.padding;
	if (padding <= 0.0f) {
		padding = 1.0f;
	}
	f32 radius = fmaxf(bounds.radius * padding, fmaxf(align.min_radius, 0.0f));
	if (radius < 0.0001f) {
		radius = 0.0001f;
	}

	s_vec3 direction = align.view_direction;
	f32 direction_len = s_vec3_length(&direction);
	if (direction_len <= 0.0001f) {
		direction = s_vec3(0.0f, 0.0f, 1.0f);
		direction_len = 1.0f;
	}
	direction = s_vec3_divs(&direction, direction_len);

	const f32 min_distance = fmaxf(align.min_distance, 0.0001f);
	const f32 aspect = camera_ptr->aspect > 0.0001f ? camera_ptr->aspect : 1.0f;
	f32 near_plane = camera_ptr->near;
	f32 far_plane = camera_ptr->far;
	f32 distance = min_distance;

	if (camera_ptr->use_orthographic) {
		f32 ortho_height = 2.0f * radius;
		if (aspect < 1.0f && aspect > 0.0001f) {
			ortho_height = 2.0f * radius / aspect;
		}
		distance = fmaxf(min_distance, radius * 2.0f);
		if (align.update_clip_planes) {
			const f32 near_margin = fmaxf(align.near_margin, 0.0f);
			const f32 far_margin = fmaxf(align.far_margin, 0.0f);
			near_plane = fmaxf(0.01f, distance - radius - near_margin);
			far_plane = fmaxf(near_plane + 0.01f, distance + radius + far_margin);
		}
		se_camera_set_orthographic(camera, ortho_height, near_plane, far_plane);
	} else {
		const f32 pi = 3.14159265358979323846f;
		f32 fov_degrees = camera_ptr->fov;
		if (fov_degrees <= 1.0f || fov_degrees >= 179.0f) {
			fov_degrees = 45.0f;
		}
		const f32 half_fov_y = (fov_degrees * (pi / 180.0f)) * 0.5f;
		const f32 half_fov_x = atanf(tanf(half_fov_y) * aspect);
		const f32 fit_half_fov = fmaxf(0.01f, fminf(half_fov_x, half_fov_y));
		distance = fmaxf(min_distance, radius / fmaxf(sinf(fit_half_fov), 0.0001f));
		if (align.update_clip_planes) {
			const f32 near_margin = fmaxf(align.near_margin, 0.0f);
			const f32 far_margin = fmaxf(align.far_margin, 0.0f);
			near_plane = fmaxf(0.01f, distance - radius - near_margin);
			far_plane = fmaxf(near_plane + 0.01f, distance + radius + far_margin);
		}
		se_camera_set_perspective(camera, fov_degrees, near_plane, far_plane);
	}

	const s_vec3 target = bounds.center;
	const s_vec3 position = s_vec3_add(&bounds.center, &s_vec3_muls(&direction, distance));
	se_camera_set_target_mode(camera, true);
	se_camera_set_location(camera, &position);
	se_camera_set_target(camera, &target);
	return 1;
}

typedef enum {
	SE_SDF_PHYSICS_DETAIL_NEAR = 0,
	SE_SDF_PHYSICS_DETAIL_MID,
	SE_SDF_PHYSICS_DETAIL_FAR
} se_sdf_physics_detail_tier;

typedef struct {
	s_vec3 offset;
	s_vec3 half_extents;
	f32 volume;
} se_sdf_physics_box;
typedef s_array(se_sdf_physics_box, se_sdf_physics_boxes);
typedef s_array(u8, se_sdf_u8_array);

static b8 se_sdf_physics_vec3_is_finite(const s_vec3* v) {
	if (!v) {
		return 0;
	}
	return isfinite(v->x) && isfinite(v->y) && isfinite(v->z);
}

static f32 se_sdf_physics_clampf(const f32 v, const f32 min_v, const f32 max_v) {
	if (v < min_v) {
		return min_v;
	}
	if (v > max_v) {
		return max_v;
	}
	return v;
}

static i32 se_sdf_physics_clampi(const i32 v, const i32 min_v, const i32 max_v) {
	if (v < min_v) {
		return min_v;
	}
	if (v > max_v) {
		return max_v;
	}
	return v;
}

static f32 se_sdf_physics_length2(const f32 x, const f32 y) {
	return sqrtf(x * x + y * y);
}

static f32 se_sdf_physics_length3(const f32 x, const f32 y, const f32 z) {
	return sqrtf(x * x + y * y + z * z);
}

static f32 se_sdf_physics_dot2(const f32 ax, const f32 ay, const f32 bx, const f32 by) {
	return ax * bx + ay * by;
}

static f32 se_sdf_physics_box_distance(const s_vec3* p, const s_vec3* half_extents) {
	if (!p || !half_extents) {
		return 1e9f;
	}
	const f32 qx = fabsf(p->x) - half_extents->x;
	const f32 qy = fabsf(p->y) - half_extents->y;
	const f32 qz = fabsf(p->z) - half_extents->z;
	const f32 ox = fmaxf(qx, 0.0f);
	const f32 oy = fmaxf(qy, 0.0f);
	const f32 oz = fmaxf(qz, 0.0f);
	const f32 outside = sqrtf(ox * ox + oy * oy + oz * oz);
	const f32 inside = fminf(fmaxf(qx, fmaxf(qy, qz)), 0.0f);
	return outside + inside;
}

static b8 se_sdf_physics_object_to_primitive_desc(
	const se_sdf_object* object,
	se_sdf_primitive_desc* out
) {
	if (!object || !out) {
		return 0;
	}

	memset(out, 0, sizeof(*out));
	switch (object->type) {
		case SE_SDF_SPHERE:
			out->type = SE_SDF_PRIMITIVE_SPHERE;
			out->sphere.radius = object->sphere.radius;
			return 1;
		case SE_SDF_BOX:
			out->type = SE_SDF_PRIMITIVE_BOX;
			out->box.size = object->box.size;
			return 1;
		case SE_SDF_ROUND_BOX:
			out->type = SE_SDF_PRIMITIVE_ROUND_BOX;
			out->round_box.size = object->round_box.size;
			out->round_box.roundness = object->round_box.roundness;
			return 1;
		case SE_SDF_BOX_FRAME:
			out->type = SE_SDF_PRIMITIVE_BOX_FRAME;
			out->box_frame.size = object->box_frame.size;
			out->box_frame.thickness = object->box_frame.thickness;
			return 1;
		case SE_SDF_TORUS:
			out->type = SE_SDF_PRIMITIVE_TORUS;
			out->torus.radii = object->torus.radii;
			return 1;
		case SE_SDF_CAPPED_TORUS:
			out->type = SE_SDF_PRIMITIVE_CAPPED_TORUS;
			out->capped_torus.axis_sin_cos = object->capped_torus.axis_sin_cos;
			out->capped_torus.major_radius = object->capped_torus.major_radius;
			out->capped_torus.minor_radius = object->capped_torus.minor_radius;
			return 1;
		case SE_SDF_LINK:
			out->type = SE_SDF_PRIMITIVE_LINK;
			out->link.half_length = object->link.half_length;
			out->link.outer_radius = object->link.outer_radius;
			out->link.inner_radius = object->link.inner_radius;
			return 1;
		case SE_SDF_CYLINDER:
			out->type = SE_SDF_PRIMITIVE_CYLINDER;
			out->cylinder.axis_and_radius = object->cylinder.axis_and_radius;
			return 1;
		case SE_SDF_CONE:
			out->type = SE_SDF_PRIMITIVE_CONE;
			out->cone.angle_sin_cos = object->cone.angle_sin_cos;
			out->cone.height = object->cone.height;
			return 1;
		case SE_SDF_CONE_INFINITE:
			out->type = SE_SDF_PRIMITIVE_CONE_INFINITE;
			out->cone_infinite.angle_sin_cos = object->cone_infinite.angle_sin_cos;
			return 1;
		case SE_SDF_PLANE:
			out->type = SE_SDF_PRIMITIVE_PLANE;
			out->plane.normal = object->plane.normal;
			out->plane.offset = object->plane.offset;
			return 1;
		case SE_SDF_HEX_PRISM:
			out->type = SE_SDF_PRIMITIVE_HEX_PRISM;
			out->hex_prism.half_size = object->hex_prism.half_size;
			return 1;
		case SE_SDF_CAPSULE:
			out->type = SE_SDF_PRIMITIVE_CAPSULE;
			out->capsule.a = object->capsule.a;
			out->capsule.b = object->capsule.b;
			out->capsule.radius = object->capsule.radius;
			return 1;
		case SE_SDF_VERTICAL_CAPSULE:
			out->type = SE_SDF_PRIMITIVE_VERTICAL_CAPSULE;
			out->vertical_capsule.height = object->vertical_capsule.height;
			out->vertical_capsule.radius = object->vertical_capsule.radius;
			return 1;
		case SE_SDF_CAPPED_CYLINDER:
			out->type = SE_SDF_PRIMITIVE_CAPPED_CYLINDER;
			out->capped_cylinder.radius = object->capped_cylinder.radius;
			out->capped_cylinder.half_height = object->capped_cylinder.half_height;
			return 1;
		case SE_SDF_CAPPED_CYLINDER_ARBITRARY:
			out->type = SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY;
			out->capped_cylinder_arbitrary.a = object->capped_cylinder_arbitrary.a;
			out->capped_cylinder_arbitrary.b = object->capped_cylinder_arbitrary.b;
			out->capped_cylinder_arbitrary.radius = object->capped_cylinder_arbitrary.radius;
			return 1;
		case SE_SDF_ROUNDED_CYLINDER:
			out->type = SE_SDF_PRIMITIVE_ROUNDED_CYLINDER;
			out->rounded_cylinder.outer_radius = object->rounded_cylinder.outer_radius;
			out->rounded_cylinder.corner_radius = object->rounded_cylinder.corner_radius;
			out->rounded_cylinder.half_height = object->rounded_cylinder.half_height;
			return 1;
		case SE_SDF_CAPPED_CONE:
			out->type = SE_SDF_PRIMITIVE_CAPPED_CONE;
			out->capped_cone.height = object->capped_cone.height;
			out->capped_cone.radius_base = object->capped_cone.radius_base;
			out->capped_cone.radius_top = object->capped_cone.radius_top;
			return 1;
		case SE_SDF_CAPPED_CONE_ARBITRARY:
			out->type = SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY;
			out->capped_cone_arbitrary.a = object->capped_cone_arbitrary.a;
			out->capped_cone_arbitrary.b = object->capped_cone_arbitrary.b;
			out->capped_cone_arbitrary.radius_a = object->capped_cone_arbitrary.radius_a;
			out->capped_cone_arbitrary.radius_b = object->capped_cone_arbitrary.radius_b;
			return 1;
		case SE_SDF_SOLID_ANGLE:
			out->type = SE_SDF_PRIMITIVE_SOLID_ANGLE;
			out->solid_angle.angle_sin_cos = object->solid_angle.angle_sin_cos;
			out->solid_angle.radius = object->solid_angle.radius;
			return 1;
		case SE_SDF_CUT_SPHERE:
			out->type = SE_SDF_PRIMITIVE_CUT_SPHERE;
			out->cut_sphere.radius = object->cut_sphere.radius;
			out->cut_sphere.cut_height = object->cut_sphere.cut_height;
			return 1;
		case SE_SDF_CUT_HOLLOW_SPHERE:
			out->type = SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE;
			out->cut_hollow_sphere.radius = object->cut_hollow_sphere.radius;
			out->cut_hollow_sphere.cut_height = object->cut_hollow_sphere.cut_height;
			out->cut_hollow_sphere.thickness = object->cut_hollow_sphere.thickness;
			return 1;
		case SE_SDF_DEATH_STAR:
			out->type = SE_SDF_PRIMITIVE_DEATH_STAR;
			out->death_star.radius_a = object->death_star.radius_a;
			out->death_star.radius_b = object->death_star.radius_b;
			out->death_star.separation = object->death_star.separation;
			return 1;
		case SE_SDF_ROUND_CONE:
			out->type = SE_SDF_PRIMITIVE_ROUND_CONE;
			out->round_cone.radius_base = object->round_cone.radius_base;
			out->round_cone.radius_top = object->round_cone.radius_top;
			out->round_cone.height = object->round_cone.height;
			return 1;
		case SE_SDF_ROUND_CONE_ARBITRARY:
			out->type = SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY;
			out->round_cone_arbitrary.a = object->round_cone_arbitrary.a;
			out->round_cone_arbitrary.b = object->round_cone_arbitrary.b;
			out->round_cone_arbitrary.radius_a = object->round_cone_arbitrary.radius_a;
			out->round_cone_arbitrary.radius_b = object->round_cone_arbitrary.radius_b;
			return 1;
		case SE_SDF_VESICA_SEGMENT:
			out->type = SE_SDF_PRIMITIVE_VESICA_SEGMENT;
			out->vesica_segment.a = object->vesica_segment.a;
			out->vesica_segment.b = object->vesica_segment.b;
			out->vesica_segment.width = object->vesica_segment.width;
			return 1;
		case SE_SDF_RHOMBUS:
			out->type = SE_SDF_PRIMITIVE_RHOMBUS;
			out->rhombus.length_a = object->rhombus.length_a;
			out->rhombus.length_b = object->rhombus.length_b;
			out->rhombus.height = object->rhombus.height;
			out->rhombus.corner_radius = object->rhombus.corner_radius;
			return 1;
		case SE_SDF_OCTAHEDRON:
			out->type = SE_SDF_PRIMITIVE_OCTAHEDRON;
			out->octahedron.size = object->octahedron.size;
			return 1;
		case SE_SDF_OCTAHEDRON_BOUND:
			out->type = SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND;
			out->octahedron_bound.size = object->octahedron_bound.size;
			return 1;
		case SE_SDF_PYRAMID:
			out->type = SE_SDF_PRIMITIVE_PYRAMID;
			out->pyramid.height = object->pyramid.height;
			return 1;
		case SE_SDF_TRIANGLE:
			out->type = SE_SDF_PRIMITIVE_TRIANGLE;
			out->triangle.a = object->triangle.a;
			out->triangle.b = object->triangle.b;
			out->triangle.c = object->triangle.c;
			return 1;
		case SE_SDF_QUAD:
			out->type = SE_SDF_PRIMITIVE_QUAD;
			out->quad.a = object->quad.a;
			out->quad.b = object->quad.b;
			out->quad.c = object->quad.c;
			out->quad.d = object->quad.d;
			return 1;
		case SE_SDF_NONE:
		default:
			break;
	}
	return 0;
}

static f32 se_sdf_physics_eval_primitive_distance_local(
	se_sdf_object* object,
	const s_vec3* local_point
) {
	if (!object || !local_point) {
		return 1e9f;
	}

	switch (object->type) {
		case SE_SDF_SPHERE:
			return se_sdf_physics_length3(local_point->x, local_point->y, local_point->z) - fabsf(object->sphere.radius);
		case SE_SDF_BOX: {
			const s_vec3 half = s_vec3(
				fabsf(object->box.size.x),
				fabsf(object->box.size.y),
				fabsf(object->box.size.z)
			);
			return se_sdf_physics_box_distance(local_point, &half);
		}
		case SE_SDF_ROUND_BOX: {
			const f32 r = fabsf(object->round_box.roundness);
			const s_vec3 half = s_vec3(
				fabsf(object->round_box.size.x) + r,
				fabsf(object->round_box.size.y) + r,
				fabsf(object->round_box.size.z) + r
			);
			return se_sdf_physics_box_distance(local_point, &half) - r;
		}
		case SE_SDF_TORUS: {
			const f32 major_r = fabsf(object->torus.radii.x);
			const f32 minor_r = fabsf(object->torus.radii.y);
			const f32 qx = se_sdf_physics_length2(local_point->x, local_point->z) - major_r;
			const f32 qy = local_point->y;
			return se_sdf_physics_length2(qx, qy) - minor_r;
		}
		case SE_SDF_CAPSULE: {
			const s_vec3 pa = s_vec3_sub(local_point, &object->capsule.a);
			const s_vec3 ba = s_vec3_sub(&object->capsule.b, &object->capsule.a);
			const f32 ba_len2 = s_vec3_dot(&ba, &ba);
			f32 h = 0.0f;
			if (ba_len2 > 0.000001f) {
				h = se_sdf_physics_clampf(s_vec3_dot(&pa, &ba) / ba_len2, 0.0f, 1.0f);
			}
			const s_vec3 q = s_vec3_sub(&pa, &s_vec3_muls(&ba, h));
			return s_vec3_length(&q) - fabsf(object->capsule.radius);
		}
		case SE_SDF_VERTICAL_CAPSULE: {
			s_vec3 q = *local_point;
			const f32 h = fmaxf(fabsf(object->vertical_capsule.height), 0.0f);
			q.y -= se_sdf_physics_clampf(q.y, 0.0f, h);
			return s_vec3_length(&q) - fabsf(object->vertical_capsule.radius);
		}
		case SE_SDF_CAPPED_CYLINDER: {
			const f32 r = fabsf(object->capped_cylinder.radius);
			const f32 h = fabsf(object->capped_cylinder.half_height);
			const f32 dx = fabsf(se_sdf_physics_length2(local_point->x, local_point->z)) - r;
			const f32 dy = fabsf(local_point->y) - h;
			const f32 ox = fmaxf(dx, 0.0f);
			const f32 oy = fmaxf(dy, 0.0f);
			return fminf(fmaxf(dx, dy), 0.0f) + se_sdf_physics_length2(ox, oy);
		}
		case SE_SDF_CONE: {
			const f32 qx = se_sdf_physics_length2(local_point->x, local_point->z);
			const f32 qy = local_point->y;
			const f32 sx = object->cone.angle_sin_cos.x;
			const f32 sy = object->cone.angle_sin_cos.y;
			const f32 h = fabsf(object->cone.height);
			const f32 k1x = sy;
			const f32 k1y = h;
			const f32 k2x = sy - sx;
			const f32 k2y = 2.0f * h;
			const f32 ca_x = qx - fminf(qx, (qy < 0.0f) ? sx : sy);
			const f32 ca_y = fabsf(qy) - h;
			const f32 k2_dot = k2x * k2x + k2y * k2y;
			f32 t = 0.0f;
			if (k2_dot > 0.000001f) {
				t = se_sdf_physics_clampf(
					((k1x - qx) * k2x + (k1y - qy) * k2y) / k2_dot,
					0.0f,
					1.0f
				);
			}
			const f32 cb_x = qx - k1x + k2x * t;
			const f32 cb_y = qy - k1y + k2y * t;
			const f32 s = (cb_x < 0.0f && ca_y < 0.0f) ? -1.0f : 1.0f;
			return s * sqrtf(fminf(ca_x * ca_x + ca_y * ca_y, cb_x * cb_x + cb_y * cb_y));
		}
		case SE_SDF_CONE_INFINITE: {
			const f32 sx = object->cone_infinite.angle_sin_cos.x;
			const f32 sy = object->cone_infinite.angle_sin_cos.y;
			const f32 qx = se_sdf_physics_length2(local_point->x, local_point->z);
			const f32 qy = -local_point->y;
			const f32 dot_v = se_sdf_physics_dot2(qx, qy, sx, sy);
			const f32 m = fmaxf(dot_v, 0.0f);
			const f32 wx = qx - sx * m;
			const f32 wy = qy - sy * m;
			const f32 d = se_sdf_physics_length2(wx, wy);
			return d * ((qx * sy - qy * sx < 0.0f) ? -1.0f : 1.0f);
		}
		case SE_SDF_PLANE: {
			const s_vec3 n = object->plane.normal;
			return s_vec3_dot(local_point, &n) + object->plane.offset;
		}
		case SE_SDF_CYLINDER: {
			const f32 dx = local_point->x - object->cylinder.axis_and_radius.x;
			const f32 dz = local_point->z - object->cylinder.axis_and_radius.y;
			return se_sdf_physics_length2(dx, dz) - fabsf(object->cylinder.axis_and_radius.z);
		}
		case SE_SDF_CUT_SPHERE: {
			const f32 r = fabsf(object->cut_sphere.radius);
			const f32 ch = object->cut_sphere.cut_height;
			const f32 w = sqrtf(fmaxf(r * r - ch * ch, 0.0f));
			const f32 qx = se_sdf_physics_length2(local_point->x, local_point->z);
			const f32 qy = local_point->y;
			const f32 s = fmaxf(
				(ch - r) * qx * qx + w * w * (ch + r - 2.0f * qy),
				ch * qx - w * qy
			);
			if (s < 0.0f) {
				return se_sdf_physics_length2(qx, qy) - r;
			}
			if (qx < w) {
				return ch - qy;
			}
			return se_sdf_physics_length2(qx - w, qy - ch);
		}
		case SE_SDF_CUT_HOLLOW_SPHERE: {
			const f32 r = fabsf(object->cut_hollow_sphere.radius);
			const f32 ch = object->cut_hollow_sphere.cut_height;
			const f32 t = fabsf(object->cut_hollow_sphere.thickness);
			const f32 w = sqrtf(fmaxf(r * r - ch * ch, 0.0f));
			const f32 qx = se_sdf_physics_length2(local_point->x, local_point->z);
			const f32 qy = local_point->y;
			const f32 candidate = (ch * qx < w * qy)
				? se_sdf_physics_length2(qx - w, qy - ch)
				: fabsf(se_sdf_physics_length2(qx, qy) - r);
			return candidate - t;
		}
		case SE_SDF_ROUND_CONE: {
			const f32 r1 = fabsf(object->round_cone.radius_base);
			const f32 r2 = fabsf(object->round_cone.radius_top);
			const f32 h = fmaxf(fabsf(object->round_cone.height), 0.0001f);
			const f32 b = (r1 - r2) / h;
			const f32 a = sqrtf(fmaxf(1.0f - b * b, 0.0f));
			const f32 qx = se_sdf_physics_length2(local_point->x, local_point->z);
			const f32 qy = local_point->y;
			const f32 k = se_sdf_physics_dot2(qx, qy, -b, a);
			if (k < 0.0f) {
				return se_sdf_physics_length2(qx, qy) - r1;
			}
			if (k > a * h) {
				return se_sdf_physics_length2(qx, qy - h) - r2;
			}
			return se_sdf_physics_dot2(qx, qy, a, b) - r1;
		}
		case SE_SDF_NONE:
		default: {
			se_sdf_primitive_desc primitive = {0};
			if (!se_sdf_physics_object_to_primitive_desc(object, &primitive)) {
				return 1e9f;
			}
			s_vec3 local_min = s_vec3(0.0f, 0.0f, 0.0f);
			s_vec3 local_max = s_vec3(0.0f, 0.0f, 0.0f);
			b8 is_unbounded = 0;
			if (!se_sdf_get_primitive_local_bounds(&primitive, &local_min, &local_max, &is_unbounded)) {
				return is_unbounded ? 1e9f : 0.0f;
			}
			const s_vec3 center = s_vec3(
				(local_min.x + local_max.x) * 0.5f,
				(local_min.y + local_max.y) * 0.5f,
				(local_min.z + local_max.z) * 0.5f
			);
			const s_vec3 half = s_vec3(
				fmaxf((local_max.x - local_min.x) * 0.5f, 0.0001f),
				fmaxf((local_max.y - local_min.y) * 0.5f, 0.0001f),
				fmaxf((local_max.z - local_min.z) * 0.5f, 0.0001f)
			);
			const s_vec3 p = s_vec3_sub(local_point, &center);
			return se_sdf_physics_box_distance(&p, &half);
		}
	}
}

static f32 se_sdf_physics_apply_operation(
	const se_sdf_operation operation,
	const f32 lhs,
	const f32 rhs
) {
	switch (operation) {
		case SE_SDF_OP_UNION:
		case SE_SDF_OP_SMOOTH_UNION:
			return fminf(lhs, rhs);
		case SE_SDF_OP_INTERSECTION:
		case SE_SDF_OP_SMOOTH_INTERSECTION:
			return fmaxf(lhs, rhs);
		case SE_SDF_OP_SUBTRACTION:
		case SE_SDF_OP_SMOOTH_SUBTRACTION:
			return fmaxf(lhs, -rhs);
		case SE_SDF_OP_NONE:
		default:
			return lhs;
	}
}

static f32 se_sdf_physics_apply_noise(
	const se_sdf_noise* noise,
	const s_vec3* local_point,
	const f32 distance
) {
	if (!noise || !noise->active || !local_point) {
		return distance;
	}
	const f32 frequency = fmaxf(fabsf(noise->frequency), 0.0001f);
	const f32 n = sinf(local_point->x * frequency)
		* sinf(local_point->y * frequency)
		* sinf(local_point->z * frequency);
	return distance + noise->offset + noise->scale * n;
}

static f32 se_sdf_physics_eval_object_distance_recursive(
	se_sdf_object* object,
	const s_vec3* world_point,
	const s_mat4* parent_transform,
	const sz depth,
	const sz max_depth
) {
	if (!object || !world_point || !parent_transform || depth > max_depth) {
		return 1e9f;
	}

	const s_mat4 world_transform = s_mat4_mul(parent_transform, &object->transform);
	const s_mat4 inverse_transform = s_mat4_inverse(&world_transform);
	const s_vec3 local_point = se_sdf_mul_mat4_point(&inverse_transform, world_point);
	if (!se_sdf_physics_vec3_is_finite(&local_point)) {
		return 1e9f;
	}

	const sz child_count = s_array_get_size(&object->children);
	f32 distance = 1e9f;
	if (child_count > 0) {
		se_sdf_object* first = s_array_get(&object->children, s_array_handle(&object->children, 0));
		distance = se_sdf_physics_eval_object_distance_recursive(
			first,
			world_point,
			&world_transform,
			depth + 1,
			max_depth
		);
		for (sz i = 1; i < child_count; ++i) {
			se_sdf_object* child = s_array_get(&object->children, s_array_handle(&object->children, (u32)i));
			const f32 rhs = se_sdf_physics_eval_object_distance_recursive(
				child,
				world_point,
				&world_transform,
				depth + 1,
				max_depth
			);
			distance = se_sdf_physics_apply_operation(object->operation, distance, rhs);
		}
	} else {
		distance = se_sdf_physics_eval_primitive_distance_local(object, &local_point);
	}

	distance = se_sdf_physics_apply_noise(&object->noise, &local_point, distance);
	return isfinite(distance) ? distance : 1e9f;
}

static void se_sdf_physics_collect_object_bounds_recursive(
	se_sdf_object* object,
	const s_mat4* parent_transform,
	se_sdf_scene_bounds* bounds,
	const sz depth,
	const sz max_depth
) {
	if (!object || !parent_transform || !bounds || depth > max_depth) {
		return;
	}

	const s_mat4 world_transform = s_mat4_mul(parent_transform, &object->transform);
	const sz child_count = s_array_get_size(&object->children);

	if (child_count == 0 && object->type != SE_SDF_NONE) {
		se_sdf_primitive_desc primitive = {0};
		if (se_sdf_physics_object_to_primitive_desc(object, &primitive)) {
			s_vec3 local_min = s_vec3(0.0f, 0.0f, 0.0f);
			s_vec3 local_max = s_vec3(0.0f, 0.0f, 0.0f);
			b8 is_unbounded = 0;
			if (se_sdf_get_primitive_local_bounds(&primitive, &local_min, &local_max, &is_unbounded)) {
				se_sdf_scene_bounds_expand_transformed_aabb(bounds, &world_transform, &local_min, &local_max);
			} else if (is_unbounded) {
				bounds->has_unbounded_primitives = 1;
			}
		}
	}

	for (sz i = 0; i < child_count; ++i) {
		se_sdf_object* child = s_array_get(&object->children, s_array_handle(&object->children, (u32)i));
		se_sdf_physics_collect_object_bounds_recursive(
			child,
			&world_transform,
			bounds,
			depth + 1,
			max_depth
		);
	}
}

static void se_sdf_physics_finalize_bounds(se_sdf_scene_bounds* bounds) {
	if (!bounds || !bounds->valid) {
		return;
	}
	bounds->center = s_vec3(
		(bounds->min.x + bounds->max.x) * 0.5f,
		(bounds->min.y + bounds->max.y) * 0.5f,
		(bounds->min.z + bounds->max.z) * 0.5f
	);
	const s_vec3 span = s_vec3(
		bounds->max.x - bounds->min.x,
		bounds->max.y - bounds->min.y,
		bounds->max.z - bounds->min.z
	);
	bounds->radius = 0.5f * sqrtf(span.x * span.x + span.y * span.y + span.z * span.z);
}

static se_sdf_physics_detail_tier se_sdf_physics_pick_tier(
	const se_sdf_scene_bounds* bounds,
	const s_vec3* reference_position
) {
	if (!bounds || !bounds->valid || !reference_position) {
		return SE_SDF_PHYSICS_DETAIL_MID;
	}
	const s_vec3 delta = s_vec3_sub(reference_position, &bounds->center);
	const f32 distance = s_vec3_length(&delta);
	const f32 normalized = distance / fmaxf(bounds->radius, 0.001f);
	if (normalized > 8.0f) {
		return SE_SDF_PHYSICS_DETAIL_FAR;
	}
	if (normalized > 2.0f) {
		return SE_SDF_PHYSICS_DETAIL_MID;
	}
	return SE_SDF_PHYSICS_DETAIL_NEAR;
}

static b8 se_sdf_physics_extract_simple_transform(
	const s_mat4* transform,
	s_vec3* out_translation,
	s_vec3* out_scale
) {
	if (!transform || !out_translation || !out_scale) {
		return 0;
	}
	const f32 eps = 0.0001f;
	if (fabsf(transform->m[0][1]) > eps || fabsf(transform->m[0][2]) > eps ||
		fabsf(transform->m[1][0]) > eps || fabsf(transform->m[1][2]) > eps ||
		fabsf(transform->m[2][0]) > eps || fabsf(transform->m[2][1]) > eps ||
		fabsf(transform->m[0][3]) > eps || fabsf(transform->m[1][3]) > eps ||
		fabsf(transform->m[2][3]) > eps || fabsf(transform->m[3][3] - 1.0f) > eps) {
		return 0;
	}
	*out_translation = s_vec3(transform->m[3][0], transform->m[3][1], transform->m[3][2]);
	*out_scale = s_vec3(
		fabsf(transform->m[0][0]),
		fabsf(transform->m[1][1]),
		fabsf(transform->m[2][2])
	);
	return (out_scale->x > eps && out_scale->y > eps && out_scale->z > eps);
}

static b8 se_sdf_physics_try_add_simple_shape(
	se_physics_world_3d_handle world,
	se_physics_body_3d_handle body,
	se_sdf_object* object,
	const b8 is_trigger
) {
	if (!world || !body || !object || object->noise.active || s_array_get_size(&object->children) != 0) {
		return 0;
	}
	s_vec3 offset = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 scale = s_vec3(1.0f, 1.0f, 1.0f);
	if (!se_sdf_physics_extract_simple_transform(&object->transform, &offset, &scale)) {
		return 0;
	}

	switch (object->type) {
		case SE_SDF_SPHERE: {
			if (fabsf(scale.x - scale.y) > 0.0001f || fabsf(scale.y - scale.z) > 0.0001f) {
				return 0;
			}
			const f32 radius = fabsf(object->sphere.radius) * scale.x;
			return se_physics_body_3d_add_sphere(world, body, &offset, radius, is_trigger) != SE_PHYSICS_SHAPE_3D_HANDLE_NULL;
		}
		case SE_SDF_BOX: {
			const s_vec3 half_extents = s_vec3(
				fabsf(object->box.size.x) * scale.x,
				fabsf(object->box.size.y) * scale.y,
				fabsf(object->box.size.z) * scale.z
			);
			const s_vec3 rotation = s_vec3(0.0f, 0.0f, 0.0f);
			return se_physics_body_3d_add_box(world, body, &offset, &half_extents, &rotation, is_trigger) != SE_PHYSICS_SHAPE_3D_HANDLE_NULL;
		}
		case SE_SDF_ROUND_BOX: {
			const f32 r = fabsf(object->round_box.roundness);
			const s_vec3 half_extents = s_vec3(
				(fabsf(object->round_box.size.x) + r) * scale.x,
				(fabsf(object->round_box.size.y) + r) * scale.y,
				(fabsf(object->round_box.size.z) + r) * scale.z
			);
			const s_vec3 rotation = s_vec3(0.0f, 0.0f, 0.0f);
			return se_physics_body_3d_add_box(world, body, &offset, &half_extents, &rotation, is_trigger) != SE_PHYSICS_SHAPE_3D_HANDLE_NULL;
		}
		default:
			break;
	}
	return 0;
}

static sz se_sdf_physics_voxel_index(
	const i32 x,
	const i32 y,
	const i32 z,
	const i32 nx,
	const i32 ny
) {
	return (sz)x + (sz)nx * ((sz)y + (sz)ny * (sz)z);
}

static void se_sdf_physics_u8_array_set_count(
	se_sdf_u8_array* array,
	const sz count,
	const u8 fill_value
) {
	if (!array) {
		return;
	}
	s_array_reserve(array, count);
	for (sz i = 0; i < count; ++i) {
		u8 value = fill_value;
		s_array_add(array, value);
	}
}

static void se_sdf_physics_build_voxel_occupancy(
	se_sdf_object* object,
	const s_vec3* bounds_min,
	const s_vec3* cell_size,
	const i32 nx,
	const i32 ny,
	const i32 nz,
	const f32 inside_bias,
	se_sdf_u8_array* out_occupancy
) {
	if (!object || !bounds_min || !cell_size || !out_occupancy) {
		return;
	}
	const s_mat4 identity = s_mat4_identity;
	for (i32 z = 0; z < nz; ++z) {
		for (i32 y = 0; y < ny; ++y) {
			for (i32 x = 0; x < nx; ++x) {
				const s_vec3 sample_point = s_vec3(
					bounds_min->x + ((f32)x + 0.5f) * cell_size->x,
					bounds_min->y + ((f32)y + 0.5f) * cell_size->y,
					bounds_min->z + ((f32)z + 0.5f) * cell_size->z
				);
				const f32 distance = se_sdf_physics_eval_object_distance_recursive(
					object,
					&sample_point,
					&identity,
					0,
					128
				);
				u8 filled = (distance <= inside_bias) ? 1u : 0u;
				s_array_add(out_occupancy, filled);
			}
		}
	}
}

static void se_sdf_physics_greedy_merge_voxels(
	const s_vec3* bounds_min,
	const s_vec3* cell_size,
	const i32 nx,
	const i32 ny,
	const i32 nz,
	se_sdf_u8_array* occupancy,
	se_sdf_physics_boxes* out_boxes
) {
	if (!bounds_min || !cell_size || !occupancy || !out_boxes) {
		return;
	}

	se_sdf_u8_array used;
	s_array_init(&used);
	se_sdf_physics_u8_array_set_count(&used, s_array_get_size(occupancy), 0u);
	u8* occ_data = s_array_get_data(occupancy);
	u8* used_data = s_array_get_data(&used);

	for (i32 z = 0; z < nz; ++z) {
		for (i32 y = 0; y < ny; ++y) {
			for (i32 x = 0; x < nx; ++x) {
				const sz start_idx = se_sdf_physics_voxel_index(x, y, z, nx, ny);
				if (!occ_data[start_idx] || used_data[start_idx]) {
					continue;
				}

				i32 x2 = x;
				while (x2 + 1 < nx) {
					const sz idx = se_sdf_physics_voxel_index(x2 + 1, y, z, nx, ny);
					if (!occ_data[idx] || used_data[idx]) {
						break;
					}
					++x2;
				}

				i32 y2 = y;
				for (;;) {
					if (y2 + 1 >= ny) {
						break;
					}
					b8 can_expand_y = 1;
					for (i32 xx = x; xx <= x2; ++xx) {
						const sz idx = se_sdf_physics_voxel_index(xx, y2 + 1, z, nx, ny);
						if (!occ_data[idx] || used_data[idx]) {
							can_expand_y = 0;
							break;
						}
					}
					if (!can_expand_y) {
						break;
					}
					++y2;
				}

				i32 z2 = z;
				for (;;) {
					if (z2 + 1 >= nz) {
						break;
					}
					b8 can_expand_z = 1;
					for (i32 yy = y; yy <= y2 && can_expand_z; ++yy) {
						for (i32 xx = x; xx <= x2; ++xx) {
							const sz idx = se_sdf_physics_voxel_index(xx, yy, z2 + 1, nx, ny);
							if (!occ_data[idx] || used_data[idx]) {
								can_expand_z = 0;
								break;
							}
						}
					}
					if (!can_expand_z) {
						break;
					}
					++z2;
				}

				for (i32 zz = z; zz <= z2; ++zz) {
					for (i32 yy = y; yy <= y2; ++yy) {
						for (i32 xx = x; xx <= x2; ++xx) {
							const sz idx = se_sdf_physics_voxel_index(xx, yy, zz, nx, ny);
							used_data[idx] = 1u;
						}
					}
				}

				const s_vec3 min_v = s_vec3(
					bounds_min->x + (f32)x * cell_size->x,
					bounds_min->y + (f32)y * cell_size->y,
					bounds_min->z + (f32)z * cell_size->z
				);
				const s_vec3 max_v = s_vec3(
					bounds_min->x + (f32)(x2 + 1) * cell_size->x,
					bounds_min->y + (f32)(y2 + 1) * cell_size->y,
					bounds_min->z + (f32)(z2 + 1) * cell_size->z
				);
				se_sdf_physics_box box = {0};
				box.offset = s_vec3(
					(min_v.x + max_v.x) * 0.5f,
					(min_v.y + max_v.y) * 0.5f,
					(min_v.z + max_v.z) * 0.5f
				);
				box.half_extents = s_vec3(
					fmaxf((max_v.x - min_v.x) * 0.5f, 0.0001f),
					fmaxf((max_v.y - min_v.y) * 0.5f, 0.0001f),
					fmaxf((max_v.z - min_v.z) * 0.5f, 0.0001f)
				);
				box.volume = 8.0f * box.half_extents.x * box.half_extents.y * box.half_extents.z;
				s_array_add(out_boxes, box);
			}
		}
	}

	s_array_clear(&used);
}

static u32 se_sdf_physics_add_boxes_with_budget(
	se_physics_world_3d_handle world,
	se_physics_body_3d_handle body,
	se_sdf_physics_boxes* boxes,
	const u32 budget,
	const b8 is_trigger
) {
	if (!world || !body || !boxes || budget == 0 || s_array_get_size(boxes) == 0) {
		return 0;
	}

	const s_vec3 rotation = s_vec3(0.0f, 0.0f, 0.0f);
	u32 added = 0;
	const sz box_count = s_array_get_size(boxes);
	if (box_count <= (sz)budget) {
		for (sz i = 0; i < box_count; ++i) {
			se_sdf_physics_box* box = s_array_get(boxes, s_array_handle(boxes, (u32)i));
			if (!box) {
				continue;
			}
			if (se_physics_body_3d_add_box(world, body, &box->offset, &box->half_extents, &rotation, is_trigger) != SE_PHYSICS_SHAPE_3D_HANDLE_NULL) {
				++added;
			}
		}
		return added;
	}

	se_sdf_u8_array selected;
	s_array_init(&selected);
	se_sdf_physics_u8_array_set_count(&selected, box_count, 0u);
	u8* selected_data = s_array_get_data(&selected);

	for (u32 pick = 0; pick < budget; ++pick) {
		f32 best_volume = -1.0f;
		sz best_index = (sz)-1;
		for (sz i = 0; i < box_count; ++i) {
			if (selected_data[i] != 0u) {
				continue;
			}
			se_sdf_physics_box* box = s_array_get(boxes, s_array_handle(boxes, (u32)i));
			if (!box) {
				continue;
			}
			if (box->volume > best_volume) {
				best_volume = box->volume;
				best_index = i;
			}
		}
		if (best_index == (sz)-1) {
			break;
		}
		selected_data[best_index] = 1u;
		se_sdf_physics_box* box = s_array_get(boxes, s_array_handle(boxes, (u32)best_index));
		if (box &&
			se_physics_body_3d_add_box(world, body, &box->offset, &box->half_extents, &rotation, is_trigger) != SE_PHYSICS_SHAPE_3D_HANDLE_NULL) {
			++added;
		}
	}

	s_array_clear(&selected);
	return added;
}

se_physics_body_3d_handle se_sdf_object_create_physics_body_3d(
	const se_physics_world_3d_handle world,
	const se_sdf_object* object,
	const se_physics_body_params_3d* body_params,
	const s_vec3* reference_position,
	const b8 is_trigger
) {
	if (world == SE_PHYSICS_WORLD_3D_HANDLE_NULL || !object || !reference_position) {
		return SE_PHYSICS_BODY_3D_HANDLE_NULL;
	}

	se_sdf_object* root = (se_sdf_object*)object;
	se_physics_body_params_3d body_cfg = body_params
		? *body_params
		: SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	if (!body_params) {
		body_cfg.type = SE_PHYSICS_BODY_STATIC;
	}

	se_physics_body_3d_handle body = se_physics_body_3d_create(world, &body_cfg);
	if (body == SE_PHYSICS_BODY_3D_HANDLE_NULL) {
		return SE_PHYSICS_BODY_3D_HANDLE_NULL;
	}

	se_sdf_scene_bounds bounds = SE_SDF_SCENE_BOUNDS_DEFAULTS;
	const s_mat4 identity = s_mat4_identity;
	se_sdf_physics_collect_object_bounds_recursive(root, &identity, &bounds, 0, 128);

	if (!bounds.valid) {
		const s_vec3 origin_local = se_sdf_mul_mat4_point(&root->transform, &s_vec3(0.0f, 0.0f, 0.0f));
		const s_vec3 fallback_half = s_vec3(2.0f, 2.0f, 2.0f);
		bounds.min = s_vec3_sub(&origin_local, &fallback_half);
		bounds.max = s_vec3_add(&origin_local, &fallback_half);
		bounds.valid = 1;
	}
	se_sdf_physics_finalize_bounds(&bounds);

	if (bounds.has_unbounded_primitives) {
		s_vec3 half = s_vec3(
			(bounds.max.x - bounds.min.x) * 0.5f,
			(bounds.max.y - bounds.min.y) * 0.5f,
			(bounds.max.z - bounds.min.z) * 0.5f
		);
		half.x = fmaxf(half.x, 64.0f);
		half.y = fmaxf(half.y, 64.0f);
		half.z = fmaxf(half.z, 64.0f);
		bounds.min = s_vec3_sub(&bounds.center, &half);
		bounds.max = s_vec3_add(&bounds.center, &half);
		se_sdf_physics_finalize_bounds(&bounds);
	}

	s_vec3 span = s_vec3(
		bounds.max.x - bounds.min.x,
		bounds.max.y - bounds.min.y,
		bounds.max.z - bounds.min.z
	);
	if (span.x < 0.02f || span.y < 0.02f || span.z < 0.02f) {
		const s_vec3 min_half = s_vec3(
			fmaxf(span.x * 0.5f, 0.05f),
			fmaxf(span.y * 0.5f, 0.05f),
			fmaxf(span.z * 0.5f, 0.05f)
		);
		bounds.min = s_vec3_sub(&bounds.center, &min_half);
		bounds.max = s_vec3_add(&bounds.center, &min_half);
		span = s_vec3(
			bounds.max.x - bounds.min.x,
			bounds.max.y - bounds.min.y,
			bounds.max.z - bounds.min.z
		);
		se_sdf_physics_finalize_bounds(&bounds);
	}

	const s_vec3 reference_local = s_vec3_sub(reference_position, &body_cfg.position);
	const se_sdf_physics_detail_tier detail_tier = se_sdf_physics_pick_tier(&bounds, &reference_local);
	const u32 world_shape_budget = s_max((u32)1u, se_physics_world_3d_get_shape_limit(world));

	if (se_sdf_physics_try_add_simple_shape(world, body, root, is_trigger)) {
		return body;
	}

	const s_vec3 aabb_half = s_vec3(
		fmaxf(span.x * 0.5f, 0.05f),
		fmaxf(span.y * 0.5f, 0.05f),
		fmaxf(span.z * 0.5f, 0.05f)
	);

	if (detail_tier == SE_SDF_PHYSICS_DETAIL_FAR) {
		if (se_physics_body_3d_add_aabb(world, body, &bounds.center, &aabb_half, is_trigger) != SE_PHYSICS_SHAPE_3D_HANDLE_NULL) {
			return body;
		}
		se_physics_body_3d_destroy(world, body);
		return SE_PHYSICS_BODY_3D_HANDLE_NULL;
	}

	u32 shape_budget = world_shape_budget;
	i32 base_resolution = 22;
	if (detail_tier == SE_SDF_PHYSICS_DETAIL_MID) {
		shape_budget = s_max((u32)1u, world_shape_budget / 2u);
		base_resolution = 14;
	}
	if (shape_budget <= 2u) {
		base_resolution = 8;
	} else if (shape_budget <= 4u) {
		base_resolution = s_min(base_resolution, 10);
	} else if (shape_budget <= 8u) {
		base_resolution = s_min(base_resolution, 12);
	}

	const f32 max_span = fmaxf(span.x, fmaxf(span.y, span.z));
	const f32 safe_max_span = fmaxf(max_span, 0.0001f);
	const i32 nx = se_sdf_physics_clampi((i32)ceilf((f32)base_resolution * span.x / safe_max_span), 4, base_resolution);
	const i32 ny = se_sdf_physics_clampi((i32)ceilf((f32)base_resolution * span.y / safe_max_span), 4, base_resolution);
	const i32 nz = se_sdf_physics_clampi((i32)ceilf((f32)base_resolution * span.z / safe_max_span), 4, base_resolution);
	const s_vec3 cell_size = s_vec3(
		span.x / (f32)nx,
		span.y / (f32)ny,
		span.z / (f32)nz
	);
	const f32 min_cell = fmaxf(fminf(cell_size.x, fminf(cell_size.y, cell_size.z)), 0.0001f);
	const f32 inside_bias = min_cell * 0.15f;

	se_sdf_u8_array occupancy;
	se_sdf_physics_boxes boxes;
	s_array_init(&occupancy);
	s_array_init(&boxes);

	se_sdf_physics_build_voxel_occupancy(
		root,
		&bounds.min,
		&cell_size,
		nx,
		ny,
		nz,
		inside_bias,
		&occupancy
	);
	se_sdf_physics_greedy_merge_voxels(
		&bounds.min,
		&cell_size,
		nx,
		ny,
		nz,
		&occupancy,
		&boxes
	);

	const u32 added = se_sdf_physics_add_boxes_with_budget(world, body, &boxes, shape_budget, is_trigger);
	s_array_clear(&boxes);
	s_array_clear(&occupancy);

	if (added > 0) {
		return body;
	}

	if (se_physics_body_3d_add_aabb(world, body, &bounds.center, &aabb_half, is_trigger) != SE_PHYSICS_SHAPE_3D_HANDLE_NULL) {
		return body;
	}

	se_physics_body_3d_destroy(world, body);
	return SE_PHYSICS_BODY_3D_HANDLE_NULL;
}

static void se_sdf_free_legacy_object(se_sdf_object* obj) {
	if (!obj) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&obj->children); ++i) {
		se_sdf_object* child = s_array_get(&obj->children, s_array_handle(&obj->children, (u32)i));
		if (child) {
			se_sdf_free_legacy_object(child);
		}
	}
	s_array_clear(&obj->children);
}

static b8 se_sdf_copy_primitive_to_legacy_object(
	const se_sdf_primitive_desc* primitive,
	se_sdf_object* out
) {
	if (!primitive || !out) {
		return 0;
	}

	switch (primitive->type) {
		case SE_SDF_PRIMITIVE_SPHERE: out->sphere.radius = primitive->sphere.radius; break;
		case SE_SDF_PRIMITIVE_BOX: out->box.size = primitive->box.size; break;
		case SE_SDF_PRIMITIVE_ROUND_BOX:
			out->round_box.size = primitive->round_box.size;
			out->round_box.roundness = primitive->round_box.roundness;
			break;
		case SE_SDF_PRIMITIVE_BOX_FRAME:
			out->box_frame.size = primitive->box_frame.size;
			out->box_frame.thickness = primitive->box_frame.thickness;
			break;
		case SE_SDF_PRIMITIVE_TORUS: out->torus.radii = primitive->torus.radii; break;
		case SE_SDF_PRIMITIVE_CAPPED_TORUS:
			out->capped_torus.axis_sin_cos = primitive->capped_torus.axis_sin_cos;
			out->capped_torus.major_radius = primitive->capped_torus.major_radius;
			out->capped_torus.minor_radius = primitive->capped_torus.minor_radius;
			break;
		case SE_SDF_PRIMITIVE_LINK:
			out->link.half_length = primitive->link.half_length;
			out->link.outer_radius = primitive->link.outer_radius;
			out->link.inner_radius = primitive->link.inner_radius;
			break;
		case SE_SDF_PRIMITIVE_CYLINDER: out->cylinder.axis_and_radius = primitive->cylinder.axis_and_radius; break;
		case SE_SDF_PRIMITIVE_CONE:
			out->cone.angle_sin_cos = primitive->cone.angle_sin_cos;
			out->cone.height = primitive->cone.height;
			break;
		case SE_SDF_PRIMITIVE_CONE_INFINITE: out->cone_infinite.angle_sin_cos = primitive->cone_infinite.angle_sin_cos; break;
		case SE_SDF_PRIMITIVE_PLANE:
			out->plane.normal = primitive->plane.normal;
			out->plane.offset = primitive->plane.offset;
			break;
		case SE_SDF_PRIMITIVE_HEX_PRISM: out->hex_prism.half_size = primitive->hex_prism.half_size; break;
		case SE_SDF_PRIMITIVE_CAPSULE:
			out->capsule.a = primitive->capsule.a;
			out->capsule.b = primitive->capsule.b;
			out->capsule.radius = primitive->capsule.radius;
			break;
		case SE_SDF_PRIMITIVE_VERTICAL_CAPSULE:
			out->vertical_capsule.height = primitive->vertical_capsule.height;
			out->vertical_capsule.radius = primitive->vertical_capsule.radius;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER:
			out->capped_cylinder.radius = primitive->capped_cylinder.radius;
			out->capped_cylinder.half_height = primitive->capped_cylinder.half_height;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY:
			out->capped_cylinder_arbitrary.a = primitive->capped_cylinder_arbitrary.a;
			out->capped_cylinder_arbitrary.b = primitive->capped_cylinder_arbitrary.b;
			out->capped_cylinder_arbitrary.radius = primitive->capped_cylinder_arbitrary.radius;
			break;
		case SE_SDF_PRIMITIVE_ROUNDED_CYLINDER:
			out->rounded_cylinder.outer_radius = primitive->rounded_cylinder.outer_radius;
			out->rounded_cylinder.corner_radius = primitive->rounded_cylinder.corner_radius;
			out->rounded_cylinder.half_height = primitive->rounded_cylinder.half_height;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CONE:
			out->capped_cone.height = primitive->capped_cone.height;
			out->capped_cone.radius_base = primitive->capped_cone.radius_base;
			out->capped_cone.radius_top = primitive->capped_cone.radius_top;
			break;
		case SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY:
			out->capped_cone_arbitrary.a = primitive->capped_cone_arbitrary.a;
			out->capped_cone_arbitrary.b = primitive->capped_cone_arbitrary.b;
			out->capped_cone_arbitrary.radius_a = primitive->capped_cone_arbitrary.radius_a;
			out->capped_cone_arbitrary.radius_b = primitive->capped_cone_arbitrary.radius_b;
			break;
		case SE_SDF_PRIMITIVE_SOLID_ANGLE:
			out->solid_angle.angle_sin_cos = primitive->solid_angle.angle_sin_cos;
			out->solid_angle.radius = primitive->solid_angle.radius;
			break;
		case SE_SDF_PRIMITIVE_CUT_SPHERE:
			out->cut_sphere.radius = primitive->cut_sphere.radius;
			out->cut_sphere.cut_height = primitive->cut_sphere.cut_height;
			break;
		case SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE:
			out->cut_hollow_sphere.radius = primitive->cut_hollow_sphere.radius;
			out->cut_hollow_sphere.cut_height = primitive->cut_hollow_sphere.cut_height;
			out->cut_hollow_sphere.thickness = primitive->cut_hollow_sphere.thickness;
			break;
		case SE_SDF_PRIMITIVE_DEATH_STAR:
			out->death_star.radius_a = primitive->death_star.radius_a;
			out->death_star.radius_b = primitive->death_star.radius_b;
			out->death_star.separation = primitive->death_star.separation;
			break;
		case SE_SDF_PRIMITIVE_ROUND_CONE:
			out->round_cone.radius_base = primitive->round_cone.radius_base;
			out->round_cone.radius_top = primitive->round_cone.radius_top;
			out->round_cone.height = primitive->round_cone.height;
			break;
		case SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY:
			out->round_cone_arbitrary.a = primitive->round_cone_arbitrary.a;
			out->round_cone_arbitrary.b = primitive->round_cone_arbitrary.b;
			out->round_cone_arbitrary.radius_a = primitive->round_cone_arbitrary.radius_a;
			out->round_cone_arbitrary.radius_b = primitive->round_cone_arbitrary.radius_b;
			break;
		case SE_SDF_PRIMITIVE_VESICA_SEGMENT:
			out->vesica_segment.a = primitive->vesica_segment.a;
			out->vesica_segment.b = primitive->vesica_segment.b;
			out->vesica_segment.width = primitive->vesica_segment.width;
			break;
		case SE_SDF_PRIMITIVE_RHOMBUS:
			out->rhombus.length_a = primitive->rhombus.length_a;
			out->rhombus.length_b = primitive->rhombus.length_b;
			out->rhombus.height = primitive->rhombus.height;
			out->rhombus.corner_radius = primitive->rhombus.corner_radius;
			break;
		case SE_SDF_PRIMITIVE_OCTAHEDRON: out->octahedron.size = primitive->octahedron.size; break;
		case SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND: out->octahedron_bound.size = primitive->octahedron_bound.size; break;
		case SE_SDF_PRIMITIVE_PYRAMID: out->pyramid.height = primitive->pyramid.height; break;
		case SE_SDF_PRIMITIVE_TRIANGLE:
			out->triangle.a = primitive->triangle.a;
			out->triangle.b = primitive->triangle.b;
			out->triangle.c = primitive->triangle.c;
			break;
		case SE_SDF_PRIMITIVE_QUAD:
			out->quad.a = primitive->quad.a;
			out->quad.b = primitive->quad.b;
			out->quad.c = primitive->quad.c;
			out->quad.d = primitive->quad.d;
			break;
		default:
			return 0;
	}
	return 1;
}

static b8 se_sdf_build_legacy_object_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node_handle,
	se_sdf_object* out
) {
	if (!scene_ptr || !out || node_handle == SE_SDF_NODE_NULL) {
		return 0;
	}
	se_sdf_node* runtime_node = se_sdf_node_from_handle(scene_ptr, scene, node_handle);
	if (!runtime_node) {
		return 0;
	}

	memset(out, 0, sizeof(*out));
	s_array_init(&out->children);
	out->transform = runtime_node->transform;
	out->source_scene = scene;
	out->source_node = node_handle;
	out->operation = runtime_node->operation;
	out->operation_amount = runtime_node->operation_amount;
	out->noise = (se_sdf_noise){0};

	if (runtime_node->is_group) {
		out->type = SE_SDF_NONE;
	} else {
		out->type = (se_sdf_object_type)runtime_node->primitive.type;
		if (!se_sdf_copy_primitive_to_legacy_object(&runtime_node->primitive, out)) {
			se_sdf_free_legacy_object(out);
			return 0;
		}
	}

	for (sz i = 0; i < s_array_get_size(&runtime_node->children); ++i) {
		se_sdf_node_handle* child_handle = s_array_get(&runtime_node->children, s_array_handle(&runtime_node->children, (u32)i));
		if (!child_handle) {
			continue;
		}
		se_sdf_object child = {0};
		if (!se_sdf_build_legacy_object_recursive(scene_ptr, scene, *child_handle, &child)) {
			se_sdf_free_legacy_object(out);
			return 0;
		}
		s_array_add(&out->children, child);
	}

	return 1;
}

se_sdf_renderer_handle se_sdf_renderer_create(const se_sdf_renderer_desc* desc) {
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return SE_SDF_RENDERER_NULL;
	}
	se_sdf_renderer_desc renderer_desc = SE_SDF_RENDERER_DESC_DEFAULTS;
	if (desc) {
		renderer_desc = *desc;
		if (renderer_desc.shader_source_capacity == 0) {
			renderer_desc.shader_source_capacity = SE_SDF_RENDERER_DESC_DEFAULTS.shader_source_capacity;
		}
	}

	se_sdf_renderer_handle handle = s_array_increment(&ctx->sdf_renderers);
	se_sdf_renderer* renderer = s_array_get(&ctx->sdf_renderers, handle);
	if (!renderer) {
		return SE_SDF_RENDERER_NULL;
	}
	memset(renderer, 0, sizeof(*renderer));
	renderer->desc = renderer_desc;
	renderer->quality = SE_SDF_RAYMARCH_QUALITY_DEFAULTS;
	renderer->debug = SE_SDF_RENDERER_DEBUG_DEFAULTS;
	renderer->scene = SE_SDF_SCENE_NULL;
	renderer->built = 0;
	s_array_init(&renderer->controls);
	s_array_init(&renderer->node_bindings);
	s_array_init(&renderer->primitive_bindings);
	renderer->generated_fragment_source.data = NULL;
	renderer->generated_fragment_source.capacity = 0;
	renderer->generated_fragment_source.size = 0;
	renderer->generated_fragment_source.oom = 0;
	renderer->last_uniform_write_count = 0;
	renderer->total_uniform_write_count = 0;
	renderer->shader = S_HANDLE_NULL;
	memset(&renderer->quad, 0, sizeof(renderer->quad));
	renderer->quad_ready = 0;
	renderer->material = SE_SDF_MATERIAL_DESC_DEFAULTS;
	renderer->stylized = SE_SDF_STYLIZED_DESC_DEFAULTS;
	renderer->lighting_direction = s_vec3(0.58f, 0.76f, 0.28f);
	renderer->lighting_color = s_vec3(1.0f, 1.0f, 1.0f);
	renderer->fog_color = s_vec3(0.11f, 0.15f, 0.21f);
	renderer->fog_density = 0.0012f;
	renderer->material_base_color_control = SE_SDF_CONTROL_NULL;
	renderer->lighting_direction_control = SE_SDF_CONTROL_NULL;
	renderer->lighting_color_control = SE_SDF_CONTROL_NULL;
	renderer->fog_color_control = SE_SDF_CONTROL_NULL;
	renderer->fog_density_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_band_levels_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_rim_power_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_rim_strength_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_fill_strength_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_back_strength_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_checker_scale_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_checker_strength_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_ground_blend_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_desaturate_control = SE_SDF_CONTROL_NULL;
	renderer->stylized_gamma_control = SE_SDF_CONTROL_NULL;
	renderer->render_generation = se_render_get_generation();
	se_sdf_set_diagnostics(renderer, 1, "init", "renderer created");
	return handle;
}

void se_sdf_renderer_destroy(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return;
	}
	se_sdf_renderer_invalidate_gpu_state(renderer_ptr);
	s_array_clear(&renderer_ptr->controls);
	s_array_clear(&renderer_ptr->node_bindings);
	s_array_clear(&renderer_ptr->primitive_bindings);
	se_sdf_string_free(&renderer_ptr->generated_fragment_source);
	s_array_remove(&ctx->sdf_renderers, renderer);
}

void se_sdf_shutdown(void) {
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return;
	}

	while (s_array_get_size(&ctx->sdf_renderers) > 0) {
		se_sdf_renderer_handle renderer_handle = s_array_handle(
			&ctx->sdf_renderers,
			(u32)(s_array_get_size(&ctx->sdf_renderers) - 1)
		);
		se_sdf_renderer_destroy(renderer_handle);
	}

	while (s_array_get_size(&ctx->sdf_scenes) > 0) {
		se_sdf_scene_handle scene_handle = s_array_handle(
			&ctx->sdf_scenes,
			(u32)(s_array_get_size(&ctx->sdf_scenes) - 1)
		);
		se_sdf_scene_destroy(scene_handle);
	}

	s_array_clear(&ctx->sdf_renderers);
	s_array_clear(&ctx->sdf_scenes);
}

b8 se_sdf_renderer_set_scene(const se_sdf_renderer_handle renderer, const se_sdf_scene_handle scene) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	if (scene != SE_SDF_SCENE_NULL && !se_sdf_scene_from_handle(scene)) {
		return 0;
	}
	renderer_ptr->scene = scene;
	renderer_ptr->built = 0;
	return 1;
}

b8 se_sdf_renderer_set_quality(const se_sdf_renderer_handle renderer, const se_sdf_raymarch_quality* quality) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !quality) {
		return 0;
	}
	renderer_ptr->quality = *quality;
	if (renderer_ptr->quality.max_steps < 1) {
		renderer_ptr->quality.max_steps = 1;
	}
	if (renderer_ptr->quality.max_steps > 512) {
		renderer_ptr->quality.max_steps = 512;
	}
	if (renderer_ptr->quality.hit_epsilon <= 0.0f) {
		renderer_ptr->quality.hit_epsilon = SE_SDF_RAYMARCH_QUALITY_DEFAULTS.hit_epsilon;
	}
	if (renderer_ptr->quality.max_distance <= 0.0f) {
		renderer_ptr->quality.max_distance = SE_SDF_RAYMARCH_QUALITY_DEFAULTS.max_distance;
	}
	if (renderer_ptr->quality.shadow_steps < 1) {
		renderer_ptr->quality.shadow_steps = 1;
	}
	if (renderer_ptr->quality.shadow_strength < 0.0f) {
		renderer_ptr->quality.shadow_strength = 0.0f;
	}
	if (renderer_ptr->quality.shadow_strength > 1.0f) {
		renderer_ptr->quality.shadow_strength = 1.0f;
	}
	return 1;
}

b8 se_sdf_renderer_set_debug(const se_sdf_renderer_handle renderer, const se_sdf_renderer_debug* debug) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !debug) {
		return 0;
	}
	renderer_ptr->debug = *debug;
	return 1;
}

b8 se_sdf_renderer_set_material(const se_sdf_renderer_handle renderer, const se_sdf_material_desc* material) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !material) {
		return 0;
	}
	renderer_ptr->material = *material;
	return 1;
}

b8 se_sdf_renderer_set_stylized(const se_sdf_renderer_handle renderer, const se_sdf_stylized_desc* stylized) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !stylized) {
		return 0;
	}
	renderer_ptr->stylized = *stylized;
	return 1;
}

se_sdf_stylized_desc se_sdf_renderer_get_stylized(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return SE_SDF_STYLIZED_DESC_DEFAULTS;
	}
	return renderer_ptr->stylized;
}

b8 se_sdf_renderer_set_base_color(const se_sdf_renderer_handle renderer, const s_vec3* base_color) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !base_color) {
		return 0;
	}
	renderer_ptr->material.base_color = *base_color;
	return 1;
}

b8 se_sdf_renderer_set_light_rig(
	const se_sdf_renderer_handle renderer,
	const s_vec3* light_direction,
	const s_vec3* light_color,
	const s_vec3* fog_color,
	const f32 fog_density
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !light_direction || !light_color || !fog_color) {
		return 0;
	}
	renderer_ptr->lighting_direction = *light_direction;
	renderer_ptr->lighting_color = *light_color;
	renderer_ptr->fog_color = *fog_color;
	renderer_ptr->fog_density = fog_density;
	return 1;
}

b8 se_sdf_renderer_build(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	se_sdf_renderer_refresh_context_state(renderer_ptr);

	se_sdf_string_free(&renderer_ptr->generated_fragment_source);
	se_sdf_string_init(&renderer_ptr->generated_fragment_source, renderer_ptr->desc.shader_source_capacity);
	if (!renderer_ptr->generated_fragment_source.data) {
		se_sdf_set_diagnostics(
			renderer_ptr,
			0,
			"codegen_alloc",
			"failed to allocate shader buffer (capacity=%zu)",
			renderer_ptr->desc.shader_source_capacity
		);
		return 0;
	}

	char map_name[32] = "map";
	if (!se_sdf_codegen_emit_fragment_prelude(&renderer_ptr->generated_fragment_source) ||
		!se_sdf_codegen_emit_uniform_block(&renderer_ptr->generated_fragment_source) ||
		!se_sdf_emit_control_uniform_declarations(renderer_ptr, &renderer_ptr->generated_fragment_source)) {
		se_sdf_set_diagnostics(
			renderer_ptr,
			0,
			"codegen_emit",
			"failed to emit prelude/uniform block/control uniforms"
		);
		return 0;
	}

	b8 emitted_map = 0;
	if (renderer_ptr->scene != SE_SDF_SCENE_NULL) {
		se_sdf_scene* runtime_scene = se_sdf_scene_from_handle(renderer_ptr->scene);
		if (runtime_scene && runtime_scene->root != SE_SDF_NODE_NULL) {
			char validation_error[256] = {0};
			if (!se_sdf_scene_validate(renderer_ptr->scene, validation_error, sizeof(validation_error))) {
				se_sdf_set_diagnostics(
					renderer_ptr,
					0,
					"scene_validate",
					"scene validation failed: %s",
					validation_error[0] != '\0' ? validation_error : "unknown error"
				);
				return 0;
			}
			se_sdf_object legacy_root = {0};
			if (!se_sdf_build_legacy_object_recursive(
				runtime_scene,
				renderer_ptr->scene,
				runtime_scene->root,
				&legacy_root
			)) {
				se_sdf_set_diagnostics(
					renderer_ptr,
					0,
					"scene_codegen",
					"failed to build runtime scene graph for map '%s'",
					map_name
				);
				return 0;
			}
			se_sdf_codegen_active_renderer = renderer_ptr;
			se_sdf_generate_distance_function(&renderer_ptr->generated_fragment_source, &legacy_root, map_name);
			se_sdf_codegen_active_renderer = NULL;
			se_sdf_free_legacy_object(&legacy_root);
			emitted_map = 1;
		}
	}

	if (!emitted_map) {
		if (!se_sdf_codegen_emit_map_stub(&renderer_ptr->generated_fragment_source, map_name)) {
			se_sdf_set_diagnostics(
				renderer_ptr,
				0,
				"codegen_emit",
				"failed to emit map stub for '%s'",
				map_name
			);
			return 0;
		}
	}

	if (!se_sdf_codegen_emit_shading_stub(&renderer_ptr->generated_fragment_source) ||
		!se_sdf_codegen_emit_fragment_main(&renderer_ptr->generated_fragment_source, map_name)) {
		sz preview = renderer_ptr->generated_fragment_source.size;
		if (preview > 120) {
			preview = 120;
		}
		se_sdf_set_diagnostics(
			renderer_ptr,
			0,
			"codegen_emit",
			"failed to emit shading/main for map '%s' (preview='%.*s')",
			map_name,
			(int)preview,
			renderer_ptr->generated_fragment_source.data ? renderer_ptr->generated_fragment_source.data : ""
		);
		return 0;
	}

	se_sdf_renderer_release_shader(renderer_ptr);
	renderer_ptr->shader = se_shader_load_from_memory(
		se_sdf_fullscreen_vertex_shader,
		renderer_ptr->generated_fragment_source.data
	);
	if (renderer_ptr->shader == S_HANDLE_NULL) {
		se_sdf_set_diagnostics(
			renderer_ptr,
			0,
			"shader_compile",
			"failed to compile generated fragment source"
		);
		return 0;
	}
	if (!renderer_ptr->quad_ready) {
		se_quad_3d_create(&renderer_ptr->quad);
		renderer_ptr->quad_ready = 1;
	}

	for (sz i = 0; i < s_array_get_size(&renderer_ptr->controls); ++i) {
		se_sdf_control* control = s_array_get(&renderer_ptr->controls, s_array_handle(&renderer_ptr->controls, (u32)i));
		if (!control) {
			continue;
		}
		control->cached_uniform_location = se_shader_get_uniform_location(renderer_ptr->shader, control->uniform_name);
		control->has_last_uploaded_value = 0;
		control->dirty = 1;
	}

	renderer_ptr->built = 1;
	renderer_ptr->render_generation = se_render_get_generation();
	renderer_ptr->last_uniform_write_count = 0;
	se_sdf_set_diagnostics(
		renderer_ptr,
		1,
		"build_complete",
		"codegen ok (bytes=%zu, controls=%zu)",
		renderer_ptr->generated_fragment_source.size,
		s_array_get_size(&renderer_ptr->controls)
	);
	return 1;
}

b8 se_sdf_renderer_rebuild_if_dirty(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	if (!renderer_ptr->built) {
		return se_sdf_renderer_build(renderer);
	}
	return 1;
}

b8 se_sdf_frame_set_camera(se_sdf_frame_desc* frame, const se_camera_handle camera) {
	if (!frame || camera == S_HANDLE_NULL) {
		return 0;
	}
	if (!se_camera_get(camera)) {
		return 0;
	}
	frame->camera = camera;
	return 1;
}

b8 se_sdf_frame_set_scene_depth_texture(se_sdf_frame_desc* frame, const u32 depth_texture) {
	if (!frame || depth_texture == 0u) {
		return 0;
	}
	frame->has_scene_depth_texture = 1;
	frame->scene_depth_texture = depth_texture;
	return 1;
}

b8 se_sdf_renderer_render(const se_sdf_renderer_handle renderer, const se_sdf_frame_desc* frame) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !frame) {
		return 0;
	}
	if (!se_render_has_context()) {
		se_sdf_set_diagnostics(
			renderer_ptr,
			0,
			"render_context",
			"render skipped: no active render context"
		);
		return 0;
	}
	se_sdf_renderer_refresh_context_state(renderer_ptr);
	if (!renderer_ptr->built) {
		if (!se_sdf_renderer_build(renderer)) {
			return 0;
		}
	}
	se_sdf_sync_control_bindings(renderer_ptr);
	(void)se_sdf_apply_node_bindings(renderer_ptr);
	(void)se_sdf_apply_primitive_bindings(renderer_ptr);
	se_sdf_apply_renderer_shading_bindings(renderer_ptr);
	if (renderer_ptr->shader == S_HANDLE_NULL) {
		return 0;
	}

	s_vec2 resolution = frame->resolution;
	if (resolution.x <= 0.0f) resolution.x = 1.0f;
	if (resolution.y <= 0.0f) resolution.y = 1.0f;
	f32 time_value = renderer_ptr->debug.freeze_time ? 0.0f : frame->time_seconds;
	if (frame->camera == S_HANDLE_NULL) {
		se_sdf_set_diagnostics(
			renderer_ptr,
			0,
			"frame_camera",
			"render skipped: frame camera is null"
		);
		return 0;
	}
	se_camera* camera_ptr = se_camera_get(frame->camera);
	if (!camera_ptr) {
		se_sdf_set_diagnostics(
			renderer_ptr,
			0,
			"frame_camera",
			"render skipped: frame camera handle is invalid"
		);
		return 0;
	}
	const s_mat4 camera_view = se_camera_get_view_matrix(frame->camera);
	const s_mat4 camera_projection = se_camera_get_projection_matrix(frame->camera);
	const s_mat4 camera_view_projection = s_mat4_mul(&camera_projection, &camera_view);
	const s_mat4 camera_inv_view_projection = s_mat4_inverse(&camera_view_projection);
	const s_vec3 camera_position = camera_ptr->position;
	const s_vec3 camera_forward = se_camera_get_forward_vector(frame->camera);
	const s_vec3 camera_right = se_camera_get_right_vector(frame->camera);
	const s_vec3 camera_up = se_camera_get_up_vector(frame->camera);
	const s_vec2 camera_near_far = s_vec2(camera_ptr->near, camera_ptr->far);
	const f32 camera_aspect = camera_ptr->aspect > 0.0001f
		? camera_ptr->aspect
		: (resolution.x / s_max(resolution.y, 0.0001f));
	const f32 camera_tan_half_fov = tanf((camera_ptr->fov * 0.5f) * (3.14159265358979323846f / 180.0f));
	const f32 camera_ortho_height = camera_ptr->ortho_height > 0.0001f ? camera_ptr->ortho_height : 1.0f;
	const i32 camera_is_orthographic = camera_ptr->use_orthographic ? 1 : 0;

	se_shader_set_vec2(renderer_ptr->shader, "u_resolution", &resolution);
	se_shader_set_float(renderer_ptr->shader, "u_time", time_value);
	se_shader_set_vec2(renderer_ptr->shader, "u_mouse", &frame->mouse_normalized);
	se_shader_set_mat4(renderer_ptr->shader, "u_camera_view", &camera_view);
	se_shader_set_mat4(renderer_ptr->shader, "u_camera_projection", &camera_projection);
	se_shader_set_mat4(renderer_ptr->shader, "u_camera_inv_view_projection", &camera_inv_view_projection);
	se_shader_set_vec3(renderer_ptr->shader, "u_camera_position", &camera_position);
	se_shader_set_vec3(renderer_ptr->shader, "u_camera_forward", &camera_forward);
	se_shader_set_vec3(renderer_ptr->shader, "u_camera_right", &camera_right);
	se_shader_set_vec3(renderer_ptr->shader, "u_camera_up", &camera_up);
	se_shader_set_vec2(renderer_ptr->shader, "u_camera_near_far", &camera_near_far);
	se_shader_set_float(renderer_ptr->shader, "u_camera_tan_half_fov", camera_tan_half_fov);
	se_shader_set_float(renderer_ptr->shader, "u_camera_aspect", camera_aspect);
	se_shader_set_float(renderer_ptr->shader, "u_camera_ortho_height", camera_ortho_height);
	se_shader_set_int(renderer_ptr->shader, "u_camera_is_orthographic", camera_is_orthographic);
	se_shader_set_int(renderer_ptr->shader, "u_has_scene_depth", frame->has_scene_depth_texture ? 1 : 0);
	if (frame->has_scene_depth_texture && frame->scene_depth_texture != 0u) {
		se_shader_set_texture(renderer_ptr->shader, "u_scene_depth", frame->scene_depth_texture);
	}
	se_shader_set_int(renderer_ptr->shader, "u_quality_max_steps", renderer_ptr->quality.max_steps);
	se_shader_set_float(renderer_ptr->shader, "u_quality_hit_epsilon", renderer_ptr->quality.hit_epsilon);
	se_shader_set_float(renderer_ptr->shader, "u_quality_max_distance", renderer_ptr->quality.max_distance);
	se_shader_set_int(renderer_ptr->shader, "u_debug_show_normals", renderer_ptr->debug.show_normals ? 1 : 0);
	se_shader_set_int(renderer_ptr->shader, "u_debug_show_distance", renderer_ptr->debug.show_distance ? 1 : 0);
	se_shader_set_int(renderer_ptr->shader, "u_debug_show_steps", renderer_ptr->debug.show_steps ? 1 : 0);
	se_shader_set_vec3(renderer_ptr->shader, "u_material_base_color", &renderer_ptr->material.base_color);
	se_shader_set_vec3(renderer_ptr->shader, "u_light_direction", &renderer_ptr->lighting_direction);
	se_shader_set_vec3(renderer_ptr->shader, "u_light_color", &renderer_ptr->lighting_color);
	se_shader_set_vec3(renderer_ptr->shader, "u_fog_color", &renderer_ptr->fog_color);
	se_shader_set_float(renderer_ptr->shader, "u_fog_density", renderer_ptr->fog_density);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_band_levels", renderer_ptr->stylized.band_levels);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_rim_power", renderer_ptr->stylized.rim_power);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_rim_strength", renderer_ptr->stylized.rim_strength);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_fill_strength", renderer_ptr->stylized.fill_strength);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_back_strength", renderer_ptr->stylized.back_strength);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_checker_scale", renderer_ptr->stylized.checker_scale);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_checker_strength", renderer_ptr->stylized.checker_strength);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_ground_blend", renderer_ptr->stylized.ground_blend);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_desaturate", renderer_ptr->stylized.desaturate);
	se_shader_set_float(renderer_ptr->shader, "u_stylized_gamma", renderer_ptr->stylized.gamma);

	renderer_ptr->last_uniform_write_count = 0;
	for (sz i = 0; i < s_array_get_size(&renderer_ptr->controls); ++i) {
		se_sdf_control* control = s_array_get(&renderer_ptr->controls, s_array_handle(&renderer_ptr->controls, (u32)i));
		if (!control || control->cached_uniform_location < 0 || !control->dirty) {
			continue;
		}
		if (control->has_last_uploaded_value &&
			se_sdf_control_value_equals(control->type, &control->value, &control->last_uploaded_value)) {
			control->dirty = 0;
			continue;
		}
		control->last_uploaded_value = control->value;
		control->has_last_uploaded_value = 1;
		control->dirty = 0;
		renderer_ptr->last_uniform_write_count++;
		switch (control->type) {
			case SE_SDF_CONTROL_FLOAT:
				se_shader_set_float(renderer_ptr->shader, control->uniform_name, control->value.f);
				break;
			case SE_SDF_CONTROL_VEC2:
				se_shader_set_vec2(renderer_ptr->shader, control->uniform_name, &control->value.vec2);
				break;
			case SE_SDF_CONTROL_VEC3:
				se_shader_set_vec3(renderer_ptr->shader, control->uniform_name, &control->value.vec3);
				break;
			case SE_SDF_CONTROL_VEC4:
				se_shader_set_vec4(renderer_ptr->shader, control->uniform_name, &control->value.vec4);
				break;
			case SE_SDF_CONTROL_INT:
				se_shader_set_int(renderer_ptr->shader, control->uniform_name, control->value.i);
				break;
			case SE_SDF_CONTROL_MAT4:
				se_shader_set_mat4(renderer_ptr->shader, control->uniform_name, &control->value.mat4);
				break;
			default:
				break;
		}
	}
	renderer_ptr->total_uniform_write_count += renderer_ptr->last_uniform_write_count;

	const GLboolean was_cull_enabled = glIsEnabled(GL_CULL_FACE);
	const GLboolean was_depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
	const GLboolean was_blend_enabled = glIsEnabled(GL_BLEND);
	GLboolean previous_depth_write_mask = GL_TRUE;
	GLint previous_depth_func = GL_LESS;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &previous_depth_write_mask);
	glGetIntegerv(GL_DEPTH_FUNC, &previous_depth_func);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);

	se_shader_use(renderer_ptr->shader, true, false);
	if (renderer_ptr->quad_ready) {
		se_quad_render(&renderer_ptr->quad, 0);
	}

	if (was_cull_enabled) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if (was_depth_test_enabled) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
	if (was_blend_enabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
	glDepthMask(previous_depth_write_mask);
	glDepthFunc((GLenum)previous_depth_func);

	return 1;
}

const char* se_sdf_renderer_get_shader_source(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return NULL;
	}
	return renderer_ptr->generated_fragment_source.data;
}

sz se_sdf_renderer_get_shader_source_size(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return renderer_ptr->generated_fragment_source.size;
}

b8 se_sdf_renderer_dump_shader_source(const se_sdf_renderer_handle renderer, const char* path) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !path || path[0] == '\0') {
		return 0;
	}
	if (!renderer_ptr->generated_fragment_source.data || renderer_ptr->generated_fragment_source.size == 0) {
		return 0;
	}

	FILE* file = fopen(path, "wb");
	if (!file) {
		return 0;
	}
	sz written = fwrite(renderer_ptr->generated_fragment_source.data, 1, renderer_ptr->generated_fragment_source.size, file);
	fclose(file);
	return written == renderer_ptr->generated_fragment_source.size;
}

sz se_sdf_renderer_get_uniform_writes(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return renderer_ptr->last_uniform_write_count;
}

sz se_sdf_renderer_get_total_uniform_writes(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return renderer_ptr->total_uniform_write_count;
}

const char* se_sdf_control_get_uniform_name(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || control == SE_SDF_CONTROL_NULL) {
		return NULL;
	}
	se_sdf_control* control_ptr = s_array_get(&renderer_ptr->controls, control);
	if (!control_ptr) {
		return NULL;
	}
	return control_ptr->uniform_name;
}

se_sdf_build_diagnostics se_sdf_renderer_get_build_diagnostics(const se_sdf_renderer_handle renderer) {
	se_sdf_build_diagnostics diagnostics = {0};
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return diagnostics;
	}
	return renderer_ptr->diagnostics;
}

static se_sdf_control_handle se_sdf_find_control_handle(
	se_sdf_renderer* renderer,
	const char* name
) {
	if (!renderer || !name || name[0] == '\0') {
		return SE_SDF_CONTROL_NULL;
	}
	for (sz i = 0; i < s_array_get_size(&renderer->controls); ++i) {
		s_handle control_handle = s_array_handle(&renderer->controls, (u32)i);
		se_sdf_control* control = s_array_get(&renderer->controls, control_handle);
		if (control && strcmp(control->name, name) == 0) {
			return control_handle;
		}
	}
	return SE_SDF_CONTROL_NULL;
}

static void se_sdf_make_control_uniform_name(
	const s_handle control_handle,
	const char* name,
	char* out_name,
	const sz out_name_size
) {
	if (!out_name || out_name_size == 0) {
		return;
	}
	const char* source = name ? name : "control";
	sz write_index = 0;
	int prefix_written = snprintf(out_name, out_name_size, "u_ctrl_%u_", s_handle_slot(control_handle));
	if (prefix_written < 0) {
		out_name[0] = '\0';
		return;
	}
	write_index = (sz)prefix_written;
	if (write_index >= out_name_size) {
		out_name[out_name_size - 1] = '\0';
		return;
	}

	for (sz i = 0; source[i] != '\0' && write_index + 1 < out_name_size; ++i) {
		char c = source[i];
		if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9')) {
			out_name[write_index++] = c;
		} else {
			out_name[write_index++] = '_';
		}
	}
	out_name[write_index] = '\0';
}

static b8 se_sdf_control_value_equals(
	const se_sdf_control_type type,
	const se_sdf_control_value* a,
	const se_sdf_control_value* b
) {
	if (!a || !b) {
		return 0;
	}
	switch (type) {
		case SE_SDF_CONTROL_FLOAT:
			return a->f == b->f;
		case SE_SDF_CONTROL_VEC2:
			return a->vec2.x == b->vec2.x && a->vec2.y == b->vec2.y;
		case SE_SDF_CONTROL_VEC3:
			return a->vec3.x == b->vec3.x && a->vec3.y == b->vec3.y && a->vec3.z == b->vec3.z;
		case SE_SDF_CONTROL_VEC4:
			return a->vec4.x == b->vec4.x && a->vec4.y == b->vec4.y && a->vec4.z == b->vec4.z && a->vec4.w == b->vec4.w;
		case SE_SDF_CONTROL_INT:
			return a->i == b->i;
		case SE_SDF_CONTROL_MAT4:
			return memcmp(&a->mat4, &b->mat4, sizeof(s_mat4)) == 0;
		default:
			return 0;
	}
}

static b8 se_sdf_control_assign_value(
	se_sdf_control* control,
	const se_sdf_control_value* value
) {
	if (!control || !value) {
		return 0;
	}
	if (se_sdf_control_value_equals(control->type, &control->value, value)) {
		return 0;
	}
	control->value = *value;
	control->dirty = 1;
	return 1;
}

static se_sdf_control* se_sdf_get_or_create_control(
	se_sdf_renderer* renderer,
	const char* name,
	const se_sdf_control_type type,
	se_sdf_control_handle* out_handle
) {
	if (!renderer || !name || name[0] == '\0') {
		return NULL;
	}
	se_sdf_control_handle handle = se_sdf_find_control_handle(renderer, name);
	b8 created = 0;
	if (handle == SE_SDF_CONTROL_NULL) {
		handle = s_array_increment(&renderer->controls);
		created = 1;
	}
	se_sdf_control* control = s_array_get(&renderer->controls, handle);
	if (!control) {
		return NULL;
	}
	if (handle == SE_SDF_CONTROL_NULL || control->name[0] == '\0') {
		memset(control, 0, sizeof(*control));
		strncpy(control->name, name, sizeof(control->name) - 1);
		control->name[sizeof(control->name) - 1] = '\0';
		se_sdf_make_control_uniform_name(handle, name, control->uniform_name, sizeof(control->uniform_name));
		control->cached_uniform_location = -1;
	}
	if (control->type != type) {
		se_sdf_control_clear_binding(control);
		renderer->built = 0;
	}
	control->type = type;
	control->dirty = 1;
	if (created) {
		renderer->built = 0;
	}
	if (out_handle) {
		*out_handle = handle;
	}
	return control;
}

static se_sdf_control* se_sdf_control_from_handle(
	se_sdf_renderer* renderer,
	const se_sdf_control_handle control,
	const se_sdf_control_type expected_type
) {
	if (!renderer || control == SE_SDF_CONTROL_NULL) {
		return NULL;
	}
	se_sdf_control* control_ptr = s_array_get(&renderer->controls, control);
	if (!control_ptr || control_ptr->type != expected_type) {
		return NULL;
	}
	return control_ptr;
}

static void se_sdf_control_clear_binding(se_sdf_control* control) {
	if (!control) {
		return;
	}
	memset(&control->binding, 0, sizeof(control->binding));
	control->has_binding = 0;
}

static void se_sdf_sync_control_bindings(se_sdf_renderer* renderer) {
	if (!renderer) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&renderer->controls); ++i) {
		se_sdf_control* control = s_array_get(&renderer->controls, s_array_handle(&renderer->controls, (u32)i));
		if (!control || !control->has_binding) {
			continue;
		}

		switch (control->type) {
			case SE_SDF_CONTROL_FLOAT:
				if (control->binding.f) {
					se_sdf_control_value v = {.f = *control->binding.f};
					se_sdf_control_assign_value(control, &v);
				}
				else se_sdf_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_VEC2:
				if (control->binding.vec2) {
					se_sdf_control_value v = {.vec2 = *control->binding.vec2};
					se_sdf_control_assign_value(control, &v);
				}
				else se_sdf_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_VEC3:
				if (control->binding.vec3) {
					se_sdf_control_value v = {.vec3 = *control->binding.vec3};
					se_sdf_control_assign_value(control, &v);
				}
				else se_sdf_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_VEC4:
				if (control->binding.vec4) {
					se_sdf_control_value v = {.vec4 = *control->binding.vec4};
					se_sdf_control_assign_value(control, &v);
				}
				else se_sdf_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_INT:
				if (control->binding.i) {
					se_sdf_control_value v = {.i = *control->binding.i};
					se_sdf_control_assign_value(control, &v);
				}
				else se_sdf_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_MAT4:
				if (control->binding.mat4) {
					se_sdf_control_value v = {.mat4 = *control->binding.mat4};
					se_sdf_control_assign_value(control, &v);
				}
				else se_sdf_control_clear_binding(control);
				break;
			default:
				se_sdf_control_clear_binding(control);
				break;
		}
	}
}

static f32 se_sdf_abs_or_one(const f32 value) {
	const f32 abs_value = fabsf(value);
	return abs_value < 0.000001f ? 1.0f : abs_value;
}

static void se_sdf_node_init_control_trs(se_sdf_node* node) {
	if (!node || node->control_trs_initialized) {
		return;
	}
	node->control_translation = s_vec3(
		node->transform.m[3][0],
		node->transform.m[3][1],
		node->transform.m[3][2]
	);
	node->control_rotation = s_vec3(0.0f, 0.0f, 0.0f);
	node->control_scale = s_vec3(
		se_sdf_abs_or_one(node->transform.m[0][0]),
		se_sdf_abs_or_one(node->transform.m[1][1]),
		se_sdf_abs_or_one(node->transform.m[2][2])
	);
	node->control_trs_initialized = 1;
}

static void se_sdf_node_apply_control_trs(se_sdf_node* node) {
	if (!node) {
		return;
	}
	s_mat4 transform = s_mat4_identity;
	transform = s_mat4_translate(&transform, &node->control_translation);
	if (node->control_rotation.x != 0.0f) {
		transform = s_mat4_rotate_x(&transform, node->control_rotation.x);
	}
	if (node->control_rotation.y != 0.0f) {
		transform = s_mat4_rotate_y(&transform, node->control_rotation.y);
	}
	if (node->control_rotation.z != 0.0f) {
		transform = s_mat4_rotate_z(&transform, node->control_rotation.z);
	}
	transform = s_mat4_scale(&transform, &node->control_scale);
	node->transform = transform;
}

static b8 se_sdf_bind_node_target(
	se_sdf_renderer* renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_control_handle control,
	const se_sdf_node_bind_target target
) {
	if (!renderer || scene == SE_SDF_SCENE_NULL || node == SE_SDF_NODE_NULL || control == SE_SDF_CONTROL_NULL) {
		return 0;
	}
	se_sdf_control* control_ptr = se_sdf_control_from_handle(renderer, control, SE_SDF_CONTROL_VEC3);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	se_sdf_node_init_control_trs(node_ptr);

	for (sz i = 0; i < s_array_get_size(&renderer->node_bindings); ++i) {
		se_sdf_node_binding* binding = s_array_get(&renderer->node_bindings, s_array_handle(&renderer->node_bindings, (u32)i));
		if (!binding) {
			continue;
		}
		if (binding->scene == scene && binding->node == node && binding->target == target) {
			binding->control = control;
			renderer->built = 0;
			(void)control_ptr;
			return 1;
		}
	}

	se_sdf_node_binding binding = {0};
	binding.scene = scene;
	binding.node = node;
	binding.control = control;
	binding.target = target;
	s_array_add(&renderer->node_bindings, binding);
	renderer->built = 0;
	return 1;
}

static b8 se_sdf_apply_node_bindings(se_sdf_renderer* renderer) {
	if (!renderer) {
		return 0;
	}
	b8 changed = 0;

	for (sz i = 0; i < s_array_get_size(&renderer->node_bindings); ++i) {
		se_sdf_node_binding* binding = s_array_get(&renderer->node_bindings, s_array_handle(&renderer->node_bindings, (u32)i));
		if (!binding) {
			continue;
		}

		se_sdf_control* control = se_sdf_control_from_handle(renderer, binding->control, SE_SDF_CONTROL_VEC3);
		if (!control) {
			continue;
		}
		se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(binding->scene);
		if (!scene_ptr) {
			continue;
		}
		se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, binding->scene, binding->node);
		if (!node_ptr) {
			continue;
		}

		se_sdf_node_init_control_trs(node_ptr);
		b8 node_changed = 0;
		switch (binding->target) {
			case SE_SDF_NODE_BIND_TRANSLATION:
				node_changed = (node_ptr->control_translation.x != control->value.vec3.x) ||
					(node_ptr->control_translation.y != control->value.vec3.y) ||
					(node_ptr->control_translation.z != control->value.vec3.z);
				if (node_changed) {
					node_ptr->control_translation = control->value.vec3;
				}
				break;
			case SE_SDF_NODE_BIND_ROTATION:
				node_changed = (node_ptr->control_rotation.x != control->value.vec3.x) ||
					(node_ptr->control_rotation.y != control->value.vec3.y) ||
					(node_ptr->control_rotation.z != control->value.vec3.z);
				if (node_changed) {
					node_ptr->control_rotation = control->value.vec3;
				}
				break;
			case SE_SDF_NODE_BIND_SCALE:
				node_changed = (node_ptr->control_scale.x != control->value.vec3.x) ||
					(node_ptr->control_scale.y != control->value.vec3.y) ||
					(node_ptr->control_scale.z != control->value.vec3.z);
				if (node_changed) {
					node_ptr->control_scale = control->value.vec3;
				}
				break;
			default:
				break;
		}
		if (node_changed) {
			se_sdf_node_apply_control_trs(node_ptr);
			changed = 1;
		}
	}
	return changed;
}

static b8 se_sdf_apply_primitive_param_float(
	se_sdf_node* node,
	const se_sdf_primitive_param param,
	const f32 value
) {
	if (!node || node->is_group) {
		return 0;
	}
	if ((param == SE_SDF_PRIMITIVE_PARAM_SIZE_X ||
			param == SE_SDF_PRIMITIVE_PARAM_SIZE_Y ||
			param == SE_SDF_PRIMITIVE_PARAM_SIZE_Z ||
			param == SE_SDF_PRIMITIVE_PARAM_HEIGHT ||
			param == SE_SDF_PRIMITIVE_PARAM_THICKNESS) && value <= 0.0f) {
		return 0;
	}
	if ((param == SE_SDF_PRIMITIVE_PARAM_RADIUS ||
			param == SE_SDF_PRIMITIVE_PARAM_RADIUS_A ||
			param == SE_SDF_PRIMITIVE_PARAM_RADIUS_B) && value < 0.0f) {
		return 0;
	}
	switch (node->primitive.type) {
		case SE_SDF_PRIMITIVE_SPHERE:
			if (param == SE_SDF_PRIMITIVE_PARAM_RADIUS) {
				if (node->primitive.sphere.radius == value) return 0;
				node->primitive.sphere.radius = value;
				return 1;
			}
			break;
		case SE_SDF_PRIMITIVE_BOX:
			if (param == SE_SDF_PRIMITIVE_PARAM_SIZE_X) {
				if (node->primitive.box.size.x == value) return 0;
				node->primitive.box.size.x = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_SIZE_Y) {
				if (node->primitive.box.size.y == value) return 0;
				node->primitive.box.size.y = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_SIZE_Z) {
				if (node->primitive.box.size.z == value) return 0;
				node->primitive.box.size.z = value;
			}
			else break;
			return 1;
		case SE_SDF_PRIMITIVE_ROUND_BOX:
			if (param == SE_SDF_PRIMITIVE_PARAM_SIZE_X) {
				if (node->primitive.round_box.size.x == value) return 0;
				node->primitive.round_box.size.x = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_SIZE_Y) {
				if (node->primitive.round_box.size.y == value) return 0;
				node->primitive.round_box.size.y = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_SIZE_Z) {
				if (node->primitive.round_box.size.z == value) return 0;
				node->primitive.round_box.size.z = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_RADIUS) {
				if (node->primitive.round_box.roundness == value) return 0;
				node->primitive.round_box.roundness = value;
			}
			else break;
			return 1;
		case SE_SDF_PRIMITIVE_CAPSULE:
			if (param == SE_SDF_PRIMITIVE_PARAM_RADIUS) {
				if (node->primitive.capsule.radius == value) return 0;
				node->primitive.capsule.radius = value;
				return 1;
			}
			break;
		case SE_SDF_PRIMITIVE_VERTICAL_CAPSULE:
			if (param == SE_SDF_PRIMITIVE_PARAM_HEIGHT) {
				if (node->primitive.vertical_capsule.height == value) return 0;
				node->primitive.vertical_capsule.height = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_RADIUS) {
				if (node->primitive.vertical_capsule.radius == value) return 0;
				node->primitive.vertical_capsule.radius = value;
			}
			else break;
			return 1;
		case SE_SDF_PRIMITIVE_CAPPED_CYLINDER:
			if (param == SE_SDF_PRIMITIVE_PARAM_RADIUS) {
				if (node->primitive.capped_cylinder.radius == value) return 0;
				node->primitive.capped_cylinder.radius = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_HEIGHT) {
				if (node->primitive.capped_cylinder.half_height == value) return 0;
				node->primitive.capped_cylinder.half_height = value;
			}
			else break;
			return 1;
		case SE_SDF_PRIMITIVE_CONE:
			if (param == SE_SDF_PRIMITIVE_PARAM_HEIGHT) {
				if (node->primitive.cone.height == value) return 0;
				node->primitive.cone.height = value;
				return 1;
			}
			break;
		case SE_SDF_PRIMITIVE_ROUND_CONE:
			if (param == SE_SDF_PRIMITIVE_PARAM_HEIGHT) {
				if (node->primitive.round_cone.height == value) return 0;
				node->primitive.round_cone.height = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_RADIUS_A) {
				if (node->primitive.round_cone.radius_base == value) return 0;
				node->primitive.round_cone.radius_base = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_RADIUS_B) {
				if (node->primitive.round_cone.radius_top == value) return 0;
				node->primitive.round_cone.radius_top = value;
			}
			else break;
			return 1;
		case SE_SDF_PRIMITIVE_CUT_SPHERE:
			if (param == SE_SDF_PRIMITIVE_PARAM_RADIUS) {
				if (node->primitive.cut_sphere.radius == value) return 0;
				node->primitive.cut_sphere.radius = value;
			}
			else if (param == SE_SDF_PRIMITIVE_PARAM_HEIGHT) {
				if (node->primitive.cut_sphere.cut_height == value) return 0;
				node->primitive.cut_sphere.cut_height = value;
			}
			else break;
			return 1;
		default:
			break;
	}
	return 0;
}

static b8 se_sdf_apply_primitive_bindings(se_sdf_renderer* renderer) {
	if (!renderer) {
		return 0;
	}
	b8 changed = 0;

	for (sz i = 0; i < s_array_get_size(&renderer->primitive_bindings); ++i) {
		se_sdf_primitive_binding* binding = s_array_get(&renderer->primitive_bindings, s_array_handle(&renderer->primitive_bindings, (u32)i));
		if (!binding) {
			continue;
		}
		se_sdf_control* control = se_sdf_control_from_handle(renderer, binding->control, SE_SDF_CONTROL_FLOAT);
		if (!control) {
			continue;
		}
		se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(binding->scene);
		if (!scene_ptr) {
			continue;
		}
		se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, binding->scene, binding->node);
		if (!node_ptr) {
			continue;
		}
		if (se_sdf_apply_primitive_param_float(node_ptr, binding->param, control->value.f)) {
			changed = 1;
		}
	}
	return changed;
}

static void se_sdf_apply_renderer_shading_bindings(se_sdf_renderer* renderer) {
	if (!renderer) {
		return;
	}
	se_sdf_control* control = NULL;

	control = se_sdf_control_from_handle(renderer, renderer->material_base_color_control, SE_SDF_CONTROL_VEC3);
	if (control) {
		renderer->material.base_color = control->value.vec3;
	}

	control = se_sdf_control_from_handle(renderer, renderer->lighting_direction_control, SE_SDF_CONTROL_VEC3);
	if (control) {
		renderer->lighting_direction = control->value.vec3;
	}

	control = se_sdf_control_from_handle(renderer, renderer->lighting_color_control, SE_SDF_CONTROL_VEC3);
	if (control) {
		renderer->lighting_color = control->value.vec3;
	}

	control = se_sdf_control_from_handle(renderer, renderer->fog_color_control, SE_SDF_CONTROL_VEC3);
	if (control) {
		renderer->fog_color = control->value.vec3;
	}

	control = se_sdf_control_from_handle(renderer, renderer->fog_density_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->fog_density = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_band_levels_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.band_levels = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_rim_power_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.rim_power = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_rim_strength_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.rim_strength = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_fill_strength_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.fill_strength = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_back_strength_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.back_strength = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_checker_scale_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.checker_scale = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_checker_strength_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.checker_strength = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_ground_blend_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.ground_blend = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_desaturate_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.desaturate = control->value.f;
	}

	control = se_sdf_control_from_handle(renderer, renderer->stylized_gamma_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.gamma = control->value.f;
	}
}

static const char* se_sdf_control_glsl_type(const se_sdf_control_type type) {
	switch (type) {
		case SE_SDF_CONTROL_FLOAT: return "float";
		case SE_SDF_CONTROL_VEC2: return "vec2";
		case SE_SDF_CONTROL_VEC3: return "vec3";
		case SE_SDF_CONTROL_VEC4: return "vec4";
		case SE_SDF_CONTROL_INT: return "int";
		case SE_SDF_CONTROL_MAT4: return "mat4";
		default: return NULL;
	}
}

static b8 se_sdf_emit_control_uniform_declarations(
	se_sdf_renderer* renderer,
	se_sdf_string* out
) {
	if (!renderer || !out) {
		return 0;
	}

	sz declared_count = 0;
	for (sz i = 0; i < s_array_get_size(&renderer->controls); ++i) {
		const se_sdf_control* control = s_array_get(
			&renderer->controls,
			s_array_handle(&renderer->controls, (u32)i)
		);
		if (!control || control->uniform_name[0] == '\0') {
			continue;
		}
		const char* glsl_type = se_sdf_control_glsl_type(control->type);
		if (!glsl_type) {
			continue;
		}
		se_sdf_string_append(out, "uniform %s %s;\n", glsl_type, control->uniform_name);
		declared_count++;
	}
	if (declared_count > 0) {
		se_sdf_string_append(out, "\n");
	}
	return !out->oom;
}

static const char* se_sdf_find_node_binding_uniform_name(
	se_sdf_renderer* renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_node_bind_target target
) {
	if (!renderer || scene == SE_SDF_SCENE_NULL || node == SE_SDF_NODE_NULL) {
		return NULL;
	}

	for (sz i = 0; i < s_array_get_size(&renderer->node_bindings); ++i) {
		const se_sdf_node_binding* binding = s_array_get(
			&renderer->node_bindings,
			s_array_handle(&renderer->node_bindings, (u32)i)
		);
		if (!binding || binding->scene != scene || binding->node != node || binding->target != target) {
			continue;
		}
		const se_sdf_control* control = s_array_get(&renderer->controls, binding->control);
		if (!control || control->type != SE_SDF_CONTROL_VEC3 || control->uniform_name[0] == '\0') {
			continue;
		}
		return control->uniform_name;
	}
	return NULL;
}

static const char* se_sdf_find_primitive_binding_uniform_name(
	se_sdf_renderer* renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_primitive_param param
) {
	if (!renderer || scene == SE_SDF_SCENE_NULL || node == SE_SDF_NODE_NULL) {
		return NULL;
	}

	for (sz i = 0; i < s_array_get_size(&renderer->primitive_bindings); ++i) {
		const se_sdf_primitive_binding* binding = s_array_get(
			&renderer->primitive_bindings,
			s_array_handle(&renderer->primitive_bindings, (u32)i)
		);
		if (!binding || binding->scene != scene || binding->node != node || binding->param != param) {
			continue;
		}
		const se_sdf_control* control = s_array_get(&renderer->controls, binding->control);
		if (!control || control->type != SE_SDF_CONTROL_FLOAT || control->uniform_name[0] == '\0') {
			continue;
		}
		return control->uniform_name;
	}
	return NULL;
}

se_sdf_control_handle se_sdf_control_define_float(
	const se_sdf_renderer_handle renderer,
	const char* name,
	const f32 default_value
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_control* control = se_sdf_get_or_create_control(
		renderer_ptr, name, SE_SDF_CONTROL_FLOAT, &handle);
	if (!control) {
		return SE_SDF_CONTROL_NULL;
	}
	control->value.f = default_value;
	return handle;
}

se_sdf_control_handle se_sdf_control_define_vec2(
	const se_sdf_renderer_handle renderer,
	const char* name,
	const s_vec2* default_value
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !default_value) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_control* control = se_sdf_get_or_create_control(
		renderer_ptr, name, SE_SDF_CONTROL_VEC2, &handle);
	if (!control) {
		return SE_SDF_CONTROL_NULL;
	}
	control->value.vec2 = *default_value;
	return handle;
}

se_sdf_control_handle se_sdf_control_define_vec3(
	const se_sdf_renderer_handle renderer,
	const char* name,
	const s_vec3* default_value
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !default_value) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_control* control = se_sdf_get_or_create_control(
		renderer_ptr, name, SE_SDF_CONTROL_VEC3, &handle);
	if (!control) {
		return SE_SDF_CONTROL_NULL;
	}
	control->value.vec3 = *default_value;
	return handle;
}

se_sdf_control_handle se_sdf_control_define_vec4(
	const se_sdf_renderer_handle renderer,
	const char* name,
	const s_vec4* default_value
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !default_value) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_control* control = se_sdf_get_or_create_control(
		renderer_ptr, name, SE_SDF_CONTROL_VEC4, &handle);
	if (!control) {
		return SE_SDF_CONTROL_NULL;
	}
	control->value.vec4 = *default_value;
	return handle;
}

se_sdf_control_handle se_sdf_control_define_int(
	const se_sdf_renderer_handle renderer,
	const char* name,
	const i32 default_value
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_control* control = se_sdf_get_or_create_control(
		renderer_ptr, name, SE_SDF_CONTROL_INT, &handle);
	if (!control) {
		return SE_SDF_CONTROL_NULL;
	}
	control->value.i = default_value;
	return handle;
}

se_sdf_control_handle se_sdf_control_define_mat4(
	const se_sdf_renderer_handle renderer,
	const char* name,
	const s_mat4* default_value
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !default_value) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_control* control = se_sdf_get_or_create_control(
		renderer_ptr, name, SE_SDF_CONTROL_MAT4, &handle);
	if (!control) {
		return SE_SDF_CONTROL_NULL;
	}
	control->value.mat4 = *default_value;
	return handle;
}

b8 se_sdf_control_set_float(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const f32 value
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_FLOAT);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_control_value v = {.f = value};
	se_sdf_control_assign_value(control_ptr, &v);
	return 1;
}

b8 se_sdf_control_set_vec2(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const s_vec2* value
) {
	if (!value) {
		return 0;
	}
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC2);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_control_value v = {.vec2 = *value};
	se_sdf_control_assign_value(control_ptr, &v);
	return 1;
}

b8 se_sdf_control_set_vec3(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const s_vec3* value
) {
	if (!value) {
		return 0;
	}
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC3);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_control_value v = {.vec3 = *value};
	se_sdf_control_assign_value(control_ptr, &v);
	return 1;
}

b8 se_sdf_control_set_vec4(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const s_vec4* value
) {
	if (!value) {
		return 0;
	}
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC4);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_control_value v = {.vec4 = *value};
	se_sdf_control_assign_value(control_ptr, &v);
	return 1;
}

b8 se_sdf_control_set_int(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const i32 value
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_INT);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_control_value v = {.i = value};
	se_sdf_control_assign_value(control_ptr, &v);
	return 1;
}

b8 se_sdf_control_set_mat4(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const s_mat4* value
) {
	if (!value) {
		return 0;
	}
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_MAT4);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_control_value v = {.mat4 = *value};
	se_sdf_control_assign_value(control_ptr, &v);
	return 1;
}

b8 se_sdf_control_bind_ptr_float(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const f32* value_ptr
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_FLOAT);
	if (!control_ptr) {
		return 0;
	}
	control_ptr->binding.f = value_ptr;
	control_ptr->has_binding = (value_ptr != NULL);
	return 1;
}

b8 se_sdf_control_bind_ptr_vec2(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const s_vec2* value_ptr
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC2);
	if (!control_ptr) {
		return 0;
	}
	control_ptr->binding.vec2 = value_ptr;
	control_ptr->has_binding = (value_ptr != NULL);
	return 1;
}

b8 se_sdf_control_bind_ptr_vec3(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const s_vec3* value_ptr
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC3);
	if (!control_ptr) {
		return 0;
	}
	control_ptr->binding.vec3 = value_ptr;
	control_ptr->has_binding = (value_ptr != NULL);
	return 1;
}

b8 se_sdf_control_bind_ptr_vec4(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const s_vec4* value_ptr
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC4);
	if (!control_ptr) {
		return 0;
	}
	control_ptr->binding.vec4 = value_ptr;
	control_ptr->has_binding = (value_ptr != NULL);
	return 1;
}

b8 se_sdf_control_bind_ptr_int(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const i32* value_ptr
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_INT);
	if (!control_ptr) {
		return 0;
	}
	control_ptr->binding.i = value_ptr;
	control_ptr->has_binding = (value_ptr != NULL);
	return 1;
}

b8 se_sdf_control_bind_ptr_mat4(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const s_mat4* value_ptr
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	se_sdf_control* control_ptr = se_sdf_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_MAT4);
	if (!control_ptr) {
		return 0;
	}
	control_ptr->binding.mat4 = value_ptr;
	control_ptr->has_binding = (value_ptr != NULL);
	return 1;
}

b8 se_sdf_control_bind_node_position(
	const se_sdf_renderer_handle renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return se_sdf_bind_node_target(
		renderer_ptr, scene, node, control, SE_SDF_NODE_BIND_TRANSLATION);
}

b8 se_sdf_control_bind_node_rotation(
	const se_sdf_renderer_handle renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return se_sdf_bind_node_target(
		renderer_ptr, scene, node, control, SE_SDF_NODE_BIND_ROTATION);
}

b8 se_sdf_control_bind_node_scale(
	const se_sdf_renderer_handle renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return se_sdf_bind_node_target(
		renderer_ptr, scene, node, control, SE_SDF_NODE_BIND_SCALE);
}

static b8 se_sdf_bind_primitive_param_float(
	se_sdf_renderer* renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_primitive_param param,
	const se_sdf_control_handle control
) {
	if (!renderer || scene == SE_SDF_SCENE_NULL || node == SE_SDF_NODE_NULL || control == SE_SDF_CONTROL_NULL) {
		return 0;
	}
	if (!se_sdf_control_from_handle(renderer, control, SE_SDF_CONTROL_FLOAT)) {
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr || node_ptr->is_group) {
		return 0;
	}

	for (sz i = 0; i < s_array_get_size(&renderer->primitive_bindings); ++i) {
		se_sdf_primitive_binding* binding = s_array_get(&renderer->primitive_bindings, s_array_handle(&renderer->primitive_bindings, (u32)i));
		if (!binding) {
			continue;
		}
		if (binding->scene == scene && binding->node == node && binding->param == param) {
			binding->control = control;
			renderer->built = 0;
			return 1;
		}
	}

	se_sdf_primitive_binding binding = {0};
	binding.scene = scene;
	binding.node = node;
	binding.param = param;
	binding.control = control;
	s_array_add(&renderer->primitive_bindings, binding);
	renderer->built = 0;
	return 1;
}

b8 se_sdf_control_bind_primitive_float(
	const se_sdf_renderer_handle renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_primitive_param param,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return se_sdf_bind_primitive_param_float(renderer_ptr, scene, node, param, control);
}

static b8 se_sdf_bind_renderer_float_control(
	se_sdf_renderer* renderer,
	se_sdf_control_handle* target_handle,
	const se_sdf_control_handle control
) {
	if (!renderer || !target_handle) {
		return 0;
	}
	if (!se_sdf_control_from_handle(renderer, control, SE_SDF_CONTROL_FLOAT)) {
		return 0;
	}
	*target_handle = control;
	return 1;
}

b8 se_sdf_control_bind_base_color(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !se_sdf_control_from_handle(renderer_ptr, control, SE_SDF_CONTROL_VEC3)) {
		return 0;
	}
	renderer_ptr->material_base_color_control = control;
	return 1;
}

b8 se_sdf_control_bind_light_direction(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !se_sdf_control_from_handle(renderer_ptr, control, SE_SDF_CONTROL_VEC3)) {
		return 0;
	}
	renderer_ptr->lighting_direction_control = control;
	return 1;
}

b8 se_sdf_control_bind_light_color(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !se_sdf_control_from_handle(renderer_ptr, control, SE_SDF_CONTROL_VEC3)) {
		return 0;
	}
	renderer_ptr->lighting_color_control = control;
	return 1;
}

b8 se_sdf_control_bind_fog_color(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr || !se_sdf_control_from_handle(renderer_ptr, control, SE_SDF_CONTROL_VEC3)) {
		return 0;
	}
	renderer_ptr->fog_color_control = control;
	return 1;
}

b8 se_sdf_control_bind_fog_density(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->fog_density_control, control);
}

b8 se_sdf_control_bind_style_bands(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_band_levels_control, control);
}

b8 se_sdf_control_bind_style_rim_power(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_rim_power_control, control);
}

b8 se_sdf_control_bind_style_rim_strength(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_rim_strength_control, control);
}

b8 se_sdf_control_bind_style_fill(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_fill_strength_control, control);
}

b8 se_sdf_control_bind_style_back(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_back_strength_control, control);
}

b8 se_sdf_control_bind_style_checker_scale(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_checker_scale_control, control);
}

b8 se_sdf_control_bind_style_checker_strength(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_checker_strength_control, control);
}

b8 se_sdf_control_bind_style_ground_blend(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_ground_blend_control, control);
}

b8 se_sdf_control_bind_style_desaturate(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_desaturate_control, control);
}

b8 se_sdf_control_bind_style_gamma(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	return se_sdf_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_gamma_control, control);
}

// ============================================================================
// String Buffer
// ============================================================================

static b8 se_sdf_string_ensure_capacity(se_sdf_string* str, const sz min_capacity) {
	if (!str) {
		return 0;
	}
	if (str->data && str->capacity >= min_capacity) {
		return 1;
	}

	sz new_capacity = str->capacity > 0 ? str->capacity : (sz)256;
	while (new_capacity < min_capacity) {
		if (new_capacity > (((sz)-1) / 2)) {
			new_capacity = min_capacity;
			break;
		}
		new_capacity *= 2;
	}

	char* new_data = (char*)realloc(str->data, new_capacity);
	if (!new_data) {
		str->oom = 1;
		return 0;
	}

	str->data = new_data;
	str->capacity = new_capacity;
	if (str->size == 0) {
		str->data[0] = '\0';
	}
	return 1;
}

void se_sdf_string_init(se_sdf_string* str, sz capacity) {
	if (!str) {
		return;
	}
	str->data = NULL;
	str->capacity = 0;
	str->size = 0;
	str->oom = 0;
	if (capacity > 0) {
		se_sdf_string_ensure_capacity(str, capacity);
	}
}

void se_sdf_string_free(se_sdf_string* str) {
	if (!str) {
		return;
	}
	if (str->data) {
		free(str->data);
	}
	str->data = NULL;
	str->capacity = 0;
	str->size = 0;
	str->oom = 0;
}

void se_sdf_string_append(se_sdf_string* str, const char* fmt, ...) {
	if (!str || !fmt || str->oom) {
		return;
	}

	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	int required = vsnprintf(NULL, 0, fmt, args_copy);
	va_end(args_copy);
	if (required < 0) {
		str->oom = 1;
		va_end(args);
		return;
	}

	sz required_size = str->size + (sz)required + 1;
	if (!se_sdf_string_ensure_capacity(str, required_size)) {
		va_end(args);
		return;
	}

	int written = vsnprintf(str->data + str->size, str->capacity - str->size, fmt, args);
	va_end(args);
	if (written < 0) {
		str->oom = 1;
		return;
	}
	str->size += (sz)written;
}

static void se_sdf_string_append_raw(se_sdf_string* str, const char* data, sz length) {
	if (!str || !data || length == 0 || str->oom) {
		return;
	}
	if (!se_sdf_string_ensure_capacity(str, str->size + length + 1)) {
		return;
	}
	memcpy(str->data + str->size, data, length);
	str->size += length;
	str->data[str->size] = '\0';
}

void se_sdf_string_append_vec2(se_sdf_string* str, s_vec2 v) {
	se_sdf_string_append(str, "vec2(%.6f, %.6f)", v.x, v.y);
}

void se_sdf_string_append_vec3(se_sdf_string* str, s_vec3 v) {
	se_sdf_string_append(str, "vec3(%.6f, %.6f, %.6f)", v.x, v.y, v.z);
}

void se_sdf_symbol_allocator_init(se_sdf_symbol_allocator* allocator) {
	if (!allocator) {
		return;
	}
	allocator->next_id = 0;
}

void se_sdf_symbol_allocator_reset(se_sdf_symbol_allocator* allocator) {
	se_sdf_symbol_allocator_init(allocator);
}

u32 se_sdf_symbol_allocator_next(se_sdf_symbol_allocator* allocator) {
	if (!allocator) {
		return 0;
	}
	return allocator->next_id++;
}

void se_sdf_symbol_allocator_make_name(
	se_sdf_symbol_allocator* allocator,
	const char* prefix,
	char* out_name,
	sz out_name_size
) {
	if (!out_name || out_name_size == 0) {
		return;
	}
	const char* safe_prefix = prefix ? prefix : "_sym";
	u32 id = se_sdf_symbol_allocator_next(allocator);
	snprintf(out_name, out_name_size, "%s%u", safe_prefix, id);
}

b8 se_sdf_codegen_emit_fragment_prelude(se_sdf_string* out) {
	if (!out) {
		return 0;
	}
	se_sdf_string_append(out,
		"#version 330 core\n"
		"in vec2 v_uv;\n"
		"out vec4 FragColor;\n\n");
	return !out->oom;
}

b8 se_sdf_codegen_emit_uniform_block(se_sdf_string* out) {
	if (!out) {
		return 0;
	}
	se_sdf_string_append(out,
		"uniform vec2 u_resolution;\n"
		"uniform float u_time;\n"
		"uniform vec2 u_mouse;\n"
		"uniform mat4 u_camera_view;\n"
		"uniform mat4 u_camera_projection;\n"
		"uniform mat4 u_camera_inv_view_projection;\n"
		"uniform vec3 u_camera_position;\n"
		"uniform vec3 u_camera_forward;\n"
		"uniform vec3 u_camera_right;\n"
		"uniform vec3 u_camera_up;\n"
		"uniform vec2 u_camera_near_far;\n"
		"uniform float u_camera_tan_half_fov;\n"
		"uniform float u_camera_aspect;\n"
		"uniform float u_camera_ortho_height;\n"
		"uniform int u_camera_is_orthographic;\n"
		"uniform sampler2D u_scene_depth;\n"
		"uniform int u_has_scene_depth;\n"
		"uniform int u_quality_max_steps;\n"
		"uniform float u_quality_hit_epsilon;\n"
		"uniform float u_quality_max_distance;\n"
		"uniform int u_debug_show_normals;\n"
		"uniform int u_debug_show_distance;\n"
		"uniform int u_debug_show_steps;\n\n");
	se_sdf_string_append(out,
		"uniform vec3 u_material_base_color;\n"
		"uniform vec3 u_light_direction;\n"
		"uniform vec3 u_light_color;\n"
		"uniform vec3 u_fog_color;\n"
		"uniform float u_fog_density;\n"
		"uniform float u_stylized_band_levels;\n"
		"uniform float u_stylized_rim_power;\n"
		"uniform float u_stylized_rim_strength;\n"
		"uniform float u_stylized_fill_strength;\n"
		"uniform float u_stylized_back_strength;\n"
		"uniform float u_stylized_checker_scale;\n"
		"uniform float u_stylized_checker_strength;\n"
		"uniform float u_stylized_ground_blend;\n"
		"uniform float u_stylized_desaturate;\n"
		"uniform float u_stylized_gamma;\n\n");
	return !out->oom;
}

b8 se_sdf_codegen_emit_map_stub(se_sdf_string* out, const char* map_func_name) {
	if (!out) {
		return 0;
	}
	const char* name = (map_func_name && map_func_name[0] != '\0') ? map_func_name : "map";
	se_sdf_string_append(out,
		"float %s(vec3 p) {\n"
		"	return length(p) + 1e10;\n"
		"}\n\n", name);
	return !out->oom;
}

b8 se_sdf_codegen_emit_shading_stub(se_sdf_string* out) {
	if (!out) {
		return 0;
	}
	se_sdf_string_append(out,
		"float se_sdf_quantize(float v, float levels) {\n"
		"	levels = max(levels, 1.0);\n"
		"	v = clamp(v, 0.0, 0.9999);\n"
		"	return floor(v * levels) / max(levels - 1.0, 1.0);\n"
		"}\n"
		"vec3 se_sdf_desaturate(vec3 c, float amount) {\n"
		"	float l = dot(c, vec3(0.299, 0.587, 0.114));\n"
		"	return mix(c, vec3(l), clamp(amount, 0.0, 1.0));\n"
		"}\n"
		"vec3 se_sdf_normal(vec3 p) {\n"
		"	vec2 e = vec2(0.0015, 0.0);\n"
		"	return normalize(vec3(\n"
		"		map(p + e.xyy) - map(p - e.xyy),\n"
		"		map(p + e.yxy) - map(p - e.yxy),\n"
		"		map(p + e.yyx) - map(p - e.yyx)\n"
		"	));\n"
		"}\n"
		"vec3 se_sdf_world_from_ndc(vec3 ndc) {\n"
		"	vec4 world = u_camera_inv_view_projection * vec4(ndc, 1.0);\n"
		"	float inv_w = (abs(world.w) > 0.000001) ? (1.0 / world.w) : 0.0;\n"
		"	return world.xyz * inv_w;\n"
		"}\n"
		"vec3 se_sdf_stylized(vec3 p, vec3 rd) {\n"
		"	vec3 n = se_sdf_normal(p);\n"
		"	vec3 key_dir = normalize(u_light_direction);\n"
		"	vec3 fill_dir = normalize(vec3(-key_dir.z, 0.35, -key_dir.x));\n"
		"	vec3 back_dir = normalize(vec3(0.0, 0.2, -1.0));\n"
		"	float key = max(dot(n, key_dir), 0.0);\n"
		"	float fill = max(dot(n, fill_dir), 0.0);\n"
		"	float back = max(dot(n, back_dir), 0.0);\n"
		"	float key_band = se_sdf_quantize(key, u_stylized_band_levels);\n"
		"	float rim = pow(clamp(1.0 - max(dot(n, -rd), 0.0), 0.0, 1.0), max(u_stylized_rim_power, 0.001));\n"
		"	float checker_freq = max(u_stylized_checker_scale, 0.0001);\n"
		"	float checker = 0.5 + 0.5 * sign(sin(checker_freq * p.x) * sin(checker_freq * p.z));\n"
		"	float checker_mix = clamp(u_stylized_checker_strength, 0.0, 1.0);\n"
		"	vec3 checker_tint = mix(vec3(0.88), vec3(1.16), checker);\n"
		"	vec3 base = u_material_base_color * mix(vec3(1.0), checker_tint, checker_mix);\n"
		"	vec3 tone0 = base * 0.24;\n"
		"	vec3 tone1 = base * 0.45 + vec3(0.03);\n"
		"	vec3 tone2 = base * 0.70 + vec3(0.05);\n"
		"	vec3 tone3 = base * 0.95 + vec3(0.08);\n"
		"	vec3 stylized_color = tone0;\n"
		"	stylized_color = mix(stylized_color, tone1, step(0.25, key_band));\n"
		"	stylized_color = mix(stylized_color, tone2, step(0.55, key_band));\n"
		"	stylized_color = mix(stylized_color, tone3, step(0.85, key_band));\n"
		"	stylized_color += u_light_color * fill * u_stylized_fill_strength * 0.20;\n"
		"	stylized_color += vec3(0.18, 0.16, 0.14) * back * u_stylized_back_strength * 0.18;\n"
		"	stylized_color += vec3(0.30, 0.34, 0.40) * rim * u_stylized_rim_strength * 0.35;\n"
		"	float ground_mask = 1.0 - smoothstep(-0.9, 0.4, p.y);\n"
		"	float ground_mix = clamp(ground_mask * u_stylized_ground_blend, 0.0, 1.0);\n"
		"	vec3 ground_tint = mix(vec3(0.20, 0.16, 0.12), vec3(0.28, 0.24, 0.20), checker);\n"
		"	stylized_color = mix(stylized_color, stylized_color * 0.80 + ground_tint * 0.20, ground_mix);\n"
		"	stylized_color = se_sdf_desaturate(stylized_color, u_stylized_desaturate);\n"
		"	return max(stylized_color, vec3(0.0));\n"
		"}\n\n");
	return !out->oom;
}

b8 se_sdf_codegen_emit_fragment_main(se_sdf_string* out, const char* map_func_name) {
	if (!out) {
		return 0;
	}
	const char* name = (map_func_name && map_func_name[0] != '\0') ? map_func_name : "map";
	se_sdf_string_append(out,
		"void main(void) {\n"
		"	vec2 res = max(u_resolution, vec2(1.0));\n"
		"	vec2 screen_uv = gl_FragCoord.xy / res;\n"
		"	vec2 uv = screen_uv * 2.0 - 1.0;\n"
		"	vec3 camera_forward = normalize(u_camera_forward);\n"
		"	vec3 camera_right = normalize(u_camera_right);\n"
		"	vec3 camera_up = normalize(u_camera_up);\n"
		"	vec3 ro = u_camera_position;\n"
		"	vec3 rd = camera_forward;\n"
		"	if (u_camera_is_orthographic != 0) {\n"
		"		float half_height = max(u_camera_ortho_height * 0.5, 0.0001);\n"
		"		float half_width = half_height * max(u_camera_aspect, 0.0001);\n"
		"		ro += camera_right * (uv.x * half_width);\n"
		"		ro += camera_up * (uv.y * half_height);\n"
		"	} else {\n"
		"		float tan_half_fov = max(u_camera_tan_half_fov, 0.0001);\n"
		"		rd = normalize(camera_forward +\n"
		"			camera_right * (uv.x * tan_half_fov * u_camera_aspect) +\n"
		"			camera_up * (uv.y * tan_half_fov));\n"
		"	}\n"
		"	float march_start = max(u_camera_near_far.x, 0.0);\n"
		"	float march_limit = min(u_quality_max_distance, max(u_camera_near_far.y, 0.0001));\n"
		"	float scene_depth_sample = 1.0;\n"
		"	if (u_has_scene_depth != 0) {\n"
		"		scene_depth_sample = texture(u_scene_depth, screen_uv).r;\n"
		"		if (scene_depth_sample < 0.999999) {\n"
		"			float near_plane = max(u_camera_near_far.x, 0.0001);\n"
		"			float far_plane = max(u_camera_near_far.y, near_plane + 0.0001);\n"
		"			float linear_depth = 0.0;\n"
		"			if (u_camera_is_orthographic != 0) {\n"
		"				linear_depth = mix(near_plane, far_plane, scene_depth_sample);\n"
		"			} else {\n"
		"				float z_ndc = scene_depth_sample * 2.0 - 1.0;\n"
		"				linear_depth = (2.0 * near_plane * far_plane) /\n"
		"					max(far_plane + near_plane - z_ndc * (far_plane - near_plane), 0.0001);\n"
		"			}\n"
		"			float ray_forward = max(dot(rd, camera_forward), 0.0001);\n"
		"			float depth_t = linear_depth / ray_forward;\n"
		"			if (depth_t > 0.0) {\n"
		"				march_limit = min(march_limit, depth_t);\n"
		"			}\n"
		"		}\n"
		"	}\n"
		"	float t = march_start;\n"
		"	int used_steps = 0;\n"
		"	bool hit = false;\n"
		"	vec3 p = ro;\n"
		"	for (int i = 0; i < 512; ++i) {\n"
		"		if (i >= u_quality_max_steps) break;\n"
		"		if (t > march_limit) break;\n"
		"		p = ro + rd * t;\n"
		"		float d = %s(p);\n"
		"		used_steps = i + 1;\n"
		"		if (abs(d) < u_quality_hit_epsilon) {\n"
		"			hit = true;\n"
		"			break;\n"
		"		}\n"
		"		t += d;\n"
		"		if (t > march_limit) break;\n"
		"	}\n"
		"	vec3 color = u_fog_color;\n"
		"	bool debug_enabled = (u_debug_show_normals != 0) || (u_debug_show_distance != 0) || (u_debug_show_steps != 0);\n"
		"	if (hit) {\n"
		"		color = se_sdf_stylized(p, rd);\n"
		"		float fog = exp(-u_fog_density * t * t);\n"
		"		color = mix(u_fog_color, color, fog);\n"
		"	}\n"
		"	if (u_debug_show_normals != 0 && hit) {\n"
		"		color = 0.5 + 0.5 * se_sdf_normal(p);\n"
		"	}\n"
		"	if (u_debug_show_distance != 0) {\n"
		"		float travelled = max(t - march_start, 0.0);\n"
		"		float max_travel = max(march_limit - march_start, 0.0001);\n"
		"		float dnorm = clamp(travelled / max_travel, 0.0, 1.0);\n"
		"		color = vec3(dnorm);\n"
		"	}\n"
		"	if (u_debug_show_steps != 0) {\n"
		"		float snorm = float(used_steps) / max(float(u_quality_max_steps), 1.0);\n"
		"		color = vec3(snorm, snorm * snorm, 1.0 - snorm);\n"
		"	}\n"
		"	if (!hit && u_has_scene_depth != 0 && !debug_enabled) {\n"
		"		discard;\n"
		"	}\n"
		"	color = pow(max(color, vec3(0.0)), vec3(1.0 / max(u_stylized_gamma, 0.0001)));\n"
		"	FragColor = vec4(color, 1.0);\n"
		"	if (hit) {\n"
		"		vec4 clip_hit = u_camera_projection * (u_camera_view * vec4(p, 1.0));\n"
		"		float inv_w = (abs(clip_hit.w) > 0.000001) ? (1.0 / clip_hit.w) : 0.0;\n"
		"		float hit_depth = (clip_hit.z * inv_w) * 0.5 + 0.5;\n"
		"		gl_FragDepth = clamp(hit_depth, 0.0, 1.0);\n"
		"	} else if (u_has_scene_depth != 0) {\n"
		"		gl_FragDepth = clamp(scene_depth_sample, 0.0, 1.0);\n"
		"	} else {\n"
		"		gl_FragDepth = 1.0;\n"
		"	}\n"
		"}\n", name);
	return !out->oom;
}

b8 se_sdf_codegen_emit_fragment_scaffold(se_sdf_string* out, const char* map_func_name) {
	if (!out) {
		return 0;
	}
	if (!se_sdf_codegen_emit_fragment_prelude(out)) {
		return 0;
	}
	if (!se_sdf_codegen_emit_uniform_block(out)) {
		return 0;
	}
	if (!se_sdf_codegen_emit_map_stub(out, map_func_name)) {
		return 0;
	}
	if (!se_sdf_codegen_emit_shading_stub(out)) {
		return 0;
	}
	return se_sdf_codegen_emit_fragment_main(out, map_func_name);
}

// ============================================================================
// Shape Generators
// ============================================================================

void se_sdf_gen_sphere(se_sdf_string* out, const char* p, f32 radius) {
    se_sdf_string_append(out, "(length(%s) - %.6f)", p, radius);
}

void se_sdf_gen_box(se_sdf_string* out, const char* p, s_vec3 size) {
    se_sdf_string_append(out, "{ vec3 q = abs(%s) - ", p);
    se_sdf_string_append_vec3(out, size);
    se_sdf_string_append(out, "; length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); }");
}

void se_sdf_gen_round_box(se_sdf_string* out, const char* p, s_vec3 size, f32 roundness) {
    se_sdf_string_append(out, "{ vec3 q = abs(%s) - ", p);
    se_sdf_string_append_vec3(out, size);
    se_sdf_string_append(out, " + vec3(%.6f); length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - %.6f; }", 
                          roundness, roundness);
}

void se_sdf_gen_box_frame(se_sdf_string* out, const char* p, s_vec3 size, f32 thickness) {
    se_sdf_string_append(out, "{ vec3 q = abs(%s) - ", p);
    se_sdf_string_append_vec3(out, size);
    se_sdf_string_append(out, "; vec3 w = abs(q + vec3(%.6f)) - vec3(%.6f); ", thickness, thickness);
    se_sdf_string_append(out, "min(min(length(max(vec3(q.x, w.y, w.z), 0.0)) + min(max(q.x, max(w.y, w.z)), 0.0), ");
    se_sdf_string_append(out, "length(max(vec3(w.x, q.y, w.z), 0.0)) + min(max(w.x, max(q.y, w.z)), 0.0)), ");
    se_sdf_string_append(out, "length(max(vec3(w.x, w.y, q.z), 0.0)) + min(max(w.x, max(w.y, q.z)), 0.0)); }");
}

void se_sdf_gen_torus(se_sdf_string* out, const char* p, f32 major_r, f32 minor_r) {
    se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz) - %.6f, %s.y); length(q) - %.6f; }",
                          p, major_r, p, minor_r);
}

void se_sdf_gen_capped_torus(se_sdf_string* out, const char* p, s_vec2 sc, f32 ra, f32 rb) {
    se_sdf_string_append(out, "{ vec3 pp = %s; pp.x = abs(pp.x); ", p);
    se_sdf_string_append(out, "float k = (%.6f * pp.x > %.6f * pp.y) ? dot(pp.xy, vec2(%.6f, %.6f)) : length(pp.xy); ",
                          sc.y, sc.x, sc.x, sc.y);
    se_sdf_string_append(out, "sqrt(dot(pp, pp) + %.6f * %.6f - 2.0 * %.6f * k) - %.6f; }",
                          ra, ra, ra, rb);
}

void se_sdf_gen_link(se_sdf_string* out, const char* p, f32 le, f32 r1, f32 r2) {
    se_sdf_string_append(out, "{ vec3 q = vec3(%s.x, max(abs(%s.y) - %.6f, 0.0), %s.z); ", p, p, le, p);
    se_sdf_string_append(out, "length(vec2(length(q.xy) - %.6f, q.z)) - %.6f; }", r1, r2);
}

void se_sdf_gen_cylinder(se_sdf_string* out, const char* p, s_vec3 c) {
    se_sdf_string_append(out, "(length(%s.xz - vec2(%.6f, %.6f)) - %.6f)",
                          p, c.x, c.y, c.z);
}

void se_sdf_gen_cone(se_sdf_string* out, const char* p, s_vec2 sc, f32 h) {
    se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "vec2 k1 = vec2(%.6f, %.6f); ", sc.y, h);
    se_sdf_string_append(out, "vec2 k2 = vec2(%.6f - %.6f, 2.0 * %.6f); ", sc.y, sc.x, h);
    se_sdf_string_append(out, "vec2 ca = vec2(q.x - min(q.x, (q.y < 0.0) ? %.6f : %.6f), abs(q.y) - %.6f); ",
                          sc.x, sc.y, h);
    se_sdf_string_append(out, "vec2 cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot(k2, k2), 0.0, 1.0); ");
    se_sdf_string_append(out, "float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0; ");
    se_sdf_string_append(out, "s * sqrt(min(dot(ca, ca), dot(cb, cb))); }");
}

void se_sdf_gen_cone_inf(se_sdf_string* out, const char* p, s_vec2 sc) {
    se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz), -%s.y); ", p, p);
    se_sdf_string_append(out, "vec2 w = q - vec2(%.6f, %.6f) * max(dot(q, vec2(%.6f, %.6f)), 0.0); ",
                          sc.x, sc.y, sc.x, sc.y);
    se_sdf_string_append(out, "float d = length(w); ");
    se_sdf_string_append(out, "d * ((q.x * %.6f - q.y * %.6f < 0.0) ? -1.0 : 1.0); }", sc.y, sc.x);
}

void se_sdf_gen_plane(se_sdf_string* out, const char* p, s_vec3 n, f32 h) {
    se_sdf_string_append(out, "(dot(%s, vec3(%.6f, %.6f, %.6f)) + %.6f)",
                          p, n.x, n.y, n.z, h);
}

void se_sdf_gen_hex_prism(se_sdf_string* out, const char* p, s_vec2 h) {
    se_sdf_string_append(out, "{ const vec3 k = vec3(-0.8660254, 0.5, 0.57735); ");
    se_sdf_string_append(out, "vec3 pp = abs(%s); ", p);
    se_sdf_string_append(out, "pp.xy -= 2.0 * min(dot(k.xy, pp.xy), 0.0) * k.xy; ");
    se_sdf_string_append(out, "vec2 d = vec2(length(pp.xy - vec2(clamp(pp.x, -k.z * %.6f, k.z * %.6f), %.6f)) * sign(pp.y - %.6f), pp.z - %.6f); ",
                          h.x, h.x, h.x, h.x, h.y);
    se_sdf_string_append(out, "min(max(d.x, d.y), 0.0) + length(max(d, 0.0)); }");
}

void se_sdf_gen_capsule(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r) {
    se_sdf_string_append(out, "{ vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0); ");
    se_sdf_string_append(out, "length(pa - ba * h) - %.6f; }", r);
}

void se_sdf_gen_vert_capsule(se_sdf_string* out, const char* p, f32 h, f32 r) {
    se_sdf_string_append(out, "{ vec3 q = %s; q.y -= clamp(q.y, 0.0, %.6f); length(q) - %.6f; }",
                          p, h, r);
}

void se_sdf_gen_capped_cyl(se_sdf_string* out, const char* p, f32 r, f32 hh) {
    se_sdf_string_append(out, "{ vec2 d = abs(vec2(length(%s.xz), %s.y)) - vec2(%.6f, %.6f); ",
                          p, p, r, hh);
    se_sdf_string_append(out, "min(max(d.x, d.y), 0.0) + length(max(d, 0.0)); }");
}

void se_sdf_gen_capped_cyl_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r) {
    se_sdf_string_append(out, "{ vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float baba = dot(ba, ba); float paba = dot(pa, ba); ");
    se_sdf_string_append(out, "float x = length(pa * baba - ba * paba) - %.6f * baba; ", r);
    se_sdf_string_append(out, "float y = abs(paba - baba * 0.5) - baba * 0.5; ");
    se_sdf_string_append(out, "float x2 = x * x; float y2 = y * y * baba; ");
    se_sdf_string_append(out, "float d = (max(x, y) < 0.0) ? -min(x2, y2) : (((x > 0.0) ? x2 : 0.0) + ((y > 0.0) ? y2 : 0.0)); ");
    se_sdf_string_append(out, "sign(d) * sqrt(abs(d)) / baba; }");
}

void se_sdf_gen_rounded_cyl(se_sdf_string* out, const char* p, f32 ra, f32 rb, f32 hh) {
    se_sdf_string_append(out, "{ vec2 d = vec2(length(%s.xz) - %.6f + %.6f, abs(%s.y) - %.6f + %.6f); ",
                          p, ra, rb, p, hh, rb);
    se_sdf_string_append(out, "min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - %.6f; }", rb);
}

void se_sdf_gen_capped_cone(se_sdf_string* out, const char* p, f32 h, f32 r1, f32 r2) {
    se_sdf_gen_cone(out, p, (s_vec2){r1, h}, h); // Simplified
}

void se_sdf_gen_capped_cone_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 ra, f32 rb) {
    se_sdf_string_append(out, "{ float rba = %.6f - %.6f; ", rb, ra);
    se_sdf_string_append(out, "vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float baba = dot(ba, ba); vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float paba = dot(pa, ba) / baba; ");
    se_sdf_string_append(out, "float x = length(pa - ba * paba); ");
    se_sdf_string_append(out, "float cax = max(0.0, x - ((paba < 0.5) ? %.6f : %.6f)); ", ra, rb);
    se_sdf_string_append(out, "float cay = abs(paba - 0.5) - 0.5; ");
    se_sdf_string_append(out, "float k = rba * rba + baba; ");
    se_sdf_string_append(out, "float f = clamp((rba * (x - %.6f) + paba * baba) / k, 0.0, 1.0); ", ra);
    se_sdf_string_append(out, "float cbx = x - %.6f - f * rba; ", ra);
    se_sdf_string_append(out, "float cby = paba - f; ");
    se_sdf_string_append(out, "float s = (cbx < 0.0 && cay < 0.0) ? -1.0 : 1.0; ");
    se_sdf_string_append(out, "s * sqrt(min(cax * cax + cay * cay * baba, cbx * cbx + cby * cby * baba)); }");
}

void se_sdf_gen_solid_angle(se_sdf_string* out, const char* p, s_vec2 sc, f32 r) {
    se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "float l = length(q) - %.6f; ", r);
    se_sdf_string_append(out, "float m = length(q - vec2(%.6f, %.6f) * clamp(dot(q, vec2(%.6f, %.6f)), 0.0, %.6f)); ",
                          sc.x, sc.y, sc.x, sc.y, r);
    se_sdf_string_append(out, "max(l, m * sign(%.6f * q.x - %.6f * q.y)); }", sc.y, sc.x);
}

void se_sdf_gen_cut_sphere(se_sdf_string* out, const char* p, f32 r, f32 ch) {
    se_sdf_string_append(out, "{ float w = sqrt(%.6f * %.6f - %.6f * %.6f); ", r, r, ch, ch);
    se_sdf_string_append(out, "vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "float s = max((%.6f - %.6f) * q.x * q.x + w * w * (%.6f + %.6f - 2.0 * q.y), %.6f * q.x - w * q.y); ",
                          ch, r, ch, r, ch);
    se_sdf_string_append(out, "(s < 0.0) ? length(q) - %.6f : (q.x < w) ? %.6f - q.y : length(q - vec2(w, %.6f)); }",
                          r, ch, ch);
}

void se_sdf_gen_cut_hollow_sphere(se_sdf_string* out, const char* p, f32 r, f32 ch, f32 t) {
    se_sdf_string_append(out, "{ float w = sqrt(%.6f * %.6f - %.6f * %.6f); ", r, r, ch, ch);
    se_sdf_string_append(out, "vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "((%.6f * q.x < w * q.y) ? length(q - vec2(w, %.6f)) : abs(length(q) - %.6f)) - %.6f; }",
                          ch, ch, r, t);
}

void se_sdf_gen_death_star(se_sdf_string* out, const char* p, f32 ra, f32 rb, f32 d) {
    se_sdf_string_append(out, "{ float a = (%.6f * %.6f - %.6f * %.6f + %.6f * %.6f) / (2.0 * %.6f); ",
                          ra, ra, rb, rb, d, d, d);
    se_sdf_string_append(out, "float b = sqrt(max(%.6f * %.6f - a * a, 0.0)); ", ra, ra);
    se_sdf_string_append(out, "vec2 pp = vec2(%s.x, length(%s.yz)); ", p, p);
    se_sdf_string_append(out, "float ds = (pp.x * b - pp.y * a > %.6f * max(b - pp.y, 0.0)) ? ",
                          ra);
    se_sdf_string_append(out, "length(pp - vec2(a, b)) : max(length(pp) - %.6f, -(length(pp - vec2(%.6f, 0.0)) - %.6f)); ",
                          ra, d, rb);
    se_sdf_string_append(out, "ds; }");
}

void se_sdf_gen_round_cone(se_sdf_string* out, const char* p, f32 r1, f32 r2, f32 h) {
    se_sdf_string_append(out, "{ float b = (%.6f - %.6f) / %.6f; ", r1, r2, h);
    se_sdf_string_append(out, "float a = sqrt(1.0 - b * b); ");
    se_sdf_string_append(out, "vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "float k = dot(q, vec2(-b, a)); ");
    se_sdf_string_append(out, "float d = (k < 0.0) ? (length(q) - %.6f) : ", r1);
    se_sdf_string_append(out, "((k > a * %.6f) ? (length(q - vec2(0.0, %.6f)) - %.6f) : (dot(q, vec2(a, b)) - %.6f)); ",
                          h, h, r2, r1);
    se_sdf_string_append(out, "d; }");
}

void se_sdf_gen_round_cone_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r1, f32 r2) {
    se_sdf_string_append(out, "{ vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float l2 = dot(ba, ba); float rr = %.6f - %.6f; float a2 = l2 - rr * rr; float il2 = 1.0 / l2; ",
                          r1, r2);
    se_sdf_string_append(out, "vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float y = dot(pa, ba); float z = y - l2; ");
    se_sdf_string_append(out, "float x2 = dot(pa * l2 - ba * y, pa * l2 - ba * y); ");
    se_sdf_string_append(out, "float y2 = y * y * l2; float z2 = z * z * l2; ");
    se_sdf_string_append(out, "float k = sign(rr) * rr * rr * x2; ");
    se_sdf_string_append(out, "float d = (sign(z) * a2 * z2 > k) ? (sqrt(x2 + z2) * il2 - %.6f) : ",
                          r2);
    se_sdf_string_append(out, "((sign(y) * a2 * y2 < k) ? (sqrt(x2 + y2) * il2 - %.6f) : ((sqrt(x2 * a2 * il2) + y * rr) * il2 - %.6f)); ",
                          r1, r1);
    se_sdf_string_append(out, "d; }");
}

void se_sdf_gen_vesica(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 w) {
    se_sdf_string_append(out, "{ vec3 c = (");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, " + ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, ") * 0.5; float l = length(");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "); vec3 v = (");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, ") / l; float y = dot(%s - c, v); vec2 q = vec2(length(%s - c - y * v), abs(y)); ",
                          p, p);
    se_sdf_string_append(out, "float r = 0.5 * l; float d = 0.5 * (r * r - %.6f * %.6f) / %.6f; ", w, w, w);
    se_sdf_string_append(out, "vec3 h = (r * q.x < d * (q.y - r)) ? vec3(0.0, r, 0.0) : vec3(-d, 0.0, d + %.6f); ", w);
    se_sdf_string_append(out, "length(q - h.xy) - h.z; }");
}

void se_sdf_gen_rhombus(se_sdf_string* out, const char* p, f32 la, f32 lb, f32 h, f32 r) {
    se_sdf_string_append(out, "{ vec3 pp = abs(%s); ", p);
    se_sdf_string_append(out, "float f = clamp((%.6f * pp.x - %.6f * pp.z + %.6f * %.6f) / (%.6f * %.6f + %.6f * %.6f), 0.0, 1.0); ",
                          la, lb, lb, lb, la, la, lb, lb);
    se_sdf_string_append(out, "vec2 w = pp.xz - vec2(%.6f, %.6f) * vec2(f, 1.0 - f); ", la, lb);
    se_sdf_string_append(out, "vec2 q = vec2(length(w) * sign(w.x) - %.6f, pp.y - %.6f); ", r, h);
    se_sdf_string_append(out, "min(max(q.x, q.y), 0.0) + length(max(q, 0.0)); }");
}

void se_sdf_gen_octa(se_sdf_string* out, const char* p, f32 s) {
    se_sdf_string_append(out, "{ vec3 pp = abs(%s); ", p);
    se_sdf_string_append(out, "float m = pp.x + pp.y + pp.z - %.6f; ", s);
    se_sdf_string_append(out, "vec3 q = vec3(0.0); ");
    se_sdf_string_append(out, "if (3.0 * pp.x < m) q = pp.xyz; else if (3.0 * pp.y < m) q = pp.yzx; ");
    se_sdf_string_append(out, "else if (3.0 * pp.z < m) q = pp.zxy; ");
    se_sdf_string_append(out, "float k = clamp(0.5 * (q.z - q.y + %.6f), 0.0, %.6f); ",
                          s, s);
    se_sdf_string_append(out, "float d = ((3.0 * pp.x < m) || (3.0 * pp.y < m) || (3.0 * pp.z < m)) ? ");
    se_sdf_string_append(out, "length(vec3(q.x, q.y - %.6f + k, q.z - k)) : (m * 0.57735027); ", s);
    se_sdf_string_append(out, "d; }");
}

void se_sdf_gen_octa_bound(se_sdf_string* out, const char* p, f32 s) {
    se_sdf_string_append(out, "{ vec3 pp = abs(%s); (pp.x + pp.y + pp.z - %.6f) * 0.57735027; }",
                          p, s);
}

void se_sdf_gen_pyramid(se_sdf_string* out, const char* p, f32 h) {
    se_sdf_string_append(out, "{ float m2 = %.6f * %.6f + 0.25; ", h, h);
    se_sdf_string_append(out, "vec3 pp = %s; pp.xz = abs(pp.xz); ", p);
    se_sdf_string_append(out, "if (pp.z > pp.x) pp.xz = pp.zx; pp.xz -= 0.5; ");
    se_sdf_string_append(out, "vec3 q = vec3(pp.z, %.6f * pp.y - 0.5 * pp.x, %.6f * pp.x + 0.5 * pp.y); ", h, h);
    se_sdf_string_append(out, "float s = max(-q.x, 0.0); ");
    se_sdf_string_append(out, "float t = clamp((q.y - 0.5 * pp.z) / (m2 + 0.25), 0.0, 1.0); ");
    se_sdf_string_append(out, "float a = m2 * (q.x + s) * (q.x + s) + q.y * q.y; ");
    se_sdf_string_append(out, "float b = m2 * (q.x + 0.5 * t) * (q.x + 0.5 * t) + (q.y - m2 * t) * (q.y - m2 * t); ");
    se_sdf_string_append(out, "float d2 = (min(q.y, -q.x * m2 - q.y * 0.5) > 0.0) ? 0.0 : min(a, b); ");
    se_sdf_string_append(out, "sqrt((d2 + q.z * q.z) / m2) * sign(max(q.z, -pp.y)); }");
}

void se_sdf_gen_tri(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, s_vec3 c) {
    se_sdf_string_append(out, "{ vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 cb = ");
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, "; vec3 pb = %s - ", p);
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, "; vec3 ac = ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, "; vec3 pc = %s - ", p);
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, "; vec3 nor = cross(ba, ac); ");
    se_sdf_string_append(out, "sqrt((sign(dot(cross(ba, nor), pa)) + sign(dot(cross(cb, nor), pb)) + ");
    se_sdf_string_append(out, "sign(dot(cross(ac, nor), pc)) < 2.0) ? min(min(");
    se_sdf_string_append(out, "dot(ba * clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa, ba * clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa), ");
    se_sdf_string_append(out, "dot(cb * clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb, cb * clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb)), ");
    se_sdf_string_append(out, "dot(ac * clamp(dot(ac, pc) / dot(ac, ac), 0.0, 1.0) - pc, ac * clamp(dot(ac, pc) / dot(ac, ac), 0.0, 1.0) - pc)) ");
    se_sdf_string_append(out, ": dot(nor, pa) * dot(nor, pa) / dot(nor, nor)); }");
}

void se_sdf_gen_quad(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, s_vec3 c, s_vec3 d) {
    se_sdf_string_append(out, "{ vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 cb = ");
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, "; vec3 pb = %s - ", p);
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, "; vec3 dc = ");
    se_sdf_string_append_vec3(out, d);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, "; vec3 pc = %s - ", p);
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, "; vec3 ad = ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, d);
    se_sdf_string_append(out, "; vec3 pd = %s - ", p);
    se_sdf_string_append_vec3(out, d);
    se_sdf_string_append(out, "; vec3 nor = cross(ba, ad); ");
    se_sdf_string_append(out, "sqrt((sign(dot(cross(ba, nor), pa)) + sign(dot(cross(cb, nor), pb)) + ");
    se_sdf_string_append(out, "sign(dot(cross(dc, nor), pc)) + sign(dot(cross(ad, nor), pd)) < 3.0) ? ");
    se_sdf_string_append(out, "min(min(min(dot(ba * clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa, ba * clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa), ");
    se_sdf_string_append(out, "dot(cb * clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb, cb * clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb)), ");
    se_sdf_string_append(out, "dot(dc * clamp(dot(dc, pc) / dot(dc, dc), 0.0, 1.0) - pc, dc * clamp(dot(dc, pc) / dot(dc, dc), 0.0, 1.0) - pc)), ");
    se_sdf_string_append(out, "dot(ad * clamp(dot(ad, pd) / dot(ad, ad), 0.0, 1.0) - pd, ad * clamp(dot(ad, pd) / dot(ad, ad), 0.0, 1.0) - pd)) ");
    se_sdf_string_append(out, ": dot(nor, pa) * dot(nor, pa) / dot(nor, nor)); }");
}

// ============================================================================
// Transform & Operations
// ============================================================================

void se_sdf_gen_transform(se_sdf_string* out, const char* p_var, s_mat4 transform, const char* result_var) {
    se_sdf_string_append(out, "vec3 %s = (inverse(mat4(", result_var);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            se_sdf_string_append(out, "%.6f", transform.m[i][j]);
            if (i != 3 || j != 3) se_sdf_string_append(out, ", ");
        }
    }
    se_sdf_string_append(out, ")) * vec4(%s, 1.0)).xyz;", p_var);
}

void se_sdf_gen_operation(se_sdf_string* out, se_sdf_operation op, const char* d1, const char* d2) {
	const f32 smooth_amount = se_sdf_sanitize_operation_amount(op, SE_SDF_OPERATION_AMOUNT_DEFAULT);
	switch (op) {
		case SE_SDF_OP_UNION:
			se_sdf_string_append(out, "min(%s, %s)", d1, d2);
			break;
        case SE_SDF_OP_INTERSECTION:
            se_sdf_string_append(out, "max(%s, %s)", d1, d2);
            break;
		case SE_SDF_OP_SUBTRACTION:
			se_sdf_string_append(out, "max(%s, -%s)", d1, d2);
			break;
		case SE_SDF_OP_SMOOTH_UNION:
			se_sdf_string_append(
				out,
				"(mix(%s, %s, clamp(0.5 + 0.5 * ((%s) - (%s)) / %.6f, 0.0, 1.0)) - %.6f * clamp(0.5 + 0.5 * ((%s) - (%s)) / %.6f, 0.0, 1.0) * (1.0 - clamp(0.5 + 0.5 * ((%s) - (%s)) / %.6f, 0.0, 1.0)))",
				d2, d1,
				d2, d1, smooth_amount,
				smooth_amount,
				d2, d1, smooth_amount,
				d2, d1, smooth_amount
			);
			break;
		case SE_SDF_OP_SMOOTH_INTERSECTION:
			se_sdf_string_append(
				out,
				"(mix(%s, %s, clamp(0.5 - 0.5 * ((%s) - (%s)) / %.6f, 0.0, 1.0)) + %.6f * clamp(0.5 - 0.5 * ((%s) - (%s)) / %.6f, 0.0, 1.0) * (1.0 - clamp(0.5 - 0.5 * ((%s) - (%s)) / %.6f, 0.0, 1.0)))",
				d2, d1,
				d2, d1, smooth_amount,
				smooth_amount,
				d2, d1, smooth_amount,
				d2, d1, smooth_amount
			);
			break;
		case SE_SDF_OP_SMOOTH_SUBTRACTION:
			se_sdf_string_append(
				out,
				"(mix(%s, -%s, clamp(0.5 - 0.5 * ((%s) + (%s)) / %.6f, 0.0, 1.0)) + %.6f * clamp(0.5 - 0.5 * ((%s) + (%s)) / %.6f, 0.0, 1.0) * (1.0 - clamp(0.5 - 0.5 * ((%s) + (%s)) / %.6f, 0.0, 1.0)))",
				d1, d2,
				d2, d1, smooth_amount,
				smooth_amount,
				d2, d1, smooth_amount,
				d2, d1, smooth_amount
			);
			break;
		default:
			se_sdf_string_append(out, "%s", d1);
			break;
	}
}

// ============================================================================
// Recursive Generator
// ============================================================================

static sz se_sdf_count_children(se_sdf_object* obj) {
    return s_array_get_size(&obj->children);
}

static se_sdf_object* se_sdf_get_child(se_sdf_object* obj, sz index) {
    s_handle h = s_array_handle(&obj->children, (u32)index);
    return s_array_get(&obj->children, h);
}

static void se_sdf_emit_indent(se_sdf_string* out, u32 indent) {
    for (u32 i = 0; i < indent; ++i) {
        se_sdf_string_append(out, "    ");
    }
}

static b8 se_sdf_is_identity(const s_mat4* transform) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if ((i == j && transform->m[i][j] != 1.0f) ||
                (i != j && transform->m[i][j] != 0.0f)) {
                return 0;
            }
        }
    }
    return 1;
}

static void se_sdf_trim_bounds(const char* text, sz* start, sz* end) {
    while (*start < *end && isspace((unsigned char)text[*start])) (*start)++;
    while (*end > *start && isspace((unsigned char)text[*end - 1])) (*end)--;
}

static b8 se_sdf_extract_block_parts(
    const char* text, sz size,
    sz* prefix_start, sz* prefix_end,
    sz* expr_start, sz* expr_end
) {
    sz start = 0;
    sz end = size;
    se_sdf_trim_bounds(text, &start, &end);
    if (start >= end || text[start] != '{' || text[end - 1] != '}') return 0;

    sz body_start = start + 1;
    sz body_end = end - 1;
    se_sdf_trim_bounds(text, &body_start, &body_end);
    if (body_start >= body_end) return 0;

    sz last_semicolon = body_end;
    sz prev_semicolon = body_end;
    int brace_depth = 0;
    for (sz i = body_start; i < body_end; ++i) {
        char c = text[i];
        if (c == '{') {
            brace_depth++;
            continue;
        }
        if (c == '}') {
            if (brace_depth > 0) brace_depth--;
            continue;
        }
        if (c == ';' && brace_depth == 0) {
            prev_semicolon = last_semicolon;
            last_semicolon = i;
        }
    }

    if (last_semicolon == body_end) return 0;
    sz last_stmt_start = (prev_semicolon == body_end) ? body_start : (prev_semicolon + 1);

    *prefix_start = body_start;
    *prefix_end = last_stmt_start;
    *expr_start = last_stmt_start;
    *expr_end = last_semicolon;

    se_sdf_trim_bounds(text, prefix_start, prefix_end);
    se_sdf_trim_bounds(text, expr_start, expr_end);
    return *expr_start < *expr_end;
}

static const char* se_sdf_codegen_get_node_bind_uniform(
	const se_sdf_object* obj,
	const se_sdf_node_bind_target target
) {
	if (!obj || !se_sdf_codegen_active_renderer) {
		return NULL;
	}
	if (obj->source_scene == SE_SDF_SCENE_NULL || obj->source_node == SE_SDF_NODE_NULL) {
		return NULL;
	}
	return se_sdf_find_node_binding_uniform_name(
		se_sdf_codegen_active_renderer,
		obj->source_scene,
		obj->source_node,
		target
	);
}

static const char* se_sdf_codegen_get_primitive_bind_uniform(
	const se_sdf_object* obj,
	const se_sdf_primitive_param param
) {
	if (!obj || !se_sdf_codegen_active_renderer) {
		return NULL;
	}
	if (obj->source_scene == SE_SDF_SCENE_NULL || obj->source_node == SE_SDF_NODE_NULL) {
		return NULL;
	}
	return se_sdf_find_primitive_binding_uniform_name(
		se_sdf_codegen_active_renderer,
		obj->source_scene,
		obj->source_node,
		param
	);
}

static void se_sdf_codegen_write_scalar_expr(
	char* out,
	const sz out_size,
	const f32 fallback_value,
	const char* uniform_name
) {
	if (!out || out_size == 0) {
		return;
	}
	if (uniform_name && uniform_name[0] != '\0') {
		snprintf(out, out_size, "%s", uniform_name);
		return;
	}
	snprintf(out, out_size, "%.6f", fallback_value);
}

static void se_sdf_emit_leaf_raw(se_sdf_string* out, se_sdf_object* obj, const char* p_var) {
    switch (obj->type) {
        case SE_SDF_SPHERE:
        {
            const char* radius_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_RADIUS);
            if (radius_uniform) {
                se_sdf_string_append(out, "(length(%s) - %s)", p_var, radius_uniform);
            } else {
                se_sdf_gen_sphere(out, p_var, obj->sphere.radius);
            }
            break;
        }
        case SE_SDF_BOX:
        {
            const char* sx_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_SIZE_X);
            const char* sy_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_SIZE_Y);
            const char* sz_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_SIZE_Z);
            if (sx_uniform || sy_uniform || sz_uniform) {
                char sx[96];
                char sy[96];
                char sz_val[96];
                se_sdf_codegen_write_scalar_expr(sx, sizeof(sx), obj->box.size.x, sx_uniform);
                se_sdf_codegen_write_scalar_expr(sy, sizeof(sy), obj->box.size.y, sy_uniform);
                se_sdf_codegen_write_scalar_expr(sz_val, sizeof(sz_val), obj->box.size.z, sz_uniform);
                se_sdf_string_append(
                    out,
                    "{ vec3 q = abs(%s) - vec3(%s, %s, %s); length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); }",
                    p_var,
                    sx,
                    sy,
                    sz_val
                );
            } else {
                se_sdf_gen_box(out, p_var, obj->box.size);
            }
            break;
        }
        case SE_SDF_ROUND_BOX:
        {
            const char* sx_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_SIZE_X);
            const char* sy_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_SIZE_Y);
            const char* sz_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_SIZE_Z);
            const char* r_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_RADIUS);
            if (sx_uniform || sy_uniform || sz_uniform || r_uniform) {
                char sx[96];
                char sy[96];
                char sz_val[96];
                char r[96];
                se_sdf_codegen_write_scalar_expr(sx, sizeof(sx), obj->round_box.size.x, sx_uniform);
                se_sdf_codegen_write_scalar_expr(sy, sizeof(sy), obj->round_box.size.y, sy_uniform);
                se_sdf_codegen_write_scalar_expr(sz_val, sizeof(sz_val), obj->round_box.size.z, sz_uniform);
                se_sdf_codegen_write_scalar_expr(r, sizeof(r), obj->round_box.roundness, r_uniform);
                se_sdf_string_append(
                    out,
                    "{ vec3 q = abs(%s) - vec3(%s, %s, %s) + vec3(%s); length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - %s; }",
                    p_var,
                    sx,
                    sy,
                    sz_val,
                    r,
                    r
                );
            } else {
                se_sdf_gen_round_box(out, p_var, obj->round_box.size, obj->round_box.roundness);
            }
            break;
        }
        case SE_SDF_BOX_FRAME:
            se_sdf_gen_box_frame(out, p_var, obj->box_frame.size, obj->box_frame.thickness);
            break;
        case SE_SDF_TORUS:
            se_sdf_gen_torus(out, p_var, obj->torus.radii.x, obj->torus.radii.y);
            break;
        case SE_SDF_CAPPED_TORUS:
            se_sdf_gen_capped_torus(out, p_var, obj->capped_torus.axis_sin_cos,
                                    obj->capped_torus.major_radius, obj->capped_torus.minor_radius);
            break;
        case SE_SDF_LINK:
            se_sdf_gen_link(out, p_var, obj->link.half_length,
                            obj->link.outer_radius, obj->link.inner_radius);
            break;
        case SE_SDF_CYLINDER:
            se_sdf_gen_cylinder(out, p_var, obj->cylinder.axis_and_radius);
            break;
        case SE_SDF_CONE:
        {
            const char* height_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_HEIGHT);
            if (height_uniform) {
                se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz), %s.y); ", p_var, p_var);
                se_sdf_string_append(out, "vec2 k1 = vec2(%.6f, %s); ", obj->cone.angle_sin_cos.y, height_uniform);
                se_sdf_string_append(out, "vec2 k2 = vec2(%.6f - %.6f, 2.0 * %s); ", obj->cone.angle_sin_cos.y, obj->cone.angle_sin_cos.x, height_uniform);
                se_sdf_string_append(out, "vec2 ca = vec2(q.x - min(q.x, (q.y < 0.0) ? %.6f : %.6f), abs(q.y) - %s); ",
                                     obj->cone.angle_sin_cos.x, obj->cone.angle_sin_cos.y, height_uniform);
                se_sdf_string_append(out, "vec2 cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot(k2, k2), 0.0, 1.0); ");
                se_sdf_string_append(out, "float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0; ");
                se_sdf_string_append(out, "s * sqrt(min(dot(ca, ca), dot(cb, cb))); }");
            } else {
                se_sdf_gen_cone(out, p_var, obj->cone.angle_sin_cos, obj->cone.height);
            }
            break;
        }
        case SE_SDF_CONE_INFINITE:
            se_sdf_gen_cone_inf(out, p_var, obj->cone_infinite.angle_sin_cos);
            break;
        case SE_SDF_PLANE:
            se_sdf_gen_plane(out, p_var, obj->plane.normal, obj->plane.offset);
            break;
        case SE_SDF_HEX_PRISM:
            se_sdf_gen_hex_prism(out, p_var, obj->hex_prism.half_size);
            break;
        case SE_SDF_CAPSULE:
        {
            const char* radius_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_RADIUS);
            if (radius_uniform) {
                se_sdf_string_append(out, "{ vec3 pa = %s - ", p_var);
                se_sdf_string_append_vec3(out, obj->capsule.a);
                se_sdf_string_append(out, "; vec3 ba = ");
                se_sdf_string_append_vec3(out, obj->capsule.b);
                se_sdf_string_append(out, " - ");
                se_sdf_string_append_vec3(out, obj->capsule.a);
                se_sdf_string_append(out, "; float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0); ");
                se_sdf_string_append(out, "length(pa - ba * h) - %s; }", radius_uniform);
            } else {
                se_sdf_gen_capsule(out, p_var, obj->capsule.a, obj->capsule.b, obj->capsule.radius);
            }
            break;
        }
        case SE_SDF_VERTICAL_CAPSULE:
        {
            const char* height_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_HEIGHT);
            const char* radius_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_RADIUS);
            if (height_uniform || radius_uniform) {
                char h[96];
                char r[96];
                se_sdf_codegen_write_scalar_expr(h, sizeof(h), obj->vertical_capsule.height, height_uniform);
                se_sdf_codegen_write_scalar_expr(r, sizeof(r), obj->vertical_capsule.radius, radius_uniform);
                se_sdf_string_append(out, "{ vec3 q = %s; q.y -= clamp(q.y, 0.0, %s); length(q) - %s; }", p_var, h, r);
            } else {
                se_sdf_gen_vert_capsule(out, p_var, obj->vertical_capsule.height, obj->vertical_capsule.radius);
            }
            break;
        }
        case SE_SDF_CAPPED_CYLINDER:
        {
            const char* radius_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_RADIUS);
            const char* height_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_HEIGHT);
            if (radius_uniform || height_uniform) {
                char r[96];
                char h[96];
                se_sdf_codegen_write_scalar_expr(r, sizeof(r), obj->capped_cylinder.radius, radius_uniform);
                se_sdf_codegen_write_scalar_expr(h, sizeof(h), obj->capped_cylinder.half_height, height_uniform);
                se_sdf_string_append(out, "{ vec2 d = abs(vec2(length(%s.xz), %s.y)) - vec2(%s, %s); ", p_var, p_var, r, h);
                se_sdf_string_append(out, "min(max(d.x, d.y), 0.0) + length(max(d, 0.0)); }");
            } else {
                se_sdf_gen_capped_cyl(out, p_var, obj->capped_cylinder.radius, obj->capped_cylinder.half_height);
            }
            break;
        }
        case SE_SDF_CAPPED_CYLINDER_ARBITRARY:
            se_sdf_gen_capped_cyl_arb(out, p_var, obj->capped_cylinder_arbitrary.a,
                                      obj->capped_cylinder_arbitrary.b, obj->capped_cylinder_arbitrary.radius);
            break;
        case SE_SDF_ROUNDED_CYLINDER:
            se_sdf_gen_rounded_cyl(out, p_var, obj->rounded_cylinder.outer_radius,
                                   obj->rounded_cylinder.corner_radius, obj->rounded_cylinder.half_height);
            break;
        case SE_SDF_CAPPED_CONE:
            se_sdf_gen_capped_cone(out, p_var, obj->capped_cone.height,
                                   obj->capped_cone.radius_base, obj->capped_cone.radius_top);
            break;
        case SE_SDF_CAPPED_CONE_ARBITRARY:
            se_sdf_gen_capped_cone_arb(out, p_var, obj->capped_cone_arbitrary.a,
                                       obj->capped_cone_arbitrary.b, obj->capped_cone_arbitrary.radius_a,
                                       obj->capped_cone_arbitrary.radius_b);
            break;
        case SE_SDF_SOLID_ANGLE:
            se_sdf_gen_solid_angle(out, p_var, obj->solid_angle.angle_sin_cos, obj->solid_angle.radius);
            break;
        case SE_SDF_CUT_SPHERE:
        {
            const char* radius_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_RADIUS);
            const char* height_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_HEIGHT);
            if (radius_uniform || height_uniform) {
                char r[96];
                char h[96];
                se_sdf_codegen_write_scalar_expr(r, sizeof(r), obj->cut_sphere.radius, radius_uniform);
                se_sdf_codegen_write_scalar_expr(h, sizeof(h), obj->cut_sphere.cut_height, height_uniform);
                se_sdf_string_append(out, "{ float w = sqrt(%s * %s - %s * %s); ", r, r, h, h);
                se_sdf_string_append(out, "vec2 q = vec2(length(%s.xz), %s.y); ", p_var, p_var);
                se_sdf_string_append(out, "float s = max((%s - %s) * q.x * q.x + w * w * (%s + %s - 2.0 * q.y), %s * q.x - w * q.y); ",
                                     h, r, h, r, h);
                se_sdf_string_append(out, "(s < 0.0) ? length(q) - %s : (q.x < w) ? %s - q.y : length(q - vec2(w, %s)); }", r, h, h);
            } else {
                se_sdf_gen_cut_sphere(out, p_var, obj->cut_sphere.radius, obj->cut_sphere.cut_height);
            }
            break;
        }
        case SE_SDF_CUT_HOLLOW_SPHERE:
            se_sdf_gen_cut_hollow_sphere(out, p_var, obj->cut_hollow_sphere.radius,
                                         obj->cut_hollow_sphere.cut_height, obj->cut_hollow_sphere.thickness);
            break;
        case SE_SDF_DEATH_STAR:
            se_sdf_gen_death_star(out, p_var, obj->death_star.radius_a,
                                  obj->death_star.radius_b, obj->death_star.separation);
            break;
        case SE_SDF_ROUND_CONE:
        {
            const char* radius_a_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_RADIUS_A);
            const char* radius_b_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_RADIUS_B);
            const char* height_uniform = se_sdf_codegen_get_primitive_bind_uniform(obj, SE_SDF_PRIMITIVE_PARAM_HEIGHT);
            if (radius_a_uniform || radius_b_uniform || height_uniform) {
                char r1[96];
                char r2[96];
                char h[96];
                se_sdf_codegen_write_scalar_expr(r1, sizeof(r1), obj->round_cone.radius_base, radius_a_uniform);
                se_sdf_codegen_write_scalar_expr(r2, sizeof(r2), obj->round_cone.radius_top, radius_b_uniform);
                se_sdf_codegen_write_scalar_expr(h, sizeof(h), obj->round_cone.height, height_uniform);
                se_sdf_string_append(out, "{ float b = (%s - %s) / %s; ", r1, r2, h);
                se_sdf_string_append(out, "float a = sqrt(1.0 - b * b); ");
                se_sdf_string_append(out, "vec2 q = vec2(length(%s.xz), %s.y); ", p_var, p_var);
                se_sdf_string_append(out, "float k = dot(q, vec2(-b, a)); ");
                se_sdf_string_append(out, "float d = (k < 0.0) ? (length(q) - %s) : ", r1);
                se_sdf_string_append(out, "((k > a * %s) ? (length(q - vec2(0.0, %s)) - %s) : (dot(q, vec2(a, b)) - %s)); ", h, h, r2, r1);
                se_sdf_string_append(out, "d; }");
            } else {
                se_sdf_gen_round_cone(out, p_var, obj->round_cone.radius_base,
                                      obj->round_cone.radius_top, obj->round_cone.height);
            }
            break;
        }
        case SE_SDF_ROUND_CONE_ARBITRARY:
            se_sdf_gen_round_cone_arb(out, p_var, obj->round_cone_arbitrary.a,
                                      obj->round_cone_arbitrary.b, obj->round_cone_arbitrary.radius_a,
                                      obj->round_cone_arbitrary.radius_b);
            break;
        case SE_SDF_VESICA_SEGMENT:
            se_sdf_gen_vesica(out, p_var, obj->vesica_segment.a,
                              obj->vesica_segment.b, obj->vesica_segment.width);
            break;
        case SE_SDF_RHOMBUS:
            se_sdf_gen_rhombus(out, p_var, obj->rhombus.length_a, obj->rhombus.length_b,
                               obj->rhombus.height, obj->rhombus.corner_radius);
            break;
        case SE_SDF_OCTAHEDRON:
            se_sdf_gen_octa(out, p_var, obj->octahedron.size);
            break;
        case SE_SDF_OCTAHEDRON_BOUND:
            se_sdf_gen_octa_bound(out, p_var, obj->octahedron_bound.size);
            break;
        case SE_SDF_PYRAMID:
            se_sdf_gen_pyramid(out, p_var, obj->pyramid.height);
            break;
        case SE_SDF_TRIANGLE:
            se_sdf_gen_tri(out, p_var, obj->triangle.a, obj->triangle.b, obj->triangle.c);
            break;
        case SE_SDF_QUAD:
            se_sdf_gen_quad(out, p_var, obj->quad.a, obj->quad.b, obj->quad.c, obj->quad.d);
            break;
        case SE_SDF_NONE:
        default:
            se_sdf_string_append(out, "1e10");
            break;
    }
}

static void se_sdf_emit_leaf_eval(
    se_sdf_string* out, se_sdf_object* obj, const char* p_var,
    const char* result_var, u32 indent
) {
    se_sdf_string expr;
    se_sdf_string_init(&expr, SE_SDF_MAX_STRING_LENGTH * 4);
    if (!expr.data) {
        se_sdf_emit_indent(out, indent);
        se_sdf_string_append(out, "float %s = 1e10;\n", result_var);
        return;
    }

    se_sdf_emit_leaf_raw(&expr, obj, p_var);

    sz start = 0;
    sz end = expr.size;
    se_sdf_trim_bounds(expr.data, &start, &end);

    if (start < end && expr.data[start] == '{' && expr.data[end - 1] == '}') {
        sz prefix_start = 0, prefix_end = 0, value_start = 0, value_end = 0;
        if (!se_sdf_extract_block_parts(expr.data, expr.size, &prefix_start, &prefix_end, &value_start, &value_end)) {
            se_sdf_emit_indent(out, indent);
            se_sdf_string_append(out, "float %s = 1e10;\n", result_var);
            se_sdf_string_free(&expr);
            return;
        }

        se_sdf_emit_indent(out, indent);
        se_sdf_string_append(out, "float %s = 1e10;\n", result_var);
        se_sdf_emit_indent(out, indent);
        se_sdf_string_append(out, "{ ");
        if (prefix_end > prefix_start) {
            se_sdf_string_append_raw(out, expr.data + prefix_start, prefix_end - prefix_start);
            if (!isspace((unsigned char)expr.data[prefix_end - 1])) {
                se_sdf_string_append(out, " ");
            }
        }
        se_sdf_string_append(out, "%s = ", result_var);
        se_sdf_string_append_raw(out, expr.data + value_start, value_end - value_start);
        se_sdf_string_append(out, "; }\n");
    } else {
        se_sdf_emit_indent(out, indent);
        se_sdf_string_append(out, "float %s = ", result_var);
        if (end > start) {
            se_sdf_string_append_raw(out, expr.data + start, end - start);
        } else {
            se_sdf_string_append(out, "1e10");
        }
        se_sdf_string_append(out, ";\n");
    }

    se_sdf_string_free(&expr);
}

static void se_sdf_emit_operation(
	se_sdf_string* out, se_sdf_operation op, f32 operation_amount,
	const char* d1, const char* d2, u32 indent
) {
	const f32 smooth_amount = se_sdf_sanitize_operation_amount(op, operation_amount);
	se_sdf_emit_indent(out, indent);
	switch (op) {
		case SE_SDF_OP_UNION:
			se_sdf_string_append(out, "%s = min(%s, %s);\n", d1, d1, d2);
			break;
        case SE_SDF_OP_INTERSECTION:
            se_sdf_string_append(out, "%s = max(%s, %s);\n", d1, d1, d2);
            break;
		case SE_SDF_OP_SUBTRACTION:
			se_sdf_string_append(out, "%s = max(%s, -%s);\n", d1, d1, d2);
			break;
		case SE_SDF_OP_SMOOTH_UNION:
			se_sdf_string_append(
				out,
				"{ const float k = %.6f; float h = clamp(0.5 + 0.5 * ((%s) - (%s)) / k, 0.0, 1.0); %s = mix(%s, %s, h) - k * h * (1.0 - h); }\n",
				smooth_amount,
				d2, d1,
				d1, d2, d1
			);
			break;
		case SE_SDF_OP_SMOOTH_INTERSECTION:
			se_sdf_string_append(
				out,
				"{ const float k = %.6f; float h = clamp(0.5 - 0.5 * ((%s) - (%s)) / k, 0.0, 1.0); %s = mix(%s, %s, h) + k * h * (1.0 - h); }\n",
				smooth_amount,
				d2, d1,
				d1, d2, d1
			);
			break;
		case SE_SDF_OP_SMOOTH_SUBTRACTION:
			se_sdf_string_append(
				out,
				"{ const float k = %.6f; float h = clamp(0.5 - 0.5 * ((%s) + (%s)) / k, 0.0, 1.0); %s = mix(%s, -%s, h) + k * h * (1.0 - h); }\n",
				smooth_amount,
				d2, d1,
				d1, d1, d2
			);
			break;
		case SE_SDF_OP_NONE:
		default:
            break;
    }
}

static void se_sdf_emit_object_eval(
    se_sdf_string* out, se_sdf_object* obj, const char* p_var,
    const char* result_var, u32* temp_counter, u32 indent
) {
    const char* local_p = p_var;
    char transformed_p[32];
	const char* translation_uniform = se_sdf_codegen_get_node_bind_uniform(obj, SE_SDF_NODE_BIND_TRANSLATION);
	const char* rotation_uniform = se_sdf_codegen_get_node_bind_uniform(obj, SE_SDF_NODE_BIND_ROTATION);
	const char* scale_uniform = se_sdf_codegen_get_node_bind_uniform(obj, SE_SDF_NODE_BIND_SCALE);
	const b8 has_dynamic_trs = (translation_uniform != NULL) || (rotation_uniform != NULL) || (scale_uniform != NULL);

	if (has_dynamic_trs) {
		const s_vec3 base_translation = s_vec3(
			obj->transform.m[3][0],
			obj->transform.m[3][1],
			obj->transform.m[3][2]
		);
		const s_vec3 base_rotation = s_vec3(0.0f, 0.0f, 0.0f);
		const s_vec3 base_scale = s_vec3(
			se_sdf_abs_or_one(obj->transform.m[0][0]),
			se_sdf_abs_or_one(obj->transform.m[1][1]),
			se_sdf_abs_or_one(obj->transform.m[2][2])
		);
		const b8 has_non_identity_scale =
			(fabsf(base_scale.x - 1.0f) > 0.000001f) ||
			(fabsf(base_scale.y - 1.0f) > 0.000001f) ||
			(fabsf(base_scale.z - 1.0f) > 0.000001f);
		const b8 has_rotation =
			(rotation_uniform != NULL) ||
			(fabsf(base_rotation.x) > 0.000001f) ||
			(fabsf(base_rotation.y) > 0.000001f) ||
			(fabsf(base_rotation.z) > 0.000001f);

		snprintf(transformed_p, sizeof(transformed_p), "_p%u", (*temp_counter)++);
		se_sdf_emit_indent(out, indent);
		se_sdf_string_append(out, "vec3 %s = %s;\n", transformed_p, p_var);
		se_sdf_emit_indent(out, indent);
		if (translation_uniform) {
			se_sdf_string_append(out, "%s -= %s;\n", transformed_p, translation_uniform);
		} else {
			se_sdf_string_append(
				out,
				"%s -= vec3(%.6f, %.6f, %.6f);\n",
				transformed_p,
				base_translation.x,
				base_translation.y,
				base_translation.z
			);
		}
		if (scale_uniform || has_non_identity_scale) {
			se_sdf_emit_indent(out, indent);
			if (scale_uniform) {
				se_sdf_string_append(out, "%s /= max(abs(%s), vec3(0.00001));\n", transformed_p, scale_uniform);
			} else {
				se_sdf_string_append(
					out,
					"%s /= max(abs(vec3(%.6f, %.6f, %.6f)), vec3(0.00001));\n",
					transformed_p,
					base_scale.x,
					base_scale.y,
					base_scale.z
				);
			}
		}
		if (has_rotation) {
			char rot_var[32];
			char c_var[32];
			char s_var[32];
			snprintf(rot_var, sizeof(rot_var), "_r%u", (*temp_counter)++);
			snprintf(c_var, sizeof(c_var), "_c%u", (*temp_counter)++);
			snprintf(s_var, sizeof(s_var), "_s%u", (*temp_counter)++);
			se_sdf_emit_indent(out, indent);
			if (rotation_uniform) {
				se_sdf_string_append(out, "vec3 %s = -%s;\n", rot_var, rotation_uniform);
			} else {
				se_sdf_string_append(out, "vec3 %s = vec3(%.6f, %.6f, %.6f);\n",
					rot_var,
					-base_rotation.x,
					-base_rotation.y,
					-base_rotation.z
				);
			}
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "float %s = cos(%s.z);\n", c_var, rot_var);
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "float %s = sin(%s.z);\n", s_var, rot_var);
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "%s.xy = mat2(%s, -%s, %s, %s) * %s.xy;\n", transformed_p, c_var, s_var, s_var, c_var, transformed_p);
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "%s = cos(%s.y);\n", c_var, rot_var);
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "%s = sin(%s.y);\n", s_var, rot_var);
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "%s.xz = mat2(%s, -%s, %s, %s) * %s.xz;\n", transformed_p, c_var, s_var, s_var, c_var, transformed_p);
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "%s = cos(%s.x);\n", c_var, rot_var);
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "%s = sin(%s.x);\n", s_var, rot_var);
			se_sdf_emit_indent(out, indent);
			se_sdf_string_append(out, "%s.yz = mat2(%s, -%s, %s, %s) * %s.yz;\n", transformed_p, c_var, s_var, s_var, c_var, transformed_p);
		}
		local_p = transformed_p;
	} else if (!se_sdf_is_identity(&obj->transform)) {
        snprintf(transformed_p, sizeof(transformed_p), "_p%u", (*temp_counter)++);
        se_sdf_emit_indent(out, indent);
        se_sdf_gen_transform(out, p_var, obj->transform, transformed_p);
        se_sdf_string_append(out, "\n");
        local_p = transformed_p;
    }

    sz child_count = se_sdf_count_children(obj);
    if (child_count == 0) {
        se_sdf_emit_leaf_eval(out, obj, local_p, result_var, indent);
        return;
    }

    char child_result[32];
    snprintf(child_result, sizeof(child_result), "_d%u", (*temp_counter)++);
    se_sdf_object* child0 = se_sdf_get_child(obj, 0);
    se_sdf_emit_object_eval(out, child0, local_p, child_result, temp_counter, indent);

    se_sdf_emit_indent(out, indent);
    se_sdf_string_append(out, "float %s = %s;\n", result_var, child_result);

    for (sz i = 1; i < child_count; ++i) {
        char rhs_result[32];
        snprintf(rhs_result, sizeof(rhs_result), "_d%u", (*temp_counter)++);
        se_sdf_object* child = se_sdf_get_child(obj, i);
        se_sdf_emit_object_eval(out, child, local_p, rhs_result, temp_counter, indent);
		se_sdf_emit_operation(out, obj->operation, obj->operation_amount, result_var, rhs_result, indent);
	}
}

void se_sdf_object_to_string(se_sdf_string* out, se_sdf_object* obj, const char* p_var) {
    u32 temp_counter = 0;
    char root_result[32];
    snprintf(root_result, sizeof(root_result), "_d%u", temp_counter++);
    se_sdf_emit_object_eval(out, obj, p_var, root_result, &temp_counter, 1);
    se_sdf_emit_indent(out, 1);
    se_sdf_string_append(out, "return %s;\n", root_result);
}

void se_sdf_generate_distance_function(se_sdf_string* out, se_sdf_object* root, const char* func_name) {
    se_sdf_string_append(out, "float %s(vec3 p) {\n", func_name);
    se_sdf_object_to_string(out, root, "p");
    se_sdf_string_append(out, "}\n");
}

typedef struct {
	s_vec3 a;
	s_vec3 b;
	s_vec3 c;
	s_vec3 color_a;
	s_vec3 color_b;
	s_vec3 color_c;
	se_box_3d bounds;
} se_sdf_bake_triangle;
typedef s_array(se_sdf_bake_triangle, se_sdf_bake_triangles);

static s_vec4 se_sdf_bake_get_mesh_base_factor(const se_mesh *mesh, const c8 *uniform_name) {
	s_vec4 factor = s_vec4(1.0f, 1.0f, 1.0f, 1.0f);
	if (!mesh || mesh->shader == S_HANDLE_NULL) {
		return factor;
	}

	const c8 *name = uniform_name;
	if (!name || name[0] == '\0') {
		name = "u_base_color_factor";
	}

	s_vec4 *uniform = se_shader_get_uniform_vec4(mesh->shader, name);
	if (!uniform && strcmp(name, "u_base_color_factor") != 0) {
		uniform = se_shader_get_uniform_vec4(mesh->shader, "u_base_color_factor");
	}
	if (uniform) {
		factor = *uniform;
	}
	return factor;
}

static se_texture_handle se_sdf_bake_get_mesh_base_texture(const se_mesh *mesh, const c8 *uniform_name) {
	if (!mesh || mesh->shader == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}

	const c8 *name = uniform_name;
	if (!name || name[0] == '\0') {
		name = "u_texture";
	}

	u32 *uniform = se_shader_get_uniform_texture(mesh->shader, name);
	if (!uniform && strcmp(name, "u_texture") != 0) {
		uniform = se_shader_get_uniform_texture(mesh->shader, "u_texture");
	}
	if (!uniform || *uniform == 0u) {
		return S_HANDLE_NULL;
	}
	return se_texture_find_by_id(*uniform);
}

static s_vec3 se_sdf_bake_sample_diffuse_color(
	const se_vertex_3d *vertex,
	const se_texture_handle texture_handle,
	const s_vec4 *base_factor
) {
	s_vec3 sampled = s_vec3(1.0f, 1.0f, 1.0f);
	if (texture_handle != S_HANDLE_NULL && vertex != NULL) {
		const s_vec2 uv = vertex->uv;
		(void)se_texture_sample_rgb(texture_handle, &uv, &sampled);
	}
	return s_vec3(
		sampled.x * base_factor->x,
		sampled.y * base_factor->y,
		sampled.z * base_factor->z
	);
}

static void se_sdf_bake_bounds_include_point(
	s_vec3 *out_min,
	s_vec3 *out_max,
	b8 *has_value,
	const s_vec3 *point
) {
	if (!out_min || !out_max || !has_value || !point) {
		return;
	}
	if (!*has_value) {
		*out_min = *point;
		*out_max = *point;
		*has_value = true;
		return;
	}
	out_min->x = s_min(out_min->x, point->x);
	out_min->y = s_min(out_min->y, point->y);
	out_min->z = s_min(out_min->z, point->z);
	out_max->x = s_max(out_max->x, point->x);
	out_max->y = s_max(out_max->y, point->y);
	out_max->z = s_max(out_max->z, point->z);
}

static f32 se_sdf_bake_point_aabb_distance_sq(const s_vec3 *point, const se_box_3d *bounds) {
	const f32 cx = s_max(bounds->min.x, s_min(bounds->max.x, point->x));
	const f32 cy = s_max(bounds->min.y, s_min(bounds->max.y, point->y));
	const f32 cz = s_max(bounds->min.z, s_min(bounds->max.z, point->z));
	const f32 dx = point->x - cx;
	const f32 dy = point->y - cy;
	const f32 dz = point->z - cz;
	return dx * dx + dy * dy + dz * dz;
}

static void se_sdf_bake_closest_point_on_triangle(
	const s_vec3 *point,
	const s_vec3 *a,
	const s_vec3 *b,
	const s_vec3 *c,
	s_vec3 *out_closest,
	s_vec3 *out_barycentric
) {
	const s_vec3 ab = s_vec3_sub(b, a);
	const s_vec3 ac = s_vec3_sub(c, a);
	const s_vec3 ap = s_vec3_sub(point, a);
	const f32 d1 = s_vec3_dot(&ab, &ap);
	const f32 d2 = s_vec3_dot(&ac, &ap);
	if (d1 <= 0.0f && d2 <= 0.0f) {
		*out_closest = *a;
		*out_barycentric = s_vec3(1.0f, 0.0f, 0.0f);
		return;
	}

	const s_vec3 bp = s_vec3_sub(point, b);
	const f32 d3 = s_vec3_dot(&ab, &bp);
	const f32 d4 = s_vec3_dot(&ac, &bp);
	if (d3 >= 0.0f && d4 <= d3) {
		*out_closest = *b;
		*out_barycentric = s_vec3(0.0f, 1.0f, 0.0f);
		return;
	}

	const f32 vc = d1 * d4 - d3 * d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
		const f32 v = d1 / (d1 - d3);
		*out_closest = s_vec3_add(a, &s_vec3_muls(&ab, v));
		*out_barycentric = s_vec3(1.0f - v, v, 0.0f);
		return;
	}

	const s_vec3 cp = s_vec3_sub(point, c);
	const f32 d5 = s_vec3_dot(&ab, &cp);
	const f32 d6 = s_vec3_dot(&ac, &cp);
	if (d6 >= 0.0f && d5 <= d6) {
		*out_closest = *c;
		*out_barycentric = s_vec3(0.0f, 0.0f, 1.0f);
		return;
	}

	const f32 vb = d5 * d2 - d1 * d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
		const f32 w = d2 / (d2 - d6);
		*out_closest = s_vec3_add(a, &s_vec3_muls(&ac, w));
		*out_barycentric = s_vec3(1.0f - w, 0.0f, w);
		return;
	}

	const f32 va = d3 * d6 - d5 * d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
		const s_vec3 bc = s_vec3_sub(c, b);
		const f32 w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		*out_closest = s_vec3_add(b, &s_vec3_muls(&bc, w));
		*out_barycentric = s_vec3(0.0f, 1.0f - w, w);
		return;
	}

	const f32 denom = 1.0f / (va + vb + vc);
	const f32 v = vb * denom;
	const f32 w = vc * denom;
	const f32 u = 1.0f - v - w;
	*out_closest = s_vec3_add(a, &s_vec3_add(&s_vec3_muls(&ab, v), &s_vec3_muls(&ac, w)));
	*out_barycentric = s_vec3(u, v, w);
}

static b8 se_sdf_bake_ray_intersects_triangle(
	const s_vec3 *origin,
	const s_vec3 *direction,
	const se_sdf_bake_triangle *triangle
) {
	const f32 eps = 1e-6f;
	const s_vec3 edge_ab = s_vec3_sub(&triangle->b, &triangle->a);
	const s_vec3 edge_ac = s_vec3_sub(&triangle->c, &triangle->a);
	const s_vec3 pvec = s_vec3_cross(direction, &edge_ac);
	const f32 det = s_vec3_dot(&edge_ab, &pvec);
	if (fabsf(det) < eps) {
		return false;
	}

	const f32 inv_det = 1.0f / det;
	const s_vec3 tvec = s_vec3_sub(origin, &triangle->a);
	const f32 u = s_vec3_dot(&tvec, &pvec) * inv_det;
	if (u < -eps || u > 1.0f + eps) {
		return false;
	}

	const s_vec3 qvec = s_vec3_cross(&tvec, &edge_ab);
	const f32 v = s_vec3_dot(direction, &qvec) * inv_det;
	if (v < -eps || (u + v) > 1.0f + eps) {
		return false;
	}

	const f32 t = s_vec3_dot(&edge_ac, &qvec) * inv_det;
	return t > eps;
}

static b8 se_sdf_bake_point_inside_mesh_parity(const s_vec3 *point, const se_sdf_bake_triangles *triangles) {
	if (!point || !triangles || s_array_get_size((se_sdf_bake_triangles *)triangles) == 0u) {
		return false;
	}

	const s_vec3 ray_dir_x = s_vec3(1.0f, 0.0f, 0.0f);
	const s_vec3 ray_dir_y = s_vec3(0.0f, 1.0f, 0.0f);
	const s_vec3 ray_dir_z = s_vec3(0.0f, 0.0f, 1.0f);
	const f32 eps = 1e-6f;

	u32 odd_rays = 0u;
	for (u32 axis = 0u; axis < 3u; ++axis) {
		const s_vec3 *ray_dir = axis == 0u ? &ray_dir_x : (axis == 1u ? &ray_dir_y : &ray_dir_z);
		u32 intersections = 0u;
		for (sz i = 0; i < s_array_get_size((se_sdf_bake_triangles *)triangles); ++i) {
			se_sdf_bake_triangle *triangle = s_array_get((se_sdf_bake_triangles *)triangles, s_array_handle((se_sdf_bake_triangles *)triangles, (u32)i));
			if (!triangle) {
				continue;
			}

			if (axis == 0u) {
				if (triangle->bounds.max.x <= point->x + eps ||
					point->y < triangle->bounds.min.y - eps || point->y > triangle->bounds.max.y + eps ||
					point->z < triangle->bounds.min.z - eps || point->z > triangle->bounds.max.z + eps) {
					continue;
				}
			} else if (axis == 1u) {
				if (triangle->bounds.max.y <= point->y + eps ||
					point->x < triangle->bounds.min.x - eps || point->x > triangle->bounds.max.x + eps ||
					point->z < triangle->bounds.min.z - eps || point->z > triangle->bounds.max.z + eps) {
					continue;
				}
			} else {
				if (triangle->bounds.max.z <= point->z + eps ||
					point->x < triangle->bounds.min.x - eps || point->x > triangle->bounds.max.x + eps ||
					point->y < triangle->bounds.min.y - eps || point->y > triangle->bounds.max.y + eps) {
					continue;
				}
			}

			if (se_sdf_bake_ray_intersects_triangle(point, ray_dir, triangle)) {
				intersections++;
			}
		}

		if ((intersections & 1u) != 0u) {
			odd_rays++;
		}
	}

	return odd_rays >= 2u;
}

b8 se_sdf_bake_model_texture3d(
	se_model_handle model_handle,
	const se_sdf_model_texture3d_desc *desc,
	se_sdf_model_texture3d_result *out_result
) {
	if (!out_result) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_context *ctx = se_current_context();
	se_model *model = ctx ? s_array_get(&ctx->models, model_handle) : NULL;
	if (!ctx || !model) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_sdf_model_texture3d_desc cfg = SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS;
	if (desc) {
		cfg = *desc;
		if (cfg.resolution_x == 0u) cfg.resolution_x = SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS.resolution_x;
		if (cfg.resolution_y == 0u) cfg.resolution_y = SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS.resolution_y;
		if (cfg.resolution_z == 0u) cfg.resolution_z = SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS.resolution_z;
		if (cfg.padding < 0.0f) cfg.padding = 0.0f;
		if (!cfg.base_color_texture_uniform || cfg.base_color_texture_uniform[0] == '\0') {
			cfg.base_color_texture_uniform = SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS.base_color_texture_uniform;
		}
		if (!cfg.base_color_factor_uniform || cfg.base_color_factor_uniform[0] == '\0') {
			cfg.base_color_factor_uniform = SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS.base_color_factor_uniform;
		}
	}

	se_sdf_bake_triangles triangles = {0};
	s_array_init(&triangles);

	s_vec3 scene_min = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 scene_max = s_vec3(0.0f, 0.0f, 0.0f);
	b8 has_bounds = false;

	se_mesh *mesh = NULL;
	s_foreach(&model->meshes, mesh) {
		if (!mesh || !se_mesh_has_cpu_data(mesh)) {
			s_array_clear(&triangles);
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			return false;
		}

		const sz vertex_count = s_array_get_size(&mesh->cpu.vertices);
		const sz index_count = s_array_get_size(&mesh->cpu.indices);
		if (vertex_count == 0 || index_count < 3) {
			continue;
		}

		se_texture_handle base_texture = se_sdf_bake_get_mesh_base_texture(mesh, cfg.base_color_texture_uniform);
		const s_vec4 base_factor = se_sdf_bake_get_mesh_base_factor(mesh, cfg.base_color_factor_uniform);

		s_array(s_vec3, vertex_colors) = {0};
		s_array_init(&vertex_colors);
		s_array_reserve(&vertex_colors, vertex_count);

		for (sz v = 0; v < vertex_count; ++v) {
			const se_vertex_3d *vertex = s_array_get(&mesh->cpu.vertices, s_array_handle(&mesh->cpu.vertices, (u32)v));
			const s_vec3 color = se_sdf_bake_sample_diffuse_color(vertex, base_texture, &base_factor);
			s_handle color_handle = s_array_increment(&vertex_colors);
			s_vec3 *dst = s_array_get(&vertex_colors, color_handle);
			*dst = color;
		}

		const sz triangle_count = index_count / 3u;
		for (sz t = 0; t < triangle_count; ++t) {
			const u32 *i0_ptr = s_array_get(&mesh->cpu.indices, s_array_handle(&mesh->cpu.indices, (u32)(t * 3u + 0u)));
			const u32 *i1_ptr = s_array_get(&mesh->cpu.indices, s_array_handle(&mesh->cpu.indices, (u32)(t * 3u + 1u)));
			const u32 *i2_ptr = s_array_get(&mesh->cpu.indices, s_array_handle(&mesh->cpu.indices, (u32)(t * 3u + 2u)));
			if (!i0_ptr || !i1_ptr || !i2_ptr) {
				continue;
			}
			const u32 i0 = *i0_ptr;
			const u32 i1 = *i1_ptr;
			const u32 i2 = *i2_ptr;
			if ((sz)i0 >= vertex_count || (sz)i1 >= vertex_count || (sz)i2 >= vertex_count) {
				continue;
			}

			const se_vertex_3d *v0 = s_array_get(&mesh->cpu.vertices, s_array_handle(&mesh->cpu.vertices, i0));
			const se_vertex_3d *v1 = s_array_get(&mesh->cpu.vertices, s_array_handle(&mesh->cpu.vertices, i1));
			const se_vertex_3d *v2 = s_array_get(&mesh->cpu.vertices, s_array_handle(&mesh->cpu.vertices, i2));
			const s_vec3 *c0 = s_array_get(&vertex_colors, s_array_handle(&vertex_colors, i0));
			const s_vec3 *c1 = s_array_get(&vertex_colors, s_array_handle(&vertex_colors, i1));
			const s_vec3 *c2 = s_array_get(&vertex_colors, s_array_handle(&vertex_colors, i2));
			if (!v0 || !v1 || !v2 || !c0 || !c1 || !c2) {
				continue;
			}

			const s_vec3 p0 = se_sdf_mul_mat4_point(&mesh->matrix, &v0->position);
			const s_vec3 p1 = se_sdf_mul_mat4_point(&mesh->matrix, &v1->position);
			const s_vec3 p2 = se_sdf_mul_mat4_point(&mesh->matrix, &v2->position);

			se_sdf_bake_triangle tri = {0};
			tri.a = p0;
			tri.b = p1;
			tri.c = p2;
			tri.color_a = *c0;
			tri.color_b = *c1;
			tri.color_c = *c2;
			tri.bounds.min = s_vec3(
				s_min(p0.x, s_min(p1.x, p2.x)),
				s_min(p0.y, s_min(p1.y, p2.y)),
				s_min(p0.z, s_min(p1.z, p2.z))
			);
			tri.bounds.max = s_vec3(
				s_max(p0.x, s_max(p1.x, p2.x)),
				s_max(p0.y, s_max(p1.y, p2.y)),
				s_max(p0.z, s_max(p1.z, p2.z))
			);

			se_sdf_bake_bounds_include_point(&scene_min, &scene_max, &has_bounds, &p0);
			se_sdf_bake_bounds_include_point(&scene_min, &scene_max, &has_bounds, &p1);
			se_sdf_bake_bounds_include_point(&scene_min, &scene_max, &has_bounds, &p2);

			s_handle tri_handle = s_array_increment(&triangles);
			se_sdf_bake_triangle *dst = s_array_get(&triangles, tri_handle);
			*dst = tri;
		}

		s_array_clear(&vertex_colors);
	}

	if (!has_bounds || s_array_get_size(&triangles) == 0) {
		s_array_clear(&triangles);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	const s_vec3 size = s_vec3_sub(&scene_max, &scene_min);
	const f32 max_extent = s_max(size.x, s_max(size.y, size.z));
	const f32 padding = max_extent > 0.0f ? max_extent * cfg.padding : 0.1f;

	scene_min = s_vec3(scene_min.x - padding, scene_min.y - padding, scene_min.z - padding);
	scene_max = s_vec3(scene_max.x + padding, scene_max.y + padding, scene_max.z + padding);

	s_vec3 volume_size = s_vec3_sub(&scene_max, &scene_min);
	if (volume_size.x <= 0.0f) volume_size.x = 0.001f;
	if (volume_size.y <= 0.0f) volume_size.y = 0.001f;
	if (volume_size.z <= 0.0f) volume_size.z = 0.001f;

	const s_vec3 voxel_size = s_vec3(
		volume_size.x / (f32)cfg.resolution_x,
		volume_size.y / (f32)cfg.resolution_y,
		volume_size.z / (f32)cfg.resolution_z
	);

	const sz voxel_count = (sz)cfg.resolution_x * (sz)cfg.resolution_y * (sz)cfg.resolution_z;
	const sz value_count = voxel_count * 4u;
	if (voxel_count == 0 || value_count / 4u != voxel_count) {
		s_array_clear(&triangles);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}

	f32 *volume_rgbsdf = malloc(sizeof(f32) * value_count);
	if (!volume_rgbsdf) {
		s_array_clear(&triangles);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}

	const f32 max_distance = cfg.max_distance > 0.0f
		? cfg.max_distance
		: s_vec3_length(&volume_size);

	for (u32 z = 0; z < cfg.resolution_z; ++z) {
		for (u32 y = 0; y < cfg.resolution_y; ++y) {
			for (u32 x = 0; x < cfg.resolution_x; ++x) {
				const s_vec3 point = s_vec3(
					scene_min.x + ((f32)x + 0.5f) * voxel_size.x,
					scene_min.y + ((f32)y + 0.5f) * voxel_size.y,
					scene_min.z + ((f32)z + 0.5f) * voxel_size.z
				);

				f32 best_distance_sq = FLT_MAX;
				s_vec3 best_barycentric = s_vec3(1.0f, 0.0f, 0.0f);
				se_sdf_bake_triangle *best_triangle = NULL;

				for (sz i = 0; i < s_array_get_size(&triangles); ++i) {
					se_sdf_bake_triangle *triangle = s_array_get(&triangles, s_array_handle(&triangles, (u32)i));
					if (!triangle) {
						continue;
					}

					const f32 aabb_distance_sq = se_sdf_bake_point_aabb_distance_sq(&point, &triangle->bounds);
					if (aabb_distance_sq > best_distance_sq) {
						continue;
					}

					s_vec3 closest = s_vec3(0.0f, 0.0f, 0.0f);
					s_vec3 barycentric = s_vec3(1.0f, 0.0f, 0.0f);
					se_sdf_bake_closest_point_on_triangle(
						&point,
						&triangle->a,
						&triangle->b,
						&triangle->c,
						&closest,
						&barycentric
					);

					const s_vec3 delta = s_vec3_sub(&point, &closest);
					const f32 distance_sq = s_vec3_dot(&delta, &delta);
					if (distance_sq < best_distance_sq) {
						best_distance_sq = distance_sq;
						best_barycentric = barycentric;
						best_triangle = triangle;
					}
				}

				const sz voxel_index = ((sz)z * (sz)cfg.resolution_y * (sz)cfg.resolution_x) +
					((sz)y * (sz)cfg.resolution_x) + (sz)x;
				const sz base = voxel_index * 4u;
				if (!best_triangle) {
					volume_rgbsdf[base + 0u] = 1.0f;
					volume_rgbsdf[base + 1u] = 1.0f;
					volume_rgbsdf[base + 2u] = 1.0f;
					volume_rgbsdf[base + 3u] = max_distance;
					continue;
				}

				const b8 inside = se_sdf_bake_point_inside_mesh_parity(&point, &triangles);
				const f32 distance = sqrtf(best_distance_sq);
				const f32 signed_distance = inside ? -distance : distance;

				const s_vec3 interp_color = s_vec3(
					best_triangle->color_a.x * best_barycentric.x + best_triangle->color_b.x * best_barycentric.y + best_triangle->color_c.x * best_barycentric.z,
					best_triangle->color_a.y * best_barycentric.x + best_triangle->color_b.y * best_barycentric.y + best_triangle->color_c.y * best_barycentric.z,
					best_triangle->color_a.z * best_barycentric.x + best_triangle->color_b.z * best_barycentric.y + best_triangle->color_c.z * best_barycentric.z
				);

				volume_rgbsdf[base + 0u] = interp_color.x;
				volume_rgbsdf[base + 1u] = interp_color.y;
				volume_rgbsdf[base + 2u] = interp_color.z;
				volume_rgbsdf[base + 3u] = s_max(-max_distance, s_min(max_distance, signed_distance));
			}
		}
	}

	se_texture_handle baked_texture = se_texture_create_3d_rgba16f(
		volume_rgbsdf,
		cfg.resolution_x,
		cfg.resolution_y,
		cfg.resolution_z,
		SE_CLAMP
	);

	free(volume_rgbsdf);
	s_array_clear(&triangles);

	if (baked_texture == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}

	memset(out_result, 0, sizeof(*out_result));
	out_result->texture = baked_texture;
	out_result->bounds_min = scene_min;
	out_result->bounds_max = scene_max;
	out_result->voxel_size = voxel_size;
	out_result->max_distance = max_distance;

	se_set_last_error(SE_RESULT_OK);
	return true;
}
