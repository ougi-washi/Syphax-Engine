// Syphax-Engine - Ougi Washi

#include "se_scene.h"

#include <stdio.h>

int main(void) {
	se_context* ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - Scene QoL", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("21_scene_qol :: window unavailable, skipping runtime\n");
		se_context_destroy(ctx);
		return 0;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.06f, 0.07f, 0.09f, 1.0f));

	se_scene_3d_handle scene = se_scene_3d_create_for_window(window, 4);
	se_model_handle cube_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
	if (scene == S_HANDLE_NULL || cube_model == S_HANDLE_NULL) {
		printf("21_scene_qol :: scene/model setup failed, skipping runtime\n");
		se_window_destroy(window);
		se_context_destroy(ctx);
		return 0;
	}

	se_object_3d_handle object = se_object_3d_create(cube_model, &s_mat4_identity, 8);
	if (object == S_HANDLE_NULL) {
		printf("21_scene_qol :: object creation failed, skipping runtime\n");
		se_window_destroy(window);
		se_context_destroy(ctx);
		return 0;
	}
	se_scene_3d_add_object(scene, object);
	se_instance_id ids[3] = {0};
	for (i32 i = 0; i < 3; ++i) {
		s_mat4 transform = s_mat4_identity;
		s_vec3 pos = s_vec3(-2.0f + i * 2.0f, 0.0f, -4.0f);
		s_mat4_set_translation(&transform, &pos);
		ids[i] = se_object_3d_add_instance(object, &transform, &s_mat4_identity);
	}
	se_object_3d_set_instance_active(object, ids[1], false);
	s_mat4 metadata = s_mat4_scale(&s_mat4_identity, &s_vec3(1.2f, 1.2f, 1.2f));
	se_object_3d_set_instance_metadata(object, ids[0], &metadata);

	const f64 start = se_window_get_time(window);
	b8 picked_once = false;
	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_context_reload_changed_shaders();
		se_scene_3d_draw(scene, window);

		if (!picked_once && se_window_get_time(window) - start > 0.8) {
			se_object_3d_handle picked = S_HANDLE_NULL;
			f32 distance = 0.0f;
			if (se_scene_3d_pick_object_screen(scene, 640.0f, 360.0f, 1280.0f, 720.0f, 1.5f, NULL, NULL, &picked, &distance)) {
				printf("21_scene_qol :: picked object=%llu distance=%.2f\n", (unsigned long long)picked, distance);
			}
			picked_once = true;
		}
		if (se_window_get_time(window) - start > 2.5) {
			break;
		}
	}

	printf(
		"21_scene_qol :: active=%zu inactive=%zu\n",
		se_object_3d_get_instance_count(object),
		se_object_3d_get_inactive_slot_count(object));

	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
