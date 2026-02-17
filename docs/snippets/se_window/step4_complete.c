#include "se_window.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_window_complete", 640, 360);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	(void)se_window_should_close(window);
	u16 target_fps = 120;
	se_window_set_target_fps(window, target_fps);
	se_window_set_vsync(window, 1);
	(void)se_window_is_vsync_enabled(window);

	se_window_begin_frame(window);
	se_window_poll_events();
	se_window_end_frame(window);

	for (u32 frame = 0; frame < 3; ++frame) {
		se_window_begin_frame(window);
		se_window_check_exit_keys(window);
		se_window_poll_events();
		se_window_end_frame(window);
	}

	se_context_destroy(context);
	return 0;
}
