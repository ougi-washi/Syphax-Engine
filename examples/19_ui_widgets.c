// Syphax-Engine - Ougi Washi

#include "se_ui.h"

#include <stdio.h>

typedef struct {
	i32 clicks;
} ui_example_state;

static void ui_on_button(se_ui_element_handle ui, void* user_data) {
	(void)ui;
	ui_example_state* state = (ui_example_state*)user_data;
	state->clicks++;
	printf("19_ui_widgets :: button clicks=%d\n", state->clicks);
}

int main(void) {
	se_context* ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - UI Widgets", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("19_ui_widgets :: window unavailable, skipping runtime\n");
		se_context_destroy(ctx);
		return 0;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.08f, 0.09f, 0.10f, 1.0f));

	const se_ui_element_params root_params = {
		.layout = SE_UI_LAYOUT_VERTICAL,
		.position = s_vec2(0.0f, 0.0f),
		.size = s_vec2(0.85f, 0.75f),
		.padding = s_vec2(0.01f, 0.01f),
		.visible = true
	};
	se_ui_element_handle root = se_ui_panel_create(window, &root_params);

	const se_ui_element_params button_params = {
		.layout = SE_UI_LAYOUT_HORIZONTAL,
		.position = s_vec2(0.0f, 0.0f),
		.size = s_vec2(0.25f, 0.08f),
		.padding = s_vec2(0.01f, 0.01f),
		.visible = true
	};
	const se_ui_element_params slider_params = {
		.layout = SE_UI_LAYOUT_HORIZONTAL,
		.position = s_vec2(0.0f, -0.2f),
		.size = s_vec2(0.30f, 0.06f),
		.padding = s_vec2(0.01f, 0.01f),
		.visible = true
	};
	const se_ui_element_params checkbox_params = {
		.layout = SE_UI_LAYOUT_HORIZONTAL,
		.position = s_vec2(0.0f, -0.34f),
		.size = s_vec2(0.08f, 0.08f),
		.padding = s_vec2(0.01f, 0.01f),
		.visible = true
	};

	ui_example_state state = {0};
	se_ui_element_handle button = se_ui_button_create(
		window,
		&button_params,
		"Click",
		SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"),
		30.0f,
		ui_on_button,
		&state);
	se_ui_element_handle slider = se_ui_slider_create(window, &slider_params, 0.0f, 100.0f, 42.0f);
	se_ui_element_handle checkbox = se_ui_checkbox_create(window, &checkbox_params, true);
	if (root == S_HANDLE_NULL || button == S_HANDLE_NULL || slider == S_HANDLE_NULL || checkbox == S_HANDLE_NULL) {
		printf("19_ui_widgets :: ui creation failed, skipping runtime\n");
		se_ui_element_destroy(root);
		se_ui_element_destroy(button);
		se_ui_element_destroy(slider);
		se_ui_element_destroy(checkbox);
		se_window_destroy(window);
		se_context_destroy(ctx);
		return 0;
	}

	se_ui_set_debug_overlay(true);
	se_ui_begin_batch();
	se_ui_element_set_clipping(root, true);
	se_ui_element_set_z_order(button, 2);
	se_ui_element_set_z_order(slider, 1);
	se_ui_element_set_z_order(checkbox, 3);
	se_ui_end_batch();

	f64 start = se_window_get_time(window);
	b8 simulated_click = false;
	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_context_reload_changed_shaders();
		se_render_clear();

		if (!simulated_click && se_window_get_time(window) - start > 0.5) {
			const s_vec2 press_pos = s_vec2(0.0f, 0.0f);
			(void)se_ui_element_dispatch_pointer(button, &press_pos, true, false);
			(void)se_ui_element_dispatch_pointer(button, &press_pos, false, true);
			simulated_click = true;
		}

		se_ui_element_render(root);
		se_ui_element_render(button);
		se_ui_element_render(slider);
		se_ui_element_render(checkbox);
		se_ui_element_render_to_screen(root, window);
		se_ui_element_render_to_screen(button, window);
		se_ui_element_render_to_screen(slider, window);
		se_ui_element_render_to_screen(checkbox, window);
		se_window_render_screen(window);

		if (se_window_get_time(window) - start > 3.0) {
			break;
		}
	}

	printf(
		"19_ui_widgets :: slider=%.2f checkbox=%d\n",
		se_ui_slider_get_value(slider),
		se_ui_checkbox_is_checked(checkbox) ? 1 : 0);

	se_ui_element_destroy(root);
	se_ui_element_destroy(button);
	se_ui_element_destroy(slider);
	se_ui_element_destroy(checkbox);
	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
