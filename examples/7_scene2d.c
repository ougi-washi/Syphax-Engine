// Syphax-Engine - Ougi Washi

#include "se_scene.h"

#define WIDTH 1920
#define HEIGHT 1080

void on_button_yes_pressed(se_window_handle window, void *data) {
	if (se_window_is_mouse_pressed(window, SE_MOUSE_LEFT)) {
		(*(int *)data)++;
		printf("Pressed yes, data: %d\n", *(int *)data);
	}
}

void on_button_no_pressed(se_window_handle window, void *data) {
	if (se_window_is_mouse_pressed(window, SE_MOUSE_LEFT)) {
		(*(int *)data)++;
		printf("Pressed no, data: %d\n", *(int *)data);
	}
}

i32 main(void) {
	se_context *ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - Scene 2D Example", WIDTH, HEIGHT);
	se_scene_2d_handle scene_2d = se_scene_2d_create(&s_vec2(WIDTH, HEIGHT), 4);
	se_object_2d_handle borders = S_HANDLE_NULL;
	se_object_2d_handle panel = S_HANDLE_NULL;
	se_object_2d_handle button_yes = S_HANDLE_NULL;
	se_object_2d_handle button_no = S_HANDLE_NULL;

	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	s_mat3 transform = s_mat3_identity;

	borders = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/borders.glsl"), &transform, 0);
	panel = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/panel.glsl"), &transform, 0);
	button_yes = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/button.glsl"), &transform, 0);
	button_no = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/button.glsl"), &transform, 0);
	se_object_2d_set_scale(borders, &s_vec2(0.95, 0.95));
	se_object_2d_set_scale(panel, &s_vec2(0.5, 0.5));
	se_object_2d_set_position(button_yes, &s_vec2(0.15, 0.));
	se_object_2d_set_scale(button_yes, &s_vec2(0.1, 0.1));
	se_object_2d_set_position(button_no, &s_vec2(-0.15, 0.));
	se_object_2d_set_scale(button_no, &s_vec2(0.1, 0.1));

	const se_shader_handle button_yes_shader = se_object_2d_get_shader(button_yes);
	const se_shader_handle button_no_shader = se_object_2d_get_shader(button_no);
	if (button_yes_shader != S_HANDLE_NULL) {
		se_shader_set_vec3(button_yes_shader, "u_color", &s_vec3(0, 1, 0));
	}
	if (button_no_shader != S_HANDLE_NULL) {
		se_shader_set_vec3(button_no_shader, "u_color", &s_vec3(1, 0, 0));
	}

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
		se_context_reload_changed_shaders();

		s_vec2 current_pos = se_object_2d_get_position(button_yes);
		se_object_2d_set_position(button_yes, &s_vec2(current_pos.x + 0.005, current_pos.y + 0.005));
		se_object_2d_get_box_2d(button_yes, &button_box_yes);
		se_window_update_input_event(button_yes_update_id, window, &button_box_yes, 0, &on_button_yes_pressed, NULL, &button_yes_data);
		se_scene_2d_draw(scene_2d, window);
	}

	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
