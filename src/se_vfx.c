// Syphax-Engine - Ougi Washi

#include "se_vfx.h"

#include "se_camera.h"
#include "se_framebuffer.h"
#include "se_model.h"
#include "se_graphics.h"
#include "se_texture.h"
#include "se_window.h"
#include "render/se_gl.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SE_VFX_2D_DEFAULT_VERTEX_SHADER_PATH SE_RESOURCE_PUBLIC("shaders/vfx_2d_vert.glsl")
#define SE_VFX_2D_DEFAULT_FRAGMENT_SHADER_PATH SE_RESOURCE_PUBLIC("shaders/vfx_2d_frag.glsl")
#define SE_VFX_3D_DEFAULT_VERTEX_SHADER_PATH SE_RESOURCE_PUBLIC("shaders/vfx_3d_billboard_vert.glsl")
#define SE_VFX_3D_DEFAULT_FRAGMENT_SHADER_PATH SE_RESOURCE_PUBLIC("shaders/vfx_3d_billboard_frag.glsl")
#define SE_VFX_3D_MESH_DEFAULT_VERTEX_SHADER_PATH SE_RESOURCE_PUBLIC("shaders/vfx_3d_mesh_vert.glsl")
#define SE_VFX_3D_MESH_DEFAULT_FRAGMENT_SHADER_PATH SE_RESOURCE_PUBLIC("shaders/vfx_3d_mesh_frag.glsl")

#define SE_VFX_MAX_DT 0.25f
#define SE_VFX_MIN_LIFETIME 0.0001f

typedef enum {
	SE_VFX_TRACK_VALUE_NONE = 0,
	SE_VFX_TRACK_VALUE_FLOAT,
	SE_VFX_TRACK_VALUE_VEC2,
	SE_VFX_TRACK_VALUE_VEC3,
	SE_VFX_TRACK_VALUE_VEC4
} se_vfx_track_value_type;

typedef struct {
	b8 enabled : 1;
	se_curve_mode mode;
	se_vfx_track_value_type value_type;
	se_curve_float_keys float_keys;
	se_curve_vec2_keys vec2_keys;
	se_curve_vec3_keys vec3_keys;
	se_curve_vec4_keys vec4_keys;
} se_vfx_track;

typedef struct {
	c8 name[SE_MAX_NAME_LENGTH];
	se_vfx_track track;
} se_vfx_uniform_track;
typedef s_array(se_vfx_uniform_track, se_vfx_uniform_tracks);

typedef s_array(i32, se_vfx_indices);
typedef s_array(se_vfx_particle_2d, se_vfx_particles_2d);
typedef s_array(se_vfx_particle_3d, se_vfx_particles_3d);
typedef s_array(s_mat3, se_vfx_render_transforms_2d);
typedef s_array(s_mat4, se_vfx_render_buffers_2d);
typedef s_array(s_mat4, se_vfx_render_instances_3d);
typedef s_array(s_vec4, se_vfx_render_colors_3d);

typedef struct {
	u32 vao;
	u32 transform_vbo;
	u32 color_vbo;
	u32 index_count;
	u32 mesh_index;
} se_vfx_mesh_draw;
typedef s_array(se_vfx_mesh_draw, se_vfx_mesh_draws);

typedef struct se_vfx_emitter_2d {
	se_vfx_emitter_2d_params params;
	se_vfx_particles_2d particles;
	se_vfx_indices free_slots;
	se_vfx_render_transforms_2d render_transforms;
	se_vfx_render_buffers_2d render_buffers;
	se_quad quad;
	se_shader_handle shader;
	c8 vertex_shader_path[SE_MAX_PATH_LENGTH];
	c8 fragment_shader_path[SE_MAX_PATH_LENGTH];
	se_vfx_track builtin_tracks[SE_VFX_TARGET_COUNT];
	se_vfx_uniform_tracks uniform_tracks;
	f32 spawn_accum;
	f32 uniform_time;
	f32 last_tick_dt;
	u32 rng_state;
	b8 running : 1;
} se_vfx_emitter_2d;
typedef s_array(se_vfx_emitter_2d, se_vfx_emitters_2d);

typedef struct se_vfx_emitter_3d {
	se_vfx_emitter_3d_params params;
	se_vfx_particles_3d particles;
	se_vfx_indices free_slots;
	se_vfx_render_instances_3d render_instances;
	se_vfx_render_colors_3d render_colors;
	se_vfx_mesh_draws mesh_draws;
	se_quad quad;
	se_shader_handle shader;
	c8 vertex_shader_path[SE_MAX_PATH_LENGTH];
	c8 fragment_shader_path[SE_MAX_PATH_LENGTH];
	se_vfx_track builtin_tracks[SE_VFX_TARGET_COUNT];
	se_vfx_uniform_tracks uniform_tracks;
	f32 spawn_accum;
	f32 uniform_time;
	f32 last_tick_dt;
	u32 rng_state;
	b8 use_mesh : 1;
	b8 running : 1;
} se_vfx_emitter_3d;
typedef s_array(se_vfx_emitter_3d, se_vfx_emitters_3d);

struct se_vfx_2d {
	se_vfx_2d_params params;
	se_vfx_emitters_2d emitters;
	se_framebuffer_handle output;
	s_vec2 output_size;
	b8 output_initialized : 1;
	u64 spawned_particles;
	u64 expired_particles;
};

struct se_vfx_3d {
	se_vfx_3d_params params;
	se_vfx_emitters_3d emitters;
	se_framebuffer_handle output;
	s_vec2 output_size;
	b8 output_initialized : 1;
	u64 spawned_particles;
	u64 expired_particles;
};

static struct se_vfx_2d* se_vfx_2d_from_handle(se_context* ctx, const se_vfx_2d_handle handle) {
	return s_array_get(&ctx->vfx_2ds, handle);
}

static struct se_vfx_3d* se_vfx_3d_from_handle(se_context* ctx, const se_vfx_3d_handle handle) {
	return s_array_get(&ctx->vfx_3ds, handle);
}

static b8 se_vfx_texture_exists(se_context* ctx, const se_texture_handle texture) {
	return ctx && texture != S_HANDLE_NULL && s_array_get(&ctx->textures, texture) != NULL;
}

static b8 se_vfx_framebuffer_exists(se_context* ctx, const se_framebuffer_handle framebuffer) {
	return ctx && framebuffer != S_HANDLE_NULL && s_array_get(&ctx->framebuffers, framebuffer) != NULL;
}

static b8 se_vfx_window_exists(se_context* ctx, const se_window_handle window) {
	return ctx && window != S_HANDLE_NULL && s_array_get(&ctx->windows, window) != NULL;
}

static b8 se_vfx_camera_exists(se_context* ctx, const se_camera_handle camera) {
	return ctx && camera != S_HANDLE_NULL && s_array_get(&ctx->cameras, camera) != NULL;
}

static b8 se_vfx_model_exists(se_context* ctx, const se_model_handle model) {
	return ctx && model != S_HANDLE_NULL && s_array_get(&ctx->models, model) != NULL;
}

static b8 se_vfx_shader_pair_valid(const c8* vertex_shader_path, const c8* fragment_shader_path) {
	const b8 has_vertex = vertex_shader_path && vertex_shader_path[0] != '\0';
	const b8 has_fragment = fragment_shader_path && fragment_shader_path[0] != '\0';
	return has_vertex == has_fragment;
}

static b8 se_vfx_shader_path_copy(c8* out_path, const c8* source) {
	if (!out_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	out_path[0] = '\0';
	if (!source || source[0] == '\0') {
		return true;
	}
	const sz len = strlen(source);
	if (len == 0 || len >= SE_MAX_PATH_LENGTH) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	strncpy(out_path, source, SE_MAX_PATH_LENGTH - 1u);
	out_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
	return true;
}

static void se_vfx_emitter_2d_resolve_shader_paths(const se_vfx_emitter_2d* emitter, const c8** out_vertex_path, const c8** out_fragment_path) {
	if (!emitter || !out_vertex_path || !out_fragment_path) {
		return;
	}
	*out_vertex_path = emitter->vertex_shader_path[0] != '\0' ? emitter->vertex_shader_path : SE_VFX_2D_DEFAULT_VERTEX_SHADER_PATH;
	*out_fragment_path = emitter->fragment_shader_path[0] != '\0' ? emitter->fragment_shader_path : SE_VFX_2D_DEFAULT_FRAGMENT_SHADER_PATH;
}

static void se_vfx_emitter_3d_resolve_shader_paths(const se_vfx_emitter_3d* emitter, const b8 mesh_mode, const c8** out_vertex_path, const c8** out_fragment_path) {
	if (!emitter || !out_vertex_path || !out_fragment_path) {
		return;
	}
	if (emitter->vertex_shader_path[0] != '\0' && emitter->fragment_shader_path[0] != '\0') {
		*out_vertex_path = emitter->vertex_shader_path;
		*out_fragment_path = emitter->fragment_shader_path;
		return;
	}
	if (mesh_mode) {
		*out_vertex_path = SE_VFX_3D_MESH_DEFAULT_VERTEX_SHADER_PATH;
		*out_fragment_path = SE_VFX_3D_MESH_DEFAULT_FRAGMENT_SHADER_PATH;
		return;
	}
	*out_vertex_path = SE_VFX_3D_DEFAULT_VERTEX_SHADER_PATH;
	*out_fragment_path = SE_VFX_3D_DEFAULT_FRAGMENT_SHADER_PATH;
}

static void se_vfx_emitter_2d_sync_shader_param_ptrs(se_vfx_emitter_2d* emitter) {
	if (!emitter) {
		return;
	}
	emitter->params.vertex_shader_path = emitter->vertex_shader_path[0] != '\0' ? emitter->vertex_shader_path : NULL;
	emitter->params.fragment_shader_path = emitter->fragment_shader_path[0] != '\0' ? emitter->fragment_shader_path : NULL;
}

static void se_vfx_emitter_3d_sync_shader_param_ptrs(se_vfx_emitter_3d* emitter) {
	if (!emitter) {
		return;
	}
	emitter->params.vertex_shader_path = emitter->vertex_shader_path[0] != '\0' ? emitter->vertex_shader_path : NULL;
	emitter->params.fragment_shader_path = emitter->fragment_shader_path[0] != '\0' ? emitter->fragment_shader_path : NULL;
}

typedef struct {
	GLboolean blend_enabled;
	GLboolean depth_enabled;
	GLboolean cull_enabled;
	GLboolean depth_mask;
	GLint depth_func;
	GLint blend_src_rgb;
	GLint blend_dst_rgb;
	GLint blend_src_alpha;
	GLint blend_dst_alpha;
	GLint blend_eq_rgb;
	GLint blend_eq_alpha;
	GLint viewport[4];
} se_vfx_gl_state;

static void se_vfx_capture_gl_state(se_vfx_gl_state* state) {
	if (!state) {
		return;
	}
	memset(state, 0, sizeof(*state));
	state->blend_enabled = glIsEnabled(GL_BLEND);
	state->depth_enabled = glIsEnabled(GL_DEPTH_TEST);
	state->cull_enabled = glIsEnabled(GL_CULL_FACE);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &state->depth_mask);
	glGetIntegerv(GL_DEPTH_FUNC, &state->depth_func);
	glGetIntegerv(GL_BLEND_SRC_RGB, &state->blend_src_rgb);
	glGetIntegerv(GL_BLEND_DST_RGB, &state->blend_dst_rgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &state->blend_src_alpha);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &state->blend_dst_alpha);
	glGetIntegerv(GL_BLEND_EQUATION_RGB, &state->blend_eq_rgb);
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &state->blend_eq_alpha);
	glGetIntegerv(GL_VIEWPORT, state->viewport);
}

static void se_vfx_restore_gl_state(const se_vfx_gl_state* state) {
	if (!state) {
		return;
	}
	glViewport(state->viewport[0], state->viewport[1], state->viewport[2], state->viewport[3]);
	glDepthMask(state->depth_mask);
	glDepthFunc(state->depth_func);
	glBlendEquation(state->blend_eq_rgb);
	glBlendFunc(state->blend_src_rgb, state->blend_dst_rgb);
	if (state->blend_enabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
	if (state->cull_enabled) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if (state->depth_enabled) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
}

static f32 se_vfx_clamp01(const f32 value) {
	return s_max(0.0f, s_min(1.0f, value));
}

static f32 se_vfx_sanitize_dt(const f32 dt) {
	if (!isfinite(dt) || dt <= 0.0f) {
		return 0.0f;
	}
	return s_min(dt, SE_VFX_MAX_DT);
}

static void se_vfx_track_init(se_vfx_track* track) {
	if (!track) {
		return;
	}
	memset(track, 0, sizeof(*track));
	track->mode = SE_CURVE_LINEAR;
	track->value_type = SE_VFX_TRACK_VALUE_NONE;
	se_curve_float_init(&track->float_keys);
	se_curve_vec2_init(&track->vec2_keys);
	se_curve_vec3_init(&track->vec3_keys);
	se_curve_vec4_init(&track->vec4_keys);
}

static void se_vfx_track_clear_values(se_vfx_track* track) {
	if (!track) {
		return;
	}
	se_curve_float_clear(&track->float_keys);
	se_curve_vec2_clear(&track->vec2_keys);
	se_curve_vec3_clear(&track->vec3_keys);
	se_curve_vec4_clear(&track->vec4_keys);
}

static void se_vfx_track_reset(se_vfx_track* track) {
	if (!track) {
		return;
	}
	se_curve_float_reset(&track->float_keys);
	se_curve_vec2_reset(&track->vec2_keys);
	se_curve_vec3_reset(&track->vec3_keys);
	se_curve_vec4_reset(&track->vec4_keys);
	track->enabled = false;
	track->mode = SE_CURVE_LINEAR;
	track->value_type = SE_VFX_TRACK_VALUE_NONE;
}

static b8 se_vfx_track_prepare(se_vfx_track* track, const se_vfx_track_value_type value_type, const se_curve_mode mode) {
	if (!track || value_type == SE_VFX_TRACK_VALUE_NONE) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!track->enabled) {
		track->enabled = true;
		track->value_type = value_type;
		track->mode = mode;
		return true;
	}
	if (track->value_type != value_type) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	track->mode = mode;
	return true;
}

static b8 se_vfx_track_eval_float(const se_vfx_track* track, const f32 t, f32* out_value) {
	if (!track || !out_value || !track->enabled || track->value_type != SE_VFX_TRACK_VALUE_FLOAT) {
		return false;
	}
	return se_curve_float_eval(&track->float_keys, track->mode, t, out_value);
}

static b8 se_vfx_track_eval_vec2(const se_vfx_track* track, const f32 t, s_vec2* out_value) {
	if (!track || !out_value || !track->enabled || track->value_type != SE_VFX_TRACK_VALUE_VEC2) {
		return false;
	}
	return se_curve_vec2_eval(&track->vec2_keys, track->mode, t, out_value);
}

static b8 se_vfx_track_eval_vec3(const se_vfx_track* track, const f32 t, s_vec3* out_value) {
	if (!track || !out_value || !track->enabled || track->value_type != SE_VFX_TRACK_VALUE_VEC3) {
		return false;
	}
	return se_curve_vec3_eval(&track->vec3_keys, track->mode, t, out_value);
}

static b8 se_vfx_track_eval_vec4(const se_vfx_track* track, const f32 t, s_vec4* out_value) {
	if (!track || !out_value || !track->enabled || track->value_type != SE_VFX_TRACK_VALUE_VEC4) {
		return false;
	}
	return se_curve_vec4_eval(&track->vec4_keys, track->mode, t, out_value);
}

static se_vfx_uniform_track* se_vfx_uniform_track_find(se_vfx_uniform_tracks* tracks, const c8* name) {
	if (!tracks || !name) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(tracks); ++i) {
		se_vfx_uniform_track* track = s_array_get(tracks, s_array_handle(tracks, (u32)i));
		if (track && strncmp(track->name, name, sizeof(track->name)) == 0) {
			return track;
		}
	}
	return NULL;
}

static se_vfx_uniform_track* se_vfx_uniform_track_get_or_create(se_vfx_uniform_tracks* tracks, const c8* name) {
	if (!tracks || !name || name[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_vfx_uniform_track* found = se_vfx_uniform_track_find(tracks, name);
	if (found) {
		return found;
	}
	s_handle handle = s_array_increment(tracks);
	se_vfx_uniform_track* track = s_array_get(tracks, handle);
	if (!track) {
		s_array_remove(tracks, handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(track, 0, sizeof(*track));
	strncpy(track->name, name, sizeof(track->name) - 1);
	se_vfx_track_init(&track->track);
	return track;
}

static b8 se_vfx_uniform_track_remove(se_vfx_uniform_tracks* tracks, const c8* name) {
	if (!tracks || !name || name[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(tracks); ++i) {
		s_handle handle = s_array_handle(tracks, (u32)i);
		se_vfx_uniform_track* track = s_array_get(tracks, handle);
		if (!track || strncmp(track->name, name, sizeof(track->name)) != 0) {
			continue;
		}
		se_vfx_track_reset(&track->track);
		s_array_remove(tracks, handle);
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	se_set_last_error(SE_RESULT_NOT_FOUND);
	return false;
}

static void se_vfx_uniform_tracks_clear_all(se_vfx_uniform_tracks* tracks) {
	if (!tracks) {
		return;
	}
	while (s_array_get_size(tracks) > 0) {
		s_handle handle = s_array_handle(tracks, (u32)(s_array_get_size(tracks) - 1));
		se_vfx_uniform_track* track = s_array_get(tracks, handle);
		if (track) {
			se_vfx_track_reset(&track->track);
		}
		s_array_remove(tracks, handle);
	}
}

static b8 se_vfx_builtin_accepts_type_2d(const se_vfx_builtin_target target, const se_vfx_track_value_type value_type) {
	switch (target) {
		case SE_VFX_TARGET_VELOCITY:
			return value_type == SE_VFX_TRACK_VALUE_VEC2;
		case SE_VFX_TARGET_COLOR:
			return value_type == SE_VFX_TRACK_VALUE_VEC4;
		case SE_VFX_TARGET_SIZE:
			return value_type == SE_VFX_TRACK_VALUE_FLOAT;
		case SE_VFX_TARGET_ROTATION:
			return value_type == SE_VFX_TRACK_VALUE_FLOAT;
		default:
			return false;
	}
}

static b8 se_vfx_builtin_accepts_type_3d(const se_vfx_builtin_target target, const se_vfx_track_value_type value_type) {
	switch (target) {
		case SE_VFX_TARGET_VELOCITY:
			return value_type == SE_VFX_TRACK_VALUE_VEC3;
		case SE_VFX_TARGET_COLOR:
			return value_type == SE_VFX_TRACK_VALUE_VEC4;
		case SE_VFX_TARGET_SIZE:
			return value_type == SE_VFX_TRACK_VALUE_FLOAT;
		case SE_VFX_TARGET_ROTATION:
			return value_type == SE_VFX_TRACK_VALUE_FLOAT;
		default:
			return false;
	}
}

static b8 se_vfx_pop_free_slot(se_vfx_indices* free_slots, i32* out_index) {
	if (!free_slots || !out_index || s_array_get_size(free_slots) == 0) {
		return false;
	}
	s_handle handle = s_array_handle(free_slots, (u32)(s_array_get_size(free_slots) - 1));
	i32* index = s_array_get(free_slots, handle);
	if (!index) {
		s_array_remove(free_slots, handle);
		return false;
	}
	*out_index = *index;
	s_array_remove(free_slots, handle);
	return true;
}

static void se_vfx_push_free_slot(se_vfx_indices* free_slots, const i32 index) {
	if (!free_slots) {
		return;
	}
	s_array_add(free_slots, index);
}

static u32 se_vfx_seed_from_handle(const s_handle handle) {
	u32 seed = (u32)s_handle_slot(handle);
	seed ^= (u32)(seed << 13);
	seed ^= 0x9E3779B9u;
	return seed == 0u ? 1u : seed;
}

static f32 se_vfx_random01(u32* state) {
	if (!state) {
		return 0.0f;
	}
	*state = (*state * 1664525u) + 1013904223u;
	return (f32)((*state >> 8) & 0x00FFFFFFu) / 16777215.0f;
}

static u32 se_vfx_texture_id(se_context* ctx, const se_texture_handle texture_handle) {
	if (!se_vfx_texture_exists(ctx, texture_handle)) {
		return 0u;
	}
	se_texture* texture = s_array_get(&ctx->textures, texture_handle);
	return texture ? texture->id : 0u;
}

static void se_vfx_apply_blend_mode(const se_vfx_blend_mode blend_mode) {
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	if (blend_mode == SE_VFX_BLEND_ADDITIVE) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		return;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void se_vfx_restore_alpha_blend(void) {
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void se_vfx_render_buffers_2d_clear(se_vfx_render_buffers_2d* buffers) {
	if (!buffers) {
		return;
	}
	while (s_array_get_size(buffers) > 0) {
		s_array_remove(buffers, s_array_handle(buffers, (u32)(s_array_get_size(buffers) - 1)));
	}
}

static void se_vfx_render_transforms_2d_clear(se_vfx_render_transforms_2d* transforms) {
	if (!transforms) {
		return;
	}
	while (s_array_get_size(transforms) > 0) {
		s_array_remove(transforms, s_array_handle(transforms, (u32)(s_array_get_size(transforms) - 1)));
	}
}

static void se_vfx_render_instances_3d_clear(se_vfx_render_instances_3d* instances) {
	if (!instances) {
		return;
	}
	while (s_array_get_size(instances) > 0) {
		s_array_remove(instances, s_array_handle(instances, (u32)(s_array_get_size(instances) - 1)));
	}
}

static void se_vfx_render_colors_3d_clear(se_vfx_render_colors_3d* colors) {
	if (!colors) {
		return;
	}
	while (s_array_get_size(colors) > 0) {
		s_array_remove(colors, s_array_handle(colors, (u32)(s_array_get_size(colors) - 1)));
	}
}

static void se_vfx_mesh_draw_destroy(se_vfx_mesh_draw* draw) {
	if (!draw) {
		return;
	}
	if (draw->color_vbo != 0u) {
		glDeleteBuffers(1, &draw->color_vbo);
		draw->color_vbo = 0u;
	}
	if (draw->transform_vbo != 0u) {
		glDeleteBuffers(1, &draw->transform_vbo);
		draw->transform_vbo = 0u;
	}
	if (draw->vao != 0u) {
		glDeleteVertexArrays(1, &draw->vao);
		draw->vao = 0u;
	}
	draw->index_count = 0u;
	draw->mesh_index = 0u;
}

static void se_vfx_mesh_draws_clear(se_vfx_mesh_draws* draws) {
	if (!draws) {
		return;
	}
	while (s_array_get_size(draws) > 0) {
		s_handle handle = s_array_handle(draws, (u32)(s_array_get_size(draws) - 1));
		se_vfx_mesh_draw* draw = s_array_get(draws, handle);
		se_vfx_mesh_draw_destroy(draw);
		s_array_remove(draws, handle);
	}
}

static b8 se_vfx_emitter_3d_create_mesh_draws(se_context* ctx, se_vfx_emitter_3d* emitter) {
	if (!ctx || !emitter || !se_vfx_model_exists(ctx, emitter->params.model)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_model* model = s_array_get(&ctx->models, emitter->params.model);
	if (!model) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const sz mesh_count = s_array_get_size(&model->meshes);
	if (mesh_count == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	se_vfx_mesh_draws_clear(&emitter->mesh_draws);
	s_array_reserve(&emitter->mesh_draws, mesh_count);
	const u32 max_instances = s_max(1u, emitter->params.max_particles);
	for (sz mesh_index = 0; mesh_index < mesh_count; ++mesh_index) {
		se_mesh* mesh = s_array_get(&model->meshes, s_array_handle(&model->meshes, (u32)mesh_index));
		if (!mesh || !se_mesh_has_gpu_data(mesh) || mesh->gpu.index_count == 0u) {
			continue;
		}
		s_handle draw_handle = s_array_increment(&emitter->mesh_draws);
		se_vfx_mesh_draw* draw = s_array_get(&emitter->mesh_draws, draw_handle);
		if (!draw) {
			s_array_remove(&emitter->mesh_draws, draw_handle);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
		memset(draw, 0, sizeof(*draw));
		draw->index_count = mesh->gpu.index_count;
		draw->mesh_index = (u32)mesh_index;

		glGenVertexArrays(1, &draw->vao);
		glBindVertexArray(draw->vao);

		glBindBuffer(GL_ARRAY_BUFFER, mesh->gpu.vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpu.ebo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void*)offsetof(se_vertex_3d, position));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void*)offsetof(se_vertex_3d, normal));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(se_vertex_3d), (const void*)offsetof(se_vertex_3d, uv));

		glGenBuffers(1, &draw->transform_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, draw->transform_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(s_mat4) * max_instances, NULL, GL_DYNAMIC_DRAW);
		for (u32 col = 0u; col < 4u; ++col) {
			const u32 attrib = 3u + col;
			glEnableVertexAttribArray(attrib);
			glVertexAttribPointer(attrib, 4, GL_FLOAT, GL_FALSE, sizeof(s_mat4), (const void*)(sizeof(s_vec4) * col));
			glVertexAttribDivisor(attrib, 1);
		}

		glGenBuffers(1, &draw->color_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, draw->color_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(s_vec4) * max_instances, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(s_vec4), (const void*)0);
		glVertexAttribDivisor(7, 1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	if (s_array_get_size(&emitter->mesh_draws) == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void se_vfx_mesh_draw_upload(const se_vfx_mesh_draw* draw, const se_vfx_render_instances_3d* transforms, const se_vfx_render_colors_3d* colors, const sz instance_count) {
	if (!draw || instance_count == 0) {
		return;
	}
	const s_mat4* transform_data = s_array_get_data((se_vfx_render_instances_3d*)transforms);
	const s_vec4* color_data = s_array_get_data((se_vfx_render_colors_3d*)colors);
	glBindBuffer(GL_ARRAY_BUFFER, draw->transform_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(s_mat4) * instance_count, transform_data);
	glBindBuffer(GL_ARRAY_BUFFER, draw->color_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(s_vec4) * instance_count, color_data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static b8 se_vfx_emitter_2d_build_render_data(se_vfx_emitter_2d* emitter, sz* out_alive_count) {
	if (!emitter || !out_alive_count) {
		return false;
	}
	se_vfx_render_transforms_2d_clear(&emitter->render_transforms);
	se_vfx_render_buffers_2d_clear(&emitter->render_buffers);

	sz alive_count = 0;
	for (sz i = 0; i < s_array_get_size(&emitter->particles); ++i) {
		se_vfx_particle_2d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)i));
		if (!particle || !particle->alive) {
			continue;
		}
		s_mat3 transform = s_mat3_identity;
		s_mat3_set_translation(&transform, &particle->position);
		s_mat3_set_rotation(&transform, particle->rotation);
		s_vec2 scale = s_vec2(particle->size, particle->size);
		s_mat3_set_scale(&transform, &scale);
		s_mat4 buffer = s_mat4_identity;
		buffer.m[0][0] = particle->color.x;
		buffer.m[0][1] = particle->color.y;
		buffer.m[0][2] = particle->color.z;
		buffer.m[0][3] = particle->color.w;
		s_array_add(&emitter->render_transforms, transform);
		s_array_add(&emitter->render_buffers, buffer);
		alive_count++;
	}
	emitter->quad.instance_buffers_dirty = true;
	*out_alive_count = alive_count;
	return true;
}

static b8 se_vfx_emitter_3d_build_billboard_render_data(se_vfx_emitter_3d* emitter, sz* out_alive_count) {
	if (!emitter || !out_alive_count) {
		return false;
	}
	se_vfx_render_instances_3d_clear(&emitter->render_instances);
	sz alive_count = 0;
	for (sz i = 0; i < s_array_get_size(&emitter->particles); ++i) {
		se_vfx_particle_3d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)i));
		if (!particle || !particle->alive) {
			continue;
		}
		s_mat4 instance = s_mat4_identity;
		instance.m[0][0] = particle->position.x;
		instance.m[0][1] = particle->position.y;
		instance.m[0][2] = particle->position.z;
		instance.m[0][3] = particle->size;
		instance.m[1][0] = particle->color.x;
		instance.m[1][1] = particle->color.y;
		instance.m[1][2] = particle->color.z;
		instance.m[1][3] = particle->color.w;
		instance.m[2][0] = particle->rotation;
		s_array_add(&emitter->render_instances, instance);
		alive_count++;
	}
	emitter->quad.instance_buffers_dirty = true;
	*out_alive_count = alive_count;
	return true;
}

static b8 se_vfx_emitter_3d_build_mesh_render_data(
	se_vfx_emitter_3d* emitter,
	const s_mat4* view_proj,
	const s_mat4* mesh_matrix,
	sz* out_alive_count) {
	if (!emitter || !view_proj || !mesh_matrix || !out_alive_count) {
		return false;
	}
	se_vfx_render_instances_3d_clear(&emitter->render_instances);
	se_vfx_render_colors_3d_clear(&emitter->render_colors);

	sz alive_count = 0;
	for (sz i = 0; i < s_array_get_size(&emitter->particles); ++i) {
		se_vfx_particle_3d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)i));
		if (!particle || !particle->alive) {
			continue;
		}
		const s_vec3 uniform_scale = s_vec3(particle->size, particle->size, particle->size);
		s_mat4 model_matrix = s_mat4_identity;
		model_matrix = s_mat4_scale(&model_matrix, &uniform_scale);
		model_matrix = s_mat4_rotate_y(&model_matrix, particle->rotation);
		s_mat4_set_translation(&model_matrix, &particle->position);
		model_matrix = s_mat4_mul(&model_matrix, mesh_matrix);
		const s_mat4 mvp = s_mat4_mul(view_proj, &model_matrix);
		s_array_add(&emitter->render_instances, mvp);
		s_array_add(&emitter->render_colors, particle->color);
		alive_count++;
	}
	*out_alive_count = alive_count;
	return true;
}

static void se_vfx_emitter_2d_apply_builtin_tracks(se_vfx_emitter_2d* emitter, se_vfx_particle_2d* particle, const f32 life01) {
	if (!emitter || !particle) {
		return;
	}
	s_vec2 velocity = s_vec2(0.0f, 0.0f);
	s_vec4 color = s_vec4(0.0f, 0.0f, 0.0f, 0.0f);
	f32 scalar = 0.0f;

	if (se_vfx_track_eval_vec2(&emitter->builtin_tracks[SE_VFX_TARGET_VELOCITY], life01, &velocity)) {
		particle->velocity = velocity;
	}
	if (se_vfx_track_eval_vec4(&emitter->builtin_tracks[SE_VFX_TARGET_COLOR], life01, &color)) {
		particle->color = color;
	}
	if (se_vfx_track_eval_float(&emitter->builtin_tracks[SE_VFX_TARGET_SIZE], life01, &scalar)) {
		particle->size = s_max(0.0f, scalar);
	}
	if (se_vfx_track_eval_float(&emitter->builtin_tracks[SE_VFX_TARGET_ROTATION], life01, &scalar)) {
		particle->rotation = scalar;
	}
}

static void se_vfx_emitter_3d_apply_builtin_tracks(se_vfx_emitter_3d* emitter, se_vfx_particle_3d* particle, const f32 life01) {
	if (!emitter || !particle) {
		return;
	}
	s_vec3 velocity = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec4 color = s_vec4(0.0f, 0.0f, 0.0f, 0.0f);
	f32 scalar = 0.0f;

	if (se_vfx_track_eval_vec3(&emitter->builtin_tracks[SE_VFX_TARGET_VELOCITY], life01, &velocity)) {
		particle->velocity = velocity;
	}
	if (se_vfx_track_eval_vec4(&emitter->builtin_tracks[SE_VFX_TARGET_COLOR], life01, &color)) {
		particle->color = color;
	}
	if (se_vfx_track_eval_float(&emitter->builtin_tracks[SE_VFX_TARGET_SIZE], life01, &scalar)) {
		particle->size = s_max(0.0f, scalar);
	}
	if (se_vfx_track_eval_float(&emitter->builtin_tracks[SE_VFX_TARGET_ROTATION], life01, &scalar)) {
		particle->rotation = scalar;
	}
}

static void se_vfx_apply_uniform_tracks(const se_vfx_uniform_tracks* tracks, const se_shader_handle shader, const f32 time01) {
	if (!tracks || shader == S_HANDLE_NULL) {
		return;
	}
	for (sz i = 0; i < s_array_get_size((se_vfx_uniform_tracks*)tracks); ++i) {
		se_vfx_uniform_track* uniform_track = s_array_get((se_vfx_uniform_tracks*)tracks, s_array_handle((se_vfx_uniform_tracks*)tracks, (u32)i));
		if (!uniform_track || uniform_track->name[0] == '\0' || !uniform_track->track.enabled) {
			continue;
		}
		switch (uniform_track->track.value_type) {
			case SE_VFX_TRACK_VALUE_FLOAT: {
				f32 value = 0.0f;
				if (se_vfx_track_eval_float(&uniform_track->track, time01, &value)) {
					se_shader_set_float(shader, uniform_track->name, value);
				}
				break;
			}
			case SE_VFX_TRACK_VALUE_VEC2: {
				s_vec2 value = s_vec2(0.0f, 0.0f);
				if (se_vfx_track_eval_vec2(&uniform_track->track, time01, &value)) {
					se_shader_set_vec2(shader, uniform_track->name, &value);
				}
				break;
			}
			case SE_VFX_TRACK_VALUE_VEC3: {
				s_vec3 value = s_vec3(0.0f, 0.0f, 0.0f);
				if (se_vfx_track_eval_vec3(&uniform_track->track, time01, &value)) {
					se_shader_set_vec3(shader, uniform_track->name, &value);
				}
				break;
			}
			case SE_VFX_TRACK_VALUE_VEC4: {
				s_vec4 value = s_vec4(0.0f, 0.0f, 0.0f, 0.0f);
				if (se_vfx_track_eval_vec4(&uniform_track->track, time01, &value)) {
					se_shader_set_vec4(shader, uniform_track->name, &value);
				}
				break;
			}
			default:
				break;
		}
	}
}

static b8 se_vfx_emitter_2d_spawn_particle(se_vfx_emitter_2d* emitter) {
	if (!emitter) {
		return false;
	}
	i32 particle_index = -1;
	if (!se_vfx_pop_free_slot(&emitter->free_slots, &particle_index)) {
		if (s_array_get_size(&emitter->particles) >= emitter->params.max_particles) {
			return false;
		}
		s_handle handle = s_array_increment(&emitter->particles);
		particle_index = (i32)(s_array_get_size(&emitter->particles) - 1);
		if (!s_array_get(&emitter->particles, handle)) {
			s_array_remove(&emitter->particles, handle);
			return false;
		}
	}
	se_vfx_particle_2d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)particle_index));
	if (!particle) {
		return false;
	}
	memset(particle, 0, sizeof(*particle));
	particle->alive = true;
	particle->position = emitter->params.position;
	particle->velocity = emitter->params.velocity;
	particle->color = emitter->params.color;
	particle->size = s_max(0.0f, emitter->params.size);
	particle->rotation = emitter->params.rotation;
	const f32 min_life = s_max(SE_VFX_MIN_LIFETIME, emitter->params.lifetime_min);
	const f32 max_life = s_max(min_life, emitter->params.lifetime_max);
	particle->lifetime = min_life + (max_life - min_life) * se_vfx_random01(&emitter->rng_state);
	particle->age = 0.0f;
	se_vfx_emitter_2d_apply_builtin_tracks(emitter, particle, 0.0f);
	return true;
}

static b8 se_vfx_emitter_3d_spawn_particle(se_vfx_emitter_3d* emitter) {
	if (!emitter) {
		return false;
	}
	i32 particle_index = -1;
	if (!se_vfx_pop_free_slot(&emitter->free_slots, &particle_index)) {
		if (s_array_get_size(&emitter->particles) >= emitter->params.max_particles) {
			return false;
		}
		s_handle handle = s_array_increment(&emitter->particles);
		particle_index = (i32)(s_array_get_size(&emitter->particles) - 1);
		if (!s_array_get(&emitter->particles, handle)) {
			s_array_remove(&emitter->particles, handle);
			return false;
		}
	}
	se_vfx_particle_3d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)particle_index));
	if (!particle) {
		return false;
	}
	memset(particle, 0, sizeof(*particle));
	particle->alive = true;
	particle->position = emitter->params.position;
	particle->velocity = emitter->params.velocity;
	particle->color = emitter->params.color;
	particle->size = s_max(0.0f, emitter->params.size);
	particle->rotation = emitter->params.rotation;
	const f32 min_life = s_max(SE_VFX_MIN_LIFETIME, emitter->params.lifetime_min);
	const f32 max_life = s_max(min_life, emitter->params.lifetime_max);
	particle->lifetime = min_life + (max_life - min_life) * se_vfx_random01(&emitter->rng_state);
	particle->age = 0.0f;
	se_vfx_emitter_3d_apply_builtin_tracks(emitter, particle, 0.0f);
	return true;
}

static void se_vfx_emitter_2d_tick(se_vfx_emitter_2d* emitter, const f32 dt, u64* out_spawned, u64* out_expired) {
	if (!emitter || dt <= 0.0f) {
		return;
	}
	emitter->last_tick_dt = dt;
	emitter->uniform_time += dt;
	if (emitter->running && emitter->params.spawn_rate > 0.0f) {
		emitter->spawn_accum += emitter->params.spawn_rate * dt;
		u32 spawn_count = (u32)emitter->spawn_accum;
		emitter->spawn_accum -= (f32)spawn_count;
		for (u32 i = 0; i < spawn_count; ++i) {
			if (se_vfx_emitter_2d_spawn_particle(emitter) && out_spawned) {
				(*out_spawned)++;
			}
		}
	}

	for (sz i = 0; i < s_array_get_size(&emitter->particles); ++i) {
		se_vfx_particle_2d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)i));
		if (!particle || !particle->alive) {
			continue;
		}
		particle->age += dt;
		if (particle->age >= particle->lifetime) {
			particle->alive = false;
			se_vfx_push_free_slot(&emitter->free_slots, (i32)i);
			if (out_expired) {
				(*out_expired)++;
			}
			continue;
		}
		const f32 life01 = se_vfx_clamp01(particle->age / s_max(particle->lifetime, SE_VFX_MIN_LIFETIME));
		se_vfx_emitter_2d_apply_builtin_tracks(emitter, particle, life01);
		particle->position = s_vec2_add(&particle->position, &s_vec2_muls(&particle->velocity, dt));
		if (emitter->params.particle_callback) {
			emitter->params.particle_callback(particle, dt, emitter->params.particle_user_data);
		}
	}
}

static void se_vfx_emitter_3d_tick(se_vfx_emitter_3d* emitter, const f32 dt, u64* out_spawned, u64* out_expired) {
	if (!emitter || dt <= 0.0f) {
		return;
	}
	emitter->last_tick_dt = dt;
	emitter->uniform_time += dt;
	if (emitter->running && emitter->params.spawn_rate > 0.0f) {
		emitter->spawn_accum += emitter->params.spawn_rate * dt;
		u32 spawn_count = (u32)emitter->spawn_accum;
		emitter->spawn_accum -= (f32)spawn_count;
		for (u32 i = 0; i < spawn_count; ++i) {
			if (se_vfx_emitter_3d_spawn_particle(emitter) && out_spawned) {
				(*out_spawned)++;
			}
		}
	}

	for (sz i = 0; i < s_array_get_size(&emitter->particles); ++i) {
		se_vfx_particle_3d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)i));
		if (!particle || !particle->alive) {
			continue;
		}
		particle->age += dt;
		if (particle->age >= particle->lifetime) {
			particle->alive = false;
			se_vfx_push_free_slot(&emitter->free_slots, (i32)i);
			if (out_expired) {
				(*out_expired)++;
			}
			continue;
		}
		const f32 life01 = se_vfx_clamp01(particle->age / s_max(particle->lifetime, SE_VFX_MIN_LIFETIME));
		se_vfx_emitter_3d_apply_builtin_tracks(emitter, particle, life01);
		particle->position = s_vec3_add(&particle->position, &s_vec3_muls(&particle->velocity, dt));
		if (emitter->params.particle_callback) {
			emitter->params.particle_callback(particle, dt, emitter->params.particle_user_data);
		}
	}
}

static void se_vfx_emitter_2d_destroy(se_context* ctx, se_vfx_emitter_2d* emitter) {
	if (!emitter) {
		return;
	}
	for (u32 target = 0; target < SE_VFX_TARGET_COUNT; ++target) {
		se_vfx_track_reset(&emitter->builtin_tracks[target]);
	}
	se_vfx_uniform_tracks_clear_all(&emitter->uniform_tracks);
	s_array_clear(&emitter->uniform_tracks);
	se_quad_destroy(&emitter->quad);
	if (ctx && emitter->shader != S_HANDLE_NULL && s_array_get(&ctx->shaders, emitter->shader) != NULL) {
		se_shader_destroy(emitter->shader);
		emitter->shader = S_HANDLE_NULL;
	}
	s_array_clear(&emitter->particles);
	s_array_clear(&emitter->free_slots);
	s_array_clear(&emitter->render_transforms);
	s_array_clear(&emitter->render_buffers);
	memset(emitter, 0, sizeof(*emitter));
}

static b8 se_vfx_emitter_2d_reload_shader(se_context* ctx, se_vfx_emitter_2d* emitter) {
	if (!ctx || !emitter) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const c8* vertex_path = NULL;
	const c8* fragment_path = NULL;
	se_vfx_emitter_2d_resolve_shader_paths(emitter, &vertex_path, &fragment_path);
	const se_shader_handle new_shader = se_shader_load(vertex_path, fragment_path);
	if (new_shader == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	if (emitter->shader != S_HANDLE_NULL && s_array_get(&ctx->shaders, emitter->shader) != NULL) {
		se_shader_destroy(emitter->shader);
	}
	emitter->shader = new_shader;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void se_vfx_emitter_3d_reset_renderer(se_context* ctx, se_vfx_emitter_3d* emitter) {
	if (!emitter) {
		return;
	}
	se_quad_destroy(&emitter->quad);
	se_vfx_mesh_draws_clear(&emitter->mesh_draws);
	emitter->use_mesh = false;
	if (ctx && emitter->shader != S_HANDLE_NULL && s_array_get(&ctx->shaders, emitter->shader) != NULL) {
		se_shader_destroy(emitter->shader);
	}
	emitter->shader = S_HANDLE_NULL;
}

static b8 se_vfx_emitter_3d_setup_renderer(se_context* ctx, se_vfx_emitter_3d* emitter) {
	if (!ctx || !emitter) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_vfx_emitter_3d_reset_renderer(ctx, emitter);
	const b8 mesh_mode = emitter->params.model != S_HANDLE_NULL;
	const c8* vertex_path = NULL;
	const c8* fragment_path = NULL;
	se_vfx_emitter_3d_resolve_shader_paths(emitter, mesh_mode, &vertex_path, &fragment_path);
	if (mesh_mode) {
		if (!se_vfx_model_exists(ctx, emitter->params.model)) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return false;
		}
		emitter->shader = se_shader_load(vertex_path, fragment_path);
		if (emitter->shader == S_HANDLE_NULL) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
		if (!se_vfx_emitter_3d_create_mesh_draws(ctx, emitter)) {
			se_vfx_emitter_3d_reset_renderer(ctx, emitter);
			if (se_get_last_error() == SE_RESULT_OK) {
				se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			}
			return false;
		}
		emitter->use_mesh = true;
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	emitter->shader = se_shader_load(vertex_path, fragment_path);
	if (emitter->shader == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	se_quad_3d_create(&emitter->quad);
	if (emitter->quad.vao == 0u) {
		se_vfx_emitter_3d_reset_renderer(ctx, emitter);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	se_quad_2d_add_instance_buffer(&emitter->quad, s_array_get_data(&emitter->render_instances), emitter->params.max_particles);
	emitter->use_mesh = false;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void se_vfx_emitter_3d_destroy(se_context* ctx, se_vfx_emitter_3d* emitter) {
	if (!emitter) {
		return;
	}
	for (u32 target = 0; target < SE_VFX_TARGET_COUNT; ++target) {
		se_vfx_track_reset(&emitter->builtin_tracks[target]);
	}
	se_vfx_uniform_tracks_clear_all(&emitter->uniform_tracks);
	s_array_clear(&emitter->uniform_tracks);
	se_vfx_emitter_3d_reset_renderer(ctx, emitter);
	s_array_clear(&emitter->particles);
	s_array_clear(&emitter->free_slots);
	s_array_clear(&emitter->render_instances);
	s_array_clear(&emitter->render_colors);
	s_array_clear(&emitter->mesh_draws);
	memset(emitter, 0, sizeof(*emitter));
}

static b8 se_vfx_2d_add_builtin_key_internal(
	se_vfx_emitter_2d* emitter,
	const se_vfx_builtin_target target,
	const se_vfx_track_value_type value_type,
	const se_curve_mode mode,
	const f32 t,
	const void* value) {
	if (!emitter || !value || target >= SE_VFX_TARGET_COUNT || !se_vfx_builtin_accepts_type_2d(target, value_type)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_vfx_track* track = &emitter->builtin_tracks[target];
	if (!se_vfx_track_prepare(track, value_type, mode)) {
		return false;
	}
	switch (value_type) {
		case SE_VFX_TRACK_VALUE_FLOAT:
			return se_curve_float_add_key(&track->float_keys, t, *(const f32*)value);
		case SE_VFX_TRACK_VALUE_VEC2:
			return se_curve_vec2_add_key(&track->vec2_keys, t, (const s_vec2*)value);
		case SE_VFX_TRACK_VALUE_VEC3:
			return se_curve_vec3_add_key(&track->vec3_keys, t, (const s_vec3*)value);
		case SE_VFX_TRACK_VALUE_VEC4:
			return se_curve_vec4_add_key(&track->vec4_keys, t, (const s_vec4*)value);
		default:
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
	}
}

static b8 se_vfx_3d_add_builtin_key_internal(
	se_vfx_emitter_3d* emitter,
	const se_vfx_builtin_target target,
	const se_vfx_track_value_type value_type,
	const se_curve_mode mode,
	const f32 t,
	const void* value) {
	if (!emitter || !value || target >= SE_VFX_TARGET_COUNT || !se_vfx_builtin_accepts_type_3d(target, value_type)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_vfx_track* track = &emitter->builtin_tracks[target];
	if (!se_vfx_track_prepare(track, value_type, mode)) {
		return false;
	}
	switch (value_type) {
		case SE_VFX_TRACK_VALUE_FLOAT:
			return se_curve_float_add_key(&track->float_keys, t, *(const f32*)value);
		case SE_VFX_TRACK_VALUE_VEC2:
			return se_curve_vec2_add_key(&track->vec2_keys, t, (const s_vec2*)value);
		case SE_VFX_TRACK_VALUE_VEC3:
			return se_curve_vec3_add_key(&track->vec3_keys, t, (const s_vec3*)value);
		case SE_VFX_TRACK_VALUE_VEC4:
			return se_curve_vec4_add_key(&track->vec4_keys, t, (const s_vec4*)value);
		default:
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
	}
}

static b8 se_vfx_add_uniform_key_internal(
	se_vfx_uniform_tracks* tracks,
	const c8* uniform_name,
	const se_vfx_track_value_type value_type,
	const se_curve_mode mode,
	const f32 t,
	const void* value) {
	if (!tracks || !uniform_name || !value || uniform_name[0] == '\0') {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_vfx_uniform_track* uniform_track = se_vfx_uniform_track_get_or_create(tracks, uniform_name);
	if (!uniform_track) {
		return false;
	}
	if (!se_vfx_track_prepare(&uniform_track->track, value_type, mode)) {
		return false;
	}
	switch (value_type) {
		case SE_VFX_TRACK_VALUE_FLOAT:
			return se_curve_float_add_key(&uniform_track->track.float_keys, t, *(const f32*)value);
		case SE_VFX_TRACK_VALUE_VEC2:
			return se_curve_vec2_add_key(&uniform_track->track.vec2_keys, t, (const s_vec2*)value);
		case SE_VFX_TRACK_VALUE_VEC3:
			return se_curve_vec3_add_key(&uniform_track->track.vec3_keys, t, (const s_vec3*)value);
		case SE_VFX_TRACK_VALUE_VEC4:
			return se_curve_vec4_add_key(&uniform_track->track.vec4_keys, t, (const s_vec4*)value);
		default:
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
	}
}

static b8 se_vfx_get_window_output_size(const se_window_handle window, s_vec2* out_size) {
	if (window == S_HANDLE_NULL || !out_size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	u32 width = 0;
	u32 height = 0;
	se_window_get_framebuffer_size(window, &width, &height);
	if (width == 0 || height == 0) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	*out_size = s_vec2((f32)width, (f32)height);
	return true;
}

static b8 se_vfx_size_is_valid(const s_vec2* size) {
	return size && size->x > 0.0f && size->y > 0.0f;
}

static b8 se_vfx_sizes_differ(const s_vec2* a, const s_vec2* b) {
	if (!a || !b) {
		return true;
	}
	return fabsf(a->x - b->x) > 0.5f || fabsf(a->y - b->y) > 0.5f;
}

static b8 se_vfx_resolve_output_size(const s_vec2* configured_size, const b8 auto_resize, const se_window_handle window, s_vec2* out_size) {
	if (!out_size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (auto_resize || !se_vfx_size_is_valid(configured_size)) {
		return se_vfx_get_window_output_size(window, out_size);
	}
	*out_size = *configured_size;
	return true;
}

static b8 se_vfx_ensure_output_2d(struct se_vfx_2d* vfx, const se_window_handle window) {
	if (!vfx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	s_vec2 desired_size = s_vec2(0.0f, 0.0f);
	if (!se_vfx_resolve_output_size(&vfx->params.render_size, vfx->params.auto_resize_with_window, window, &desired_size)) {
		return false;
	}
	if (!se_vfx_size_is_valid(&desired_size)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!vfx->output_initialized || !se_vfx_framebuffer_exists(se_current_context(), vfx->output)) {
		vfx->output = se_framebuffer_create(&desired_size);
		if (vfx->output == S_HANDLE_NULL) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
		vfx->output_initialized = true;
		vfx->output_size = desired_size;
		return true;
	}
	if (se_vfx_sizes_differ(&vfx->output_size, &desired_size)) {
		se_framebuffer_set_size(vfx->output, &desired_size);
		vfx->output_size = desired_size;
	}
	return true;
}

static b8 se_vfx_ensure_output_3d(struct se_vfx_3d* vfx, const se_window_handle window) {
	if (!vfx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	s_vec2 desired_size = s_vec2(0.0f, 0.0f);
	if (!se_vfx_resolve_output_size(&vfx->params.render_size, vfx->params.auto_resize_with_window, window, &desired_size)) {
		return false;
	}
	if (!se_vfx_size_is_valid(&desired_size)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!vfx->output_initialized || !se_vfx_framebuffer_exists(se_current_context(), vfx->output)) {
		vfx->output = se_framebuffer_create(&desired_size);
		if (vfx->output == S_HANDLE_NULL) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
		vfx->output_initialized = true;
		vfx->output_size = desired_size;
		return true;
	}
	if (se_vfx_sizes_differ(&vfx->output_size, &desired_size)) {
		se_framebuffer_set_size(vfx->output, &desired_size);
		vfx->output_size = desired_size;
	}
	return true;
}

static void se_vfx_render_emitters_2d(struct se_vfx_2d* vfx) {
	se_context* ctx = se_current_context();
	if (!ctx || !vfx) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&vfx->emitters); ++i) {
		const se_vfx_emitter_2d_handle emitter_handle = s_array_handle(&vfx->emitters, (u32)i);
		se_vfx_emitter_2d* emitter = s_array_get(&vfx->emitters, emitter_handle);
		if (!emitter || emitter->shader == S_HANDLE_NULL) {
			continue;
		}
		sz alive_count = 0;
		if (!se_vfx_emitter_2d_build_render_data(emitter, &alive_count) || alive_count == 0) {
			continue;
		}

		const f32 uniform_t = se_vfx_clamp01(fmodf(emitter->uniform_time, 1.0f));
		const u32 texture_id = se_vfx_texture_id(ctx, emitter->params.texture);
		if (texture_id != 0u) {
			se_shader_set_texture(emitter->shader, "u_texture", texture_id);
			se_shader_set_int(emitter->shader, "u_has_texture", 1);
		} else {
			se_shader_set_int(emitter->shader, "u_has_texture", 0);
		}
		se_shader_set_int(emitter->shader, "u_additive", emitter->params.blend_mode == SE_VFX_BLEND_ADDITIVE ? 1 : 0);
		se_vfx_apply_uniform_tracks(&emitter->uniform_tracks, emitter->shader, uniform_t);
		if (emitter->params.uniform_callback) {
			emitter->params.uniform_callback(emitter_handle, emitter->shader, emitter->last_tick_dt, emitter->params.uniform_user_data);
		}

		se_shader_use(emitter->shader, true, true);
		se_vfx_apply_blend_mode(emitter->params.blend_mode);
		se_quad_render(&emitter->quad, alive_count);
		se_vfx_restore_alpha_blend();
	}
}

static void se_vfx_render_emitters_3d(struct se_vfx_3d* vfx, const se_camera_handle camera) {
	se_context* ctx = se_current_context();
	if (!ctx || !vfx) {
		return;
	}
	const s_mat4 view = se_camera_get_view_matrix(camera);
	const s_mat4 proj = se_camera_get_projection_matrix(camera);
	const s_mat4 view_proj = s_mat4_mul(&proj, &view);
	const s_vec3 camera_right = se_camera_get_right_vector(camera);
	const s_vec3 camera_up = se_camera_get_up_vector(camera);

	for (sz i = 0; i < s_array_get_size(&vfx->emitters); ++i) {
		const se_vfx_emitter_3d_handle emitter_handle = s_array_handle(&vfx->emitters, (u32)i);
		se_vfx_emitter_3d* emitter = s_array_get(&vfx->emitters, emitter_handle);
		if (!emitter || emitter->shader == S_HANDLE_NULL) {
			continue;
		}

		const f32 uniform_t = se_vfx_clamp01(fmodf(emitter->uniform_time, 1.0f));
		const u32 texture_id = se_vfx_texture_id(ctx, emitter->params.texture);
		if (texture_id != 0u) {
			se_shader_set_texture(emitter->shader, "u_texture", texture_id);
			se_shader_set_int(emitter->shader, "u_has_texture", 1);
		} else {
			se_shader_set_int(emitter->shader, "u_has_texture", 0);
		}
		se_shader_set_int(emitter->shader, "u_additive", emitter->params.blend_mode == SE_VFX_BLEND_ADDITIVE ? 1 : 0);
		se_vfx_apply_uniform_tracks(&emitter->uniform_tracks, emitter->shader, uniform_t);
		if (emitter->params.uniform_callback) {
			emitter->params.uniform_callback(emitter_handle, emitter->shader, emitter->last_tick_dt, emitter->params.uniform_user_data);
		}

		se_shader_use(emitter->shader, true, true);
		se_vfx_apply_blend_mode(emitter->params.blend_mode);
		if (emitter->use_mesh) {
			glDepthMask(GL_TRUE);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
			if (!se_vfx_model_exists(ctx, emitter->params.model)) {
				glDepthMask(GL_FALSE);
				glDisable(GL_CULL_FACE);
				se_vfx_restore_alpha_blend();
				continue;
			}
			se_model* model = s_array_get(&ctx->models, emitter->params.model);
			if (!model) {
				glDepthMask(GL_FALSE);
				glDisable(GL_CULL_FACE);
				se_vfx_restore_alpha_blend();
				continue;
			}
			for (sz draw_index = 0; draw_index < s_array_get_size(&emitter->mesh_draws); ++draw_index) {
				se_vfx_mesh_draw* draw = s_array_get(&emitter->mesh_draws, s_array_handle(&emitter->mesh_draws, (u32)draw_index));
				if (!draw || draw->vao == 0u || draw->index_count == 0u || draw->mesh_index >= s_array_get_size(&model->meshes)) {
					continue;
				}
				se_mesh* mesh = s_array_get(&model->meshes, s_array_handle(&model->meshes, draw->mesh_index));
				if (!mesh || !se_mesh_has_gpu_data(mesh) || mesh->gpu.index_count == 0u) {
					continue;
				}
				sz alive_count = 0;
				if (!se_vfx_emitter_3d_build_mesh_render_data(emitter, &view_proj, &mesh->matrix, &alive_count) || alive_count == 0) {
					continue;
				}
				se_vfx_mesh_draw_upload(draw, &emitter->render_instances, &emitter->render_colors, alive_count);
				glBindVertexArray(draw->vao);
				glDrawElementsInstanced(GL_TRIANGLES, draw->index_count, GL_UNSIGNED_INT, 0, (GLsizei)alive_count);
			}
			glBindVertexArray(0);
			glDepthMask(GL_FALSE);
			glDisable(GL_CULL_FACE);
			se_vfx_restore_alpha_blend();
			continue;
		}
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		sz alive_count = 0;
		if (!se_vfx_emitter_3d_build_billboard_render_data(emitter, &alive_count) || alive_count == 0) {
			se_vfx_restore_alpha_blend();
			continue;
		}
		se_shader_set_mat4(emitter->shader, "u_view_proj", &view_proj);
		se_shader_set_vec3(emitter->shader, "u_camera_right", &camera_right);
		se_shader_set_vec3(emitter->shader, "u_camera_up", &camera_up);
		se_quad_render(&emitter->quad, alive_count);
		se_vfx_restore_alpha_blend();
	}
}

static b8 se_vfx_draw_output_to_window(const se_framebuffer_handle output, const se_window_handle window) {
	se_context* ctx = se_current_context();
	if (!ctx || output == S_HANDLE_NULL || window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_vfx_framebuffer_exists(ctx, output) || !se_vfx_window_exists(ctx, window)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_window* window_ptr = s_array_get(&ctx->windows, window);
	if (!window_ptr || window_ptr->shader == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	u32 texture = 0;
	if (!se_framebuffer_get_texture_id(output, &texture) || texture == 0u) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	u32 width = 0;
	u32 height = 0;
	se_window_get_framebuffer_size(window, &width, &height);
	if (width == 0 || height == 0) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}

	se_vfx_gl_state state = {0};
	se_vfx_capture_gl_state(&state);

	se_render_unbind_framebuffer();
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	se_shader_set_texture(window_ptr->shader, "u_texture", texture);
	se_window_render_quad(window);

	se_vfx_restore_gl_state(&state);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void se_vfx_clear_offscreen_transparent(void) {
	GLfloat clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_color);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	se_render_clear();
	glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
}

se_vfx_2d_handle se_vfx_2d_create(const se_vfx_2d_params* params) {
	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->vfx_2ds) == 0) {
		s_array_init(&ctx->vfx_2ds);
	}
	const se_vfx_2d_params defaults = SE_VFX_2D_PARAMS_DEFAULTS;
	se_vfx_2d_params cfg = params ? *params : defaults;
	if (cfg.max_emitters == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (cfg.render_size.x < 0.0f || cfg.render_size.y < 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if ((cfg.render_size.x == 0.0f) != (cfg.render_size.y == 0.0f)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	const se_vfx_2d_handle handle = s_array_increment(&ctx->vfx_2ds);
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, handle);
	if (!vfx) {
		s_array_remove(&ctx->vfx_2ds, handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	memset(vfx, 0, sizeof(*vfx));
	vfx->params = cfg;
	vfx->output = S_HANDLE_NULL;
	vfx->output_size = s_vec2(0.0f, 0.0f);
	vfx->output_initialized = false;
	s_array_init(&vfx->emitters);
	s_array_reserve(&vfx->emitters, cfg.max_emitters);
	se_set_last_error(SE_RESULT_OK);
	return handle;
}

se_vfx_3d_handle se_vfx_3d_create(const se_vfx_3d_params* params) {
	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->vfx_3ds) == 0) {
		s_array_init(&ctx->vfx_3ds);
	}
	const se_vfx_3d_params defaults = SE_VFX_3D_PARAMS_DEFAULTS;
	se_vfx_3d_params cfg = params ? *params : defaults;
	if (cfg.max_emitters == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (cfg.render_size.x < 0.0f || cfg.render_size.y < 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if ((cfg.render_size.x == 0.0f) != (cfg.render_size.y == 0.0f)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	const se_vfx_3d_handle handle = s_array_increment(&ctx->vfx_3ds);
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, handle);
	if (!vfx) {
		s_array_remove(&ctx->vfx_3ds, handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	memset(vfx, 0, sizeof(*vfx));
	vfx->params = cfg;
	vfx->output = S_HANDLE_NULL;
	vfx->output_size = s_vec2(0.0f, 0.0f);
	vfx->output_initialized = false;
	s_array_init(&vfx->emitters);
	s_array_reserve(&vfx->emitters, cfg.max_emitters);
	se_set_last_error(SE_RESULT_OK);
	return handle;
}

void se_vfx_2d_destroy(se_vfx_2d_handle vfx_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL) {
		return;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		return;
	}
	if (vfx->output != S_HANDLE_NULL && se_vfx_framebuffer_exists(ctx, vfx->output)) {
		se_framebuffer_destroy(vfx->output);
	}
	vfx->output = S_HANDLE_NULL;
	vfx->output_initialized = false;
	vfx->output_size = s_vec2(0.0f, 0.0f);
	while (s_array_get_size(&vfx->emitters) > 0) {
		s_handle emitter_handle = s_array_handle(&vfx->emitters, (u32)(s_array_get_size(&vfx->emitters) - 1));
		se_vfx_emitter_2d* emitter = s_array_get(&vfx->emitters, emitter_handle);
		se_vfx_emitter_2d_destroy(ctx, emitter);
		s_array_remove(&vfx->emitters, emitter_handle);
	}
	s_array_clear(&vfx->emitters);
	s_array_remove(&ctx->vfx_2ds, vfx_handle);
}

void se_vfx_3d_destroy(se_vfx_3d_handle vfx_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL) {
		return;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		return;
	}
	if (vfx->output != S_HANDLE_NULL && se_vfx_framebuffer_exists(ctx, vfx->output)) {
		se_framebuffer_destroy(vfx->output);
	}
	vfx->output = S_HANDLE_NULL;
	vfx->output_initialized = false;
	vfx->output_size = s_vec2(0.0f, 0.0f);
	while (s_array_get_size(&vfx->emitters) > 0) {
		s_handle emitter_handle = s_array_handle(&vfx->emitters, (u32)(s_array_get_size(&vfx->emitters) - 1));
		se_vfx_emitter_3d* emitter = s_array_get(&vfx->emitters, emitter_handle);
		se_vfx_emitter_3d_destroy(ctx, emitter);
		s_array_remove(&vfx->emitters, emitter_handle);
	}
	s_array_clear(&vfx->emitters);
	s_array_remove(&ctx->vfx_3ds, vfx_handle);
}


se_vfx_emitter_2d_handle se_vfx_2d_add_emitter(se_vfx_2d_handle vfx_handle, const se_vfx_emitter_2d_params* params) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}
	if (s_array_get_size(&vfx->emitters) >= vfx->params.max_emitters) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return S_HANDLE_NULL;
	}
	const se_vfx_emitter_2d_params defaults = SE_VFX_EMITTER_2D_PARAMS_DEFAULTS;
	se_vfx_emitter_2d_params cfg = params ? *params : defaults;
	cfg.max_particles = s_max(1u, cfg.max_particles);
	cfg.lifetime_min = s_max(SE_VFX_MIN_LIFETIME, cfg.lifetime_min);
	cfg.lifetime_max = s_max(cfg.lifetime_min, cfg.lifetime_max);
	if (!se_vfx_shader_pair_valid(cfg.vertex_shader_path, cfg.fragment_shader_path)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	const se_vfx_emitter_2d_handle emitter_handle = s_array_increment(&vfx->emitters);
	se_vfx_emitter_2d* emitter = s_array_get(&vfx->emitters, emitter_handle);
	if (!emitter) {
		s_array_remove(&vfx->emitters, emitter_handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	memset(emitter, 0, sizeof(*emitter));
	emitter->params = cfg;
	emitter->running = cfg.start_active;
	emitter->rng_state = se_vfx_seed_from_handle(emitter_handle);
	if (!se_vfx_shader_path_copy(emitter->vertex_shader_path, cfg.vertex_shader_path) ||
		!se_vfx_shader_path_copy(emitter->fragment_shader_path, cfg.fragment_shader_path)) {
		s_array_remove(&vfx->emitters, emitter_handle);
		return S_HANDLE_NULL;
	}
	se_vfx_emitter_2d_sync_shader_param_ptrs(emitter);
	s_array_init(&emitter->particles);
	s_array_init(&emitter->free_slots);
	s_array_init(&emitter->render_transforms);
	s_array_init(&emitter->render_buffers);
	s_array_init(&emitter->uniform_tracks);
	s_array_reserve(&emitter->particles, cfg.max_particles);
	s_array_reserve(&emitter->free_slots, cfg.max_particles);
	s_array_reserve(&emitter->render_transforms, cfg.max_particles);
	s_array_reserve(&emitter->render_buffers, cfg.max_particles);
	s_array_reserve(&emitter->uniform_tracks, 8);
	for (u32 i = 0; i < SE_VFX_TARGET_COUNT; ++i) {
		se_vfx_track_init(&emitter->builtin_tracks[i]);
	}
	se_quad_2d_create(&emitter->quad, cfg.max_particles * 2u);
	if (emitter->quad.vao == 0u) {
		se_vfx_emitter_2d_destroy(ctx, emitter);
		s_array_remove(&vfx->emitters, emitter_handle);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return S_HANDLE_NULL;
	}
	se_quad_2d_add_instance_buffer_mat3(&emitter->quad, s_array_get_data(&emitter->render_transforms), cfg.max_particles);
	se_quad_2d_add_instance_buffer(&emitter->quad, s_array_get_data(&emitter->render_buffers), cfg.max_particles);
	if (!se_vfx_emitter_2d_reload_shader(ctx, emitter)) {
		se_vfx_emitter_2d_destroy(ctx, emitter);
		s_array_remove(&vfx->emitters, emitter_handle);
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		}
		return S_HANDLE_NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return emitter_handle;
}

se_vfx_emitter_3d_handle se_vfx_3d_add_emitter(se_vfx_3d_handle vfx_handle, const se_vfx_emitter_3d_params* params) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}
	if (s_array_get_size(&vfx->emitters) >= vfx->params.max_emitters) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return S_HANDLE_NULL;
	}
	const se_vfx_emitter_3d_params defaults = SE_VFX_EMITTER_3D_PARAMS_DEFAULTS;
	se_vfx_emitter_3d_params cfg = params ? *params : defaults;
	cfg.max_particles = s_max(1u, cfg.max_particles);
	cfg.lifetime_min = s_max(SE_VFX_MIN_LIFETIME, cfg.lifetime_min);
	cfg.lifetime_max = s_max(cfg.lifetime_min, cfg.lifetime_max);
	if (!se_vfx_shader_pair_valid(cfg.vertex_shader_path, cfg.fragment_shader_path)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (cfg.model != S_HANDLE_NULL && !se_vfx_model_exists(ctx, cfg.model)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return S_HANDLE_NULL;
	}

	const se_vfx_emitter_3d_handle emitter_handle = s_array_increment(&vfx->emitters);
	se_vfx_emitter_3d* emitter = s_array_get(&vfx->emitters, emitter_handle);
	if (!emitter) {
		s_array_remove(&vfx->emitters, emitter_handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}
	memset(emitter, 0, sizeof(*emitter));
	emitter->params = cfg;
	emitter->running = cfg.start_active;
	emitter->rng_state = se_vfx_seed_from_handle(emitter_handle);
	if (!se_vfx_shader_path_copy(emitter->vertex_shader_path, cfg.vertex_shader_path) ||
		!se_vfx_shader_path_copy(emitter->fragment_shader_path, cfg.fragment_shader_path)) {
		s_array_remove(&vfx->emitters, emitter_handle);
		return S_HANDLE_NULL;
	}
	se_vfx_emitter_3d_sync_shader_param_ptrs(emitter);
	s_array_init(&emitter->particles);
	s_array_init(&emitter->free_slots);
	s_array_init(&emitter->render_instances);
	s_array_init(&emitter->render_colors);
	s_array_init(&emitter->mesh_draws);
	s_array_init(&emitter->uniform_tracks);
	s_array_reserve(&emitter->particles, cfg.max_particles);
	s_array_reserve(&emitter->free_slots, cfg.max_particles);
	s_array_reserve(&emitter->render_instances, cfg.max_particles);
	s_array_reserve(&emitter->render_colors, cfg.max_particles);
	s_array_reserve(&emitter->uniform_tracks, 8);
	for (u32 i = 0; i < SE_VFX_TARGET_COUNT; ++i) {
		se_vfx_track_init(&emitter->builtin_tracks[i]);
	}
	if (!se_vfx_emitter_3d_setup_renderer(ctx, emitter)) {
		se_vfx_emitter_3d_destroy(ctx, emitter);
		s_array_remove(&vfx->emitters, emitter_handle);
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		}
		return S_HANDLE_NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return emitter_handle;
}

b8 se_vfx_2d_remove_emitter(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_vfx_emitter_2d* emitter = s_array_get(&vfx->emitters, emitter_handle);
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_vfx_emitter_2d_destroy(ctx, emitter);
	s_array_remove(&vfx->emitters, emitter_handle);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_remove_emitter(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_vfx_emitter_3d* emitter = s_array_get(&vfx->emitters, emitter_handle);
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	se_vfx_emitter_3d_destroy(ctx, emitter);
	s_array_remove(&vfx->emitters, emitter_handle);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_start(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->running = true;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_stop(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->running = false;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_start(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->running = true;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_stop(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->running = false;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_burst(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, u32 count) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const u32 burst_count = count > 0 ? count : emitter->params.burst_count;
	for (u32 i = 0; i < burst_count; ++i) {
		if (se_vfx_emitter_2d_spawn_particle(emitter)) {
			vfx->spawned_particles++;
		}
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_burst(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, u32 count) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const u32 burst_count = count > 0 ? count : emitter->params.burst_count;
	for (u32 i = 0; i < burst_count; ++i) {
		if (se_vfx_emitter_3d_spawn_particle(emitter)) {
			vfx->spawned_particles++;
		}
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_set_texture(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_texture_handle texture) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (texture != S_HANDLE_NULL && !se_vfx_texture_exists(ctx, texture)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.texture = texture;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_set_texture(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_texture_handle texture) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (texture != S_HANDLE_NULL && !se_vfx_texture_exists(ctx, texture)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.texture = texture;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_set_shader(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, const c8* vertex_shader_path, const c8* fragment_shader_path) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL || !se_vfx_shader_pair_valid(vertex_shader_path, fragment_shader_path)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	c8 new_vertex_path[SE_MAX_PATH_LENGTH] = {0};
	c8 new_fragment_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_vfx_shader_path_copy(new_vertex_path, vertex_shader_path) ||
		!se_vfx_shader_path_copy(new_fragment_path, fragment_shader_path)) {
		return false;
	}
	c8 old_vertex_path[SE_MAX_PATH_LENGTH] = {0};
	c8 old_fragment_path[SE_MAX_PATH_LENGTH] = {0};
	strncpy(old_vertex_path, emitter->vertex_shader_path, SE_MAX_PATH_LENGTH - 1u);
	strncpy(old_fragment_path, emitter->fragment_shader_path, SE_MAX_PATH_LENGTH - 1u);

	strncpy(emitter->vertex_shader_path, new_vertex_path, SE_MAX_PATH_LENGTH - 1u);
	strncpy(emitter->fragment_shader_path, new_fragment_path, SE_MAX_PATH_LENGTH - 1u);
	emitter->vertex_shader_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
	emitter->fragment_shader_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
	se_vfx_emitter_2d_sync_shader_param_ptrs(emitter);
	if (!se_vfx_emitter_2d_reload_shader(ctx, emitter)) {
		strncpy(emitter->vertex_shader_path, old_vertex_path, SE_MAX_PATH_LENGTH - 1u);
		strncpy(emitter->fragment_shader_path, old_fragment_path, SE_MAX_PATH_LENGTH - 1u);
		emitter->vertex_shader_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
		emitter->fragment_shader_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
		se_vfx_emitter_2d_sync_shader_param_ptrs(emitter);
		(void)se_vfx_emitter_2d_reload_shader(ctx, emitter);
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		}
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_set_model(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_model_handle model) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (model != S_HANDLE_NULL && !se_vfx_model_exists(ctx, model)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (emitter->params.model == model) {
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	emitter->params.model = model;
	if (!se_vfx_emitter_3d_setup_renderer(ctx, emitter)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		}
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_set_shader(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, const c8* vertex_shader_path, const c8* fragment_shader_path) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL || !se_vfx_shader_pair_valid(vertex_shader_path, fragment_shader_path)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	c8 new_vertex_path[SE_MAX_PATH_LENGTH] = {0};
	c8 new_fragment_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_vfx_shader_path_copy(new_vertex_path, vertex_shader_path) ||
		!se_vfx_shader_path_copy(new_fragment_path, fragment_shader_path)) {
		return false;
	}
	c8 old_vertex_path[SE_MAX_PATH_LENGTH] = {0};
	c8 old_fragment_path[SE_MAX_PATH_LENGTH] = {0};
	strncpy(old_vertex_path, emitter->vertex_shader_path, SE_MAX_PATH_LENGTH - 1u);
	strncpy(old_fragment_path, emitter->fragment_shader_path, SE_MAX_PATH_LENGTH - 1u);

	strncpy(emitter->vertex_shader_path, new_vertex_path, SE_MAX_PATH_LENGTH - 1u);
	strncpy(emitter->fragment_shader_path, new_fragment_path, SE_MAX_PATH_LENGTH - 1u);
	emitter->vertex_shader_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
	emitter->fragment_shader_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
	se_vfx_emitter_3d_sync_shader_param_ptrs(emitter);
	if (!se_vfx_emitter_3d_setup_renderer(ctx, emitter)) {
		strncpy(emitter->vertex_shader_path, old_vertex_path, SE_MAX_PATH_LENGTH - 1u);
		strncpy(emitter->fragment_shader_path, old_fragment_path, SE_MAX_PATH_LENGTH - 1u);
		emitter->vertex_shader_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
		emitter->fragment_shader_path[SE_MAX_PATH_LENGTH - 1u] = '\0';
		se_vfx_emitter_3d_sync_shader_param_ptrs(emitter);
		(void)se_vfx_emitter_3d_setup_renderer(ctx, emitter);
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		}
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_set_blend_mode(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_vfx_blend_mode blend_mode) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL || blend_mode > SE_VFX_BLEND_ADDITIVE) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.blend_mode = blend_mode;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_set_blend_mode(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_vfx_blend_mode blend_mode) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL || blend_mode > SE_VFX_BLEND_ADDITIVE) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.blend_mode = blend_mode;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_set_spawn(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, f32 spawn_rate, u32 burst_count, f32 lifetime_min, f32 lifetime_max) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL || lifetime_min <= 0.0f || lifetime_max < lifetime_min || spawn_rate < 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.spawn_rate = spawn_rate;
	emitter->params.burst_count = burst_count;
	emitter->params.lifetime_min = s_max(SE_VFX_MIN_LIFETIME, lifetime_min);
	emitter->params.lifetime_max = s_max(emitter->params.lifetime_min, lifetime_max);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_set_spawn(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, f32 spawn_rate, u32 burst_count, f32 lifetime_min, f32 lifetime_max) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL || lifetime_min <= 0.0f || lifetime_max < lifetime_min || spawn_rate < 0.0f) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.spawn_rate = spawn_rate;
	emitter->params.burst_count = burst_count;
	emitter->params.lifetime_min = s_max(SE_VFX_MIN_LIFETIME, lifetime_min);
	emitter->params.lifetime_max = s_max(emitter->params.lifetime_min, lifetime_max);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_set_particle_callback(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_vfx_particle_update_2d_callback callback, void* user_data) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.particle_callback = callback;
	emitter->params.particle_user_data = user_data;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_set_particle_callback(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_vfx_particle_update_3d_callback callback, void* user_data) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.particle_callback = callback;
	emitter->params.particle_user_data = user_data;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_set_uniform_callback(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_vfx_uniform_2d_callback callback, void* user_data) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_2d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.uniform_callback = callback;
	emitter->params.uniform_user_data = user_data;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_set_uniform_callback(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_vfx_uniform_3d_callback callback, void* user_data) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || emitter_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	se_vfx_emitter_3d* emitter = vfx ? s_array_get(&vfx->emitters, emitter_handle) : NULL;
	if (!emitter) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	emitter->params.uniform_callback = callback;
	emitter->params.uniform_user_data = user_data;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

#define SE_VFX_GET_EMITTER_2D_OR_RETURN(_vfx_handle, _emitter_handle, _out_vfx, _out_emitter) \
	se_context* ctx = se_current_context(); \
	if (!ctx || (_vfx_handle) == S_HANDLE_NULL || (_emitter_handle) == S_HANDLE_NULL) { \
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT); \
		return false; \
	} \
	struct se_vfx_2d* (_out_vfx) = se_vfx_2d_from_handle(ctx, (_vfx_handle)); \
	se_vfx_emitter_2d* (_out_emitter) = (_out_vfx) ? s_array_get(&(_out_vfx)->emitters, (_emitter_handle)) : NULL; \
	if (!(_out_emitter)) { \
		se_set_last_error(SE_RESULT_NOT_FOUND); \
		return false; \
	}

#define SE_VFX_GET_EMITTER_3D_OR_RETURN(_vfx_handle, _emitter_handle, _out_vfx, _out_emitter) \
	se_context* ctx = se_current_context(); \
	if (!ctx || (_vfx_handle) == S_HANDLE_NULL || (_emitter_handle) == S_HANDLE_NULL) { \
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT); \
		return false; \
	} \
	struct se_vfx_3d* (_out_vfx) = se_vfx_3d_from_handle(ctx, (_vfx_handle)); \
	se_vfx_emitter_3d* (_out_emitter) = (_out_vfx) ? s_array_get(&(_out_vfx)->emitters, (_emitter_handle)) : NULL; \
	if (!(_out_emitter)) { \
		se_set_last_error(SE_RESULT_NOT_FOUND); \
		return false; \
	}

b8 se_vfx_2d_emitter_add_builtin_float_key(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_vfx_builtin_target target, se_curve_mode mode, f32 t, f32 value) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_2d_add_builtin_key_internal(emitter, target, SE_VFX_TRACK_VALUE_FLOAT, mode, t, &value);
}

b8 se_vfx_2d_emitter_add_builtin_vec2_key(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec2* value) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_2d_add_builtin_key_internal(emitter, target, SE_VFX_TRACK_VALUE_VEC2, mode, t, value);
}

b8 se_vfx_2d_emitter_add_builtin_vec3_key(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec3* value) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_2d_add_builtin_key_internal(emitter, target, SE_VFX_TRACK_VALUE_VEC3, mode, t, value);
}

b8 se_vfx_2d_emitter_add_builtin_vec4_key(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec4* value) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_2d_add_builtin_key_internal(emitter, target, SE_VFX_TRACK_VALUE_VEC4, mode, t, value);
}

b8 se_vfx_3d_emitter_add_builtin_float_key(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_vfx_builtin_target target, se_curve_mode mode, f32 t, f32 value) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_3d_add_builtin_key_internal(emitter, target, SE_VFX_TRACK_VALUE_FLOAT, mode, t, &value);
}

b8 se_vfx_3d_emitter_add_builtin_vec2_key(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec2* value) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_3d_add_builtin_key_internal(emitter, target, SE_VFX_TRACK_VALUE_VEC2, mode, t, value);
}

b8 se_vfx_3d_emitter_add_builtin_vec3_key(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec3* value) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_3d_add_builtin_key_internal(emitter, target, SE_VFX_TRACK_VALUE_VEC3, mode, t, value);
}

b8 se_vfx_3d_emitter_add_builtin_vec4_key(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec4* value) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_3d_add_builtin_key_internal(emitter, target, SE_VFX_TRACK_VALUE_VEC4, mode, t, value);
}

b8 se_vfx_2d_emitter_add_uniform_float_key(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, const c8* uniform_name, se_curve_mode mode, f32 t, f32 value) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_add_uniform_key_internal(&emitter->uniform_tracks, uniform_name, SE_VFX_TRACK_VALUE_FLOAT, mode, t, &value);
}

b8 se_vfx_2d_emitter_add_uniform_vec2_key(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec2* value) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_add_uniform_key_internal(&emitter->uniform_tracks, uniform_name, SE_VFX_TRACK_VALUE_VEC2, mode, t, value);
}

b8 se_vfx_2d_emitter_add_uniform_vec3_key(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec3* value) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_add_uniform_key_internal(&emitter->uniform_tracks, uniform_name, SE_VFX_TRACK_VALUE_VEC3, mode, t, value);
}

b8 se_vfx_2d_emitter_add_uniform_vec4_key(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec4* value) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_add_uniform_key_internal(&emitter->uniform_tracks, uniform_name, SE_VFX_TRACK_VALUE_VEC4, mode, t, value);
}

b8 se_vfx_3d_emitter_add_uniform_float_key(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, const c8* uniform_name, se_curve_mode mode, f32 t, f32 value) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_add_uniform_key_internal(&emitter->uniform_tracks, uniform_name, SE_VFX_TRACK_VALUE_FLOAT, mode, t, &value);
}

b8 se_vfx_3d_emitter_add_uniform_vec2_key(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec2* value) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_add_uniform_key_internal(&emitter->uniform_tracks, uniform_name, SE_VFX_TRACK_VALUE_VEC2, mode, t, value);
}

b8 se_vfx_3d_emitter_add_uniform_vec3_key(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec3* value) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_add_uniform_key_internal(&emitter->uniform_tracks, uniform_name, SE_VFX_TRACK_VALUE_VEC3, mode, t, value);
}

b8 se_vfx_3d_emitter_add_uniform_vec4_key(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec4* value) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_add_uniform_key_internal(&emitter->uniform_tracks, uniform_name, SE_VFX_TRACK_VALUE_VEC4, mode, t, value);
}

b8 se_vfx_2d_emitter_clear_builtin_track(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, se_vfx_builtin_target target) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	if (target >= SE_VFX_TARGET_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_vfx_track_clear_values(&emitter->builtin_tracks[target]);
	emitter->builtin_tracks[target].enabled = false;
	emitter->builtin_tracks[target].value_type = SE_VFX_TRACK_VALUE_NONE;
	emitter->builtin_tracks[target].mode = SE_CURVE_LINEAR;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_clear_builtin_track(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, se_vfx_builtin_target target) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	if (target >= SE_VFX_TARGET_COUNT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_vfx_track_clear_values(&emitter->builtin_tracks[target]);
	emitter->builtin_tracks[target].enabled = false;
	emitter->builtin_tracks[target].value_type = SE_VFX_TRACK_VALUE_NONE;
	emitter->builtin_tracks[target].mode = SE_CURVE_LINEAR;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_emitter_clear_uniform_track(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle, const c8* uniform_name) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_uniform_track_remove(&emitter->uniform_tracks, uniform_name);
}

b8 se_vfx_3d_emitter_clear_uniform_track(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle, const c8* uniform_name) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	return se_vfx_uniform_track_remove(&emitter->uniform_tracks, uniform_name);
}

b8 se_vfx_2d_emitter_clear_tracks(se_vfx_2d_handle vfx_handle, se_vfx_emitter_2d_handle emitter_handle) {
	SE_VFX_GET_EMITTER_2D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	for (u32 target = 0; target < SE_VFX_TARGET_COUNT; ++target) {
		se_vfx_track_reset(&emitter->builtin_tracks[target]);
	}
	se_vfx_uniform_tracks_clear_all(&emitter->uniform_tracks);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_emitter_clear_tracks(se_vfx_3d_handle vfx_handle, se_vfx_emitter_3d_handle emitter_handle) {
	SE_VFX_GET_EMITTER_3D_OR_RETURN(vfx_handle, emitter_handle, vfx, emitter);
	(void)vfx;
	for (u32 target = 0; target < SE_VFX_TARGET_COUNT; ++target) {
		se_vfx_track_reset(&emitter->builtin_tracks[target]);
	}
	se_vfx_uniform_tracks_clear_all(&emitter->uniform_tracks);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_tick(se_vfx_2d_handle vfx_handle, f32 dt) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const f32 clamped_dt = se_vfx_sanitize_dt(dt);
	for (sz i = 0; i < s_array_get_size(&vfx->emitters); ++i) {
		se_vfx_emitter_2d* emitter = s_array_get(&vfx->emitters, s_array_handle(&vfx->emitters, (u32)i));
		se_vfx_emitter_2d_tick(emitter, clamped_dt, &vfx->spawned_particles, &vfx->expired_particles);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_tick(se_vfx_3d_handle vfx_handle, f32 dt) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const f32 clamped_dt = se_vfx_sanitize_dt(dt);
	for (sz i = 0; i < s_array_get_size(&vfx->emitters); ++i) {
		se_vfx_emitter_3d* emitter = s_array_get(&vfx->emitters, s_array_handle(&vfx->emitters, (u32)i));
		se_vfx_emitter_3d_tick(emitter, clamped_dt, &vfx->spawned_particles, &vfx->expired_particles);
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_tick_window(se_vfx_2d_handle vfx_handle, se_window_handle window) {
	if (window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const f32 dt = (f32)se_window_get_delta_time(window);
	return se_vfx_2d_tick(vfx_handle, dt);
}

b8 se_vfx_3d_tick_window(se_vfx_3d_handle vfx_handle, se_window_handle window) {
	if (window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const f32 dt = (f32)se_window_get_delta_time(window);
	return se_vfx_3d_tick(vfx_handle, dt);
}

b8 se_vfx_2d_render(se_vfx_2d_handle vfx_handle, se_window_handle window) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	if (!vfx || !se_vfx_window_exists(ctx, window)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (!se_vfx_ensure_output_2d(vfx, window)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		}
		return false;
	}

	se_vfx_gl_state state = {0};
	se_vfx_capture_gl_state(&state);

	se_framebuffer_bind(vfx->output);
	se_vfx_clear_offscreen_transparent();
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	se_vfx_render_emitters_2d(vfx);
	se_framebuffer_unbind(vfx->output);

	se_vfx_restore_gl_state(&state);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_render(se_vfx_3d_handle vfx_handle, se_window_handle window, se_camera_handle camera) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || window == S_HANDLE_NULL || camera == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	if (!vfx || !se_vfx_window_exists(ctx, window) || !se_vfx_camera_exists(ctx, camera)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (!se_vfx_ensure_output_3d(vfx, window)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		}
		return false;
	}

	se_vfx_gl_state state = {0};
	se_vfx_capture_gl_state(&state);

	se_framebuffer_bind(vfx->output);
	se_vfx_clear_offscreen_transparent();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	se_vfx_render_emitters_3d(vfx, camera);
	se_framebuffer_unbind(vfx->output);

	se_vfx_restore_gl_state(&state);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_draw(se_vfx_2d_handle vfx_handle, se_window_handle window) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	if (!vfx || !se_vfx_window_exists(ctx, window)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (!vfx->output_initialized || !se_vfx_framebuffer_exists(ctx, vfx->output)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	return se_vfx_draw_output_to_window(vfx->output, window);
}

b8 se_vfx_3d_draw(se_vfx_3d_handle vfx_handle, se_window_handle window) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	if (!vfx || !se_vfx_window_exists(ctx, window)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (!vfx->output_initialized || !se_vfx_framebuffer_exists(ctx, vfx->output)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	return se_vfx_draw_output_to_window(vfx->output, window);
}

b8 se_vfx_2d_get_framebuffer(se_vfx_2d_handle vfx_handle, se_framebuffer_handle* out_fb) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || !out_fb) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	if (!vfx || !vfx->output_initialized || !se_vfx_framebuffer_exists(ctx, vfx->output)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_fb = vfx->output;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_get_framebuffer(se_vfx_3d_handle vfx_handle, se_framebuffer_handle* out_fb) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || !out_fb) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	if (!vfx || !vfx->output_initialized || !se_vfx_framebuffer_exists(ctx, vfx->output)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_fb = vfx->output;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_2d_get_texture_id(se_vfx_2d_handle vfx_handle, u32* out_texture) {
	se_framebuffer_handle output = S_HANDLE_NULL;
	if (!out_texture) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_vfx_2d_get_framebuffer(vfx_handle, &output)) {
		return false;
	}
	return se_framebuffer_get_texture_id(output, out_texture);
}

b8 se_vfx_3d_get_texture_id(se_vfx_3d_handle vfx_handle, u32* out_texture) {
	se_framebuffer_handle output = S_HANDLE_NULL;
	if (!out_texture) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!se_vfx_3d_get_framebuffer(vfx_handle, &output)) {
		return false;
	}
	return se_framebuffer_get_texture_id(output, out_texture);
}

b8 se_vfx_2d_get_diagnostics(se_vfx_2d_handle vfx_handle, se_vfx_diagnostics* out_diagnostics) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || !out_diagnostics) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_2d* vfx = se_vfx_2d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	memset(out_diagnostics, 0, sizeof(*out_diagnostics));
	out_diagnostics->emitter_count = (u32)s_array_get_size(&vfx->emitters);
	out_diagnostics->spawned_particles = vfx->spawned_particles;
	out_diagnostics->expired_particles = vfx->expired_particles;
	for (sz i = 0; i < s_array_get_size(&vfx->emitters); ++i) {
		se_vfx_emitter_2d* emitter = s_array_get(&vfx->emitters, s_array_handle(&vfx->emitters, (u32)i));
		if (!emitter) {
			continue;
		}
		for (sz p = 0; p < s_array_get_size(&emitter->particles); ++p) {
			se_vfx_particle_2d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)p));
			if (particle && particle->alive) {
				out_diagnostics->alive_particles++;
			}
		}
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_vfx_3d_get_diagnostics(se_vfx_3d_handle vfx_handle, se_vfx_diagnostics* out_diagnostics) {
	se_context* ctx = se_current_context();
	if (!ctx || vfx_handle == S_HANDLE_NULL || !out_diagnostics) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	struct se_vfx_3d* vfx = se_vfx_3d_from_handle(ctx, vfx_handle);
	if (!vfx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	memset(out_diagnostics, 0, sizeof(*out_diagnostics));
	out_diagnostics->emitter_count = (u32)s_array_get_size(&vfx->emitters);
	out_diagnostics->spawned_particles = vfx->spawned_particles;
	out_diagnostics->expired_particles = vfx->expired_particles;
	for (sz i = 0; i < s_array_get_size(&vfx->emitters); ++i) {
		se_vfx_emitter_3d* emitter = s_array_get(&vfx->emitters, s_array_handle(&vfx->emitters, (u32)i));
		if (!emitter) {
			continue;
		}
		for (sz p = 0; p < s_array_get_size(&emitter->particles); ++p) {
			se_vfx_particle_3d* particle = s_array_get(&emitter->particles, s_array_handle(&emitter->particles, (u32)p));
			if (particle && particle->alive) {
				out_diagnostics->alive_particles++;
			}
		}
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}
