// Syphax-Engine - Ougi Washi

#include "se_sdf.h"
#include "se_camera.h"
#include "se_render.h"
#include "se_shader.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
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

typedef struct {
	se_sdf_scene_handle owner_scene;
	s_mat4 transform;
	se_sdf_operation operation;
	se_sdf_primitive_desc primitive;
	se_sdf_node_handle parent;
	b8 is_group;
	s_array(se_sdf_node_handle, children);
	s_vec3 control_translation;
	s_vec3 control_rotation;
	s_vec3 control_scale;
	b8 control_trs_initialized;
} se_sdf_runtime_node;
typedef s_array(se_sdf_runtime_node, se_sdf_runtime_nodes);

typedef struct {
	se_sdf_runtime_nodes nodes;
	se_sdf_node_handle root;
} se_sdf_runtime_scene;
typedef s_array(se_sdf_runtime_scene, se_sdf_runtime_scenes);

typedef enum {
	SE_SDF_RUNTIME_NODE_BIND_TRANSLATION,
	SE_SDF_RUNTIME_NODE_BIND_ROTATION,
	SE_SDF_RUNTIME_NODE_BIND_SCALE
} se_sdf_runtime_node_bind_target;

typedef struct {
	se_sdf_scene_handle scene;
	se_sdf_node_handle node;
	se_sdf_control_handle control;
	se_sdf_runtime_node_bind_target target;
} se_sdf_runtime_node_binding;
typedef s_array(se_sdf_runtime_node_binding, se_sdf_runtime_node_bindings);

typedef struct {
	se_sdf_scene_handle scene;
	se_sdf_node_handle node;
	se_sdf_primitive_param param;
	se_sdf_control_handle control;
} se_sdf_runtime_primitive_binding;
typedef s_array(se_sdf_runtime_primitive_binding, se_sdf_runtime_primitive_bindings);

typedef union {
	f32 f;
	s_vec2 vec2;
	s_vec3 vec3;
	s_vec4 vec4;
	i32 i;
	s_mat4 mat4;
} se_sdf_runtime_control_value;

typedef struct {
	char name[64];
	char uniform_name[96];
	se_sdf_control_type type;
	se_sdf_runtime_control_value value;
	se_sdf_runtime_control_value last_uploaded_value;
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
} se_sdf_runtime_control;
typedef s_array(se_sdf_runtime_control, se_sdf_runtime_controls);

typedef struct {
	se_sdf_renderer_desc desc;
	se_sdf_raymarch_quality quality;
	se_sdf_renderer_debug debug;
	se_sdf_scene_handle scene;
	se_sdf_runtime_controls controls;
	se_sdf_runtime_node_bindings node_bindings;
	se_sdf_runtime_primitive_bindings primitive_bindings;
	se_sdf_string generated_fragment_source;
	sz last_uniform_write_count;
	sz total_uniform_write_count;
	se_shader_handle shader;
	se_quad quad;
	b8 quad_ready;
	se_sdf_material_desc material;
	se_sdf_stylized_desc stylized;
	s_vec3 default_camera_position;
	s_vec3 default_camera_target;
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
} se_sdf_runtime_renderer;
typedef s_array(se_sdf_runtime_renderer, se_sdf_runtime_renderers);

// TODO: move from here, not thread safe. Maybe we should use se_context instead?
static se_sdf_runtime_scenes se_sdf_runtime_scene_storage;
static se_sdf_runtime_renderers se_sdf_runtime_renderer_storage;
static b8 se_sdf_runtime_scene_storage_initialized = 0;
static se_sdf_runtime_renderer* se_sdf_codegen_active_renderer = NULL;

static void se_sdf_runtime_control_clear_binding(se_sdf_runtime_control* control);
static b8 se_sdf_runtime_control_value_equals(
	se_sdf_control_type type,
	const se_sdf_runtime_control_value* a,
	const se_sdf_runtime_control_value* b
);
static b8 se_sdf_runtime_validate_primitive_desc(const se_sdf_primitive_desc* primitive);
static void se_sdf_runtime_sync_control_bindings(se_sdf_runtime_renderer* renderer);
static b8 se_sdf_runtime_apply_node_bindings(se_sdf_runtime_renderer* renderer);
static b8 se_sdf_runtime_apply_primitive_bindings(se_sdf_runtime_renderer* renderer);
static void se_sdf_runtime_apply_renderer_shading_bindings(se_sdf_runtime_renderer* renderer);
static b8 se_sdf_runtime_emit_control_uniform_declarations(
	se_sdf_runtime_renderer* renderer,
	se_sdf_string* out
);
static const char* se_sdf_runtime_find_node_binding_uniform_name(
	se_sdf_runtime_renderer* renderer,
	se_sdf_scene_handle scene,
	se_sdf_node_handle node,
	se_sdf_runtime_node_bind_target target
);
static const char* se_sdf_runtime_find_primitive_binding_uniform_name(
	se_sdf_runtime_renderer* renderer,
	se_sdf_scene_handle scene,
	se_sdf_node_handle node,
	se_sdf_primitive_param param
);
static b8 se_sdf_runtime_renderer_has_live_shader(const se_sdf_runtime_renderer* renderer);
static void se_sdf_runtime_renderer_release_shader(se_sdf_runtime_renderer* renderer);
static void se_sdf_runtime_renderer_release_quad(se_sdf_runtime_renderer* renderer);
static void se_sdf_runtime_renderer_invalidate_gpu_state(se_sdf_runtime_renderer* renderer);
static void se_sdf_runtime_renderer_refresh_context_state(se_sdf_runtime_renderer* renderer);
static s_vec3 se_sdf_runtime_mul_mat4_point(const s_mat4* mat, const s_vec3* point);
static void se_sdf_runtime_scene_bounds_expand_point(se_sdf_scene_bounds* bounds, const s_vec3* point);
static void se_sdf_runtime_scene_bounds_expand_transformed_aabb(
	se_sdf_scene_bounds* bounds,
	const s_mat4* transform,
	const s_vec3* local_min,
	const s_vec3* local_max
);
static b8 se_sdf_runtime_get_primitive_local_bounds(
	const se_sdf_primitive_desc* primitive,
	s_vec3* out_min,
	s_vec3* out_max,
	b8* out_unbounded
);
static void se_sdf_runtime_collect_scene_bounds_recursive(
	se_sdf_runtime_scene* scene_ptr,
	se_sdf_scene_handle scene,
	se_sdf_node_handle node_handle,
	const s_mat4* parent_transform,
	se_sdf_scene_bounds* bounds,
	sz depth,
	sz max_depth
);

static void se_sdf_runtime_init_storage(void) {
	if (se_sdf_runtime_scene_storage_initialized) {
		return;
	}
	s_array_init(&se_sdf_runtime_scene_storage);
	s_array_init(&se_sdf_runtime_renderer_storage);
	se_sdf_runtime_scene_storage_initialized = 1;
}

static se_sdf_runtime_renderer* se_sdf_runtime_renderer_from_handle(const se_sdf_renderer_handle renderer) {
	if (renderer == SE_SDF_RENDERER_NULL) {
		return NULL;
	}
	se_sdf_runtime_init_storage();
	return s_array_get(&se_sdf_runtime_renderer_storage, renderer);
}

static void se_sdf_runtime_set_diagnostics(
	se_sdf_runtime_renderer* renderer,
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

static b8 se_sdf_runtime_renderer_has_live_shader(const se_sdf_runtime_renderer* renderer) {
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

static void se_sdf_runtime_renderer_release_shader(se_sdf_runtime_renderer* renderer) {
	if (!renderer || renderer->shader == S_HANDLE_NULL) {
		return;
	}
	se_context* ctx = se_current_context();
	if (ctx && s_array_get(&ctx->shaders, renderer->shader) != NULL) {
		se_shader_destroy(renderer->shader);
	}
	renderer->shader = S_HANDLE_NULL;
}

static void se_sdf_runtime_renderer_release_quad(se_sdf_runtime_renderer* renderer) {
	if (!renderer || !renderer->quad_ready) {
		return;
	}
	if (se_render_has_context()) {
		se_quad_destroy(&renderer->quad);
	}
	memset(&renderer->quad, 0, sizeof(renderer->quad));
	renderer->quad_ready = 0;
}

static void se_sdf_runtime_renderer_invalidate_gpu_state(se_sdf_runtime_renderer* renderer) {
	if (!renderer) {
		return;
	}
	se_sdf_runtime_renderer_release_shader(renderer);
	se_sdf_runtime_renderer_release_quad(renderer);
	renderer->built = 0;
	for (sz i = 0; i < s_array_get_size(&renderer->controls); ++i) {
		se_sdf_runtime_control* control = s_array_get(&renderer->controls, s_array_handle(&renderer->controls, (u32)i));
		if (!control) {
			continue;
		}
		control->cached_uniform_location = -1;
		control->has_last_uploaded_value = 0;
		control->dirty = 1;
	}
}

static void se_sdf_runtime_renderer_refresh_context_state(se_sdf_runtime_renderer* renderer) {
	if (!renderer) {
		return;
	}
	const u64 current_generation = se_render_get_generation();
	if (renderer->render_generation != 0 && current_generation != 0 &&
		renderer->render_generation != current_generation) {
		se_sdf_runtime_renderer_invalidate_gpu_state(renderer);
	}
	renderer->render_generation = current_generation;

	if (renderer->shader != S_HANDLE_NULL && !se_sdf_runtime_renderer_has_live_shader(renderer)) {
		renderer->shader = S_HANDLE_NULL;
		renderer->built = 0;
	}
}

static se_sdf_runtime_scene* se_sdf_runtime_scene_from_handle(const se_sdf_scene_handle scene) {
	if (scene == SE_SDF_SCENE_NULL) {
		return NULL;
	}
	se_sdf_runtime_init_storage();
	return s_array_get(&se_sdf_runtime_scene_storage, scene);
}

static se_sdf_runtime_node* se_sdf_runtime_node_from_handle(
	se_sdf_runtime_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node
) {
	if (!scene_ptr || node == SE_SDF_NODE_NULL) {
		return NULL;
	}
	se_sdf_runtime_node* node_ptr = s_array_get(&scene_ptr->nodes, node);
	if (!node_ptr || node_ptr->owner_scene != scene) {
		return NULL;
	}
	return node_ptr;
}

static void se_sdf_runtime_scene_release_nodes(se_sdf_runtime_scene* scene_ptr) {
	if (!scene_ptr) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		se_sdf_runtime_node* node = s_array_get(&scene_ptr->nodes, s_array_handle(&scene_ptr->nodes, (u32)i));
		if (node) {
			s_array_clear(&node->children);
		}
	}
	s_array_clear(&scene_ptr->nodes);
	scene_ptr->root = SE_SDF_NODE_NULL;
}

static b8 se_sdf_runtime_remove_child_entry(se_sdf_runtime_node* parent_node, const se_sdf_node_handle child) {
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

static b8 se_sdf_runtime_has_child_entry(se_sdf_runtime_node* parent_node, const se_sdf_node_handle child) {
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

static b8 se_sdf_runtime_is_valid_primitive_type(const se_sdf_primitive_type type) {
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

static b8 se_sdf_runtime_vec3_all_positive(const s_vec3 v) {
	return v.x > 0.0f && v.y > 0.0f && v.z > 0.0f;
}

static b8 se_sdf_runtime_validate_primitive_desc(const se_sdf_primitive_desc* primitive) {
	if (!primitive || !se_sdf_runtime_is_valid_primitive_type(primitive->type)) {
		return 0;
	}

	switch (primitive->type) {
		case SE_SDF_PRIMITIVE_SPHERE:
			return primitive->sphere.radius > 0.0f;
		case SE_SDF_PRIMITIVE_BOX:
			return se_sdf_runtime_vec3_all_positive(primitive->box.size);
		case SE_SDF_PRIMITIVE_ROUND_BOX:
			return se_sdf_runtime_vec3_all_positive(primitive->round_box.size) &&
				primitive->round_box.roundness >= 0.0f;
		case SE_SDF_PRIMITIVE_BOX_FRAME:
			return se_sdf_runtime_vec3_all_positive(primitive->box_frame.size) &&
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

static b8 se_sdf_runtime_operation_child_count_is_legal(
	const se_sdf_operation operation,
	const sz child_count
) {
	if (operation == SE_SDF_OP_SUBTRACTION || operation == SE_SDF_OP_SMOOTH_SUBTRACTION) {
		return child_count != 1;
	}
	return 1;
}

static b8 se_sdf_runtime_is_ancestor(
	se_sdf_runtime_scene* scene_ptr,
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
		se_sdf_runtime_node* current = se_sdf_runtime_node_from_handle(scene_ptr, scene, node);
		if (!current) {
			return 0;
		}
		node = current->parent;
	}
	return 0;
}

static void se_sdf_runtime_detach_from_parent(
	se_sdf_runtime_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node_handle
) {
	se_sdf_runtime_node* node = se_sdf_runtime_node_from_handle(scene_ptr, scene, node_handle);
	if (!node || node->parent == SE_SDF_NODE_NULL) {
		return;
	}
	se_sdf_runtime_node* parent = se_sdf_runtime_node_from_handle(scene_ptr, scene, node->parent);
	if (parent) {
		se_sdf_runtime_remove_child_entry(parent, node_handle);
	}
	node->parent = SE_SDF_NODE_NULL;
}

static void se_sdf_runtime_destroy_node_recursive(
	se_sdf_runtime_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node_handle
) {
	se_sdf_runtime_node* node = se_sdf_runtime_node_from_handle(scene_ptr, scene, node_handle);
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
			se_sdf_runtime_destroy_node_recursive(scene_ptr, scene, *child);
		}
	}
	s_array_clear(&children_copy);

	node = se_sdf_runtime_node_from_handle(scene_ptr, scene, node_handle);
	if (!node) {
		return;
	}

	se_sdf_runtime_detach_from_parent(scene_ptr, scene, node_handle);
	if (scene_ptr->root == node_handle) {
		scene_ptr->root = SE_SDF_NODE_NULL;
	}

	s_array_clear(&node->children);
	s_array_remove(&scene_ptr->nodes, node_handle);
}

se_sdf_scene_handle se_sdf_scene_create(const se_sdf_scene_desc* desc) {
	se_sdf_runtime_init_storage();
	se_sdf_scene_desc scene_desc = SE_SDF_SCENE_DESC_DEFAULTS;
	if (desc) {
		scene_desc = *desc;
	}

	se_sdf_scene_handle scene_handle = s_array_increment(&se_sdf_runtime_scene_storage);
	se_sdf_runtime_scene* scene = s_array_get(&se_sdf_runtime_scene_storage, scene_handle);
	if (!scene) {
		return SE_SDF_SCENE_NULL;
	}
	memset(scene, 0, sizeof(*scene));
	s_array_init(&scene->nodes);
	scene->root = SE_SDF_NODE_NULL;

	if (scene_desc.initial_node_capacity > 0) {
		s_array_reserve(&scene->nodes, scene_desc.initial_node_capacity);
	}

	return scene_handle;
}

void se_sdf_scene_destroy(const se_sdf_scene_handle scene) {
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	se_sdf_runtime_scene_release_nodes(scene_ptr);
	s_array_remove(&se_sdf_runtime_scene_storage, scene);
}

void se_sdf_scene_clear(const se_sdf_scene_handle scene) {
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	se_sdf_runtime_scene_release_nodes(scene_ptr);
}

b8 se_sdf_scene_set_root(const se_sdf_scene_handle scene, const se_sdf_node_handle node) {
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	if (node == SE_SDF_NODE_NULL) {
		scene_ptr->root = SE_SDF_NODE_NULL;
		return 1;
	}
	se_sdf_runtime_node* node_ptr = se_sdf_runtime_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	scene_ptr->root = node;
	return 1;
}

se_sdf_node_handle se_sdf_scene_get_root(const se_sdf_scene_handle scene) {
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return SE_SDF_NODE_NULL;
	}
	return scene_ptr->root;
}

static void se_sdf_runtime_write_scene_error(
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

	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		se_sdf_runtime_write_scene_error(error_message, error_message_size, "invalid scene handle");
		return 0;
	}

	if (scene_ptr->root != SE_SDF_NODE_NULL &&
		!se_sdf_runtime_node_from_handle(scene_ptr, scene, scene_ptr->root)) {
		se_sdf_runtime_write_scene_error(
			error_message, error_message_size, "root handle is invalid (slot=%u)", s_handle_slot(scene_ptr->root));
		return 0;
	}

	const sz node_count = s_array_get_size(&scene_ptr->nodes);
	for (sz i = 0; i < node_count; ++i) {
		se_sdf_node_handle handle = s_array_handle(&scene_ptr->nodes, (u32)i);
		se_sdf_runtime_node* node = s_array_get(&scene_ptr->nodes, handle);
		if (!node) {
			continue;
		}

		if (node->owner_scene != scene) {
			se_sdf_runtime_write_scene_error(
				error_message, error_message_size,
				"node owner mismatch (node_slot=%u)", s_handle_slot(handle));
			return 0;
		}

		if (!node->is_group && !se_sdf_runtime_validate_primitive_desc(&node->primitive)) {
			se_sdf_runtime_write_scene_error(
				error_message, error_message_size,
				"invalid primitive parameters (node_slot=%u, type=%d)",
				s_handle_slot(handle), (int)node->primitive.type);
			return 0;
		}

		if (!se_sdf_runtime_operation_child_count_is_legal(
				node->operation, s_array_get_size(&node->children))) {
			se_sdf_runtime_write_scene_error(
				error_message, error_message_size,
				"illegal operation for child count (node_slot=%u, op=%d, child_count=%zu)",
				s_handle_slot(handle), (int)node->operation, s_array_get_size(&node->children));
			return 0;
		}

		if (node->parent != SE_SDF_NODE_NULL) {
			se_sdf_runtime_node* parent = se_sdf_runtime_node_from_handle(scene_ptr, scene, node->parent);
			if (!parent) {
				se_sdf_runtime_write_scene_error(
					error_message, error_message_size,
					"invalid parent handle (node_slot=%u, parent_slot=%u)",
					s_handle_slot(handle), s_handle_slot(node->parent));
				return 0;
			}
			if (!se_sdf_runtime_has_child_entry(parent, handle)) {
				se_sdf_runtime_write_scene_error(
					error_message, error_message_size,
					"parent link missing child entry (node_slot=%u, parent_slot=%u)",
					s_handle_slot(handle), s_handle_slot(node->parent));
				return 0;
			}
		}

		for (sz child_i = 0; child_i < s_array_get_size(&node->children); ++child_i) {
			se_sdf_node_handle* child_handle = s_array_get(&node->children, s_array_handle(&node->children, (u32)child_i));
			if (!child_handle || *child_handle == SE_SDF_NODE_NULL || *child_handle == handle) {
				se_sdf_runtime_write_scene_error(
					error_message, error_message_size,
					"invalid child link (parent_slot=%u)", s_handle_slot(handle));
				return 0;
			}
			se_sdf_runtime_node* child = se_sdf_runtime_node_from_handle(scene_ptr, scene, *child_handle);
			if (!child) {
				se_sdf_runtime_write_scene_error(
					error_message, error_message_size,
					"dangling child handle (parent_slot=%u, child_slot=%u)",
					s_handle_slot(handle), s_handle_slot(*child_handle));
				return 0;
			}
			if (child->parent != handle) {
				se_sdf_runtime_write_scene_error(
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
			se_sdf_runtime_node* current = se_sdf_runtime_node_from_handle(scene_ptr, scene, walker);
			if (!current) {
				se_sdf_runtime_write_scene_error(
					error_message, error_message_size,
					"invalid ancestor chain (node_slot=%u)", s_handle_slot(handle));
				return 0;
			}
			walker = current->parent;
		}
		if (walker != SE_SDF_NODE_NULL) {
			se_sdf_runtime_write_scene_error(
				error_message, error_message_size,
				"cycle detected in parent chain (node_slot=%u)", s_handle_slot(handle));
			return 0;
		}
	}

	return 1;
}

se_sdf_node_handle se_sdf_node_create_primitive(
	const se_sdf_scene_handle scene,
	const se_sdf_node_primitive_desc* desc
) {
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr || !desc || !se_sdf_runtime_validate_primitive_desc(&desc->primitive)) {
		return SE_SDF_NODE_NULL;
	}

	se_sdf_node_handle node_handle = s_array_increment(&scene_ptr->nodes);
	se_sdf_runtime_node* node = s_array_get(&scene_ptr->nodes, node_handle);
	if (!node) {
		return SE_SDF_NODE_NULL;
	}
	memset(node, 0, sizeof(*node));
	node->owner_scene = scene;
	node->transform = desc->transform;
	node->operation = desc->operation;
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
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return SE_SDF_NODE_NULL;
	}

	se_sdf_node_group_desc group_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	if (desc) {
		group_desc = *desc;
	}

	se_sdf_node_handle node_handle = s_array_increment(&scene_ptr->nodes);
	se_sdf_runtime_node* node = s_array_get(&scene_ptr->nodes, node_handle);
	if (!node) {
		return SE_SDF_NODE_NULL;
	}
	memset(node, 0, sizeof(*node));
	node->owner_scene = scene;
	node->transform = group_desc.transform;
	node->operation = group_desc.operation;
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
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	if (!se_sdf_runtime_node_from_handle(scene_ptr, scene, node)) {
		return;
	}
	se_sdf_runtime_destroy_node_recursive(scene_ptr, scene, node);
}

b8 se_sdf_node_add_child(
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle parent,
	const se_sdf_node_handle child
) {
	if (parent == child || parent == SE_SDF_NODE_NULL || child == SE_SDF_NODE_NULL) {
		return 0;
	}
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}

	se_sdf_runtime_node* parent_node = se_sdf_runtime_node_from_handle(scene_ptr, scene, parent);
	se_sdf_runtime_node* child_node = se_sdf_runtime_node_from_handle(scene_ptr, scene, child);
	if (!parent_node || !child_node) {
		return 0;
	}

	if (se_sdf_runtime_is_ancestor(scene_ptr, scene, child, parent)) {
		return 0;
	}

	if (child_node->parent != SE_SDF_NODE_NULL && child_node->parent != parent) {
		se_sdf_runtime_node* old_parent = se_sdf_runtime_node_from_handle(scene_ptr, scene, child_node->parent);
		if (old_parent) {
			se_sdf_runtime_remove_child_entry(old_parent, child);
		}
	}

	if (!se_sdf_runtime_has_child_entry(parent_node, child)) {
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
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_runtime_node* parent_node = se_sdf_runtime_node_from_handle(scene_ptr, scene, parent);
	if (!parent_node) {
		return 0;
	}

	b8 removed = se_sdf_runtime_remove_child_entry(parent_node, child);
	if (!removed) {
		return 0;
	}

	se_sdf_runtime_node* child_node = se_sdf_runtime_node_from_handle(scene_ptr, scene, child);
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
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_runtime_node* node_ptr = se_sdf_runtime_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	if (!se_sdf_runtime_operation_child_count_is_legal(operation, s_array_get_size(&node_ptr->children))) {
		return 0;
	}
	node_ptr->operation = operation;
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
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_runtime_node* node_ptr = se_sdf_runtime_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	node_ptr->transform = *transform;
	node_ptr->control_trs_initialized = 0;
	return 1;
}

s_mat4 se_sdf_node_get_transform(const se_sdf_scene_handle scene, const se_sdf_node_handle node) {
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return s_mat4_identity;
	}
	se_sdf_runtime_node* node_ptr = se_sdf_runtime_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return s_mat4_identity;
	}
	return node_ptr->transform;
}

s_mat4 se_sdf_transform_trs(
	const f32 tx, const f32 ty, const f32 tz,
	const f32 rx, const f32 ry, const f32 rz,
	const f32 sx, const f32 sy, const f32 sz
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

s_mat4 se_sdf_transform_grid_cell(
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

static se_sdf_primitive_desc se_sdf_runtime_default_sphere_primitive(void) {
	se_sdf_primitive_desc primitive = {0};
	primitive.type = SE_SDF_PRIMITIVE_SPHERE;
	primitive.sphere.radius = 0.82f;
	return primitive;
}

static se_sdf_primitive_desc se_sdf_runtime_default_box_primitive(void) {
	se_sdf_primitive_desc primitive = {0};
	primitive.type = SE_SDF_PRIMITIVE_BOX;
	primitive.box.size = s_vec3(0.60f, 0.60f, 0.60f);
	return primitive;
}

static se_sdf_primitive_desc se_sdf_runtime_gallery_primitive(const i32 index) {
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

	se_sdf_primitive_desc fallback = se_sdf_runtime_default_sphere_primitive();
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

	se_sdf_primitive_desc fallback = se_sdf_runtime_default_sphere_primitive();
	const se_sdf_primitive_desc* selected = primitive ? primitive : &fallback;
	const i32 total_cells = columns * rows;
	for (i32 i = 0; i < total_cells; ++i) {
		const s_mat4 node_transform = se_sdf_transform_grid_cell(i, columns, rows, spacing, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
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
		se_sdf_primitive_desc primitive = se_sdf_runtime_gallery_primitive(i);
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

	se_sdf_primitive_desc center_fallback = se_sdf_runtime_default_sphere_primitive();
	se_sdf_primitive_desc orbit_fallback = se_sdf_runtime_default_box_primitive();
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

static s_vec3 se_sdf_runtime_mul_mat4_point(const s_mat4* mat, const s_vec3* point) {
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

static void se_sdf_runtime_scene_bounds_expand_point(se_sdf_scene_bounds* bounds, const s_vec3* point) {
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

static void se_sdf_runtime_scene_bounds_expand_transformed_aabb(
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
				const s_vec3 world_point = se_sdf_runtime_mul_mat4_point(transform, &local_point);
				se_sdf_runtime_scene_bounds_expand_point(bounds, &world_point);
			}
		}
	}
}

static void se_sdf_runtime_bounds_from_points(
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

static b8 se_sdf_runtime_get_primitive_local_bounds(
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
			se_sdf_runtime_bounds_from_points(points, 2, fabsf(primitive->capsule.radius), out_min, out_max);
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
			se_sdf_runtime_bounds_from_points(points, 2, fabsf(primitive->capped_cylinder_arbitrary.radius), out_min, out_max);
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
			se_sdf_runtime_bounds_from_points(points, 2, radial, out_min, out_max);
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
			se_sdf_runtime_bounds_from_points(points, 2, radial, out_min, out_max);
			return 1;
		}
		case SE_SDF_PRIMITIVE_VESICA_SEGMENT: {
			const s_vec3 points[2] = { primitive->vesica_segment.a, primitive->vesica_segment.b };
			se_sdf_runtime_bounds_from_points(points, 2, fabsf(primitive->vesica_segment.width), out_min, out_max);
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
			se_sdf_runtime_bounds_from_points(points, 3, 0.0f, out_min, out_max);
			return 1;
		}
		case SE_SDF_PRIMITIVE_QUAD: {
			const s_vec3 points[4] = { primitive->quad.a, primitive->quad.b, primitive->quad.c, primitive->quad.d };
			se_sdf_runtime_bounds_from_points(points, 4, 0.0f, out_min, out_max);
			return 1;
		}
		default:
			break;
	}

	return 0;
}

static void se_sdf_runtime_collect_scene_bounds_recursive(
	se_sdf_runtime_scene* scene_ptr,
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
	se_sdf_runtime_node* node = se_sdf_runtime_node_from_handle(scene_ptr, scene, node_handle);
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
		if (se_sdf_runtime_get_primitive_local_bounds(&node->primitive, &local_min, &local_max, &is_unbounded)) {
			se_sdf_runtime_scene_bounds_expand_transformed_aabb(bounds, &world_transform, &local_min, &local_max);
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
		se_sdf_runtime_collect_scene_bounds_recursive(
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

	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr || scene_ptr->root == SE_SDF_NODE_NULL) {
		return 0;
	}

	const sz max_depth = s_array_get_size(&scene_ptr->nodes) + 1;
	se_sdf_runtime_collect_scene_bounds_recursive(
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

	s_vec3 up = align.up;
	f32 up_len = s_vec3_length(&up);
	if (up_len <= 0.0001f) {
		up = s_vec3(0.0f, 1.0f, 0.0f);
	} else {
		up = s_vec3_divs(&up, up_len);
	}
	if (fabsf(s_vec3_dot(&direction, &up)) > 0.995f) {
		up = fabsf(direction.y) < 0.95f ? s_vec3(0.0f, 1.0f, 0.0f) : s_vec3(1.0f, 0.0f, 0.0f);
	}

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
			se_camera_set_orthographic(camera, ortho_height, near_plane, far_plane);
		} else {
			camera_ptr->ortho_height = ortho_height;
		}
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
			se_camera_set_perspective(camera, fov_degrees, near_plane, far_plane);
		}
	}

	camera_ptr->target = bounds.center;
	camera_ptr->position = s_vec3_add(&bounds.center, &s_vec3_muls(&direction, distance));
	camera_ptr->up = up;
	return 1;
}

static void se_sdf_runtime_free_legacy_object(se_sdf_object* obj) {
	if (!obj) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&obj->children); ++i) {
		se_sdf_object* child = s_array_get(&obj->children, s_array_handle(&obj->children, (u32)i));
		if (child) {
			se_sdf_runtime_free_legacy_object(child);
		}
	}
	s_array_clear(&obj->children);
}

static b8 se_sdf_runtime_copy_primitive_to_legacy_object(
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

static b8 se_sdf_runtime_build_legacy_object_recursive(
	se_sdf_runtime_scene* scene_ptr,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node_handle,
	se_sdf_object* out
) {
	if (!scene_ptr || !out || node_handle == SE_SDF_NODE_NULL) {
		return 0;
	}
	se_sdf_runtime_node* runtime_node = se_sdf_runtime_node_from_handle(scene_ptr, scene, node_handle);
	if (!runtime_node) {
		return 0;
	}

	memset(out, 0, sizeof(*out));
	s_array_init(&out->children);
	out->transform = runtime_node->transform;
	out->source_scene = scene;
	out->source_node = node_handle;
	out->operation = runtime_node->operation;
	out->noise = (se_sdf_noise){0};

	if (runtime_node->is_group) {
		out->type = SE_SDF_NONE;
	} else {
		out->type = (se_sdf_object_type)runtime_node->primitive.type;
		if (!se_sdf_runtime_copy_primitive_to_legacy_object(&runtime_node->primitive, out)) {
			se_sdf_runtime_free_legacy_object(out);
			return 0;
		}
	}

	for (sz i = 0; i < s_array_get_size(&runtime_node->children); ++i) {
		se_sdf_node_handle* child_handle = s_array_get(&runtime_node->children, s_array_handle(&runtime_node->children, (u32)i));
		if (!child_handle) {
			continue;
		}
		se_sdf_object child = {0};
		if (!se_sdf_runtime_build_legacy_object_recursive(scene_ptr, scene, *child_handle, &child)) {
			se_sdf_runtime_free_legacy_object(out);
			return 0;
		}
		s_array_add(&out->children, child);
	}

	return 1;
}

se_sdf_renderer_handle se_sdf_renderer_create(const se_sdf_renderer_desc* desc) {
	se_sdf_runtime_init_storage();
	se_sdf_renderer_desc renderer_desc = SE_SDF_RENDERER_DESC_DEFAULTS;
	if (desc) {
		renderer_desc = *desc;
		if (renderer_desc.shader_source_capacity == 0) {
			renderer_desc.shader_source_capacity = SE_SDF_RENDERER_DESC_DEFAULTS.shader_source_capacity;
		}
	}

	se_sdf_renderer_handle handle = s_array_increment(&se_sdf_runtime_renderer_storage);
	se_sdf_runtime_renderer* renderer = s_array_get(&se_sdf_runtime_renderer_storage, handle);
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
	renderer->default_camera_position = s_vec3(0.0f, 0.0f, 6.0f);
	renderer->default_camera_target = s_vec3(0.0f, 0.0f, 0.0f);
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
	se_sdf_runtime_set_diagnostics(renderer, 1, "init", "renderer created");
	return handle;
}

void se_sdf_renderer_destroy(const se_sdf_renderer_handle renderer) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return;
	}
	se_sdf_runtime_renderer_invalidate_gpu_state(renderer_ptr);
	s_array_clear(&renderer_ptr->controls);
	s_array_clear(&renderer_ptr->node_bindings);
	s_array_clear(&renderer_ptr->primitive_bindings);
	se_sdf_string_free(&renderer_ptr->generated_fragment_source);
	s_array_remove(&se_sdf_runtime_renderer_storage, renderer);
}

void se_sdf_shutdown(void) {
	if (!se_sdf_runtime_scene_storage_initialized) {
		return;
	}

	while (s_array_get_size(&se_sdf_runtime_renderer_storage) > 0) {
		se_sdf_renderer_handle renderer_handle = s_array_handle(
			&se_sdf_runtime_renderer_storage,
			(u32)(s_array_get_size(&se_sdf_runtime_renderer_storage) - 1)
		);
		se_sdf_renderer_destroy(renderer_handle);
	}

	while (s_array_get_size(&se_sdf_runtime_scene_storage) > 0) {
		se_sdf_scene_handle scene_handle = s_array_handle(
			&se_sdf_runtime_scene_storage,
			(u32)(s_array_get_size(&se_sdf_runtime_scene_storage) - 1)
		);
		se_sdf_scene_destroy(scene_handle);
	}

	s_array_clear(&se_sdf_runtime_renderer_storage);
	s_array_clear(&se_sdf_runtime_scene_storage);
	se_sdf_runtime_scene_storage_initialized = 0;
}

b8 se_sdf_renderer_set_scene(const se_sdf_renderer_handle renderer, const se_sdf_scene_handle scene) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	if (scene != SE_SDF_SCENE_NULL && !se_sdf_runtime_scene_from_handle(scene)) {
		return 0;
	}
	renderer_ptr->scene = scene;
	renderer_ptr->built = 0;
	return 1;
}

b8 se_sdf_renderer_set_quality(const se_sdf_renderer_handle renderer, const se_sdf_raymarch_quality* quality) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !debug) {
		return 0;
	}
	renderer_ptr->debug = *debug;
	return 1;
}

b8 se_sdf_renderer_set_material(const se_sdf_renderer_handle renderer, const se_sdf_material_desc* material) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !material) {
		return 0;
	}
	renderer_ptr->material = *material;
	return 1;
}

b8 se_sdf_renderer_set_stylized(const se_sdf_renderer_handle renderer, const se_sdf_stylized_desc* stylized) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !stylized) {
		return 0;
	}
	renderer_ptr->stylized = *stylized;
	return 1;
}

se_sdf_stylized_desc se_sdf_renderer_get_stylized(const se_sdf_renderer_handle renderer) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return SE_SDF_STYLIZED_DESC_DEFAULTS;
	}
	return renderer_ptr->stylized;
}

b8 se_sdf_renderer_set_base_color(const se_sdf_renderer_handle renderer, const s_vec3* base_color) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	se_sdf_runtime_renderer_refresh_context_state(renderer_ptr);

	se_sdf_string_free(&renderer_ptr->generated_fragment_source);
	se_sdf_string_init(&renderer_ptr->generated_fragment_source, renderer_ptr->desc.shader_source_capacity);
	if (!renderer_ptr->generated_fragment_source.data) {
		se_sdf_runtime_set_diagnostics(
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
		!se_sdf_runtime_emit_control_uniform_declarations(renderer_ptr, &renderer_ptr->generated_fragment_source)) {
		se_sdf_runtime_set_diagnostics(
			renderer_ptr,
			0,
			"codegen_emit",
			"failed to emit prelude/uniform block/control uniforms"
		);
		return 0;
	}

	b8 emitted_map = 0;
	if (renderer_ptr->scene != SE_SDF_SCENE_NULL) {
		se_sdf_runtime_scene* runtime_scene = se_sdf_runtime_scene_from_handle(renderer_ptr->scene);
		if (runtime_scene && runtime_scene->root != SE_SDF_NODE_NULL) {
			char validation_error[256] = {0};
			if (!se_sdf_scene_validate(renderer_ptr->scene, validation_error, sizeof(validation_error))) {
				se_sdf_runtime_set_diagnostics(
					renderer_ptr,
					0,
					"scene_validate",
					"scene validation failed: %s",
					validation_error[0] != '\0' ? validation_error : "unknown error"
				);
				return 0;
			}
			se_sdf_object legacy_root = {0};
			if (!se_sdf_runtime_build_legacy_object_recursive(
				runtime_scene,
				renderer_ptr->scene,
				runtime_scene->root,
				&legacy_root
			)) {
				se_sdf_runtime_set_diagnostics(
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
			se_sdf_runtime_free_legacy_object(&legacy_root);
			emitted_map = 1;
		}
	}

	if (!emitted_map) {
		if (!se_sdf_codegen_emit_map_stub(&renderer_ptr->generated_fragment_source, map_name)) {
			se_sdf_runtime_set_diagnostics(
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
		se_sdf_runtime_set_diagnostics(
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

	se_sdf_runtime_renderer_release_shader(renderer_ptr);
	renderer_ptr->shader = se_shader_load_from_memory(
		se_sdf_fullscreen_vertex_shader,
		renderer_ptr->generated_fragment_source.data
	);
	if (renderer_ptr->shader == S_HANDLE_NULL) {
		se_sdf_runtime_set_diagnostics(
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
		se_sdf_runtime_control* control = s_array_get(&renderer_ptr->controls, s_array_handle(&renderer_ptr->controls, (u32)i));
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
	se_sdf_runtime_set_diagnostics(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
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
	se_camera* camera_ptr = se_camera_get(camera);
	if (!camera_ptr) {
		return 0;
	}
	frame->has_camera_override = 1;
	frame->camera_position = camera_ptr->position;
	frame->camera_target = camera_ptr->target;
	return 1;
}

b8 se_sdf_renderer_render(const se_sdf_renderer_handle renderer, const se_sdf_frame_desc* frame) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !frame) {
		return 0;
	}
	if (!se_render_has_context()) {
		se_sdf_runtime_set_diagnostics(
			renderer_ptr,
			0,
			"render_context",
			"render skipped: no active render context"
		);
		return 0;
	}
	se_sdf_runtime_renderer_refresh_context_state(renderer_ptr);
	if (!renderer_ptr->built) {
		if (!se_sdf_renderer_build(renderer)) {
			return 0;
		}
	}
	se_sdf_runtime_sync_control_bindings(renderer_ptr);
	(void)se_sdf_runtime_apply_node_bindings(renderer_ptr);
	(void)se_sdf_runtime_apply_primitive_bindings(renderer_ptr);
	se_sdf_runtime_apply_renderer_shading_bindings(renderer_ptr);
	if (renderer_ptr->shader == S_HANDLE_NULL) {
		return 0;
	}

	s_vec2 resolution = frame->resolution;
	if (resolution.x <= 0.0f) resolution.x = 1.0f;
	if (resolution.y <= 0.0f) resolution.y = 1.0f;
	f32 time_value = renderer_ptr->debug.freeze_time ? 0.0f : frame->time_seconds;
	s_vec3 camera_position = frame->has_camera_override ? frame->camera_position : renderer_ptr->default_camera_position;
	s_vec3 camera_target = frame->has_camera_override ? frame->camera_target : renderer_ptr->default_camera_target;

	se_shader_set_vec2(renderer_ptr->shader, "u_resolution", &resolution);
	se_shader_set_float(renderer_ptr->shader, "u_time", time_value);
	se_shader_set_vec2(renderer_ptr->shader, "u_mouse", &frame->mouse_normalized);
	se_shader_set_vec3(renderer_ptr->shader, "u_camera_position", &camera_position);
	se_shader_set_vec3(renderer_ptr->shader, "u_camera_target", &camera_target);
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
		se_sdf_runtime_control* control = s_array_get(&renderer_ptr->controls, s_array_handle(&renderer_ptr->controls, (u32)i));
		if (!control || control->cached_uniform_location < 0 || !control->dirty) {
			continue;
		}
		if (control->has_last_uploaded_value &&
			se_sdf_runtime_control_value_equals(control->type, &control->value, &control->last_uploaded_value)) {
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
	se_shader_use(renderer_ptr->shader, true, false);
	if (renderer_ptr->quad_ready) {
		se_quad_render(&renderer_ptr->quad, 0);
	}
	(void)frame;
	return 1;
}

const char* se_sdf_renderer_get_generated_fragment_source(const se_sdf_renderer_handle renderer) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return NULL;
	}
	return renderer_ptr->generated_fragment_source.data;
}

sz se_sdf_renderer_get_generated_fragment_source_size(const se_sdf_renderer_handle renderer) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return renderer_ptr->generated_fragment_source.size;
}

b8 se_sdf_renderer_dump_shader_source(const se_sdf_renderer_handle renderer, const char* path) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
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

sz se_sdf_renderer_get_last_uniform_write_count(const se_sdf_renderer_handle renderer) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return renderer_ptr->last_uniform_write_count;
}

sz se_sdf_renderer_get_total_uniform_write_count(const se_sdf_renderer_handle renderer) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return renderer_ptr->total_uniform_write_count;
}

const char* se_sdf_control_get_uniform_name(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || control == SE_SDF_CONTROL_NULL) {
		return NULL;
	}
	se_sdf_runtime_control* control_ptr = s_array_get(&renderer_ptr->controls, control);
	if (!control_ptr) {
		return NULL;
	}
	return control_ptr->uniform_name;
}

se_sdf_build_diagnostics se_sdf_renderer_get_last_build_diagnostics(const se_sdf_renderer_handle renderer) {
	se_sdf_build_diagnostics diagnostics = {0};
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return diagnostics;
	}
	return renderer_ptr->diagnostics;
}

static se_sdf_control_handle se_sdf_runtime_find_control_handle(
	se_sdf_runtime_renderer* renderer,
	const char* name
) {
	if (!renderer || !name || name[0] == '\0') {
		return SE_SDF_CONTROL_NULL;
	}
	for (sz i = 0; i < s_array_get_size(&renderer->controls); ++i) {
		s_handle control_handle = s_array_handle(&renderer->controls, (u32)i);
		se_sdf_runtime_control* control = s_array_get(&renderer->controls, control_handle);
		if (control && strcmp(control->name, name) == 0) {
			return control_handle;
		}
	}
	return SE_SDF_CONTROL_NULL;
}

static void se_sdf_runtime_make_control_uniform_name(
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

static b8 se_sdf_runtime_control_value_equals(
	const se_sdf_control_type type,
	const se_sdf_runtime_control_value* a,
	const se_sdf_runtime_control_value* b
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

static b8 se_sdf_runtime_control_assign_value(
	se_sdf_runtime_control* control,
	const se_sdf_runtime_control_value* value
) {
	if (!control || !value) {
		return 0;
	}
	if (se_sdf_runtime_control_value_equals(control->type, &control->value, value)) {
		return 0;
	}
	control->value = *value;
	control->dirty = 1;
	return 1;
}

static se_sdf_runtime_control* se_sdf_runtime_get_or_create_control(
	se_sdf_runtime_renderer* renderer,
	const char* name,
	const se_sdf_control_type type,
	se_sdf_control_handle* out_handle
) {
	if (!renderer || !name || name[0] == '\0') {
		return NULL;
	}
	se_sdf_control_handle handle = se_sdf_runtime_find_control_handle(renderer, name);
	b8 created = 0;
	if (handle == SE_SDF_CONTROL_NULL) {
		handle = s_array_increment(&renderer->controls);
		created = 1;
	}
	se_sdf_runtime_control* control = s_array_get(&renderer->controls, handle);
	if (!control) {
		return NULL;
	}
	if (handle == SE_SDF_CONTROL_NULL || control->name[0] == '\0') {
		memset(control, 0, sizeof(*control));
		strncpy(control->name, name, sizeof(control->name) - 1);
		control->name[sizeof(control->name) - 1] = '\0';
		se_sdf_runtime_make_control_uniform_name(handle, name, control->uniform_name, sizeof(control->uniform_name));
		control->cached_uniform_location = -1;
	}
	if (control->type != type) {
		se_sdf_runtime_control_clear_binding(control);
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

static se_sdf_runtime_control* se_sdf_runtime_control_from_handle(
	se_sdf_runtime_renderer* renderer,
	const se_sdf_control_handle control,
	const se_sdf_control_type expected_type
) {
	if (!renderer || control == SE_SDF_CONTROL_NULL) {
		return NULL;
	}
	se_sdf_runtime_control* control_ptr = s_array_get(&renderer->controls, control);
	if (!control_ptr || control_ptr->type != expected_type) {
		return NULL;
	}
	return control_ptr;
}

static void se_sdf_runtime_control_clear_binding(se_sdf_runtime_control* control) {
	if (!control) {
		return;
	}
	memset(&control->binding, 0, sizeof(control->binding));
	control->has_binding = 0;
}

static void se_sdf_runtime_sync_control_bindings(se_sdf_runtime_renderer* renderer) {
	if (!renderer) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&renderer->controls); ++i) {
		se_sdf_runtime_control* control = s_array_get(&renderer->controls, s_array_handle(&renderer->controls, (u32)i));
		if (!control || !control->has_binding) {
			continue;
		}

		switch (control->type) {
			case SE_SDF_CONTROL_FLOAT:
				if (control->binding.f) {
					se_sdf_runtime_control_value v = {.f = *control->binding.f};
					se_sdf_runtime_control_assign_value(control, &v);
				}
				else se_sdf_runtime_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_VEC2:
				if (control->binding.vec2) {
					se_sdf_runtime_control_value v = {.vec2 = *control->binding.vec2};
					se_sdf_runtime_control_assign_value(control, &v);
				}
				else se_sdf_runtime_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_VEC3:
				if (control->binding.vec3) {
					se_sdf_runtime_control_value v = {.vec3 = *control->binding.vec3};
					se_sdf_runtime_control_assign_value(control, &v);
				}
				else se_sdf_runtime_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_VEC4:
				if (control->binding.vec4) {
					se_sdf_runtime_control_value v = {.vec4 = *control->binding.vec4};
					se_sdf_runtime_control_assign_value(control, &v);
				}
				else se_sdf_runtime_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_INT:
				if (control->binding.i) {
					se_sdf_runtime_control_value v = {.i = *control->binding.i};
					se_sdf_runtime_control_assign_value(control, &v);
				}
				else se_sdf_runtime_control_clear_binding(control);
				break;
			case SE_SDF_CONTROL_MAT4:
				if (control->binding.mat4) {
					se_sdf_runtime_control_value v = {.mat4 = *control->binding.mat4};
					se_sdf_runtime_control_assign_value(control, &v);
				}
				else se_sdf_runtime_control_clear_binding(control);
				break;
			default:
				se_sdf_runtime_control_clear_binding(control);
				break;
		}
	}
}

static f32 se_sdf_runtime_abs_or_one(const f32 value) {
	const f32 abs_value = fabsf(value);
	return abs_value < 0.000001f ? 1.0f : abs_value;
}

static void se_sdf_runtime_node_init_control_trs(se_sdf_runtime_node* node) {
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
		se_sdf_runtime_abs_or_one(node->transform.m[0][0]),
		se_sdf_runtime_abs_or_one(node->transform.m[1][1]),
		se_sdf_runtime_abs_or_one(node->transform.m[2][2])
	);
	node->control_trs_initialized = 1;
}

static void se_sdf_runtime_node_apply_control_trs(se_sdf_runtime_node* node) {
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

static b8 se_sdf_runtime_bind_node_target(
	se_sdf_runtime_renderer* renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_control_handle control,
	const se_sdf_runtime_node_bind_target target
) {
	if (!renderer || scene == SE_SDF_SCENE_NULL || node == SE_SDF_NODE_NULL || control == SE_SDF_CONTROL_NULL) {
		return 0;
	}
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(renderer, control, SE_SDF_CONTROL_VEC3);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_runtime_node* node_ptr = se_sdf_runtime_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	se_sdf_runtime_node_init_control_trs(node_ptr);

	for (sz i = 0; i < s_array_get_size(&renderer->node_bindings); ++i) {
		se_sdf_runtime_node_binding* binding = s_array_get(&renderer->node_bindings, s_array_handle(&renderer->node_bindings, (u32)i));
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

	se_sdf_runtime_node_binding binding = {0};
	binding.scene = scene;
	binding.node = node;
	binding.control = control;
	binding.target = target;
	s_array_add(&renderer->node_bindings, binding);
	renderer->built = 0;
	return 1;
}

static b8 se_sdf_runtime_apply_node_bindings(se_sdf_runtime_renderer* renderer) {
	if (!renderer) {
		return 0;
	}
	b8 changed = 0;

	for (sz i = 0; i < s_array_get_size(&renderer->node_bindings); ++i) {
		se_sdf_runtime_node_binding* binding = s_array_get(&renderer->node_bindings, s_array_handle(&renderer->node_bindings, (u32)i));
		if (!binding) {
			continue;
		}

		se_sdf_runtime_control* control = se_sdf_runtime_control_from_handle(renderer, binding->control, SE_SDF_CONTROL_VEC3);
		if (!control) {
			continue;
		}
		se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(binding->scene);
		if (!scene_ptr) {
			continue;
		}
		se_sdf_runtime_node* node_ptr = se_sdf_runtime_node_from_handle(scene_ptr, binding->scene, binding->node);
		if (!node_ptr) {
			continue;
		}

		se_sdf_runtime_node_init_control_trs(node_ptr);
		b8 node_changed = 0;
		switch (binding->target) {
			case SE_SDF_RUNTIME_NODE_BIND_TRANSLATION:
				node_changed = (node_ptr->control_translation.x != control->value.vec3.x) ||
					(node_ptr->control_translation.y != control->value.vec3.y) ||
					(node_ptr->control_translation.z != control->value.vec3.z);
				if (node_changed) {
					node_ptr->control_translation = control->value.vec3;
				}
				break;
			case SE_SDF_RUNTIME_NODE_BIND_ROTATION:
				node_changed = (node_ptr->control_rotation.x != control->value.vec3.x) ||
					(node_ptr->control_rotation.y != control->value.vec3.y) ||
					(node_ptr->control_rotation.z != control->value.vec3.z);
				if (node_changed) {
					node_ptr->control_rotation = control->value.vec3;
				}
				break;
			case SE_SDF_RUNTIME_NODE_BIND_SCALE:
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
			se_sdf_runtime_node_apply_control_trs(node_ptr);
			changed = 1;
		}
	}
	return changed;
}

static b8 se_sdf_runtime_apply_primitive_param_float(
	se_sdf_runtime_node* node,
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

static b8 se_sdf_runtime_apply_primitive_bindings(se_sdf_runtime_renderer* renderer) {
	if (!renderer) {
		return 0;
	}
	b8 changed = 0;

	for (sz i = 0; i < s_array_get_size(&renderer->primitive_bindings); ++i) {
		se_sdf_runtime_primitive_binding* binding = s_array_get(&renderer->primitive_bindings, s_array_handle(&renderer->primitive_bindings, (u32)i));
		if (!binding) {
			continue;
		}
		se_sdf_runtime_control* control = se_sdf_runtime_control_from_handle(renderer, binding->control, SE_SDF_CONTROL_FLOAT);
		if (!control) {
			continue;
		}
		se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(binding->scene);
		if (!scene_ptr) {
			continue;
		}
		se_sdf_runtime_node* node_ptr = se_sdf_runtime_node_from_handle(scene_ptr, binding->scene, binding->node);
		if (!node_ptr) {
			continue;
		}
		if (se_sdf_runtime_apply_primitive_param_float(node_ptr, binding->param, control->value.f)) {
			changed = 1;
		}
	}
	return changed;
}

static void se_sdf_runtime_apply_renderer_shading_bindings(se_sdf_runtime_renderer* renderer) {
	if (!renderer) {
		return;
	}
	se_sdf_runtime_control* control = NULL;

	control = se_sdf_runtime_control_from_handle(renderer, renderer->material_base_color_control, SE_SDF_CONTROL_VEC3);
	if (control) {
		renderer->material.base_color = control->value.vec3;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->lighting_direction_control, SE_SDF_CONTROL_VEC3);
	if (control) {
		renderer->lighting_direction = control->value.vec3;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->lighting_color_control, SE_SDF_CONTROL_VEC3);
	if (control) {
		renderer->lighting_color = control->value.vec3;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->fog_color_control, SE_SDF_CONTROL_VEC3);
	if (control) {
		renderer->fog_color = control->value.vec3;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->fog_density_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->fog_density = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_band_levels_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.band_levels = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_rim_power_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.rim_power = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_rim_strength_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.rim_strength = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_fill_strength_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.fill_strength = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_back_strength_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.back_strength = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_checker_scale_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.checker_scale = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_checker_strength_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.checker_strength = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_ground_blend_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.ground_blend = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_desaturate_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.desaturate = control->value.f;
	}

	control = se_sdf_runtime_control_from_handle(renderer, renderer->stylized_gamma_control, SE_SDF_CONTROL_FLOAT);
	if (control) {
		renderer->stylized.gamma = control->value.f;
	}
}

static const char* se_sdf_runtime_control_glsl_type(const se_sdf_control_type type) {
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

static b8 se_sdf_runtime_emit_control_uniform_declarations(
	se_sdf_runtime_renderer* renderer,
	se_sdf_string* out
) {
	if (!renderer || !out) {
		return 0;
	}

	sz declared_count = 0;
	for (sz i = 0; i < s_array_get_size(&renderer->controls); ++i) {
		const se_sdf_runtime_control* control = s_array_get(
			&renderer->controls,
			s_array_handle(&renderer->controls, (u32)i)
		);
		if (!control || control->uniform_name[0] == '\0') {
			continue;
		}
		const char* glsl_type = se_sdf_runtime_control_glsl_type(control->type);
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

static const char* se_sdf_runtime_find_node_binding_uniform_name(
	se_sdf_runtime_renderer* renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_runtime_node_bind_target target
) {
	if (!renderer || scene == SE_SDF_SCENE_NULL || node == SE_SDF_NODE_NULL) {
		return NULL;
	}

	for (sz i = 0; i < s_array_get_size(&renderer->node_bindings); ++i) {
		const se_sdf_runtime_node_binding* binding = s_array_get(
			&renderer->node_bindings,
			s_array_handle(&renderer->node_bindings, (u32)i)
		);
		if (!binding || binding->scene != scene || binding->node != node || binding->target != target) {
			continue;
		}
		const se_sdf_runtime_control* control = s_array_get(&renderer->controls, binding->control);
		if (!control || control->type != SE_SDF_CONTROL_VEC3 || control->uniform_name[0] == '\0') {
			continue;
		}
		return control->uniform_name;
	}
	return NULL;
}

static const char* se_sdf_runtime_find_primitive_binding_uniform_name(
	se_sdf_runtime_renderer* renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_primitive_param param
) {
	if (!renderer || scene == SE_SDF_SCENE_NULL || node == SE_SDF_NODE_NULL) {
		return NULL;
	}

	for (sz i = 0; i < s_array_get_size(&renderer->primitive_bindings); ++i) {
		const se_sdf_runtime_primitive_binding* binding = s_array_get(
			&renderer->primitive_bindings,
			s_array_handle(&renderer->primitive_bindings, (u32)i)
		);
		if (!binding || binding->scene != scene || binding->node != node || binding->param != param) {
			continue;
		}
		const se_sdf_runtime_control* control = s_array_get(&renderer->controls, binding->control);
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_runtime_control* control = se_sdf_runtime_get_or_create_control(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !default_value) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_runtime_control* control = se_sdf_runtime_get_or_create_control(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !default_value) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_runtime_control* control = se_sdf_runtime_get_or_create_control(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !default_value) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_runtime_control* control = se_sdf_runtime_get_or_create_control(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_runtime_control* control = se_sdf_runtime_get_or_create_control(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !default_value) {
		return SE_SDF_CONTROL_NULL;
	}
	se_sdf_control_handle handle = SE_SDF_CONTROL_NULL;
	se_sdf_runtime_control* control = se_sdf_runtime_get_or_create_control(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_FLOAT);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_runtime_control_value v = {.f = value};
	se_sdf_runtime_control_assign_value(control_ptr, &v);
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC2);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_runtime_control_value v = {.vec2 = *value};
	se_sdf_runtime_control_assign_value(control_ptr, &v);
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC3);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_runtime_control_value v = {.vec3 = *value};
	se_sdf_runtime_control_assign_value(control_ptr, &v);
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_VEC4);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_runtime_control_value v = {.vec4 = *value};
	se_sdf_runtime_control_assign_value(control_ptr, &v);
	return 1;
}

b8 se_sdf_control_set_int(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const i32 value
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_INT);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_runtime_control_value v = {.i = value};
	se_sdf_runtime_control_assign_value(control_ptr, &v);
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_MAT4);
	if (!control_ptr) {
		return 0;
	}
	se_sdf_runtime_control_value v = {.mat4 = *value};
	se_sdf_runtime_control_assign_value(control_ptr, &v);
	return 1;
}

b8 se_sdf_control_bind_ptr_float(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control,
	const f32* value_ptr
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
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
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	se_sdf_runtime_control* control_ptr = se_sdf_runtime_control_from_handle(
		renderer_ptr, control, SE_SDF_CONTROL_MAT4);
	if (!control_ptr) {
		return 0;
	}
	control_ptr->binding.mat4 = value_ptr;
	control_ptr->has_binding = (value_ptr != NULL);
	return 1;
}

b8 se_sdf_control_bind_node_translation(
	const se_sdf_renderer_handle renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return se_sdf_runtime_bind_node_target(
		renderer_ptr, scene, node, control, SE_SDF_RUNTIME_NODE_BIND_TRANSLATION);
}

b8 se_sdf_control_bind_node_rotation(
	const se_sdf_renderer_handle renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return se_sdf_runtime_bind_node_target(
		renderer_ptr, scene, node, control, SE_SDF_RUNTIME_NODE_BIND_ROTATION);
}

b8 se_sdf_control_bind_node_scale(
	const se_sdf_renderer_handle renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return se_sdf_runtime_bind_node_target(
		renderer_ptr, scene, node, control, SE_SDF_RUNTIME_NODE_BIND_SCALE);
}

static b8 se_sdf_runtime_bind_primitive_param_float(
	se_sdf_runtime_renderer* renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_primitive_param param,
	const se_sdf_control_handle control
) {
	if (!renderer || scene == SE_SDF_SCENE_NULL || node == SE_SDF_NODE_NULL || control == SE_SDF_CONTROL_NULL) {
		return 0;
	}
	if (!se_sdf_runtime_control_from_handle(renderer, control, SE_SDF_CONTROL_FLOAT)) {
		return 0;
	}
	se_sdf_runtime_scene* scene_ptr = se_sdf_runtime_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_runtime_node* node_ptr = se_sdf_runtime_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr || node_ptr->is_group) {
		return 0;
	}

	for (sz i = 0; i < s_array_get_size(&renderer->primitive_bindings); ++i) {
		se_sdf_runtime_primitive_binding* binding = s_array_get(&renderer->primitive_bindings, s_array_handle(&renderer->primitive_bindings, (u32)i));
		if (!binding) {
			continue;
		}
		if (binding->scene == scene && binding->node == node && binding->param == param) {
			binding->control = control;
			renderer->built = 0;
			return 1;
		}
	}

	se_sdf_runtime_primitive_binding binding = {0};
	binding.scene = scene;
	binding.node = node;
	binding.param = param;
	binding.control = control;
	s_array_add(&renderer->primitive_bindings, binding);
	renderer->built = 0;
	return 1;
}

b8 se_sdf_control_bind_primitive_param_float(
	const se_sdf_renderer_handle renderer,
	const se_sdf_scene_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_primitive_param param,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	return se_sdf_runtime_bind_primitive_param_float(renderer_ptr, scene, node, param, control);
}

static b8 se_sdf_runtime_bind_renderer_float_control(
	se_sdf_runtime_renderer* renderer,
	se_sdf_control_handle* target_handle,
	const se_sdf_control_handle control
) {
	if (!renderer || !target_handle) {
		return 0;
	}
	if (!se_sdf_runtime_control_from_handle(renderer, control, SE_SDF_CONTROL_FLOAT)) {
		return 0;
	}
	*target_handle = control;
	return 1;
}

b8 se_sdf_control_bind_material_base_color(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !se_sdf_runtime_control_from_handle(renderer_ptr, control, SE_SDF_CONTROL_VEC3)) {
		return 0;
	}
	renderer_ptr->material_base_color_control = control;
	return 1;
}

b8 se_sdf_control_bind_lighting_direction(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !se_sdf_runtime_control_from_handle(renderer_ptr, control, SE_SDF_CONTROL_VEC3)) {
		return 0;
	}
	renderer_ptr->lighting_direction_control = control;
	return 1;
}

b8 se_sdf_control_bind_lighting_color(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !se_sdf_runtime_control_from_handle(renderer_ptr, control, SE_SDF_CONTROL_VEC3)) {
		return 0;
	}
	renderer_ptr->lighting_color_control = control;
	return 1;
}

b8 se_sdf_control_bind_fog_color(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	if (!renderer_ptr || !se_sdf_runtime_control_from_handle(renderer_ptr, control, SE_SDF_CONTROL_VEC3)) {
		return 0;
	}
	renderer_ptr->fog_color_control = control;
	return 1;
}

b8 se_sdf_control_bind_fog_density(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->fog_density_control, control);
}

b8 se_sdf_control_bind_stylized_band_levels(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_band_levels_control, control);
}

b8 se_sdf_control_bind_stylized_rim_power(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_rim_power_control, control);
}

b8 se_sdf_control_bind_stylized_rim_strength(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_rim_strength_control, control);
}

b8 se_sdf_control_bind_stylized_fill_strength(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_fill_strength_control, control);
}

b8 se_sdf_control_bind_stylized_back_strength(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_back_strength_control, control);
}

b8 se_sdf_control_bind_stylized_checker_scale(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_checker_scale_control, control);
}

b8 se_sdf_control_bind_stylized_checker_strength(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_checker_strength_control, control);
}

b8 se_sdf_control_bind_stylized_ground_blend(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_ground_blend_control, control);
}

b8 se_sdf_control_bind_stylized_desaturate(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_desaturate_control, control);
}

b8 se_sdf_control_bind_stylized_gamma(
	const se_sdf_renderer_handle renderer,
	const se_sdf_control_handle control
) {
	se_sdf_runtime_renderer* renderer_ptr = se_sdf_runtime_renderer_from_handle(renderer);
	return se_sdf_runtime_bind_renderer_float_control(renderer_ptr, &renderer_ptr->stylized_gamma_control, control);
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
		"uniform vec3 u_camera_position;\n"
		"uniform vec3 u_camera_target;\n"
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
		"	vec2 uv = v_uv * 2.0 - 1.0;\n"
		"	uv.x *= res.x / res.y;\n"
		"	uv.y *= -1.0;\n"
		"	vec3 ro = u_camera_position;\n"
		"	vec3 ta = u_camera_target;\n"
		"	vec3 f = normalize(ta - ro);\n"
		"	vec3 r = normalize(cross(f, vec3(0.0, 1.0, 0.0)));\n"
		"	vec3 u = cross(r, f);\n"
		"	vec3 rd = normalize(f + uv.x * r + uv.y * u);\n"
		"	float t = 0.0;\n"
		"	int used_steps = 0;\n"
		"	bool hit = false;\n"
		"	vec3 p = ro;\n"
		"	for (int i = 0; i < 512; ++i) {\n"
		"		if (i >= u_quality_max_steps) break;\n"
		"		p = ro + rd * t;\n"
		"		float d = %s(p);\n"
		"		used_steps = i + 1;\n"
		"		if (abs(d) < u_quality_hit_epsilon) {\n"
		"			hit = true;\n"
		"			break;\n"
		"		}\n"
		"		t += d;\n"
		"		if (t > u_quality_max_distance) break;\n"
		"	}\n"
		"	vec3 color = u_fog_color;\n"
		"	if (hit) {\n"
		"		color = se_sdf_stylized(p, rd);\n"
		"		float fog = exp(-u_fog_density * t * t);\n"
		"		color = mix(u_fog_color, color, fog);\n"
		"	}\n"
		"	if (u_debug_show_normals != 0 && hit) {\n"
		"		color = 0.5 + 0.5 * se_sdf_normal(p);\n"
		"	}\n"
		"	if (u_debug_show_distance != 0) {\n"
		"		float dnorm = clamp(t / max(u_quality_max_distance, 0.0001), 0.0, 1.0);\n"
		"		color = vec3(dnorm);\n"
		"	}\n"
		"	if (u_debug_show_steps != 0) {\n"
		"		float snorm = float(used_steps) / max(float(u_quality_max_steps), 1.0);\n"
		"		color = vec3(snorm, snorm * snorm, 1.0 - snorm);\n"
		"	}\n"
		"	color = pow(max(color, vec3(0.0)), vec3(1.0 / max(u_stylized_gamma, 0.0001)));\n"
		"	FragColor = vec4(color, 1.0);\n"
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
            se_sdf_string_append(out, "min(%s, %s) /* smooth_union_reserved */", d1, d2);
            break;
        case SE_SDF_OP_SMOOTH_INTERSECTION:
            se_sdf_string_append(out, "max(%s, %s) /* smooth_intersection_reserved */", d1, d2);
            break;
        case SE_SDF_OP_SMOOTH_SUBTRACTION:
            se_sdf_string_append(out, "max(%s, -%s) /* smooth_subtraction_reserved */", d1, d2);
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
	const se_sdf_runtime_node_bind_target target
) {
	if (!obj || !se_sdf_codegen_active_renderer) {
		return NULL;
	}
	if (obj->source_scene == SE_SDF_SCENE_NULL || obj->source_node == SE_SDF_NODE_NULL) {
		return NULL;
	}
	return se_sdf_runtime_find_node_binding_uniform_name(
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
	return se_sdf_runtime_find_primitive_binding_uniform_name(
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
    se_sdf_string* out, se_sdf_operation op,
    const char* d1, const char* d2, u32 indent
) {
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
            se_sdf_string_append(out, "%s = min(%s, %s); // smooth_union_reserved\n", d1, d1, d2);
            break;
        case SE_SDF_OP_SMOOTH_INTERSECTION:
            se_sdf_string_append(out, "%s = max(%s, %s); // smooth_intersection_reserved\n", d1, d1, d2);
            break;
        case SE_SDF_OP_SMOOTH_SUBTRACTION:
            se_sdf_string_append(out, "%s = max(%s, -%s); // smooth_subtraction_reserved\n", d1, d1, d2);
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
	const char* translation_uniform = se_sdf_codegen_get_node_bind_uniform(obj, SE_SDF_RUNTIME_NODE_BIND_TRANSLATION);
	const char* rotation_uniform = se_sdf_codegen_get_node_bind_uniform(obj, SE_SDF_RUNTIME_NODE_BIND_ROTATION);
	const char* scale_uniform = se_sdf_codegen_get_node_bind_uniform(obj, SE_SDF_RUNTIME_NODE_BIND_SCALE);
	const b8 has_dynamic_trs = (translation_uniform != NULL) || (rotation_uniform != NULL) || (scale_uniform != NULL);

	if (has_dynamic_trs) {
		const s_vec3 base_translation = s_vec3(
			obj->transform.m[3][0],
			obj->transform.m[3][1],
			obj->transform.m[3][2]
		);
		const s_vec3 base_rotation = s_vec3(0.0f, 0.0f, 0.0f);
		const s_vec3 base_scale = s_vec3(
			se_sdf_runtime_abs_or_one(obj->transform.m[0][0]),
			se_sdf_runtime_abs_or_one(obj->transform.m[1][1]),
			se_sdf_runtime_abs_or_one(obj->transform.m[2][2])
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
        se_sdf_emit_operation(out, obj->operation, result_var, rhs_result, indent);
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
