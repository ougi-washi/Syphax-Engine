// Syphax-Engine - Ougi Washi

#include <stdio.h>
#include <stdlib.h>

#include "se.h"
#include "se_camera.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_sdf.h"
#include "se_window.h"

typedef struct {
	se_window_handle window;
	se_camera_handle camera;
	se_sdf_renderer_handle renderer;
	b8 render_failed;
} sdf_nested_present_state;

static void sdf_nested_render_scene(se_scene_3d_handle scene, void* user_data) {
	(void)scene;
	sdf_nested_present_state* present = user_data;
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
		fprintf(stderr, "sdf_nested_refs :: render failed at stage '%s': %s\n", diagnostics.stage, diagnostics.message);
		present->render_failed = true;
		se_window_set_should_close(present->window, true);
	}
}

int main(void) {
	const b8 lightweight_runtime = getenv("SE_TERMINAL_HIDE_WINDOW") != NULL || getenv("SE_DOCS_CAPTURE_PATH") != NULL;
	se_context* context = se_context_create();
	se_window_handle window = se_window_create(
		"Syphax-Engine - sdf_nested_refs",
		lightweight_runtime ? 640 : 1280,
		lightweight_runtime ? 360 : 720
	);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_vsync(window, false);
	se_window_set_target_fps(window, lightweight_runtime ? 240 : 60);
	se_render_set_background(s_vec4(0.03f, 0.04f, 0.06f, 1.0f));

	se_scene_3d_handle camera_scene = se_scene_3d_create_for_window(window, 1);
	se_camera_handle camera = se_scene_3d_get_camera(camera_scene);
	se_camera_set_target_mode(camera, true);
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 50.0f, 0.05f, 200.0f);
	se_camera_set_location(camera, lightweight_runtime ? &s_vec3(7.5f, 4.8f, 7.6f) : &s_vec3(9.0f, 6.0f, 10.0f));
	se_camera_set_target(camera, lightweight_runtime ? &s_vec3(0.0f, 0.2f, 0.0f) : &s_vec3(0.0f, 0.5f, 0.0f));

	se_sdf_handle child_sdf = se_sdf_create(NULL);
	se_sdf_prepare_desc child_prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_sdf_prepare_desc root_prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_sdf_node_group_desc child_root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	child_root_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	child_root_desc.operation_amount = 0.30f;
	se_sdf_node_handle child_root = se_sdf_node_create_group(child_sdf, &child_root_desc);
	se_sdf_set_root(child_sdf, child_root);

	se_sdf_node_primitive_desc sphere_desc = {0};
	sphere_desc.transform = s_mat4_identity;
	sphere_desc.operation = SE_SDF_OP_UNION;
	sphere_desc.primitive.type = SE_SDF_PRIMITIVE_SPHERE;
	sphere_desc.primitive.sphere.radius = 0.75f;

	se_sdf_node_primitive_desc box_desc = {0};
	box_desc.transform = se_sdf_transform(s_vec3(0.65f, -0.15f, 0.0f), s_vec3(0.0f, 0.0f, 0.0f), s_vec3(0.65f, 0.35f, 0.65f));
	box_desc.operation = SE_SDF_OP_UNION;
	box_desc.primitive.type = SE_SDF_PRIMITIVE_BOX;
	box_desc.primitive.box.size = s_vec3(0.75f, 0.35f, 0.75f);

	se_sdf_node_handle sphere = se_sdf_node_create_primitive(child_sdf, &sphere_desc);
	se_sdf_node_handle box = se_sdf_node_create_primitive(child_sdf, &box_desc);
	(void)se_sdf_node_set_color(child_sdf, sphere, &s_vec3(0.92f, 0.48f, 0.18f));
	(void)se_sdf_node_set_color(child_sdf, box, &s_vec3(0.96f, 0.80f, 0.28f));
	(void)se_sdf_node_add_child(child_sdf, child_root, sphere);
	(void)se_sdf_node_add_child(child_sdf, child_root, box);

	se_sdf_handle child_spire_sdf = se_sdf_create(NULL);
	se_sdf_node_group_desc spire_root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	spire_root_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	spire_root_desc.operation_amount = 0.24f;
	se_sdf_node_handle child_spire_root = se_sdf_node_create_group(child_spire_sdf, &spire_root_desc);
	se_sdf_set_root(child_spire_sdf, child_spire_root);

	se_sdf_node_primitive_desc spire_capsule_desc = {0};
	spire_capsule_desc.transform = se_sdf_transform(s_vec3(0.0f, 0.55f, 0.0f), s_vec3(0.0f, 0.0f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
	spire_capsule_desc.operation = SE_SDF_OP_UNION;
	spire_capsule_desc.primitive.type = SE_SDF_PRIMITIVE_CAPSULE;
	spire_capsule_desc.primitive.capsule.a = s_vec3(0.0f, -0.85f, 0.0f);
	spire_capsule_desc.primitive.capsule.b = s_vec3(0.0f, 0.85f, 0.0f);
	spire_capsule_desc.primitive.capsule.radius = 0.36f;

	se_sdf_node_primitive_desc spire_ring_desc = {0};
	spire_ring_desc.transform = se_sdf_transform(s_vec3(0.0f, -0.15f, 0.0f), s_vec3(1.57f, 0.0f, 0.0f), s_vec3(1.0f, 1.0f, 1.0f));
	spire_ring_desc.operation = SE_SDF_OP_UNION;
	spire_ring_desc.primitive.type = SE_SDF_PRIMITIVE_TORUS;
	spire_ring_desc.primitive.torus.radii = s_vec2(0.78f, 0.14f);

	se_sdf_node_handle spire_capsule = se_sdf_node_create_primitive(child_spire_sdf, &spire_capsule_desc);
	se_sdf_node_handle spire_ring = se_sdf_node_create_primitive(child_spire_sdf, &spire_ring_desc);
	(void)se_sdf_node_set_color(child_spire_sdf, spire_capsule, &s_vec3(0.38f, 0.78f, 0.92f));
	(void)se_sdf_node_set_color(child_spire_sdf, spire_ring, &s_vec3(0.18f, 0.55f, 0.88f));
	(void)se_sdf_node_add_child(child_spire_sdf, child_spire_root, spire_capsule);
	(void)se_sdf_node_add_child(child_spire_sdf, child_spire_root, spire_ring);

	se_sdf_handle root_sdf = se_sdf_create(NULL);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_UNION;
	se_sdf_node_handle root = se_sdf_node_create_group(root_sdf, &root_desc);
	se_sdf_set_root(root_sdf, root);

	se_sdf_node_primitive_desc platform_desc = {0};
	platform_desc.transform = se_sdf_transform(
		s_vec3(0.0f, -1.25f, 0.0f),
		s_vec3(0.0f, 0.0f, 0.0f),
		lightweight_runtime ? s_vec3(3.6f, 0.40f, 2.8f) : s_vec3(5.2f, 0.45f, 4.3f)
	);
	platform_desc.operation = SE_SDF_OP_UNION;
	platform_desc.primitive.type = SE_SDF_PRIMITIVE_BOX;
	platform_desc.primitive.box.size = s_vec3(1.0f, 1.0f, 1.0f);
	se_sdf_node_handle platform = se_sdf_node_create_primitive(root_sdf, &platform_desc);
	(void)se_sdf_node_set_color(root_sdf, platform, &s_vec3(0.22f, 0.26f, 0.32f));
	(void)se_sdf_node_add_child(root_sdf, root, platform);

	const s_vec3 offsets[] = {
		{-3.0f, 0.0f, -2.5f},
		{0.0f, 0.0f, -1.5f},
		{3.0f, 0.0f, -2.5f},
		{-1.5f, 0.0f, 2.0f},
		{1.5f, 0.0f, 2.0f}
	};
	const f32 yaws[] = { 0.0f, 0.45f, -0.35f, 0.9f, -0.8f };
	const s_vec3 scales[] = {
		{1.0f, 1.0f, 1.0f},
		{1.3f, 1.3f, 1.3f},
		{0.9f, 0.9f, 0.9f},
		{1.1f, 1.1f, 1.1f},
		{1.5f, 1.5f, 1.5f}
	};
	const se_sdf_handle sources[] = {
		child_sdf,
		child_spire_sdf,
		child_sdf,
		child_spire_sdf,
		child_sdf
	};
	const s_vec3 lightweight_offsets[] = {
		{-1.6f, 0.0f, 0.2f},
		{0.0f, 0.0f, -0.2f},
		{1.6f, 0.0f, 0.3f}
	};
	const f32 lightweight_yaws[] = { -0.22f, 0.0f, 0.28f };
	const s_vec3 lightweight_scales[] = {
		{1.0f, 1.0f, 1.0f},
		{1.15f, 1.15f, 1.15f},
		{1.0f, 1.0f, 1.0f}
	};
	const se_sdf_handle lightweight_sources[] = {
		child_sdf,
		child_spire_sdf,
		child_sdf
	};
	const sz ref_count = lightweight_runtime ? 3u : (sizeof(offsets) / sizeof(offsets[0]));
	for (sz i = 0; i < ref_count; ++i) {
		se_sdf_node_ref_desc ref_desc = SE_SDF_NODE_REF_DESC_DEFAULTS;
		const s_vec3 position = lightweight_runtime ? lightweight_offsets[i] : offsets[i];
		const f32 yaw = lightweight_runtime ? lightweight_yaws[i] : yaws[i];
		const s_vec3 scale = lightweight_runtime ? lightweight_scales[i] : scales[i];
		ref_desc.source = lightweight_runtime ? lightweight_sources[i] : sources[i];
		ref_desc.transform = se_sdf_transform(position, s_vec3(0.0f, yaw, 0.0f), scale);
		se_sdf_node_handle ref = se_sdf_node_create_ref(root_sdf, &ref_desc);
		(void)se_sdf_node_add_child(root_sdf, root, ref);
	}

	char validation_error[256] = {0};
	if (!se_sdf_validate(root_sdf, validation_error, sizeof(validation_error))) {
		printf("sdf_nested_refs :: validation failed: %s\n", validation_error);
		se_sdf_destroy(root_sdf);
		se_sdf_destroy(child_spire_sdf);
		se_sdf_destroy(child_sdf);
		se_scene_3d_destroy(camera_scene);
		se_context_destroy(context);
		return 1;
	}
	root_prepare_desc.lod_count = lightweight_runtime ? 1u : 2u;
	root_prepare_desc.lod_resolutions[0] = lightweight_runtime ? 10u : 24u;
	root_prepare_desc.lod_resolutions[1] = lightweight_runtime ? 10u : 12u;
	root_prepare_desc.lod_resolutions[2] = lightweight_runtime ? 8u : 8u;
	root_prepare_desc.lod_resolutions[3] = lightweight_runtime ? 8u : 8u;
	root_prepare_desc.brick_size = lightweight_runtime ? 6u : 8u;
	root_prepare_desc.brick_border = 1u;
	child_prepare_desc = root_prepare_desc;
	if (lightweight_runtime) {
		child_prepare_desc.lod_resolutions[0] = 16u;
		child_prepare_desc.lod_resolutions[1] = 10u;
	}
	(void)se_sdf_prepare(child_sdf, &child_prepare_desc);
	(void)se_sdf_prepare(child_spire_sdf, &child_prepare_desc);
	(void)se_sdf_prepare(root_sdf, &root_prepare_desc);

	se_sdf_renderer_desc renderer_desc = SE_SDF_RENDERER_DESC_DEFAULTS;
	renderer_desc.brick_budget = 96u;
	renderer_desc.brick_upload_budget = 72u;
	se_sdf_renderer_handle renderer = se_sdf_renderer_create(&renderer_desc);
	sdf_nested_present_state present_state = {0};
	(void)se_sdf_renderer_set_sdf(renderer, root_sdf);
	{
		se_sdf_raymarch_quality quality = SE_SDF_RAYMARCH_QUALITY_DEFAULTS;
		quality.max_steps = lightweight_runtime ? 26 : 52;
		quality.hit_epsilon = lightweight_runtime ? 0.0038f : 0.0025f;
		quality.enable_shadows = 0;
		quality.shadow_steps = 0;
		quality.shadow_strength = 0.0f;
		(void)se_sdf_renderer_set_quality(renderer, &quality);
	}
	(void)se_sdf_renderer_set_light_rig(
		renderer,
		&s_vec3(-0.42f, -0.82f, -0.36f),
		&s_vec3(1.12f, 1.06f, 1.00f),
		&s_vec3(0.08f, 0.11f, 0.15f),
		0.018f
	);
	present_state.window = window;
	present_state.camera = camera;
	present_state.renderer = renderer;
	(void)se_scene_3d_register_custom_render(camera_scene, sdf_nested_render_scene, &present_state);

	se_window_loop(window,
		u32 fb_w = 0;
		u32 fb_h = 0;
		se_window_get_framebuffer_size(window, &fb_w, &fb_h);
		const f32 width = fb_w > 0 ? (f32)fb_w : 1280.0f;
		const f32 height = fb_h > 0 ? (f32)fb_h : 720.0f;
		se_camera_set_aspect(camera, width, height);
		se_scene_3d_draw(camera_scene, window);
	);

	se_sdf_renderer_destroy(renderer);
	se_sdf_destroy(root_sdf);
	se_sdf_destroy(child_spire_sdf);
	se_sdf_destroy(child_sdf);
	se_scene_3d_destroy(camera_scene);
	se_sdf_shutdown();
	se_context_destroy(context);
	return 0;
}
