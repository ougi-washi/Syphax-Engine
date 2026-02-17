#include "se_ui.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_ui_complete", 640, 360);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_ui_handle ui = se_ui_create(window, 256);
	if (ui == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_ui_widget_handle root = se_ui_create_root(ui);
	se_ui_widget_handle button = se_ui_button_create(ui, root, "Apply");
	if (button != S_HANDLE_NULL) {
		se_ui_widget_set_text(ui, button, "Apply Changes");
	}

	se_window_begin_frame(window);
	se_ui_tick(ui);
	se_ui_mark_layout_dirty(ui);
	se_ui_mark_visual_dirty(ui);
	if (se_ui_is_dirty(ui)) {
		se_ui_mark_text_dirty(ui);
		se_ui_clear_focus(ui);
	}
	se_ui_draw(ui);
	se_window_end_frame(window);

	se_ui_destroy(ui);
	se_context_destroy(context);
	return 0;
}
