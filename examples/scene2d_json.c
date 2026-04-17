// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_scene.h"
#include "se_window.h"

#include <stdio.h>

int main(void) {
	const c8* json_path = "scene2d_snapshot.json";
	const c8* object_json_path = "scene2d_object_snapshot.json";
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Scene2D JSON", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("scene2d_json :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.08f, 0.10f, 0.12f, 1.0f));

	se_scene_2d_handle source_scene = se_scene_2d_create(&s_vec2(1280.0f, 720.0f), 4);
	se_scene_2d_set_fit_to_window(source_scene, window, &s_vec2(1.0f, 1.0f));

	se_object_2d_handle panel = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/panel.glsl"), &s_mat3_identity, 0);
	se_object_2d_set_scale(panel, &s_vec2(0.62f, 0.45f));
	se_scene_2d_add_object(source_scene, panel);

	se_object_2d_handle button = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/button.glsl"), &s_mat3_identity, 4);
	se_object_2d_set_scale(button, &s_vec2(0.18f, 0.12f));
	se_scene_2d_add_object(source_scene, button);
	s_mat3 base_instance = s_mat3_identity;
	s_mat4 base_buffer = s_mat4_identity;
	se_object_2d_add_instance(button, &base_instance, &base_buffer);
	s_mat3 second_instance = s_mat3_identity;
	s_mat3_set_translation(&second_instance, &s_vec2(0.32f, 0.0f));
	s_mat4 second_buffer = s_mat4_identity;
	const se_instance_id second_id = se_object_2d_add_instance(button, &second_instance, &second_buffer);
	if (second_id >= 0) {
		s_mat4 metadata = s_mat4_identity;
		metadata.m[0][3] = 7.0f;
		se_object_2d_set_metadata_by_id(button, second_id, &metadata);
		se_object_2d_set_active_by_id(button, second_id, false);
	}
	if (!se_object_2d_to_json_file(button, object_json_path) ||
		!se_object_2d_from_json_file(button, object_json_path)) {
		printf("scene2d_json :: object save/load failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_2d_destroy_full(source_scene, true);
		se_context_destroy(context);
		return 1;
	}

	if (!se_scene_2d_to_json_file(source_scene, json_path)) {
		printf("scene2d_json :: save failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_2d_destroy_full(source_scene, true);
		se_context_destroy(context);
		return 1;
	}

	se_scene_2d_handle loaded_scene = se_scene_2d_create(&s_vec2(1280.0f, 720.0f), 4);
	se_scene_2d_set_fit_to_window(loaded_scene, window, &s_vec2(1.0f, 1.0f));
	if (!se_scene_2d_from_json_file(loaded_scene, json_path)) {
		printf("scene2d_json :: load failed (%s)\n", se_result_str(se_get_last_error()));
		se_scene_2d_destroy_full(loaded_scene, true);
		se_scene_2d_destroy_full(source_scene, true);
		se_context_destroy(context);
		return 1;
	}

	printf("scene2d_json :: saved and loaded %s\n", json_path);
	printf("scene2d_json controls:\n");
	printf("  Esc: quit\n");

	se_scene_2d_destroy_full(source_scene, false);
	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_render_clear();
		se_scene_2d_draw(loaded_scene, window);
		se_window_end_frame(window);
	}

	se_scene_2d_destroy_full(loaded_scene, true);
	se_context_destroy(context);
	return 0;
}
