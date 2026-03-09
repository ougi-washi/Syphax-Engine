// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_sdf.h"
#include "se_window.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static b8 sdf_json_save(const se_sdf_handle sdf, const c8* path) {
	s_json* root = se_sdf_to_json(sdf);
	if (!root) {
		return 0;
	}
	c8* text = s_json_stringify(root);
	s_json_free(root);
	if (!text) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return 0;
	}
	const b8 ok = s_file_write(path, text, strlen(text));
	free(text);
	if (!ok) {
		se_set_last_error(SE_RESULT_IO);
	}
	return ok;
}

static b8 sdf_json_load(const se_sdf_handle sdf, const c8* path) {
	return se_sdf_from_json_file(sdf, path);
}

typedef struct {
	se_window_handle window;
	se_camera_handle camera;
	se_sdf_renderer_handle renderer;
} sdf_json_present_state;

static void sdf_json_render_scene(se_scene_3d_handle scene, void* user_data) {
	(void)scene;
	sdf_json_present_state* present = user_data;
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
		fprintf(stderr, "sdf_json :: render failed at stage '%s': %s\n", diagnostics.stage, diagnostics.message);
		se_window_set_should_close(present->window, true);
	}
}

int main(void) {
	const c8* json_path = "sdf_snapshot.json";
	const b8 lightweight_runtime = getenv("SE_TERMINAL_HIDE_WINDOW") != NULL || getenv("SE_DOCS_CAPTURE_PATH") != NULL;
	se_sdf_prepare_desc prepare_desc = SE_SDF_PREPARE_DESC_DEFAULTS;
	se_context* context = se_context_create();
	se_window_handle window = se_window_create(
		"Syphax - SDF JSON",
		lightweight_runtime ? 640 : 1280,
		lightweight_runtime ? 360 : 720
	);
	if (window == S_HANDLE_NULL) {
		printf("sdf_json :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_vsync(window, false);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.03f, 0.04f, 0.06f, 1.0f));

	se_sdf_handle source_sdf = se_sdf_create(NULL);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	root_desc.operation_amount = 0.35f;
	se_sdf_node_handle root = se_sdf_node_create_group(source_sdf, &root_desc);
	se_sdf_set_root(source_sdf, root);

	se_sdf_node_primitive_desc box_desc = {0};
	box_desc.transform = s_mat4_identity;
	box_desc.operation = SE_SDF_OP_UNION;
	box_desc.primitive.type = SE_SDF_PRIMITIVE_BOX;
	box_desc.primitive.box.size = s_vec3(1.35f, 1.35f, 1.35f);

	se_sdf_node_primitive_desc sphere_desc = {0};
	sphere_desc.transform = s_mat4_identity;
	sphere_desc.operation = SE_SDF_OP_UNION;
	sphere_desc.primitive.type = SE_SDF_PRIMITIVE_SPHERE;
	sphere_desc.primitive.sphere.radius = 1.00f;

	se_sdf_node_handle box = se_sdf_node_create_primitive(source_sdf, &box_desc);
	se_sdf_node_handle sphere = se_sdf_node_create_primitive(source_sdf, &sphere_desc);
	se_sdf_node_add_child(source_sdf, root, box);
	se_sdf_node_add_child(source_sdf, root, sphere);

	if (!sdf_json_save(source_sdf, json_path)) {
		printf("sdf_json :: save failed (%s)\n", se_result_str(se_get_last_error()));
		se_sdf_destroy(source_sdf);
		se_context_destroy(context);
		return 1;
	}

	se_sdf_handle loaded_sdf = se_sdf_create(NULL);
	if (!sdf_json_load(loaded_sdf, json_path)) {
		printf("sdf_json :: load failed (%s)\n", se_result_str(se_get_last_error()));
		se_sdf_destroy(loaded_sdf);
		se_sdf_destroy(source_sdf);
		se_context_destroy(context);
		return 1;
	}
	se_sdf_destroy(source_sdf);
	prepare_desc.lod_count = lightweight_runtime ? 1u : 2u;
	prepare_desc.lod_resolutions[0] = lightweight_runtime ? 10u : 24u;
	prepare_desc.lod_resolutions[1] = lightweight_runtime ? 10u : 12u;
	prepare_desc.lod_resolutions[2] = lightweight_runtime ? 8u : 8u;
	prepare_desc.lod_resolutions[3] = lightweight_runtime ? 8u : 8u;
	prepare_desc.brick_size = lightweight_runtime ? 6u : 8u;
	prepare_desc.brick_border = 1u;
	prepare_desc.max_distance_scale = lightweight_runtime ? 1.35f : 1.20f;
	(void)se_sdf_prepare(loaded_sdf, &prepare_desc);

	se_scene_3d_handle scene = se_scene_3d_create_for_window(window, 1);
	se_camera_handle camera = scene != S_HANDLE_NULL ? se_scene_3d_get_camera(scene) : S_HANDLE_NULL;
	se_sdf_renderer_handle renderer = se_sdf_renderer_create(NULL);
	sdf_json_present_state present_state = {0};
	if (scene == S_HANDLE_NULL || camera == S_HANDLE_NULL || renderer == SE_SDF_RENDERER_NULL) {
		printf("sdf_json :: scene/renderer create failed (%s)\n", se_result_str(se_get_last_error()));
		se_sdf_destroy(loaded_sdf);
		se_context_destroy(context);
		return 1;
	}
	se_camera_set_target_mode(camera, true);
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 46.0f, 0.05f, 60.0f);
	se_camera_set_location(camera, lightweight_runtime ? &s_vec3(3.2f, 2.6f, 4.2f) : &s_vec3(4.4f, 3.2f, 5.4f));
	se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));
	{
		se_sdf_raymarch_quality quality = SE_SDF_RAYMARCH_QUALITY_DEFAULTS;
		quality.max_steps = lightweight_runtime ? 24 : 44;
		quality.hit_epsilon = lightweight_runtime ? 0.0035f : 0.0022f;
		quality.enable_shadows = 0;
		quality.shadow_steps = 0;
		quality.shadow_strength = 0.0f;
		(void)se_sdf_renderer_set_quality(renderer, &quality);
	}
	(void)se_sdf_renderer_set_light_rig(
		renderer,
		&s_vec3(-0.44f, -0.84f, -0.32f),
		&s_vec3(1.10f, 1.05f, 1.00f),
		&s_vec3(0.08f, 0.10f, 0.14f),
		0.015f
	);
	(void)se_sdf_renderer_set_base_color(renderer, &s_vec3(0.94f, 0.66f, 0.26f));
	(void)se_sdf_renderer_set_sdf(renderer, loaded_sdf);
	present_state.window = window;
	present_state.camera = camera;
	present_state.renderer = renderer;
	(void)se_scene_3d_register_custom_render(scene, sdf_json_render_scene, &present_state);

	printf("sdf_json :: saved and loaded %s\n", json_path);
	printf("sdf_json controls:\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_scene_3d_draw(scene, window);
		se_window_end_frame(window);
	}

	se_sdf_renderer_destroy(renderer);
	se_scene_3d_destroy(scene);
	se_sdf_destroy(loaded_sdf);
	se_sdf_shutdown();
	se_context_destroy(context);
	return 0;
}
