// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_debug.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_sdf.h"
#include "se_window.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define STREAM_CELL_SIZE 18.0f
#define STREAM_RADIUS 5

typedef struct {
	se_window_handle window;
	se_scene_3d_handle scene;
	se_camera_handle camera;
	se_sdf_renderer_handle renderer;
	se_sdf_handle chunk_sdf;
	se_sdf_handle root_sdf;
	i32 chunk_origin_x;
	i32 chunk_origin_z;
	i32 stream_radius;
	se_sdf_prepare_desc prepare_desc;
	b8 chunks_ready;
} sdf_stream_app;

static void sdf_stream_render_scene(se_scene_3d_handle scene, void* user_data) {
	(void)scene;
	sdf_stream_app* app = user_data;
	if (!app || app->window == S_HANDLE_NULL || app->camera == S_HANDLE_NULL || app->renderer == SE_SDF_RENDERER_NULL) {
		return;
	}
	u32 width = 0u;
	u32 height = 0u;
	se_window_get_framebuffer_size(app->window, &width, &height);
	se_sdf_frame_desc frame = SE_SDF_FRAME_DESC_DEFAULTS;
	frame.camera = app->camera;
	frame.resolution = s_vec2((f32)s_max(width, 1u), (f32)s_max(height, 1u));
	frame.time_seconds = (f32)se_window_get_time(app->window);
	if (!se_sdf_renderer_render(app->renderer, &frame)) {
		const se_sdf_build_diagnostics diagnostics = se_sdf_renderer_get_build_diagnostics(app->renderer);
		fprintf(stderr, "sdf_root_streaming :: render failed at stage '%s': %s\n", diagnostics.stage, diagnostics.message);
		se_window_set_should_close(app->window, true);
	}
}

static s_mat4 sdf_stream_chunk_transform(const i32 x, const i32 z) {
	s_mat4 transform = s_mat4_identity;
	s_mat4_set_translation(&transform, &s_vec3((f32)x * STREAM_CELL_SIZE, 0.0f, (f32)z * STREAM_CELL_SIZE));
	return transform;
}

static b8 sdf_stream_build_chunk(se_sdf_handle chunk_sdf, const se_sdf_prepare_desc* prepare_desc) {
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	se_sdf_node_handle root = SE_SDF_NODE_NULL;
	if (chunk_sdf == SE_SDF_NULL) {
		return false;
	}
	se_sdf_clear(chunk_sdf);
	root = se_sdf_node_create_group(chunk_sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(chunk_sdf, root)) {
		return false;
	}

	for (i32 z = -2; z <= 2; ++z) {
		for (i32 x = -2; x <= 2; ++x) {
			se_sdf_node_primitive_desc box = {0};
			se_sdf_node_primitive_desc sphere = {0};
			s_mat4 box_transform = s_mat4_identity;
			s_mat4 sphere_transform = s_mat4_identity;
			box.operation = SE_SDF_OP_UNION;
			box.primitive.type = SE_SDF_PRIMITIVE_BOX;
			box.primitive.box.size = s_vec3(1.20f, 0.85f + 0.12f * (f32)((x + z + 4) % 4), 1.20f);
			s_mat4_set_translation(&box_transform, &s_vec3((f32)x * 3.25f, box.primitive.box.size.y, (f32)z * 3.25f));
			box.transform = box_transform;
			box.noise = SE_SDF_NOISE_DEFAULTS;

			sphere.operation = SE_SDF_OP_SMOOTH_UNION;
			sphere.operation_amount = 0.26f;
			sphere.primitive.type = SE_SDF_PRIMITIVE_SPHERE;
			sphere.primitive.sphere.radius = 0.50f;
			s_mat4_set_translation(&sphere_transform, &s_vec3((f32)x * 3.25f, box.primitive.box.size.y * 2.0f + 0.15f, (f32)z * 3.25f));
			sphere.transform = sphere_transform;

			se_sdf_node_handle box_node = se_sdf_node_create_primitive(chunk_sdf, &box);
			se_sdf_node_handle sphere_node = se_sdf_node_create_primitive(chunk_sdf, &sphere);
			if (box_node == SE_SDF_NODE_NULL || sphere_node == SE_SDF_NODE_NULL ||
				!se_sdf_node_add_child(chunk_sdf, root, box_node) ||
				!se_sdf_node_add_child(chunk_sdf, root, sphere_node)) {
				return false;
			}
			{
				const s_vec3 box_color = (x + z) & 1 ? s_vec3(0.32f, 0.50f, 0.78f) : s_vec3(0.26f, 0.42f, 0.68f);
				const s_vec3 sphere_color = (x + z) & 1 ? s_vec3(0.96f, 0.78f, 0.34f) : s_vec3(0.94f, 0.62f, 0.26f);
				(void)se_sdf_node_set_color(chunk_sdf, box_node, &box_color);
				(void)se_sdf_node_set_color(chunk_sdf, sphere_node, &sphere_color);
			}
		}
	}

	return se_sdf_prepare(chunk_sdf, prepare_desc);
}

static b8 sdf_stream_rebuild_root(sdf_stream_app* app, const i32 origin_x, const i32 origin_z) {
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	se_sdf_node_handle root = SE_SDF_NODE_NULL;
	if (!app || app->root_sdf == SE_SDF_NULL || app->chunk_sdf == SE_SDF_NULL) {
		return false;
	}

	se_sdf_clear(app->root_sdf);
	root_desc.operation = SE_SDF_OP_UNION;
	root = se_sdf_node_create_group(app->root_sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(app->root_sdf, root)) {
		return false;
	}

	for (i32 z = origin_z - app->stream_radius; z <= origin_z + app->stream_radius; ++z) {
		for (i32 x = origin_x - app->stream_radius; x <= origin_x + app->stream_radius; ++x) {
			se_sdf_node_ref_desc ref = SE_SDF_NODE_REF_DESC_DEFAULTS;
			ref.source = app->chunk_sdf;
			ref.transform = sdf_stream_chunk_transform(x, z);
			se_sdf_node_handle ref_node = se_sdf_node_create_ref(app->root_sdf, &ref);
			if (ref_node == SE_SDF_NODE_NULL || !se_sdf_node_add_child(app->root_sdf, root, ref_node)) {
				return false;
			}
		}
	}

	app->chunk_origin_x = origin_x;
	app->chunk_origin_z = origin_z;
	app->chunks_ready = se_sdf_prepare(app->root_sdf, &app->prepare_desc);
	return app->chunks_ready;
}

static void sdf_stream_update_chunks(sdf_stream_app* app) {
	s_vec3 camera_position = s_vec3(0.0f, 0.0f, 0.0f);
	i32 cell_x = 0;
	i32 cell_z = 0;
	if (!app || app->camera == S_HANDLE_NULL) {
		return;
	}
	(void)se_camera_get_location(app->camera, &camera_position);
	cell_x = (i32)floorf(camera_position.x / STREAM_CELL_SIZE);
	cell_z = (i32)floorf(camera_position.z / STREAM_CELL_SIZE);
	if (!app->chunks_ready || cell_x != app->chunk_origin_x || cell_z != app->chunk_origin_z) {
		(void)sdf_stream_rebuild_root(app, cell_x, cell_z);
	}
}

int main(void) {
	const b8 lightweight_runtime = getenv("SE_TERMINAL_HIDE_WINDOW") != NULL || getenv("SE_DOCS_CAPTURE_PATH") != NULL;
	se_sdf_prepare_desc prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_context* context = se_context_create();
	se_window_handle window = se_window_create(
		"Syphax-Engine - sdf_root_streaming",
		lightweight_runtime ? 640 : 1280,
		lightweight_runtime ? 360 : 720
	);
	se_scene_3d_handle scene = S_HANDLE_NULL;
	se_camera_handle camera = S_HANDLE_NULL;
	se_sdf_renderer_handle renderer = SE_SDF_RENDERER_NULL;
	se_sdf_handle chunk_sdf = SE_SDF_NULL;
	se_sdf_handle root_sdf = SE_SDF_NULL;
	sdf_stream_app app = {0};
	if (!context || window == S_HANDLE_NULL) {
		return 1;
	}
	se_window_set_vsync(window, false);
	se_window_set_target_fps(window, lightweight_runtime ? 240 : 60);

	scene = se_scene_3d_create_for_window(window, 1);
	camera = scene != S_HANDLE_NULL ? se_scene_3d_get_camera(scene) : S_HANDLE_NULL;
	se_camera_set_target_mode(camera, true);
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 52.0f, 0.05f, 400.0f);
	se_camera_set_location(camera, &s_vec3(0.0f, 38.0f, -46.0f));
	se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));

	chunk_sdf = se_sdf_create(NULL);
	root_sdf = se_sdf_create(NULL);
	renderer = se_sdf_renderer_create(NULL);
	if (camera == S_HANDLE_NULL || chunk_sdf == SE_SDF_NULL || root_sdf == SE_SDF_NULL || renderer == SE_SDF_RENDERER_NULL) {
		return 1;
	}
	{
		se_sdf_raymarch_quality quality = SE_SDF_RAYMARCH_QUALITY_DEFAULTS;
		quality.max_steps = lightweight_runtime ? 14 : 34;
		quality.hit_epsilon = lightweight_runtime ? 0.0080f : 0.0035f;
		quality.enable_shadows = 0;
		quality.shadow_steps = 0;
		quality.shadow_strength = 0.0f;
		(void)se_sdf_renderer_set_quality(renderer, &quality);
	}
	(void)se_sdf_renderer_set_light_rig(
		renderer,
		&s_vec3(-0.40f, -0.84f, -0.28f),
		&s_vec3(1.08f, 1.04f, 1.00f),
		&s_vec3(0.10f, 0.13f, 0.17f),
		0.012f
	);
	prepare_desc.lod_count = lightweight_runtime ? 1u : 2u;
	prepare_desc.lod_resolutions[0] = lightweight_runtime ? 10u : 24u;
	prepare_desc.lod_resolutions[1] = lightweight_runtime ? 10u : 12u;
	prepare_desc.lod_resolutions[2] = lightweight_runtime ? 8u : 8u;
	prepare_desc.lod_resolutions[3] = lightweight_runtime ? 8u : 8u;
	prepare_desc.brick_size = lightweight_runtime ? 6u : 8u;
	prepare_desc.brick_border = 1u;
	if (!sdf_stream_build_chunk(chunk_sdf, &prepare_desc)) {
		fprintf(stderr, "sdf_root_streaming :: failed to build chunk (%s)\n", se_result_str(se_get_last_error()));
		return 1;
	}
	if (!sdf_stream_rebuild_root(&(sdf_stream_app){
			.root_sdf = root_sdf,
			.chunk_sdf = chunk_sdf,
			.stream_radius = lightweight_runtime ? 2 : STREAM_RADIUS,
			.prepare_desc = prepare_desc
		}, 0, 0)) {
		fprintf(stderr, "sdf_root_streaming :: failed to build root (%s)\n", se_result_str(se_get_last_error()));
		return 1;
	}

	app.window = window;
	app.scene = scene;
	app.camera = camera;
	app.renderer = renderer;
	app.chunk_sdf = chunk_sdf;
	app.root_sdf = root_sdf;
	app.stream_radius = lightweight_runtime ? 2 : STREAM_RADIUS;
	app.prepare_desc = prepare_desc;
	app.chunk_origin_x = INT32_MAX;
	app.chunk_origin_z = INT32_MAX;
	app.chunks_ready = 0;
	(void)sdf_stream_rebuild_root(&app, 0, 0);

	(void)se_sdf_renderer_set_sdf(renderer, root_sdf);
	(void)se_scene_3d_register_custom_render(scene, sdf_stream_render_scene, &app);

	se_window_loop(window,
		const f32 t = (f32)se_window_get_time(window);
		const s_vec3 orbit = lightweight_runtime ?
			s_vec3(28.0f, 22.0f, -32.0f) :
			s_vec3(sinf(t * 0.12f) * 96.0f, 42.0f + sinf(t * 0.23f) * 6.0f, cosf(t * 0.12f) * 96.0f);
		const s_vec3 target = lightweight_runtime ?
			s_vec3(0.0f, 0.0f, 0.0f) :
			s_vec3(orbit.x + 16.0f, 0.0f, orbit.z + 10.0f);
		u32 width = 1u;
		u32 height = 1u;
		se_debug_trace_begin("sdf_root_streaming");
		se_camera_set_location(camera, &orbit);
		se_camera_set_target(camera, &target);
		sdf_stream_update_chunks(&app);
		se_window_get_size(window, &width, &height);
		se_render_set_background(s_vec4(0.05f, 0.07f, 0.09f, 1.0f));
		(void)width;
		(void)height;
		se_scene_3d_draw(scene, window);
		se_debug_render_overlay(window, NULL);
		se_debug_trace_end("sdf_root_streaming");
	);

	if (renderer != SE_SDF_RENDERER_NULL) {
		se_sdf_renderer_destroy(renderer);
	}
	if (root_sdf != SE_SDF_NULL) {
		se_sdf_destroy(root_sdf);
	}
	if (chunk_sdf != SE_SDF_NULL) {
		se_sdf_destroy(chunk_sdf);
	}
	if (scene != S_HANDLE_NULL) {
		se_scene_3d_destroy(scene);
	}
	se_context_destroy(context);
	return 0;
}
