// Syphax-Engine - Ougi Washi

#include "se_render_frame.h"

#include "render/se_render_queue.h"
#include "se_defines.h"

void se_render_frame_begin(const se_window_handle window) {
	if (window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	se_render_queue_begin_frame(window);
}

void se_render_frame_submit(const se_window_handle window) {
	if (window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	se_render_queue_submit_frame(window);
}

void se_render_frame_wait_presented(const se_window_handle window) {
	if (window == S_HANDLE_NULL) {
		return;
	}
	se_render_queue_wait_presented(window);
}

b8 se_render_frame_get_stats(const se_window_handle window, se_render_frame_stats* out_stats) {
	if (window == S_HANDLE_NULL || out_stats == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	return se_render_queue_get_frame_stats(window, out_stats);
}
