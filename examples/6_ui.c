// Syphax-Engine - Ougi Washi

#include "se_ui.h"

#include <stdio.h>

#define WIDTH 1920
#define HEIGHT 1080

void increment_button_color(se_object_2d_handle button) {
	se_shader_handle shader = se_object_2d_get_shader(button);
	if (shader == S_HANDLE_NULL) {
		return;
	}
	s_vec3 *color = se_shader_get_uniform_vec3(shader, "u_color");
	if (!color) {
		return;
	}
	color->x = s_min(color->x * 1.1, 1);
	color->y = s_min(color->y * 1.1, 1);
	color->z = s_min(color->z * 1.1, 1);
}

void on_button_exit_captured(se_window_handle window, void *data) {
	se_object_2d_handle *button_exit = (se_object_2d_handle *)data;
	if (se_window_is_mouse_pressed(window, SE_MOUSE_LEFT)) {
		se_window_set_should_close(window, true);
	}
	if (button_exit) {
		increment_button_color(*button_exit);
	}
}

void on_button_hovered(se_window_handle window, void *data) {
	se_object_2d_handle *button = (se_object_2d_handle *)data;
	if (button) {
		increment_button_color(*button);
	}
}

void on_button_stop_hovered(se_window_handle window, void *data) {
	se_object_2d_handle *button = (se_object_2d_handle *)data;
	if (!button) {
		return;
	}
	se_shader_handle shader = se_object_2d_get_shader(*button);
	if (shader == S_HANDLE_NULL) {
		return;
	}
	s_vec3 *color = se_shader_get_uniform_vec3(shader, "u_color");
	if (!color) {
		return;
	}
	color->x = s_max(color->x / 2., 0);
	color->y = s_max(color->y / 2., 0);
	color->z = s_max(color->z / 2., 0);
}

i32 main(void) {
	se_context *ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - UI Example", WIDTH, HEIGHT);
	if (window == S_HANDLE_NULL) {
		printf("6_ui :: window unavailable, skipping runtime\n");
		se_context_destroy(ctx);
		return 0;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	se_ui_element_params ui_element_params = {0};
	ui_element_params.visible = true;
	ui_element_params.position = s_vec2(0.0f, 0.0f);
	ui_element_params.size = s_vec2(1.0f, 1.0f);
	ui_element_params.padding = s_vec2(0.01f, 0.01f);
	ui_element_params.layout = SE_UI_LAYOUT_VERTICAL;
	se_ui_element_handle root = se_ui_element_create(window, &ui_element_params);

	ui_element_params.layout = SE_UI_LAYOUT_HORIZONTAL;
	se_ui_element_handle toolbar = se_ui_element_add_child(window, root, &ui_element_params);
	se_ui_layout_rules toolbar_rules = {0};
	toolbar_rules.anchor_min = s_vec2(0.0f, 0.93f);
	toolbar_rules.anchor_max = s_vec2(1.0f, 1.0f);
	toolbar_rules.margin = (se_ui_margin){0.0f, 0.0f, 0.0f, 0.0f};
	toolbar_rules.min_size = s_vec2(0.0f, 0.0f);
	toolbar_rules.max_size = s_vec2(0.0f, 0.0f);
	toolbar_rules.align_x = SE_UI_ALIGN_CENTER;
	toolbar_rules.align_y = SE_UI_ALIGN_CENTER;
	toolbar_rules.stretch_x = true;
	toolbar_rules.stretch_y = true;
	se_ui_element_set_layout_rules(toolbar, &toolbar_rules);
	
	se_object_2d_handle button_minimize = se_ui_element_add_object(toolbar, SE_RESOURCE_EXAMPLE("ui/button.glsl"));
	se_object_2d_handle button_maximize = se_ui_element_add_object(toolbar, SE_RESOURCE_EXAMPLE("ui/button.glsl"));
	se_object_2d_handle button_exit = se_ui_element_add_object(toolbar, SE_RESOURCE_EXAMPLE("ui/button.glsl"));
	se_ui_element_add_text(toolbar, "Hello World!", SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.f);
	{
		se_shader_handle minimize_shader = se_object_2d_get_shader(button_minimize);
		se_shader_handle maximize_shader = se_object_2d_get_shader(button_maximize);
		se_shader_handle exit_shader = se_object_2d_get_shader(button_exit);
		if (minimize_shader != S_HANDLE_NULL) {
			se_shader_set_vec3(minimize_shader, "u_color", &s_vec3(0, 0, .3));
		}
		if (maximize_shader != S_HANDLE_NULL) {
			se_shader_set_vec3(maximize_shader, "u_color", &s_vec3(0, .3, 0));
		}
		if (exit_shader != S_HANDLE_NULL) {
			se_shader_set_vec3(exit_shader, "u_color", &s_vec3(.3, 0, 0));
		}
	}

	ui_element_params.layout = SE_UI_LAYOUT_VERTICAL;
	se_ui_element_handle content = se_ui_element_add_child(window, root, &ui_element_params);
	se_ui_layout_rules content_rules = {0};
	content_rules.anchor_min = s_vec2(0.0f, 0.0f);
	content_rules.anchor_max = s_vec2(1.0f, 0.93f);
	content_rules.margin = (se_ui_margin){0.0f, 0.0f, 0.0f, 0.0f};
	content_rules.min_size = s_vec2(0.0f, 0.0f);
	content_rules.max_size = s_vec2(0.0f, 0.0f);
	content_rules.align_x = SE_UI_ALIGN_CENTER;
	content_rules.align_y = SE_UI_ALIGN_CENTER;
	content_rules.stretch_x = true;
	content_rules.stretch_y = true;
	se_ui_element_set_layout_rules(content, &content_rules);
	se_object_2d_handle item_1 = se_ui_element_add_object(content, SE_RESOURCE_EXAMPLE("ui/button.glsl"));
	se_object_2d_handle item_2 = se_ui_element_add_object(content, SE_RESOURCE_EXAMPLE("ui/button.glsl"));
	se_object_2d_handle item_3 = se_ui_element_add_object(content, SE_RESOURCE_EXAMPLE("ui/button.glsl"));
	{
		se_shader_handle item_1_shader = se_object_2d_get_shader(item_1);
		se_shader_handle item_2_shader = se_object_2d_get_shader(item_2);
		se_shader_handle item_3_shader = se_object_2d_get_shader(item_3);
		if (item_1_shader != S_HANDLE_NULL) {
			se_shader_set_vec3(item_1_shader, "u_color", &s_vec3(.5, .5, 0));
		}
		if (item_2_shader != S_HANDLE_NULL) {
			se_shader_set_vec3(item_2_shader, "u_color", &s_vec3(0, .5, .5));
		}
		if (item_3_shader != S_HANDLE_NULL) {
			se_shader_set_vec3(item_3_shader, "u_color", &s_vec3(.5, 0, .5));
		}
	}

	se_box_2d exit_box = {0};
	se_object_2d_get_box_2d(button_exit, &exit_box);
	se_window_register_input_event(window, &exit_box, 0, &on_button_exit_captured, &on_button_stop_hovered, &button_exit);

	se_box_2d minimize_box = {0};
	se_object_2d_get_box_2d(button_minimize, &minimize_box);
	se_window_register_input_event(window, &minimize_box, 0, &on_button_hovered, &on_button_stop_hovered, &button_minimize);

	se_box_2d maximize_box = {0};
	se_object_2d_get_box_2d(button_maximize, &maximize_box);
	se_window_register_input_event(window, &maximize_box, 0, &on_button_hovered, &on_button_stop_hovered, &button_maximize);

	se_window_set_target_fps(window, 30);
	while (!se_window_should_close(window)) {
	    se_window_tick(window);
        s_vec2 normalized_mouse_position = {0};
	    se_window_get_mouse_position_normalized(window, &normalized_mouse_position);
	    c8 fps_text[1024] = "";
	    sprintf(fps_text, "fps: %.2f\n mouse_pos: %.2f, %.2f (%.2f, %.2f)",
	    		1. / se_window_get_delta_time(window),
	    		se_window_get_mouse_position_x(window),
	    		se_window_get_mouse_position_y(window), normalized_mouse_position.x,
	    		normalized_mouse_position.y);
	    se_ui_element_set_text(toolbar, fps_text, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.f);
	    se_render_clear();
	    se_ui_element_render(root);
	    se_ui_element_render_to_screen(root, window);
	    se_window_render_screen(window);
	}

	se_ui_element_destroy_full(root, true);
	se_context_destroy(ctx);
	return 0;
}
