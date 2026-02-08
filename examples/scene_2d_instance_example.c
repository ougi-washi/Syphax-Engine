// Syphax-Engine - Ougi Washi

#include "se_scene.h"

#define WIDTH 1920
#define HEIGHT 1080

#define INSTANCE_COUNT 16

i32 main() {
	se_render_handle_params params = {0};
	params.framebuffers_count = 8;
	params.render_buffers_count = 8;
	params.textures_count = 0;
	params.shaders_count = 8;
	params.models_count = 0;
	params.cameras_count = 0;
	se_render_handle *render_handle = se_render_handle_create(&params);

	se_scene_handle_params scene_params = {0};
	se_window *window = se_window_create(render_handle, "Syphax-Engine - Scene 2D Example", WIDTH, HEIGHT);

	scene_params.objects_2d_count = 4;
	scene_params.objects_3d_count = 0;
	scene_params.scenes_2d_count = 2;
	scene_params.scenes_3d_count = 0;
	se_scene_handle *scene_handle = se_scene_handle_create(render_handle, &scene_params);
	se_scene_2d *scene_2d = se_scene_2d_create(scene_handle, &s_vec2(WIDTH, HEIGHT), 4);

	s_mat3 transform = s_mat3_identity;
	se_object_2d *button = se_object_2d_create(scene_handle, "examples/scene_example/button.glsl", &transform, 16);
	se_object_2d_set_position(button, &s_vec2(0.15, 0.));
	se_object_2d_set_scale(button, &s_vec2(0.1, 0.1));
	s_mat3 instance_transform = s_mat3_identity;
	for (i32 i = 0; i < 16; i++) {
		s_mat3_set_translation(&instance_transform, &s_vec2(-0.8f + i * 0.1f, 0.0f));
		se_object_2d_add_instance(button, &instance_transform, &s_mat3_identity);
	}

	se_shader_set_vec3(button->shader, "u_color", &s_vec3(0, 1, 0));
	se_scene_2d_add_object(scene_2d, button);
	se_scene_2d_set_auto_resize(scene_2d, window, &s_vec2(1., 1.));

	// TODO: Edit syphax array and make this in a single line
	se_key_combo exit_keys = {0};
	s_array_init(&exit_keys, 1);
	s_array_add(&exit_keys, GLFW_KEY_ESCAPE);
	se_window_set_exit_keys(window, &exit_keys);

	while (!se_window_should_close(window)) {
		se_window_poll_events();
		se_window_update(window);
		se_render_handle_reload_changed_shaders(render_handle);
		se_render_clear();
		// Update position using setter
		s_vec2 current_pos = se_object_2d_get_position(button);
		se_object_2d_set_position(button, &s_vec2(current_pos.x + 0.001, current_pos.y));
		se_scene_2d_render(scene_2d, render_handle);
		se_scene_2d_render_to_screen(scene_2d, render_handle, window);
		se_window_render_screen(window);
	}

	se_scene_handle_cleanup(scene_handle);
	se_render_handle_cleanup(render_handle);
	se_window_destroy(window);
	return 0;
}
