// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_ui.h"
#include "se_window.h"

#include <stdio.h>

// What you will learn: create a basic UI, add buttons, and handle click events.
// Try this: change button text/colors, add more labels, or resize the panel.
typedef struct {
	se_ui_handle ui;
	se_ui_widget_handle label;
	i32 clicks;
} ui_basics_state;

static void ui_on_paint_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_basics_state* state = (ui_basics_state*)user_data;
	if (!state) {
		return;
	}
	state->clicks++;
	c8 text[64] = {0};
	snprintf(text, sizeof(text), "Paint clicks: %d", state->clicks);
	se_ui_widget_set_text(state->ui, state->label, text);
}

static void ui_on_reset_click(const se_ui_click_event* event, void* user_data) {
	(void)event;
	ui_basics_state* state = (ui_basics_state*)user_data;
	if (!state) {
		return;
	}
	state->clicks = 0;
	se_ui_widget_set_text(state->ui, state->label, "Paint clicks: 0");
}

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - UI Basics", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("ui_basics :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.08f, 0.10f, 0.12f, 1.0f));

	se_ui_handle ui = se_ui_create(window, 64);
	se_ui_widget_handle root = se_ui_create_root(ui);
	se_ui_vstack(ui, root, 0.012f, se_ui_edge_all(0.03f));

	ui_basics_state state = { .ui = ui, .clicks = 0 };
	state.label = se_ui_add_text(root, { .text = "Paint clicks: 0", .size = s_vec2(0.35f, 0.08f) });
	se_ui_add_text(root, { .text = "Press D to show/hide widget boxes", .size = s_vec2(0.48f, 0.07f) });
	se_ui_add_button(root, {
		.text = "Add Color Stroke",
		.size = s_vec2(0.28f, 0.09f),
		.on_click_fn = ui_on_paint_click,
		.on_click_data = &state
	});
	se_ui_add_button(root, {
		.text = "Reset Counter",
		.size = s_vec2(0.24f, 0.08f),
		.on_click_fn = ui_on_reset_click,
		.on_click_data = &state
	});

	printf("ui_basics controls:\n");
	printf("  Click buttons to update text\n");
	printf("  D: toggle debug overlay\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		if (se_window_is_key_pressed(window, SE_KEY_D)) {
			se_ui_set_debug_overlay(!se_ui_is_debug_overlay_enabled());
		}
		se_ui_tick(ui);
		se_render_clear();
		se_ui_draw(ui);
		se_window_end_frame(window);
	}

	se_ui_destroy(ui);
	se_context_destroy(context);
	return 0;
}
