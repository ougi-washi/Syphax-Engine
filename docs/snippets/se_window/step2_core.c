#include "se_window.h"

int main(void) {
	se_window_handle window = S_HANDLE_NULL;
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	(void)se_window_should_close(window);
	se_window_begin_frame(window);
	se_window_end_frame(window);
	se_window_poll_events();
	return 0;
}
