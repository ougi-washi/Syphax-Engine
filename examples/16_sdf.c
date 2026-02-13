// Syphax-Engine - Ougi Washi

#include "se_sdf.h"
#include "se_window.h"
#include "se_render.h"

#include <math.h>
#include <stdio.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 450
#define TARGET_FPS 60
#define GALLERY_COLUMNS 8
#define GALLERY_ROWS 4
#define GALLERY_SPACING 3.35f

int main(void) {
	se_context* ctx = se_context_create();
	if (!ctx) {
		fprintf(stderr, "16_sdf :: failed to create context\n");
		return 1;
	}

	se_window_handle window = se_window_create("Syphax-Engine - 16_sdf", WINDOW_WIDTH, WINDOW_HEIGHT);
	if (window == S_HANDLE_NULL) {
		fprintf(stderr, "16_sdf :: failed to create window\n");
		se_context_destroy(ctx);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, TARGET_FPS);
	se_render_set_background_color(s_vec4(0.03f, 0.04f, 0.05f, 1.0f));

	se_sdf_scene_desc scene_desc = SE_SDF_SCENE_DESC_DEFAULTS;
	scene_desc.initial_node_capacity = GALLERY_COLUMNS * GALLERY_ROWS + 2;
	se_sdf_scene_handle scene = se_sdf_scene_create(&scene_desc);
	if (scene == SE_SDF_SCENE_NULL) {
		fprintf(stderr, "16_sdf :: failed to create sdf scene\n");
		se_window_destroy(window);
		se_context_destroy(ctx);
		return 1;
	}

	se_sdf_node_handle root_node = SE_SDF_NODE_NULL;
	se_sdf_node_handle hero_node = SE_SDF_NODE_NULL;
	if (!se_sdf_scene_build_primitive_gallery_preset(
		scene, GALLERY_COLUMNS, GALLERY_ROWS, GALLERY_SPACING, &root_node, &hero_node
	)) {
		fprintf(stderr, "16_sdf :: failed to build gallery scene\n");
		se_sdf_scene_destroy(scene);
		se_window_destroy(window);
		se_context_destroy(ctx);
		return 1;
	}
	(void)root_node;

	se_sdf_renderer_desc renderer_desc = SE_SDF_RENDERER_DESC_DEFAULTS;
	se_sdf_renderer_handle renderer = se_sdf_renderer_create(&renderer_desc);
	if (renderer == SE_SDF_RENDERER_NULL) {
		fprintf(stderr, "16_sdf :: failed to create sdf renderer\n");
		se_sdf_scene_destroy(scene);
		se_window_destroy(window);
		se_context_destroy(ctx);
		return 1;
	}

	if (!se_sdf_renderer_set_scene(renderer, scene)) {
		fprintf(stderr, "16_sdf :: failed to attach scene to renderer\n");
		se_sdf_renderer_destroy(renderer);
		se_sdf_scene_destroy(scene);
		se_window_destroy(window);
		se_context_destroy(ctx);
		return 1;
	}

	se_sdf_material_desc shading = SE_SDF_MATERIAL_DESC_DEFAULTS;
	shading.model = SE_SDF_SHADING_STYLIZED;
	shading.base_color = s_vec3(0.70f, 0.72f, 0.75f);
	se_sdf_renderer_set_material(renderer, &shading);

	se_sdf_stylized_desc stylized = SE_SDF_STYLIZED_DESC_DEFAULTS;
	stylized.band_levels = 4.0f;
	stylized.rim_power = 2.2f;
	stylized.rim_strength = 0.55f;
	stylized.fill_strength = 0.42f;
	stylized.back_strength = 0.30f;
	stylized.checker_scale = 2.0f;
	stylized.checker_strength = 0.20f;
	stylized.ground_blend = 0.45f;
	stylized.desaturate = 0.08f;
	stylized.gamma = 2.2f;
	se_sdf_renderer_set_stylized(renderer, &stylized);

	f32 control_time = 0.0f;
	s_vec2 control_mouse = s_vec2(0.0f, 0.0f);
	f32 control_pulse = 0.0f;
	f32 control_hero_radius = 0.82f;
	s_vec3 control_hero_translation = s_vec3(0.0f, 0.08f, 0.0f);
	s_vec3 control_hero_rotation = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 control_hero_scale = s_vec3(1.0f, 1.0f, 1.0f);
	s_vec3 control_base_color = s_vec3(0.70f, 0.72f, 0.75f);
	s_vec3 control_light_dir = s_vec3(0.58f, 0.76f, 0.28f);
	s_vec3 control_light_color = s_vec3(1.0f, 1.0f, 1.0f);
	s_vec3 control_fog_color = s_vec3(0.11f, 0.15f, 0.21f);
	f32 control_fog_density = 0.0012f;
	f32 control_stylized_band_levels = stylized.band_levels;
	f32 control_stylized_rim_power = stylized.rim_power;
	f32 control_stylized_rim_strength = stylized.rim_strength;
	f32 control_stylized_fill_strength = stylized.fill_strength;
	f32 control_stylized_back_strength = stylized.back_strength;
	f32 control_stylized_checker_scale = stylized.checker_scale;
	f32 control_stylized_checker_strength = stylized.checker_strength;
	f32 control_stylized_ground_blend = stylized.ground_blend;
	f32 control_stylized_desaturate = stylized.desaturate;
	f32 control_stylized_gamma = stylized.gamma;

	se_sdf_control_handle time_control = se_sdf_control_define_float(renderer, "u_time", 0.0f);
	se_sdf_control_handle mouse_control = se_sdf_control_define_vec2(renderer, "u_mouse", &control_mouse);
	se_sdf_control_handle pulse_control = se_sdf_control_define_float(renderer, "u_pulse", 0.0f);
	se_sdf_control_handle hero_radius_control = se_sdf_control_define_float(renderer, "hero_radius", control_hero_radius);
	se_sdf_control_handle hero_translation_control = se_sdf_control_define_vec3(renderer, "hero_translation", &control_hero_translation);
	se_sdf_control_handle hero_rotation_control = se_sdf_control_define_vec3(renderer, "hero_rotation", &control_hero_rotation);
	se_sdf_control_handle hero_scale_control = se_sdf_control_define_vec3(renderer, "hero_scale", &control_hero_scale);
	se_sdf_control_handle base_color_control = se_sdf_control_define_vec3(renderer, "base_color", &control_base_color);
	se_sdf_control_handle light_dir_control = se_sdf_control_define_vec3(renderer, "light_direction", &control_light_dir);
	se_sdf_control_handle light_color_control = se_sdf_control_define_vec3(renderer, "light_color", &control_light_color);
	se_sdf_control_handle fog_color_control = se_sdf_control_define_vec3(renderer, "fog_color", &control_fog_color);
	se_sdf_control_handle fog_density_control = se_sdf_control_define_float(renderer, "fog_density", control_fog_density);
	se_sdf_control_handle stylized_band_levels_control = se_sdf_control_define_float(renderer, "stylized_band_levels", control_stylized_band_levels);
	se_sdf_control_handle stylized_rim_power_control = se_sdf_control_define_float(renderer, "stylized_rim_power", control_stylized_rim_power);
	se_sdf_control_handle stylized_rim_strength_control = se_sdf_control_define_float(renderer, "stylized_rim_strength", control_stylized_rim_strength);
	se_sdf_control_handle stylized_fill_strength_control = se_sdf_control_define_float(renderer, "stylized_fill_strength", control_stylized_fill_strength);
	se_sdf_control_handle stylized_back_strength_control = se_sdf_control_define_float(renderer, "stylized_back_strength", control_stylized_back_strength);
	se_sdf_control_handle stylized_checker_scale_control = se_sdf_control_define_float(renderer, "stylized_checker_scale", control_stylized_checker_scale);
	se_sdf_control_handle stylized_checker_strength_control = se_sdf_control_define_float(renderer, "stylized_checker_strength", control_stylized_checker_strength);
	se_sdf_control_handle stylized_ground_blend_control = se_sdf_control_define_float(renderer, "stylized_ground_blend", control_stylized_ground_blend);
	se_sdf_control_handle stylized_desaturate_control = se_sdf_control_define_float(renderer, "stylized_desaturate", control_stylized_desaturate);
	se_sdf_control_handle stylized_gamma_control = se_sdf_control_define_float(renderer, "stylized_gamma", control_stylized_gamma);
	se_sdf_control_bind_ptr_float(renderer, time_control, &control_time);
	se_sdf_control_bind_ptr_vec2(renderer, mouse_control, &control_mouse);
	se_sdf_control_bind_ptr_vec3(renderer, hero_translation_control, &control_hero_translation);
	se_sdf_control_bind_ptr_vec3(renderer, hero_rotation_control, &control_hero_rotation);
	se_sdf_control_bind_ptr_vec3(renderer, hero_scale_control, &control_hero_scale);
	se_sdf_control_bind_ptr_vec3(renderer, base_color_control, &control_base_color);
	se_sdf_control_bind_ptr_vec3(renderer, light_dir_control, &control_light_dir);
	se_sdf_control_bind_ptr_vec3(renderer, light_color_control, &control_light_color);
	se_sdf_control_bind_ptr_vec3(renderer, fog_color_control, &control_fog_color);
	se_sdf_control_bind_ptr_float(renderer, fog_density_control, &control_fog_density);
	se_sdf_control_bind_ptr_float(renderer, stylized_band_levels_control, &control_stylized_band_levels);
	se_sdf_control_bind_ptr_float(renderer, stylized_rim_power_control, &control_stylized_rim_power);
	se_sdf_control_bind_ptr_float(renderer, stylized_rim_strength_control, &control_stylized_rim_strength);
	se_sdf_control_bind_ptr_float(renderer, stylized_fill_strength_control, &control_stylized_fill_strength);
	se_sdf_control_bind_ptr_float(renderer, stylized_back_strength_control, &control_stylized_back_strength);
	se_sdf_control_bind_ptr_float(renderer, stylized_checker_scale_control, &control_stylized_checker_scale);
	se_sdf_control_bind_ptr_float(renderer, stylized_checker_strength_control, &control_stylized_checker_strength);
	se_sdf_control_bind_ptr_float(renderer, stylized_ground_blend_control, &control_stylized_ground_blend);
	se_sdf_control_bind_ptr_float(renderer, stylized_desaturate_control, &control_stylized_desaturate);
	se_sdf_control_bind_ptr_float(renderer, stylized_gamma_control, &control_stylized_gamma);
	if (hero_node != SE_SDF_NODE_NULL) {
		se_sdf_control_bind_node_translation(renderer, scene, hero_node, hero_translation_control);
		se_sdf_control_bind_node_rotation(renderer, scene, hero_node, hero_rotation_control);
		se_sdf_control_bind_node_scale(renderer, scene, hero_node, hero_scale_control);
		se_sdf_control_bind_primitive_param_float(renderer, scene, hero_node, SE_SDF_PRIMITIVE_PARAM_RADIUS, hero_radius_control);
	}
	se_sdf_control_bind_material_base_color(renderer, base_color_control);
	se_sdf_control_bind_lighting_direction(renderer, light_dir_control);
	se_sdf_control_bind_lighting_color(renderer, light_color_control);
	se_sdf_control_bind_fog_color(renderer, fog_color_control);
	se_sdf_control_bind_fog_density(renderer, fog_density_control);
	se_sdf_control_bind_stylized_band_levels(renderer, stylized_band_levels_control);
	se_sdf_control_bind_stylized_rim_power(renderer, stylized_rim_power_control);
	se_sdf_control_bind_stylized_rim_strength(renderer, stylized_rim_strength_control);
	se_sdf_control_bind_stylized_fill_strength(renderer, stylized_fill_strength_control);
	se_sdf_control_bind_stylized_back_strength(renderer, stylized_back_strength_control);
	se_sdf_control_bind_stylized_checker_scale(renderer, stylized_checker_scale_control);
	se_sdf_control_bind_stylized_checker_strength(renderer, stylized_checker_strength_control);
	se_sdf_control_bind_stylized_ground_blend(renderer, stylized_ground_blend_control);
	se_sdf_control_bind_stylized_desaturate(renderer, stylized_desaturate_control);
	se_sdf_control_bind_stylized_gamma(renderer, stylized_gamma_control);

	s_vec2 resolution = s_vec2((f32)WINDOW_WIDTH, (f32)WINDOW_HEIGHT);

	while (!se_window_should_close(window)) {
		se_window_tick(window);

		control_time = (f32)se_window_get_time(window);
		se_window_get_mouse_position_normalized(window, &control_mouse);
		control_pulse = 0.5f + 0.5f * sinf(control_time * 2.4f);
		control_hero_radius = 0.72f + 0.14f * (0.5f + 0.5f * sinf(control_time * 1.7f));
		se_sdf_control_set_float(renderer, pulse_control, control_pulse);
		se_sdf_control_set_float(renderer, hero_radius_control, control_hero_radius);
		control_hero_translation = s_vec3(0.0f, 0.08f + 0.22f * sinf(control_time * 2.1f), 0.0f);
		control_hero_rotation = s_vec3(0.0f, control_time * 0.9f, 0.0f);
		control_hero_scale = s_vec3(
			1.0f + 0.10f * sinf(control_time * 1.6f),
			1.0f + 0.16f * sinf(control_time * 2.0f),
			1.0f + 0.10f * sinf(control_time * 1.3f)
		);
		control_base_color = s_vec3(
			0.55f + 0.20f * (0.5f + 0.5f * sinf(control_time * 0.8f)),
			0.60f + 0.18f * (0.5f + 0.5f * sinf(control_time * 1.0f + 0.7f)),
			0.70f + 0.15f * (0.5f + 0.5f * sinf(control_time * 0.9f + 1.5f))
		);
		control_light_dir = s_vec3(cosf(control_time * 0.35f), 0.65f, sinf(control_time * 0.35f));
		control_light_color = s_vec3(0.95f, 0.96f, 1.0f);
		control_fog_color = s_vec3(0.10f, 0.14f, 0.20f);
		control_fog_density = 0.0009f + 0.0005f * (0.5f + 0.5f * sinf(control_time * 0.55f));
		control_stylized_band_levels = 3.0f + floorf(2.0f + 2.0f * (0.5f + 0.5f * sinf(control_time * 0.45f)));
		control_stylized_rim_power = 1.5f + 2.2f * (0.5f + 0.5f * sinf(control_time * 0.60f));
		control_stylized_rim_strength = 0.30f + 0.55f * (0.5f + 0.5f * sinf(control_time * 1.05f));
		control_stylized_fill_strength = 0.22f + 0.62f * (0.5f + 0.5f * sinf(control_time * 0.73f + 0.4f));
		control_stylized_back_strength = 0.10f + 0.45f * (0.5f + 0.5f * sinf(control_time * 0.81f + 1.0f));
		control_stylized_checker_scale = 1.4f + 2.0f * (0.5f + 0.5f * sinf(control_time * 0.48f));
		control_stylized_checker_strength = 0.08f + 0.42f * control_pulse;
		control_stylized_ground_blend = 0.18f + 0.52f * (0.5f + 0.5f * sinf(control_time * 0.62f));
		control_stylized_desaturate = 0.02f + 0.28f * (0.5f + 0.5f * sinf(control_time * 0.37f + 0.8f));
		control_stylized_gamma = 1.9f + 0.8f * (0.5f + 0.5f * sinf(control_time * 0.29f));

		se_sdf_frame_desc frame = SE_SDF_FRAME_DESC_DEFAULTS;
		frame.resolution = resolution;
		frame.time_seconds = control_time;
		frame.mouse_normalized = control_mouse;
		frame.has_camera_override = true;
		frame.camera_position = s_vec3(14.0f * cosf(control_time * 0.18f), 6.5f, 14.0f * sinf(control_time * 0.18f));
		frame.camera_target = s_vec3(0.0f, 0.0f, 0.0f);

		se_sdf_renderer_rebuild_if_dirty(renderer);
		se_render_clear();
		se_sdf_renderer_render(renderer, &frame);
		se_window_present(window);
	}

	se_sdf_renderer_destroy(renderer);
	se_sdf_scene_destroy(scene);
	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
