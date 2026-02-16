// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_model.h"
#include "se_render.h"
#include "se_shader.h"
#include "se_texture.h"
#include "se_vfx.h"
#include "se_window.h"

#include <math.h>
#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

typedef struct {
	f32* time_seconds;
	f32 base;
	f32 amplitude;
	f32 speed;
} se_vfx_uniform_anim;

static const u8 se_example_png_1x1_rgba[] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
	0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
	0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41,
	0x54, 0x78, 0x9C, 0x63, 0xF8, 0xCF, 0xC0, 0xF0,
	0x1F, 0x00, 0x05, 0x00, 0x01, 0xFF, 0x89, 0x99,
	0x3D, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
	0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

static void se_vfx_uniform_callback_2d(
	const se_vfx_emitter_2d_handle emitter,
	const se_shader_handle shader,
	const f32 dt,
	void* user_data) {
	(void)emitter;
	(void)dt;
	se_vfx_uniform_anim* anim = (se_vfx_uniform_anim*)user_data;
	if (!anim || !anim->time_seconds) {
		return;
	}
	const f32 value = anim->base + sinf((*anim->time_seconds) * anim->speed) * anim->amplitude;
	se_shader_set_float(shader, "u_intensity", value);
}

static void se_vfx_uniform_callback_3d(
	const se_vfx_emitter_3d_handle emitter,
	const se_shader_handle shader,
	const f32 dt,
	void* user_data) {
	(void)emitter;
	(void)dt;
	se_vfx_uniform_anim* anim = (se_vfx_uniform_anim*)user_data;
	if (!anim || !anim->time_seconds) {
		return;
	}
	const f32 value = anim->base + cosf((*anim->time_seconds) * anim->speed) * anim->amplitude;
	se_shader_set_float(shader, "u_intensity", value);
}

i32 main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("Syphax-Engine - 24_vfx", WINDOW_WIDTH, WINDOW_HEIGHT);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.07f, 0.08f, 0.11f, 1.0f));

	se_camera_handle camera_handle = se_camera_create();
	if (camera_handle == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}
	se_camera_set_aspect_from_window(camera_handle, window);
	se_camera* camera = se_camera_get(camera_handle);
	if (camera) {
		camera->position = s_vec3(0.0f, 1.2f, 4.2f);
		camera->target = s_vec3(0.0f, 0.6f, 0.0f);
	}

	se_vfx_2d_params vfx_2d_params = SE_VFX_2D_PARAMS_DEFAULTS;
	vfx_2d_params.auto_resize_with_window = true;
	se_vfx_3d_params vfx_3d_params = SE_VFX_3D_PARAMS_DEFAULTS;
	vfx_3d_params.auto_resize_with_window = true;

	se_vfx_2d_handle vfx_2d = se_vfx_2d_create(&vfx_2d_params);
	se_vfx_3d_handle vfx_3d = se_vfx_3d_create(&vfx_3d_params);
	if (vfx_2d == S_HANDLE_NULL || vfx_3d == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_model_handle vfx_cube_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
	if (vfx_cube_model == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_texture_handle particle_texture = se_texture_load_from_memory(
		se_example_png_1x1_rgba,
		sizeof(se_example_png_1x1_rgba),
		SE_CLAMP);

	se_vfx_emitter_2d_params emitter_2d_params = SE_VFX_EMITTER_2D_PARAMS_DEFAULTS;
	emitter_2d_params.position = s_vec2(0.0f, -0.75f);
	emitter_2d_params.velocity = s_vec2(0.0f, 0.65f);
	emitter_2d_params.spawn_rate = 36.0f;
	emitter_2d_params.max_particles = 1024u;
	emitter_2d_params.texture = particle_texture;
	emitter_2d_params.size = 0.03f;
	se_vfx_emitter_2d_handle emitter_2d = se_vfx_2d_add_emitter(vfx_2d, &emitter_2d_params);
	if (emitter_2d == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_vfx_emitter_3d_params emitter_3d_params = SE_VFX_EMITTER_3D_PARAMS_DEFAULTS;
	emitter_3d_params.position = s_vec3(0.0f, 0.4f, 0.0f);
	emitter_3d_params.velocity = s_vec3(0.0f, 1.1f, 0.0f);
	emitter_3d_params.spawn_rate = 18.0f;
	emitter_3d_params.max_particles = 1024u;
	emitter_3d_params.texture = S_HANDLE_NULL;
	emitter_3d_params.model = vfx_cube_model;
	emitter_3d_params.size = 0.14f;
	se_vfx_emitter_3d_handle emitter_3d = se_vfx_3d_add_emitter(vfx_3d, &emitter_3d_params);
	if (emitter_3d == S_HANDLE_NULL) {
		se_model_destroy(vfx_cube_model);
		se_context_destroy(context);
		return 1;
	}

	se_vfx_2d_emitter_add_builtin_vec2_key(vfx_2d, emitter_2d, SE_VFX_TARGET_VELOCITY, SE_CURVE_SMOOTH, 0.0f, &s_vec2(0.0f, 0.9f));
	se_vfx_2d_emitter_add_builtin_vec2_key(vfx_2d, emitter_2d, SE_VFX_TARGET_VELOCITY, SE_CURVE_SMOOTH, 1.0f, &s_vec2(0.0f, 0.15f));
	se_vfx_2d_emitter_add_builtin_vec4_key(vfx_2d, emitter_2d, SE_VFX_TARGET_COLOR, SE_CURVE_LINEAR, 0.0f, &s_vec4(1.0f, 0.84f, 0.35f, 1.0f));
	se_vfx_2d_emitter_add_builtin_vec4_key(vfx_2d, emitter_2d, SE_VFX_TARGET_COLOR, SE_CURVE_LINEAR, 1.0f, &s_vec4(1.0f, 0.2f, 0.1f, 0.0f));
	se_vfx_2d_emitter_add_builtin_float_key(vfx_2d, emitter_2d, SE_VFX_TARGET_SIZE, SE_CURVE_EASE_OUT, 0.0f, 0.02f);
	se_vfx_2d_emitter_add_builtin_float_key(vfx_2d, emitter_2d, SE_VFX_TARGET_SIZE, SE_CURVE_EASE_OUT, 1.0f, 0.09f);

	se_vfx_3d_emitter_add_builtin_vec3_key(vfx_3d, emitter_3d, SE_VFX_TARGET_VELOCITY, SE_CURVE_LINEAR, 0.0f, &s_vec3(0.0f, 1.6f, 0.0f));
	se_vfx_3d_emitter_add_builtin_vec3_key(vfx_3d, emitter_3d, SE_VFX_TARGET_VELOCITY, SE_CURVE_LINEAR, 1.0f, &s_vec3(0.0f, 0.4f, 0.0f));
	se_vfx_3d_emitter_add_builtin_vec4_key(vfx_3d, emitter_3d, SE_VFX_TARGET_COLOR, SE_CURVE_LINEAR, 0.0f, &s_vec4(0.1f, 0.35f, 1.0f, 0.95f));
	se_vfx_3d_emitter_add_builtin_vec4_key(vfx_3d, emitter_3d, SE_VFX_TARGET_COLOR, SE_CURVE_LINEAR, 1.0f, &s_vec4(0.05f, 0.2f, 0.85f, 0.0f));
	se_vfx_3d_emitter_add_builtin_float_key(vfx_3d, emitter_3d, SE_VFX_TARGET_SIZE, SE_CURVE_EASE_IN_OUT, 0.0f, 0.06f);
	se_vfx_3d_emitter_add_builtin_float_key(vfx_3d, emitter_3d, SE_VFX_TARGET_SIZE, SE_CURVE_EASE_IN_OUT, 1.0f, 0.22f);

	se_vfx_2d_emitter_add_uniform_float_key(vfx_2d, emitter_2d, "u_intensity", SE_CURVE_EASE_IN_OUT, 0.0f, 0.65f);
	se_vfx_2d_emitter_add_uniform_float_key(vfx_2d, emitter_2d, "u_intensity", SE_CURVE_EASE_IN_OUT, 1.0f, 1.15f);
	se_vfx_3d_emitter_add_uniform_float_key(vfx_3d, emitter_3d, "u_intensity", SE_CURVE_EASE_IN_OUT, 0.0f, 0.8f);
	se_vfx_3d_emitter_add_uniform_float_key(vfx_3d, emitter_3d, "u_intensity", SE_CURVE_EASE_IN_OUT, 1.0f, 1.35f);

	f32 scene_time = 0.0f;
	se_vfx_uniform_anim uniform_anim_2d = {&scene_time, 0.9f, 0.2f, 2.0f};
	se_vfx_uniform_anim uniform_anim_3d = {&scene_time, 1.05f, 0.25f, 1.6f};
	se_vfx_2d_emitter_set_uniform_callback(vfx_2d, emitter_2d, se_vfx_uniform_callback_2d, &uniform_anim_2d);
	se_vfx_3d_emitter_set_uniform_callback(vfx_3d, emitter_3d, se_vfx_uniform_callback_3d, &uniform_anim_3d);

	b8 simulation_enabled = true;
	b8 additive_blend = false;
	u32 render_divisor = 1u;
	u64 frame_index = 0u;

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_context_reload_changed_shaders();
		scene_time = (f32)se_window_get_time(window);
		se_camera_set_aspect_from_window(camera_handle, window);

		if (camera) {
			const f32 orbit = scene_time * 0.55f;
			camera->position = s_vec3(cosf(orbit) * 4.0f, 1.5f, sinf(orbit) * 4.0f);
			camera->target = s_vec3(0.0f, 0.8f, 0.0f);
		}

		if (se_window_is_key_pressed(window, SE_KEY_T)) {
			simulation_enabled = !simulation_enabled;
			printf("24_vfx :: simulation = %s\n", simulation_enabled ? "on" : "off");
		}

		if (se_window_is_key_pressed(window, SE_KEY_R)) {
			render_divisor = s_min(render_divisor + 1u, 12u);
			printf("24_vfx :: render divisor = %u\n", render_divisor);
		}
		if (se_window_is_key_pressed(window, SE_KEY_F)) {
			render_divisor = s_max(1u, render_divisor - 1u);
			printf("24_vfx :: render divisor = %u\n", render_divisor);
		}

		if (se_window_is_key_pressed(window, SE_KEY_B)) {
			additive_blend = !additive_blend;
			se_vfx_blend_mode blend = additive_blend ? SE_VFX_BLEND_ADDITIVE : SE_VFX_BLEND_ALPHA;
			se_vfx_2d_emitter_set_blend_mode(vfx_2d, emitter_2d, blend);
			se_vfx_3d_emitter_set_blend_mode(vfx_3d, emitter_3d, blend);
		}

		if (se_window_is_key_pressed(window, SE_KEY_SPACE)) {
			se_vfx_2d_emitter_burst(vfx_2d, emitter_2d, 48u);
			se_vfx_3d_emitter_burst(vfx_3d, emitter_3d, 32u);
		}

		if (simulation_enabled) {
			const f32 dt = (f32)se_window_get_delta_time(window);
			se_vfx_2d_tick(vfx_2d, dt);
			se_vfx_3d_tick(vfx_3d, dt);
		}

		frame_index++;
		const b8 should_render = (render_divisor <= 1u) || ((frame_index % (u64)render_divisor) == 0u);
		if (should_render) {
			(void)se_vfx_3d_render(vfx_3d, window, camera_handle);
			(void)se_vfx_2d_render(vfx_2d, window);
		}

		se_render_clear();
		(void)se_vfx_3d_draw(vfx_3d, window);
		(void)se_vfx_2d_draw(vfx_2d, window);
		se_window_render_screen(window);
	}

	se_vfx_2d_destroy(vfx_2d);
	se_vfx_3d_destroy(vfx_3d);
	se_model_destroy(vfx_cube_model);
	se_camera_destroy(camera_handle);
	se_context_destroy(context);
	return 0;
}
