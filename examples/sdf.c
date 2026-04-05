// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_debug.h"
#include "se_defines.h"
#include "se_graphics.h"
#include "se_noise.h"
#include "se_sdf.h"
#include "se_window.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"

static s_json* sdf_scene_json_load(void) {
	c8 path[SE_MAX_PATH_LENGTH] = {0};
	c8* text = NULL;
	s_json* root = NULL;
	if (!se_paths_resolve_resource_path(path, sizeof(path), SE_RESOURCE_EXAMPLE("sdf/scene.json")) ||
		!s_file_read(path, &text, NULL)) {
		return NULL;
	}
	root = s_json_parse(text);
	free(text);
	return root;
}

i32 main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - SDF", 1280, 720);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 144);
	se_window_set_vsync(window, false);
	se_render_set_background(s_vec4(0.05f, 0.06f, 0.08f, 1.0f));

	se_camera_handle camera = se_camera_create();
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 50.0f, 0.05f, 100.0f);
	se_camera_set_location(camera, &s_vec3(0.0f, 5.0f, -10.0f));
	se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));

	se_sdf_handle scene = se_sdf_create();
	s_json* scene_json = sdf_scene_json_load();
	if (scene == SE_SDF_NULL || !scene_json || !se_sdf_from_json(scene, scene_json)) {
		s_json_free(scene_json);
		se_context_destroy(context);
		return 1;
	}
	s_json_free(scene_json);

	se_sdf_add_point_light(scene,
		.position = {2.5f, 3.0f, -2.0f},
		.color = {1.0f, 1.0f, 0.10f},
		.radius = 9.0f,
	);
	se_sdf_add_directional_light(scene,
		.direction = {0.45f, 0.85f, 0.35f},
		.color = {0.82f, 0.90f, 1.0f},
	);

	se_sdf_handle sphere = se_sdf_create(
		.type = SE_SDF_SPHERE,
		.shading = {
			.ambient = {0.09f, 0.03f, 0.05f},
			.diffuse = {0.92f, 0.28f, 0.42f},
			.specular = {0.80f, 0.74f, 0.78f},
			.roughness = 0.25f,
			.bias = 0.46f,
			.smoothness = 0.18f,
		},
		.shadow = {
			.softness = 10.0f,
			.bias = 0.025f,
			.samples = 56,
		},
		.sphere = {.radius = 1.0f});

	se_sdf_set_position(sphere, &s_vec3(0.0f, 1.0f, 0.0f));
	se_sdf_add_noise(sphere,
		.type = SE_NOISE_PERLIN,
		.frequency = 2.0f,
		.offset = s_vec2(0.25f, 0.0f),
		.scale = s_vec2(1.0f, 1.0f),
		.seed = 7u
	);
	se_sdf_set_shading_smoothness(sphere, 0.14f);
	se_sdf_set_shadow_softness(sphere, 12.0f);
	se_sdf_add_child(scene, sphere);

	se_debug_set_overlay_enabled(true);

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);

		const f64 curr_time = se_window_get_time(window);
		s_vec3 camera_pos;
		se_camera_get_location(camera, &camera_pos);
		camera_pos.x += sin(curr_time * 0.1) * .001;
		camera_pos.y += cos(curr_time * 0.1) * .001;
		camera_pos.z += sin(cos(curr_time * 0.1)) * .001; 
		se_camera_set_location(camera, &camera_pos);

		f64 stepped_time = floor(curr_time * 4.) / 4.;
		se_sdf_set_position(sphere, &s_vec3(sin(stepped_time) * 5., 1, cos(stepped_time) * 5.));		
		se_camera_set_window_aspect(camera, window);
		se_render_clear();
		se_sdf_render_to_window(scene, camera, window, .1f);
		se_window_end_frame(window);
	}

	se_context_destroy(context);
	return 0;
}
