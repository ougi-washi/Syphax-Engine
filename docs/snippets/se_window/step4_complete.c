#include "se_window.h"

int main(void) {
	se_window_handle window = S_HANDLE_NULL;
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	(void)se_window_should_close(window);
	se_window_begin_frame(window);
	se_window_end_frame(window);
	se_window_poll_events();
	u16 target_fps = 120;
	se_window_set_target_fps(window, target_fps);
	se_window_set_vsync(window, 1);
	(void)se_window_is_vsync_enabled(window);
	for (u32 frame = 0; frame < 3; ++frame) {
		se_window_begin_frame(window);
		se_window_check_exit_keys(window);
		se_window_end_frame(window);
	}
	se_window_destroy(window);
	return 0;
}
