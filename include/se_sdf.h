// Syphax-Engine - Ougi Washi

#ifndef SE_SDF_H
#define SE_SDF_H

#include "syphax/s_types.h"
#include "syphax/s_array.h"

#define SE_SDF_MAX_STRING_LENGTH 4096

typedef s_handle se_sdf_scene_handle;
typedef s_handle se_sdf_node_handle;
typedef s_handle se_sdf_material_handle;
typedef s_handle se_sdf_renderer_handle;
typedef s_handle se_sdf_control_handle;

#define SE_SDF_SCENE_NULL ((se_sdf_scene_handle)S_HANDLE_NULL)
#define SE_SDF_NODE_NULL ((se_sdf_node_handle)S_HANDLE_NULL)
#define SE_SDF_MATERIAL_NULL ((se_sdf_material_handle)S_HANDLE_NULL)
#define SE_SDF_RENDERER_NULL ((se_sdf_renderer_handle)S_HANDLE_NULL)
#define SE_SDF_CONTROL_NULL ((se_sdf_control_handle)S_HANDLE_NULL)

typedef enum {
	SE_SDF_NONE,  // Container/group node with no shape
	SE_SDF_SPHERE,
	SE_SDF_BOX,
	SE_SDF_ROUND_BOX,
	SE_SDF_BOX_FRAME,
	SE_SDF_TORUS,
	SE_SDF_CAPPED_TORUS,
	SE_SDF_LINK,
	SE_SDF_CYLINDER,
	SE_SDF_CONE,
	SE_SDF_CONE_INFINITE,
	SE_SDF_PLANE,
	SE_SDF_HEX_PRISM,
	SE_SDF_CAPSULE,
	SE_SDF_VERTICAL_CAPSULE,
	SE_SDF_CAPPED_CYLINDER,
	SE_SDF_CAPPED_CYLINDER_ARBITRARY,
	SE_SDF_ROUNDED_CYLINDER,
	SE_SDF_CAPPED_CONE,
	SE_SDF_CAPPED_CONE_ARBITRARY,
	SE_SDF_SOLID_ANGLE,
	SE_SDF_CUT_SPHERE,
	SE_SDF_CUT_HOLLOW_SPHERE,
	SE_SDF_DEATH_STAR,
	SE_SDF_ROUND_CONE,
	SE_SDF_ROUND_CONE_ARBITRARY,
	SE_SDF_VESICA_SEGMENT,
	SE_SDF_RHOMBUS,
	SE_SDF_OCTAHEDRON,
	SE_SDF_OCTAHEDRON_BOUND,
	SE_SDF_PYRAMID,
	SE_SDF_TRIANGLE,
	SE_SDF_QUAD
} se_sdf_object_type;

typedef struct {
	b8 active;
	f32 scale;
	f32 offset;
	f32 frequency;
} se_sdf_noise;

typedef enum {
	SE_SDF_OP_NONE,
	SE_SDF_OP_UNION,
	SE_SDF_OP_INTERSECTION,
	SE_SDF_OP_SUBTRACTION,
	SE_SDF_OP_SMOOTH_UNION,
	SE_SDF_OP_SMOOTH_INTERSECTION,
	SE_SDF_OP_SMOOTH_SUBTRACTION
} se_sdf_operation;

typedef enum {
	SE_SDF_PRIMITIVE_SPHERE = SE_SDF_SPHERE,
	SE_SDF_PRIMITIVE_BOX = SE_SDF_BOX,
	SE_SDF_PRIMITIVE_ROUND_BOX = SE_SDF_ROUND_BOX,
	SE_SDF_PRIMITIVE_BOX_FRAME = SE_SDF_BOX_FRAME,
	SE_SDF_PRIMITIVE_TORUS = SE_SDF_TORUS,
	SE_SDF_PRIMITIVE_CAPPED_TORUS = SE_SDF_CAPPED_TORUS,
	SE_SDF_PRIMITIVE_LINK = SE_SDF_LINK,
	SE_SDF_PRIMITIVE_CYLINDER = SE_SDF_CYLINDER,
	SE_SDF_PRIMITIVE_CONE = SE_SDF_CONE,
	SE_SDF_PRIMITIVE_CONE_INFINITE = SE_SDF_CONE_INFINITE,
	SE_SDF_PRIMITIVE_PLANE = SE_SDF_PLANE,
	SE_SDF_PRIMITIVE_HEX_PRISM = SE_SDF_HEX_PRISM,
	SE_SDF_PRIMITIVE_CAPSULE = SE_SDF_CAPSULE,
	SE_SDF_PRIMITIVE_VERTICAL_CAPSULE = SE_SDF_VERTICAL_CAPSULE,
	SE_SDF_PRIMITIVE_CAPPED_CYLINDER = SE_SDF_CAPPED_CYLINDER,
	SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY = SE_SDF_CAPPED_CYLINDER_ARBITRARY,
	SE_SDF_PRIMITIVE_ROUNDED_CYLINDER = SE_SDF_ROUNDED_CYLINDER,
	SE_SDF_PRIMITIVE_CAPPED_CONE = SE_SDF_CAPPED_CONE,
	SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY = SE_SDF_CAPPED_CONE_ARBITRARY,
	SE_SDF_PRIMITIVE_SOLID_ANGLE = SE_SDF_SOLID_ANGLE,
	SE_SDF_PRIMITIVE_CUT_SPHERE = SE_SDF_CUT_SPHERE,
	SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE = SE_SDF_CUT_HOLLOW_SPHERE,
	SE_SDF_PRIMITIVE_DEATH_STAR = SE_SDF_DEATH_STAR,
	SE_SDF_PRIMITIVE_ROUND_CONE = SE_SDF_ROUND_CONE,
	SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY = SE_SDF_ROUND_CONE_ARBITRARY,
	SE_SDF_PRIMITIVE_VESICA_SEGMENT = SE_SDF_VESICA_SEGMENT,
	SE_SDF_PRIMITIVE_RHOMBUS = SE_SDF_RHOMBUS,
	SE_SDF_PRIMITIVE_OCTAHEDRON = SE_SDF_OCTAHEDRON,
	SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND = SE_SDF_OCTAHEDRON_BOUND,
	SE_SDF_PRIMITIVE_PYRAMID = SE_SDF_PYRAMID,
	SE_SDF_PRIMITIVE_TRIANGLE = SE_SDF_TRIANGLE,
	SE_SDF_PRIMITIVE_QUAD = SE_SDF_QUAD
} se_sdf_primitive_type;

typedef struct { f32 radius; } se_sdf_sphere_desc;
typedef struct { s_vec3 size; } se_sdf_box_desc;
typedef struct { s_vec3 size; f32 roundness; } se_sdf_round_box_desc;
typedef struct { s_vec3 size; f32 thickness; } se_sdf_box_frame_desc;
typedef struct { s_vec2 radii; } se_sdf_torus_desc;
typedef struct { s_vec2 axis_sin_cos; f32 major_radius; f32 minor_radius; } se_sdf_capped_torus_desc;
typedef struct { f32 half_length; f32 outer_radius; f32 inner_radius; } se_sdf_link_desc;
typedef struct { s_vec3 axis_and_radius; } se_sdf_cylinder_desc;
typedef struct { s_vec2 angle_sin_cos; f32 height; } se_sdf_cone_desc;
typedef struct { s_vec2 angle_sin_cos; } se_sdf_cone_infinite_desc;
typedef struct { s_vec3 normal; f32 offset; } se_sdf_plane_desc;
typedef struct { s_vec2 half_size; } se_sdf_hex_prism_desc;
typedef struct { s_vec3 a; s_vec3 b; f32 radius; } se_sdf_capsule_desc;
typedef struct { f32 height; f32 radius; } se_sdf_vertical_capsule_desc;
typedef struct { f32 radius; f32 half_height; } se_sdf_capped_cylinder_desc;
typedef struct { s_vec3 a; s_vec3 b; f32 radius; } se_sdf_capped_cylinder_arbitrary_desc;
typedef struct { f32 outer_radius; f32 corner_radius; f32 half_height; } se_sdf_rounded_cylinder_desc;
typedef struct { f32 height; f32 radius_base; f32 radius_top; } se_sdf_capped_cone_desc;
typedef struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } se_sdf_capped_cone_arbitrary_desc;
typedef struct { s_vec2 angle_sin_cos; f32 radius; } se_sdf_solid_angle_desc;
typedef struct { f32 radius; f32 cut_height; } se_sdf_cut_sphere_desc;
typedef struct { f32 radius; f32 cut_height; f32 thickness; } se_sdf_cut_hollow_sphere_desc;
typedef struct { f32 radius_a; f32 radius_b; f32 separation; } se_sdf_death_star_desc;
typedef struct { f32 radius_base; f32 radius_top; f32 height; } se_sdf_round_cone_desc;
typedef struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } se_sdf_round_cone_arbitrary_desc;
typedef struct { s_vec3 a; s_vec3 b; f32 width; } se_sdf_vesica_segment_desc;
typedef struct { f32 length_a; f32 length_b; f32 height; f32 corner_radius; } se_sdf_rhombus_desc;
typedef struct { f32 size; } se_sdf_octahedron_desc;
typedef struct { f32 size; } se_sdf_octahedron_bound_desc;
typedef struct { f32 height; } se_sdf_pyramid_desc;
typedef struct { s_vec3 a; s_vec3 b; s_vec3 c; } se_sdf_triangle_desc;
typedef struct { s_vec3 a; s_vec3 b; s_vec3 c; s_vec3 d; } se_sdf_quad_desc;

typedef struct {
	se_sdf_primitive_type type;
	union {
		se_sdf_sphere_desc sphere;
		se_sdf_box_desc box;
		se_sdf_round_box_desc round_box;
		se_sdf_box_frame_desc box_frame;
		se_sdf_torus_desc torus;
		se_sdf_capped_torus_desc capped_torus;
		se_sdf_link_desc link;
		se_sdf_cylinder_desc cylinder;
		se_sdf_cone_desc cone;
		se_sdf_cone_infinite_desc cone_infinite;
		se_sdf_plane_desc plane;
		se_sdf_hex_prism_desc hex_prism;
		se_sdf_capsule_desc capsule;
		se_sdf_vertical_capsule_desc vertical_capsule;
		se_sdf_capped_cylinder_desc capped_cylinder;
		se_sdf_capped_cylinder_arbitrary_desc capped_cylinder_arbitrary;
		se_sdf_rounded_cylinder_desc rounded_cylinder;
		se_sdf_capped_cone_desc capped_cone;
		se_sdf_capped_cone_arbitrary_desc capped_cone_arbitrary;
		se_sdf_solid_angle_desc solid_angle;
		se_sdf_cut_sphere_desc cut_sphere;
		se_sdf_cut_hollow_sphere_desc cut_hollow_sphere;
		se_sdf_death_star_desc death_star;
		se_sdf_round_cone_desc round_cone;
		se_sdf_round_cone_arbitrary_desc round_cone_arbitrary;
		se_sdf_vesica_segment_desc vesica_segment;
		se_sdf_rhombus_desc rhombus;
		se_sdf_octahedron_desc octahedron;
		se_sdf_octahedron_bound_desc octahedron_bound;
		se_sdf_pyramid_desc pyramid;
		se_sdf_triangle_desc triangle;
		se_sdf_quad_desc quad;
	};
} se_sdf_primitive_desc;

typedef struct se_sdf_object {
	s_mat4 transform;
	se_sdf_object_type type;
	union {
		struct { f32 radius; } sphere;
		struct { s_vec3 size; } box;
		struct { s_vec3 size; f32 roundness; } round_box;
		struct { s_vec3 size; f32 thickness; } box_frame;
		struct { s_vec2 radii; } torus;
		struct { s_vec2 axis_sin_cos; f32 major_radius; f32 minor_radius; } capped_torus;
		struct { f32 half_length; f32 outer_radius; f32 inner_radius; } link;
		struct { s_vec3 axis_and_radius; } cylinder;
		struct { s_vec2 angle_sin_cos; f32 height; } cone;
		struct { s_vec2 angle_sin_cos; } cone_infinite;
		struct { s_vec3 normal; f32 offset; } plane;
		struct { s_vec2 half_size; } hex_prism;
		struct { s_vec3 a; s_vec3 b; f32 radius; } capsule;
		struct { f32 height; f32 radius; } vertical_capsule;
		struct { f32 radius; f32 half_height; } capped_cylinder;
		struct { s_vec3 a; s_vec3 b; f32 radius; } capped_cylinder_arbitrary;
		struct { f32 outer_radius; f32 corner_radius; f32 half_height; } rounded_cylinder;
		struct { f32 height; f32 radius_base; f32 radius_top; } capped_cone;
		struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } capped_cone_arbitrary;
		struct { s_vec2 angle_sin_cos; f32 radius; } solid_angle;
		struct { f32 radius; f32 cut_height; } cut_sphere;
		struct { f32 radius; f32 cut_height; f32 thickness; } cut_hollow_sphere;
		struct { f32 radius_a; f32 radius_b; f32 separation; } death_star;
		struct { f32 radius_base; f32 radius_top; f32 height; } round_cone;
		struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } round_cone_arbitrary;
		struct { s_vec3 a; s_vec3 b; f32 width; } vesica_segment;
		struct { f32 length_a; f32 length_b; f32 height; f32 corner_radius; } rhombus;
		struct { f32 size; } octahedron;
		struct { f32 size; } octahedron_bound;
		struct { f32 height; } pyramid;
		struct { s_vec3 a; s_vec3 b; s_vec3 c; } triangle;
		struct { s_vec3 a; s_vec3 b; s_vec3 c; s_vec3 d; } quad;
	};			
	se_sdf_operation operation;
	se_sdf_noise noise;
	s_array(struct se_sdf_object, children);
} se_sdf_object;

static inline void se_sdf_add_children(se_sdf_object* parent, sz count, se_sdf_object* children) {
	for (sz i = 0; i < count; ++i) s_array_add(&parent->children, children[i]);
}

static inline se_sdf_object se_sdf_make(se_sdf_object obj) {
	s_array_init(&obj.children);
	return obj;
}

// Shape macros: se_sdf_sphere(transform, .radius = value, ...)
#define se_sdf_sphere(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_SPHERE, .transform = (xform), __VA_ARGS__})
#define se_sdf_box(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_BOX, .transform = (xform), __VA_ARGS__})
#define se_sdf_round_box(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_ROUND_BOX, .transform = (xform), __VA_ARGS__})
#define se_sdf_box_frame(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_BOX_FRAME, .transform = (xform), __VA_ARGS__})
#define se_sdf_torus(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_TORUS, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_torus(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_TORUS, .transform = (xform), __VA_ARGS__})
#define se_sdf_link(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_LINK, .transform = (xform), __VA_ARGS__})
#define se_sdf_cylinder(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CYLINDER, .transform = (xform), __VA_ARGS__})
#define se_sdf_cone(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CONE, .transform = (xform), __VA_ARGS__})
#define se_sdf_cone_inf(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CONE_INFINITE, .transform = (xform), __VA_ARGS__})
#define se_sdf_plane(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_PLANE, .transform = (xform), __VA_ARGS__})
#define se_sdf_hex_prism(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_HEX_PRISM, .transform = (xform), __VA_ARGS__})
#define se_sdf_capsule(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPSULE, .transform = (xform), __VA_ARGS__})
#define se_sdf_vert_capsule(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_VERTICAL_CAPSULE, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_cyl(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_CYLINDER, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_cyl_arb(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_CYLINDER_ARBITRARY, .transform = (xform), __VA_ARGS__})
#define se_sdf_rounded_cyl(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_ROUNDED_CYLINDER, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_cone(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_CONE, .transform = (xform), __VA_ARGS__})
#define se_sdf_capped_cone_arb(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CAPPED_CONE_ARBITRARY, .transform = (xform), __VA_ARGS__})
#define se_sdf_solid_angle(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_SOLID_ANGLE, .transform = (xform), __VA_ARGS__})
#define se_sdf_cut_sphere(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CUT_SPHERE, .transform = (xform), __VA_ARGS__})
#define se_sdf_cut_hollow_sphere(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_CUT_HOLLOW_SPHERE, .transform = (xform), __VA_ARGS__})
#define se_sdf_death_star(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_DEATH_STAR, .transform = (xform), __VA_ARGS__})
#define se_sdf_round_cone(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_ROUND_CONE, .transform = (xform), __VA_ARGS__})
#define se_sdf_round_cone_arb(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_ROUND_CONE_ARBITRARY, .transform = (xform), __VA_ARGS__})
#define se_sdf_vesica(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_VESICA_SEGMENT, .transform = (xform), __VA_ARGS__})
#define se_sdf_rhombus(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_RHOMBUS, .transform = (xform), __VA_ARGS__})
#define se_sdf_octa(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_OCTAHEDRON, .transform = (xform), __VA_ARGS__})
#define se_sdf_octa_bound(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_OCTAHEDRON_BOUND, .transform = (xform), __VA_ARGS__})
#define se_sdf_pyramid(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_PYRAMID, .transform = (xform), __VA_ARGS__})
#define se_sdf_tri(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_TRIANGLE, .transform = (xform), __VA_ARGS__})
#define se_sdf_quad(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_QUAD, .transform = (xform), __VA_ARGS__})

// Container/group node (no shape, just holds children with operation)
#define se_sdf_group(xform, ...) se_sdf_make((se_sdf_object){.type = SE_SDF_NONE, .transform = (xform), __VA_ARGS__})

// Add nodes with operation: se_sdf_nodes(parent, SE_SDF_OP_UNION, child1, child2)
// Alternative names considered: subshapes, operands, elements, branches
// 'nodes' is used as it fits scene graph terminology
#define se_sdf_nodes(parent, op, ...) do { \
	se_sdf_object _n[] = { __VA_ARGS__ }; \
	(parent).operation = (op); \
	se_sdf_add_children(&(parent), sizeof(_n)/sizeof(_n[0]), _n); \
} while(0)

// Convenience constants
#define se_sdf_union SE_SDF_OP_UNION
#define se_sdf_intersection SE_SDF_OP_INTERSECTION
#define se_sdf_subtraction SE_SDF_OP_SUBTRACTION
#define se_sdf_smooth_union SE_SDF_OP_SMOOTH_UNION
#define se_sdf_smooth_intersection SE_SDF_OP_SMOOTH_INTERSECTION
#define se_sdf_smooth_subtraction SE_SDF_OP_SMOOTH_SUBTRACTION
#define se_sdf_noop SE_SDF_OP_NONE

// Default transform (identity)
#define se_sdf_default s_mat4_identity

typedef struct {
	sz initial_node_capacity;
} se_sdf_scene_desc;

typedef struct {
	s_mat4 transform;
	se_sdf_operation operation;
} se_sdf_node_group_desc;

typedef struct {
	s_mat4 transform;
	se_sdf_operation operation;
	se_sdf_primitive_desc primitive;
} se_sdf_node_primitive_desc;

#define SE_SDF_SCENE_DESC_DEFAULTS ((se_sdf_scene_desc){ .initial_node_capacity = 0 })
#define SE_SDF_NODE_GROUP_DESC_DEFAULTS ((se_sdf_node_group_desc){ .transform = s_mat4_identity, .operation = SE_SDF_OP_UNION })

extern se_sdf_scene_handle se_sdf_scene_create(const se_sdf_scene_desc* desc);
extern void se_sdf_scene_destroy(se_sdf_scene_handle scene);
extern void se_sdf_scene_clear(se_sdf_scene_handle scene);
extern b8 se_sdf_scene_set_root(se_sdf_scene_handle scene, se_sdf_node_handle node);
extern se_sdf_node_handle se_sdf_scene_get_root(se_sdf_scene_handle scene);
extern b8 se_sdf_scene_validate(se_sdf_scene_handle scene, char* error_message, sz error_message_size);

extern se_sdf_node_handle se_sdf_node_create_primitive(se_sdf_scene_handle scene, const se_sdf_node_primitive_desc* desc);
extern se_sdf_node_handle se_sdf_node_create_group(se_sdf_scene_handle scene, const se_sdf_node_group_desc* desc);
extern void se_sdf_node_destroy(se_sdf_scene_handle scene, se_sdf_node_handle node);
extern b8 se_sdf_node_add_child(se_sdf_scene_handle scene, se_sdf_node_handle parent, se_sdf_node_handle child);
extern b8 se_sdf_node_remove_child(se_sdf_scene_handle scene, se_sdf_node_handle parent, se_sdf_node_handle child);
extern b8 se_sdf_node_set_operation(se_sdf_scene_handle scene, se_sdf_node_handle node, se_sdf_operation operation);
extern b8 se_sdf_node_set_transform(se_sdf_scene_handle scene, se_sdf_node_handle node, const s_mat4* transform);
extern s_mat4 se_sdf_node_get_transform(se_sdf_scene_handle scene, se_sdf_node_handle node);
extern s_mat4 se_sdf_transform_trs(f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz);
extern s_mat4 se_sdf_transform_grid_cell(i32 index, i32 columns, i32 rows, f32 spacing, f32 y, f32 yaw, f32 sx, f32 sy, f32 sz);
extern se_sdf_node_handle se_sdf_node_spawn_primitive(
	se_sdf_scene_handle scene,
	se_sdf_node_handle parent,
	const se_sdf_primitive_desc* primitive,
	const s_mat4* transform,
	se_sdf_operation operation
);
extern b8 se_sdf_scene_build_single_object_preset(
	se_sdf_scene_handle scene,
	const se_sdf_primitive_desc* primitive,
	const s_mat4* transform,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_object
);
extern b8 se_sdf_scene_build_grid_preset(
	se_sdf_scene_handle scene,
	const se_sdf_primitive_desc* primitive,
	i32 columns,
	i32 rows,
	f32 spacing,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_first
);
extern b8 se_sdf_scene_build_primitive_gallery_preset(
	se_sdf_scene_handle scene,
	i32 columns,
	i32 rows,
	f32 spacing,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_hero
);
extern b8 se_sdf_scene_build_orbit_showcase_preset(
	se_sdf_scene_handle scene,
	const se_sdf_primitive_desc* center_primitive,
	const se_sdf_primitive_desc* orbit_primitive,
	i32 orbit_count,
	f32 orbit_radius,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_center
);

typedef struct {
	sz shader_source_capacity;
	b8 auto_rebuild_on_scene_change;
	b8 enable_shader_debug_output;
} se_sdf_renderer_desc;

typedef struct {
	b8 success;
	char stage[64];
	char message[512];
} se_sdf_build_diagnostics;

typedef struct {
	i32 max_steps;
	f32 hit_epsilon;
	f32 max_distance;
	b8 enable_shadows;
	i32 shadow_steps;
	f32 shadow_strength;
} se_sdf_raymarch_quality;

typedef struct {
	b8 show_normals;
	b8 show_distance;
	b8 show_steps;
	b8 freeze_time;
} se_sdf_renderer_debug;

typedef struct {
	s_vec2 resolution;
	f32 time_seconds;
	s_vec2 mouse_normalized;
	b8 has_camera_override;
	s_vec3 camera_position;
	s_vec3 camera_target;
} se_sdf_frame_desc;

typedef enum {
	SE_SDF_SHADING_STYLIZED,
	SE_SDF_SHADING_LIT_PBR,
	SE_SDF_SHADING_UNLIT
} se_sdf_shading_model;

typedef struct {
	se_sdf_shading_model model;
	s_vec3 base_color;
	s_vec3 emissive_color;
	f32 roughness;
	f32 metalness;
	f32 opacity;
} se_sdf_material_desc;

typedef struct {
	f32 band_levels;
	f32 rim_power;
	f32 rim_strength;
	f32 fill_strength;
	f32 back_strength;
	f32 checker_scale;
	f32 checker_strength;
	f32 ground_blend;
	f32 desaturate;
	f32 gamma;
} se_sdf_stylized_desc;

typedef enum {
	SE_SDF_CONTROL_FLOAT,
	SE_SDF_CONTROL_VEC2,
	SE_SDF_CONTROL_VEC3,
	SE_SDF_CONTROL_VEC4,
	SE_SDF_CONTROL_INT,
	SE_SDF_CONTROL_MAT4
} se_sdf_control_type;

typedef enum {
	SE_SDF_PRIMITIVE_PARAM_RADIUS,
	SE_SDF_PRIMITIVE_PARAM_RADIUS_A,
	SE_SDF_PRIMITIVE_PARAM_RADIUS_B,
	SE_SDF_PRIMITIVE_PARAM_HEIGHT,
	SE_SDF_PRIMITIVE_PARAM_THICKNESS,
	SE_SDF_PRIMITIVE_PARAM_SIZE_X,
	SE_SDF_PRIMITIVE_PARAM_SIZE_Y,
	SE_SDF_PRIMITIVE_PARAM_SIZE_Z
} se_sdf_primitive_param;

#define SE_SDF_RAYMARCH_QUALITY_DEFAULTS ((se_sdf_raymarch_quality){ \
	.max_steps = 96, \
	.hit_epsilon = 0.0015f, \
	.max_distance = 72.0f, \
	.enable_shadows = 1, \
	.shadow_steps = 32, \
	.shadow_strength = 0.65f \
})

#define SE_SDF_RENDERER_DEBUG_DEFAULTS ((se_sdf_renderer_debug){ \
	.show_normals = 0, \
	.show_distance = 0, \
	.show_steps = 0, \
	.freeze_time = 0 \
})

#define SE_SDF_RENDERER_DESC_DEFAULTS ((se_sdf_renderer_desc){ \
	.shader_source_capacity = (256u * 1024u), \
	.auto_rebuild_on_scene_change = 1, \
	.enable_shader_debug_output = 0 \
})

#define SE_SDF_FRAME_DESC_DEFAULTS ((se_sdf_frame_desc){ \
	.resolution = (s_vec2){ .x = 1.0f, .y = 1.0f }, \
	.time_seconds = 0.0f, \
	.mouse_normalized = (s_vec2){ .x = 0.0f, .y = 0.0f }, \
	.has_camera_override = 0, \
	.camera_position = (s_vec3){ .x = 0.0f, .y = 0.0f, .z = 0.0f }, \
	.camera_target = (s_vec3){ .x = 0.0f, .y = 0.0f, .z = 0.0f } \
})

#define SE_SDF_MATERIAL_DESC_DEFAULTS ((se_sdf_material_desc){ \
	.model = SE_SDF_SHADING_STYLIZED, \
	.base_color = (s_vec3){ .x = 0.70f, .y = 0.72f, .z = 0.75f }, \
	.emissive_color = (s_vec3){ .x = 0.0f, .y = 0.0f, .z = 0.0f }, \
	.roughness = 0.6f, \
	.metalness = 0.0f, \
	.opacity = 1.0f \
})

#define SE_SDF_STYLIZED_DESC_DEFAULTS ((se_sdf_stylized_desc){ \
	.band_levels = 4.0f, \
	.rim_power = 2.2f, \
	.rim_strength = 0.55f, \
	.fill_strength = 0.42f, \
	.back_strength = 0.30f, \
	.checker_scale = 2.0f, \
	.checker_strength = 0.20f, \
	.ground_blend = 0.45f, \
	.desaturate = 0.08f, \
	.gamma = 2.2f \
})

extern se_sdf_renderer_handle se_sdf_renderer_create(const se_sdf_renderer_desc* desc);
extern void se_sdf_renderer_destroy(se_sdf_renderer_handle renderer);
extern b8 se_sdf_renderer_set_scene(se_sdf_renderer_handle renderer, se_sdf_scene_handle scene);
extern b8 se_sdf_renderer_set_quality(se_sdf_renderer_handle renderer, const se_sdf_raymarch_quality* quality);
extern b8 se_sdf_renderer_set_debug(se_sdf_renderer_handle renderer, const se_sdf_renderer_debug* debug);
extern b8 se_sdf_renderer_set_material(se_sdf_renderer_handle renderer, const se_sdf_material_desc* material);
extern b8 se_sdf_renderer_set_stylized(se_sdf_renderer_handle renderer, const se_sdf_stylized_desc* stylized);
extern se_sdf_stylized_desc se_sdf_renderer_get_stylized(se_sdf_renderer_handle renderer);
extern b8 se_sdf_renderer_set_base_color(se_sdf_renderer_handle renderer, const s_vec3* base_color);
extern b8 se_sdf_renderer_set_light_rig(
	se_sdf_renderer_handle renderer,
	const s_vec3* light_direction,
	const s_vec3* light_color,
	const s_vec3* fog_color,
	f32 fog_density
);
extern b8 se_sdf_renderer_build(se_sdf_renderer_handle renderer);
extern b8 se_sdf_renderer_rebuild_if_dirty(se_sdf_renderer_handle renderer);
extern b8 se_sdf_renderer_render(se_sdf_renderer_handle renderer, const se_sdf_frame_desc* frame);
extern const char* se_sdf_renderer_get_generated_fragment_source(se_sdf_renderer_handle renderer);
extern sz se_sdf_renderer_get_generated_fragment_source_size(se_sdf_renderer_handle renderer);
extern b8 se_sdf_renderer_dump_shader_source(se_sdf_renderer_handle renderer, const char* path);
extern sz se_sdf_renderer_get_last_uniform_write_count(se_sdf_renderer_handle renderer);
extern sz se_sdf_renderer_get_total_uniform_write_count(se_sdf_renderer_handle renderer);
extern const char* se_sdf_control_get_uniform_name(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern se_sdf_build_diagnostics se_sdf_renderer_get_last_build_diagnostics(se_sdf_renderer_handle renderer);

extern se_sdf_control_handle se_sdf_control_define_float(se_sdf_renderer_handle renderer, const char* name, f32 default_value);
extern se_sdf_control_handle se_sdf_control_define_vec2(se_sdf_renderer_handle renderer, const char* name, const s_vec2* default_value);
extern se_sdf_control_handle se_sdf_control_define_vec3(se_sdf_renderer_handle renderer, const char* name, const s_vec3* default_value);
extern se_sdf_control_handle se_sdf_control_define_vec4(se_sdf_renderer_handle renderer, const char* name, const s_vec4* default_value);
extern se_sdf_control_handle se_sdf_control_define_int(se_sdf_renderer_handle renderer, const char* name, i32 default_value);
extern se_sdf_control_handle se_sdf_control_define_mat4(se_sdf_renderer_handle renderer, const char* name, const s_mat4* default_value);
extern b8 se_sdf_control_set_float(se_sdf_renderer_handle renderer, se_sdf_control_handle control, f32 value);
extern b8 se_sdf_control_set_vec2(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const s_vec2* value);
extern b8 se_sdf_control_set_vec3(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const s_vec3* value);
extern b8 se_sdf_control_set_vec4(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const s_vec4* value);
extern b8 se_sdf_control_set_int(se_sdf_renderer_handle renderer, se_sdf_control_handle control, i32 value);
extern b8 se_sdf_control_set_mat4(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const s_mat4* value);
extern b8 se_sdf_control_bind_ptr_float(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const f32* value_ptr);
extern b8 se_sdf_control_bind_ptr_vec2(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const s_vec2* value_ptr);
extern b8 se_sdf_control_bind_ptr_vec3(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const s_vec3* value_ptr);
extern b8 se_sdf_control_bind_ptr_vec4(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const s_vec4* value_ptr);
extern b8 se_sdf_control_bind_ptr_int(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const i32* value_ptr);
extern b8 se_sdf_control_bind_ptr_mat4(se_sdf_renderer_handle renderer, se_sdf_control_handle control, const s_mat4* value_ptr);
extern b8 se_sdf_control_bind_node_translation(se_sdf_renderer_handle renderer, se_sdf_scene_handle scene, se_sdf_node_handle node, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_node_rotation(se_sdf_renderer_handle renderer, se_sdf_scene_handle scene, se_sdf_node_handle node, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_node_scale(se_sdf_renderer_handle renderer, se_sdf_scene_handle scene, se_sdf_node_handle node, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_primitive_param_float(se_sdf_renderer_handle renderer, se_sdf_scene_handle scene, se_sdf_node_handle node, se_sdf_primitive_param param, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_material_base_color(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_lighting_direction(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_lighting_color(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_fog_color(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_fog_density(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_band_levels(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_rim_power(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_rim_strength(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_fill_strength(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_back_strength(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_checker_scale(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_checker_strength(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_ground_blend(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_desaturate(se_sdf_renderer_handle renderer, se_sdf_control_handle control);
extern b8 se_sdf_control_bind_stylized_gamma(se_sdf_renderer_handle renderer, se_sdf_control_handle control);

// ============================================================================
// Shader String Generation
// ============================================================================

typedef struct {
	char* data;
	sz capacity;
	sz size;
	b8 oom;
} se_sdf_string;

void se_sdf_string_init(se_sdf_string* str, sz capacity);
void se_sdf_string_free(se_sdf_string* str);
void se_sdf_string_append(se_sdf_string* str, const char* fmt, ...);
void se_sdf_string_append_vec2(se_sdf_string* str, s_vec2 v);
void se_sdf_string_append_vec3(se_sdf_string* str, s_vec3 v);

typedef struct {
	u32 next_id;
} se_sdf_symbol_allocator;

void se_sdf_symbol_allocator_init(se_sdf_symbol_allocator* allocator);
void se_sdf_symbol_allocator_reset(se_sdf_symbol_allocator* allocator);
u32 se_sdf_symbol_allocator_next(se_sdf_symbol_allocator* allocator);
void se_sdf_symbol_allocator_make_name(se_sdf_symbol_allocator* allocator, const char* prefix, char* out_name, sz out_name_size);

b8 se_sdf_codegen_emit_fragment_prelude(se_sdf_string* out);
b8 se_sdf_codegen_emit_uniform_block(se_sdf_string* out);
b8 se_sdf_codegen_emit_map_stub(se_sdf_string* out, const char* map_func_name);
b8 se_sdf_codegen_emit_shading_stub(se_sdf_string* out);
b8 se_sdf_codegen_emit_fragment_main(se_sdf_string* out, const char* map_func_name);
b8 se_sdf_codegen_emit_fragment_scaffold(se_sdf_string* out, const char* map_func_name);

// Shape generators - each returns GLSL distance function code
void se_sdf_gen_sphere(se_sdf_string* out, const char* p, f32 radius);
void se_sdf_gen_box(se_sdf_string* out, const char* p, s_vec3 size);
void se_sdf_gen_round_box(se_sdf_string* out, const char* p, s_vec3 size, f32 roundness);
void se_sdf_gen_box_frame(se_sdf_string* out, const char* p, s_vec3 size, f32 thickness);
void se_sdf_gen_torus(se_sdf_string* out, const char* p, f32 major_r, f32 minor_r);
void se_sdf_gen_capped_torus(se_sdf_string* out, const char* p, s_vec2 sc, f32 ra, f32 rb);
void se_sdf_gen_link(se_sdf_string* out, const char* p, f32 le, f32 r1, f32 r2);
void se_sdf_gen_cylinder(se_sdf_string* out, const char* p, s_vec3 c);
void se_sdf_gen_cone(se_sdf_string* out, const char* p, s_vec2 sc, f32 h);
void se_sdf_gen_cone_inf(se_sdf_string* out, const char* p, s_vec2 sc);
void se_sdf_gen_plane(se_sdf_string* out, const char* p, s_vec3 n, f32 h);
void se_sdf_gen_hex_prism(se_sdf_string* out, const char* p, s_vec2 h);
void se_sdf_gen_capsule(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r);
void se_sdf_gen_vert_capsule(se_sdf_string* out, const char* p, f32 h, f32 r);
void se_sdf_gen_capped_cyl(se_sdf_string* out, const char* p, f32 r, f32 hh);
void se_sdf_gen_capped_cyl_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r);
void se_sdf_gen_rounded_cyl(se_sdf_string* out, const char* p, f32 ra, f32 rb, f32 hh);
void se_sdf_gen_capped_cone(se_sdf_string* out, const char* p, f32 h, f32 r1, f32 r2);
void se_sdf_gen_capped_cone_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 ra, f32 rb);
void se_sdf_gen_solid_angle(se_sdf_string* out, const char* p, s_vec2 sc, f32 r);
void se_sdf_gen_cut_sphere(se_sdf_string* out, const char* p, f32 r, f32 ch);
void se_sdf_gen_cut_hollow_sphere(se_sdf_string* out, const char* p, f32 r, f32 ch, f32 t);
void se_sdf_gen_death_star(se_sdf_string* out, const char* p, f32 ra, f32 rb, f32 d);
void se_sdf_gen_round_cone(se_sdf_string* out, const char* p, f32 r1, f32 r2, f32 h);
void se_sdf_gen_round_cone_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r1, f32 r2);
void se_sdf_gen_vesica(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 w);
void se_sdf_gen_rhombus(se_sdf_string* out, const char* p, f32 la, f32 lb, f32 h, f32 r);
void se_sdf_gen_octa(se_sdf_string* out, const char* p, f32 s);
void se_sdf_gen_octa_bound(se_sdf_string* out, const char* p, f32 s);
void se_sdf_gen_pyramid(se_sdf_string* out, const char* p, f32 h);
void se_sdf_gen_tri(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, s_vec3 c);
void se_sdf_gen_quad(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, s_vec3 c, s_vec3 d);

// Generate transform application: (inverse(mat) * vec4(p, 1.0)).xyz
void se_sdf_gen_transform(se_sdf_string* out, const char* p_var, s_mat4 transform, const char* result_var);

// Generate operation combining two distances
void se_sdf_gen_operation(se_sdf_string* out, se_sdf_operation op, const char* d1, const char* d2);

// Main recursive generator (emits GLSL function-body statements ending with return)
void se_sdf_object_to_string(se_sdf_string* out, se_sdf_object* obj, const char* p_var);

// Helper to generate full distance function
void se_sdf_generate_distance_function(se_sdf_string* out, se_sdf_object* root, const char* func_name);

// Examples :

static inline void test_sdf(void) {
	se_sdf_object root = se_sdf_sphere(s_mat4_identity, .sphere.radius = 1.0f);
	
	s_vec3 a = {0, 0, 0}, b = {1, 0, 0}, c = {0, 1, 0};
	se_sdf_object child1 = se_sdf_box(s_mat4_identity, .box.size = (s_vec3){1, 1, 1});
	se_sdf_object child2 = se_sdf_tri(s_mat4_identity, .triangle.a = a, .triangle.b = b, .triangle.c = c);
	
	se_sdf_nodes(root, se_sdf_union, child1, child2);
	
	// Example using a group/container node:
	se_sdf_object scene = se_sdf_group(s_mat4_identity);  // No shape, just a container
	se_sdf_object sphere_a = se_sdf_sphere(s_mat4_identity, .sphere.radius = 1.0f);
	se_sdf_object sphere_b = se_sdf_sphere(s_mat4_identity, .sphere.radius = 0.5f);
	se_sdf_nodes(scene, se_sdf_union, sphere_a, sphere_b);
}

static inline void test_sdf_shader_generation(void) {
	// Build scene
	se_sdf_object root = se_sdf_group(s_mat4_identity);
	se_sdf_object sphere = se_sdf_sphere(s_mat4_identity, .sphere.radius = 1.0f);
	se_sdf_object box = se_sdf_box(s_mat4_identity, .box.size = (s_vec3){0.5f, 0.5f, 0.5f});
	se_sdf_nodes(root, se_sdf_union, sphere, box);
	
	// Generate shader
	se_sdf_string shader;
	se_sdf_string_init(&shader, 8192);
	se_sdf_generate_distance_function(&shader, &root, "map");
	
	// shader.data now contains the GLSL distance function
	// printf("%s\n", shader.data);
	
	se_sdf_string_free(&shader);
}

#endif // SE_SDF_H
