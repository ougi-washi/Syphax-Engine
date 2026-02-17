#include "se_ui.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_ui_core", 640, 360);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_ui_handle ui = se_ui_create(window, 128);
	if (ui == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_window_begin_frame(window);
	se_ui_tick(ui);
	se_ui_mark_layout_dirty(ui);
	se_ui_mark_visual_dirty(ui);
	se_ui_draw(ui);
	se_window_end_frame(window);

	se_ui_destroy(ui);
	se_context_destroy(context);
	return 0;
}
