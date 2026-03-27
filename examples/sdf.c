// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_sdf.h"
#include "se_window.h"

i32 main() {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - SDF", 1280, 720);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.05f, 0.06f, 0.08f, 1.0f));

	se_camera_handle camera = se_camera_create();
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 50.0f, 0.05f, 100.0f);
	se_camera_set_location(camera, &s_vec3(0.0f, 5.0f, -10.0f));	
	se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));

	// scene
	se_sdf_handle scene = se_sdf_create(
		.operation = SE_SDF_SMOOTH_UNION,
		.operation_amount = 0.35f);

	se_sdf_handle sphere = se_sdf_create(
		.type = SE_SDF_SPHERE,
		.shading = {
			.ambient = {0.09f, 0.03f, 0.05f},
			.diffuse = {0.92f, 0.28f, 0.42f},
			.specular = {0.80f, 0.74f, 0.78f},
			.roughness = 0.25f,
			.shadow_bias = 0.025f,
			.shadow_smooothness = 1.0f,
		},
		.sphere = {.radius = 1.0f});
	se_sdf_set_position(sphere, &s_vec3(0.0f, 1.0f, 0.0f));
	se_sdf_add_noise(sphere,
		.type = SE_SDF_NOISE_PERLIN,
		.frequency = 1.25f,
		.offset = {0.0f, 0.0f, 0.0f},
	);

	se_sdf_handle ground = se_sdf_create(
		.type = SE_SDF_BOX,
		.shading = {
			.ambient = {0.04f, 0.04f, 0.05f},
			.diffuse = {0.28f, 0.31f, 0.34f},
			.specular = {0.05f, 0.05f, 0.06f},
			.roughness = 0.92f,
			.shadow_bias = 0.02f,
			.shadow_smooothness = 1.0f,
		},
		.box = {.size = {10.0f, 0.01f, 10.0f}});

	// lights
	se_sdf_point_light_handle orb_light = se_sdf_add_point_light(scene,
		.position = {2.5f, 3.0f, -2.0f},
		.color = {1.0f, 0.2f, 0.1f},
		.radius = 9.0f,
	);
	se_sdf_add_directional_light(scene,
		.direction = {0.45f, 0.85f, 0.35f},
		.color = {0.82f, 0.90f, 1.0f},
	);
	se_sdf_point_light_set_radius(orb_light, 9.0f);

	se_sdf_add_child(scene, ground);
	se_sdf_add_child(scene, sphere);

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_camera_set_window_aspect(camera, window);

		se_render_clear();
		se_sdf_render(scene, camera);

		se_window_end_frame(window);
	}

	se_context_destroy(context);
	return 0;
}
