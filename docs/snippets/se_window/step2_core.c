#include "se_window.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_window_core", 640, 360);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	(void)se_window_should_close(window);
	se_window_begin_frame(window);
	se_window_poll_events();
	se_window_end_frame(window);

	se_context_destroy(context);
	return 0;
}
