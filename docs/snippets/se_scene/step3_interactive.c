#include "se_scene.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_scene_interactive", 960, 540);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	s_vec2 size = s_vec2(1280.0f, 720.0f);
	se_scene_2d_handle scene = se_scene_2d_create(&size, 16);
	if (scene == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_object_2d_handle object = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/scene2d_frag.glsl"), &s_mat3_identity, 0);
	if (object == S_HANDLE_NULL) {
		se_scene_2d_destroy(scene);
		se_context_destroy(context);
		return 1;
	}

	se_scene_2d_add_object(scene, object);
	s_vec2 position = s_vec2(0.25f, -0.10f);
	se_object_2d_set_position(object, &position);

	se_window_begin_frame(window);
	se_scene_2d_render_to_screen(scene, window);
	s_vec2 readback = se_object_2d_get_position(object);
	(void)readback;
	se_scene_2d_render_to_buffer(scene);
	se_window_end_frame(window);

	se_object_2d_destroy(object);
	se_scene_2d_destroy(scene);
	se_context_destroy(context);
	return 0;
}
