// Syphax-Engine - Ougi Washi

#ifndef SE_SDF_H
#define SE_SDF_H

#include "syphax/s_array.h"
#include "syphax/s_types.h"
#include "se_camera.h"
#include "se_physics.h"

typedef struct s_json s_json;

#define SE_SDF_MAX_STRING_LENGTH 4096

typedef s_handle se_sdf_handle;
typedef s_handle se_sdf_node_handle;
typedef s_handle se_sdf_physics_handle;
typedef s_handle se_sdf_material_handle;
typedef s_handle se_sdf_renderer_handle;

#define SE_SDF_NULL ((se_sdf_handle)S_HANDLE_NULL)
#define SE_SDF_NODE_NULL ((se_sdf_node_handle)S_HANDLE_NULL)
#define SE_SDF_PHYSICS_NULL ((se_sdf_physics_handle)S_HANDLE_NULL)
#define SE_SDF_MATERIAL_NULL ((se_sdf_material_handle)S_HANDLE_NULL)
#define SE_SDF_RENDERER_NULL ((se_sdf_renderer_handle)S_HANDLE_NULL)

typedef enum {
	SE_SDF_PRIMITIVE_SPHERE = 1,
	SE_SDF_PRIMITIVE_BOX = 2,
	SE_SDF_PRIMITIVE_ROUND_BOX = 3,
	SE_SDF_PRIMITIVE_BOX_FRAME = 4,
	SE_SDF_PRIMITIVE_TORUS = 5,
	SE_SDF_PRIMITIVE_CAPPED_TORUS = 6,
	SE_SDF_PRIMITIVE_LINK = 7,
	SE_SDF_PRIMITIVE_CYLINDER = 8,
	SE_SDF_PRIMITIVE_CONE = 9,
	SE_SDF_PRIMITIVE_CONE_INFINITE = 10,
	SE_SDF_PRIMITIVE_PLANE = 11,
	SE_SDF_PRIMITIVE_HEX_PRISM = 12,
	SE_SDF_PRIMITIVE_CAPSULE = 13,
	SE_SDF_PRIMITIVE_VERTICAL_CAPSULE = 14,
	SE_SDF_PRIMITIVE_CAPPED_CYLINDER = 15,
	SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY = 16,
	SE_SDF_PRIMITIVE_ROUNDED_CYLINDER = 17,
	SE_SDF_PRIMITIVE_CAPPED_CONE = 18,
	SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY = 19,
	SE_SDF_PRIMITIVE_SOLID_ANGLE = 20,
	SE_SDF_PRIMITIVE_CUT_SPHERE = 21,
	SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE = 22,
	SE_SDF_PRIMITIVE_DEATH_STAR = 23,
	SE_SDF_PRIMITIVE_ROUND_CONE = 24,
	SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY = 25,
	SE_SDF_PRIMITIVE_VESICA_SEGMENT = 26,
	SE_SDF_PRIMITIVE_RHOMBUS = 27,
	SE_SDF_PRIMITIVE_OCTAHEDRON = 28,
	SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND = 29,
	SE_SDF_PRIMITIVE_PYRAMID = 30,
	SE_SDF_PRIMITIVE_TRIANGLE = 31,
	SE_SDF_PRIMITIVE_QUAD = 32
} se_sdf_primitive_type;

typedef struct {
	b8 active;
	f32 scale;
	f32 offset;
	f32 frequency;
} se_sdf_noise;

#define SE_SDF_NOISE_DEFAULTS ((se_sdf_noise){ \
	.active = 0, \
	.scale = 0.0f, \
	.offset = 0.0f, \
	.frequency = 1.0f \
})

typedef enum {
	SE_SDF_OP_NONE,
	SE_SDF_OP_UNION,
	SE_SDF_OP_INTERSECTION,
	SE_SDF_OP_SUBTRACTION,
	SE_SDF_OP_SMOOTH_UNION,
	SE_SDF_OP_SMOOTH_INTERSECTION,
	SE_SDF_OP_SMOOTH_SUBTRACTION
} se_sdf_operation;

#define SE_SDF_OPERATION_AMOUNT_DEFAULT 0.5f

typedef enum {
	SE_SDF_NODE_GROUP,
	SE_SDF_NODE_PRIMITIVE,
	SE_SDF_NODE_REF
} se_sdf_node_type;

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

typedef struct {
	sz initial_node_capacity;
} se_sdf_desc;

typedef struct {
	s_vec3 position;
	s_vec3 normal;
	f32 distance;
	se_sdf_handle sdf;
	se_sdf_node_handle node;
	b8 hit;
} se_sdf_surface_hit;

typedef struct {
	s_mat4 transform;
	se_sdf_operation operation;
	f32 operation_amount;
} se_sdf_node_group_desc;

typedef struct {
	s_mat4 transform;
	se_sdf_operation operation;
	f32 operation_amount;
	se_sdf_primitive_desc primitive;
	se_sdf_noise noise;
} se_sdf_node_primitive_desc;

typedef struct {
	se_sdf_handle source;
	s_mat4 transform;
	se_sdf_operation operation;
	f32 operation_amount;
} se_sdf_node_ref_desc;

typedef struct {
	s_vec3 min;
	s_vec3 max;
	s_vec3 center;
	f32 radius;
	b8 valid;
	b8 has_unbounded_primitives;
} se_sdf_bounds;

typedef struct {
	s_vec3 view_direction;
	s_vec3 up;
	f32 padding;
	f32 min_radius;
	f32 min_distance;
	f32 near_margin;
	f32 far_margin;
	b8 update_clip_planes;
} se_sdf_camera_align_desc;

typedef struct {
	u32 lod_count;
	u32 lod_resolutions[4];
	u32 brick_size;
	u32 brick_border;
	f32 max_distance_scale;
	b8 force_rebuild;
} se_sdf_prepare_desc;

typedef struct {
	u32 resolution;
	se_texture_handle texture;
	s_vec3 voxel_size;
	f32 max_distance;
	b8 valid;
} se_sdf_lod_volume;

typedef struct {
	b8 pending;
	b8 applied;
	b8 failed;
	b8 ready;
	se_result result;
} se_sdf_prepare_status;

typedef struct {
	se_sdf_bounds bounds;
	u32 lod_count;
	se_sdf_lod_volume lods[4];
} se_sdf_bake_result;

typedef struct {
	se_sdf_prepare_desc prepare;
} se_sdf_bake_desc;

#define SE_SDF_DESC_DEFAULTS ((se_sdf_desc){ .initial_node_capacity = 0 })
#define SE_SDF_NODE_GROUP_DESC_DEFAULTS ((se_sdf_node_group_desc){ .transform = s_mat4_identity, .operation = SE_SDF_OP_UNION, .operation_amount = SE_SDF_OPERATION_AMOUNT_DEFAULT })
#define SE_SDF_NODE_REF_DESC_DEFAULTS ((se_sdf_node_ref_desc){ .source = SE_SDF_NULL, .transform = s_mat4_identity, .operation = SE_SDF_OP_UNION, .operation_amount = SE_SDF_OPERATION_AMOUNT_DEFAULT })
#define SE_SDF_BOUNDS_DEFAULTS ((se_sdf_bounds){ \
	.min = (s_vec3){ .x = 0.0f, .y = 0.0f, .z = 0.0f }, \
	.max = (s_vec3){ .x = 0.0f, .y = 0.0f, .z = 0.0f }, \
	.center = (s_vec3){ .x = 0.0f, .y = 0.0f, .z = 0.0f }, \
	.radius = 0.0f, \
	.valid = 0, \
	.has_unbounded_primitives = 0 \
})
#define SE_SDF_CAMERA_ALIGN_DESC_DEFAULTS ((se_sdf_camera_align_desc){ \
	.view_direction = (s_vec3){ .x = 1.35f, .y = 0.75f, .z = 1.35f }, \
	.up = (s_vec3){ .x = 0.0f, .y = 1.0f, .z = 0.0f }, \
	.padding = 1.20f, \
	.min_radius = 0.50f, \
	.min_distance = 1.00f, \
	.near_margin = 0.10f, \
	.far_margin = 8.00f, \
	.update_clip_planes = 1 \
})
#define SE_SDF_PREPARE_DESC_DEFAULTS ((se_sdf_prepare_desc){ \
	.lod_count = 4u, \
	.lod_resolutions = { 128u, 64u, 32u, 16u }, \
	.brick_size = 32u, \
	.brick_border = 1u, \
	.max_distance_scale = 1.0f, \
	.force_rebuild = 0 \
})

extern s_mat4 se_sdf_transform(s_vec3 translation, s_vec3 rotation, s_vec3 scale);
extern se_sdf_handle se_sdf_create(const se_sdf_desc* desc);
extern se_object_2d_handle se_sdf_to_object_2d(se_sdf_handle sdf);
extern se_object_3d_handle se_sdf_to_object_3d(se_sdf_handle sdf);
extern void se_sdf_destroy(se_sdf_handle sdf);
extern void se_sdf_clear(se_sdf_handle sdf);
extern b8 se_sdf_set_root(se_sdf_handle sdf, se_sdf_node_handle node);
extern se_sdf_node_handle se_sdf_get_root(se_sdf_handle sdf);
extern u64 se_sdf_get_generation(se_sdf_handle sdf);
extern b8 se_sdf_validate(se_sdf_handle sdf, char* error_message, sz error_message_size);
extern b8 se_sdf_prepare(se_sdf_handle sdf, const se_sdf_prepare_desc* desc);
extern b8 se_sdf_prepare_async(se_sdf_handle sdf, const se_sdf_prepare_desc* desc);
extern b8 se_sdf_prepare_poll(se_sdf_handle sdf, se_sdf_prepare_status* out_status);
extern b8 se_sdf_bake(se_sdf_handle sdf, const se_sdf_bake_desc* desc, se_sdf_bake_result* out_result);
extern b8 se_sdf_sample_distance(se_sdf_handle sdf, const s_vec3* point, f32* out_distance, se_sdf_node_handle* out_node);
extern b8 se_sdf_project_point(se_sdf_handle sdf, const s_vec3* point, const s_vec3* direction, f32 max_distance, se_sdf_surface_hit* out_hit);
extern b8 se_sdf_raycast(se_sdf_handle sdf, const s_vec3* origin, const s_vec3* direction, f32 max_distance, se_sdf_surface_hit* out_hit);
extern b8 se_sdf_sample_height(se_sdf_handle sdf, f32 x, f32 z, f32 start_y, f32 max_distance, se_sdf_surface_hit* out_hit);
extern b8 se_sdf_calculate_bounds(se_sdf_handle sdf, se_sdf_bounds* out_bounds);
extern b8 se_sdf_align_camera(
	se_sdf_handle sdf,
	se_camera_handle camera,
	const se_sdf_camera_align_desc* desc,
	se_sdf_bounds* out_bounds
);
extern s_json* se_sdf_to_json(se_sdf_handle sdf);
extern b8 se_sdf_from_json(se_sdf_handle sdf, const s_json* root);
extern b8 se_sdf_from_json_file(se_sdf_handle sdf, const c8* path);

extern se_sdf_node_handle se_sdf_node_create_primitive(se_sdf_handle sdf, const se_sdf_node_primitive_desc* desc);
extern se_sdf_node_handle se_sdf_node_create_group(se_sdf_handle sdf, const se_sdf_node_group_desc* desc);
extern se_sdf_node_handle se_sdf_node_create_ref(se_sdf_handle sdf, const se_sdf_node_ref_desc* desc);
extern u32 se_sdf_get_node_count(se_sdf_handle sdf);
extern se_sdf_node_handle se_sdf_get_node(se_sdf_handle sdf, u32 index);
extern se_sdf_node_type se_sdf_node_get_type(se_sdf_handle sdf, se_sdf_node_handle node);
extern se_sdf_node_handle se_sdf_node_get_parent(se_sdf_handle sdf, se_sdf_node_handle node);
extern u32 se_sdf_node_get_child_count(se_sdf_handle sdf, se_sdf_node_handle node);
extern se_sdf_node_handle se_sdf_node_get_child(se_sdf_handle sdf, se_sdf_node_handle node, u32 index);
extern se_sdf_operation se_sdf_node_get_operation(se_sdf_handle sdf, se_sdf_node_handle node);
extern b8 se_sdf_node_set_operation_amount(se_sdf_handle sdf, se_sdf_node_handle node, f32 operation_amount);
extern f32 se_sdf_node_get_operation_amount(se_sdf_handle sdf, se_sdf_node_handle node);
extern se_sdf_handle se_sdf_node_get_ref_source(se_sdf_handle sdf, se_sdf_node_handle node);
extern b8 se_sdf_node_set_primitive(se_sdf_handle sdf, se_sdf_node_handle node, const se_sdf_primitive_desc* primitive);
extern b8 se_sdf_node_get_primitive(se_sdf_handle sdf, se_sdf_node_handle node, se_sdf_primitive_desc* out_primitive);
extern b8 se_sdf_node_has_color(se_sdf_handle sdf, se_sdf_node_handle node);
extern void se_sdf_node_destroy(se_sdf_handle sdf, se_sdf_node_handle node);
extern b8 se_sdf_node_add_child(se_sdf_handle sdf, se_sdf_node_handle parent, se_sdf_node_handle child);
extern b8 se_sdf_node_remove_child(se_sdf_handle sdf, se_sdf_node_handle parent, se_sdf_node_handle child);
extern b8 se_sdf_node_set_operation(se_sdf_handle sdf, se_sdf_node_handle node, se_sdf_operation operation);
extern b8 se_sdf_node_set_transform(se_sdf_handle sdf, se_sdf_node_handle node, const s_mat4* transform);
extern s_mat4 se_sdf_node_get_transform(se_sdf_handle sdf, se_sdf_node_handle node);
extern b8 se_sdf_node_set_noise(se_sdf_handle sdf, se_sdf_node_handle node, const se_sdf_noise* noise);
extern se_sdf_noise se_sdf_node_get_noise(se_sdf_handle sdf, se_sdf_node_handle node);
extern b8 se_sdf_node_set_color(se_sdf_handle sdf, se_sdf_node_handle node, const s_vec3* color);
extern b8 se_sdf_node_get_color(se_sdf_handle sdf, se_sdf_node_handle node, s_vec3* out_color);
extern se_sdf_node_handle se_sdf_node_spawn_primitive(
	se_sdf_handle sdf,
	se_sdf_node_handle parent,
	const se_sdf_primitive_desc* primitive,
	const s_mat4* transform,
	se_sdf_operation operation
);
extern b8 se_sdf_build_single_object_preset(
	se_sdf_handle sdf,
	const se_sdf_primitive_desc* primitive,
	const s_mat4* transform,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_object
);
extern b8 se_sdf_build_grid_preset(
	se_sdf_handle sdf,
	const se_sdf_primitive_desc* primitive,
	i32 columns,
	i32 rows,
	f32 spacing,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_first
);
extern b8 se_sdf_build_primitive_gallery_preset(
	se_sdf_handle sdf,
	i32 columns,
	i32 rows,
	f32 spacing,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_focus
);
extern b8 se_sdf_build_orbit_showcase_preset(
	se_sdf_handle sdf,
	const se_sdf_primitive_desc* center_primitive,
	const se_sdf_primitive_desc* orbit_primitive,
	i32 orbit_count,
	f32 orbit_radius,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_center
);

extern se_sdf_physics_handle se_sdf_physics_create(se_sdf_handle sdf);
extern void se_sdf_physics_destroy(se_sdf_physics_handle physics);
extern b8 se_sdf_physics_refresh(se_sdf_physics_handle physics);
extern se_physics_shape_3d_handle se_sdf_physics_add_shape_3d(
	se_sdf_physics_handle physics,
	se_physics_world_3d_handle world,
	se_physics_body_3d_handle body,
	const s_vec3* offset,
	const s_vec3* rotation,
	b8 is_trigger
);

typedef struct {
	u32 resolution_x;
	u32 resolution_y;
	u32 resolution_z;
	f32 padding;
	f32 max_distance;
	const c8* base_color_texture_uniform;
	const c8* base_color_factor_uniform;
} se_sdf_model_texture3d_desc;

typedef struct {
	se_texture_handle texture;
	s_vec3 bounds_min;
	s_vec3 bounds_max;
	s_vec3 voxel_size;
	f32 max_distance;
} se_sdf_model_texture3d_result;

#define SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS ((se_sdf_model_texture3d_desc){ \
	.resolution_x = 128u, \
	.resolution_y = 128u, \
	.resolution_z = 128u, \
	.padding = 0.05f, \
	.max_distance = 0.0f, \
	.base_color_texture_uniform = "u_texture", \
	.base_color_factor_uniform = "u_base_color_factor" \
})

extern b8 se_sdf_bake_model_texture3d(
	se_model_handle model,
	const se_sdf_model_texture3d_desc* desc,
	se_sdf_model_texture3d_result* out_result
);

typedef struct {
	u32 brick_budget;
	u32 brick_upload_budget;
	u32 max_visible_instances;
	f32 lod_fade_ratio;
	b8 use_compute_if_available;
	b8 front_to_back_sort;
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
	u32 visible_refs;
	u32 resident_bricks;
	u32 page_misses;
	f32 average_march_steps;
	u32 selected_lod_counts[4];
} se_sdf_renderer_stats;

typedef struct {
	s_vec2 resolution;
	f32 time_seconds;
	s_vec2 mouse_normalized;
	se_camera_handle camera;
	b8 has_scene_depth_texture;
	u32 scene_depth_texture;
} se_sdf_frame_desc;

extern b8 se_sdf_frame_set_camera(se_sdf_frame_desc* frame, se_camera_handle camera);
extern b8 se_sdf_frame_set_scene_depth_texture(se_sdf_frame_desc* frame, u32 depth_texture);

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
	.brick_budget = 512u, \
	.brick_upload_budget = 32u, \
	.max_visible_instances = 2048u, \
	.lod_fade_ratio = 0.15f, \
	.use_compute_if_available = 1, \
	.front_to_back_sort = 1 \
})
#define SE_SDF_FRAME_DESC_DEFAULTS ((se_sdf_frame_desc){ \
	.resolution = (s_vec2){ .x = 1.0f, .y = 1.0f }, \
	.time_seconds = 0.0f, \
	.mouse_normalized = (s_vec2){ .x = 0.0f, .y = 0.0f }, \
	.camera = S_HANDLE_NULL, \
	.has_scene_depth_texture = 0, \
	.scene_depth_texture = 0u \
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
extern void se_sdf_shutdown(void);
extern b8 se_sdf_renderer_set_sdf(se_sdf_renderer_handle renderer, se_sdf_handle sdf);
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
extern b8 se_sdf_renderer_render(se_sdf_renderer_handle renderer, const se_sdf_frame_desc* frame);
extern se_sdf_renderer_stats se_sdf_renderer_get_stats(se_sdf_renderer_handle renderer);
extern se_sdf_build_diagnostics se_sdf_renderer_get_build_diagnostics(se_sdf_renderer_handle renderer);

#endif // SE_SDF_H
