// Syphax-Engine - Ougi Washi

#include "se_ui.h"

#define WIDTH 1920
#define HEIGHT 1080

void increment_button_color(se_object_2d *button) {
	s_vec3 *color = se_shader_get_uniform_vec3(button->shader, "u_color");
	if (color) {
	color->x = s_min(color->x * 1.1, 1);
	color->y = s_min(color->y * 1.1, 1);
	color->z = s_min(color->z * 1.1, 1);
	}
}

void on_button_exit_captured(void *window, void *data) {
	se_object_2d *button_exit = (se_object_2d *)data;
	if (se_window_is_mouse_down(window, 0)) {
	se_window_set_should_close((se_window *)window, true);
	}
	increment_button_color(button_exit);
}

void on_button_hovered(void *window, void *data) {
	se_object_2d *button = (se_object_2d *)data;
	increment_button_color(button);
}

void on_button_stop_hovered(void *window, void *data) {
	se_object_2d *button = (se_object_2d *)data;
	s_vec3 *color = se_shader_get_uniform_vec3(button->shader, "u_color");
	if (color) {
	color->x = s_max(color->x / 2., 0);
	color->y = s_max(color->y / 2., 0);
	color->z = s_max(color->z / 2., 0);
	}
}

i32 main() {
	se_render_handle_params params = {0};
	params.framebuffers_count = 8;
	params.render_buffers_count = 8;
	params.shaders_count = 16;
	se_render_handle *render_handle = se_render_handle_create(&params);

	se_window *window = se_window_create(render_handle, "Syphax-Engine - UI Example", WIDTH, HEIGHT);

	se_ui_handle_params ui_handle_params = SE_UI_HANDLE_PARAMS_DEFAULTS;
	se_ui_handle *ui_handle = se_ui_handle_create(window, render_handle, &ui_handle_params);

	se_ui_element_params ui_element_params = SE_UI_ELEMENT_PARAMS_DEFAULTS;
	ui_element_params.layout = SE_UI_LAYOUT_VERTICAL;
	se_ui_element *root = se_ui_element_create(ui_handle, &ui_element_params);

	ui_element_params.layout = SE_UI_LAYOUT_HORIZONTAL;
	se_ui_element *toolbar = se_ui_element_add_child(root, &ui_element_params);
	se_object_2d *button_minimize = se_ui_element_add_object(toolbar, "examples/ui/button.glsl");
	se_object_2d *button_maximize = se_ui_element_add_object(toolbar, "examples/ui/button.glsl");
	se_object_2d *button_exit = se_ui_element_add_object(toolbar, "examples/ui/button.glsl");
	se_shader_set_vec3(button_minimize->shader, "u_color", &s_vec3(0, 0, .3));
	se_shader_set_vec3(button_maximize->shader, "u_color", &s_vec3(0, .3, 0));
	se_shader_set_vec3(button_exit->shader,     "u_color", &s_vec3(.3, 0, 0));

	ui_element_params.layout = SE_UI_LAYOUT_VERTICAL;
	se_ui_element *content = se_ui_element_add_child(root, &ui_element_params);
	se_object_2d *item_1 = se_ui_element_add_object(content, "examples/ui/button.glsl");
	se_object_2d *item_2 = se_ui_element_add_object(content, "examples/ui/button.glsl");
	se_object_2d *item_3 = se_ui_element_add_object(content, "examples/ui/button.glsl");
	se_shader_set_vec3(item_1->shader, "u_color", &s_vec3(.5, .5, 0));
	se_shader_set_vec3(item_2->shader, "u_color", &s_vec3(0, .5, .5));
	se_shader_set_vec3(item_3->shader, "u_color", &s_vec3(.5, 0, .5));

	se_box_2d exit_box = {0};
	se_object_2d_get_box_2d(button_exit, &exit_box);
	i32 exit_update_id = se_window_register_input_event(window, &exit_box, 0, &on_button_exit_captured, &on_button_stop_hovered, button_exit);

	se_box_2d minimize_box = {0};
	se_object_2d_get_box_2d(button_minimize, &minimize_box);
	i32 minimize_update_id = se_window_register_input_event(window, &minimize_box, 0, &on_button_hovered, &on_button_stop_hovered, button_minimize);

	se_box_2d maximize_box = {0};
	se_object_2d_get_box_2d(button_maximize, &maximize_box);
	i32 maximize_update_id = se_window_register_input_event(window, &maximize_box, 0, &on_button_hovered, &on_button_stop_hovered, button_maximize);

	// TODO: Edit syphax array and make this in a single line
	se_key_combo exit_keys = {0};
	s_array_init(&exit_keys, 1);
	s_array_add(&exit_keys, GLFW_KEY_ESCAPE);
	se_window_set_exit_keys(window, &exit_keys);

	se_window_set_target_fps(window, 30);
	while (!se_window_should_close(window)) {
	    se_window_poll_events();
	    se_window_update(window);
        s_vec2 normalized_mouse_position = {0};
	    se_window_get_mouse_position_normalized(window, &normalized_mouse_position);
	    c8 fps_text[1024] = "";
	    sprintf(fps_text, "fps: %.2f\n mouse_pos: %.2f, %.2f (%.2f, %.2f)",
	    		1. / se_window_get_delta_time(window),
	    		se_window_get_mouse_position_x(window),
	    		se_window_get_mouse_position_y(window), normalized_mouse_position.x,
	    		normalized_mouse_position.y);
	    se_ui_element_set_text(toolbar, fps_text, "fonts/ithaca.ttf", 32.f);
	    se_render_clear();
	    se_ui_element_render(root);
	    se_ui_element_render_to_screen(root);
	    se_window_render_screen(window);
	}

	se_ui_handle_cleanup(ui_handle);
	se_render_handle_cleanup(render_handle);
	se_window_destroy(window);
	return 0;
}
