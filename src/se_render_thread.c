// Syphax-Engine - Ougi Washi

#include "se_render_thread.h"

#include "render/se_render_queue.h"
#include "se_defines.h"

#define SE_RENDER_THREAD_SINGLETON_HANDLE ((se_render_thread_handle)1u)

se_render_thread_handle se_render_thread_start(const se_window_handle window, const se_render_thread_config* config) {
	if (window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return SE_RENDER_THREAD_NULL;
	}
	if (!se_render_queue_start(window, config)) {
		return SE_RENDER_THREAD_NULL;
	}
	se_set_last_error(SE_RESULT_OK);
	return SE_RENDER_THREAD_SINGLETON_HANDLE;
}

void se_render_thread_stop(const se_render_thread_handle thread) {
	if (thread != SE_RENDER_THREAD_SINGLETON_HANDLE) {
		return;
	}
	const se_window_handle window = se_render_queue_active_window();
	if (window != S_HANDLE_NULL) {
		se_render_queue_stop(window);
	}
}

b8 se_render_thread_is_running(const se_render_thread_handle thread) {
	if (thread != SE_RENDER_THREAD_SINGLETON_HANDLE) {
		return false;
	}
	return se_render_queue_is_running();
}

void se_render_thread_wait_idle(const se_render_thread_handle thread) {
	if (thread != SE_RENDER_THREAD_SINGLETON_HANDLE) {
		return;
	}
	const se_window_handle window = se_render_queue_active_window();
	if (window != S_HANDLE_NULL) {
		se_render_queue_wait_idle(window);
	}
}

b8 se_render_thread_get_diagnostics(const se_render_thread_handle thread, se_render_thread_diagnostics* out_diag) {
	if (thread != SE_RENDER_THREAD_SINGLETON_HANDLE || out_diag == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const se_window_handle window = se_render_queue_active_window();
	if (window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	return se_render_queue_get_diagnostics(window, out_diag);
}
