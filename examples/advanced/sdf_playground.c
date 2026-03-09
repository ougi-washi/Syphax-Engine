// Syphax-Engine - Ougi Washi

#include <stdio.h>
#include <stdlib.h>

#include "se.h"
#include "se_window.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_scene.h"
#include "se_physics.h"
#include "se_sdf.h"

typedef struct {
	se_window_handle window;
	se_camera_handle camera;
	se_sdf_renderer_handle renderer;
	b8 render_failed;
} sdf_playground_present_state;

static void sdf_playground_render_scene(se_scene_3d_handle scene, void* user_data) {
	(void)scene;
	sdf_playground_present_state* present = user_data;
	if (!present || present->window == S_HANDLE_NULL || present->camera == S_HANDLE_NULL || present->renderer == SE_SDF_RENDERER_NULL) {
		return;
	}
	u32 width = 0u;
	u32 height = 0u;
	se_window_get_framebuffer_size(present->window, &width, &height);
	se_sdf_frame_desc frame = SE_SDF_FRAME_DESC_DEFAULTS;
	frame.camera = present->camera;
	frame.resolution = s_vec2((f32)s_max(width, 1u), (f32)s_max(height, 1u));
	frame.time_seconds = (f32)se_window_get_time(present->window);
	if (!se_sdf_renderer_render(present->renderer, &frame)) {
		const se_sdf_build_diagnostics diagnostics = se_sdf_renderer_get_build_diagnostics(present->renderer);
		fprintf(stderr, "sdf_playground :: render failed at stage '%s': %s\n", diagnostics.stage, diagnostics.message);
		present->render_failed = true;
		se_window_set_should_close(present->window, true);
	}
}

int main(void) {
	const b8 lightweight_runtime = getenv("SE_TERMINAL_HIDE_WINDOW") != NULL || getenv("SE_DOCS_CAPTURE_PATH") != NULL;
	enum {
		ball_capacity = 16
	};
	const i32 ball_columns = lightweight_runtime ? 2 : 4;
	const i32 ball_rows = lightweight_runtime ? 2 : 4;
	const i32 ball_count = ball_columns * ball_rows;
	const b8 stats_enabled = getenv("SE_SDF_EXAMPLE_STATS") != NULL;
	f32 last_stats_print = -1000.0f;
	se_context* context = se_context_create();
	se_window_handle window = se_window_create(
		"Syphax-Engine - sdf_playground",
		lightweight_runtime ? 640 : 1280,
		lightweight_runtime ? 360 : 720
	);
	if (window == S_HANDLE_NULL) {
		fprintf(stderr, "sdf_playground :: failed to create window\n");
		se_context_destroy(context);
		return 1;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_vsync(window, false);
	se_window_set_target_fps(window, lightweight_runtime ? 240 : 60);
	se_render_set_background(s_vec4(0.03f, 0.04f, 0.05f, 1.0f));

	se_scene_3d_handle camera_scene = se_scene_3d_create_for_window(window, 1);
	se_camera_handle camera = se_scene_3d_get_camera(camera_scene);
	se_camera_set_target_mode(camera, true);
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 52.0f, 0.05f, 200.0f);
	se_camera_set_location(camera, lightweight_runtime ? &s_vec3(7.6f, 5.0f, 9.2f) : &s_vec3(11.5f, 7.8f, 15.0f));
	se_camera_set_target(camera, lightweight_runtime ? &s_vec3(0.0f, 2.0f, 0.0f) : &s_vec3(0.0f, 1.5f, 0.0f));

	const f32 ball_radius = 0.45f;
	const f32 ball_spacing = 2.35f;
	const f32 physics_step = lightweight_runtime ? (1.0f / 180.0f) : (1.0f / 120.0f);
	const f32 physics_max_catchup = physics_step * 8.0f;
	const s_vec3 ground_size = lightweight_runtime ? s_vec3(8.5f, 0.20f, 8.5f) : s_vec3(14.0f, 0.20f, 14.0f);
	const s_vec3 ground_center = lightweight_runtime ? s_vec3(0.0f, -2.4f, 0.0f) : s_vec3(0.0f, -1.0f, 0.0f);
	s_vec3 ball_starts[ball_capacity];
	se_sdf_node_handle ball_refs[ball_capacity];
	se_physics_body_3d_handle ball_bodies[ball_capacity];
	s_mat4 ground_transform = s_mat4_identity;
	s_mat4_set_translation(&ground_transform, &ground_center);

	se_sdf_handle sdf = se_sdf_create(NULL);
	se_sdf_handle ball_sdf = se_sdf_create(NULL);
	se_sdf_prepare_desc root_prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_sdf_prepare_desc ball_prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	root_desc.operation_amount = 1.0f;
	se_sdf_node_handle root = se_sdf_node_create_group(sdf, &root_desc);
	se_sdf_set_root(sdf, root);

	se_sdf_node_primitive_desc ground_node_desc = {
		.transform = ground_transform,
		.operation = SE_SDF_OP_UNION,
		.operation_amount = 1.f,
		.primitive = {
			.type = SE_SDF_PRIMITIVE_BOX,
			.box = {
				.size = ground_size
			}
		}
	};
	se_sdf_node_primitive_desc ball_node_desc = {
		.transform = s_mat4_identity,
		.operation = SE_SDF_OP_UNION,
		.primitive = {
			.type = SE_SDF_PRIMITIVE_SPHERE,
			.sphere = {
				.radius = ball_radius
			}
		}
	};
	se_sdf_node_handle ground_node = se_sdf_node_create_primitive(sdf, &ground_node_desc);
	se_sdf_node_set_color(sdf, ground_node, &s_vec3(0.22f, 0.26f, 0.32f));
	se_sdf_node_add_child(sdf, root, ground_node);
	const se_sdf_noise ground_noise = {
		.active = !lightweight_runtime,
		.scale = .5f,
		.offset = 0.0f,
		.frequency = 2.f
	};
	se_sdf_node_set_noise(sdf, ground_node, &ground_noise);

	se_sdf_node_group_desc ball_root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	se_sdf_node_handle ball_root = se_sdf_node_create_group(ball_sdf, &ball_root_desc);
	se_sdf_set_root(ball_sdf, ball_root);
	se_sdf_node_handle ball_shape = se_sdf_node_create_primitive(ball_sdf, &ball_node_desc);
	se_sdf_node_set_color(ball_sdf, ball_shape, &s_vec3(0.92f, 0.18f, 0.16f));
	se_sdf_node_add_child(ball_sdf, ball_root, ball_shape);

	se_sdf_renderer_handle renderer = se_sdf_renderer_create(NULL);
	sdf_playground_present_state present_state = {0};
	se_sdf_renderer_set_sdf(renderer, sdf);
	{
		se_sdf_raymarch_quality quality = SE_SDF_RAYMARCH_QUALITY_DEFAULTS;
		quality.max_steps = lightweight_runtime ? 28 : 56;
		quality.hit_epsilon = lightweight_runtime ? 0.0035f : 0.0025f;
		quality.enable_shadows = 0;
		quality.shadow_steps = 0;
		quality.shadow_strength = 0.0f;
		(void)se_sdf_renderer_set_quality(renderer, &quality);
	}
	(void)se_sdf_renderer_set_light_rig(
		renderer,
		&s_vec3(-0.55f, -0.85f, -0.20f),
		&s_vec3(1.0f, 0.98f, 0.94f),
		&s_vec3(0.09f, 0.12f, 0.16f),
		0.03f
	);
	for (i32 row = 0; row < ball_rows; ++row) {
		for (i32 column = 0; column < ball_columns; ++column) {
			const i32 index = row * ball_columns + column;
			ball_starts[index] = s_vec3(
				((f32)column - ((f32)ball_columns - 1.0f) * 0.5f) * ball_spacing,
				2.0f + (f32)row * 0.9f,
				((f32)row - ((f32)ball_rows - 1.0f) * 0.5f) * ball_spacing
			);
			se_sdf_node_ref_desc ball_ref_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
			ball_ref_desc.source = ball_sdf;
			ball_ref_desc.transform = se_sdf_transform(ball_starts[index], s_vec3(0.0f, 0.0f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
			ball_refs[index] = se_sdf_node_create_ref(sdf, &ball_ref_desc);
			se_sdf_node_add_child(sdf, root, ball_refs[index]);
		}
	}
	root_prepare_desc.lod_count = lightweight_runtime ? 1u : 2u;
	root_prepare_desc.lod_resolutions[0] = lightweight_runtime ? 12u : 28u;
	root_prepare_desc.lod_resolutions[1] = lightweight_runtime ? 12u : 14u;
	root_prepare_desc.lod_resolutions[2] = lightweight_runtime ? 8u : 8u;
	root_prepare_desc.lod_resolutions[3] = lightweight_runtime ? 8u : 8u;
	root_prepare_desc.brick_size = lightweight_runtime ? 6u : 8u;
	root_prepare_desc.brick_border = 1u;
	ball_prepare_desc = root_prepare_desc;
	if (lightweight_runtime) {
		ball_prepare_desc.lod_resolutions[0] = 18u;
		ball_prepare_desc.lod_resolutions[1] = 10u;
		ball_prepare_desc.brick_size = 6u;
	}
	(void)se_sdf_prepare(ball_sdf, &ball_prepare_desc);
	(void)se_sdf_prepare(sdf, &root_prepare_desc);
	present_state.window = window;
	present_state.camera = camera;
	present_state.renderer = renderer;
	(void)se_scene_3d_register_custom_render(camera_scene, sdf_playground_render_scene, &present_state);

	se_sdf_handle ground_physics_scene = SE_SDF_NULL;
	se_sdf_physics_handle ground_physics = SE_SDF_PHYSICS_NULL;
	if (ground_noise.active) {
		ground_physics_scene = se_sdf_create(NULL);
		se_sdf_node_handle ground_physics_root = se_sdf_node_create_group(ground_physics_scene, &root_desc);
		se_sdf_set_root(ground_physics_scene, ground_physics_root);
		se_sdf_node_handle ground_physics_node = se_sdf_node_create_primitive(ground_physics_scene, &ground_node_desc);
		se_sdf_node_add_child(ground_physics_scene, ground_physics_root, ground_physics_node);
		se_sdf_node_set_noise(ground_physics_scene, ground_physics_node, &ground_noise);
		ground_physics = se_sdf_physics_create(ground_physics_scene);
	}

	se_physics_world_params_3d world_params = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS;
	world_params.gravity = s_vec3(0.0f, -9.81f, 0.0f);
	world_params.shapes_per_body = 24;
	world_params.contacts_count = 2048;
	world_params.solver_iterations = lightweight_runtime ? 16u : 12u;
	se_physics_world_3d_handle physics_world = se_physics_world_3d_create(&world_params);
	se_physics_body_params_3d ground_body_params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	ground_body_params.type = SE_PHYSICS_BODY_STATIC;
	ground_body_params.position = s_vec3(0.0f, 0.0f, 0.0f);
	se_physics_body_3d_handle ground_body = se_physics_body_3d_create(physics_world, &ground_body_params);
	(void)ground_body;
	if (ground_noise.active) {
		(void)se_sdf_physics_add_shape_3d(
			ground_physics,
			physics_world,
			ground_body,
			&s_vec3(0.0f, 0.0f, 0.0f),
			&s_vec3(0.0f, 0.0f, 0.0f),
			false
		);
	} else {
		(void)se_physics_body_3d_add_box(
			physics_world,
			ground_body,
			&ground_center,
			&ground_size,
			&s_vec3(0.0f, 0.0f, 0.0f),
			false
		);
	}

	for (i32 i = 0; i < ball_count; ++i) {
		se_physics_body_params_3d ball_body_params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
		ball_body_params.type = SE_PHYSICS_BODY_DYNAMIC;
		ball_body_params.position = ball_starts[i];
		ball_body_params.mass = 1.0f;
		ball_body_params.friction = 0.7f;
		ball_body_params.restitution = 0.55f;
		ball_body_params.linear_damping = 0.02f;
		ball_body_params.angular_damping = 0.04f;
		ball_bodies[i] = se_physics_body_3d_create(physics_world, &ball_body_params);
		(void)se_physics_body_3d_add_sphere(
			physics_world,
			ball_bodies[i],
			&s_vec3(0.0f, 0.0f, 0.0f),
			ball_radius,
			false
		);
	}

	f32 physics_accumulator = 0.0f;
	se_window_loop(window,
		const f32 dt = (f32)se_window_get_delta_time(window);
		if (dt > 0.0f) {
			physics_accumulator = fminf(physics_accumulator + dt, physics_max_catchup);
			while (physics_accumulator >= physics_step) {
				se_physics_world_3d_step(physics_world, physics_step);
				physics_accumulator -= physics_step;
			}
		}

		for (i32 i = 0; i < ball_count; ++i) {
			const s_vec3 ball_position = se_physics_body_3d_get_position(physics_world, ball_bodies[i]);
			const s_mat4 ball_transform = se_sdf_transform(ball_position, s_vec3(0.0f, 0.0f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
			(void)se_sdf_node_set_transform(sdf, ball_refs[i], &ball_transform);
		}

		u32 fb_w = 0;
		u32 fb_h = 0;
		se_window_get_framebuffer_size(window, &fb_w, &fb_h);
		const f32 aspect_w = fb_w > 0 ? (f32)fb_w : 1280.0f;
		const f32 aspect_h = fb_h > 0 ? (f32)fb_h : 720.0f;
		const f32 current_time = (f32)se_window_get_time(window);
		se_camera_set_aspect(camera, aspect_w, aspect_h);

		se_scene_3d_draw(camera_scene, window);
		if (stats_enabled && current_time - last_stats_print >= 1.0f) {
			const se_sdf_renderer_stats stats = se_sdf_renderer_get_stats(renderer);
			fprintf(
				stderr,
				"sdf_playground :: visible=%u resident=%u misses=%u lods=[%u,%u,%u,%u]\n",
				stats.visible_refs,
				stats.resident_bricks,
				stats.page_misses,
				stats.selected_lod_counts[0],
				stats.selected_lod_counts[1],
				stats.selected_lod_counts[2],
				stats.selected_lod_counts[3]
			);
			last_stats_print = current_time;
		}
	);

	se_physics_world_3d_destroy(physics_world);
	if (ground_physics != SE_SDF_PHYSICS_NULL) {
		se_sdf_physics_destroy(ground_physics);
	}
	if (ground_physics_scene != SE_SDF_NULL) {
		se_sdf_destroy(ground_physics_scene);
	}
	se_sdf_renderer_destroy(renderer);
	se_sdf_destroy(ball_sdf);
	se_sdf_destroy(sdf);
	se_scene_3d_destroy(camera_scene);
	se_sdf_shutdown();
	se_context_destroy(context);
	return 0;
}
