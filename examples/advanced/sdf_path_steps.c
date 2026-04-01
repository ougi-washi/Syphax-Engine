// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_sdf.h"
#include "se_window.h"

#include <stdlib.h>

typedef enum se_docs_sdf_step {
	SE_DOCS_SDF_STEP_1 = 1,
	SE_DOCS_SDF_STEP_2 = 2,
	SE_DOCS_SDF_STEP_3 = 3,
	SE_DOCS_SDF_STEP_4 = 4,
} se_docs_sdf_step;

static se_docs_sdf_step se_docs_sdf_step_from_env(void) {
	const c8* value = getenv("SE_DOCS_SDF_STEP");
	if (value == NULL || value[0] == '\0') {
		return SE_DOCS_SDF_STEP_4;
	}

	i32 parsed = atoi(value);
	if (parsed < (i32)SE_DOCS_SDF_STEP_1 || parsed > (i32)SE_DOCS_SDF_STEP_4) {
		return SE_DOCS_SDF_STEP_4;
	}
	return (se_docs_sdf_step)parsed;
}

i32 main(void) {
	const se_docs_sdf_step docs_step = se_docs_sdf_step_from_env();
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - SDF Path Steps", 1280, 720);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.05f, 0.06f, 0.08f, 1.0f));

	se_camera_handle camera = se_camera_create();
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 50.0f, 0.05f, 100.0f);
	if (docs_step == SE_DOCS_SDF_STEP_1) {
		se_camera_set_location(camera, &s_vec3(0.0f, 2.5f, -6.0f));
		se_camera_set_target(camera, &s_vec3(0.0f, 1.0f, 0.0f));
	} else {
		se_camera_set_location(camera, &s_vec3(0.0f, 5.0f, -10.0f));
		se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));
	}

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
	se_sdf_handle render_target = sphere;

	if (docs_step >= SE_DOCS_SDF_STEP_2) {
		se_sdf_handle scene = se_sdf_create(
			.operation = SE_SDF_SMOOTH_UNION,
			.operation_amount = 0.35f);
		se_sdf_handle ground = se_sdf_create(
			.type = SE_SDF_BOX,
			.shading = {
				.ambient = {0.04f, 0.04f, 0.05f},
				.diffuse = {0.28f, 0.31f, 0.34f},
				.specular = {0.05f, 0.05f, 0.06f},
				.roughness = 0.92f,
				.bias = 0.32f,
				.smoothness = 0.30f,
			},
			.shadow = {
				.softness = 6.5f,
				.bias = 0.02f,
				.samples = 40,
			},
			.box = {.size = {10.0f, 0.01f, 10.0f}});

		se_sdf_add_child(scene, ground);
		se_sdf_add_child(scene, sphere);
		render_target = scene;

		if (docs_step >= SE_DOCS_SDF_STEP_3) {
			se_sdf_noise_handle surface_noise = se_sdf_add_noise(sphere,
				.type = SE_SDF_NOISE_PERLIN,
				.frequency = 1.25f,
				.offset = {0.0f, 0.0f, 0.0f},
			);
			se_sdf_point_light_handle orb_light = se_sdf_add_point_light(scene,
				.position = {2.5f, 3.0f, -2.0f},
				.color = {1.0f, 1.0f, 0.10f},
				.radius = 9.0f,
			);

			se_sdf_noise_set_frequency(surface_noise, 2.0f);
			se_sdf_noise_set_offset(surface_noise, &s_vec3(0.25f, 0.0f, 0.0f));
			se_sdf_point_light_set_position(orb_light, &s_vec3(3.0f, 4.0f, -2.0f));
			se_sdf_set_shading_smoothness(sphere, 0.14f);
			se_sdf_set_shadow_softness(sphere, 12.0f);

			if (docs_step >= SE_DOCS_SDF_STEP_4) {
				se_sdf_directional_light_handle sun = se_sdf_add_directional_light(scene,
					.direction = {0.45f, 0.85f, 0.35f},
					.color = {0.82f, 0.90f, 1.0f},
				);
				se_sdf_point_light_set_radius(orb_light, 9.0f);
				se_sdf_directional_light_set_direction(sun, &s_vec3(0.45f, 0.85f, 0.35f));
				se_sdf_set_shadow_samples(ground, 48);
			}
		}
	}

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_camera_set_window_aspect(camera, window);
		se_render_clear();
		se_sdf_render_to_window(render_target, camera, window, 0.05f);
		se_window_end_frame(window);
	}

	se_context_destroy(context);
	return 0;
}
