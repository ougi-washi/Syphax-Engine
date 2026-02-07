// Syphax-Engine - Ougi Washi

#include "se_scene.h"

#define WIDTH 1920
#define HEIGHT 1080

void on_button_yes_pressed(void *window, void *data) {
	if (se_window_is_mouse_down(window, 0)) {
	if (data) {
		(*(int *)data)++;
		printf("Pressed yes, data: %d\n", *(int *)data);
	}
	}
}

void on_button_no_pressed(void *window, void *data) {
	if (se_window_is_mouse_down(window, 0)) {
	if (data) {
		(*(int *)data)++;
		printf("Pressed no, data: %d\n", *(int *)data);
	}
	}
}

i32 main() {
	se_render_handle_params params = {0};
	params.framebuffers_count = 8;
	params.render_buffers_count = 8;
	params.textures_count = 0;
	params.shaders_count = 8;
	params.models_count = 0;
	params.cameras_count = 0;
	se_render_handle *render_handle = se_render_handle_create(&params);

	se_window *window = se_window_create(render_handle, "Syphax-Engine - Scene 2D Example", WIDTH, HEIGHT);

	se_scene_handle_params scene_params = {0};
	scene_params.objects_2d_count = 4;
	scene_params.objects_3d_count = 0;
	scene_params.scenes_2d_count = 2;
	scene_params.scenes_3d_count = 0;
	se_scene_handle *scene_handle = se_scene_handle_create(render_handle, &scene_params);
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

	// Compute bounding boxes from position and scale
	s_vec2 btn_yes_pos = se_object_2d_get_position(button_yes);
	s_vec2 btn_yes_scale = se_object_2d_get_scale(button_yes);
	se_box_2d button_box_yes = {
		s_vec2(btn_yes_pos.x - btn_yes_scale.x, btn_yes_pos.y - btn_yes_scale.y),
		s_vec2(btn_yes_pos.x + btn_yes_scale.x,
				btn_yes_pos.y + btn_yes_scale.y)};

	s_vec2 btn_no_pos = se_object_2d_get_position(button_no);
	s_vec2 btn_no_scale = se_object_2d_get_scale(button_no);
	se_box_2d button_box_no = {
		s_vec2(btn_no_pos.x - btn_no_scale.x, btn_no_pos.y - btn_no_scale.y),
		s_vec2(btn_no_pos.x + btn_no_scale.x, btn_no_pos.y + btn_no_scale.y)};

	int button_yes_data = 0;
	int button_no_data = 0;

	i32 button_yes_update_id = se_window_register_input_event(
		window, &button_box_yes, 0, &on_button_yes_pressed, NULL,
		&button_yes_data);
	i32 button_no_update_id = se_window_register_input_event(
		window, &button_box_no, 0, &on_button_no_pressed, NULL, &button_no_data);

	se_scene_2d_add_object(scene_2d, borders);
	se_scene_2d_add_object(scene_2d, panel);
	se_scene_2d_add_object(scene_2d, button_yes);
	se_scene_2d_add_object(scene_2d, button_no);

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

	// Update position using setter
	s_vec2 current_pos = se_object_2d_get_position(button_yes);
	se_object_2d_set_position(button_yes, &s_vec2(current_pos.x + 0.005, current_pos.y + 0.005));

	// Update bounding box from position/scale
	s_vec2 btn_pos = se_object_2d_get_position(button_yes);
	s_vec2 btn_scale = se_object_2d_get_scale(button_yes);
	button_box_yes.min = s_vec2(btn_pos.x - btn_scale.x, btn_pos.y - btn_scale.y);
	button_box_yes.max = s_vec2(btn_pos.x + btn_scale.x, btn_pos.y + btn_scale.y);
	se_window_update_input_event(button_yes_update_id, window, &button_box_yes, 0, &on_button_yes_pressed, NULL, &button_yes_data);
	se_scene_2d_render(scene_2d, render_handle);

	se_render_clear();
	se_scene_2d_render_to_screen(scene_2d, render_handle, window);
	se_window_render_screen(window);
	}

	se_scene_handle_cleanup(scene_handle);
	se_render_handle_cleanup(render_handle);
	se_window_destroy(window);
	return 0;
}
