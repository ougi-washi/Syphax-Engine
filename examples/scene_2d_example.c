// Syphax-Engine - Ougi Washi

#include "se_scene.h"

#define WIDTH 1920
#define HEIGHT 1080

void on_button_yes_pressed(void *window, void *data) {
	if (se_window_is_mouse_pressed(window, 0)) {
		if (data) {
			(*(int *)data)++;
			printf("Pressed yes, data: %d\n", *(int *)data);
		}
	}
}

void on_button_no_pressed(void *window, void *data) {
	if (se_window_is_mouse_pressed(window, 0)) {
		if (data) {
			(*(int *)data)++;
			printf("Pressed no, data: %d\n", *(int *)data);
		}
	}
}

i32 main() {
	se_render_handle *render_handle = se_render_handle_create(NULL);

	se_window *window = se_window_create(render_handle, "Syphax-Engine - Scene 2D Example", WIDTH, HEIGHT);
	se_window_set_exit_key(window, GLFW_KEY_ESCAPE);

	se_scene_handle *scene_handle = se_scene_handle_create(render_handle, NULL);
	se_scene_2d *scene_2d = se_scene_2d_create(scene_handle, &s_vec2(WIDTH, HEIGHT), 4);

	// Create objects with identity transform, then set position/scale
	s_mat3 transform = s_mat3_identity;

	se_object_2d *borders = se_object_2d_create(scene_handle, "examples/scene_example/borders.glsl", &transform, 0);
	se_object_2d_set_scale(borders, &s_vec2(0.95, 0.95));

	se_object_2d *panel = se_object_2d_create(scene_handle, "examples/scene_example/panel.glsl", &transform, 0);
	se_object_2d_set_scale(panel, &s_vec2(0.5, 0.5));

	se_object_2d *button_yes = se_object_2d_create(scene_handle, "examples/scene_example/button.glsl", &transform, 0);
	se_object_2d_set_position(button_yes, &s_vec2(0.15, 0.));
	se_object_2d_set_scale(button_yes, &s_vec2(0.1, 0.1));

	se_object_2d *button_no = se_object_2d_create(scene_handle, "examples/scene_example/button.glsl", &transform, 0);
	se_object_2d_set_position(button_no, &s_vec2(-0.15, 0.));
	se_object_2d_set_scale(button_no, &s_vec2(0.1, 0.1));

	se_shader_set_vec3(button_yes->shader, "u_color", &s_vec3(0, 1, 0));
	se_shader_set_vec3(button_no->shader, "u_color", &s_vec3(1, 0, 0));

	se_box_2d button_box_yes = {0};
	se_box_2d button_box_no = {0};
	se_object_2d_get_box_2d(button_yes, &button_box_yes);
	se_object_2d_get_box_2d(button_no, &button_box_no);

	int button_yes_data = 0;
	int button_no_data = 0;

	i32 button_yes_update_id = se_window_register_input_event(window, &button_box_yes, 0, &on_button_yes_pressed, NULL, &button_yes_data);
	se_window_register_input_event(window, &button_box_no, 0, &on_button_no_pressed, NULL, &button_no_data);

	se_scene_2d_add_object(scene_2d, borders);
	se_scene_2d_add_object(scene_2d, panel);
	se_scene_2d_add_object(scene_2d, button_yes);
	se_scene_2d_add_object(scene_2d, button_no);

	se_scene_2d_set_auto_resize(scene_2d, window, &s_vec2(1., 1.));

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_handle_reload_changed_shaders(render_handle);

		s_vec2 current_pos = se_object_2d_get_position(button_yes);
		se_object_2d_set_position(button_yes, &s_vec2(current_pos.x + 0.005, current_pos.y + 0.005));
		se_object_2d_get_box_2d(button_yes, &button_box_yes);
		se_window_update_input_event(button_yes_update_id, window, &button_box_yes, 0, &on_button_yes_pressed, NULL, &button_yes_data);
		se_scene_2d_draw(scene_2d, render_handle, window);
	}

	se_scene_handle_cleanup(scene_handle);
	se_render_handle_cleanup(render_handle);
	se_window_destroy(window);
	return 0;
}
