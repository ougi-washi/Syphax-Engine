// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_debug.h"
#include "se_graphics.h"
#include "se_sdf.h"
#include "se_window.h"

i32 main(void) {
	u32 frame_counter = 0u;
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
			.bias = 0.46f,
			.smoothness = 0.18f,
		},
		.shadow = {
			.softness = 10.0f,
			.bias = 0.025f,
			.samples = 56,
		},
		.sphere = {.radius = 1.0f});
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
			.box = {.size = {28.0f, 0.01f, 28.0f}});
		const s_vec3 extra_positions[10] = {
			{-6.0f, 0.0f, 2.5f}, {-3.0f, 0.0f, -2.0f}, {2.5f, 0.0f, 3.5f}, {6.5f, 0.0f, -4.5f}, {9.5f, 0.0f, 5.5f},
			{-15.0f, 0.0f, 10.0f}, {-10.0f, 0.0f, 15.0f}, {0.0f, 0.0f, 19.0f}, {11.5f, 0.0f, 13.0f}, {18.0f, 0.0f, 21.0f},
		};
		const s_vec3 extra_sizes[10] = {
			{0.55f, 0.55f, 0.55f}, {0.70f, 0.90f, 0.70f}, {0.82f, 0.82f, 0.82f}, {0.58f, 1.20f, 0.58f}, {1.05f, 1.05f, 1.05f},
			{0.90f, 0.90f, 0.90f}, {1.10f, 1.55f, 1.10f}, {1.35f, 1.35f, 1.35f}, {0.85f, 1.95f, 0.85f}, {1.60f, 1.60f, 1.60f},
		};
	se_sdf_point_light_handle point_light = se_sdf_add_point_light(scene,
		.position = {2.5f, 3.0f, -2.0f},
		.color = {1.0f, 1.0f, 0.10f},
		.radius = 9.0f,
	);
	se_sdf_directional_light_handle sun = se_sdf_add_directional_light(scene,
		.direction = {0.45f, 0.85f, 0.35f},
		.color = {0.82f, 0.90f, 1.0f},
	);

	se_sdf_set_position(sphere, &s_vec3(0.0f, 1.0f, 0.0f));
	se_sdf_add_noise(sphere,
		.type = SE_SDF_NOISE_PERLIN,
		.frequency = 2.0f,
		.offset = {0.25f, 0.0f, 0.0f},
	);
	se_sdf_set_shading_smoothness(sphere, 0.14f);
	se_sdf_set_shadow_softness(sphere, 12.0f);
	se_sdf_point_light_set_radius(point_light, 9.0f);
	se_sdf_directional_light_set_direction(sun, &s_vec3(0.45f, 0.85f, 0.35f));
		se_sdf_set_shadow_samples(ground, 48);
		se_sdf_add_child(scene, ground);
		se_sdf_add_child(scene, sphere);
		for (u32 i = 0u; i < 10u; ++i) {
			const b8 is_box = (i % 2u) != 0u;
			const s_vec3 size = extra_sizes[i];
			s_vec3 position = extra_positions[i];
			position.y = is_box ? size.y : size.x;
			se_sdf_handle extra = is_box
				? se_sdf_create(.type = SE_SDF_BOX, .shading = {.ambient = {0.04f, 0.03f, 0.05f}, .diffuse = {0.32f, 0.58f, 0.88f}, .specular = {0.25f, 0.25f, 0.28f}, .roughness = 0.72f, .bias = 0.34f, .smoothness = 0.15f}, .shadow = {.softness = 8.0f, .bias = 0.02f, .samples = 28}, .box = {.size = size})
				: se_sdf_create(.type = SE_SDF_SPHERE, .shading = {.ambient = {0.08f, 0.04f, 0.03f}, .diffuse = {0.92f, 0.52f, 0.24f}, .specular = {0.48f, 0.42f, 0.36f}, .roughness = 0.40f, .bias = 0.40f, .smoothness = 0.16f}, .shadow = {.softness = 8.0f, .bias = 0.02f, .samples = 28}, .sphere = {.radius = size.x});
			se_sdf_set_position(extra, &position);
			se_sdf_add_child(scene, extra);
		}

	se_debug_set_overlay_enabled(false);

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_camera_set_window_aspect(camera, window);
		se_render_clear();
		se_sdf_render(scene, camera);
		se_window_end_frame(window);
		frame_counter++;
		if ((frame_counter % 60u) == 0u) {
			se_debug_frame_timing timing = {0};
			se_debug_trace_stat trace_stats[8] = {0};
			u32 trace_count = 0u;
			if (se_debug_get_frame_timing(&timing)) {
				se_log(
					"sdf :: frame=%u frame_ms=%.3f fps=%.2f target144=%s present=%.3f other=%.3f",
					frame_counter,
					timing.frame_ms,
					timing.frame_ms > 0.0 ? 1000.0 / timing.frame_ms : 0.0,
					timing.frame_ms <= (1000.0 / 144.0) ? "yes" : "no",
					timing.window_present_ms,
					timing.other_ms);
			}
			if (se_debug_get_trace_stats(trace_stats, 8u, &trace_count, true)) {
				for (u32 i = 0; i < trace_count && i < 8u; ++i) {
					se_log(
						"sdf :: trace %s[%s] %.3fms",
						trace_stats[i].name,
						se_debug_trace_channel_name((se_debug_trace_channel)trace_stats[i].channel),
						trace_stats[i].total_ms);
				}
			}
		}
	}

	se_context_destroy(context);
	return 0;
}
