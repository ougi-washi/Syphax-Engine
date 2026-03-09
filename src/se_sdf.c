// Syphax-Engine - Ougi Washi

#include "se_sdf.h"
#include "se_camera.h"
#include "se_graphics.h"
#include "se_model.h"
#include "se_shader.h"
#include "se_texture.h"
#include "se_scene.h"
#include "se_worker.h"
#include "se_debug.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"
#include "render/se_gl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif

typedef enum {
	SE_SDF_NONE = 0,
	SE_SDF_SPHERE = 1,
	SE_SDF_BOX = 2,
	SE_SDF_ROUND_BOX = 3,
	SE_SDF_BOX_FRAME = 4,
	SE_SDF_TORUS = 5,
	SE_SDF_CAPPED_TORUS = 6,
	SE_SDF_LINK = 7,
	SE_SDF_CYLINDER = 8,
	SE_SDF_CONE = 9,
	SE_SDF_CONE_INFINITE = 10,
	SE_SDF_PLANE = 11,
	SE_SDF_HEX_PRISM = 12,
	SE_SDF_CAPSULE = 13,
	SE_SDF_VERTICAL_CAPSULE = 14,
	SE_SDF_CAPPED_CYLINDER = 15,
	SE_SDF_CAPPED_CYLINDER_ARBITRARY = 16,
	SE_SDF_ROUNDED_CYLINDER = 17,
	SE_SDF_CAPPED_CONE = 18,
	SE_SDF_CAPPED_CONE_ARBITRARY = 19,
	SE_SDF_SOLID_ANGLE = 20,
	SE_SDF_CUT_SPHERE = 21,
	SE_SDF_CUT_HOLLOW_SPHERE = 22,
	SE_SDF_DEATH_STAR = 23,
	SE_SDF_ROUND_CONE = 24,
	SE_SDF_ROUND_CONE_ARBITRARY = 25,
	SE_SDF_VESICA_SEGMENT = 26,
	SE_SDF_RHOMBUS = 27,
	SE_SDF_OCTAHEDRON = 28,
	SE_SDF_OCTAHEDRON_BOUND = 29,
	SE_SDF_PYRAMID = 30,
	SE_SDF_TRIANGLE = 31,
	SE_SDF_QUAD = 32
} se_sdf_object_type;

typedef struct se_sdf_object {
	s_mat4 transform;
	se_sdf_handle source_scene;
	se_sdf_node_handle source_node;
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
	f32 operation_amount;
	se_sdf_noise noise;
	s_vec3 color;
	b8 has_color;
	s_array(struct se_sdf_object, children);
} se_sdf_object;

static const char* se_sdf_volume_vertex_shader =
	"#version 330 core\n"
	"layout(location = 0) in vec3 a_position;\n"
	"uniform mat4 u_instance_mvp;\n"
	"uniform mat4 u_instance_model;\n"
	"out vec3 v_box_local;\n"
	"out vec3 v_world_position;\n"
	"void main(void) {\n"
	"	v_box_local = a_position;\n"
	"	v_world_position = (u_instance_model * vec4(a_position, 1.0)).xyz;\n"
	"	gl_Position = u_instance_mvp * vec4(a_position, 1.0);\n"
	"}\n";

static const char* se_sdf_volume_fragment_shader =
	"#version 330 core\n"
	"in vec3 v_box_local;\n"
	"in vec3 v_world_position;\n"
	"uniform sampler3D u_volume_atlas_primary;\n"
	"uniform sampler3D u_volume_atlas_secondary;\n"
	"uniform sampler3D u_volume_page_table_primary;\n"
	"uniform sampler3D u_volume_page_table_secondary;\n"
	"uniform sampler3D u_volume_color_atlas_primary;\n"
	"uniform sampler3D u_volume_color_atlas_secondary;\n"
	"uniform vec3 u_volume_size;\n"
	"uniform vec3 u_volume_resolution_primary;\n"
	"uniform vec3 u_volume_resolution_secondary;\n"
	"uniform vec3 u_voxel_size_primary;\n"
	"uniform vec3 u_voxel_size_secondary;\n"
	"uniform int u_brick_size;\n"
	"uniform vec3 u_camera_position;\n"
	"uniform vec3 u_camera_forward;\n"
	"uniform int u_camera_is_orthographic;\n"
	"uniform float u_surface_epsilon;\n"
	"uniform float u_min_step;\n"
	"uniform int u_max_steps;\n"
	"uniform vec3 u_material_base_color;\n"
	"uniform vec3 u_light_direction;\n"
	"uniform vec3 u_light_color;\n"
	"uniform vec3 u_fog_color;\n"
	"uniform float u_fog_density;\n"
	"uniform int u_has_volume_color_primary;\n"
	"uniform int u_has_volume_color_secondary;\n"
	"uniform float u_instance_lod_blend;\n"
	"uniform mat4 u_instance_model;\n"
	"uniform int u_debug_show_proxy_boxes;\n"
	"uniform mat4 u_instance_inverse_model;\n"
	"uniform mat4 u_view_projection;\n"
	"out vec4 frag_color;\n"
	"\n"
	"float se_sdf_trilerp_f(float c000, float c100, float c010, float c110, float c001, float c101, float c011, float c111, vec3 f) {\n"
	"	float c00 = mix(c000, c100, f.x);\n"
	"	float c10 = mix(c010, c110, f.x);\n"
	"	float c01 = mix(c001, c101, f.x);\n"
	"	float c11 = mix(c011, c111, f.x);\n"
	"	float c0 = mix(c00, c10, f.y);\n"
	"	float c1 = mix(c01, c11, f.y);\n"
	"	return mix(c0, c1, f.z);\n"
	"}\n"
	"\n"
	"vec4 se_sdf_trilerp_v4(vec4 c000, vec4 c100, vec4 c010, vec4 c110, vec4 c001, vec4 c101, vec4 c011, vec4 c111, vec3 f) {\n"
	"	vec4 c00 = mix(c000, c100, f.x);\n"
	"	vec4 c10 = mix(c010, c110, f.x);\n"
	"	vec4 c01 = mix(c001, c101, f.x);\n"
	"	vec4 c11 = mix(c011, c111, f.x);\n"
	"	vec4 c0 = mix(c00, c10, f.y);\n"
	"	vec4 c1 = mix(c01, c11, f.y);\n"
	"	return mix(c0, c1, f.z);\n"
	"}\n"
	"\n"
	"float se_sdf_bounds_distance_world(vec3 box_local) {\n"
	"	vec3 outside = max(abs(box_local) - vec3(1.0), vec3(0.0));\n"
	"	return length(outside * max(u_volume_size * 0.5, vec3(0.0001)));\n"
	"}\n"
	"\n"
	"ivec3 se_sdf_clamp_voxel(ivec3 voxel, vec3 volume_resolution) {\n"
	"	ivec3 max_voxel = max(ivec3(0), ivec3(volume_resolution) - ivec3(1));\n"
	"	return clamp(voxel, ivec3(0), max_voxel);\n"
	"}\n"
	"\n"
	"float se_sdf_fetch_distance(sampler3D atlas, sampler3D page_table, ivec3 voxel, vec3 volume_resolution) {\n"
	"	ivec3 clamped_voxel = se_sdf_clamp_voxel(voxel, volume_resolution);\n"
	"	int brick_size = max(u_brick_size, 1);\n"
	"	ivec3 page_coord = clamped_voxel / brick_size;\n"
	"	ivec3 local_coord = clamped_voxel - page_coord * brick_size;\n"
	"	vec4 page = texelFetch(page_table, page_coord, 0);\n"
	"	ivec3 atlas_size = textureSize(atlas, 0);\n"
	"	ivec3 atlas_base = ivec3(page.xyz * vec3(atlas_size) - vec3(0.5));\n"
	"	return texelFetch(atlas, atlas_base + local_coord, 0).r;\n"
	"}\n"
	"\n"
	"vec4 se_sdf_fetch_color(sampler3D atlas, sampler3D page_table, ivec3 voxel, vec3 volume_resolution) {\n"
	"	ivec3 clamped_voxel = se_sdf_clamp_voxel(voxel, volume_resolution);\n"
	"	int brick_size = max(u_brick_size, 1);\n"
	"	ivec3 page_coord = clamped_voxel / brick_size;\n"
	"	ivec3 local_coord = clamped_voxel - page_coord * brick_size;\n"
	"	vec4 page = texelFetch(page_table, page_coord, 0);\n"
	"	ivec3 atlas_size = textureSize(atlas, 0);\n"
	"	ivec3 atlas_base = ivec3(page.xyz * vec3(atlas_size) - vec3(0.5));\n"
	"	return texelFetch(atlas, atlas_base + local_coord, 0);\n"
	"}\n"
	"\n"
	"float se_sdf_sample_distance_lod(sampler3D atlas, sampler3D page_table, vec3 volume_resolution, vec3 box_local) {\n"
	"	float bounds_distance = se_sdf_bounds_distance_world(box_local);\n"
	"	vec3 uv = clamp(box_local * 0.5 + 0.5, vec3(0.0), vec3(1.0));\n"
	"	volume_resolution = max(volume_resolution, vec3(1.0));\n"
	"	vec3 sample_pos = clamp(uv * volume_resolution - vec3(0.5), vec3(0.0), volume_resolution - vec3(1.0));\n"
	"	ivec3 v0 = ivec3(floor(sample_pos));\n"
	"	ivec3 v1 = min(v0 + ivec3(1), ivec3(volume_resolution) - ivec3(1));\n"
	"	vec3 f = sample_pos - vec3(v0);\n"
	"	float sampled = se_sdf_trilerp_f(\n"
	"		se_sdf_fetch_distance(atlas, page_table, ivec3(v0.x, v0.y, v0.z), volume_resolution),\n"
	"		se_sdf_fetch_distance(atlas, page_table, ivec3(v1.x, v0.y, v0.z), volume_resolution),\n"
	"		se_sdf_fetch_distance(atlas, page_table, ivec3(v0.x, v1.y, v0.z), volume_resolution),\n"
	"		se_sdf_fetch_distance(atlas, page_table, ivec3(v1.x, v1.y, v0.z), volume_resolution),\n"
	"		se_sdf_fetch_distance(atlas, page_table, ivec3(v0.x, v0.y, v1.z), volume_resolution),\n"
	"		se_sdf_fetch_distance(atlas, page_table, ivec3(v1.x, v0.y, v1.z), volume_resolution),\n"
	"		se_sdf_fetch_distance(atlas, page_table, ivec3(v0.x, v1.y, v1.z), volume_resolution),\n"
	"		se_sdf_fetch_distance(atlas, page_table, ivec3(v1.x, v1.y, v1.z), volume_resolution),\n"
	"		f\n"
	"	);\n"
	"	return bounds_distance > 0.0 ? max(sampled, bounds_distance) : sampled;\n"
	"}\n"
	"\n"
	"vec3 se_sdf_sample_color_lod(sampler3D atlas, sampler3D page_table, vec3 volume_resolution, vec3 box_local) {\n"
	"	vec3 uv = clamp(box_local * 0.5 + 0.5, vec3(0.0), vec3(1.0));\n"
	"	volume_resolution = max(volume_resolution, vec3(1.0));\n"
	"	vec3 sample_pos = clamp(uv * volume_resolution - vec3(0.5), vec3(0.0), volume_resolution - vec3(1.0));\n"
	"	ivec3 v0 = ivec3(floor(sample_pos));\n"
	"	ivec3 v1 = min(v0 + ivec3(1), ivec3(volume_resolution) - ivec3(1));\n"
	"	vec3 f = sample_pos - vec3(v0);\n"
	"	vec4 sampled = se_sdf_trilerp_v4(\n"
	"		se_sdf_fetch_color(atlas, page_table, ivec3(v0.x, v0.y, v0.z), volume_resolution),\n"
	"		se_sdf_fetch_color(atlas, page_table, ivec3(v1.x, v0.y, v0.z), volume_resolution),\n"
	"		se_sdf_fetch_color(atlas, page_table, ivec3(v0.x, v1.y, v0.z), volume_resolution),\n"
	"		se_sdf_fetch_color(atlas, page_table, ivec3(v1.x, v1.y, v0.z), volume_resolution),\n"
	"		se_sdf_fetch_color(atlas, page_table, ivec3(v0.x, v0.y, v1.z), volume_resolution),\n"
	"		se_sdf_fetch_color(atlas, page_table, ivec3(v1.x, v0.y, v1.z), volume_resolution),\n"
	"		se_sdf_fetch_color(atlas, page_table, ivec3(v0.x, v1.y, v1.z), volume_resolution),\n"
	"		se_sdf_fetch_color(atlas, page_table, ivec3(v1.x, v1.y, v1.z), volume_resolution),\n"
	"		f\n"
	"	);\n"
	"	return sampled.rgb;\n"
	"}\n"
	"\n"
	"bool se_sdf_ray_box(vec3 ro, vec3 rd, out float t_entry, out float t_exit) {\n"
	"	vec3 safe_rd = vec3(\n"
	"		abs(rd.x) < 0.000001 ? (rd.x < 0.0 ? -0.000001 : 0.000001) : rd.x,\n"
	"		abs(rd.y) < 0.000001 ? (rd.y < 0.0 ? -0.000001 : 0.000001) : rd.y,\n"
	"		abs(rd.z) < 0.000001 ? (rd.z < 0.0 ? -0.000001 : 0.000001) : rd.z\n"
	"	);\n"
	"	vec3 t0 = (-vec3(1.0) - ro) / safe_rd;\n"
	"	vec3 t1 = ( vec3(1.0) - ro) / safe_rd;\n"
	"	vec3 tmin3 = min(t0, t1);\n"
	"	vec3 tmax3 = max(t0, t1);\n"
	"	t_entry = max(max(tmin3.x, tmin3.y), tmin3.z);\n"
	"	t_exit = min(min(tmax3.x, tmax3.y), tmax3.z);\n"
	"	return t_exit >= max(t_entry, 0.0);\n"
	"}\n"
	"\n"
	"float se_sdf_sample_distance(vec3 box_local) {\n"
	"	float primary = se_sdf_sample_distance_lod(u_volume_atlas_primary, u_volume_page_table_primary, u_volume_resolution_primary, box_local);\n"
	"	if (u_instance_lod_blend <= 0.001) {\n"
	"		return primary;\n"
	"	}\n"
	"	float secondary = se_sdf_sample_distance_lod(u_volume_atlas_secondary, u_volume_page_table_secondary, u_volume_resolution_secondary, box_local);\n"
	"	return mix(primary, secondary, clamp(u_instance_lod_blend, 0.0, 1.0));\n"
	"}\n"
	"\n"
	"vec3 se_sdf_sample_color(vec3 box_local) {\n"
	"	vec3 primary = u_has_volume_color_primary != 0 ? se_sdf_sample_color_lod(u_volume_color_atlas_primary, u_volume_page_table_primary, u_volume_resolution_primary, box_local) : u_material_base_color;\n"
	"	if (u_instance_lod_blend <= 0.001) {\n"
	"		return primary;\n"
	"	}\n"
	"	vec3 secondary = u_has_volume_color_secondary != 0 ? se_sdf_sample_color_lod(u_volume_color_atlas_secondary, u_volume_page_table_secondary, u_volume_resolution_secondary, box_local) : primary;\n"
	"	return mix(primary, secondary, clamp(u_instance_lod_blend, 0.0, 1.0));\n"
	"}\n"
	"\n"
	"vec3 se_sdf_estimate_normal(vec3 box_local) {\n"
	"	vec3 eps = vec3(\n"
	"		max(0.0005, max(u_voxel_size_primary.x / max(u_volume_size.x, 0.0001), u_surface_epsilon)),\n"
	"		max(0.0005, max(u_voxel_size_primary.y / max(u_volume_size.y, 0.0001), u_surface_epsilon)),\n"
	"		max(0.0005, max(u_voxel_size_primary.z / max(u_volume_size.z, 0.0001), u_surface_epsilon))\n"
	"	);\n"
	"	float dx = se_sdf_sample_distance(box_local + vec3(eps.x, 0.0, 0.0)) - se_sdf_sample_distance(box_local - vec3(eps.x, 0.0, 0.0));\n"
	"	float dy = se_sdf_sample_distance(box_local + vec3(0.0, eps.y, 0.0)) - se_sdf_sample_distance(box_local - vec3(0.0, eps.y, 0.0));\n"
	"	float dz = se_sdf_sample_distance(box_local + vec3(0.0, 0.0, eps.z)) - se_sdf_sample_distance(box_local - vec3(0.0, 0.0, eps.z));\n"
	"	vec3 local_normal = normalize(vec3(dx, dy, dz));\n"
	"	vec3 world_normal = normalize((transpose(mat3(u_instance_inverse_model)) * local_normal));\n"
	"	return world_normal;\n"
	"}\n"
	"\n"
	"void main(void) {\n"
	"	if (u_debug_show_proxy_boxes != 0) {\n"
	"		frag_color = vec4(1.0, 0.1, 0.1, 1.0);\n"
	"		return;\n"
	"	}\n"
	"	vec3 ray_dir = u_camera_is_orthographic != 0 ? normalize(u_camera_forward) : normalize(v_world_position - u_camera_position);\n"
	"	vec3 local_dir = (u_instance_inverse_model * vec4(ray_dir, 0.0)).xyz;\n"
	"	float dominant_voxel = max(max(u_voxel_size_primary.x, u_voxel_size_primary.y), u_voxel_size_primary.z);\n"
	"	float entry_bias = max(0.0025, dominant_voxel * 0.45);\n"
	"	vec3 local_origin = v_box_local - local_dir * entry_bias;\n"
	"	float box_entry = 0.0;\n"
	"	float box_exit = 0.0;\n"
	"	if (!se_sdf_ray_box(local_origin, local_dir, box_entry, box_exit)) {\n"
	"		discard;\n"
	"	}\n"
	"	float march_start = max(box_entry, 0.0);\n"
	"	float march_limit = max(box_exit, 0.0);\n"
	"	if (march_limit <= 0.0) {\n"
	"		discard;\n"
	"	}\n"
	"	float hit_epsilon = max(u_surface_epsilon, dominant_voxel * 0.35);\n"
	"	float min_step = max(u_min_step, dominant_voxel * 0.08);\n"
	"	float t = march_start;\n"
	"	bool hit = false;\n"
	"	vec3 hit_local = local_origin;\n"
	"	for (int i = 0; i < 512; ++i) {\n"
	"		if (i >= u_max_steps || t > march_limit) {\n"
	"			break;\n"
	"		}\n"
	"		hit_local = local_origin + local_dir * t;\n"
	"		float distance = se_sdf_sample_distance(hit_local);\n"
	"		if (abs(distance) <= hit_epsilon) {\n"
	"			hit = true;\n"
	"			break;\n"
	"		}\n"
	"		t += max(distance, min_step);\n"
	"	}\n"
	"	if (!hit) {\n"
	"		discard;\n"
	"	}\n"
	"	vec3 hit_world = (u_instance_model * vec4(hit_local, 1.0)).xyz;\n"
	"	vec3 normal = se_sdf_estimate_normal(hit_local);\n"
	"	vec3 base_color = se_sdf_sample_color(hit_local);\n"
	"	vec3 light_dir = normalize(-u_light_direction);\n"
	"	float diffuse = max(dot(normal, light_dir), 0.0);\n"
	"	float hemi = normal.y * 0.5 + 0.5;\n"
	"	vec3 light_tint = mix(vec3(1.0), max(u_light_color, vec3(0.85)), 0.65);\n"
	"	vec3 lit = base_color * light_tint * (0.46 + diffuse * 0.72 + hemi * 0.18);\n"
	"	float view_distance = length(hit_world - u_camera_position);\n"
	"	float fog = u_fog_density > 0.0 ? clamp((1.0 - exp(-u_fog_density * view_distance)) * 0.55, 0.0, 1.0) : 0.0;\n"
	"	vec3 color = mix(lit, u_fog_color, fog);\n"
	"	vec4 clip = u_view_projection * vec4(hit_world, 1.0);\n"
	"	float inv_w = abs(clip.w) > 0.000001 ? (1.0 / clip.w) : 0.0;\n"
	"	gl_FragDepth = clip.z * inv_w * 0.5 + 0.5;\n"
	"	frag_color = vec4(color, 1.0);\n"
	"}\n";

static const char* se_sdf_volume_fragment_shader_minimal =
	"#version 330 core\n"
	"out vec4 frag_color;\n"
	"void main(void) {\n"
	"	frag_color = vec4(1.0, 0.1, 0.1, 1.0);\n"
	"}\n";

static const char* se_sdf_volume_fragment_shader_probe =
	"#version 330 core\n"
	"uniform sampler3D u_volume_atlas_primary;\n"
	"out vec4 frag_color;\n"
	"void main(void) {\n"
	"	frag_color = vec4(texture(u_volume_atlas_primary, vec3(0.5)).rrr, 1.0);\n"
	"}\n";

static const char* se_sdf_volume_fragment_shader_analytic =
	"#version 330 core\n"
	"uniform vec2 u_resolution;\n"
	"uniform vec3 u_camera_position;\n"
	"uniform mat4 u_camera_inv_view_projection;\n"
	"uniform mat4 u_instance_inverse_model;\n"
	"out vec4 frag_color;\n"
	"\n"
	"bool se_sdf_ray_box(vec3 ro, vec3 rd, out float t_entry, out float t_exit) {\n"
	"	vec3 safe_rd = vec3(\n"
	"		abs(rd.x) < 0.000001 ? (rd.x < 0.0 ? -0.000001 : 0.000001) : rd.x,\n"
	"		abs(rd.y) < 0.000001 ? (rd.y < 0.0 ? -0.000001 : 0.000001) : rd.y,\n"
	"		abs(rd.z) < 0.000001 ? (rd.z < 0.0 ? -0.000001 : 0.000001) : rd.z\n"
	"	);\n"
	"	vec3 t0 = (-vec3(1.0) - ro) / safe_rd;\n"
	"	vec3 t1 = ( vec3(1.0) - ro) / safe_rd;\n"
	"	vec3 tmin3 = min(t0, t1);\n"
	"	vec3 tmax3 = max(t0, t1);\n"
	"	t_entry = max(max(tmin3.x, tmin3.y), tmin3.z);\n"
	"	t_exit = min(min(tmax3.x, tmax3.y), tmax3.z);\n"
	"	return t_exit >= max(t_entry, 0.0);\n"
	"}\n"
	"\n"
	"vec3 se_sdf_world_from_ndc(vec3 ndc) {\n"
	"	vec4 world = u_camera_inv_view_projection * vec4(ndc, 1.0);\n"
	"	float inv_w = abs(world.w) > 0.000001 ? (1.0 / world.w) : 0.0;\n"
	"	return world.xyz * inv_w;\n"
	"}\n"
	"\n"
	"float se_sdf_sphere(vec3 p, float r) {\n"
	"	return length(p) - r;\n"
	"}\n"
	"\n"
	"void main(void) {\n"
	"	vec2 resolution = max(u_resolution, vec2(1.0));\n"
	"	vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;\n"
	"	vec3 world_near = se_sdf_world_from_ndc(vec3(uv, -1.0));\n"
	"	vec3 world_far = se_sdf_world_from_ndc(vec3(uv, 1.0));\n"
	"	vec3 ray_origin = u_camera_position;\n"
	"	vec3 ray_dir = normalize(world_far - ray_origin);\n"
	"	vec3 local_origin = (u_instance_inverse_model * vec4(ray_origin, 1.0)).xyz;\n"
	"	vec3 local_dir = (u_instance_inverse_model * vec4(ray_dir, 0.0)).xyz;\n"
	"	float box_entry = 0.0;\n"
	"	float box_exit = 0.0;\n"
	"	if (!se_sdf_ray_box(local_origin, local_dir, box_entry, box_exit)) {\n"
	"		discard;\n"
	"	}\n"
	"	float t = max(box_entry, 0.0);\n"
	"	for (int i = 0; i < 128; ++i) {\n"
	"		if (t > box_exit) {\n"
	"			discard;\n"
	"		}\n"
	"		vec3 p = local_origin + local_dir * t;\n"
	"		float d = se_sdf_sphere(p, 0.55);\n"
	"		if (abs(d) < 0.01) {\n"
	"			frag_color = vec4(0.1, 1.0, 0.2, 1.0);\n"
	"			return;\n"
	"		}\n"
	"		t += max(d * 0.6, 0.01);\n"
	"	}\n"
	"	discard;\n"
	"}\n";

static const f32 se_sdf_volume_cube_vertices[] = {
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f
};

static const u32 se_sdf_volume_cube_indices[] = {
	0u, 1u, 2u, 2u, 3u, 0u,
	1u, 5u, 6u, 6u, 2u, 1u,
	5u, 4u, 7u, 7u, 6u, 5u,
	4u, 0u, 3u, 3u, 7u, 4u,
	3u, 2u, 6u, 6u, 7u, 3u,
	4u, 5u, 1u, 1u, 0u, 4u
};

#define SE_SDF_JSON_FORMAT "se_sdf_json"

enum {
	SE_SDF_JSON_VERSION = 1u
};

enum {
	SE_SDF_QUERY_INCLUDE_LOCAL = 1u << 0,
	SE_SDF_QUERY_INCLUDE_REFS = 1u << 1,
	SE_SDF_QUERY_INCLUDE_ALL = SE_SDF_QUERY_INCLUDE_LOCAL | SE_SDF_QUERY_INCLUDE_REFS
};

typedef struct {
	u32 resolution;
	se_texture_handle texture;
	se_texture_handle color_texture;
	se_texture_handle page_table_texture;
	s_vec3 voxel_size;
	f32 max_distance;
	u32 brick_size;
	u32 brick_border;
	u32 brick_count_x;
	u32 brick_count_y;
	u32 brick_count_z;
	u32 atlas_width;
	u32 atlas_height;
	u32 atlas_depth;
	f32* cpu_distances;
	u8* cpu_colors;
	sz cpu_distance_count;
	sz cpu_color_count;
	b8 valid;
} se_sdf_prepared_lod;
typedef s_array(se_sdf_prepared_lod, se_sdf_prepared_lods);

typedef struct {
	se_sdf_handle source;
	se_sdf_node_handle node;
	s_mat4 transform;
	s_mat4 inverse_transform;
	se_sdf_bounds bounds;
	f32 sort_distance;
} se_sdf_prepared_ref;
typedef s_array(se_sdf_prepared_ref, se_sdf_prepared_refs);

typedef s_array(u32, se_sdf_u32_array);
typedef s_array(s_mat4, se_sdf_mat4_array);
typedef s_array(f32, se_sdf_f32_array);

typedef struct {
	u32 primary_lod;
	u32 secondary_lod;
	f32 blend;
} se_sdf_lod_selection;

typedef struct {
	se_sdf_u32_array ref_indices;
} se_sdf_grid_cell;
typedef s_array(se_sdf_grid_cell, se_sdf_grid_cells);

typedef struct {
	u64 generation;
	u64 dependency_stamp;
	se_sdf_prepare_desc desc;
	se_sdf_bounds bounds;
	se_sdf_bounds local_bounds;
	s_vec3 full_color;
	s_vec3 local_color;
	u32 full_color_weight;
	u32 local_color_weight;
	b8 full_color_valid;
	b8 local_color_valid;
	se_sdf_prepared_lod full_lods[4];
	se_sdf_prepared_lod local_lods[4];
	b8 full_lods_valid;
	b8 local_lods_valid;
	b8 instancing_supported;
	b8 has_local_content;
	f32 cell_size;
	s_vec3 grid_min;
	i32 grid_nx;
	i32 grid_ny;
	i32 grid_nz;
	se_sdf_prepared_refs refs;
	se_sdf_grid_cells grid_cells;
	b8 ready;
} se_sdf_prepared_cache;

typedef struct {
	se_context* context;
	se_sdf_handle source_scene;
	se_sdf_handle worker_scene;
	se_sdf_prepare_desc desc;
	b8 force_full_lods;
	u64 requested_generation;
	se_result result;
	se_sdf_prepared_cache prepared;
} se_sdf_async_prepare_result;

typedef struct {
	se_worker_task task;
	se_context* worker_context;
	se_sdf_handle worker_scene;
	b8 active;
} se_sdf_async_prepare_state;

typedef struct {
	se_sdf_handle owner_scene;
	s_mat4 transform;
	se_sdf_operation operation;
	f32 operation_amount;
	se_sdf_noise noise;
	s_vec3 color;
	b8 has_color;
	se_sdf_primitive_desc primitive;
	se_sdf_node_type type;
	se_sdf_handle ref_source;
	se_sdf_node_handle parent;
	s_array(se_sdf_node_handle, children);
} se_sdf_node;
typedef s_array(se_sdf_node, se_sdf_nodes);

typedef struct se_sdf {
	se_sdf_nodes nodes;
	se_sdf_node_handle root;
	se_sdf_renderer_handle implicit_renderer;
	se_camera_handle implicit_camera;
	u64 generation;
	se_sdf_prepared_cache prepared;
	se_sdf_async_prepare_state async_prepare;
	s_array(se_sdf_handle, owned_import_sdfs);
} se_sdf_scene;

typedef struct se_sdf_physics {
	se_sdf_handle scene;
	se_sdf_bounds bounds;
	u64 built_generation;
	b8 has_cache;
} se_sdf_physics;
typedef s_array(se_sdf_physics, se_sdf_physicses);

typedef struct {
	se_sdf_handle scene;
} se_sdf_scene_object_2d_data;

typedef struct {
	se_sdf_handle scene;
} se_sdf_scene_object_3d_data;

typedef struct se_sdf_query_distance_result se_sdf_query_distance_result;
typedef s_array(se_sdf_handle, se_sdf_handles);

typedef struct {
	se_sdf_handle source;
	u8 lod_mask;
	b8 use_local_lods;
	u64 last_used_frame;
} se_sdf_budget_entry;
typedef s_array(se_sdf_budget_entry, se_sdf_budget_entries);

typedef struct {
	s_vec3 sum;
	u32 weight;
} se_sdf_color_accumulator;

typedef struct se_sdf_renderer {
	se_sdf_renderer_desc desc;
	se_sdf_raymarch_quality quality;
	se_sdf_renderer_debug debug;
	se_sdf_handle scene;
	se_shader_handle shader;
	se_sdf_material_desc material;
	se_sdf_stylized_desc stylized;
	s_vec3 lighting_direction;
	s_vec3 lighting_color;
	s_vec3 fog_color;
	f32 fog_density;
	se_sdf_build_diagnostics diagnostics;
	se_sdf_renderer_stats stats;
	u64 render_generation;
	u64 residency_frame;
	se_sdf_u32_array visible_ref_indices;
	se_sdf_mat4_array instance_models;
	se_sdf_mat4_array instance_inverses;
	se_sdf_f32_array instance_lod_blends;
	se_sdf_budget_entries resident_lod_entries;
	se_sdf_budget_entries frame_lod_entries;
	u32 cube_vao;
	u32 cube_vbo;
	u32 cube_ebo;
	u32 cube_model_vbo;
	u32 cube_inverse_vbo;
	u32 cube_lod_blend_vbo;
	u32 fallback_scene_depth_texture;
	sz instance_capacity;
} se_sdf_renderer;
typedef s_array(se_sdf_renderer, se_sdf_renderers);

static se_worker_pool* g_se_sdf_prepare_pool = NULL;
static b8 se_sdf_is_valid_primitive_type(const se_sdf_primitive_type type);
static b8 se_sdf_validate_primitive_desc(const se_sdf_primitive_desc* primitive);
static b8 se_sdf_renderer_has_live_shader(const se_sdf_renderer* renderer);
static void se_sdf_renderer_release_shader(se_sdf_renderer* renderer);
static void se_sdf_renderer_invalidate_gpu_state(se_sdf_renderer* renderer);
static void se_sdf_renderer_refresh_context_state(se_sdf_renderer* renderer);
b8 se_sdf_calculate_bounds(const se_sdf_handle sdf, se_sdf_bounds* out_bounds);
b8 se_sdf_align_camera(
	const se_sdf_handle sdf,
	const se_camera_handle camera,
	const se_sdf_camera_align_desc* desc,
	se_sdf_bounds* out_bounds
);
b8 se_sdf_validate(
	const se_sdf_handle sdf,
	char* error_message,
	const sz error_message_size
);
static void se_sdf_write_scene_error(
	char* error_message,
	sz error_message_size,
	const char* fmt,
	...
);
static void se_sdf_physics_finalize_bounds(se_sdf_bounds* bounds);
static void se_sdf_scene_touch(se_sdf_scene* scene_ptr);
static void se_sdf_touch_sdf_and_dependents(se_sdf_handle scene);
static void se_sdf_mark_dependent_scenes_dirty(se_sdf_handle source);
static b8 se_sdf_get_context(se_context** out_context);
static se_sdf_scene* se_sdf_scene_from_handle(se_sdf_handle scene);
static se_sdf_node* se_sdf_node_from_handle(
	se_sdf_scene* scene_ptr,
	se_sdf_handle scene,
	se_sdf_node_handle node
);
static b8 se_sdf_handles_contains(const se_sdf_handles* handles, se_sdf_handle handle);
static se_worker_pool* se_sdf_prepare_pool_get(void);
static u32 se_sdf_prepare_worker_thread_count(void);
static se_context* se_sdf_prepare_context_create(void);
static void se_sdf_prepare_context_destroy(se_context* context, se_sdf_handle worker_scene);
static b8 se_sdf_scene_has_ref_nodes(const se_sdf_scene* scene_ptr);
static void se_sdf_prepare_async_cleanup_result(se_sdf_async_prepare_result* result);
static void se_sdf_scene_wait_async_prepare(se_sdf_scene* scene_ptr);
static b8 se_sdf_clone_scene_graph(se_sdf_scene* source_ptr, se_sdf_handle source, se_sdf_handle clone);
static b8 se_sdf_node_is_group(const se_sdf_node* node);
static b8 se_sdf_node_is_primitive(const se_sdf_node* node);
static b8 se_sdf_node_is_ref(const se_sdf_node* node);
static void se_sdf_scene_release_implicit_resources(se_sdf_scene* scene_ptr);
static void se_sdf_scene_release_nodes(se_sdf_scene* scene_ptr);
static void se_sdf_scene_release_owned_imports(se_sdf_scene* scene_ptr, se_sdf_handle owner);
static se_sdf_prepare_desc se_sdf_prepare_desc_resolve(const se_sdf_prepare_desc* desc);
static b8 se_sdf_prepare_build_cache(
	se_sdf_handle sdf,
	se_sdf_scene* scene_ptr,
	const se_sdf_prepare_desc* prepare_desc,
	b8 force_full_lods,
	se_sdf_prepared_cache* out_cache,
	b8 build_gpu_resources
);
static void se_sdf_prepared_cache_clear(se_sdf_prepared_cache* cache);
static b8 se_sdf_prepared_cache_finalize_gpu(se_sdf_prepared_cache* cache);
static void* se_sdf_prepare_async_task(void* user_data);
static b8 se_sdf_prepare_poll_internal(se_sdf_handle sdf, se_sdf_prepare_status* out_status, b8 set_error);
static b8 se_sdf_node_calculate_world_transform(
	se_sdf_scene* scene_ptr,
	se_sdf_handle scene,
	se_sdf_node_handle node,
	s_mat4* out_transform
);
static b8 se_sdf_scene_update_prepared_ref_subtree(
	se_sdf_scene* scene_ptr,
	se_sdf_handle scene,
	se_sdf_node_handle node
);
static s_vec3 se_sdf_mul_mat4_point(const s_mat4* mat, const s_vec3* point);
static b8 se_sdf_query_sample_distance_internal(
	se_sdf_handle scene,
	const s_vec3* point,
	f32* out_distance,
	se_sdf_handle* out_hit_sdf,
	se_sdf_node_handle* out_node,
	s_vec3* out_color,
	b8* out_has_color
);
static se_sdf_query_distance_result se_sdf_query_distance_runtime_recursive(
	se_sdf_scene* scene_ptr,
	se_sdf_handle scene,
	se_sdf_node_handle node_handle,
	const s_vec3* world_point,
	const s_mat4* parent_transform,
	sz depth,
	sz max_depth,
	u32 include_mask
);
static se_sdf_query_distance_result se_sdf_query_distance_prepared_volume(
	const se_sdf_prepared_cache* cache,
	const se_sdf_prepared_lod* lods,
	const se_sdf_bounds* bounds,
	se_sdf_handle sdf,
	se_sdf_node_handle node,
	const s_vec3* point
);
static b8 se_sdf_query_sample_distance_filtered_internal(
	se_sdf_handle scene,
	const s_vec3* point,
	f32* out_distance,
	se_sdf_handle* out_hit_sdf,
	se_sdf_node_handle* out_node,
	s_vec3* out_color,
	b8* out_has_color,
	u32 include_mask
);
static b8 se_sdf_estimate_surface_normal_runtime(
	se_sdf_handle scene,
	const s_vec3* point,
	s_vec3* out_normal
);
static void se_sdf_scene_bounds_expand_point(se_sdf_bounds* bounds, const s_vec3* point);
static void se_sdf_scene_bounds_expand_transformed_aabb(
	se_sdf_bounds* bounds,
	const s_mat4* transform,
	const s_vec3* local_min,
	const s_vec3* local_max
);
static void se_sdf_expand_local_bounds_by_noise(
	const se_sdf_noise* noise,
	s_vec3* local_min,
	s_vec3* local_max
);
static b8 se_sdf_get_primitive_local_bounds(
	const se_sdf_primitive_desc* primitive,
	s_vec3* out_min,
	s_vec3* out_max,
	b8* out_unbounded
);
static void se_sdf_collect_scene_bounds_recursive(
	se_sdf_scene* scene_ptr,
	se_sdf_handle scene,
	se_sdf_node_handle node_handle,
	const s_mat4* parent_transform,
	se_sdf_bounds* bounds,
	sz depth,
	sz max_depth,
	u32 include_mask
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
static f32 se_sdf_noise_apply_distance(
	const se_sdf_noise* noise,
	const s_vec3* local_point,
	f32 distance
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

static b8 se_sdf_json_add_noise(s_json* parent, const c8* name, const se_sdf_noise* noise) {
	if (!parent || !noise) {
		return 0;
	}
	s_json* noise_json = se_sdf_json_add_object(parent, name);
	return noise_json &&
		se_sdf_json_add_bool(noise_json, "active", noise->active) &&
		se_sdf_json_add_f32(noise_json, "scale", noise->scale) &&
		se_sdf_json_add_f32(noise_json, "offset", noise->offset) &&
		se_sdf_json_add_f32(noise_json, "frequency", noise->frequency);
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

static b8 se_sdf_json_read_noise(const s_json* object, const c8* key, se_sdf_noise* out_noise) {
	if (!out_noise) {
		return 0;
	}
	*out_noise = SE_SDF_NOISE_DEFAULTS;
	if (!object || object->type != S_JSON_OBJECT || !key) {
		return 0;
	}
	const s_json* noise_json = s_json_get(object, key);
	if (!noise_json) {
		return 1;
	}
	if (noise_json->type != S_JSON_OBJECT) {
		return 0;
	}
	if (!se_sdf_json_get_bool(noise_json, "active", &out_noise->active) ||
		!se_sdf_json_get_f32(noise_json, "scale", &out_noise->scale) ||
		!se_sdf_json_get_f32(noise_json, "offset", &out_noise->offset) ||
		!se_sdf_json_get_f32(noise_json, "frequency", &out_noise->frequency)) {
		return 0;
	}
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

static b8 se_sdf_json_scene_index_of(
	const se_sdf_handles* scene_handles,
	const se_sdf_handle scene,
	u32* out_index
) {
	if (!scene_handles || !out_index || scene == SE_SDF_NULL) {
		return 0;
	}
	for (u32 i = 0u; i < (u32)s_array_get_size((se_sdf_handles*)scene_handles); ++i) {
		const se_sdf_handle* handle = s_array_get((se_sdf_handles*)scene_handles, s_array_handle((se_sdf_handles*)scene_handles, i));
		if (handle && *handle == scene) {
			*out_index = i;
			return 1;
		}
	}
	return 0;
}

static b8 se_sdf_json_collect_scene_graph_recursive(
	const se_sdf_handle scene,
	se_sdf_handles* out_scene_handles
) {
	if (scene == SE_SDF_NULL || !out_scene_handles) {
		return 0;
	}
	if (se_sdf_handles_contains(out_scene_handles, scene)) {
		return 1;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	s_array_add(out_scene_handles, scene);
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		const se_sdf_node_handle handle = s_array_handle(&scene_ptr->nodes, (u32)i);
		se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, handle);
		if (!se_sdf_node_is_ref(node)) {
			continue;
		}
		if (node->ref_source == SE_SDF_NULL || !se_sdf_json_collect_scene_graph_recursive(node->ref_source, out_scene_handles)) {
			return 0;
		}
	}
	return 1;
}

static b8 se_sdf_json_add_scene_definition(
	s_json* parent,
	const se_sdf_handles* scene_handles,
	const se_sdf_handle scene
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!parent || !scene_handles || !scene_ptr) {
		return 0;
	}

	s_json* scene_json = se_sdf_json_add_object(parent, NULL);
	if (!scene_json) {
		return 0;
	}

	se_sdf_node_handles node_handles = {0};
	s_array_init(&node_handles);
	s_array_reserve(&node_handles, s_array_get_size(&scene_ptr->nodes));
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		const se_sdf_node_handle handle = s_array_handle(&scene_ptr->nodes, (u32)i);
		se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, handle);
		if (node) {
			s_array_add(&node_handles, handle);
		}
	}

	i32 root_index = -1;
	if (scene_ptr->root != SE_SDF_NODE_NULL) {
		u32 root_u32 = 0u;
		if (!se_sdf_json_node_index_of(&node_handles, scene_ptr->root, &root_u32)) {
			s_array_clear(&node_handles);
			return 0;
		}
		root_index = (i32)root_u32;
	}
	s_json* nodes_json = se_sdf_json_add_array(scene_json, "nodes");
	if (!nodes_json || !se_sdf_json_add_i32(scene_json, "root_index", root_index)) {
		s_array_clear(&node_handles);
		return 0;
	}

	for (sz i = 0; i < s_array_get_size(&node_handles); ++i) {
		se_sdf_node_handle* handle = s_array_get(&node_handles, s_array_handle(&node_handles, (u32)i));
		se_sdf_node* node = handle ? se_sdf_node_from_handle(scene_ptr, scene, *handle) : NULL;
		if (!handle || !node) {
			continue;
		}

		s_json* node_json = se_sdf_json_add_object(nodes_json, NULL);
		s_json* children_json = node_json ? se_sdf_json_add_array(node_json, "children") : NULL;
		if (!node_json ||
			!children_json ||
			!se_sdf_json_add_bool(node_json, "is_group", se_sdf_node_is_group(node)) ||
			!se_sdf_json_add_i32(node_json, "node_type", (i32)node->type) ||
			!se_sdf_json_add_mat4(node_json, "transform", &node->transform) ||
			!se_sdf_json_add_i32(node_json, "operation", (i32)node->operation) ||
			!se_sdf_json_add_f32(node_json, "operation_amount", node->operation_amount) ||
			!se_sdf_json_add_noise(node_json, "noise", &node->noise) ||
			!se_sdf_json_add_vec3(node_json, "color", &node->color) ||
			!se_sdf_json_add_bool(node_json, "has_color", node->has_color)) {
			s_array_clear(&node_handles);
			return 0;
		}

		if (se_sdf_node_is_primitive(node) &&
			!se_sdf_json_add_primitive_desc(node_json, "primitive", &node->primitive)) {
			s_array_clear(&node_handles);
			return 0;
		}
		if (se_sdf_node_is_ref(node)) {
			u32 ref_scene_index = 0u;
			if (node->ref_source == SE_SDF_NULL ||
				!se_sdf_json_scene_index_of(scene_handles, node->ref_source, &ref_scene_index) ||
				!se_sdf_json_add_u32(node_json, "ref_sdf_index", ref_scene_index)) {
				s_array_clear(&node_handles);
				return 0;
			}
		}

		for (sz child_i = 0; child_i < s_array_get_size(&node->children); ++child_i) {
			se_sdf_node_handle* child_handle = s_array_get(&node->children, s_array_handle(&node->children, (u32)child_i));
			u32 child_index = 0u;
			if (!child_handle ||
				*child_handle == SE_SDF_NODE_NULL ||
				!se_sdf_json_node_index_of(&node_handles, *child_handle, &child_index) ||
				!se_sdf_json_add_u32(children_json, NULL, child_index)) {
				s_array_clear(&node_handles);
				return 0;
			}
		}
	}

	s_array_clear(&node_handles);
	return 1;
}

static void se_sdf_json_destroy_scene_list(se_sdf_handles* scene_handles) {
	if (!scene_handles) {
		return;
	}
	for (sz i = s_array_get_size(scene_handles); i > 0; --i) {
		se_sdf_handle* handle = s_array_get(scene_handles, s_array_handle(scene_handles, (u32)(i - 1u)));
		if (handle && *handle != SE_SDF_NULL && se_sdf_scene_from_handle(*handle)) {
			se_sdf_destroy(*handle);
		}
	}
	s_array_clear(scene_handles);
}

static void se_sdf_scene_discard_transferred_shell(const se_sdf_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_context* ctx = NULL;
	if (!scene_ptr || !se_sdf_get_context(&ctx) || !ctx) {
		return;
	}
	se_sdf_scene_wait_async_prepare(scene_ptr);
	se_sdf_scene_release_implicit_resources(scene_ptr);
	se_sdf_scene_release_nodes(scene_ptr);
	se_sdf_scene_release_owned_imports(scene_ptr, scene);
	s_array_clear(&scene_ptr->owned_import_sdfs);
	s_array_remove(&ctx->sdfs, scene);
}

static b8 se_sdf_scene_take_loaded_graph(
	const se_sdf_handle scene,
	const se_sdf_handle loaded_scene,
	const se_sdf_handles* owned_import_sdfs
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_scene* loaded_ptr = se_sdf_scene_from_handle(loaded_scene);
	if (!scene_ptr || !loaded_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}

	se_sdf_scene_wait_async_prepare(scene_ptr);
	se_sdf_scene_release_nodes(scene_ptr);
	se_sdf_scene_release_owned_imports(scene_ptr, scene);
	scene_ptr = se_sdf_scene_from_handle(scene);
	loaded_ptr = se_sdf_scene_from_handle(loaded_scene);
	if (!scene_ptr || !loaded_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}
	s_array_clear(&scene_ptr->owned_import_sdfs);

	scene_ptr->nodes = loaded_ptr->nodes;
	scene_ptr->root = loaded_ptr->root;
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		se_sdf_node* node = s_array_get(&scene_ptr->nodes, s_array_handle(&scene_ptr->nodes, (u32)i));
		if (node) {
			node->owner_scene = scene;
		}
	}
	if (owned_import_sdfs) {
		s_array_reserve(&scene_ptr->owned_import_sdfs, s_array_get_size((se_sdf_handles*)owned_import_sdfs));
		for (sz i = 0; i < s_array_get_size((se_sdf_handles*)owned_import_sdfs); ++i) {
			se_sdf_handle* handle = s_array_get((se_sdf_handles*)owned_import_sdfs, s_array_handle((se_sdf_handles*)owned_import_sdfs, (u32)i));
			if (!handle || *handle == SE_SDF_NULL || *handle == scene || *handle == loaded_scene) {
				continue;
			}
			s_array_add(&scene_ptr->owned_import_sdfs, *handle);
		}
	}

	s_array_init(&loaded_ptr->nodes);
	s_array_init(&loaded_ptr->owned_import_sdfs);
	loaded_ptr->root = SE_SDF_NODE_NULL;
	se_sdf_scene_discard_transferred_shell(loaded_scene);
	se_sdf_touch_sdf_and_dependents(scene);
	se_set_last_error(SE_RESULT_OK);
	return 1;
}

static se_worker_pool* se_sdf_prepare_pool_get(void) {
	if (g_se_sdf_prepare_pool) {
		return g_se_sdf_prepare_pool;
	}
	se_worker_config config = SE_WORKER_CONFIG_DEFAULTS;
	config.thread_count = se_sdf_prepare_worker_thread_count();
	config.queue_capacity = s_max(config.queue_capacity, config.thread_count * 64u);
	config.max_tasks = s_max(config.max_tasks, config.thread_count * 256u);
	g_se_sdf_prepare_pool = se_worker_create(&config);
	return g_se_sdf_prepare_pool;
}

static u32 se_sdf_prepare_worker_thread_count(void) {
	u32 thread_count = SE_WORKER_CONFIG_DEFAULTS.thread_count;
#if !defined(_WIN32)
	const long online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (online_cpus > 0l) {
		thread_count = (u32)online_cpus;
	}
#endif
	if (thread_count == 0u) {
		thread_count = SE_WORKER_CONFIG_DEFAULTS.thread_count;
	}
	if (thread_count > 32u) {
		thread_count = 32u;
	}
	return thread_count;
}

static se_context* se_sdf_prepare_context_create(void) {
	return calloc(1u, sizeof(se_context));
}

static void se_sdf_prepare_context_destroy(se_context* context, const se_sdf_handle worker_scene) {
	if (!context) {
		return;
	}
	se_context* previous = se_push_tls_context(context);
	if (worker_scene != SE_SDF_NULL) {
		se_sdf_destroy(worker_scene);
	}
	s_array_clear(&context->sdfs);
	s_array_clear(&context->sdf_physicses);
	s_array_clear(&context->sdf_renderers);
	se_pop_tls_context(previous);
	free(context);
}

static b8 se_sdf_scene_has_ref_nodes(const se_sdf_scene* scene_ptr) {
	if (!scene_ptr) {
		return 0;
	}
	for (sz i = 0; i < s_array_get_size((se_sdf_nodes*)&scene_ptr->nodes); ++i) {
		se_sdf_node* node = s_array_get((se_sdf_nodes*)&scene_ptr->nodes, s_array_handle((se_sdf_nodes*)&scene_ptr->nodes, (u32)i));
		if (node && se_sdf_node_is_ref(node)) {
			return 1;
		}
	}
	return 0;
}

static se_sdf_prepare_desc se_sdf_prepare_desc_resolve(const se_sdf_prepare_desc* desc) {
	se_sdf_prepare_desc prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	if (desc) {
		prepare_desc = *desc;
	}
	if (prepare_desc.lod_count == 0u || prepare_desc.lod_count > 4u) {
		prepare_desc.lod_count = SE_SDF_PREPARE_DESC_DEFAULTS.lod_count;
	}
	if (prepare_desc.brick_size == 0u) {
		prepare_desc.brick_size = SE_SDF_PREPARE_DESC_DEFAULTS.brick_size;
	}
	if (prepare_desc.brick_border > 8u) {
		prepare_desc.brick_border = 8u;
	}
	return prepare_desc;
}

static void se_sdf_prepare_async_cleanup_result(se_sdf_async_prepare_result* result) {
	if (!result) {
		return;
	}
	if (result->context) {
		se_sdf_prepare_context_destroy(result->context, result->worker_scene);
		result->context = NULL;
		result->worker_scene = SE_SDF_NULL;
	}
	se_sdf_prepared_cache_clear(&result->prepared);
	free(result);
}

static b8 se_sdf_get_context(se_context** out_context) {
	se_context* ctx = se_current_context();
	if (!ctx) {
		return 0;
	}
	if (s_array_get_capacity(&ctx->sdfs) == 0) {
		s_array_init(&ctx->sdfs);
	}
	if (s_array_get_capacity(&ctx->sdf_physicses) == 0) {
		s_array_init(&ctx->sdf_physicses);
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

static void se_sdf_renderer_release_volume_gpu(se_sdf_renderer* renderer) {
	if (!renderer) {
		return;
	}
	if (renderer->fallback_scene_depth_texture != 0u) {
		glDeleteTextures(1, &renderer->fallback_scene_depth_texture);
		renderer->fallback_scene_depth_texture = 0u;
	}
	if (renderer->cube_lod_blend_vbo != 0u) {
		glDeleteBuffers(1, &renderer->cube_lod_blend_vbo);
		renderer->cube_lod_blend_vbo = 0u;
	}
	if (renderer->cube_model_vbo != 0u) {
		glDeleteBuffers(1, &renderer->cube_model_vbo);
		renderer->cube_model_vbo = 0u;
	}
	if (renderer->cube_inverse_vbo != 0u) {
		glDeleteBuffers(1, &renderer->cube_inverse_vbo);
		renderer->cube_inverse_vbo = 0u;
	}
	if (renderer->cube_ebo != 0u) {
		glDeleteBuffers(1, &renderer->cube_ebo);
		renderer->cube_ebo = 0u;
	}
	if (renderer->cube_vbo != 0u) {
		glDeleteBuffers(1, &renderer->cube_vbo);
		renderer->cube_vbo = 0u;
	}
	if (renderer->cube_vao != 0u) {
		glDeleteVertexArrays(1, &renderer->cube_vao);
		renderer->cube_vao = 0u;
	}
	renderer->instance_capacity = 0u;
}

static b8 se_sdf_renderer_ensure_volume_gpu(se_sdf_renderer* renderer) {
	if (!renderer) {
		return 0;
	}
	se_sdf_renderer_refresh_context_state(renderer);
	if (renderer->shader == S_HANDLE_NULL) {
		const char* fragment_shader_source = se_sdf_volume_fragment_shader;
		if (getenv("SE_SDF_DEBUG_FORCE_MINIMAL_SHADER") != NULL) {
			fragment_shader_source = se_sdf_volume_fragment_shader_minimal;
		} else if (getenv("SE_SDF_DEBUG_FORCE_PROBE_SHADER") != NULL) {
			fragment_shader_source = se_sdf_volume_fragment_shader_probe;
		} else if (getenv("SE_SDF_DEBUG_FORCE_ANALYTIC_SHADER") != NULL) {
			fragment_shader_source = se_sdf_volume_fragment_shader_analytic;
		}
		renderer->shader = se_shader_load_from_memory(se_sdf_volume_vertex_shader, fragment_shader_source);
		if (renderer->shader == S_HANDLE_NULL) {
			se_sdf_set_diagnostics(renderer, 0, "shader_compile", "failed to compile volume shader");
			return 0;
		}
	}
	if (renderer->cube_vao != 0u) {
		return 1;
	}
	if (renderer->fallback_scene_depth_texture == 0u) {
		const u8 white_pixel = 255u;
		glGenTextures(1, &renderer->fallback_scene_depth_texture);
		glBindTexture(GL_TEXTURE_2D, renderer->fallback_scene_depth_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &white_pixel);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glGenVertexArrays(1, &renderer->cube_vao);
	glGenBuffers(1, &renderer->cube_vbo);
	glGenBuffers(1, &renderer->cube_ebo);

	glBindVertexArray(renderer->cube_vao);
	glBindBuffer(GL_ARRAY_BUFFER, renderer->cube_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(se_sdf_volume_cube_vertices), se_sdf_volume_cube_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->cube_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(se_sdf_volume_cube_indices), se_sdf_volume_cube_indices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3u, (const void*)0);
	glBindVertexArray(0);

	return 1;
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

static void se_sdf_renderer_invalidate_gpu_state(se_sdf_renderer* renderer) {
	if (!renderer) {
		return;
	}
	se_sdf_renderer_release_shader(renderer);
	se_sdf_renderer_release_volume_gpu(renderer);
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
	}
}

static se_sdf_scene* se_sdf_scene_from_handle(const se_sdf_handle scene) {
	if (scene == SE_SDF_NULL) {
		return NULL;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return NULL;
	}
	return s_array_get(&ctx->sdfs, scene);
}

static se_sdf_physics* se_sdf_physics_from_handle(const se_sdf_physics_handle physics) {
	if (physics == SE_SDF_PHYSICS_NULL) {
		return NULL;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return NULL;
	}
	return s_array_get(&ctx->sdf_physicses, physics);
}

static void se_sdf_scene_wait_async_prepare(se_sdf_scene* scene_ptr) {
	if (!scene_ptr || !scene_ptr->async_prepare.active) {
		return;
	}
	se_worker_pool* pool = se_sdf_prepare_pool_get();
	se_sdf_async_prepare_result* result = NULL;
	if (pool && scene_ptr->async_prepare.task != SE_WORKER_TASK_NULL) {
		(void)se_worker_wait(pool, scene_ptr->async_prepare.task, (void**)&result);
		(void)se_worker_release(pool, scene_ptr->async_prepare.task);
	}
	scene_ptr->async_prepare.task = SE_WORKER_TASK_NULL;
	scene_ptr->async_prepare.active = 0;
	se_context* worker_context = scene_ptr->async_prepare.worker_context;
	scene_ptr->async_prepare.worker_context = NULL;
	if (!result && scene_ptr->async_prepare.worker_scene != SE_SDF_NULL) {
		result = calloc(1u, sizeof(*result));
		if (result) {
			result->context = worker_context;
			result->worker_scene = scene_ptr->async_prepare.worker_scene;
		}
	}
	scene_ptr->async_prepare.worker_scene = SE_SDF_NULL;
	if (!result && worker_context) {
		se_sdf_prepare_context_destroy(worker_context, SE_SDF_NULL);
	}
	se_sdf_prepare_async_cleanup_result(result);
}

static void se_sdf_scene_touch(se_sdf_scene* scene_ptr) {
	if (!scene_ptr) {
		return;
	}
	scene_ptr->prepared.ready = 0;
	scene_ptr->generation++;
	if (scene_ptr->generation == 0u) {
		scene_ptr->generation = 1u;
	}
}

static void se_sdf_scene_commit_runtime_update(se_sdf_scene* scene_ptr) {
	if (!scene_ptr) {
		return;
	}
	scene_ptr->generation++;
	if (scene_ptr->generation == 0u) {
		scene_ptr->generation = 1u;
	}
	if (scene_ptr->prepared.ready) {
		scene_ptr->prepared.generation = scene_ptr->generation;
	}
}

static b8 se_sdf_scene_references_source(const se_sdf_scene* scene_ptr, const se_sdf_handle source) {
	if (!scene_ptr || source == SE_SDF_NULL) {
		return 0;
	}
	for (sz i = 0; i < s_array_get_size((se_sdf_nodes*)&scene_ptr->nodes); ++i) {
		se_sdf_node* node = s_array_get((se_sdf_nodes*)&scene_ptr->nodes, s_array_handle((se_sdf_nodes*)&scene_ptr->nodes, (u32)i));
		if (node && node->type == SE_SDF_NODE_REF && node->ref_source == source) {
			return 1;
		}
	}
	return 0;
}

static b8 se_sdf_handles_contains(const se_sdf_handles* handles, const se_sdf_handle handle) {
	if (!handles || handle == SE_SDF_NULL) {
		return 0;
	}
	for (sz i = 0; i < s_array_get_size((se_sdf_handles*)handles); ++i) {
		se_sdf_handle* value = s_array_get((se_sdf_handles*)handles, s_array_handle((se_sdf_handles*)handles, (u32)i));
		if (value && *value == handle) {
			return 1;
		}
	}
	return 0;
}

static void se_sdf_mark_dependent_scenes_dirty(const se_sdf_handle source) {
	se_context* ctx = NULL;
	if (source == SE_SDF_NULL || !se_sdf_get_context(&ctx) || !ctx) {
		return;
	}
	se_sdf_handles dirty_queue;
	s_array_init(&dirty_queue);
	s_array_add(&dirty_queue, source);
	for (sz index = 0; index < s_array_get_size(&dirty_queue); ++index) {
		se_sdf_handle* target_ptr = s_array_get(&dirty_queue, s_array_handle(&dirty_queue, (u32)index));
		if (!target_ptr) {
			continue;
		}
		const se_sdf_handle target = *target_ptr;
		for (sz i = 0; i < s_array_get_size(&ctx->sdfs); ++i) {
			const se_sdf_handle candidate_handle = s_array_handle(&ctx->sdfs, (u32)i);
			if (candidate_handle == target || se_sdf_handles_contains(&dirty_queue, candidate_handle)) {
				continue;
			}
			se_sdf_scene* candidate_ptr = s_array_get(&ctx->sdfs, candidate_handle);
			if (!candidate_ptr || !se_sdf_scene_references_source(candidate_ptr, target)) {
				continue;
			}
			se_sdf_scene_touch(candidate_ptr);
			s_array_add(&dirty_queue, candidate_handle);
		}
	}
	s_array_clear(&dirty_queue);
}

static void se_sdf_touch_sdf_and_dependents(const se_sdf_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	se_sdf_scene_touch(scene_ptr);
	se_sdf_mark_dependent_scenes_dirty(scene);
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
	const se_sdf_handle scene,
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
		if (!se_sdf_renderer_set_sdf(scene_ptr->implicit_renderer, scene)) {
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
		(void)se_sdf_align_camera(scene, scene_ptr->implicit_camera, NULL, NULL);
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

static void se_sdf_scene_object_render(const se_sdf_handle scene) {
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
	const se_sdf_handle scene,
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

static b8 se_sdf_node_is_group(const se_sdf_node* node) {
	return node && node->type == SE_SDF_NODE_GROUP;
}

static b8 se_sdf_node_is_primitive(const se_sdf_node* node) {
	return node && node->type == SE_SDF_NODE_PRIMITIVE;
}

static b8 se_sdf_node_is_ref(const se_sdf_node* node) {
	return node && node->type == SE_SDF_NODE_REF;
}

static b8 se_sdf_clone_scene_graph(
	se_sdf_scene* source_ptr,
	const se_sdf_handle source,
	const se_sdf_handle clone
) {
	se_sdf_scene* clone_ptr = se_sdf_scene_from_handle(clone);
	if (!source_ptr || !clone_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	se_sdf_node_handles source_handles = {0};
	se_sdf_node_handles cloned_handles = {0};
	s_array_init(&source_handles);
	s_array_init(&cloned_handles);
	s_array_reserve(&source_handles, s_array_get_size(&source_ptr->nodes));
	s_array_reserve(&cloned_handles, s_array_get_size(&source_ptr->nodes));

	for (u32 i = 0u; i < (u32)s_array_get_size(&source_ptr->nodes); ++i) {
		const se_sdf_node_handle source_handle = s_array_handle(&source_ptr->nodes, i);
		se_sdf_node* source_node = s_array_get(&source_ptr->nodes, source_handle);
		if (!source_node) {
			continue;
		}

		se_sdf_node_handle cloned_handle = SE_SDF_NODE_NULL;
		if (source_node->type == SE_SDF_NODE_GROUP) {
			se_sdf_node_group_desc group_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
			group_desc.transform = source_node->transform;
			group_desc.operation = source_node->operation;
			group_desc.operation_amount = source_node->operation_amount;
			cloned_handle = se_sdf_node_create_group(clone, &group_desc);
		} else if (source_node->type == SE_SDF_NODE_PRIMITIVE) {
			se_sdf_node_primitive_desc primitive_desc = {0};
			primitive_desc.transform = source_node->transform;
			primitive_desc.operation = source_node->operation;
			primitive_desc.operation_amount = source_node->operation_amount;
			primitive_desc.noise = source_node->noise;
			primitive_desc.primitive = source_node->primitive;
			cloned_handle = se_sdf_node_create_primitive(clone, &primitive_desc);
		} else if (source_node->type == SE_SDF_NODE_REF) {
			se_sdf_node_ref_desc ref_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
			ref_desc.source = source_node->ref_source;
			ref_desc.transform = source_node->transform;
			ref_desc.operation = source_node->operation;
			ref_desc.operation_amount = source_node->operation_amount;
			cloned_handle = se_sdf_node_create_ref(clone, &ref_desc);
		}

		if (cloned_handle == SE_SDF_NODE_NULL) {
			s_array_clear(&source_handles);
			s_array_clear(&cloned_handles);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return 0;
		}

		se_sdf_node* cloned_node = se_sdf_node_from_handle(clone_ptr, clone, cloned_handle);
		if (!cloned_node) {
			s_array_clear(&source_handles);
			s_array_clear(&cloned_handles);
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return 0;
		}
		cloned_node->noise = source_node->noise;
		cloned_node->color = source_node->color;
		cloned_node->has_color = source_node->has_color;

		s_array_add(&source_handles, source_handle);
		s_array_add(&cloned_handles, cloned_handle);
	}

	for (u32 i = 0u; i < (u32)s_array_get_size(&source_handles); ++i) {
		se_sdf_node_handle* source_handle = s_array_get(&source_handles, s_array_handle(&source_handles, i));
		se_sdf_node_handle* cloned_parent = s_array_get(&cloned_handles, s_array_handle(&cloned_handles, i));
		se_sdf_node* source_node = source_handle ? se_sdf_node_from_handle(source_ptr, source, *source_handle) : NULL;
		if (!source_handle || !cloned_parent || !source_node) {
			continue;
		}
		for (u32 child_i = 0u; child_i < (u32)s_array_get_size(&source_node->children); ++child_i) {
			se_sdf_node_handle* source_child = s_array_get(&source_node->children, s_array_handle(&source_node->children, child_i));
			u32 child_index = 0u;
			if (!source_child || !se_sdf_json_node_index_of(&source_handles, *source_child, &child_index)) {
				s_array_clear(&source_handles);
				s_array_clear(&cloned_handles);
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				return 0;
			}
			se_sdf_node_handle* cloned_child = s_array_get(&cloned_handles, s_array_handle(&cloned_handles, child_index));
			if (!cloned_child || !se_sdf_node_add_child(clone, *cloned_parent, *cloned_child)) {
				s_array_clear(&source_handles);
				s_array_clear(&cloned_handles);
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				return 0;
			}
		}
	}

	if (source_ptr->root != SE_SDF_NODE_NULL) {
		u32 root_index = 0u;
		if (!se_sdf_json_node_index_of(&source_handles, source_ptr->root, &root_index)) {
			s_array_clear(&source_handles);
			s_array_clear(&cloned_handles);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return 0;
		}
		se_sdf_node_handle* cloned_root = s_array_get(&cloned_handles, s_array_handle(&cloned_handles, root_index));
		if (!cloned_root || !se_sdf_set_root(clone, *cloned_root)) {
			s_array_clear(&source_handles);
			s_array_clear(&cloned_handles);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return 0;
		}
	}

	s_array_clear(&source_handles);
	s_array_clear(&cloned_handles);
	se_set_last_error(SE_RESULT_OK);
	return 1;
}

static void se_sdf_prepared_lod_clear(se_sdf_prepared_lod* lod) {
	if (!lod) {
		return;
	}
	if (lod->texture != S_HANDLE_NULL) {
		se_texture_destroy(lod->texture);
	}
	if (lod->color_texture != S_HANDLE_NULL) {
		se_texture_destroy(lod->color_texture);
	}
	if (lod->page_table_texture != S_HANDLE_NULL) {
		se_texture_destroy(lod->page_table_texture);
	}
	if (lod->cpu_distances) {
		free(lod->cpu_distances);
	}
	if (lod->cpu_colors) {
		free(lod->cpu_colors);
	}
	memset(lod, 0, sizeof(*lod));
}

static void se_sdf_prepared_grid_clear(se_sdf_grid_cells* cells) {
	if (!cells) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(cells); ++i) {
		se_sdf_grid_cell* cell = s_array_get(cells, s_array_handle(cells, (u32)i));
		if (cell) {
			s_array_clear(&cell->ref_indices);
		}
	}
	s_array_clear(cells);
}

static void se_sdf_prepared_cache_clear(se_sdf_prepared_cache* cache) {
	if (!cache) {
		return;
	}
	for (u32 i = 0; i < 4u; ++i) {
		se_sdf_prepared_lod_clear(&cache->full_lods[i]);
		se_sdf_prepared_lod_clear(&cache->local_lods[i]);
	}
	se_sdf_prepared_grid_clear(&cache->grid_cells);
	s_array_clear(&cache->refs);
	memset(cache, 0, sizeof(*cache));
	cache->desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	cache->bounds = SE_SDF_BOUNDS_DEFAULTS;
	cache->local_bounds = SE_SDF_BOUNDS_DEFAULTS;
	cache->cell_size = 16.0f;
	s_array_init(&cache->refs);
	s_array_init(&cache->grid_cells);
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
	se_sdf_prepared_cache_clear(&scene_ptr->prepared);
}

static void se_sdf_scene_release_owned_imports(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle owner
) {
	if (!scene_ptr) {
		return;
	}
	se_sdf_handles owned_copy = {0};
	s_array_init(&owned_copy);
	s_array_reserve(&owned_copy, s_array_get_size(&scene_ptr->owned_import_sdfs));
	for (sz i = 0; i < s_array_get_size(&scene_ptr->owned_import_sdfs); ++i) {
		se_sdf_handle* handle = s_array_get(&scene_ptr->owned_import_sdfs, s_array_handle(&scene_ptr->owned_import_sdfs, (u32)i));
		if (!handle || *handle == SE_SDF_NULL || *handle == owner) {
			continue;
		}
		s_array_add(&owned_copy, *handle);
	}
	s_array_clear(&scene_ptr->owned_import_sdfs);
	for (sz i = 0; i < s_array_get_size(&owned_copy); ++i) {
		se_sdf_handle* handle = s_array_get(&owned_copy, s_array_handle(&owned_copy, (u32)i));
		if (handle && *handle != SE_SDF_NULL && se_sdf_scene_from_handle(*handle)) {
			se_sdf_destroy(*handle);
		}
	}
	s_array_clear(&owned_copy);
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

static b8 se_sdf_handle_stack_contains(const se_sdf_handles* handles, const se_sdf_handle handle) {
	if (!handles || handle == SE_SDF_NULL) {
		return 0;
	}
	for (sz i = 0; i < s_array_get_size((se_sdf_handles*)handles); ++i) {
		se_sdf_handle* current = s_array_get((se_sdf_handles*)handles, s_array_handle((se_sdf_handles*)handles, (u32)i));
		if (current && *current == handle) {
			return 1;
		}
	}
	return 0;
}

static b8 se_sdf_validate_ref_graph_recursive(
	const se_sdf_handle scene,
	se_sdf_handles* stack,
	char* error_message,
	const sz error_message_size
) {
	if (scene == SE_SDF_NULL) {
		se_sdf_write_scene_error(error_message, error_message_size, "invalid referenced sdf handle");
		return 0;
	}
	if (se_sdf_handle_stack_contains(stack, scene)) {
		se_sdf_write_scene_error(error_message, error_message_size, "cycle detected in sdf references");
		return 0;
	}

	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		se_sdf_write_scene_error(error_message, error_message_size, "referenced sdf handle is invalid");
		return 0;
	}

	s_array_add(stack, scene);
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		const se_sdf_node_handle handle = s_array_handle(&scene_ptr->nodes, (u32)i);
		se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, handle);
		if (!se_sdf_node_is_ref(node)) {
			continue;
		}
		if (node->ref_source == SE_SDF_NULL) {
			se_sdf_write_scene_error(
				error_message,
				error_message_size,
				"ref node has null source (node_slot=%u)",
				s_handle_slot(handle)
			);
			s_array_remove(stack, s_array_handle(stack, (u32)(s_array_get_size(stack) - 1u)));
			return 0;
		}
		if (!se_sdf_validate_ref_graph_recursive(node->ref_source, stack, error_message, error_message_size)) {
			s_array_remove(stack, s_array_handle(stack, (u32)(s_array_get_size(stack) - 1u)));
			return 0;
		}
	}
	s_array_remove(stack, s_array_handle(stack, (u32)(s_array_get_size(stack) - 1u)));
	return 1;
}

static b8 se_sdf_is_ancestor(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle scene,
	const se_sdf_node_handle ancestor,
	se_sdf_node_handle node
) {
	if (!scene_ptr || ancestor == SE_SDF_NODE_NULL || node == SE_SDF_NODE_NULL) {
		return 0;
	}
	const sz max_depth = s_array_get_size(&scene_ptr->nodes) + 256u;
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
	const se_sdf_handle scene,
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
	const se_sdf_handle scene,
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

se_sdf_handle se_sdf_create(const se_sdf_desc* desc) {
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return SE_SDF_NULL;
	}
	se_sdf_desc scene_desc = SE_SDF_DESC_DEFAULTS;
	if (desc) {
		scene_desc = *desc;
	}

	se_sdf_handle sdf_handle = s_array_increment(&ctx->sdfs);
	se_sdf_scene* sdf = s_array_get(&ctx->sdfs, sdf_handle);
	if (!sdf) {
		return SE_SDF_NULL;
	}
	memset(sdf, 0, sizeof(*sdf));
	s_array_init(&sdf->nodes);
	s_array_init(&sdf->owned_import_sdfs);
	sdf->root = SE_SDF_NODE_NULL;
	sdf->implicit_renderer = SE_SDF_RENDERER_NULL;
	sdf->implicit_camera = S_HANDLE_NULL;
	sdf->generation = 1u;
	se_sdf_prepared_cache_clear(&sdf->prepared);

	if (scene_desc.initial_node_capacity > 0) {
		s_array_reserve(&sdf->nodes, scene_desc.initial_node_capacity);
	}

	return sdf_handle;
}

se_object_2d_handle se_sdf_to_object_2d(se_sdf_handle sdf) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	const b8 had_renderer = se_sdf_renderer_from_handle(scene_ptr->implicit_renderer) != NULL;
	const b8 had_camera = (scene_ptr->implicit_camera != S_HANDLE_NULL) &&
		(se_camera_get(scene_ptr->implicit_camera) != NULL);
	if (!se_sdf_scene_prepare_implicit_render(sdf, scene_ptr)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		}
		return S_HANDLE_NULL;
	}

	se_object_custom custom = {0};
	custom.render = se_sdf_scene_object_2d_render;
	const se_sdf_scene_object_2d_data custom_data = {
		.scene = sdf
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

se_object_3d_handle se_sdf_to_object_3d(se_sdf_handle sdf) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	const b8 had_renderer = se_sdf_renderer_from_handle(scene_ptr->implicit_renderer) != NULL;
	const b8 had_camera = (scene_ptr->implicit_camera != S_HANDLE_NULL) &&
		(se_camera_get(scene_ptr->implicit_camera) != NULL);
	if (!se_sdf_scene_prepare_implicit_render(sdf, scene_ptr)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		}
		return S_HANDLE_NULL;
	}

	se_object_custom custom = {0};
	custom.render = se_sdf_scene_object_3d_render;
	const se_sdf_scene_object_3d_data custom_data = {
		.scene = sdf
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

void se_sdf_destroy(const se_sdf_handle sdf) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		return;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return;
	}
	for (sz i = s_array_get_size(&ctx->sdf_physicses); i > 0; --i) {
		const se_sdf_physics_handle physics_handle = s_array_handle(&ctx->sdf_physicses, (u32)(i - 1u));
		se_sdf_physics* physics_ptr = s_array_get(&ctx->sdf_physicses, physics_handle);
		if (physics_ptr && physics_ptr->scene == sdf) {
			se_sdf_physics_destroy(physics_handle);
		}
	}
	se_sdf_scene_wait_async_prepare(scene_ptr);
	se_sdf_mark_dependent_scenes_dirty(sdf);
	se_sdf_scene_release_implicit_resources(scene_ptr);
	se_sdf_scene_release_nodes(scene_ptr);
	se_sdf_scene_release_owned_imports(scene_ptr, sdf);
	s_array_clear(&scene_ptr->owned_import_sdfs);
	s_array_remove(&ctx->sdfs, sdf);
}

void se_sdf_clear(const se_sdf_handle sdf) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		return;
	}
	se_sdf_scene_wait_async_prepare(scene_ptr);
	se_sdf_scene_release_nodes(scene_ptr);
	se_sdf_scene_release_owned_imports(scene_ptr, sdf);
	se_sdf_touch_sdf_and_dependents(sdf);
}

b8 se_sdf_set_root(const se_sdf_handle sdf, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		return 0;
	}
	if (node == SE_SDF_NODE_NULL) {
		scene_ptr->root = SE_SDF_NODE_NULL;
		se_sdf_touch_sdf_and_dependents(sdf);
		return 1;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, sdf, node);
	if (!node_ptr) {
		return 0;
	}
	scene_ptr->root = node;
	se_sdf_touch_sdf_and_dependents(sdf);
	return 1;
}

se_sdf_node_handle se_sdf_get_root(const se_sdf_handle sdf) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		return SE_SDF_NODE_NULL;
	}
	return scene_ptr->root;
}

u64 se_sdf_get_generation(const se_sdf_handle sdf) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		return 0u;
	}
	return scene_ptr->generation;
}

static b8 se_sdf_physics_build_cache(se_sdf_physics* physics) {
	if (!physics) {
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(physics->scene);
	if (!scene_ptr) {
		return 0;
	}
	if (scene_ptr->root == SE_SDF_NODE_NULL) {
		return 0;
	}

	se_sdf_bounds bounds = SE_SDF_BOUNDS_DEFAULTS;
	if (!se_sdf_calculate_bounds(physics->scene, &bounds) || !bounds.valid) {
		return 0;
	}
	if (bounds.has_unbounded_primitives) {
		s_vec3 half = s_vec3(
			bounds.max.x - bounds.min.x,
			bounds.max.y - bounds.min.y,
			bounds.max.z - bounds.min.z
		);
		half = s_vec3_muls(&half, 0.5f);
		half.x = fmaxf(half.x, 64.0f);
		half.y = fmaxf(half.y, 64.0f);
		half.z = fmaxf(half.z, 64.0f);
		bounds.min = s_vec3_sub(&bounds.center, &half);
		bounds.max = s_vec3_add(&bounds.center, &half);
		se_sdf_physics_finalize_bounds(&bounds);
	}

	physics->bounds = bounds;
	physics->built_generation = scene_ptr->generation;
	physics->has_cache = 1;
	return 1;
}

static b8 se_sdf_physics_ensure_current(se_sdf_physics* physics) {
	if (!physics) {
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(physics->scene);
	if (!scene_ptr) {
		return 0;
	}
	if (physics->has_cache && physics->built_generation == scene_ptr->generation) {
		return 1;
	}
	return se_sdf_physics_build_cache(physics);
}

static b8 se_sdf_physics_sample_callback_3d(void* user_data, const s_vec3* local_point, f32* out_distance, s_vec3* out_normal) {
	const se_sdf_physics_handle handle = (se_sdf_physics_handle)(uintptr_t)user_data;
	se_sdf_physics* physics = se_sdf_physics_from_handle(handle);
	if (!physics || !local_point || !out_distance || !se_sdf_physics_ensure_current(physics)) {
		return 0;
	}
	if (!se_sdf_query_sample_distance_internal(physics->scene, local_point, out_distance, NULL, NULL, NULL, NULL)) {
		return 0;
	}
	if (out_normal) {
		if (!se_sdf_estimate_surface_normal_runtime(physics->scene, local_point, out_normal)) {
			*out_normal = s_vec3(0.0f, 1.0f, 0.0f);
		}
	}
	return 1;
}

se_sdf_physics_handle se_sdf_physics_create(const se_sdf_handle scene) {
	if (scene == SE_SDF_NULL) {
		return SE_SDF_PHYSICS_NULL;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return SE_SDF_PHYSICS_NULL;
	}
	if (!se_sdf_scene_from_handle(scene)) {
		return SE_SDF_PHYSICS_NULL;
	}
	const se_sdf_physics_handle handle = s_array_increment(&ctx->sdf_physicses);
	se_sdf_physics* physics = s_array_get(&ctx->sdf_physicses, handle);
	memset(physics, 0, sizeof(*physics));
	physics->scene = scene;
	if (!se_sdf_physics_build_cache(physics)) {
		s_array_remove(&ctx->sdf_physicses, handle);
		return SE_SDF_PHYSICS_NULL;
	}
	return handle;
}

void se_sdf_physics_destroy(const se_sdf_physics_handle physics) {
	if (physics == SE_SDF_PHYSICS_NULL) {
		return;
	}
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return;
	}
	se_sdf_physics* physics_ptr = s_array_get(&ctx->sdf_physicses, physics);
	if (!physics_ptr) {
		return;
	}
	s_array_remove(&ctx->sdf_physicses, physics);
}

b8 se_sdf_physics_refresh(const se_sdf_physics_handle physics) {
	se_sdf_physics* physics_ptr = se_sdf_physics_from_handle(physics);
	return physics_ptr ? se_sdf_physics_build_cache(physics_ptr) : 0;
}

se_physics_shape_3d_handle se_sdf_physics_add_shape_3d(
	const se_sdf_physics_handle physics,
	const se_physics_world_3d_handle world,
	const se_physics_body_3d_handle body,
	const s_vec3* offset,
	const s_vec3* rotation,
	const b8 is_trigger
) {
	se_sdf_physics* physics_ptr = se_sdf_physics_from_handle(physics);
	if (!physics_ptr || !offset || !rotation || !se_sdf_physics_ensure_current(physics_ptr)) {
		return SE_PHYSICS_SHAPE_3D_HANDLE_NULL;
	}
	se_physics_sdf_3d sdf = {
		.user_data = (void*)(uintptr_t)physics,
		.local_bounds = {
			.min = physics_ptr->bounds.min,
			.max = physics_ptr->bounds.max
		},
		.sample = se_sdf_physics_sample_callback_3d
	};
	return se_physics_body_3d_add_sdf(world, body, &sdf, offset, rotation, is_trigger);
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

b8 se_sdf_validate(
	const se_sdf_handle sdf,
	char* error_message,
	const sz error_message_size
) {
	if (error_message && error_message_size > 0) {
		error_message[0] = '\0';
	}

	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		se_sdf_write_scene_error(error_message, error_message_size, "invalid sdf handle");
		return 0;
	}

	if (scene_ptr->root != SE_SDF_NODE_NULL &&
		!se_sdf_node_from_handle(scene_ptr, sdf, scene_ptr->root)) {
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

		if (node->owner_scene != sdf) {
			se_sdf_write_scene_error(
				error_message, error_message_size,
				"node owner mismatch (node_slot=%u)", s_handle_slot(handle));
			return 0;
		}

		if (se_sdf_node_is_primitive(node) && !se_sdf_validate_primitive_desc(&node->primitive)) {
			se_sdf_write_scene_error(
				error_message, error_message_size,
				"invalid primitive parameters (node_slot=%u, type=%d)",
				s_handle_slot(handle), (int)node->primitive.type);
			return 0;
		}
		if (se_sdf_node_is_ref(node)) {
			if (node->ref_source == SE_SDF_NULL || !se_sdf_scene_from_handle(node->ref_source)) {
				se_sdf_write_scene_error(
					error_message, error_message_size,
					"invalid ref source (node_slot=%u)",
					s_handle_slot(handle));
				return 0;
			}
			if (s_array_get_size(&node->children) != 0u) {
				se_sdf_write_scene_error(
					error_message, error_message_size,
					"ref node may not have explicit children (node_slot=%u)",
					s_handle_slot(handle));
				return 0;
			}
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
			se_sdf_node* parent = se_sdf_node_from_handle(scene_ptr, sdf, node->parent);
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
			se_sdf_node* child = se_sdf_node_from_handle(scene_ptr, sdf, *child_handle);
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
			se_sdf_node* current = se_sdf_node_from_handle(scene_ptr, sdf, walker);
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

	se_sdf_handles ref_stack = {0};
	s_array_init(&ref_stack);
	const b8 refs_valid = se_sdf_validate_ref_graph_recursive(sdf, &ref_stack, error_message, error_message_size);
	s_array_clear(&ref_stack);
	if (!refs_valid) {
		return 0;
	}

	return 1;
}

s_json* se_sdf_to_json(const se_sdf_handle scene) {
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
		!se_sdf_json_add_u32(root, "version", 2u)) {
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	se_sdf_handles scene_handles = {0};
	s_array_init(&scene_handles);
	char validation_error[256] = {0};
	if (!se_sdf_validate(scene, validation_error, sizeof(validation_error)) ||
		!se_sdf_json_collect_scene_graph_recursive(scene, &scene_handles)) {
		s_array_clear(&scene_handles);
		s_json_free(root);
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		}
		return NULL;
	}

	s_json* sdfs_json = se_sdf_json_add_array(root, "sdfs");
	if (!sdfs_json || !se_sdf_json_add_u32(root, "entry_sdf_index", 0u)) {
		s_array_clear(&scene_handles);
		s_json_free(root);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(&scene_handles); ++i) {
		se_sdf_handle* graph_scene = s_array_get(&scene_handles, s_array_handle(&scene_handles, (u32)i));
		if (!graph_scene || !se_sdf_json_add_scene_definition(sdfs_json, &scene_handles, *graph_scene)) {
			s_array_clear(&scene_handles);
			s_json_free(root);
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return NULL;
		}
	}

	s_array_clear(&scene_handles);
	se_set_last_error(SE_RESULT_OK);
	return root;
}

static b8 se_sdf_from_json_version_1(const se_sdf_handle scene, const s_json* root) {
	if (!root || root->type != S_JSON_OBJECT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (!se_sdf_scene_from_handle(scene)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	i32 root_index = -1;
	const s_json* nodes_json = se_sdf_json_get_required(root, "nodes", S_JSON_ARRAY);
	if (!se_sdf_json_get_i32(root, "root_index", &root_index) || !nodes_json) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (root_index < -1 || (root_index >= 0 && (sz)root_index >= nodes_json->as.children.count)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	se_sdf_desc temp_desc = SE_SDF_DESC_DEFAULTS;
	temp_desc.initial_node_capacity = nodes_json->as.children.count;
	const se_sdf_handle temp_scene = se_sdf_create(&temp_desc);
	if (temp_scene == SE_SDF_NULL) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return 0;
	}
	se_sdf_scene* temp_scene_ptr = se_sdf_scene_from_handle(temp_scene);
	if (!temp_scene_ptr) {
		se_sdf_destroy(temp_scene);
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
		i32 node_type_i32 = -1;
		i32 operation_i32 = 0;
		f32 operation_amount = SE_SDF_OPERATION_AMOUNT_DEFAULT;
		s_mat4 transform = s_mat4_identity;
		se_sdf_noise noise = SE_SDF_NOISE_DEFAULTS;
		s_vec3 color = s_vec3(0.0f, 0.0f, 0.0f);
		b8 has_color = 0;
		if (!se_sdf_json_get_bool(node_json, "is_group", &is_group) ||
			!se_sdf_json_get_i32(node_json, "operation", &operation_i32) ||
			!se_sdf_json_get_f32(node_json, "operation_amount", &operation_amount) ||
			!se_sdf_json_read_mat4_field(node_json, "transform", &transform)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (!se_sdf_json_get_i32(node_json, "node_type", &node_type_i32)) {
			node_type_i32 = is_group ? (i32)SE_SDF_NODE_GROUP : (i32)SE_SDF_NODE_PRIMITIVE;
		}
		if (node_type_i32 < (i32)SE_SDF_NODE_GROUP || node_type_i32 > (i32)SE_SDF_NODE_REF) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if ((se_sdf_node_type)node_type_i32 == SE_SDF_NODE_REF) {
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			goto fail;
		}
		if (operation_i32 < (i32)SE_SDF_OP_NONE || operation_i32 > (i32)SE_SDF_OP_SMOOTH_SUBTRACTION) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		const s_json* color_json = s_json_get(node_json, "color");
		const s_json* has_color_json = s_json_get(node_json, "has_color");
		if (color_json && !se_sdf_json_read_vec3_node(color_json, &color)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (has_color_json) {
			if (has_color_json->type != S_JSON_BOOL) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
			has_color = has_color_json->as.boolean;
		}
		if (!se_sdf_json_read_noise(node_json, "noise", &noise)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		se_sdf_node_handle node_handle = SE_SDF_NODE_NULL;
		if ((se_sdf_node_type)node_type_i32 == SE_SDF_NODE_GROUP) {
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
			primitive_desc.noise = noise;
			node_handle = se_sdf_node_create_primitive(temp_scene, &primitive_desc);
		}

		if (node_handle == SE_SDF_NODE_NULL) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (has_color && !se_sdf_node_set_color(temp_scene, node_handle, &color)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		s_array_add(&created_handles, node_handle);

		se_sdf_node* node_ptr = se_sdf_node_from_handle(temp_scene_ptr, temp_scene, node_handle);
		if (!node_ptr) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			goto fail;
		}
		node_ptr->noise = noise;
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
	if (!se_sdf_set_root(temp_scene, resolved_root)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		goto fail;
	}

	char validation_error[256] = {0};
	if (!se_sdf_validate(temp_scene, validation_error, sizeof(validation_error))) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		goto fail;
	}
	s_array_clear(&created_handles);
	return se_sdf_scene_take_loaded_graph(scene, temp_scene, NULL);

fail:
	s_array_clear(&created_handles);
	se_sdf_destroy(temp_scene);
	return 0;
}

static b8 se_sdf_json_load_scene_definition(
	const se_sdf_handle scene,
	const s_json* scene_json,
	const se_sdf_handles* scene_handles
) {
	if (!scene_json || scene_json->type != S_JSON_OBJECT || !scene_handles) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	i32 root_index = -1;
	const s_json* nodes_json = se_sdf_json_get_required(scene_json, "nodes", S_JSON_ARRAY);
	if (!nodes_json || !se_sdf_json_get_i32(scene_json, "root_index", &root_index)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (root_index < -1 || (root_index >= 0 && (sz)root_index >= nodes_json->as.children.count)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
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
		i32 node_type_i32 = -1;
		i32 operation_i32 = 0;
		f32 operation_amount = SE_SDF_OPERATION_AMOUNT_DEFAULT;
		s_mat4 transform = s_mat4_identity;
		se_sdf_noise noise = SE_SDF_NOISE_DEFAULTS;
		s_vec3 color = s_vec3(0.0f, 0.0f, 0.0f);
		b8 has_color = 0;
		if (!se_sdf_json_get_bool(node_json, "is_group", &is_group) ||
			!se_sdf_json_get_i32(node_json, "operation", &operation_i32) ||
			!se_sdf_json_get_f32(node_json, "operation_amount", &operation_amount) ||
			!se_sdf_json_read_mat4_field(node_json, "transform", &transform)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (!se_sdf_json_get_i32(node_json, "node_type", &node_type_i32)) {
			node_type_i32 = is_group ? (i32)SE_SDF_NODE_GROUP : (i32)SE_SDF_NODE_PRIMITIVE;
		}
		if (node_type_i32 < (i32)SE_SDF_NODE_GROUP || node_type_i32 > (i32)SE_SDF_NODE_REF) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (operation_i32 < (i32)SE_SDF_OP_NONE || operation_i32 > (i32)SE_SDF_OP_SMOOTH_SUBTRACTION) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		const s_json* color_json = s_json_get(node_json, "color");
		const s_json* has_color_json = s_json_get(node_json, "has_color");
		if (color_json && !se_sdf_json_read_vec3_node(color_json, &color)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (has_color_json) {
			if (has_color_json->type != S_JSON_BOOL) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
			has_color = has_color_json->as.boolean;
		}
		if (!se_sdf_json_read_noise(node_json, "noise", &noise)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		se_sdf_node_handle node_handle = SE_SDF_NODE_NULL;
		if ((se_sdf_node_type)node_type_i32 == SE_SDF_NODE_GROUP) {
			se_sdf_node_group_desc group_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
			group_desc.transform = transform;
			group_desc.operation = (se_sdf_operation)operation_i32;
			group_desc.operation_amount = operation_amount;
			node_handle = se_sdf_node_create_group(scene, &group_desc);
		} else if ((se_sdf_node_type)node_type_i32 == SE_SDF_NODE_PRIMITIVE) {
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
			primitive_desc.noise = noise;
			node_handle = se_sdf_node_create_primitive(scene, &primitive_desc);
		} else {
			u32 ref_sdf_index = 0u;
			if (!se_sdf_json_get_u32(node_json, "ref_sdf_index", &ref_sdf_index) ||
				(sz)ref_sdf_index >= s_array_get_size((se_sdf_handles*)scene_handles)) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
			se_sdf_handle* ref_scene_handle = s_array_get((se_sdf_handles*)scene_handles, s_array_handle((se_sdf_handles*)scene_handles, ref_sdf_index));
			if (!ref_scene_handle || *ref_scene_handle == SE_SDF_NULL) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
			se_sdf_node_ref_desc ref_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
			ref_desc.source = *ref_scene_handle;
			ref_desc.transform = transform;
			ref_desc.operation = (se_sdf_operation)operation_i32;
			ref_desc.operation_amount = operation_amount;
			node_handle = se_sdf_node_create_ref(scene, &ref_desc);
		}

		if (node_handle == SE_SDF_NODE_NULL) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		if (has_color && !se_sdf_node_set_color(scene, node_handle, &color)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
		s_array_add(&created_handles, node_handle);

		se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node_handle);
		if (!node_ptr) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			goto fail;
		}
		node_ptr->noise = noise;
	}

	for (sz i = 0; i < nodes_json->as.children.count; ++i) {
		const s_json* node_json = s_json_at(nodes_json, i);
		const s_json* children_json = se_sdf_json_get_required(node_json, "children", S_JSON_ARRAY);
		se_sdf_node_handle* parent_handle = s_array_get(&created_handles, s_array_handle(&created_handles, (u32)i));
		if (!children_json || !parent_handle || *parent_handle == SE_SDF_NODE_NULL) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
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
				!se_sdf_node_add_child(scene, *parent_handle, *child_handle)) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				goto fail;
			}
		}
	}

	if (root_index >= 0) {
		se_sdf_node_handle* root_handle = s_array_get(&created_handles, s_array_handle(&created_handles, (u32)root_index));
		if (!root_handle || !se_sdf_set_root(scene, *root_handle)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
	} else if (!se_sdf_set_root(scene, SE_SDF_NODE_NULL)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		goto fail;
	}

	s_array_clear(&created_handles);
	se_set_last_error(SE_RESULT_OK);
	return 1;

fail:
	s_array_clear(&created_handles);
	return 0;
}

static b8 se_sdf_from_json_version_2(const se_sdf_handle scene, const s_json* root) {
	if (!root || root->type != S_JSON_OBJECT || !se_sdf_scene_from_handle(scene)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	u32 entry_sdf_index = 0u;
	const s_json* sdfs_json = se_sdf_json_get_required(root, "sdfs", S_JSON_ARRAY);
	if (!sdfs_json ||
		sdfs_json->as.children.count == 0u ||
		!se_sdf_json_get_u32(root, "entry_sdf_index", &entry_sdf_index) ||
		(sz)entry_sdf_index >= sdfs_json->as.children.count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	se_sdf_handles temp_scenes = {0};
	s_array_init(&temp_scenes);
	s_array_reserve(&temp_scenes, sdfs_json->as.children.count);

	for (sz i = 0; i < sdfs_json->as.children.count; ++i) {
		const s_json* scene_json = s_json_at(sdfs_json, i);
		const s_json* nodes_json = scene_json ? se_sdf_json_get_required(scene_json, "nodes", S_JSON_ARRAY) : NULL;
		se_sdf_desc temp_desc = SE_SDF_DESC_DEFAULTS;
		temp_desc.initial_node_capacity = nodes_json ? nodes_json->as.children.count : 0u;
		const se_sdf_handle temp_scene = se_sdf_create(&temp_desc);
		if (temp_scene == SE_SDF_NULL) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			goto fail;
		}
		s_array_add(&temp_scenes, temp_scene);
	}

	for (sz i = 0; i < sdfs_json->as.children.count; ++i) {
		const s_json* scene_json = s_json_at(sdfs_json, i);
		se_sdf_handle* temp_scene = s_array_get(&temp_scenes, s_array_handle(&temp_scenes, (u32)i));
		if (!temp_scene || *temp_scene == SE_SDF_NULL || !se_sdf_json_load_scene_definition(*temp_scene, scene_json, &temp_scenes)) {
			if (se_get_last_error() == SE_RESULT_OK) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			}
			goto fail;
		}
	}

	char validation_error[256] = {0};
	for (sz i = 0; i < s_array_get_size(&temp_scenes); ++i) {
		se_sdf_handle* temp_scene = s_array_get(&temp_scenes, s_array_handle(&temp_scenes, (u32)i));
		if (!temp_scene || *temp_scene == SE_SDF_NULL ||
			!se_sdf_validate(*temp_scene, validation_error, sizeof(validation_error))) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}
	}

	se_sdf_handles owned_import_sdfs = {0};
	s_array_init(&owned_import_sdfs);
	s_array_reserve(&owned_import_sdfs, s_array_get_size(&temp_scenes));
	se_sdf_handle* entry_scene = s_array_get(&temp_scenes, s_array_handle(&temp_scenes, entry_sdf_index));
	if (!entry_scene || *entry_scene == SE_SDF_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		s_array_clear(&owned_import_sdfs);
		goto fail;
	}
	for (sz i = 0; i < s_array_get_size(&temp_scenes); ++i) {
		if ((u32)i == entry_sdf_index) {
			continue;
		}
		se_sdf_handle* temp_scene = s_array_get(&temp_scenes, s_array_handle(&temp_scenes, (u32)i));
		if (temp_scene && *temp_scene != SE_SDF_NULL) {
			s_array_add(&owned_import_sdfs, *temp_scene);
		}
	}

	const b8 ok = se_sdf_scene_take_loaded_graph(scene, *entry_scene, &owned_import_sdfs);
	s_array_clear(&owned_import_sdfs);
	if (!ok) {
		goto fail;
	}
	s_array_clear(&temp_scenes);
	se_set_last_error(SE_RESULT_OK);
	return 1;

fail:
	se_sdf_json_destroy_scene_list(&temp_scenes);
	return 0;
}

b8 se_sdf_from_json(const se_sdf_handle scene, const s_json* root) {
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
	if (!se_sdf_json_get_string(root, "format", &format) ||
		!se_sdf_json_get_u32(root, "version", &version) ||
		strcmp(format, SE_SDF_JSON_FORMAT) != 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	if (version == 1u) {
		return se_sdf_from_json_version_1(scene, root);
	}
	if (version == 2u) {
		return se_sdf_from_json_version_2(scene, root);
	}
	se_set_last_error(SE_RESULT_UNSUPPORTED);
	return 0;
}

b8 se_sdf_from_json_file(const se_sdf_handle scene, const c8* path) {
	if (!path || path[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	c8 resolved_path[SE_MAX_PATH_LENGTH] = {0};
	const c8* read_path = path;
	c8* text = NULL;
	if (!s_file_read(read_path, &text, NULL)) {
		if (se_paths_resolve_resource_path(resolved_path, sizeof(resolved_path), path)) {
			read_path = resolved_path;
		}
		if (!s_file_read(read_path, &text, NULL)) {
			se_set_last_error(SE_RESULT_IO);
			return 0;
		}
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
	const se_sdf_handle scene,
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
	node->noise = desc->noise;
	node->color = s_vec3(0.0f, 0.0f, 0.0f);
	node->has_color = 0;
	node->primitive = desc->primitive;
	node->type = SE_SDF_NODE_PRIMITIVE;
	node->ref_source = SE_SDF_NULL;
	node->parent = SE_SDF_NODE_NULL;
	s_array_init(&node->children);
	se_sdf_touch_sdf_and_dependents(scene);
	return node_handle;
}

se_sdf_node_handle se_sdf_node_create_group(
	const se_sdf_handle scene,
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
	node->noise = SE_SDF_NOISE_DEFAULTS;
	node->color = s_vec3(0.0f, 0.0f, 0.0f);
	node->has_color = 0;
	node->type = SE_SDF_NODE_GROUP;
	node->ref_source = SE_SDF_NULL;
	node->parent = SE_SDF_NODE_NULL;
	s_array_init(&node->children);
	se_sdf_touch_sdf_and_dependents(scene);
	return node_handle;
}

se_sdf_node_handle se_sdf_node_create_ref(
	const se_sdf_handle scene,
	const se_sdf_node_ref_desc* desc
) {
	if (!desc || desc->source == SE_SDF_NULL || !se_sdf_scene_from_handle(desc->source)) {
		return SE_SDF_NODE_NULL;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
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
	node->noise = SE_SDF_NOISE_DEFAULTS;
	node->color = s_vec3(0.0f, 0.0f, 0.0f);
	node->has_color = 0;
	node->type = SE_SDF_NODE_REF;
	node->ref_source = desc->source;
	node->parent = SE_SDF_NODE_NULL;
	s_array_init(&node->children);
	se_sdf_touch_sdf_and_dependents(scene);
	return node_handle;
}

u32 se_sdf_get_node_count(const se_sdf_handle scene) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0u;
	}
	return (u32)s_array_get_size(&scene_ptr->nodes);
}

se_sdf_node_handle se_sdf_get_node(const se_sdf_handle scene, const u32 index) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr || (sz)index >= s_array_get_size(&scene_ptr->nodes)) {
		return SE_SDF_NODE_NULL;
	}
	return s_array_handle(&scene_ptr->nodes, index);
}

se_sdf_node_type se_sdf_node_get_type(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!node_ptr) {
		return SE_SDF_NODE_GROUP;
	}
	return node_ptr->type;
}

se_sdf_node_handle se_sdf_node_get_parent(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!node_ptr) {
		return SE_SDF_NODE_NULL;
	}
	return node_ptr->parent;
}

u32 se_sdf_node_get_child_count(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!node_ptr) {
		return 0u;
	}
	return (u32)s_array_get_size(&node_ptr->children);
}

se_sdf_node_handle se_sdf_node_get_child(const se_sdf_handle scene, const se_sdf_node_handle node, const u32 index) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	se_sdf_node_handle* child = NULL;
	if (!node_ptr || (sz)index >= s_array_get_size(&node_ptr->children)) {
		return SE_SDF_NODE_NULL;
	}
	child = s_array_get(&node_ptr->children, s_array_handle(&node_ptr->children, index));
	return child ? *child : SE_SDF_NODE_NULL;
}

se_sdf_operation se_sdf_node_get_operation(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!node_ptr) {
		return SE_SDF_OP_NONE;
	}
	return node_ptr->operation;
}

b8 se_sdf_node_set_operation_amount(
	const se_sdf_handle scene,
	const se_sdf_node_handle node,
	const f32 operation_amount
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!node_ptr) {
		return 0;
	}
	node_ptr->operation_amount = se_sdf_sanitize_operation_amount(node_ptr->operation, operation_amount);
	se_sdf_touch_sdf_and_dependents(scene);
	return 1;
}

f32 se_sdf_node_get_operation_amount(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!node_ptr) {
		return SE_SDF_OPERATION_AMOUNT_DEFAULT;
	}
	return node_ptr->operation_amount;
}

se_sdf_handle se_sdf_node_get_ref_source(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!se_sdf_node_is_ref(node_ptr)) {
		return SE_SDF_NULL;
	}
	return node_ptr->ref_source;
}

b8 se_sdf_node_set_primitive(
	const se_sdf_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_primitive_desc* primitive
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!primitive || !se_sdf_node_is_primitive(node_ptr) || !se_sdf_is_valid_primitive_type(primitive->type)) {
		return 0;
	}
	node_ptr->primitive = *primitive;
	se_sdf_touch_sdf_and_dependents(scene);
	return 1;
}

b8 se_sdf_node_get_primitive(
	const se_sdf_handle scene,
	const se_sdf_node_handle node,
	se_sdf_primitive_desc* out_primitive
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!out_primitive || !se_sdf_node_is_primitive(node_ptr)) {
		return 0;
	}
	*out_primitive = node_ptr->primitive;
	return 1;
}

b8 se_sdf_node_has_color(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	se_sdf_node* node_ptr = scene_ptr ? se_sdf_node_from_handle(scene_ptr, scene, node) : NULL;
	if (!se_sdf_node_is_primitive(node_ptr)) {
		return 0;
	}
	return node_ptr->has_color;
}

void se_sdf_node_destroy(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return;
	}
	if (!se_sdf_node_from_handle(scene_ptr, scene, node)) {
		return;
	}
	se_sdf_destroy_node_recursive(scene_ptr, scene, node);
	se_sdf_touch_sdf_and_dependents(scene);
}

b8 se_sdf_node_add_child(
	const se_sdf_handle scene,
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
	if (se_sdf_node_is_ref(parent_node)) {
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
	se_sdf_touch_sdf_and_dependents(scene);
	return 1;
}

b8 se_sdf_node_remove_child(
	const se_sdf_handle scene,
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
	se_sdf_touch_sdf_and_dependents(scene);
	return 1;
}

b8 se_sdf_node_set_operation(
	const se_sdf_handle scene,
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
	se_sdf_touch_sdf_and_dependents(scene);
	return 1;
}

b8 se_sdf_node_set_transform(
	const se_sdf_handle scene,
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
	if (se_sdf_scene_update_prepared_ref_subtree(scene_ptr, scene, node)) {
		se_sdf_mark_dependent_scenes_dirty(scene);
		return 1;
	}
	se_sdf_touch_sdf_and_dependents(scene);
	return 1;
}

s_mat4 se_sdf_node_get_transform(const se_sdf_handle scene, const se_sdf_node_handle node) {
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

b8 se_sdf_node_set_noise(
	const se_sdf_handle scene,
	const se_sdf_node_handle node,
	const se_sdf_noise* noise
) {
	if (!noise) {
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!se_sdf_node_is_primitive(node_ptr)) {
		return 0;
	}
	node_ptr->noise = *noise;
	se_sdf_touch_sdf_and_dependents(scene);
	return 1;
}

se_sdf_noise se_sdf_node_get_noise(const se_sdf_handle scene, const se_sdf_node_handle node) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return SE_SDF_NOISE_DEFAULTS;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return SE_SDF_NOISE_DEFAULTS;
	}
	return node_ptr->noise;
}

b8 se_sdf_node_set_color(
	const se_sdf_handle scene,
	const se_sdf_node_handle node,
	const s_vec3* color
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr) {
		return 0;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!se_sdf_node_is_primitive(node_ptr)) {
		return 0;
	}
	if (!color) {
		node_ptr->color = s_vec3(0.0f, 0.0f, 0.0f);
		node_ptr->has_color = 0;
		se_sdf_touch_sdf_and_dependents(scene);
		return 1;
	}
	node_ptr->color = *color;
	node_ptr->has_color = 1;
	se_sdf_touch_sdf_and_dependents(scene);
	return 1;
}

b8 se_sdf_node_get_color(
	const se_sdf_handle scene,
	const se_sdf_node_handle node,
	s_vec3* out_color
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr || !out_color) {
		return 0;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!se_sdf_node_is_primitive(node_ptr) || !node_ptr->has_color) {
		return 0;
	}
	*out_color = node_ptr->color;
	return 1;
}

s_mat4 se_sdf_transform(const s_vec3 translation, const s_vec3 rotation, const s_vec3 scale) {
	s_mat4 transform = s_mat4_identity;

	transform = s_mat4_translate(&transform, &translation);
	if (rotation.x != 0.0f) transform = s_mat4_rotate_x(&transform, rotation.x);
	if (rotation.y != 0.0f) transform = s_mat4_rotate_y(&transform, rotation.y);
	if (rotation.z != 0.0f) transform = s_mat4_rotate_z(&transform, rotation.z);
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
	return se_sdf_transform(s_vec3(x, y, z), s_vec3(0.0f, yaw, 0.0f), s_vec3(sx, sy, sz));
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
	const se_sdf_handle scene,
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

	if (se_sdf_get_root(scene) == SE_SDF_NODE_NULL) {
		(void)se_sdf_set_root(scene, node);
	}
	return node;
}

b8 se_sdf_build_single_object_preset(
	const se_sdf_handle scene,
	const se_sdf_primitive_desc* primitive,
	const s_mat4* transform,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_object
) {
	if (scene == SE_SDF_NULL) {
		return 0;
	}
	if (out_root) *out_root = SE_SDF_NODE_NULL;
	if (out_object) *out_object = SE_SDF_NODE_NULL;

	se_sdf_clear(scene);

	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.transform = s_mat4_identity;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(scene, root)) {
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

b8 se_sdf_build_grid_preset(
	const se_sdf_handle scene,
	const se_sdf_primitive_desc* primitive,
	const i32 columns,
	const i32 rows,
	const f32 spacing,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_first
) {
	if (scene == SE_SDF_NULL || columns <= 0 || rows <= 0) {
		return 0;
	}
	if (out_root) *out_root = SE_SDF_NODE_NULL;
	if (out_first) *out_first = SE_SDF_NODE_NULL;

	se_sdf_clear(scene);

	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.transform = s_mat4_identity;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(scene, root)) {
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

b8 se_sdf_build_primitive_gallery_preset(
	const se_sdf_handle scene,
	const i32 columns,
	const i32 rows,
	const f32 spacing,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_focus
) {
	if (scene == SE_SDF_NULL || columns <= 0 || rows <= 0) {
		return 0;
	}
	if (out_root) *out_root = SE_SDF_NODE_NULL;
	if (out_focus) *out_focus = SE_SDF_NODE_NULL;

	se_sdf_clear(scene);

	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.transform = s_mat4_identity;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(scene, root)) {
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

b8 se_sdf_build_orbit_showcase_preset(
	const se_sdf_handle scene,
	const se_sdf_primitive_desc* center_primitive,
	const se_sdf_primitive_desc* orbit_primitive,
	const i32 orbit_count,
	const f32 orbit_radius,
	se_sdf_node_handle* out_root,
	se_sdf_node_handle* out_center
) {
	if (scene == SE_SDF_NULL || orbit_count <= 0 || orbit_radius <= 0.0f) {
		return 0;
	}
	if (out_root) *out_root = SE_SDF_NODE_NULL;
	if (out_center) *out_center = SE_SDF_NODE_NULL;

	se_sdf_clear(scene);

	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.transform = s_mat4_identity;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(scene, root)) {
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
		const s_mat4 orbit_transform = se_sdf_transform(
			s_vec3(x, 0.35f, z),
			s_vec3(0.0f, yaw, 0.0f),
			s_vec3(0.72f, 0.72f, 0.72f)
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

static void se_sdf_scene_bounds_expand_point(se_sdf_bounds* bounds, const s_vec3* point) {
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
	se_sdf_bounds* bounds,
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

static void se_sdf_expand_local_bounds_by_noise(
	const se_sdf_noise* noise,
	s_vec3* local_min,
	s_vec3* local_max
) {
	if (!noise || !noise->active || !local_min || !local_max) {
		return;
	}
	const f32 pad = fabsf(noise->offset) + fabsf(noise->scale);
	local_min->x -= pad;
	local_min->y -= pad;
	local_min->z -= pad;
	local_max->x += pad;
	local_max->y += pad;
	local_max->z += pad;
}

static f32 se_sdf_noise_apply_distance(
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
	const se_sdf_handle scene,
	const se_sdf_node_handle node_handle,
	const s_mat4* parent_transform,
	se_sdf_bounds* bounds,
	const sz depth,
	const sz max_depth,
	const u32 include_mask
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

	if (se_sdf_node_is_ref(node)) {
		if ((include_mask & SE_SDF_QUERY_INCLUDE_REFS) == 0u) {
			return;
		}
		se_sdf_scene* ref_scene_ptr = se_sdf_scene_from_handle(node->ref_source);
		if (!ref_scene_ptr || ref_scene_ptr->root == SE_SDF_NODE_NULL) {
			return;
		}
		se_sdf_collect_scene_bounds_recursive(
			ref_scene_ptr,
			node->ref_source,
			ref_scene_ptr->root,
			&world_transform,
			bounds,
			depth + 1,
			max_depth,
			include_mask
		);
		return;
	}

	if (se_sdf_node_is_primitive(node) && (include_mask & SE_SDF_QUERY_INCLUDE_LOCAL) != 0u) {
		s_vec3 local_min = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 local_max = s_vec3(0.0f, 0.0f, 0.0f);
		b8 is_unbounded = 0;
		if (se_sdf_get_primitive_local_bounds(&node->primitive, &local_min, &local_max, &is_unbounded)) {
			se_sdf_expand_local_bounds_by_noise(&node->noise, &local_min, &local_max);
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
			max_depth,
			include_mask
		);
	}
}

b8 se_sdf_calculate_bounds(const se_sdf_handle scene, se_sdf_bounds* out_bounds) {
	if (!out_bounds || scene == SE_SDF_NULL) {
		return 0;
	}

	*out_bounds = SE_SDF_BOUNDS_DEFAULTS;

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
		max_depth,
		SE_SDF_QUERY_INCLUDE_ALL
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

static b8 se_sdf_calculate_bounds_filtered_internal(
	const se_sdf_handle scene,
	se_sdf_bounds* out_bounds,
	const u32 include_mask
) {
	if (!out_bounds || scene == SE_SDF_NULL || include_mask == 0u) {
		return 0;
	}

	*out_bounds = SE_SDF_BOUNDS_DEFAULTS;

	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr || scene_ptr->root == SE_SDF_NODE_NULL) {
		return 0;
	}

	const sz max_depth = s_array_get_size(&scene_ptr->nodes) + 1u;
	se_sdf_collect_scene_bounds_recursive(
		scene_ptr,
		scene,
		scene_ptr->root,
		NULL,
		out_bounds,
		0u,
		max_depth,
		include_mask
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

static se_sdf_bounds se_sdf_prepare_expand_bounds(
	se_sdf_bounds bounds,
	const se_sdf_prepare_desc* prepare_desc
) {
	if (!bounds.valid || !prepare_desc) {
		return bounds;
	}
	const s_vec3 center = bounds.center;
	const s_vec3 half = s_vec3(
		fmaxf((bounds.max.x - bounds.min.x) * 0.5f, 0.25f),
		fmaxf((bounds.max.y - bounds.min.y) * 0.5f, 0.25f),
		fmaxf((bounds.max.z - bounds.min.z) * 0.5f, 0.25f)
	);
	const f32 pad = fmaxf(0.0f, prepare_desc->max_distance_scale - 1.0f);
	bounds.min = s_vec3(center.x - half.x * (1.0f + pad), center.y - half.y * (1.0f + pad), center.z - half.z * (1.0f + pad));
	bounds.max = s_vec3(center.x + half.x * (1.0f + pad), center.y + half.y * (1.0f + pad), center.z + half.z * (1.0f + pad));
	bounds.center = center;
	bounds.radius = 0.5f * sqrtf(
		(bounds.max.x - bounds.min.x) * (bounds.max.x - bounds.min.x) +
		(bounds.max.y - bounds.min.y) * (bounds.max.y - bounds.min.y) +
		(bounds.max.z - bounds.min.z) * (bounds.max.z - bounds.min.z)
	);
	return bounds;
}

static b8 se_sdf_prepare_desc_equals(const se_sdf_prepare_desc* a, const se_sdf_prepare_desc* b) {
	if (!a || !b) {
		return 0;
	}
	if (a->lod_count != b->lod_count ||
		a->brick_size != b->brick_size ||
		a->brick_border != b->brick_border ||
		a->max_distance_scale != b->max_distance_scale) {
		return 0;
	}
	for (u32 i = 0; i < 4u; ++i) {
		if (a->lod_resolutions[i] != b->lod_resolutions[i]) {
			return 0;
		}
	}
	return 1;
}

static u64 se_sdf_prepare_dependency_stamp_recursive(
	const se_sdf_handle sdf,
	const sz depth,
	const sz max_depth
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	u64 stamp = 1469598103934665603ull;
	if (!scene_ptr || depth > max_depth) {
		return 0ull;
	}
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		se_sdf_node* node = s_array_get(&scene_ptr->nodes, s_array_handle(&scene_ptr->nodes, (u32)i));
		if (!node || !se_sdf_node_is_ref(node)) {
			continue;
		}
		se_sdf_scene* child_ptr = se_sdf_scene_from_handle(node->ref_source);
		const u64 child_generation = child_ptr ? child_ptr->generation : 0ull;
		const u64 child_stamp = child_ptr ? se_sdf_prepare_dependency_stamp_recursive(node->ref_source, depth + 1u, max_depth) : 0ull;
		stamp ^= (u64)node->ref_source + 0x9e3779b97f4a7c15ull + (stamp << 6u) + (stamp >> 2u);
		stamp ^= child_generation + 0x9e3779b97f4a7c15ull + (stamp << 6u) + (stamp >> 2u);
		stamp ^= child_stamp + 0x9e3779b97f4a7c15ull + (stamp << 6u) + (stamp >> 2u);
	}
	return stamp;
}

static b8 se_sdf_prepare_build_lod_atlas(
	se_sdf_prepared_lod* out_lod,
	const f32* distances,
	const u8* colors,
	const u32 resolution,
	const u32 brick_size,
	const u32 brick_border
) {
	if (!out_lod || !distances || resolution == 0u || brick_size == 0u) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	const u32 safe_border = brick_border > 8u ? 8u : brick_border;
	const u32 brick_extent = brick_size + safe_border * 2u;
	const u32 brick_count_x = (resolution + brick_size - 1u) / brick_size;
	const u32 brick_count_y = brick_count_x;
	const u32 brick_count_z = brick_count_x;
	const u32 atlas_width = brick_count_x * brick_extent;
	const u32 atlas_height = brick_count_y * brick_extent;
	const u32 atlas_depth = brick_count_z * brick_extent;
	const sz atlas_voxel_count = (sz)atlas_width * (sz)atlas_height * (sz)atlas_depth;
	const sz page_voxel_count = (sz)brick_count_x * (sz)brick_count_y * (sz)brick_count_z;
	f32* atlas_distances = calloc(atlas_voxel_count, sizeof(*atlas_distances));
	f32* atlas_colors = colors ? calloc(atlas_voxel_count * 4u, sizeof(*atlas_colors)) : NULL;
	f32* page_table = calloc(page_voxel_count * 4u, sizeof(*page_table));
	if (!atlas_distances || !page_table || (colors && !atlas_colors)) {
		free(atlas_distances);
		free(atlas_colors);
		free(page_table);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return 0;
	}

	for (u32 bz = 0u; bz < brick_count_z; ++bz) {
		for (u32 by = 0u; by < brick_count_y; ++by) {
			for (u32 bx = 0u; bx < brick_count_x; ++bx) {
				const u32 brick_origin_x = bx * brick_extent;
				const u32 brick_origin_y = by * brick_extent;
				const u32 brick_origin_z = bz * brick_extent;
				const sz page_index = (sz)bx + (sz)brick_count_x * ((sz)by + (sz)brick_count_y * (sz)bz);
				const sz page_base = page_index * 4u;
				page_table[page_base + 0u] = ((f32)brick_origin_x + (f32)safe_border + 0.5f) / (f32)atlas_width;
				page_table[page_base + 1u] = ((f32)brick_origin_y + (f32)safe_border + 0.5f) / (f32)atlas_height;
				page_table[page_base + 2u] = ((f32)brick_origin_z + (f32)safe_border + 0.5f) / (f32)atlas_depth;
				page_table[page_base + 3u] = 1.0f;

				for (u32 local_z = 0u; local_z < brick_extent; ++local_z) {
					const i32 src_z = s_max(0, s_min((i32)resolution - 1, (i32)(bz * brick_size + local_z) - (i32)safe_border));
					const u32 dst_z = brick_origin_z + local_z;
					for (u32 local_y = 0u; local_y < brick_extent; ++local_y) {
						const i32 src_y = s_max(0, s_min((i32)resolution - 1, (i32)(by * brick_size + local_y) - (i32)safe_border));
						const u32 dst_y = brick_origin_y + local_y;
						for (u32 local_x = 0u; local_x < brick_extent; ++local_x) {
							const i32 src_x = s_max(0, s_min((i32)resolution - 1, (i32)(bx * brick_size + local_x) - (i32)safe_border));
							const u32 dst_x = brick_origin_x + local_x;
							const sz src_index = (sz)(u32)src_x + (sz)resolution * ((sz)(u32)src_y + (sz)resolution * (sz)(u32)src_z);
							const sz dst_index = (sz)dst_x + (sz)atlas_width * ((sz)dst_y + (sz)atlas_height * (sz)dst_z);
							atlas_distances[dst_index] = distances[src_index];
							if (atlas_colors) {
								const sz src_color = src_index * 4u;
								const sz dst_color = dst_index * 4u;
								atlas_colors[dst_color + 0u] = (f32)colors[src_color + 0u] / 255.0f;
								atlas_colors[dst_color + 1u] = (f32)colors[src_color + 1u] / 255.0f;
								atlas_colors[dst_color + 2u] = (f32)colors[src_color + 2u] / 255.0f;
								atlas_colors[dst_color + 3u] = (f32)colors[src_color + 3u] / 255.0f;
							}
						}
					}
				}
			}
		}
	}

	se_texture_handle atlas_texture = se_texture_create_3d_r16f(atlas_distances, atlas_width, atlas_height, atlas_depth, SE_CLAMP);
	se_texture_handle color_texture = S_HANDLE_NULL;
	se_texture_handle page_table_texture = S_HANDLE_NULL;
	if (atlas_texture != S_HANDLE_NULL) {
		if (atlas_colors) {
			color_texture = se_texture_create_3d_rgba16f(atlas_colors, atlas_width, atlas_height, atlas_depth, SE_CLAMP);
		}
		page_table_texture = se_texture_create_3d_rgba16f(page_table, brick_count_x, brick_count_y, brick_count_z, SE_CLAMP);
	}
	free(atlas_distances);
	free(atlas_colors);
	free(page_table);
	if (atlas_texture == S_HANDLE_NULL || page_table_texture == S_HANDLE_NULL || (colors && color_texture == S_HANDLE_NULL)) {
		if (atlas_texture != S_HANDLE_NULL) {
			se_texture_destroy(atlas_texture);
		}
		if (color_texture != S_HANDLE_NULL) {
			se_texture_destroy(color_texture);
		}
		if (page_table_texture != S_HANDLE_NULL) {
			se_texture_destroy(page_table_texture);
		}
		return 0;
	}

	out_lod->texture = atlas_texture;
	out_lod->color_texture = color_texture;
	out_lod->page_table_texture = page_table_texture;
	out_lod->brick_size = brick_size;
	out_lod->brick_border = safe_border;
	out_lod->brick_count_x = brick_count_x;
	out_lod->brick_count_y = brick_count_y;
	out_lod->brick_count_z = brick_count_z;
	out_lod->atlas_width = atlas_width;
	out_lod->atlas_height = atlas_height;
	out_lod->atlas_depth = atlas_depth;
	return 1;
}

static b8 se_sdf_prepared_lod_finalize_gpu(
	se_sdf_prepared_lod* lod,
	const u32 brick_size,
	const u32 brick_border
) {
	if (!lod || !lod->valid || !lod->cpu_distances || lod->cpu_distance_count == 0u) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (lod->texture != S_HANDLE_NULL && lod->page_table_texture != S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_OK);
		return 1;
	}
	return se_sdf_prepare_build_lod_atlas(
		lod,
		lod->cpu_distances,
		lod->cpu_colors,
		lod->resolution,
		brick_size,
		brick_border
	);
}

static b8 se_sdf_prepare_volume_lod(
	const se_sdf_handle sdf,
	const se_sdf_bounds* bounds,
	const u32 resolution,
	const u32 brick_size,
	const u32 brick_border,
	se_sdf_prepared_lod* out_lod,
	const u32 include_mask,
	const b8 build_gpu_resources
) {
	if (sdf == SE_SDF_NULL || !bounds || !bounds->valid || resolution < 4u || !out_lod || include_mask == 0u) {
		return 0;
	}

	const s_vec3 span = s_vec3(
		fmaxf(bounds->max.x - bounds->min.x, 0.001f),
		fmaxf(bounds->max.y - bounds->min.y, 0.001f),
		fmaxf(bounds->max.z - bounds->min.z, 0.001f)
	);
	const s_vec3 voxel_size = s_vec3(
		span.x / (f32)resolution,
		span.y / (f32)resolution,
		span.z / (f32)resolution
	);
	const sz voxel_count = (sz)resolution * (sz)resolution * (sz)resolution;
	f32* distances = calloc(voxel_count, sizeof(*distances));
	u8* colors = calloc(voxel_count * 4u, sizeof(*colors));
	if (!distances || !colors) {
		free(distances);
		free(colors);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return 0;
	}

	f32 max_abs_distance = 0.0f;
	b8 any_color = 0;
	for (u32 z = 0u; z < resolution; ++z) {
		for (u32 y = 0u; y < resolution; ++y) {
			for (u32 x = 0u; x < resolution; ++x) {
				const s_vec3 point = s_vec3(
					bounds->min.x + (((f32)x + 0.5f) / (f32)resolution) * span.x,
					bounds->min.y + (((f32)y + 0.5f) / (f32)resolution) * span.y,
					bounds->min.z + (((f32)z + 0.5f) / (f32)resolution) * span.z
				);
				f32 distance = 0.0f;
				s_vec3 hit_color = s_vec3(0.0f, 0.0f, 0.0f);
				b8 has_hit_color = 0;
				if (!se_sdf_query_sample_distance_filtered_internal(
						sdf,
						&point,
						&distance,
						NULL,
						NULL,
						&hit_color,
						&has_hit_color,
						include_mask)) {
					free(distances);
					free(colors);
					return 0;
				}
				const sz index = (sz)x + (sz)resolution * ((sz)y + (sz)resolution * (sz)z);
				distances[index] = distance;
				if (has_hit_color) {
					const sz color_index = index * 4u;
					colors[color_index + 0u] = (u8)s_max(0, s_min(255, (i32)roundf(hit_color.x * 255.0f)));
					colors[color_index + 1u] = (u8)s_max(0, s_min(255, (i32)roundf(hit_color.y * 255.0f)));
					colors[color_index + 2u] = (u8)s_max(0, s_min(255, (i32)roundf(hit_color.z * 255.0f)));
					colors[color_index + 3u] = 255u;
					any_color = 1;
				}
				const f32 abs_distance = fabsf(distance);
				if (abs_distance > max_abs_distance) {
					max_abs_distance = abs_distance;
				}
			}
		}
	}

	se_sdf_prepared_lod_clear(out_lod);
	out_lod->cpu_distances = distances;
	out_lod->cpu_distance_count = voxel_count;
	if (!any_color) {
		free(colors);
		colors = NULL;
	}
	out_lod->cpu_colors = colors;
	out_lod->cpu_color_count = colors ? voxel_count * 4u : 0u;
	out_lod->resolution = resolution;
	out_lod->voxel_size = voxel_size;
	out_lod->max_distance = max_abs_distance;
	out_lod->valid = 1;
	if (build_gpu_resources && !se_sdf_prepared_lod_finalize_gpu(out_lod, brick_size, brick_border)) {
		se_sdf_prepared_lod_clear(out_lod);
		return 0;
	}
	return 1;
}

static b8 se_sdf_prepared_cache_finalize_gpu(se_sdf_prepared_cache* cache) {
	if (!cache) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	for (u32 i = 0u; i < cache->desc.lod_count && i < 4u; ++i) {
		if (cache->local_lods_valid && cache->local_lods[i].valid &&
			!se_sdf_prepared_lod_finalize_gpu(&cache->local_lods[i], cache->desc.brick_size, cache->desc.brick_border)) {
			return 0;
		}
		if (cache->full_lods_valid && cache->full_lods[i].valid &&
			!se_sdf_prepared_lod_finalize_gpu(&cache->full_lods[i], cache->desc.brick_size, cache->desc.brick_border)) {
			return 0;
		}
	}
	se_set_last_error(SE_RESULT_OK);
	return 1;
}

static b8 se_sdf_prepare_collect_refs_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle sdf,
	const se_sdf_node_handle node_handle,
	const s_mat4* parent_transform,
	se_sdf_prepared_cache* cache,
	const sz depth,
	const sz max_depth
) {
	if (!scene_ptr || !cache || node_handle == SE_SDF_NODE_NULL || depth > max_depth) {
		return 0;
	}
	se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, sdf, node_handle);
	if (!node) {
		return 0;
	}
	const s_mat4 world_transform = parent_transform ? s_mat4_mul(parent_transform, &node->transform) : node->transform;
	if (se_sdf_node_is_primitive(node)) {
		cache->has_local_content = 1;
	}
	if (se_sdf_node_is_ref(node)) {
		se_sdf_scene* child_ptr = se_sdf_scene_from_handle(node->ref_source);
		if (!child_ptr || !child_ptr->prepared.ready) {
			return 0;
		}
		se_sdf_prepared_ref ref = {0};
		ref.source = node->ref_source;
		ref.node = node_handle;
		ref.transform = world_transform;
		ref.inverse_transform = s_mat4_inverse(&world_transform);
		ref.bounds = child_ptr->prepared.bounds;
		ref.bounds.valid = child_ptr->prepared.bounds.valid;
		if (ref.bounds.valid) {
			se_sdf_bounds transformed = SE_SDF_BOUNDS_DEFAULTS;
			se_sdf_scene_bounds_expand_transformed_aabb(&transformed, &world_transform, &ref.bounds.min, &ref.bounds.max);
			transformed.center = s_vec3(
				(transformed.min.x + transformed.max.x) * 0.5f,
				(transformed.min.y + transformed.max.y) * 0.5f,
				(transformed.min.z + transformed.max.z) * 0.5f
			);
			const s_vec3 transformed_span = s_vec3(
				transformed.max.x - transformed.min.x,
				transformed.max.y - transformed.min.y,
				transformed.max.z - transformed.min.z
			);
			transformed.radius = 0.5f * sqrtf(
				transformed_span.x * transformed_span.x +
				transformed_span.y * transformed_span.y +
				transformed_span.z * transformed_span.z
			);
			ref.bounds = transformed;
		}
		s_array_add(&cache->refs, ref);
		return 1;
	}

	for (sz i = 0; i < s_array_get_size(&node->children); ++i) {
		se_sdf_node_handle* child = s_array_get(&node->children, s_array_handle(&node->children, (u32)i));
		if (!child) {
			continue;
		}
		if (!se_sdf_prepare_collect_refs_recursive(scene_ptr, sdf, *child, &world_transform, cache, depth + 1u, max_depth)) {
			return 0;
		}
	}
	return 1;
}

static b8 se_sdf_prepare_tree_supports_instancing_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle sdf,
	const se_sdf_node_handle node_handle,
	const sz depth,
	const sz max_depth
) {
	if (!scene_ptr || node_handle == SE_SDF_NODE_NULL || depth > max_depth) {
		return 0;
	}
	se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, sdf, node_handle);
	if (!node) {
		return 0;
	}
	if (node->operation != SE_SDF_OP_NONE && node->operation != SE_SDF_OP_UNION) {
		return 0;
	}
	for (sz i = 0; i < s_array_get_size(&node->children); ++i) {
		se_sdf_node_handle* child = s_array_get(&node->children, s_array_handle(&node->children, (u32)i));
		if (!child) {
			continue;
		}
		if (!se_sdf_prepare_tree_supports_instancing_recursive(scene_ptr, sdf, *child, depth + 1u, max_depth)) {
			return 0;
		}
	}
	return 1;
}

static void se_sdf_prepare_build_grid(se_sdf_prepared_cache* cache) {
	if (!cache || !cache->bounds.valid) {
		return;
	}
	se_sdf_prepared_grid_clear(&cache->grid_cells);
	cache->cell_size = 16.0f;
	cache->grid_min = cache->bounds.min;
	const s_vec3 span = s_vec3(
		fmaxf(cache->bounds.max.x - cache->bounds.min.x, cache->cell_size),
		fmaxf(cache->bounds.max.y - cache->bounds.min.y, cache->cell_size),
		fmaxf(cache->bounds.max.z - cache->bounds.min.z, cache->cell_size)
	);
	cache->grid_nx = s_max(1, (i32)ceilf(span.x / cache->cell_size));
	cache->grid_ny = s_max(1, (i32)ceilf(span.y / cache->cell_size));
	cache->grid_nz = s_max(1, (i32)ceilf(span.z / cache->cell_size));
	const sz cell_count = (sz)cache->grid_nx * (sz)cache->grid_ny * (sz)cache->grid_nz;
	s_array_reserve(&cache->grid_cells, cell_count);
	for (sz i = 0; i < cell_count; ++i) {
		se_sdf_grid_cell cell = {0};
		s_array_init(&cell.ref_indices);
		s_array_add(&cache->grid_cells, cell);
	}

	for (u32 ref_index = 0u; ref_index < (u32)s_array_get_size(&cache->refs); ++ref_index) {
		se_sdf_prepared_ref* ref = s_array_get(&cache->refs, s_array_handle(&cache->refs, ref_index));
		if (!ref || !ref->bounds.valid) {
			continue;
		}
		const i32 min_x = s_max(0, s_min(cache->grid_nx - 1, (i32)floorf((ref->bounds.min.x - cache->grid_min.x) / cache->cell_size)));
		const i32 min_y = s_max(0, s_min(cache->grid_ny - 1, (i32)floorf((ref->bounds.min.y - cache->grid_min.y) / cache->cell_size)));
		const i32 min_z = s_max(0, s_min(cache->grid_nz - 1, (i32)floorf((ref->bounds.min.z - cache->grid_min.z) / cache->cell_size)));
		const i32 max_x = s_max(0, s_min(cache->grid_nx - 1, (i32)floorf((ref->bounds.max.x - cache->grid_min.x) / cache->cell_size)));
		const i32 max_y = s_max(0, s_min(cache->grid_ny - 1, (i32)floorf((ref->bounds.max.y - cache->grid_min.y) / cache->cell_size)));
		const i32 max_z = s_max(0, s_min(cache->grid_nz - 1, (i32)floorf((ref->bounds.max.z - cache->grid_min.z) / cache->cell_size)));
		for (i32 z = min_z; z <= max_z; ++z) {
			for (i32 y = min_y; y <= max_y; ++y) {
				for (i32 x = min_x; x <= max_x; ++x) {
					const sz cell_index = (sz)x + (sz)cache->grid_nx * ((sz)y + (sz)cache->grid_ny * (sz)z);
					se_sdf_grid_cell* cell = s_array_get(&cache->grid_cells, s_array_handle(&cache->grid_cells, (u32)cell_index));
					if (!cell) {
						continue;
					}
					s_array_add(&cell->ref_indices, ref_index);
				}
			}
		}
	}
}

static void se_sdf_prepared_ref_apply_bounds(
	se_sdf_prepared_ref* ref,
	const s_mat4* world_transform,
	const se_sdf_bounds* child_bounds
) {
	if (!ref || !world_transform) {
		return;
	}
	ref->transform = *world_transform;
	ref->inverse_transform = s_mat4_inverse(world_transform);
	ref->bounds = child_bounds ? *child_bounds : SE_SDF_BOUNDS_DEFAULTS;
	ref->bounds.valid = child_bounds ? child_bounds->valid : 0;
	if (ref->bounds.valid) {
		se_sdf_bounds transformed = SE_SDF_BOUNDS_DEFAULTS;
		se_sdf_scene_bounds_expand_transformed_aabb(&transformed, world_transform, &ref->bounds.min, &ref->bounds.max);
		se_sdf_physics_finalize_bounds(&transformed);
		ref->bounds = transformed;
	}
}

static void se_sdf_prepared_recalculate_bounds(se_sdf_prepared_cache* cache) {
	if (!cache) {
		return;
	}
	se_sdf_bounds bounds = SE_SDF_BOUNDS_DEFAULTS;
	if (cache->local_bounds.valid) {
		bounds = cache->local_bounds;
	}
	for (sz i = 0; i < s_array_get_size(&cache->refs); ++i) {
		se_sdf_prepared_ref* ref = s_array_get(&cache->refs, s_array_handle(&cache->refs, (u32)i));
		if (!ref || !ref->bounds.valid) {
			continue;
		}
		if (!bounds.valid) {
			bounds = ref->bounds;
			continue;
		}
		bounds.min.x = fminf(bounds.min.x, ref->bounds.min.x);
		bounds.min.y = fminf(bounds.min.y, ref->bounds.min.y);
		bounds.min.z = fminf(bounds.min.z, ref->bounds.min.z);
		bounds.max.x = fmaxf(bounds.max.x, ref->bounds.max.x);
		bounds.max.y = fmaxf(bounds.max.y, ref->bounds.max.y);
		bounds.max.z = fmaxf(bounds.max.z, ref->bounds.max.z);
		bounds.has_unbounded_primitives = bounds.has_unbounded_primitives || ref->bounds.has_unbounded_primitives;
	}
	if (bounds.valid) {
		se_sdf_physics_finalize_bounds(&bounds);
	}
	cache->bounds = bounds;
}

static void se_sdf_color_accumulator_add(
	se_sdf_color_accumulator* accumulator,
	const s_vec3* color,
	const u32 weight
) {
	if (!accumulator || !color || weight == 0u) {
		return;
	}
	accumulator->sum.x += color->x * (f32)weight;
	accumulator->sum.y += color->y * (f32)weight;
	accumulator->sum.z += color->z * (f32)weight;
	accumulator->weight += weight;
}

static b8 se_sdf_prepare_collect_color_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle sdf,
	const se_sdf_node_handle node_handle,
	const u32 query_flags,
	const sz depth,
	const sz max_depth,
	se_sdf_color_accumulator* accumulator
) {
	if (!scene_ptr || node_handle == SE_SDF_NODE_NULL || depth > max_depth || !accumulator) {
		return 0;
	}
	se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, sdf, node_handle);
	if (!node) {
		return 0;
	}
	if (se_sdf_node_is_primitive(node)) {
		if ((query_flags & SE_SDF_QUERY_INCLUDE_LOCAL) != 0u && node->has_color) {
			se_sdf_color_accumulator_add(accumulator, &node->color, 1u);
		}
	} else if (se_sdf_node_is_ref(node)) {
		if ((query_flags & SE_SDF_QUERY_INCLUDE_REFS) != 0u) {
			se_sdf_scene* child_ptr = se_sdf_scene_from_handle(node->ref_source);
			if (!child_ptr || !child_ptr->prepared.ready) {
				return 0;
			}
			if (child_ptr->prepared.full_color_valid && child_ptr->prepared.full_color_weight > 0u) {
				se_sdf_color_accumulator_add(
					accumulator,
					&child_ptr->prepared.full_color,
					child_ptr->prepared.full_color_weight
				);
			}
		}
		return 1;
	}
	for (sz i = 0; i < s_array_get_size(&node->children); ++i) {
		se_sdf_node_handle* child = s_array_get(&node->children, s_array_handle(&node->children, (u32)i));
		if (!child) {
			continue;
		}
		if (!se_sdf_prepare_collect_color_recursive(
				scene_ptr,
				sdf,
				*child,
				query_flags,
				depth + 1u,
				max_depth,
				accumulator)) {
			return 0;
		}
	}
	return 1;
}

static void se_sdf_prepare_calculate_colors(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle sdf
) {
	if (!scene_ptr || scene_ptr->root == SE_SDF_NODE_NULL) {
		return;
	}
	se_sdf_color_accumulator local_accumulator = {0};
	se_sdf_color_accumulator full_accumulator = {0};
	const sz max_depth = s_array_get_size(&scene_ptr->nodes) + 256u;
	if (!se_sdf_prepare_collect_color_recursive(
			scene_ptr,
			sdf,
			scene_ptr->root,
			SE_SDF_QUERY_INCLUDE_LOCAL,
			0u,
			max_depth,
			&local_accumulator)) {
		return;
	}
	if (!se_sdf_prepare_collect_color_recursive(
			scene_ptr,
			sdf,
			scene_ptr->root,
			SE_SDF_QUERY_INCLUDE_ALL,
			0u,
			max_depth,
			&full_accumulator)) {
		return;
	}
	if (local_accumulator.weight > 0u) {
		const f32 inv_weight = 1.0f / (f32)local_accumulator.weight;
		scene_ptr->prepared.local_color = s_vec3(
			local_accumulator.sum.x * inv_weight,
			local_accumulator.sum.y * inv_weight,
			local_accumulator.sum.z * inv_weight
		);
		scene_ptr->prepared.local_color_weight = local_accumulator.weight;
		scene_ptr->prepared.local_color_valid = 1;
	}
	if (full_accumulator.weight > 0u) {
		const f32 inv_weight = 1.0f / (f32)full_accumulator.weight;
		scene_ptr->prepared.full_color = s_vec3(
			full_accumulator.sum.x * inv_weight,
			full_accumulator.sum.y * inv_weight,
			full_accumulator.sum.z * inv_weight
		);
		scene_ptr->prepared.full_color_weight = full_accumulator.weight;
		scene_ptr->prepared.full_color_valid = 1;
	}
}

static b8 se_sdf_node_calculate_world_transform(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle scene,
	const se_sdf_node_handle node,
	s_mat4* out_transform
) {
	if (!scene_ptr || node == SE_SDF_NODE_NULL || !out_transform) {
		return 0;
	}
	se_sdf_node_handles chain;
	s_array_init(&chain);
	se_sdf_node_handle current = node;
	const sz max_depth = s_array_get_size(&scene_ptr->nodes) + 1u;
	for (sz depth = 0u; current != SE_SDF_NODE_NULL && depth < max_depth; ++depth) {
		s_array_add(&chain, current);
		se_sdf_node* current_node = se_sdf_node_from_handle(scene_ptr, scene, current);
		if (!current_node) {
			s_array_clear(&chain);
			return 0;
		}
		current = current_node->parent;
	}
	if (current != SE_SDF_NODE_NULL) {
		s_array_clear(&chain);
		return 0;
	}
	*out_transform = s_mat4_identity;
	for (sz i = s_array_get_size(&chain); i > 0u; --i) {
		se_sdf_node_handle* handle = s_array_get(&chain, s_array_handle(&chain, (u32)(i - 1u)));
		se_sdf_node* chain_node = handle ? se_sdf_node_from_handle(scene_ptr, scene, *handle) : NULL;
		if (!chain_node) {
			s_array_clear(&chain);
			return 0;
		}
		*out_transform = s_mat4_mul(out_transform, &chain_node->transform);
	}
	s_array_clear(&chain);
	return 1;
}

static b8 se_sdf_scene_subtree_has_local_content_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle scene,
	const se_sdf_node_handle node
) {
	if (!scene_ptr || node == SE_SDF_NODE_NULL) {
		return 1;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 1;
	}
	if (se_sdf_node_is_primitive(node_ptr)) {
		return 1;
	}
	for (sz i = 0; i < s_array_get_size(&node_ptr->children); ++i) {
		se_sdf_node_handle* child = s_array_get(&node_ptr->children, s_array_handle(&node_ptr->children, (u32)i));
		if (child && se_sdf_scene_subtree_has_local_content_recursive(scene_ptr, scene, *child)) {
			return 1;
		}
	}
	return 0;
}

static b8 se_sdf_scene_update_prepared_ref_subtree_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle scene,
	const se_sdf_node_handle node,
	const s_mat4* world_transform,
	b8* out_updated_any
) {
	if (!scene_ptr || node == SE_SDF_NODE_NULL || !world_transform) {
		return 0;
	}
	se_sdf_node* node_ptr = se_sdf_node_from_handle(scene_ptr, scene, node);
	if (!node_ptr) {
		return 0;
	}
	if (se_sdf_node_is_ref(node_ptr)) {
		se_sdf_scene* child_ptr = se_sdf_scene_from_handle(node_ptr->ref_source);
		if (!child_ptr || !child_ptr->prepared.ready) {
			return 0;
		}
		for (sz i = 0; i < s_array_get_size(&scene_ptr->prepared.refs); ++i) {
			se_sdf_prepared_ref* ref = s_array_get(&scene_ptr->prepared.refs, s_array_handle(&scene_ptr->prepared.refs, (u32)i));
			if (!ref || ref->node != node) {
				continue;
			}
			se_sdf_prepared_ref_apply_bounds(ref, world_transform, &child_ptr->prepared.bounds);
			if (out_updated_any) {
				*out_updated_any = 1;
			}
			break;
		}
	}
	for (sz i = 0; i < s_array_get_size(&node_ptr->children); ++i) {
		se_sdf_node_handle* child = s_array_get(&node_ptr->children, s_array_handle(&node_ptr->children, (u32)i));
		se_sdf_node* child_ptr = child ? se_sdf_node_from_handle(scene_ptr, scene, *child) : NULL;
		if (!child || !child_ptr) {
			continue;
		}
		const s_mat4 child_world_transform = s_mat4_mul(world_transform, &child_ptr->transform);
		if (!se_sdf_scene_update_prepared_ref_subtree_recursive(scene_ptr, scene, *child, &child_world_transform, out_updated_any)) {
			return 0;
		}
	}
	return 1;
}

static b8 se_sdf_scene_update_prepared_ref_subtree(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle scene,
	const se_sdf_node_handle node
) {
	if (!scene_ptr || node == SE_SDF_NODE_NULL || !scene_ptr->prepared.ready || !scene_ptr->prepared.instancing_supported) {
		return 0;
	}
	if (se_sdf_scene_subtree_has_local_content_recursive(scene_ptr, scene, node)) {
		return 0;
	}
	s_mat4 world_transform = s_mat4_identity;
	b8 updated_any = 0;
	if (!se_sdf_node_calculate_world_transform(scene_ptr, scene, node, &world_transform)) {
		return 0;
	}
	if (!se_sdf_scene_update_prepared_ref_subtree_recursive(scene_ptr, scene, node, &world_transform, &updated_any) || !updated_any) {
		return 0;
	}
	se_sdf_prepared_recalculate_bounds(&scene_ptr->prepared);
	se_sdf_prepare_build_grid(&scene_ptr->prepared);
	se_sdf_scene_commit_runtime_update(scene_ptr);
	return 1;
}

static b8 se_sdf_prepare_build_cache(
	const se_sdf_handle sdf,
	se_sdf_scene* scene_ptr,
	const se_sdf_prepare_desc* prepare_desc,
	const b8 force_full_lods,
	se_sdf_prepared_cache* out_cache,
	const b8 build_gpu_resources
) {
	if (!scene_ptr || sdf == SE_SDF_NULL || scene_ptr->root == SE_SDF_NODE_NULL || !prepare_desc || !out_cache) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	se_sdf_bounds bounds = SE_SDF_BOUNDS_DEFAULTS;
	if (!se_sdf_calculate_bounds(sdf, &bounds) || !bounds.valid) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}
	bounds = se_sdf_prepare_expand_bounds(bounds, prepare_desc);

	se_sdf_prepared_cache_clear(out_cache);
	out_cache->desc = *prepare_desc;
	out_cache->bounds = bounds;
	out_cache->generation = scene_ptr->generation;
	out_cache->dependency_stamp = se_sdf_prepare_dependency_stamp_recursive(
		sdf,
		0u,
		s_array_get_size(&scene_ptr->nodes) + 256u
	);
	out_cache->instancing_supported = se_sdf_prepare_tree_supports_instancing_recursive(
		scene_ptr,
		sdf,
		scene_ptr->root,
		0u,
		s_array_get_size(&scene_ptr->nodes) + 256u
	);
	if (!se_sdf_prepare_collect_refs_recursive(
			scene_ptr,
			sdf,
			scene_ptr->root,
			&s_mat4_identity,
			out_cache,
			0u,
			s_array_get_size(&scene_ptr->nodes) + 256u)) {
		se_sdf_prepared_cache_clear(out_cache);
		return 0;
	}
	se_sdf_prepare_build_grid(out_cache);

	const b8 can_split_local_and_refs =
		!force_full_lods &&
		out_cache->instancing_supported &&
		out_cache->has_local_content &&
		s_array_get_size(&out_cache->refs) > 0u;
	if (can_split_local_and_refs) {
		se_sdf_bounds local_bounds = SE_SDF_BOUNDS_DEFAULTS;
		if (se_sdf_calculate_bounds_filtered_internal(sdf, &local_bounds, SE_SDF_QUERY_INCLUDE_LOCAL)) {
			out_cache->local_bounds = se_sdf_prepare_expand_bounds(local_bounds, prepare_desc);
			for (u32 lod_index = 0u; lod_index < prepare_desc->lod_count; ++lod_index) {
				const u32 resolution = prepare_desc->lod_resolutions[lod_index];
				if (!se_sdf_prepare_volume_lod(
						sdf,
						&out_cache->local_bounds,
						resolution,
						prepare_desc->brick_size,
						prepare_desc->brick_border,
						&out_cache->local_lods[lod_index],
						SE_SDF_QUERY_INCLUDE_LOCAL,
						build_gpu_resources)) {
					se_sdf_prepared_cache_clear(out_cache);
					return 0;
				}
			}
			out_cache->local_lods_valid = 1;
		}
	}

	const b8 needs_full_lods = force_full_lods ||
		!out_cache->instancing_supported ||
		s_array_get_size(&out_cache->refs) == 0u ||
		(out_cache->has_local_content && s_array_get_size(&out_cache->refs) > 0u && !out_cache->local_lods_valid);
	if (needs_full_lods) {
		for (u32 lod_index = 0u; lod_index < prepare_desc->lod_count; ++lod_index) {
			const u32 resolution = prepare_desc->lod_resolutions[lod_index];
			if (!se_sdf_prepare_volume_lod(
					sdf,
					&bounds,
					resolution,
					prepare_desc->brick_size,
					prepare_desc->brick_border,
					&out_cache->full_lods[lod_index],
					SE_SDF_QUERY_INCLUDE_ALL,
					build_gpu_resources)) {
				se_sdf_prepared_cache_clear(out_cache);
				return 0;
			}
		}
		out_cache->full_lods_valid = 1;
	}

	{
		se_sdf_prepared_cache previous_prepared = scene_ptr->prepared;
		scene_ptr->prepared = *out_cache;
		se_sdf_prepare_calculate_colors(scene_ptr, sdf);
		*out_cache = scene_ptr->prepared;
		scene_ptr->prepared = previous_prepared;
	}

	out_cache->ready = 1;
	se_set_last_error(SE_RESULT_OK);
	return 1;
}

static void* se_sdf_prepare_async_task(void* user_data) {
	se_sdf_async_prepare_result* result = (se_sdf_async_prepare_result*)user_data;
	if (!result || !result->context || result->worker_scene == SE_SDF_NULL) {
		if (result) {
			result->result = SE_RESULT_INVALID_ARGUMENT;
		}
		return user_data;
	}

	se_context* previous = se_push_tls_context(result->context);
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(result->worker_scene);
	if (!scene_ptr) {
		result->result = SE_RESULT_NOT_FOUND;
	} else if (se_sdf_prepare_build_cache(
			result->worker_scene,
			scene_ptr,
			&result->desc,
			result->force_full_lods,
			&result->prepared,
			0)) {
		result->result = SE_RESULT_OK;
	} else {
		result->result = se_get_last_error();
	}
	se_pop_tls_context(previous);
	return user_data;
}

static b8 se_sdf_prepare_poll_internal(const se_sdf_handle sdf, se_sdf_prepare_status* out_status, const b8 set_error) {
	if (out_status) {
		memset(out_status, 0, sizeof(*out_status));
		out_status->result = SE_RESULT_OK;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr) {
		if (set_error) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		}
		return 0;
	}
	if (out_status) {
		out_status->ready = scene_ptr->prepared.ready;
	}
	if (!scene_ptr->async_prepare.active) {
		if (set_error) {
			se_set_last_error(SE_RESULT_OK);
		}
		return 1;
	}

	se_worker_pool* pool = se_sdf_prepare_pool_get();
	if (!pool) {
		if (out_status) {
			out_status->failed = 1;
			out_status->result = SE_RESULT_NOT_FOUND;
		}
		if (set_error) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
		}
		return 0;
	}

	b8 done = 0;
	se_sdf_async_prepare_result* result = NULL;
	if (!se_worker_poll(pool, scene_ptr->async_prepare.task, &done, (void**)&result)) {
		if (out_status) {
			out_status->failed = 1;
			out_status->result = se_get_last_error();
		}
		return 0;
	}
	if (!done) {
		if (out_status) {
			out_status->pending = 1;
		}
		if (set_error) {
			se_set_last_error(SE_RESULT_OK);
		}
		return 1;
	}

	(void)se_worker_release(pool, scene_ptr->async_prepare.task);
	scene_ptr->async_prepare.task = SE_WORKER_TASK_NULL;
	scene_ptr->async_prepare.active = 0;
	scene_ptr->async_prepare.worker_context = NULL;
	scene_ptr->async_prepare.worker_scene = SE_SDF_NULL;

	b8 ok = 1;
	se_result result_code = result ? result->result : SE_RESULT_NOT_FOUND;
	if (result && result->result == SE_RESULT_OK &&
		scene_ptr->generation == result->requested_generation) {
		if (!se_sdf_prepared_cache_finalize_gpu(&result->prepared)) {
			result_code = se_get_last_error();
			ok = 0;
		} else {
			se_sdf_prepared_cache_clear(&scene_ptr->prepared);
			scene_ptr->prepared = result->prepared;
			memset(&result->prepared, 0, sizeof(result->prepared));
			scene_ptr->prepared.generation = scene_ptr->generation;
			scene_ptr->prepared.ready = 1;
			if (out_status) {
				out_status->applied = 1;
				out_status->ready = 1;
			}
		}
	} else if (result && result->result != SE_RESULT_OK) {
		ok = 0;
	}

	if (out_status) {
		out_status->failed = !ok;
		out_status->result = result_code;
		if (!out_status->applied) {
			out_status->ready = scene_ptr->prepared.ready;
		}
	}
	se_sdf_prepare_async_cleanup_result(result);
	if (set_error) {
		se_set_last_error(ok ? SE_RESULT_OK : result_code);
	}
	return ok;
}

static b8 se_sdf_prepare_internal(
	const se_sdf_handle sdf,
	const se_sdf_prepare_desc* desc,
	const b8 force_full_lods
) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr || scene_ptr->root == SE_SDF_NODE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}

	se_sdf_prepare_desc prepare_desc = se_sdf_prepare_desc_resolve(desc);

	char validation_error[256] = {0};
	if (!se_sdf_validate(sdf, validation_error, sizeof(validation_error))) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	const u64 dependency_stamp = se_sdf_prepare_dependency_stamp_recursive(
		sdf,
		0u,
		s_array_get_size(&scene_ptr->nodes) + 256u
	);
	if (!prepare_desc.force_rebuild &&
		scene_ptr->prepared.ready &&
		scene_ptr->prepared.generation == scene_ptr->generation &&
		scene_ptr->prepared.dependency_stamp == dependency_stamp &&
		se_sdf_prepare_desc_equals(&scene_ptr->prepared.desc, &prepare_desc)) {
		se_set_last_error(SE_RESULT_OK);
		return 1;
	}

	se_debug_trace_begin("sdf_prepare");
	for (sz i = 0; i < s_array_get_size(&scene_ptr->nodes); ++i) {
		se_sdf_node* node = s_array_get(&scene_ptr->nodes, s_array_handle(&scene_ptr->nodes, (u32)i));
		if (node && se_sdf_node_is_ref(node)) {
			if (!se_sdf_prepare_internal(node->ref_source, &prepare_desc, 1)) {
				se_debug_trace_end("sdf_prepare");
				return 0;
			}
		}
	}

	se_sdf_prepared_cache prepared_cache = {0};
	if (!se_sdf_prepare_build_cache(
			sdf,
			scene_ptr,
			&prepare_desc,
			force_full_lods,
			&prepared_cache,
			1)) {
		se_sdf_prepared_cache_clear(&prepared_cache);
		se_debug_trace_end("sdf_prepare");
		return 0;
	}
	se_sdf_prepared_cache_clear(&scene_ptr->prepared);
	scene_ptr->prepared = prepared_cache;
	se_debug_trace_end("sdf_prepare");
	se_set_last_error(SE_RESULT_OK);
	return 1;
}

b8 se_sdf_prepare(const se_sdf_handle sdf, const se_sdf_prepare_desc* desc) {
	return se_sdf_prepare_internal(sdf, desc, 0);
}

b8 se_sdf_prepare_async(const se_sdf_handle sdf, const se_sdf_prepare_desc* desc) {
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr || scene_ptr->root == SE_SDF_NODE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	if (!se_sdf_prepare_poll_internal(sdf, NULL, 0)) {
		se_set_last_error(se_get_last_error());
		return 0;
	}
	if (scene_ptr->async_prepare.active) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return 0;
	}
	if (se_sdf_scene_has_ref_nodes(scene_ptr)) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return 0;
	}

	se_context* worker_context = se_sdf_prepare_context_create();
	if (!worker_context) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return 0;
	}

	se_sdf_prepare_desc prepare_desc = se_sdf_prepare_desc_resolve(desc);
	se_sdf_desc clone_desc = SE_SDF_DESC_DEFAULTS;
	clone_desc.initial_node_capacity = s_array_get_size(&scene_ptr->nodes);
	se_context* previous_context = se_push_tls_context(worker_context);
	const se_sdf_handle worker_scene = se_sdf_create(&clone_desc);
	if (worker_scene == SE_SDF_NULL) {
		se_pop_tls_context(previous_context);
		se_sdf_prepare_context_destroy(worker_context, SE_SDF_NULL);
		return 0;
	}
	if (!se_sdf_clone_scene_graph(scene_ptr, sdf, worker_scene)) {
		se_pop_tls_context(previous_context);
		se_sdf_prepare_context_destroy(worker_context, worker_scene);
		return 0;
	}
	se_pop_tls_context(previous_context);

	se_sdf_async_prepare_result* result = calloc(1u, sizeof(*result));
	if (!result) {
		se_sdf_prepare_context_destroy(worker_context, worker_scene);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return 0;
	}
	result->context = worker_context;
	result->source_scene = sdf;
	result->worker_scene = worker_scene;
	result->desc = prepare_desc;
	result->force_full_lods = 0;
	result->requested_generation = scene_ptr->generation;
	result->result = SE_RESULT_OK;

	se_worker_pool* pool = se_sdf_prepare_pool_get();
	if (!pool) {
		se_sdf_prepare_async_cleanup_result(result);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}
	scene_ptr->async_prepare.task = se_worker_submit(pool, se_sdf_prepare_async_task, result);
	if (scene_ptr->async_prepare.task == SE_WORKER_TASK_NULL) {
		se_sdf_prepare_async_cleanup_result(result);
		return 0;
	}
	scene_ptr->async_prepare.worker_context = worker_context;
	scene_ptr->async_prepare.worker_scene = worker_scene;
	scene_ptr->async_prepare.active = 1;
	se_set_last_error(SE_RESULT_OK);
	return 1;
}

b8 se_sdf_prepare_poll(const se_sdf_handle sdf, se_sdf_prepare_status* out_status) {
	return se_sdf_prepare_poll_internal(sdf, out_status, 1);
}

b8 se_sdf_bake(const se_sdf_handle sdf, const se_sdf_bake_desc* desc, se_sdf_bake_result* out_result) {
	se_sdf_prepare_desc prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	if (desc) {
		prepare_desc = desc->prepare;
	}
	if (!se_sdf_prepare_internal(sdf, &prepare_desc, 1)) {
		return 0;
	}
	if (!out_result) {
		se_set_last_error(SE_RESULT_OK);
		return 1;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(sdf);
	if (!scene_ptr || !scene_ptr->prepared.ready) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}
	memset(out_result, 0, sizeof(*out_result));
	out_result->bounds = scene_ptr->prepared.bounds;
	out_result->lod_count = prepare_desc.lod_count;
	for (u32 i = 0u; i < prepare_desc.lod_count; ++i) {
		out_result->lods[i].resolution = scene_ptr->prepared.full_lods[i].resolution;
		out_result->lods[i].texture = scene_ptr->prepared.full_lods[i].texture;
		out_result->lods[i].voxel_size = scene_ptr->prepared.full_lods[i].voxel_size;
		out_result->lods[i].max_distance = scene_ptr->prepared.full_lods[i].max_distance;
		out_result->lods[i].valid = scene_ptr->prepared.full_lods[i].valid;
	}
	se_set_last_error(SE_RESULT_OK);
	return 1;
}

b8 se_sdf_align_camera(
	const se_sdf_handle scene,
	const se_camera_handle camera,
	const se_sdf_camera_align_desc* desc,
	se_sdf_bounds* out_bounds
) {
	if (camera == S_HANDLE_NULL) {
		return 0;
	}
	se_camera* camera_ptr = se_camera_get(camera);
	if (!camera_ptr) {
		return 0;
	}

	se_sdf_bounds bounds = SE_SDF_BOUNDS_DEFAULTS;
	if (!se_sdf_calculate_bounds(scene, &bounds)) {
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

static void se_sdf_physics_finalize_bounds(se_sdf_bounds* bounds) {
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

struct se_sdf_query_distance_result {
	f32 distance;
	se_sdf_handle sdf;
	se_sdf_node_handle node;
	s_vec3 color;
	b8 has_color;
};

static f32 se_sdf_query_distance_to_bounds(
	const s_vec3* world_point,
	const se_sdf_bounds* bounds
) {
	if (!world_point || !bounds || !bounds->valid) {
		return 1e9f;
	}
	const f32 dx = fmaxf(fmaxf(bounds->min.x - world_point->x, 0.0f), world_point->x - bounds->max.x);
	const f32 dy = fmaxf(fmaxf(bounds->min.y - world_point->y, 0.0f), world_point->y - bounds->max.y);
	const f32 dz = fmaxf(fmaxf(bounds->min.z - world_point->z, 0.0f), world_point->z - bounds->max.z);
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

static f32 se_sdf_query_distance_to_grid_cell(
	const s_vec3* world_point,
	const se_sdf_prepared_cache* cache,
	const i32 cell_x,
	const i32 cell_y,
	const i32 cell_z
) {
	if (!world_point || !cache || cache->cell_size <= 0.0f) {
		return 1e9f;
	}
	const s_vec3 cell_min = s_vec3(
		cache->grid_min.x + (f32)cell_x * cache->cell_size,
		cache->grid_min.y + (f32)cell_y * cache->cell_size,
		cache->grid_min.z + (f32)cell_z * cache->cell_size
	);
	const s_vec3 cell_max = s_vec3(
		cell_min.x + cache->cell_size,
		cell_min.y + cache->cell_size,
		cell_min.z + cache->cell_size
	);
	const se_sdf_bounds cell_bounds = {
		.min = cell_min,
		.max = cell_max,
		.center = s_vec3(
			(cell_min.x + cell_max.x) * 0.5f,
			(cell_min.y + cell_max.y) * 0.5f,
			(cell_min.z + cell_max.z) * 0.5f
		),
		.radius = 0.0f,
		.valid = 1,
		.has_unbounded_primitives = 0
	};
	return se_sdf_query_distance_to_bounds(world_point, &cell_bounds);
}

static void se_sdf_query_process_prepared_ref(
	const se_sdf_scene* root_scene_ptr,
	const s_vec3* world_point,
	const u32 ref_index,
	u8* visited,
	se_sdf_query_distance_result* result
) {
	if (!root_scene_ptr || !world_point || !visited || !result || ref_index >= s_array_get_size((se_sdf_prepared_refs*)&root_scene_ptr->prepared.refs)) {
		return;
	}
	if (visited[ref_index]) {
		return;
	}
	visited[ref_index] = 1u;
	se_sdf_prepared_ref* ref = s_array_get((se_sdf_prepared_refs*)&root_scene_ptr->prepared.refs, s_array_handle((se_sdf_prepared_refs*)&root_scene_ptr->prepared.refs, ref_index));
	if (!ref || !ref->bounds.valid) {
		return;
	}
	if (result->distance < 1e8f) {
		const f32 bounds_distance = se_sdf_query_distance_to_bounds(world_point, &ref->bounds);
		if (bounds_distance > result->distance) {
			return;
		}
	}
	se_sdf_scene* child_ptr = se_sdf_scene_from_handle(ref->source);
	if (!child_ptr || child_ptr->root == SE_SDF_NODE_NULL || !child_ptr->prepared.ready) {
		return;
	}
	const s_vec3 local_point = se_sdf_mul_mat4_point(&ref->inverse_transform, world_point);
	const se_sdf_query_distance_result child_result = se_sdf_query_distance_prepared_volume(
		&child_ptr->prepared,
		child_ptr->prepared.full_lods,
		&child_ptr->prepared.bounds,
		ref->source,
		child_ptr->root,
		&local_point
	);
	if (child_result.distance < result->distance) {
		*result = child_result;
	}
}

static f32 se_sdf_query_eval_runtime_primitive_distance(
	const se_sdf_node* node,
	const s_vec3* local_point
) {
	if (!se_sdf_node_is_primitive(node) || !local_point) {
		return 1e9f;
	}
	se_sdf_object object = {0};
	object.type = (se_sdf_object_type)node->primitive.type;
	if (!se_sdf_copy_primitive_to_legacy_object(&node->primitive, &object)) {
		return 1e9f;
	}
	return se_sdf_physics_eval_primitive_distance_local(&object, local_point);
}

static const se_sdf_prepared_lod* se_sdf_query_find_prepared_lod(
	const se_sdf_prepared_cache* cache,
	const se_sdf_prepared_lod* lods
) {
	if (!cache || !lods) {
		return NULL;
	}
	const u32 lod_count = cache->desc.lod_count > 0u ? cache->desc.lod_count : 1u;
	for (u32 i = 0u; i < lod_count && i < 4u; ++i) {
		if (lods[i].valid && lods[i].cpu_distances && lods[i].cpu_distance_count > 0u) {
			return &lods[i];
		}
	}
	return NULL;
}

static f32 se_sdf_query_sample_prepared_lod_distance(
	const se_sdf_prepared_lod* lod,
	const se_sdf_bounds* bounds,
	const s_vec3* point
) {
	if (!lod || !bounds || !bounds->valid || !point || lod->resolution == 0u ||
		!lod->cpu_distances || lod->cpu_distance_count == 0u) {
		return 1e9f;
	}
	const s_vec3 span = s_vec3(
		fmaxf(bounds->max.x - bounds->min.x, 0.0001f),
		fmaxf(bounds->max.y - bounds->min.y, 0.0001f),
		fmaxf(bounds->max.z - bounds->min.z, 0.0001f)
	);
	const f32 bounds_distance = se_sdf_query_distance_to_bounds(point, bounds);
	const f32 dominant_voxel = fmaxf(lod->voxel_size.x, fmaxf(lod->voxel_size.y, lod->voxel_size.z));
	if (bounds_distance > lod->max_distance + dominant_voxel) {
		return bounds_distance;
	}
	const f32 uv_x = s_max(0.0f, s_min(1.0f, (point->x - bounds->min.x) / span.x));
	const f32 uv_y = s_max(0.0f, s_min(1.0f, (point->y - bounds->min.y) / span.y));
	const f32 uv_z = s_max(0.0f, s_min(1.0f, (point->z - bounds->min.z) / span.z));
	const f32 resolution_f = (f32)lod->resolution;
	const f32 sample_x = s_max(0.0f, s_min(resolution_f - 1.0f, uv_x * resolution_f - 0.5f));
	const f32 sample_y = s_max(0.0f, s_min(resolution_f - 1.0f, uv_y * resolution_f - 0.5f));
	const f32 sample_z = s_max(0.0f, s_min(resolution_f - 1.0f, uv_z * resolution_f - 0.5f));
	const u32 x0 = (u32)floorf(sample_x);
	const u32 y0 = (u32)floorf(sample_y);
	const u32 z0 = (u32)floorf(sample_z);
	const u32 x1 = s_min(lod->resolution - 1u, x0 + 1u);
	const u32 y1 = s_min(lod->resolution - 1u, y0 + 1u);
	const u32 z1 = s_min(lod->resolution - 1u, z0 + 1u);
	const f32 fx = sample_x - (f32)x0;
	const f32 fy = sample_y - (f32)y0;
	const f32 fz = sample_z - (f32)z0;
	#define SE_SDF_SAMPLE_VOXEL(ix, iy, iz) lod->cpu_distances[(sz)(ix) + (sz)lod->resolution * ((sz)(iy) + (sz)lod->resolution * (sz)(iz))]
	const f32 c000 = SE_SDF_SAMPLE_VOXEL(x0, y0, z0);
	const f32 c100 = SE_SDF_SAMPLE_VOXEL(x1, y0, z0);
	const f32 c010 = SE_SDF_SAMPLE_VOXEL(x0, y1, z0);
	const f32 c110 = SE_SDF_SAMPLE_VOXEL(x1, y1, z0);
	const f32 c001 = SE_SDF_SAMPLE_VOXEL(x0, y0, z1);
	const f32 c101 = SE_SDF_SAMPLE_VOXEL(x1, y0, z1);
	const f32 c011 = SE_SDF_SAMPLE_VOXEL(x0, y1, z1);
	const f32 c111 = SE_SDF_SAMPLE_VOXEL(x1, y1, z1);
	#undef SE_SDF_SAMPLE_VOXEL
	const f32 c00 = c000 + (c100 - c000) * fx;
	const f32 c10 = c010 + (c110 - c010) * fx;
	const f32 c01 = c001 + (c101 - c001) * fx;
	const f32 c11 = c011 + (c111 - c011) * fx;
	const f32 c0 = c00 + (c10 - c00) * fy;
	const f32 c1 = c01 + (c11 - c01) * fy;
	const f32 sampled_distance = c0 + (c1 - c0) * fz;
	if (bounds_distance > 0.0f) {
		return fmaxf(sampled_distance, bounds_distance);
	}
	return sampled_distance;
}

static f32 se_sdf_query_trilerp(
	const f32 c000,
	const f32 c100,
	const f32 c010,
	const f32 c110,
	const f32 c001,
	const f32 c101,
	const f32 c011,
	const f32 c111,
	const f32 fx,
	const f32 fy,
	const f32 fz
) {
	const f32 c00 = c000 + (c100 - c000) * fx;
	const f32 c10 = c010 + (c110 - c010) * fx;
	const f32 c01 = c001 + (c101 - c001) * fx;
	const f32 c11 = c011 + (c111 - c011) * fx;
	const f32 c0 = c00 + (c10 - c00) * fy;
	const f32 c1 = c01 + (c11 - c01) * fy;
	return c0 + (c1 - c0) * fz;
}

static b8 se_sdf_query_sample_prepared_lod_color(
	const se_sdf_prepared_lod* lod,
	const se_sdf_bounds* bounds,
	const s_vec3* point,
	s_vec3* out_color
) {
	if (!lod || !bounds || !bounds->valid || !point || !out_color || lod->resolution == 0u ||
		!lod->cpu_colors || lod->cpu_color_count < (sz)lod->resolution * (sz)lod->resolution * (sz)lod->resolution * 4u) {
		return 0;
	}
	const s_vec3 span = s_vec3(
		fmaxf(bounds->max.x - bounds->min.x, 0.0001f),
		fmaxf(bounds->max.y - bounds->min.y, 0.0001f),
		fmaxf(bounds->max.z - bounds->min.z, 0.0001f)
	);
	const f32 uv_x = s_max(0.0f, s_min(1.0f, (point->x - bounds->min.x) / span.x));
	const f32 uv_y = s_max(0.0f, s_min(1.0f, (point->y - bounds->min.y) / span.y));
	const f32 uv_z = s_max(0.0f, s_min(1.0f, (point->z - bounds->min.z) / span.z));
	const f32 resolution_f = (f32)lod->resolution;
	const f32 sample_x = s_max(0.0f, s_min(resolution_f - 1.0f, uv_x * resolution_f - 0.5f));
	const f32 sample_y = s_max(0.0f, s_min(resolution_f - 1.0f, uv_y * resolution_f - 0.5f));
	const f32 sample_z = s_max(0.0f, s_min(resolution_f - 1.0f, uv_z * resolution_f - 0.5f));
	const u32 x0 = (u32)floorf(sample_x);
	const u32 y0 = (u32)floorf(sample_y);
	const u32 z0 = (u32)floorf(sample_z);
	const u32 x1 = s_min(lod->resolution - 1u, x0 + 1u);
	const u32 y1 = s_min(lod->resolution - 1u, y0 + 1u);
	const u32 z1 = s_min(lod->resolution - 1u, z0 + 1u);
	const f32 fx = sample_x - (f32)x0;
	const f32 fy = sample_y - (f32)y0;
	const f32 fz = sample_z - (f32)z0;
	#define SE_SDF_SAMPLE_COLOR_COMPONENT(ix, iy, iz, channel) ((f32)lod->cpu_colors[((sz)(ix) + (sz)lod->resolution * ((sz)(iy) + (sz)lod->resolution * (sz)(iz))) * 4u + (channel)] / 255.0f)
	const f32 c000r = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y0, z0, 0u);
	const f32 c100r = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y0, z0, 0u);
	const f32 c010r = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y1, z0, 0u);
	const f32 c110r = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y1, z0, 0u);
	const f32 c001r = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y0, z1, 0u);
	const f32 c101r = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y0, z1, 0u);
	const f32 c011r = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y1, z1, 0u);
	const f32 c111r = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y1, z1, 0u);
	const f32 c000g = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y0, z0, 1u);
	const f32 c100g = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y0, z0, 1u);
	const f32 c010g = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y1, z0, 1u);
	const f32 c110g = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y1, z0, 1u);
	const f32 c001g = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y0, z1, 1u);
	const f32 c101g = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y0, z1, 1u);
	const f32 c011g = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y1, z1, 1u);
	const f32 c111g = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y1, z1, 1u);
	const f32 c000b = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y0, z0, 2u);
	const f32 c100b = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y0, z0, 2u);
	const f32 c010b = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y1, z0, 2u);
	const f32 c110b = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y1, z0, 2u);
	const f32 c001b = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y0, z1, 2u);
	const f32 c101b = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y0, z1, 2u);
	const f32 c011b = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y1, z1, 2u);
	const f32 c111b = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y1, z1, 2u);
	const f32 c000a = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y0, z0, 3u);
	const f32 c100a = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y0, z0, 3u);
	const f32 c010a = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y1, z0, 3u);
	const f32 c110a = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y1, z0, 3u);
	const f32 c001a = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y0, z1, 3u);
	const f32 c101a = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y0, z1, 3u);
	const f32 c011a = SE_SDF_SAMPLE_COLOR_COMPONENT(x0, y1, z1, 3u);
	const f32 c111a = SE_SDF_SAMPLE_COLOR_COMPONENT(x1, y1, z1, 3u);
	#undef SE_SDF_SAMPLE_COLOR_COMPONENT
	const f32 alpha = se_sdf_query_trilerp(c000a, c100a, c010a, c110a, c001a, c101a, c011a, c111a, fx, fy, fz);
	if (alpha <= 0.01f) {
		return 0;
	}
	*out_color = s_vec3(
		se_sdf_query_trilerp(c000r, c100r, c010r, c110r, c001r, c101r, c011r, c111r, fx, fy, fz),
		se_sdf_query_trilerp(c000g, c100g, c010g, c110g, c001g, c101g, c011g, c111g, fx, fy, fz),
		se_sdf_query_trilerp(c000b, c100b, c010b, c110b, c001b, c101b, c011b, c111b, fx, fy, fz)
	);
	return 1;
}

static se_sdf_query_distance_result se_sdf_query_distance_prepared_volume(
	const se_sdf_prepared_cache* cache,
	const se_sdf_prepared_lod* lods,
	const se_sdf_bounds* bounds,
	const se_sdf_handle sdf,
	const se_sdf_node_handle node,
	const s_vec3* point
) {
	se_sdf_query_distance_result result = {
		.distance = 1e9f,
		.sdf = SE_SDF_NULL,
		.node = SE_SDF_NODE_NULL
	};
	const se_sdf_prepared_lod* lod = se_sdf_query_find_prepared_lod(cache, lods);
	if (!lod || !bounds || !bounds->valid || !point) {
		return result;
	}
	result.distance = se_sdf_query_sample_prepared_lod_distance(lod, bounds, point);
	result.sdf = sdf;
	result.node = node;
	result.has_color = se_sdf_query_sample_prepared_lod_color(lod, bounds, point, &result.color);
	return result;
}

static se_sdf_query_distance_result se_sdf_query_distance_prepared_root(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle scene,
	const s_vec3* world_point,
	const u32 include_mask
) {
	se_sdf_query_distance_result result = {
		.distance = 1e9f,
		.sdf = SE_SDF_NULL,
		.node = SE_SDF_NODE_NULL
	};
	if (!scene_ptr || !world_point || !scene_ptr->prepared.ready || scene_ptr->root == SE_SDF_NODE_NULL) {
		return result;
	}
	const b8 has_refs = s_array_get_size(&scene_ptr->prepared.refs) > 0u;
	const b8 wants_local = (include_mask & SE_SDF_QUERY_INCLUDE_LOCAL) != 0u;
	const b8 wants_refs = (include_mask & SE_SDF_QUERY_INCLUDE_REFS) != 0u;
	const b8 can_use_full_volume =
		scene_ptr->prepared.full_lods_valid &&
		((wants_local && wants_refs) ||
		 (wants_local && !wants_refs && !has_refs) ||
		 (!wants_local && wants_refs && !scene_ptr->prepared.has_local_content));

	if (wants_local && scene_ptr->prepared.has_local_content) {
		if (scene_ptr->prepared.local_lods_valid) {
			result = se_sdf_query_distance_prepared_volume(
				&scene_ptr->prepared,
				scene_ptr->prepared.local_lods,
				&scene_ptr->prepared.local_bounds,
				scene,
				scene_ptr->root,
				world_point
			);
		} else if (scene_ptr->prepared.full_lods_valid && s_array_get_size(&scene_ptr->prepared.refs) == 0u) {
			result = se_sdf_query_distance_prepared_volume(
				&scene_ptr->prepared,
				scene_ptr->prepared.full_lods,
				&scene_ptr->prepared.bounds,
				scene,
				scene_ptr->root,
				world_point
			);
		}
	}

	if (!wants_refs || !has_refs) {
		if (result.distance >= 1e8f && can_use_full_volume) {
			result = se_sdf_query_distance_prepared_volume(
				&scene_ptr->prepared,
				scene_ptr->prepared.full_lods,
				&scene_ptr->prepared.bounds,
				scene,
				scene_ptr->root,
				world_point
			);
		}
		return result;
	}
	const sz ref_count = s_array_get_size(&scene_ptr->prepared.refs);
	u8* visited = calloc(ref_count > 0u ? ref_count : 1u, sizeof(*visited));
	if (!visited) {
		for (u32 i = 0u; i < (u32)ref_count; ++i) {
			se_sdf_prepared_ref* ref = s_array_get(&scene_ptr->prepared.refs, s_array_handle(&scene_ptr->prepared.refs, i));
			if (!ref || !ref->bounds.valid) {
				continue;
			}
			if (result.distance < 1e8f) {
				const f32 bounds_distance = se_sdf_query_distance_to_bounds(world_point, &ref->bounds);
				if (bounds_distance > result.distance) {
					continue;
				}
			}
			se_sdf_scene* child_ptr = se_sdf_scene_from_handle(ref->source);
			if (!child_ptr || child_ptr->root == SE_SDF_NODE_NULL || !child_ptr->prepared.ready) {
				continue;
			}
			const s_vec3 local_point = se_sdf_mul_mat4_point(&ref->inverse_transform, world_point);
			const se_sdf_query_distance_result child_result = se_sdf_query_distance_prepared_volume(
				&child_ptr->prepared,
				child_ptr->prepared.full_lods,
				&child_ptr->prepared.bounds,
				ref->source,
				child_ptr->root,
				&local_point
			);
			if (child_result.distance < result.distance) {
				result = child_result;
			}
		}
		return result;
	}
	if (scene_ptr->prepared.grid_nx <= 0 || scene_ptr->prepared.grid_ny <= 0 || scene_ptr->prepared.grid_nz <= 0 || s_array_get_size(&scene_ptr->prepared.grid_cells) == 0u) {
		for (u32 i = 0u; i < (u32)ref_count; ++i) {
			se_sdf_query_process_prepared_ref(scene_ptr, world_point, i, visited, &result);
		}
		free(visited);
		return result;
	}

	const i32 cx = s_max(0, s_min(scene_ptr->prepared.grid_nx - 1, (i32)floorf((world_point->x - scene_ptr->prepared.grid_min.x) / scene_ptr->prepared.cell_size)));
	const i32 cy = s_max(0, s_min(scene_ptr->prepared.grid_ny - 1, (i32)floorf((world_point->y - scene_ptr->prepared.grid_min.y) / scene_ptr->prepared.cell_size)));
	const i32 cz = s_max(0, s_min(scene_ptr->prepared.grid_nz - 1, (i32)floorf((world_point->z - scene_ptr->prepared.grid_min.z) / scene_ptr->prepared.cell_size)));
	const i32 max_shell = s_max(scene_ptr->prepared.grid_nx, s_max(scene_ptr->prepared.grid_ny, scene_ptr->prepared.grid_nz));
	const f32 cell_diagonal = scene_ptr->prepared.cell_size * 1.7320508f;
	for (i32 shell = 0; shell < max_shell; ++shell) {
		const i32 min_x = s_max(0, cx - shell);
		const i32 min_y = s_max(0, cy - shell);
		const i32 min_z = s_max(0, cz - shell);
		const i32 max_x = s_min(scene_ptr->prepared.grid_nx - 1, cx + shell);
		const i32 max_y = s_min(scene_ptr->prepared.grid_ny - 1, cy + shell);
		const i32 max_z = s_min(scene_ptr->prepared.grid_nz - 1, cz + shell);
		for (i32 z = min_z; z <= max_z; ++z) {
			for (i32 y = min_y; y <= max_y; ++y) {
				for (i32 x = min_x; x <= max_x; ++x) {
					if (shell > 0 && x != min_x && x != max_x && y != min_y && y != max_y && z != min_z && z != max_z) {
						continue;
					}
					if (result.distance < 1e8f) {
						const f32 cell_distance = se_sdf_query_distance_to_grid_cell(world_point, &scene_ptr->prepared, x, y, z);
						if (cell_distance > result.distance + cell_diagonal) {
							continue;
						}
					}
					const sz cell_index = (sz)x + (sz)scene_ptr->prepared.grid_nx * ((sz)y + (sz)scene_ptr->prepared.grid_ny * (sz)z);
					se_sdf_grid_cell* cell = s_array_get(&scene_ptr->prepared.grid_cells, s_array_handle(&scene_ptr->prepared.grid_cells, (u32)cell_index));
					if (!cell) {
						continue;
					}
					for (sz i = 0; i < s_array_get_size(&cell->ref_indices); ++i) {
						u32* ref_index_ptr = s_array_get(&cell->ref_indices, s_array_handle(&cell->ref_indices, (u32)i));
						if (!ref_index_ptr) {
							continue;
						}
						se_sdf_query_process_prepared_ref(scene_ptr, world_point, *ref_index_ptr, visited, &result);
					}
				}
			}
		}
		if (result.distance < 1e8f && ((f32)(shell + 1) * scene_ptr->prepared.cell_size - cell_diagonal) > result.distance) {
			break;
		}
	}
	if (result.distance >= 1e8f) {
		for (u32 i = 0u; i < (u32)ref_count; ++i) {
			if (!visited[i]) {
				se_sdf_query_process_prepared_ref(scene_ptr, world_point, i, visited, &result);
			}
		}
	}
	free(visited);
	return result;
}

static se_sdf_query_distance_result se_sdf_query_distance_runtime_recursive(
	se_sdf_scene* scene_ptr,
	const se_sdf_handle scene,
	const se_sdf_node_handle node_handle,
	const s_vec3* world_point,
	const s_mat4* parent_transform,
	const sz depth,
	const sz max_depth,
	const u32 include_mask
) {
	se_sdf_query_distance_result result = {
		.distance = 1e9f,
		.sdf = SE_SDF_NULL,
		.node = SE_SDF_NODE_NULL
	};
	if (!scene_ptr || !world_point || !parent_transform || node_handle == SE_SDF_NODE_NULL || depth > max_depth || include_mask == 0u) {
		return result;
	}

	se_sdf_node* node = se_sdf_node_from_handle(scene_ptr, scene, node_handle);
	if (!node) {
		return result;
	}

	const s_mat4 world_transform = s_mat4_mul(parent_transform, &node->transform);
	const s_mat4 inverse_transform = s_mat4_inverse(&world_transform);
	const s_vec3 local_point = se_sdf_mul_mat4_point(&inverse_transform, world_point);
	if (!se_sdf_physics_vec3_is_finite(&local_point)) {
		return result;
	}

	if (se_sdf_node_is_ref(node)) {
		if ((include_mask & SE_SDF_QUERY_INCLUDE_REFS) == 0u) {
			return result;
		}
		se_sdf_scene* ref_scene_ptr = se_sdf_scene_from_handle(node->ref_source);
		if (!ref_scene_ptr || ref_scene_ptr->root == SE_SDF_NODE_NULL) {
			return result;
		}
		return se_sdf_query_distance_runtime_recursive(
			ref_scene_ptr,
			node->ref_source,
			ref_scene_ptr->root,
			world_point,
			&world_transform,
			depth + 1u,
			max_depth,
			include_mask
		);
	}

	const sz child_count = s_array_get_size(&node->children);
	if (child_count > 0u) {
		se_sdf_node_handle* first_child = s_array_get(&node->children, s_array_handle(&node->children, 0u));
		if (!first_child) {
			return result;
		}
		result = se_sdf_query_distance_runtime_recursive(
			scene_ptr,
			scene,
			*first_child,
			world_point,
			&world_transform,
			depth + 1u,
			max_depth,
			include_mask
		);
		for (sz i = 1; i < child_count; ++i) {
			se_sdf_node_handle* child_handle = s_array_get(&node->children, s_array_handle(&node->children, (u32)i));
			if (!child_handle) {
				continue;
			}
			const se_sdf_query_distance_result rhs = se_sdf_query_distance_runtime_recursive(
				scene_ptr,
				scene,
				*child_handle,
				world_point,
				&world_transform,
				depth + 1u,
				max_depth,
				include_mask
			);
			switch (node->operation) {
				case SE_SDF_OP_UNION:
				case SE_SDF_OP_SMOOTH_UNION:
					if (rhs.distance < result.distance) {
						result = rhs;
					}
					break;
				case SE_SDF_OP_INTERSECTION:
				case SE_SDF_OP_SMOOTH_INTERSECTION:
					if (rhs.distance > result.distance) {
						result = rhs;
					}
					break;
				case SE_SDF_OP_SUBTRACTION:
				case SE_SDF_OP_SMOOTH_SUBTRACTION: {
					const f32 lhs_distance = result.distance;
					const f32 rhs_distance = rhs.distance;
					const f32 combined = fmaxf(lhs_distance, -rhs_distance);
					if (-rhs_distance > lhs_distance) {
						result = rhs;
					}
					result.distance = combined;
					break;
				}
				case SE_SDF_OP_NONE:
				default:
					break;
			}
		}
		return result;
	}

	if (se_sdf_node_is_primitive(node) && (include_mask & SE_SDF_QUERY_INCLUDE_LOCAL) != 0u) {
		result.distance = se_sdf_query_eval_runtime_primitive_distance(node, &local_point);
		result.distance = se_sdf_noise_apply_distance(&node->noise, &local_point, result.distance);
		result.sdf = scene;
		result.node = node_handle;
		if (node->has_color) {
			result.color = node->color;
			result.has_color = 1;
		}
		if (!isfinite(result.distance)) {
			result.distance = 1e9f;
		}
	}
	return result;
}

static b8 se_sdf_query_sample_distance_internal(
	const se_sdf_handle scene,
	const s_vec3* point,
	f32* out_distance,
	se_sdf_handle* out_hit_sdf,
	se_sdf_node_handle* out_node,
	s_vec3* out_color,
	b8* out_has_color
) {
	return se_sdf_query_sample_distance_filtered_internal(
		scene,
		point,
		out_distance,
		out_hit_sdf,
		out_node,
		out_color,
		out_has_color,
		SE_SDF_QUERY_INCLUDE_ALL
	);
}

static b8 se_sdf_query_sample_distance_filtered_internal(
	const se_sdf_handle scene,
	const s_vec3* point,
	f32* out_distance,
	se_sdf_handle* out_hit_sdf,
	se_sdf_node_handle* out_node,
	s_vec3* out_color,
	b8* out_has_color,
	const u32 include_mask
) {
	if (!point || !out_distance) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr || scene_ptr->root == SE_SDF_NODE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}
	se_debug_trace_begin("sdf_query");
	se_sdf_query_distance_result result = { .distance = 1e9f, .sdf = SE_SDF_NULL, .node = SE_SDF_NODE_NULL };
	if (scene_ptr->prepared.ready &&
		scene_ptr->prepared.generation == scene_ptr->generation) {
		result = se_sdf_query_distance_prepared_root(scene_ptr, scene, point, include_mask);
	} else {
		const sz max_depth = s_array_get_size(&scene_ptr->nodes) + 256u;
		result = se_sdf_query_distance_runtime_recursive(
			scene_ptr,
			scene,
			scene_ptr->root,
			point,
			&s_mat4_identity,
			0u,
			max_depth,
			include_mask
		);
	}
	*out_distance = result.distance;
	if (out_hit_sdf) {
		*out_hit_sdf = result.sdf;
	}
	if (out_node) {
		*out_node = result.node;
	}
	if (out_color) {
		*out_color = result.color;
	}
	if (out_has_color) {
		*out_has_color = result.has_color;
	}
	se_debug_trace_end("sdf_query");
	se_set_last_error(SE_RESULT_OK);
	return 1;
}

static b8 se_sdf_estimate_surface_normal_runtime(
	const se_sdf_handle scene,
	const s_vec3* point,
	s_vec3* out_normal
) {
	if (scene == SE_SDF_NULL || !point || !out_normal) {
		return 0;
	}
	const f32 epsilon = 0.01f;
	const s_vec3 dx = s_vec3(epsilon, 0.0f, 0.0f);
	const s_vec3 dy = s_vec3(0.0f, epsilon, 0.0f);
	const s_vec3 dz = s_vec3(0.0f, 0.0f, epsilon);
	f32 sx_pos = 0.0f;
	f32 sx_neg = 0.0f;
	f32 sy_pos = 0.0f;
	f32 sy_neg = 0.0f;
	f32 sz_pos = 0.0f;
	f32 sz_neg = 0.0f;
	if (!se_sdf_query_sample_distance_internal(scene, &s_vec3_add(point, &dx), &sx_pos, NULL, NULL, NULL, NULL) ||
		!se_sdf_query_sample_distance_internal(scene, &s_vec3_sub(point, &dx), &sx_neg, NULL, NULL, NULL, NULL) ||
		!se_sdf_query_sample_distance_internal(scene, &s_vec3_add(point, &dy), &sy_pos, NULL, NULL, NULL, NULL) ||
		!se_sdf_query_sample_distance_internal(scene, &s_vec3_sub(point, &dy), &sy_neg, NULL, NULL, NULL, NULL) ||
		!se_sdf_query_sample_distance_internal(scene, &s_vec3_add(point, &dz), &sz_pos, NULL, NULL, NULL, NULL) ||
		!se_sdf_query_sample_distance_internal(scene, &s_vec3_sub(point, &dz), &sz_neg, NULL, NULL, NULL, NULL)) {
		return 0;
	}
	s_vec3 normal = s_vec3(sx_pos - sx_neg, sy_pos - sy_neg, sz_pos - sz_neg);
	if (s_vec3_length(&normal) <= 0.000001f) {
		normal = s_vec3(0.0f, 1.0f, 0.0f);
	}
	*out_normal = s_vec3_normalize(&normal);
	return 1;
}

b8 se_sdf_sample_distance(const se_sdf_handle scene, const s_vec3* point, f32* out_distance, se_sdf_node_handle* out_node) {
	return se_sdf_query_sample_distance_internal(scene, point, out_distance, NULL, out_node, NULL, NULL);
}

b8 se_sdf_raycast(const se_sdf_handle scene, const s_vec3* origin, const s_vec3* direction, f32 max_distance, se_sdf_surface_hit* out_hit) {
	if (!origin || !direction || !out_hit || max_distance <= 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	memset(out_hit, 0, sizeof(*out_hit));
	out_hit->sdf = scene;
	out_hit->node = SE_SDF_NODE_NULL;
	const f32 direction_length = s_vec3_length(direction);
	if (direction_length <= 0.000001f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	const s_vec3 ray_direction = s_vec3_normalize(direction);

	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(scene);
	if (!scene_ptr || scene_ptr->root == SE_SDF_NODE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return 0;
	}

	const f32 hit_epsilon = 0.0015f;
	const u32 max_steps = 192u;
	f32 travel = 0.0f;
	for (u32 step = 0u; step < max_steps && travel <= max_distance; ++step) {
		const s_vec3 point = s_vec3_add(origin, &s_vec3_muls(&ray_direction, travel));
		f32 signed_distance = 0.0f;
		se_sdf_handle hit_sdf = SE_SDF_NULL;
		se_sdf_node_handle hit_node = SE_SDF_NODE_NULL;
		if (!se_sdf_query_sample_distance_internal(scene, &point, &signed_distance, &hit_sdf, &hit_node, NULL, NULL)) {
			break;
		}
		if (fabsf(signed_distance) <= hit_epsilon) {
			out_hit->position = point;
			out_hit->distance = travel;
			out_hit->sdf = hit_sdf != SE_SDF_NULL ? hit_sdf : scene;
			out_hit->node = hit_node;
			out_hit->hit = 1;
			if (!se_sdf_estimate_surface_normal_runtime(scene, &point, &out_hit->normal)) {
				out_hit->normal = s_vec3(0.0f, 1.0f, 0.0f);
			}
			se_set_last_error(SE_RESULT_OK);
			return 1;
		}
		const f32 step_distance = fmaxf(fabsf(signed_distance), hit_epsilon * 0.5f);
		if (!isfinite(step_distance)) {
			break;
		}
		travel += step_distance;
	}

	se_set_last_error(SE_RESULT_NOT_FOUND);
	return 0;
}

b8 se_sdf_project_point(const se_sdf_handle scene, const s_vec3* point, const s_vec3* direction, f32 max_distance, se_sdf_surface_hit* out_hit) {
	return se_sdf_raycast(scene, point, direction, max_distance, out_hit);
}

b8 se_sdf_sample_height(const se_sdf_handle scene, f32 x, f32 z, f32 start_y, f32 max_distance, se_sdf_surface_hit* out_hit) {
	if (!out_hit) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return 0;
	}
	const s_vec3 origin = s_vec3(x, start_y, z);
	const s_vec3 direction = s_vec3(0.0f, -1.0f, 0.0f);
	return se_sdf_raycast(scene, &origin, &direction, max_distance, out_hit);
}

se_sdf_renderer_handle se_sdf_renderer_create(const se_sdf_renderer_desc* desc) {
	se_context* ctx = NULL;
	if (!se_sdf_get_context(&ctx)) {
		return SE_SDF_RENDERER_NULL;
	}
	se_sdf_renderer_desc renderer_desc = SE_SDF_RENDERER_DESC_DEFAULTS;
	if (desc) {
		renderer_desc = *desc;
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
	renderer->scene = SE_SDF_NULL;
	s_array_init(&renderer->visible_ref_indices);
	s_array_init(&renderer->instance_models);
	s_array_init(&renderer->instance_inverses);
	s_array_init(&renderer->instance_lod_blends);
	s_array_init(&renderer->resident_lod_entries);
	s_array_init(&renderer->frame_lod_entries);
	renderer->shader = S_HANDLE_NULL;
	renderer->material = SE_SDF_MATERIAL_DESC_DEFAULTS;
	renderer->stylized = SE_SDF_STYLIZED_DESC_DEFAULTS;
	renderer->lighting_direction = s_vec3(0.58f, 0.76f, 0.28f);
	renderer->lighting_color = s_vec3(1.0f, 1.0f, 1.0f);
	renderer->fog_color = s_vec3(0.11f, 0.15f, 0.21f);
	renderer->fog_density = 0.0012f;
	renderer->render_generation = se_render_get_generation();
	renderer->cube_vao = 0u;
	renderer->cube_vbo = 0u;
	renderer->cube_ebo = 0u;
	renderer->cube_model_vbo = 0u;
	renderer->cube_inverse_vbo = 0u;
	renderer->cube_lod_blend_vbo = 0u;
	renderer->instance_capacity = 0u;
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
	s_array_clear(&renderer_ptr->visible_ref_indices);
	s_array_clear(&renderer_ptr->instance_models);
	s_array_clear(&renderer_ptr->instance_inverses);
	s_array_clear(&renderer_ptr->instance_lod_blends);
	s_array_clear(&renderer_ptr->resident_lod_entries);
	s_array_clear(&renderer_ptr->frame_lod_entries);
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

	while (s_array_get_size(&ctx->sdf_physicses) > 0) {
		se_sdf_physics_handle physics_handle = s_array_handle(
			&ctx->sdf_physicses,
			(u32)(s_array_get_size(&ctx->sdf_physicses) - 1)
		);
		se_sdf_physics_destroy(physics_handle);
	}

	while (s_array_get_size(&ctx->sdfs) > 0) {
		se_sdf_handle scene_handle = s_array_handle(
			&ctx->sdfs,
			(u32)(s_array_get_size(&ctx->sdfs) - 1)
		);
		se_sdf_destroy(scene_handle);
	}

	s_array_clear(&ctx->sdf_physicses);
	s_array_clear(&ctx->sdf_renderers);
	s_array_clear(&ctx->sdfs);
	if (g_se_sdf_prepare_pool) {
		se_worker_destroy(g_se_sdf_prepare_pool);
		g_se_sdf_prepare_pool = NULL;
	}
}

b8 se_sdf_renderer_set_sdf(const se_sdf_renderer_handle renderer, const se_sdf_handle sdf) {
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return 0;
	}
	if (sdf != SE_SDF_NULL && !se_sdf_scene_from_handle(sdf)) {
		return 0;
	}
	renderer_ptr->scene = sdf;
	s_array_clear(&renderer_ptr->resident_lod_entries);
	s_array_clear(&renderer_ptr->frame_lod_entries);
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

static f32 se_sdf_renderer_distance_to_bounds(
	const s_vec3* camera_position,
	const se_sdf_bounds* bounds
) {
	if (!camera_position || !bounds || !bounds->valid) {
		return FLT_MAX;
	}
	const s_vec3 delta = s_vec3_sub(&bounds->center, camera_position);
	return s_max(0.0f, s_vec3_length(&delta) - bounds->radius);
}

static void se_sdf_renderer_sort_visible_refs(
	se_sdf_prepared_cache* cache,
	se_sdf_u32_array* visible_ref_indices
) {
	if (!cache || !visible_ref_indices) {
		return;
	}
	for (sz i = 1; i < s_array_get_size(visible_ref_indices); ++i) {
		u32* key_ptr = s_array_get(visible_ref_indices, s_array_handle(visible_ref_indices, (u32)i));
		if (!key_ptr) {
			continue;
		}
		const u32 key = *key_ptr;
		se_sdf_prepared_ref* key_ref = s_array_get(&cache->refs, s_array_handle(&cache->refs, key));
		if (!key_ref) {
			continue;
		}
		sz j = i;
		while (j > 0u) {
			u32* prev_ptr = s_array_get(visible_ref_indices, s_array_handle(visible_ref_indices, (u32)(j - 1u)));
			if (!prev_ptr) {
				break;
			}
			se_sdf_prepared_ref* prev_ref = s_array_get(&cache->refs, s_array_handle(&cache->refs, *prev_ptr));
			if (!prev_ref || prev_ref->sort_distance <= key_ref->sort_distance) {
				break;
			}
			*key_ptr = *prev_ptr;
			*prev_ptr = key;
			--j;
			key_ptr = s_array_get(visible_ref_indices, s_array_handle(visible_ref_indices, (u32)j));
			if (!key_ptr) {
				break;
			}
		}
	}
}

static b8 se_sdf_renderer_collect_visible_refs(
	se_sdf_renderer* renderer,
	const se_sdf_frame_desc* frame,
	se_sdf_scene* scene_ptr
) {
	if (!renderer || !frame || !scene_ptr) {
		return 0;
	}
	s_array_clear(&renderer->visible_ref_indices);
	if (!scene_ptr->prepared.ready || s_array_get_size(&scene_ptr->prepared.refs) == 0u) {
		return 1;
	}
	se_debug_trace_begin("sdf_cull");

	s_vec3 camera_position = s_vec3(0.0f, 0.0f, 0.0f);
	(void)se_camera_get_location(frame->camera, &camera_position);
	const f32 max_distance = renderer->quality.max_distance > 0.0f ? renderer->quality.max_distance : 256.0f;
	const i32 min_x = s_max(0, s_min(scene_ptr->prepared.grid_nx - 1, (i32)floorf((camera_position.x - max_distance - scene_ptr->prepared.grid_min.x) / scene_ptr->prepared.cell_size)));
	const i32 min_y = s_max(0, s_min(scene_ptr->prepared.grid_ny - 1, (i32)floorf((camera_position.y - max_distance - scene_ptr->prepared.grid_min.y) / scene_ptr->prepared.cell_size)));
	const i32 min_z = s_max(0, s_min(scene_ptr->prepared.grid_nz - 1, (i32)floorf((camera_position.z - max_distance - scene_ptr->prepared.grid_min.z) / scene_ptr->prepared.cell_size)));
	const i32 max_x = s_max(0, s_min(scene_ptr->prepared.grid_nx - 1, (i32)floorf((camera_position.x + max_distance - scene_ptr->prepared.grid_min.x) / scene_ptr->prepared.cell_size)));
	const i32 max_y = s_max(0, s_min(scene_ptr->prepared.grid_ny - 1, (i32)floorf((camera_position.y + max_distance - scene_ptr->prepared.grid_min.y) / scene_ptr->prepared.cell_size)));
	const i32 max_z = s_max(0, s_min(scene_ptr->prepared.grid_nz - 1, (i32)floorf((camera_position.z + max_distance - scene_ptr->prepared.grid_min.z) / scene_ptr->prepared.cell_size)));

	const sz ref_count = s_array_get_size(&scene_ptr->prepared.refs);
	u8* visited = calloc(ref_count > 0u ? ref_count : 1u, sizeof(*visited));
	if (!visited) {
		se_debug_trace_end("sdf_cull");
		return 0;
	}
	for (i32 z = min_z; z <= max_z; ++z) {
		for (i32 y = min_y; y <= max_y; ++y) {
			for (i32 x = min_x; x <= max_x; ++x) {
				const sz cell_index = (sz)x + (sz)scene_ptr->prepared.grid_nx * ((sz)y + (sz)scene_ptr->prepared.grid_ny * (sz)z);
				se_sdf_grid_cell* cell = s_array_get(&scene_ptr->prepared.grid_cells, s_array_handle(&scene_ptr->prepared.grid_cells, (u32)cell_index));
				if (!cell) {
					continue;
				}
				for (sz i = 0; i < s_array_get_size(&cell->ref_indices); ++i) {
					u32* ref_index_ptr = s_array_get(&cell->ref_indices, s_array_handle(&cell->ref_indices, (u32)i));
					if (!ref_index_ptr || *ref_index_ptr >= ref_count || visited[*ref_index_ptr]) {
						continue;
					}
					visited[*ref_index_ptr] = 1u;
					se_sdf_prepared_ref* ref = s_array_get(&scene_ptr->prepared.refs, s_array_handle(&scene_ptr->prepared.refs, *ref_index_ptr));
					if (!ref || !ref->bounds.valid) {
						continue;
					}
					ref->sort_distance = se_sdf_renderer_distance_to_bounds(&camera_position, &ref->bounds);
					if (ref->sort_distance > max_distance) {
						continue;
					}
					s_vec3 ndc = s_vec3(0.0f, 0.0f, 0.0f);
					if (!se_camera_world_to_ndc(frame->camera, &ref->bounds.center, &ndc)) {
						continue;
					}
					if (ndc.z < -1.5f || ndc.z > 1.5f || fabsf(ndc.x) > 2.0f || fabsf(ndc.y) > 2.0f) {
						continue;
					}
					s_array_add(&renderer->visible_ref_indices, *ref_index_ptr);
				}
			}
		}
	}
	free(visited);
	if (renderer->desc.front_to_back_sort) {
		se_sdf_renderer_sort_visible_refs(&scene_ptr->prepared, &renderer->visible_ref_indices);
	}
	while (s_array_get_size(&renderer->visible_ref_indices) > renderer->desc.max_visible_instances) {
		s_array_remove_ordered(&renderer->visible_ref_indices, s_array_handle(&renderer->visible_ref_indices, (u32)(s_array_get_size(&renderer->visible_ref_indices) - 1u)));
	}
	se_debug_trace_end("sdf_cull");
	return 1;
}

static se_sdf_lod_selection se_sdf_renderer_choose_lod(
	const se_sdf_prepared_cache* prepared,
	const se_sdf_prepared_lod* lods,
	const se_sdf_frame_desc* frame,
	const se_camera* camera_ptr,
	const se_sdf_bounds* bounds,
	const f32 lod_fade_ratio
) {
	se_sdf_lod_selection selection = { 0u, 0u, 0.0f };
	if (!prepared || !lods || !camera_ptr || !bounds || !bounds->valid) {
		return selection;
	}
	const u32 lod_count = prepared->desc.lod_count > 0u ? prepared->desc.lod_count : 1u;
	u32 current_lod = 0u;
	b8 found_valid_lod = 0;
	for (u32 lod = 0u; lod < lod_count; ++lod) {
		if (lods[lod].valid) {
			current_lod = lod;
			found_valid_lod = 1;
			break;
		}
	}
	if (!found_valid_lod) {
		return selection;
	}
	s_vec3 camera_position = s_vec3(0.0f, 0.0f, 0.0f);
	(void)se_camera_get_location(frame->camera, &camera_position);
	const f32 distance = s_max(se_sdf_renderer_distance_to_bounds(&camera_position, bounds), 0.01f);
	f32 pixels_per_world = 1.0f;
	if (camera_ptr->use_orthographic) {
		pixels_per_world = frame->resolution.y / s_max(camera_ptr->ortho_height * 2.0f, 0.01f);
	} else {
		const f32 tan_half_fov = tanf((camera_ptr->fov * PI / 180.0f) * 0.5f);
		pixels_per_world = frame->resolution.y / s_max(2.0f * tan_half_fov * distance, 0.01f);
	}
	const f32 target_pixels = 0.75f;
	const f32 fade_ratio = fminf(fmaxf(lod_fade_ratio, 0.0f), 0.95f);
	const f32 upper = target_pixels * (1.0f + fade_ratio);
	const f32 lower = target_pixels * fmaxf(1.0f - fade_ratio, 0.05f);
	selection.primary_lod = current_lod;
	selection.secondary_lod = current_lod;
	selection.blend = 0.0f;
	for (u32 lod = current_lod + 1u; lod < lod_count; ++lod) {
		const se_sdf_prepared_lod* candidate_lod = &lods[lod];
		if (!candidate_lod->valid) {
			continue;
		}
		const f32 voxel_world = fmaxf(candidate_lod->voxel_size.x, fmaxf(candidate_lod->voxel_size.y, candidate_lod->voxel_size.z));
		const f32 projected_pixels = voxel_world * pixels_per_world;
		if (projected_pixels > upper) {
			break;
		}
		if (fade_ratio <= 0.0f || projected_pixels <= lower) {
			current_lod = lod;
			selection.primary_lod = lod;
			selection.secondary_lod = lod;
			selection.blend = 0.0f;
			continue;
		}
		selection.primary_lod = current_lod;
		selection.secondary_lod = lod;
		selection.blend = s_max(0.0f, s_min(1.0f, (upper - projected_pixels) / s_max(upper - lower, 0.0001f)));
		return selection;
	}
	return selection;
}

static b8 se_sdf_renderer_draw_volume_batch(
	se_sdf_renderer* renderer,
	const se_sdf_frame_desc* frame,
	const se_sdf_prepared_cache* prepared,
	const se_sdf_bounds* bounds,
	const s_vec3* batch_color,
	const se_sdf_prepared_lod* lods,
	const u32 primary_lod_index,
	const u32 secondary_lod_index,
	const se_sdf_mat4_array* mvp_matrices,
	const se_sdf_mat4_array* inverse_matrices,
	const se_sdf_f32_array* lod_blends
) {
	if (!renderer || !frame || !prepared || !bounds || !lods || !mvp_matrices || !inverse_matrices || !lod_blends || primary_lod_index >= 4u || secondary_lod_index >= 4u) {
		return 0;
	}
	const sz instance_count = s_array_get_size((se_sdf_mat4_array*)mvp_matrices);
	if (instance_count == 0u) {
		return 1;
	}
	if (s_array_get_size((se_sdf_mat4_array*)inverse_matrices) != instance_count || s_array_get_size((se_sdf_f32_array*)lod_blends) != instance_count) {
		return 0;
	}
	if (getenv("SE_SDF_DEBUG_INSTANCE_MATRIX") != NULL) {
		const s_mat4* first_mvp = s_array_get((se_sdf_mat4_array*)mvp_matrices, s_array_handle((se_sdf_mat4_array*)mvp_matrices, 0u));
		if (first_mvp) {
			f32 min_ndc_x = 1e9f;
			f32 min_ndc_y = 1e9f;
			f32 max_ndc_x = -1e9f;
			f32 max_ndc_y = -1e9f;
			for (u32 corner = 0u; corner < 8u; ++corner) {
				const f32 x = (corner & 1u) ? 1.0f : -1.0f;
				const f32 y = (corner & 2u) ? 1.0f : -1.0f;
				const f32 z = (corner & 4u) ? 1.0f : -1.0f;
				const f32 clip_x = first_mvp->m[0][0] * x + first_mvp->m[1][0] * y + first_mvp->m[2][0] * z + first_mvp->m[3][0];
				const f32 clip_y = first_mvp->m[0][1] * x + first_mvp->m[1][1] * y + first_mvp->m[2][1] * z + first_mvp->m[3][1];
				const f32 clip_w = first_mvp->m[0][3] * x + first_mvp->m[1][3] * y + first_mvp->m[2][3] * z + first_mvp->m[3][3];
				if (fabsf(clip_w) > 0.00001f) {
					const f32 ndc_x = clip_x / clip_w;
					const f32 ndc_y = clip_y / clip_w;
					min_ndc_x = fminf(min_ndc_x, ndc_x);
					min_ndc_y = fminf(min_ndc_y, ndc_y);
					max_ndc_x = fmaxf(max_ndc_x, ndc_x);
					max_ndc_y = fmaxf(max_ndc_y, ndc_y);
				}
			}
			fprintf(stderr, "se_sdf_renderer_draw_volume_batch :: ndc_bounds=(%.3f, %.3f)-(%.3f, %.3f)\n", min_ndc_x, min_ndc_y, max_ndc_x, max_ndc_y);
		}
	}
	const se_sdf_prepared_lod* primary_lod = &lods[primary_lod_index];
	const se_sdf_prepared_lod* secondary_lod = &lods[secondary_lod_index];
	if (!primary_lod->valid || primary_lod->texture == S_HANDLE_NULL || primary_lod->page_table_texture == S_HANDLE_NULL ||
		!secondary_lod->valid || secondary_lod->texture == S_HANDLE_NULL || secondary_lod->page_table_texture == S_HANDLE_NULL) {
		return 0;
	}
	if (!se_sdf_renderer_ensure_volume_gpu(renderer)) {
		return 0;
	}

	se_context* ctx = se_current_context();
	if (!ctx) {
		return 0;
	}
	se_texture* primary_texture = s_array_get(&ctx->textures, primary_lod->texture);
	se_texture* secondary_texture = s_array_get(&ctx->textures, secondary_lod->texture);
	se_texture* primary_color_texture = primary_lod->color_texture != S_HANDLE_NULL ? s_array_get(&ctx->textures, primary_lod->color_texture) : NULL;
	se_texture* secondary_color_texture = secondary_lod->color_texture != S_HANDLE_NULL ? s_array_get(&ctx->textures, secondary_lod->color_texture) : NULL;
	se_texture* primary_page_table = s_array_get(&ctx->textures, primary_lod->page_table_texture);
	se_texture* secondary_page_table = s_array_get(&ctx->textures, secondary_lod->page_table_texture);
	if (!primary_texture || !secondary_texture || !primary_page_table || !secondary_page_table) {
		return 0;
	}

	se_debug_trace_begin("sdf_upload");
	se_debug_trace_end("sdf_upload");

	se_camera* camera_ptr = se_camera_get(frame->camera);
	const s_mat4 view_projection = se_camera_get_view_projection_matrix(frame->camera);
	const s_mat4 inverse_view_projection = s_mat4_inverse(&view_projection);
	s_vec3 camera_position = s_vec3(0.0f, 0.0f, 0.0f);
	(void)se_camera_get_location(frame->camera, &camera_position);
	const s_vec3 camera_forward = se_camera_get_forward_vector(frame->camera);
	const s_vec3 camera_right = se_camera_get_right_vector(frame->camera);
	const s_vec3 camera_up = se_camera_get_up_vector(frame->camera);
	const f32 tan_half_fov = tanf((camera_ptr ? camera_ptr->fov : 55.0f) * PI / 360.0f);
	const s_vec3 volume_size = s_vec3_sub(&bounds->max, &bounds->min);

	se_shader_set_vec2(renderer->shader, "u_resolution", &frame->resolution);
	se_shader_set_int(renderer->shader, "u_has_scene_depth", frame->has_scene_depth_texture ? 1 : 0);
	se_shader_set_texture(renderer->shader, "u_scene_depth", frame->has_scene_depth_texture ? frame->scene_depth_texture : renderer->fallback_scene_depth_texture);
	se_shader_set_texture(renderer->shader, "u_volume_atlas_primary", primary_texture->id);
	se_shader_set_texture(renderer->shader, "u_volume_atlas_secondary", secondary_texture->id);
	if (primary_color_texture) {
		se_shader_set_texture(renderer->shader, "u_volume_color_atlas_primary", primary_color_texture->id);
	}
	if (secondary_color_texture) {
		se_shader_set_texture(renderer->shader, "u_volume_color_atlas_secondary", secondary_color_texture->id);
	}
	se_shader_set_texture(renderer->shader, "u_volume_page_table_primary", primary_page_table->id);
	se_shader_set_texture(renderer->shader, "u_volume_page_table_secondary", secondary_page_table->id);
	se_shader_set_vec3(renderer->shader, "u_volume_size", &volume_size);
	const s_vec3 primary_resolution = s_vec3((f32)primary_lod->resolution, (f32)primary_lod->resolution, (f32)primary_lod->resolution);
	const s_vec3 secondary_resolution = s_vec3((f32)secondary_lod->resolution, (f32)secondary_lod->resolution, (f32)secondary_lod->resolution);
	se_shader_set_vec3(renderer->shader, "u_volume_resolution_primary", &primary_resolution);
	se_shader_set_vec3(renderer->shader, "u_volume_resolution_secondary", &secondary_resolution);
	se_shader_set_vec3(renderer->shader, "u_voxel_size_primary", &primary_lod->voxel_size);
	se_shader_set_vec3(renderer->shader, "u_voxel_size_secondary", &secondary_lod->voxel_size);
	se_shader_set_int(renderer->shader, "u_brick_size", (i32)(prepared->desc.brick_size > 0u ? prepared->desc.brick_size : primary_lod->brick_size));
	se_shader_set_int(renderer->shader, "u_brick_border", (i32)(prepared->desc.brick_border <= 8u ? prepared->desc.brick_border : 8u));
	se_shader_set_int(renderer->shader, "u_has_volume_color_primary", primary_color_texture ? 1 : 0);
	se_shader_set_int(renderer->shader, "u_has_volume_color_secondary", secondary_color_texture ? 1 : 0);
	se_shader_set_vec3(renderer->shader, "u_camera_position", &camera_position);
	se_shader_set_mat4(renderer->shader, "u_camera_inv_view_projection", &inverse_view_projection);
	se_shader_set_mat4(renderer->shader, "u_view_projection", &view_projection);
	se_shader_set_vec3(renderer->shader, "u_camera_forward", &camera_forward);
	se_shader_set_vec3(renderer->shader, "u_camera_right", &camera_right);
	se_shader_set_vec3(renderer->shader, "u_camera_up", &camera_up);
	se_shader_set_float(renderer->shader, "u_camera_tan_half_fov", tan_half_fov);
	se_shader_set_float(renderer->shader, "u_camera_aspect", frame->resolution.x / s_max(frame->resolution.y, 1.0f));
	se_shader_set_int(renderer->shader, "u_camera_is_orthographic", camera_ptr && camera_ptr->use_orthographic ? 1 : 0);
	se_shader_set_float(renderer->shader, "u_camera_ortho_height", camera_ptr ? camera_ptr->ortho_height : 10.0f);
	{
		const s_vec2 near_far = s_vec2(camera_ptr ? camera_ptr->near : 0.1f, camera_ptr ? camera_ptr->far : 100.0f);
		se_shader_set_vec2(renderer->shader, "u_camera_near_far", &near_far);
	}
	se_shader_set_float(renderer->shader, "u_surface_epsilon", renderer->quality.hit_epsilon);
	se_shader_set_float(renderer->shader, "u_min_step", 0.0012f);
	se_shader_set_int(renderer->shader, "u_max_steps", renderer->quality.max_steps);
	const s_vec3 resolved_batch_color = batch_color ? *batch_color : renderer->material.base_color;
	se_shader_set_vec3(renderer->shader, "u_material_base_color", &resolved_batch_color);
	se_shader_set_vec3(renderer->shader, "u_light_direction", &renderer->lighting_direction);
	se_shader_set_vec3(renderer->shader, "u_light_color", &renderer->lighting_color);
	se_shader_set_vec3(renderer->shader, "u_fog_color", &renderer->fog_color);
	se_shader_set_float(renderer->shader, "u_fog_density", renderer->fog_density);
	se_shader_set_int(renderer->shader, "u_debug_show_normals", renderer->debug.show_normals ? 1 : 0);
	se_shader_set_int(renderer->shader, "u_debug_show_distance", renderer->debug.show_distance ? 1 : 0);
	se_shader_set_int(renderer->shader, "u_debug_show_steps", renderer->debug.show_steps ? 1 : 0);
	se_shader_set_int(renderer->shader, "u_debug_show_proxy_boxes", getenv("SE_SDF_DEBUG_PROXY_BOXES") != NULL ? 1 : 0);
	se_shader_use(renderer->shader, true, false);

	(void)view_projection;
	if (getenv("SE_SDF_DEBUG_FRAMEBUFFER") != NULL) {
		GLint framebuffer_binding = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer_binding);
		fprintf(stderr, "se_sdf_renderer_draw_volume_batch :: framebuffer=%d\n", framebuffer_binding);
	}
	glBindVertexArray(renderer->cube_vao);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	for (sz instance_index = 0u; instance_index < instance_count; ++instance_index) {
		const s_mat4* instance_mvp = s_array_get((se_sdf_mat4_array*)mvp_matrices, s_array_handle((se_sdf_mat4_array*)mvp_matrices, (u32)instance_index));
		const s_mat4* instance_inverse = s_array_get((se_sdf_mat4_array*)inverse_matrices, s_array_handle((se_sdf_mat4_array*)inverse_matrices, (u32)instance_index));
		f32* lod_blend = s_array_get((se_sdf_f32_array*)lod_blends, s_array_handle((se_sdf_f32_array*)lod_blends, (u32)instance_index));
		if (!instance_mvp || !instance_inverse || !lod_blend) {
			continue;
		}
		const s_mat4 instance_model = s_mat4_inverse(instance_inverse);
		se_shader_set_mat4(renderer->shader, "u_instance_mvp", instance_mvp);
		se_shader_set_mat4(renderer->shader, "u_instance_model", &instance_model);
		se_shader_set_mat4(renderer->shader, "u_instance_inverse_model", instance_inverse);
		se_shader_set_float(renderer->shader, "u_instance_lod_blend", *lod_blend);
		se_shader_use(renderer->shader, true, false);
		glDrawElements(GL_TRIANGLES, (GLsizei)(sizeof(se_sdf_volume_cube_indices) / sizeof(se_sdf_volume_cube_indices[0])), GL_UNSIGNED_INT, 0);
	}
	glDisable(GL_CULL_FACE);
	glBindVertexArray(0);
	{
		const GLenum gl_error = glGetError();
		if (gl_error != GL_NO_ERROR) {
			se_sdf_set_diagnostics(renderer, 0, "gl_draw", "volume instanced draw failed");
			return 0;
		}
	}
	return 1;
}

static se_sdf_budget_entry* se_sdf_renderer_find_budget_entry(
	se_sdf_budget_entries* entries,
	const se_sdf_handle source,
	const b8 use_local_lods
) {
	if (!entries || source == SE_SDF_NULL) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(entries); ++i) {
		se_sdf_budget_entry* entry = s_array_get(entries, s_array_handle(entries, (u32)i));
		if (entry && entry->source == source && entry->use_local_lods == use_local_lods) {
			return entry;
		}
	}
	return NULL;
}

static u32 se_sdf_renderer_estimate_lod_bricks(
	const se_sdf_prepared_cache* prepared,
	const se_sdf_prepared_lod* lods,
	const u32 lod_index
) {
	if (!prepared || !lods || lod_index >= 4u || !lods[lod_index].valid) {
		return 0u;
	}
	if (lods[lod_index].brick_count_x > 0u && lods[lod_index].brick_count_y > 0u && lods[lod_index].brick_count_z > 0u) {
		return lods[lod_index].brick_count_x * lods[lod_index].brick_count_y * lods[lod_index].brick_count_z;
	}
	const u32 brick_size = prepared->desc.brick_size > 0u ? prepared->desc.brick_size : 32u;
	const u32 resolution = lods[lod_index].resolution > 0u ? lods[lod_index].resolution : 1u;
	const u32 bricks_per_axis = (resolution + brick_size - 1u) / brick_size;
	return bricks_per_axis * bricks_per_axis * bricks_per_axis;
}

static u32 se_sdf_renderer_estimate_mask_bricks(
	const se_sdf_prepared_cache* prepared,
	const se_sdf_prepared_lod* lods,
	const u8 lod_mask
) {
	u32 cost = 0u;
	for (u32 lod = 0u; lod < 4u; ++lod) {
		if ((lod_mask & (u8)(1u << lod)) != 0u) {
			cost += se_sdf_renderer_estimate_lod_bricks(prepared, lods, lod);
		}
	}
	return cost;
}

static u8 se_sdf_renderer_selection_mask(const se_sdf_lod_selection selection) {
	u8 mask = (u8)(1u << selection.primary_lod);
	if (selection.secondary_lod != selection.primary_lod && selection.blend > 0.0f) {
		mask = (u8)(mask | (u8)(1u << selection.secondary_lod));
	}
	return mask;
}

static u32 se_sdf_renderer_estimate_entry_bricks(const se_sdf_budget_entry* entry) {
	if (!entry || entry->source == SE_SDF_NULL || entry->lod_mask == 0u) {
		return 0u;
	}
	se_sdf_scene* source_ptr = se_sdf_scene_from_handle(entry->source);
	if (!source_ptr || !source_ptr->prepared.ready) {
		return 0u;
	}
	const se_sdf_prepared_lod* lods = entry->use_local_lods ? source_ptr->prepared.local_lods : source_ptr->prepared.full_lods;
	return se_sdf_renderer_estimate_mask_bricks(&source_ptr->prepared, lods, entry->lod_mask);
}

static u32 se_sdf_renderer_estimate_resident_bricks(const se_sdf_budget_entries* entries) {
	if (!entries) {
		return 0u;
	}
	u32 total = 0u;
	for (sz i = 0; i < s_array_get_size((se_sdf_budget_entries*)entries); ++i) {
		se_sdf_budget_entry* entry = s_array_get((se_sdf_budget_entries*)entries, s_array_handle((se_sdf_budget_entries*)entries, (u32)i));
		total += se_sdf_renderer_estimate_entry_bricks(entry);
	}
	return total;
}

static void se_sdf_renderer_commit_budget_entry(
	se_sdf_budget_entries* entries,
	const se_sdf_handle source,
	const b8 use_local_lods,
	const u8 lod_mask,
	const u64 last_used_frame
) {
	if (!entries || source == SE_SDF_NULL || lod_mask == 0u) {
		return;
	}
	se_sdf_budget_entry* existing = se_sdf_renderer_find_budget_entry(entries, source, use_local_lods);
	if (existing) {
		existing->lod_mask = (u8)(existing->lod_mask | lod_mask);
		existing->last_used_frame = last_used_frame;
		return;
	}
	se_sdf_budget_entry entry = {
		.source = source,
		.lod_mask = lod_mask,
		.use_local_lods = use_local_lods,
		.last_used_frame = last_used_frame
	};
	s_array_add(entries, entry);
}

static b8 se_sdf_renderer_evict_resident_bricks(
	se_sdf_renderer* renderer,
	const se_sdf_handle preserve_source,
	const b8 preserve_local_lods,
	const u8 preserve_mask,
	u32 required_free,
	u32* io_resident_bricks
) {
	if (!renderer || !io_resident_bricks) {
		return 0;
	}
	while (required_free > 0u) {
		s_handle best_handle = S_HANDLE_NULL;
		u32 best_lod = UINT_MAX;
		u64 best_last_used = UINT64_MAX;
		for (sz i = 0; i < s_array_get_size(&renderer->resident_lod_entries); ++i) {
			const s_handle handle = s_array_handle(&renderer->resident_lod_entries, (u32)i);
			se_sdf_budget_entry* entry = s_array_get(&renderer->resident_lod_entries, handle);
			if (!entry || entry->lod_mask == 0u) {
				continue;
			}
			for (u32 lod = 0u; lod < 4u; ++lod) {
				const u8 bit = (u8)(1u << lod);
				if ((entry->lod_mask & bit) == 0u) {
					continue;
				}
				if (entry->source == preserve_source &&
					entry->use_local_lods == preserve_local_lods &&
					(preserve_mask & bit) != 0u) {
					continue;
				}
				if (best_handle == S_HANDLE_NULL ||
					lod < best_lod ||
					(lod == best_lod && entry->last_used_frame < best_last_used)) {
					best_handle = handle;
					best_lod = lod;
					best_last_used = entry->last_used_frame;
				}
			}
		}
		if (best_handle == S_HANDLE_NULL || best_lod == UINT_MAX) {
			return 0;
		}
		se_sdf_budget_entry* best_entry = s_array_get(&renderer->resident_lod_entries, best_handle);
		if (!best_entry) {
			return 0;
		}
		const u8 bit = (u8)(1u << best_lod);
		const u32 freed = se_sdf_renderer_estimate_entry_bricks(&(se_sdf_budget_entry){
			.source = best_entry->source,
			.use_local_lods = best_entry->use_local_lods,
			.lod_mask = bit
		});
		best_entry->lod_mask = (u8)(best_entry->lod_mask & (u8)~bit);
		if (best_entry->lod_mask == 0u) {
			s_array_remove_ordered(&renderer->resident_lod_entries, best_handle);
		}
		if (*io_resident_bricks > freed) {
			*io_resident_bricks -= freed;
		} else {
			*io_resident_bricks = 0u;
		}
		if (required_free > freed) {
			required_free -= freed;
		} else {
			required_free = 0u;
		}
	}
	return 1;
}

static se_sdf_lod_selection se_sdf_renderer_apply_lod_budget(
	se_sdf_renderer* renderer,
	const se_sdf_handle source,
	const b8 use_local_lods,
	const se_sdf_prepared_cache* prepared,
	const se_sdf_prepared_lod* lods,
	se_sdf_lod_selection desired,
	u32* io_resident_bricks,
	u32* io_upload_bricks
) {
	if (!renderer || !prepared || !lods || !io_resident_bricks || !io_upload_bricks) {
		return desired;
	}
	const u32 brick_budget = renderer->desc.brick_budget > 0u ? renderer->desc.brick_budget : UINT_MAX;
	const u32 upload_budget = renderer->desc.brick_upload_budget > 0u ? renderer->desc.brick_upload_budget : UINT_MAX;
	const u32 lod_count = prepared->desc.lod_count > 0u ? prepared->desc.lod_count : 1u;
	const se_sdf_lod_selection original = desired;

	while (1) {
		const u8 desired_mask = se_sdf_renderer_selection_mask(desired);
		se_sdf_budget_entry* resident_entry = se_sdf_renderer_find_budget_entry(&renderer->resident_lod_entries, source, use_local_lods);
		const u8 resident_mask = resident_entry ? resident_entry->lod_mask : 0u;
		const u8 missing_mask = (u8)(desired_mask & (u8)~resident_mask);
		const u32 extra_resident = se_sdf_renderer_estimate_mask_bricks(prepared, lods, missing_mask);
		const u32 extra_upload = extra_resident;
		if (*io_resident_bricks + extra_resident > brick_budget) {
			const u32 required_free = (*io_resident_bricks + extra_resident) - brick_budget;
			(void)se_sdf_renderer_evict_resident_bricks(renderer, source, use_local_lods, desired_mask, required_free, io_resident_bricks);
		}
		const b8 fits_budget = *io_resident_bricks + extra_resident <= brick_budget;
		const b8 fits_upload = *io_upload_bricks + extra_upload <= upload_budget;
		if ((fits_budget && fits_upload) ||
			(desired.primary_lod == lod_count - 1u && desired.secondary_lod == desired.primary_lod)) {
			*io_resident_bricks += extra_resident;
			*io_upload_bricks += extra_upload;
			se_sdf_renderer_commit_budget_entry(&renderer->resident_lod_entries, source, use_local_lods, desired_mask, renderer->residency_frame);
			se_sdf_renderer_commit_budget_entry(&renderer->frame_lod_entries, source, use_local_lods, desired_mask, renderer->residency_frame);
			if (desired.primary_lod != original.primary_lod || desired.secondary_lod != original.secondary_lod || fabsf(desired.blend - original.blend) > 0.0001f) {
				renderer->stats.page_misses++;
			}
			return desired;
		}
		if (desired.secondary_lod != desired.primary_lod && desired.blend > 0.0f) {
			desired.primary_lod = desired.secondary_lod;
			desired.blend = 0.0f;
			continue;
		}
		u32 next_lod = desired.primary_lod + 1u;
		while (next_lod < lod_count && !lods[next_lod].valid) {
			next_lod++;
		}
		if (next_lod >= lod_count) {
			*io_resident_bricks += extra_resident;
			*io_upload_bricks += extra_upload;
			se_sdf_renderer_commit_budget_entry(&renderer->resident_lod_entries, source, use_local_lods, desired_mask, renderer->residency_frame);
			se_sdf_renderer_commit_budget_entry(&renderer->frame_lod_entries, source, use_local_lods, desired_mask, renderer->residency_frame);
			if (desired.primary_lod != original.primary_lod || desired.secondary_lod != original.secondary_lod || fabsf(desired.blend - original.blend) > 0.0001f) {
				renderer->stats.page_misses++;
			}
			return desired;
		}
		desired.primary_lod = next_lod;
		desired.secondary_lod = next_lod;
		desired.blend = 0.0f;
	}
}

static void se_sdf_renderer_add_bounds_instance(
	se_sdf_renderer* renderer,
	const se_sdf_bounds* bounds,
	const s_mat4* parent_transform,
	const s_mat4* view_projection,
	const f32 lod_blend
) {
	if (!renderer || !bounds || !bounds->valid || !parent_transform || !view_projection) {
		return;
	}
	const s_vec3 center = bounds->center;
	const s_vec3 half = s_vec3(
		fmaxf((bounds->max.x - bounds->min.x) * 0.5f, 0.0001f),
		fmaxf((bounds->max.y - bounds->min.y) * 0.5f, 0.0001f),
		fmaxf((bounds->max.z - bounds->min.z) * 0.5f, 0.0001f)
	);
	s_mat4 model = s_mat4_identity;
	model = s_mat4_translate(&model, &center);
	model = s_mat4_scale(&model, &half);
	model = s_mat4_mul(parent_transform, &model);
	s_mat4 mvp = s_mat4_mul(view_projection, &model);
	s_mat4 inverse_model = s_mat4_inverse(&model);
	s_array_add(&renderer->instance_models, mvp);
	s_array_add(&renderer->instance_inverses, inverse_model);
	s_array_add(&renderer->instance_lod_blends, lod_blend);
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
	if (renderer_ptr->scene == SE_SDF_NULL) {
		se_sdf_set_diagnostics(renderer_ptr, 0, "scene", "render skipped: renderer has no sdf");
		return 0;
	}
	se_sdf_scene* scene_ptr = se_sdf_scene_from_handle(renderer_ptr->scene);
	if (!scene_ptr) {
		se_sdf_set_diagnostics(renderer_ptr, 0, "scene", "render skipped: sdf handle is invalid");
		return 0;
	}
	if (!scene_ptr->prepared.ready) {
		se_sdf_set_diagnostics(renderer_ptr, 0, "prepare", "render skipped: sdf is not prepared; call se_sdf_prepare first");
		return 0;
	}
	if (scene_ptr->prepared.generation != scene_ptr->generation) {
		se_sdf_set_diagnostics(renderer_ptr, 0, "prepare", "render skipped: sdf changed after prepare; call se_sdf_prepare again");
		return 0;
	}
	s_vec2 resolution = frame->resolution;
	if (resolution.x <= 0.0f) resolution.x = 1.0f;
	if (resolution.y <= 0.0f) resolution.y = 1.0f;
	const s_mat4 view = se_camera_get_view_matrix(frame->camera);
	const s_mat4 projection = se_camera_get_projection_matrix(frame->camera);
	const s_mat4 view_projection = s_mat4_mul(&projection, &view);
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
	se_debug_trace_begin("sdf_render");

	memset(&renderer_ptr->stats, 0, sizeof(renderer_ptr->stats));
	renderer_ptr->residency_frame++;
	s_array_clear(&renderer_ptr->frame_lod_entries);
	u32 resident_bricks_used = se_sdf_renderer_estimate_resident_bricks(&renderer_ptr->resident_lod_entries);
	u32 upload_bricks_used = 0u;
	const u32 brick_budget = renderer_ptr->desc.brick_budget > 0u ? renderer_ptr->desc.brick_budget : UINT_MAX;
	if (resident_bricks_used > brick_budget) {
		(void)se_sdf_renderer_evict_resident_bricks(
			renderer_ptr,
			SE_SDF_NULL,
			0,
			0u,
			resident_bricks_used - brick_budget,
			&resident_bricks_used
		);
	}
	b8 render_ok = 1;
	const b8 render_local = scene_ptr->prepared.local_lods_valid && scene_ptr->prepared.local_bounds.valid;
	const b8 render_refs = scene_ptr->prepared.instancing_supported && s_array_get_size(&scene_ptr->prepared.refs) > 0u;
	if (render_local) {
		const se_sdf_lod_selection local_lod = se_sdf_renderer_choose_lod(
			&scene_ptr->prepared,
			scene_ptr->prepared.local_lods,
			frame,
			camera_ptr,
			&scene_ptr->prepared.local_bounds,
			renderer_ptr->desc.lod_fade_ratio
		);
		const se_sdf_lod_selection budgeted_local_lod = se_sdf_renderer_apply_lod_budget(
			renderer_ptr,
			renderer_ptr->scene,
			1,
			&scene_ptr->prepared,
			scene_ptr->prepared.local_lods,
			local_lod,
			&resident_bricks_used,
			&upload_bricks_used
		);
		s_array_clear(&renderer_ptr->instance_models);
		s_array_clear(&renderer_ptr->instance_inverses);
		s_array_clear(&renderer_ptr->instance_lod_blends);
		se_sdf_renderer_add_bounds_instance(
			renderer_ptr,
			&scene_ptr->prepared.local_bounds,
			&s_mat4_identity,
			&view_projection,
			budgeted_local_lod.blend
		);
		renderer_ptr->stats.selected_lod_counts[budgeted_local_lod.primary_lod] += 1u;
		if (budgeted_local_lod.secondary_lod != budgeted_local_lod.primary_lod && budgeted_local_lod.blend > 0.0f) {
			renderer_ptr->stats.selected_lod_counts[budgeted_local_lod.secondary_lod] += 1u;
		}
		render_ok = se_sdf_renderer_draw_volume_batch(
			renderer_ptr,
			frame,
			&scene_ptr->prepared,
			&scene_ptr->prepared.local_bounds,
			scene_ptr->prepared.local_color_valid ? &scene_ptr->prepared.local_color : NULL,
			scene_ptr->prepared.local_lods,
			budgeted_local_lod.primary_lod,
			budgeted_local_lod.secondary_lod,
			&renderer_ptr->instance_models,
			&renderer_ptr->instance_inverses,
			&renderer_ptr->instance_lod_blends
		);
	}
	if (render_refs && render_ok) {
		if (!se_sdf_renderer_collect_visible_refs(renderer_ptr, frame, scene_ptr)) {
			render_ok = 0;
		} else {
			renderer_ptr->stats.visible_refs = (u32)s_array_get_size(&renderer_ptr->visible_ref_indices);
			const sz visible_count = s_array_get_size(&renderer_ptr->visible_ref_indices);
			u8* consumed = calloc(visible_count > 0u ? visible_count : 1u, sizeof(*consumed));
			se_sdf_lod_selection* lod_choices = calloc(visible_count > 0u ? visible_count : 1u, sizeof(*lod_choices));
			if (!consumed || !lod_choices) {
				render_ok = 0;
			} else {
				for (sz i = 0; i < visible_count; ++i) {
					u32* ref_index_ptr = s_array_get(&renderer_ptr->visible_ref_indices, s_array_handle(&renderer_ptr->visible_ref_indices, (u32)i));
					se_sdf_prepared_ref* ref = ref_index_ptr ? s_array_get(&scene_ptr->prepared.refs, s_array_handle(&scene_ptr->prepared.refs, *ref_index_ptr)) : NULL;
					se_sdf_scene* child_ptr = ref ? se_sdf_scene_from_handle(ref->source) : NULL;
					if (!child_ptr) {
						lod_choices[i] = (se_sdf_lod_selection){ 0u, 0u, 0.0f };
						continue;
					}
					se_sdf_lod_selection desired_lod = se_sdf_renderer_choose_lod(
						&child_ptr->prepared,
						child_ptr->prepared.full_lods,
						frame,
						camera_ptr,
						&ref->bounds,
						renderer_ptr->desc.lod_fade_ratio
					);
					lod_choices[i] = se_sdf_renderer_apply_lod_budget(
						renderer_ptr,
						ref->source,
						0,
						&child_ptr->prepared,
						child_ptr->prepared.full_lods,
						desired_lod,
						&resident_bricks_used,
						&upload_bricks_used
					);
				}
				for (sz i = 0; i < visible_count && render_ok; ++i) {
					if (consumed[i]) {
						continue;
					}
					u32* ref_index_ptr = s_array_get(&renderer_ptr->visible_ref_indices, s_array_handle(&renderer_ptr->visible_ref_indices, (u32)i));
					se_sdf_prepared_ref* ref = ref_index_ptr ? s_array_get(&scene_ptr->prepared.refs, s_array_handle(&scene_ptr->prepared.refs, *ref_index_ptr)) : NULL;
					se_sdf_scene* child_ptr = ref ? se_sdf_scene_from_handle(ref->source) : NULL;
					if (!ref || !child_ptr) {
						continue;
					}
					const se_sdf_lod_selection lod_selection = lod_choices[i];
					s_array_clear(&renderer_ptr->instance_models);
					s_array_clear(&renderer_ptr->instance_inverses);
					s_array_clear(&renderer_ptr->instance_lod_blends);
					for (sz j = i; j < visible_count; ++j) {
						if (consumed[j]) {
							continue;
						}
						u32* candidate_ref_index_ptr = s_array_get(&renderer_ptr->visible_ref_indices, s_array_handle(&renderer_ptr->visible_ref_indices, (u32)j));
						se_sdf_prepared_ref* candidate_ref = candidate_ref_index_ptr ? s_array_get(&scene_ptr->prepared.refs, s_array_handle(&scene_ptr->prepared.refs, *candidate_ref_index_ptr)) : NULL;
						if (!candidate_ref || candidate_ref->source != ref->source ||
							lod_choices[j].primary_lod != lod_selection.primary_lod ||
							lod_choices[j].secondary_lod != lod_selection.secondary_lod) {
							continue;
						}
						consumed[j] = 1u;
						se_sdf_renderer_add_bounds_instance(
							renderer_ptr,
							&child_ptr->prepared.bounds,
							&candidate_ref->transform,
							&view_projection,
							lod_choices[j].blend
						);
					}
					renderer_ptr->stats.selected_lod_counts[lod_selection.primary_lod] += (u32)s_array_get_size(&renderer_ptr->instance_models);
					if (lod_selection.secondary_lod != lod_selection.primary_lod && lod_selection.blend > 0.0f) {
						renderer_ptr->stats.selected_lod_counts[lod_selection.secondary_lod] += (u32)s_array_get_size(&renderer_ptr->instance_models);
					}
					render_ok = se_sdf_renderer_draw_volume_batch(
						renderer_ptr,
						frame,
						&child_ptr->prepared,
						&child_ptr->prepared.bounds,
						child_ptr->prepared.full_color_valid ? &child_ptr->prepared.full_color : NULL,
						child_ptr->prepared.full_lods,
						lod_selection.primary_lod,
						lod_selection.secondary_lod,
						&renderer_ptr->instance_models,
						&renderer_ptr->instance_inverses,
						&renderer_ptr->instance_lod_blends
					);
				}
			}
			if (consumed) {
				free(consumed);
			}
			if (lod_choices) {
				free(lod_choices);
			}
		}
	} else if (!render_local) {
		const se_sdf_lod_selection lod_selection = se_sdf_renderer_choose_lod(
			&scene_ptr->prepared,
			scene_ptr->prepared.full_lods,
			frame,
			camera_ptr,
			&scene_ptr->prepared.bounds,
			renderer_ptr->desc.lod_fade_ratio
		);
		const se_sdf_lod_selection budgeted_root_lod = se_sdf_renderer_apply_lod_budget(
			renderer_ptr,
			renderer_ptr->scene,
			0,
			&scene_ptr->prepared,
			scene_ptr->prepared.full_lods,
			lod_selection,
			&resident_bricks_used,
			&upload_bricks_used
		);
		s_array_clear(&renderer_ptr->instance_models);
		s_array_clear(&renderer_ptr->instance_inverses);
		s_array_clear(&renderer_ptr->instance_lod_blends);
		se_sdf_renderer_add_bounds_instance(
			renderer_ptr,
			&scene_ptr->prepared.bounds,
			&s_mat4_identity,
			&view_projection,
			budgeted_root_lod.blend
		);
		renderer_ptr->stats.visible_refs = 1u;
		renderer_ptr->stats.selected_lod_counts[budgeted_root_lod.primary_lod] = 1u;
		if (budgeted_root_lod.secondary_lod != budgeted_root_lod.primary_lod && budgeted_root_lod.blend > 0.0f) {
			renderer_ptr->stats.selected_lod_counts[budgeted_root_lod.secondary_lod] = 1u;
		}
		render_ok = se_sdf_renderer_draw_volume_batch(
			renderer_ptr,
			frame,
			&scene_ptr->prepared,
			&scene_ptr->prepared.bounds,
			scene_ptr->prepared.full_color_valid ? &scene_ptr->prepared.full_color : NULL,
			scene_ptr->prepared.full_lods,
			budgeted_root_lod.primary_lod,
			budgeted_root_lod.secondary_lod,
			&renderer_ptr->instance_models,
			&renderer_ptr->instance_inverses,
			&renderer_ptr->instance_lod_blends
		);
	}
	renderer_ptr->stats.resident_bricks = resident_bricks_used;
	se_debug_trace_end("sdf_render");

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

	if (!render_ok) {
		se_sdf_set_diagnostics(renderer_ptr, 0, "render", "prepared volume render failed");
		return 0;
	}
	se_sdf_set_diagnostics(renderer_ptr, 1, "render", "prepared volume render ok");
	return 1;
}

se_sdf_build_diagnostics se_sdf_renderer_get_build_diagnostics(const se_sdf_renderer_handle renderer) {
	se_sdf_build_diagnostics diagnostics = {0};
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return diagnostics;
	}
	return renderer_ptr->diagnostics;
}

se_sdf_renderer_stats se_sdf_renderer_get_stats(const se_sdf_renderer_handle renderer) {
	se_sdf_renderer_stats stats = {0};
	se_sdf_renderer* renderer_ptr = se_sdf_renderer_from_handle(renderer);
	if (!renderer_ptr) {
		return stats;
	}
	return renderer_ptr->stats;
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
