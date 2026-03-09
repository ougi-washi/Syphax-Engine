// Syphax-Engine - Ougi Washi

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "se_camera.h"
#include "se_debug.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_sdf.h"
#include "se_window.h"

#define STRESS_COLS 400
#define STRESS_ROWS 250
#define STRESS_SPACING 12.0f

static i32 g_sdf_stress_cols = STRESS_COLS;
static i32 g_sdf_stress_rows = STRESS_ROWS;
static f32 g_sdf_stress_spacing = STRESS_SPACING;

static u32 sdf_stress_ref_count(void) {
	return (u32)(g_sdf_stress_cols * g_sdf_stress_rows);
}

static s_vec3 sdf_stress_world_position(const i32 x, const i32 z) {
	const f32 center_x = ((f32)g_sdf_stress_cols - 1.0f) * 0.5f;
	const f32 center_z = ((f32)g_sdf_stress_rows - 1.0f) * 0.5f;
	return s_vec3(
		((f32)x - center_x) * g_sdf_stress_spacing,
		0.0f,
		((f32)z - center_z) * g_sdf_stress_spacing
	);
}

static b8 sdf_stress_build_child(se_sdf_handle sdf, const b8 tall_variant, const se_sdf_prepare_desc* prepare_desc) {
	if (sdf == SE_SDF_NULL) {
		return false;
	}
	se_sdf_clear(sdf);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	root_desc.operation_amount = 0.22f;
	se_sdf_node_handle root = se_sdf_node_create_group(sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(sdf, root)) {
		return false;
	}

	se_sdf_node_primitive_desc base_desc = {0};
	base_desc.operation = SE_SDF_OP_UNION;
	base_desc.primitive.type = SE_SDF_PRIMITIVE_BOX;
	base_desc.primitive.box.size = s_vec3(1.35f, 0.80f, 1.35f);
	base_desc.transform = se_sdf_transform(s_vec3(0.0f, 0.8f, 0.0f), s_vec3(0.0f, 0.0f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));

	se_sdf_node_primitive_desc crown_desc = {0};
	crown_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	crown_desc.operation_amount = 0.18f;
	crown_desc.primitive.type = tall_variant ? SE_SDF_PRIMITIVE_CAPSULE : SE_SDF_PRIMITIVE_SPHERE;
	if (tall_variant) {
		crown_desc.primitive.capsule.a = s_vec3(0.0f, -1.1f, 0.0f);
		crown_desc.primitive.capsule.b = s_vec3(0.0f, 1.1f, 0.0f);
		crown_desc.primitive.capsule.radius = 0.48f;
		crown_desc.transform = se_sdf_transform(s_vec3(0.0f, 2.35f, 0.0f), s_vec3(0.0f, 0.0f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
	} else {
		crown_desc.primitive.sphere.radius = 0.82f;
		crown_desc.transform = se_sdf_transform(s_vec3(0.0f, 2.0f, 0.0f), s_vec3(0.0f, 0.0f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
	}

	se_sdf_node_handle base = se_sdf_node_create_primitive(sdf, &base_desc);
	se_sdf_node_handle crown = se_sdf_node_create_primitive(sdf, &crown_desc);
	if (base == SE_SDF_NODE_NULL || crown == SE_SDF_NODE_NULL) {
		return false;
	}
	(void)se_sdf_node_set_color(sdf, base, tall_variant ? &s_vec3(0.24f, 0.38f, 0.78f) : &s_vec3(0.76f, 0.42f, 0.20f));
	(void)se_sdf_node_set_color(sdf, crown, tall_variant ? &s_vec3(0.56f, 0.82f, 0.96f) : &s_vec3(0.98f, 0.76f, 0.34f));
	(void)se_sdf_node_add_child(sdf, root, base);
	(void)se_sdf_node_add_child(sdf, root, crown);
	return se_sdf_prepare(sdf, prepare_desc);
}

static b8 sdf_stress_build_root(
	se_sdf_handle root_sdf,
	se_sdf_handle child_a,
	se_sdf_handle child_b,
	const se_sdf_prepare_desc* prepare_desc
) {
	if (root_sdf == SE_SDF_NULL || child_a == SE_SDF_NULL || child_b == SE_SDF_NULL) {
		return false;
	}
	se_sdf_clear(root_sdf);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	se_sdf_node_handle root = se_sdf_node_create_group(root_sdf, &root_desc);
	if (root == SE_SDF_NODE_NULL || !se_sdf_set_root(root_sdf, root)) {
		return false;
	}

	for (i32 z = 0; z < g_sdf_stress_rows; ++z) {
		for (i32 x = 0; x < g_sdf_stress_cols; ++x) {
			se_sdf_node_ref_desc ref_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
			const s_vec3 position = sdf_stress_world_position(x, z);
			ref_desc.source = ((x + z) & 1) == 0 ? child_a : child_b;
			ref_desc.transform = se_sdf_transform(
				position,
				s_vec3(0.0f, ((x ^ z) & 7) * 0.18f, 0.0f),
				s_vec3(1.0f, 1.0f, 1.0f)
			);
			se_sdf_node_handle ref = se_sdf_node_create_ref(root_sdf, &ref_desc);
			if (ref == SE_SDF_NODE_NULL || !se_sdf_node_add_child(root_sdf, root, ref)) {
				return false;
			}
		}
	}

	return se_sdf_prepare(root_sdf, prepare_desc);
}

typedef struct {
	se_window_handle window;
	se_camera_handle camera;
	se_sdf_renderer_handle renderer;
	b8 render_failed;
} sdf_stress_present_state;

static void sdf_stress_render_scene(se_scene_3d_handle scene, void* user_data) {
	(void)scene;
	sdf_stress_present_state* present = user_data;
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
		fprintf(stderr, "sdf_stress_refs :: render failed at stage '%s': %s\n", diagnostics.stage, diagnostics.message);
		present->render_failed = true;
		se_window_set_should_close(present->window, true);
	}
}

int main(void) {
	const b8 lightweight_runtime = getenv("SE_TERMINAL_HIDE_WINDOW") != NULL || getenv("SE_DOCS_CAPTURE_PATH") != NULL;
	g_sdf_stress_cols = lightweight_runtime ? 56 : STRESS_COLS;
	g_sdf_stress_rows = lightweight_runtime ? 36 : STRESS_ROWS;
	g_sdf_stress_spacing = lightweight_runtime ? 10.0f : STRESS_SPACING;
	se_sdf_prepare_desc prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_context* context = se_context_create();
	se_window_handle window = se_window_create(
		"Syphax-Engine - sdf_stress_refs",
		lightweight_runtime ? 640 : 1280,
		lightweight_runtime ? 360 : 720
	);
	se_scene_3d_handle scene = S_HANDLE_NULL;
	if (!context || window == S_HANDLE_NULL) {
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_vsync(window, false);
	se_window_set_target_fps(window, lightweight_runtime ? 240 : 60);
	se_render_set_background(s_vec4(0.025f, 0.03f, 0.04f, 1.0f));

	scene = se_scene_3d_create_for_window(window, 1);
	se_camera_handle camera = scene != S_HANDLE_NULL ? se_scene_3d_get_camera(scene) : S_HANDLE_NULL;
	se_camera_set_target_mode(camera, true);
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 48.0f, 0.05f, 800.0f);
	se_camera_set_location(camera, &s_vec3(0.0f, 42.0f, -54.0f));
	se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));

	se_sdf_handle child_a = se_sdf_create(NULL);
	se_sdf_handle child_b = se_sdf_create(NULL);
	se_sdf_handle root_sdf = se_sdf_create(NULL);
	se_sdf_renderer_desc renderer_desc = SE_SDF_RENDERER_DESC_DEFAULTS;
	renderer_desc.max_visible_instances = lightweight_runtime ? 128u : 2048u;
	renderer_desc.brick_budget = 512u;
	renderer_desc.brick_upload_budget = 48u;
	se_sdf_renderer_handle renderer = se_sdf_renderer_create(&renderer_desc);
	sdf_stress_present_state present_state = {0};
	if (camera == S_HANDLE_NULL || child_a == SE_SDF_NULL || child_b == SE_SDF_NULL ||
		root_sdf == SE_SDF_NULL || renderer == SE_SDF_RENDERER_NULL) {
		return 1;
	}

	prepare_desc.lod_count = lightweight_runtime ? 1u : 2u;
	prepare_desc.lod_resolutions[0] = lightweight_runtime ? 8u : 18u;
	prepare_desc.lod_resolutions[1] = lightweight_runtime ? 8u : 10u;
	prepare_desc.lod_resolutions[2] = lightweight_runtime ? 6u : 6u;
	prepare_desc.lod_resolutions[3] = lightweight_runtime ? 6u : 6u;
	prepare_desc.brick_size = lightweight_runtime ? 4u : 6u;
	prepare_desc.brick_border = 1u;
	if (!sdf_stress_build_child(child_a, false, &prepare_desc) ||
		!sdf_stress_build_child(child_b, true, &prepare_desc) ||
		!sdf_stress_build_root(root_sdf, child_a, child_b, &prepare_desc) ||
		!se_sdf_renderer_set_sdf(renderer, root_sdf)) {
		printf("sdf_stress_refs :: setup failed\n");
		return 1;
	}
	{
		se_sdf_raymarch_quality quality = SE_SDF_RAYMARCH_QUALITY_DEFAULTS;
		quality.max_steps = lightweight_runtime ? 14 : 36;
		quality.hit_epsilon = lightweight_runtime ? 0.0080f : 0.0035f;
		quality.enable_shadows = 0;
		quality.shadow_steps = 0;
		quality.shadow_strength = 0.0f;
		(void)se_sdf_renderer_set_quality(renderer, &quality);
	}
	(void)se_sdf_renderer_set_light_rig(
		renderer,
		&s_vec3(-0.40f, -0.84f, -0.28f),
		&s_vec3(1.10f, 1.04f, 0.98f),
		&s_vec3(0.09f, 0.11f, 0.14f),
		0.010f
	);
	present_state.window = window;
	present_state.camera = camera;
	present_state.renderer = renderer;
	(void)se_scene_3d_register_custom_render(scene, sdf_stress_render_scene, &present_state);

	printf("sdf_stress_refs :: prepared %u refs\n", sdf_stress_ref_count());
	f32 last_stats_print = -1000.0f;
	se_window_loop(window,
		const f32 t = (f32)se_window_get_time(window);
		const s_vec3 camera_position = lightweight_runtime ?
			s_vec3(0.0f, 30.0f, -44.0f) :
			s_vec3(sinf(t * 0.11f) * 58.0f, 38.0f + sinf(t * 0.19f) * 4.0f, cosf(t * 0.11f) * 58.0f);
		const s_vec3 camera_target = lightweight_runtime ?
			s_vec3(0.0f, 4.0f, 0.0f) :
			s_vec3(0.0f, 4.0f, 0.0f);
		u32 width = 1u;
		u32 height = 1u;
		se_window_get_framebuffer_size(window, &width, &height);
		se_camera_set_location(camera, &camera_position);
		se_camera_set_target(camera, &camera_target);
		se_debug_trace_begin("sdf_stress_refs");
		(void)width;
		(void)height;
		se_scene_3d_draw(scene, window);
		if (t - last_stats_print >= 1.0f) {
			const se_sdf_renderer_stats stats = se_sdf_renderer_get_stats(renderer);
			printf(
				"sdf_stress_refs :: visible=%u resident=%u misses=%u lods=[%u,%u,%u,%u]\n",
				stats.visible_refs,
				stats.resident_bricks,
				stats.page_misses,
				stats.selected_lod_counts[0],
				stats.selected_lod_counts[1],
				stats.selected_lod_counts[2],
				stats.selected_lod_counts[3]
			);
			last_stats_print = t;
		}
		se_debug_trace_end("sdf_stress_refs");
	);

	se_sdf_renderer_destroy(renderer);
	se_sdf_destroy(root_sdf);
	se_sdf_destroy(child_b);
	se_sdf_destroy(child_a);
	se_scene_3d_destroy(scene);
	se_sdf_shutdown();
	se_context_destroy(context);
	return 0;
}
