// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_window.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static b8 scene_json_save(const se_scene_3d_handle scene, const c8* path) {
	s_json* root = se_scene_3d_to_json(scene);
	if (!root) {
		return false;
	}
	c8* text = s_json_stringify(root);
	s_json_free(root);
	if (!text) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}
	const b8 ok = s_file_write(path, text, strlen(text));
	free(text);
	if (!ok) {
		se_set_last_error(SE_RESULT_IO);
	}
	return ok;
}

static b8 scene_json_load(const se_scene_3d_handle scene, const c8* path) {
	c8* text = NULL;
	if (!s_file_read(path, &text, NULL)) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_json* root = s_json_parse(text);
	free(text);
	if (!root) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	const b8 ok = se_scene_3d_from_json(scene, root);
	s_json_free(root);
	return ok;
}

int main(void) {
	const c8* json_path = "scene3d_snapshot.json";
	const c8* object_json_path = "scene3d_object_snapshot.json";
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Scene3D JSON", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("scene3d_json :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.05f, 0.06f, 0.09f, 1.0f));

	se_scene_3d_handle source_scene = se_scene_3d_create_for_window(window, 16);
	se_model_handle model = se_model_load_obj_simple_with_flags(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"),
		SE_MESH_DATA_CPU_GPU);
	if (model == S_HANDLE_NULL) {
		printf("scene3d_json :: model load failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_3d_destroy_full(source_scene, false, false);
		se_context_destroy(context);
		return 1;
	}

	se_object_3d_handle cubes = se_scene_3d_add_model(source_scene, model, &s_mat4_identity);
	s_mat4 object_transform = s_mat4_identity;
	s_mat4_set_translation(&object_transform, &s_vec3(0.0f, 0.0f, 0.0f));
	se_object_3d_set_transform(cubes, &object_transform);
	se_object_3d_set_scale(cubes, &s_vec3(2.2f, 2.2f, 2.2f));
	se_scene_3d_set_culling(source_scene, false);

	if (!se_object_3d_to_json_file(cubes, object_json_path)) {
		printf("scene3d_json :: object save failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_3d_destroy_full(source_scene, true, true);
		se_context_destroy(context);
		return 1;
	}

	s_mat4 edited_transform = s_mat4_identity;
	s_mat4_set_translation(&edited_transform, &s_vec3(-2.0f, 0.0f, 0.0f));
	se_object_3d_set_transform(cubes, &edited_transform);
	se_object_3d_set_scale(cubes, &s_vec3(0.8f, 0.8f, 0.8f));

	if (!se_object_3d_from_json_file(cubes, object_json_path)) {
		printf("scene3d_json :: object load failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_3d_destroy_full(source_scene, true, true);
		se_context_destroy(context);
		return 1;
	}

	if (!scene_json_save(source_scene, json_path)) {
		printf("scene3d_json :: save failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_3d_destroy_full(source_scene, true, true);
		se_context_destroy(context);
		return 1;
	}

	se_scene_3d_handle loaded_scene = se_scene_3d_create_for_window(window, 16);
	if (!scene_json_load(loaded_scene, json_path)) {
		printf("scene3d_json :: load failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_3d_destroy_full(loaded_scene, true, true);
		se_scene_3d_destroy_full(source_scene, true, true);
		se_context_destroy(context);
		return 1;
	}

	printf("scene3d_json :: saved and loaded %s\n", json_path);
	printf("scene3d_json controls:\n");
	printf("  Esc: quit\n");

	const se_camera_handle camera = se_scene_3d_get_camera(loaded_scene);
	const s_vec3 pivot = s_vec3(0.0f, 0.0f, 0.0f);
	const s_vec3 rotation = s_vec3(0.34f, 0.62f, 0.0f);
	se_camera_set_target_mode(camera, true);
	se_camera_set_target(camera, &pivot);
	se_camera_set_rotation(camera, &rotation);
	const s_vec3 forward = se_camera_get_forward_vector(camera);
	const s_vec3 position = s_vec3_sub(&pivot, &s_vec3_muls(&forward, 10.0f));
	se_camera_set_location(camera, &position);
	se_camera_set_perspective(camera, 52.0f, 0.05f, 200.0f);

	se_scene_3d_destroy_full(source_scene, true, false);
	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_camera_set_window_aspect(camera, window);
		se_render_clear();
		se_scene_3d_draw(loaded_scene, window);
		se_window_end_frame(window);
	}

	se_scene_3d_destroy_full(loaded_scene, true, true);
	se_context_destroy(context);
	return 0;
}
