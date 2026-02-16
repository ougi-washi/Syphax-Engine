// Syphax-Engine - Ougi Washi

#ifndef SE_VFX_H
#define SE_VFX_H

#include "se.h"
#include "se_curve.h"

typedef enum {
	SE_VFX_BLEND_ALPHA = 0,
	SE_VFX_BLEND_ADDITIVE
} se_vfx_blend_mode;

typedef enum {
	SE_VFX_TARGET_VELOCITY = 0,
	SE_VFX_TARGET_COLOR,
	SE_VFX_TARGET_SIZE,
	SE_VFX_TARGET_ROTATION,
	SE_VFX_TARGET_COUNT
} se_vfx_builtin_target;

typedef struct {
	s_vec2 position;
	s_vec2 velocity;
	s_vec4 color;
	f32 size;
	f32 rotation;
	f32 age;
	f32 lifetime;
	b8 alive : 1;
} se_vfx_particle_2d;

typedef struct {
	s_vec3 position;
	s_vec3 velocity;
	s_vec4 color;
	f32 size;
	f32 rotation;
	f32 age;
	f32 lifetime;
	b8 alive : 1;
} se_vfx_particle_3d;

typedef void (*se_vfx_particle_update_2d_callback)(se_vfx_particle_2d* particle, f32 dt, void* user_data);
typedef void (*se_vfx_particle_update_3d_callback)(se_vfx_particle_3d* particle, f32 dt, void* user_data);
typedef void (*se_vfx_uniform_2d_callback)(se_vfx_emitter_2d_handle emitter, se_shader_handle shader, f32 dt, void* user_data);
typedef void (*se_vfx_uniform_3d_callback)(se_vfx_emitter_3d_handle emitter, se_shader_handle shader, f32 dt, void* user_data);

typedef struct {
	u32 max_emitters;
	s_vec2 render_size;
	b8 auto_resize_with_window : 1;
} se_vfx_2d_params;

#define SE_VFX_2D_PARAMS_DEFAULTS ((se_vfx_2d_params){ \
	.max_emitters = 16u, \
	.render_size = {0.0f, 0.0f}, \
	.auto_resize_with_window = true \
})

typedef struct {
	u32 max_emitters;
	s_vec2 render_size;
	b8 auto_resize_with_window : 1;
} se_vfx_3d_params;

#define SE_VFX_3D_PARAMS_DEFAULTS ((se_vfx_3d_params){ \
	.max_emitters = 16u, \
	.render_size = {0.0f, 0.0f}, \
	.auto_resize_with_window = true \
})

typedef struct {
	s_vec2 position;
	s_vec2 velocity;
	s_vec4 color;
	f32 size;
	f32 rotation;
	f32 spawn_rate;
	u32 burst_count;
	f32 lifetime_min;
	f32 lifetime_max;
	u32 max_particles;
	se_texture_handle texture;
	const c8* vertex_shader_path;
	const c8* fragment_shader_path;
	se_vfx_blend_mode blend_mode;
	se_vfx_particle_update_2d_callback particle_callback;
	void* particle_user_data;
	se_vfx_uniform_2d_callback uniform_callback;
	void* uniform_user_data;
	b8 start_active : 1;
} se_vfx_emitter_2d_params;

#define SE_VFX_EMITTER_2D_PARAMS_DEFAULTS ((se_vfx_emitter_2d_params){ \
	.position = {0.0f, 0.0f}, \
	.velocity = {0.0f, 0.45f}, \
	.color = {1.0f, 1.0f, 1.0f, 1.0f}, \
	.size = 0.04f, \
	.rotation = 0.0f, \
	.spawn_rate = 32.0f, \
	.burst_count = 16u, \
	.lifetime_min = 0.4f, \
	.lifetime_max = 1.1f, \
	.max_particles = 512u, \
	.texture = S_HANDLE_NULL, \
	.vertex_shader_path = NULL, \
	.fragment_shader_path = NULL, \
	.blend_mode = SE_VFX_BLEND_ALPHA, \
	.particle_callback = NULL, \
	.particle_user_data = NULL, \
	.uniform_callback = NULL, \
	.uniform_user_data = NULL, \
	.start_active = true \
})

typedef struct {
	s_vec3 position;
	s_vec3 velocity;
	s_vec4 color;
	f32 size;
	f32 rotation;
	f32 spawn_rate;
	u32 burst_count;
	f32 lifetime_min;
	f32 lifetime_max;
	u32 max_particles;
	se_texture_handle texture;
	se_model_handle model;
	const c8* vertex_shader_path;
	const c8* fragment_shader_path;
	se_vfx_blend_mode blend_mode;
	se_vfx_particle_update_3d_callback particle_callback;
	void* particle_user_data;
	se_vfx_uniform_3d_callback uniform_callback;
	void* uniform_user_data;
	b8 start_active : 1;
} se_vfx_emitter_3d_params;

#define SE_VFX_EMITTER_3D_PARAMS_DEFAULTS ((se_vfx_emitter_3d_params){ \
	.position = {0.0f, 0.0f, 0.0f}, \
	.velocity = {0.0f, 0.9f, 0.0f}, \
	.color = {1.0f, 1.0f, 1.0f, 1.0f}, \
	.size = 0.2f, \
	.rotation = 0.0f, \
	.spawn_rate = 24.0f, \
	.burst_count = 12u, \
	.lifetime_min = 0.5f, \
	.lifetime_max = 1.5f, \
	.max_particles = 1024u, \
	.texture = S_HANDLE_NULL, \
	.model = S_HANDLE_NULL, \
	.vertex_shader_path = NULL, \
	.fragment_shader_path = NULL, \
	.blend_mode = SE_VFX_BLEND_ALPHA, \
	.particle_callback = NULL, \
	.particle_user_data = NULL, \
	.uniform_callback = NULL, \
	.uniform_user_data = NULL, \
	.start_active = true \
})

typedef struct {
	u32 emitter_count;
	u32 alive_particles;
	u64 spawned_particles;
	u64 expired_particles;
} se_vfx_diagnostics;

extern se_vfx_2d_handle se_vfx_2d_create(const se_vfx_2d_params* params);
extern se_vfx_3d_handle se_vfx_3d_create(const se_vfx_3d_params* params);
extern void se_vfx_2d_destroy(se_vfx_2d_handle vfx);
extern void se_vfx_3d_destroy(se_vfx_3d_handle vfx);

extern se_vfx_emitter_2d_handle se_vfx_2d_add_emitter(se_vfx_2d_handle vfx, const se_vfx_emitter_2d_params* params);
extern se_vfx_emitter_3d_handle se_vfx_3d_add_emitter(se_vfx_3d_handle vfx, const se_vfx_emitter_3d_params* params);
extern b8 se_vfx_2d_remove_emitter(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter);
extern b8 se_vfx_3d_remove_emitter(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter);

extern b8 se_vfx_2d_emitter_start(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter);
extern b8 se_vfx_2d_emitter_stop(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter);
extern b8 se_vfx_3d_emitter_start(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter);
extern b8 se_vfx_3d_emitter_stop(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter);

extern b8 se_vfx_2d_emitter_burst(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, u32 count);
extern b8 se_vfx_3d_emitter_burst(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, u32 count);

extern b8 se_vfx_2d_emitter_set_texture(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_texture_handle texture);
extern b8 se_vfx_3d_emitter_set_texture(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_texture_handle texture);
extern b8 se_vfx_3d_emitter_set_model(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_model_handle model);
extern b8 se_vfx_2d_emitter_set_shader(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* vertex_shader_path, const c8* fragment_shader_path);
extern b8 se_vfx_3d_emitter_set_shader(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* vertex_shader_path, const c8* fragment_shader_path);
extern b8 se_vfx_2d_emitter_set_blend_mode(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_blend_mode blend_mode);
extern b8 se_vfx_3d_emitter_set_blend_mode(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_blend_mode blend_mode);
extern b8 se_vfx_2d_emitter_set_spawn(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, f32 spawn_rate, u32 burst_count, f32 lifetime_min, f32 lifetime_max);
extern b8 se_vfx_3d_emitter_set_spawn(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, f32 spawn_rate, u32 burst_count, f32 lifetime_min, f32 lifetime_max);

extern b8 se_vfx_2d_emitter_set_particle_callback(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_particle_update_2d_callback callback, void* user_data);
extern b8 se_vfx_3d_emitter_set_particle_callback(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_particle_update_3d_callback callback, void* user_data);
extern b8 se_vfx_2d_emitter_set_uniform_callback(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_uniform_2d_callback callback, void* user_data);
extern b8 se_vfx_3d_emitter_set_uniform_callback(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_uniform_3d_callback callback, void* user_data);

extern b8 se_vfx_2d_emitter_add_builtin_float_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, f32 value);
extern b8 se_vfx_2d_emitter_add_builtin_vec2_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec2* value);
extern b8 se_vfx_2d_emitter_add_builtin_vec3_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec3* value);
extern b8 se_vfx_2d_emitter_add_builtin_vec4_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec4* value);
extern b8 se_vfx_3d_emitter_add_builtin_float_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, f32 value);
extern b8 se_vfx_3d_emitter_add_builtin_vec2_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec2* value);
extern b8 se_vfx_3d_emitter_add_builtin_vec3_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec3* value);
extern b8 se_vfx_3d_emitter_add_builtin_vec4_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec4* value);

extern b8 se_vfx_2d_emitter_add_uniform_float_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, f32 value);
extern b8 se_vfx_2d_emitter_add_uniform_vec2_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec2* value);
extern b8 se_vfx_2d_emitter_add_uniform_vec3_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec3* value);
extern b8 se_vfx_2d_emitter_add_uniform_vec4_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec4* value);
extern b8 se_vfx_3d_emitter_add_uniform_float_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, f32 value);
extern b8 se_vfx_3d_emitter_add_uniform_vec2_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec2* value);
extern b8 se_vfx_3d_emitter_add_uniform_vec3_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec3* value);
extern b8 se_vfx_3d_emitter_add_uniform_vec4_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec4* value);

extern b8 se_vfx_2d_emitter_clear_builtin_track(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target);
extern b8 se_vfx_3d_emitter_clear_builtin_track(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target);
extern b8 se_vfx_2d_emitter_clear_uniform_track(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name);
extern b8 se_vfx_3d_emitter_clear_uniform_track(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name);
extern b8 se_vfx_2d_emitter_clear_tracks(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter);
extern b8 se_vfx_3d_emitter_clear_tracks(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter);

extern b8 se_vfx_2d_tick(se_vfx_2d_handle vfx, f32 dt);
extern b8 se_vfx_3d_tick(se_vfx_3d_handle vfx, f32 dt);
extern b8 se_vfx_2d_tick_window(se_vfx_2d_handle vfx, se_window_handle window);
extern b8 se_vfx_3d_tick_window(se_vfx_3d_handle vfx, se_window_handle window);
extern b8 se_vfx_2d_render(se_vfx_2d_handle vfx, se_window_handle window);
extern b8 se_vfx_3d_render(se_vfx_3d_handle vfx, se_window_handle window, se_camera_handle camera);
extern b8 se_vfx_2d_draw(se_vfx_2d_handle vfx, se_window_handle window);
extern b8 se_vfx_3d_draw(se_vfx_3d_handle vfx, se_window_handle window);
extern b8 se_vfx_2d_get_framebuffer(se_vfx_2d_handle vfx, se_framebuffer_handle* out_fb);
extern b8 se_vfx_3d_get_framebuffer(se_vfx_3d_handle vfx, se_framebuffer_handle* out_fb);
extern b8 se_vfx_2d_get_texture_id(se_vfx_2d_handle vfx, u32* out_texture);
extern b8 se_vfx_3d_get_texture_id(se_vfx_3d_handle vfx, u32* out_texture);

extern b8 se_vfx_2d_get_diagnostics(se_vfx_2d_handle vfx, se_vfx_diagnostics* out_diagnostics);
extern b8 se_vfx_3d_get_diagnostics(se_vfx_3d_handle vfx, se_vfx_diagnostics* out_diagnostics);

#endif // SE_VFX_H
