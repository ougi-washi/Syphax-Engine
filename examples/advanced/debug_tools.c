// Syphax-Engine - Ougi Washi

#include "se_debug.h"
#include "se_graphics.h"
#include "se_window.h"

#include <stdio.h>

int main(void) {
	printf("advanced/debug_tools :: Advanced example (reference)\n");
	se_context* ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - Debug Tools", 960, 540);
	if (window == S_HANDLE_NULL) {
		printf("20_debug_tools :: window unavailable, skipping runtime\n");
		se_context_destroy(ctx);
		return 0;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.04f, 0.05f, 0.07f, 1.0f));

	se_debug_set_level(SE_DEBUG_LEVEL_TRACE);
	se_debug_set_category_mask(SE_DEBUG_CATEGORY_ALL);
	se_debug_set_overlay_enabled(true);
	se_debug_log(SE_DEBUG_LEVEL_INFO, SE_DEBUG_CATEGORY_CORE, "20_debug_tools started");

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_context_reload_changed_shaders();
		se_debug_trace_begin("20_debug_frame");
		se_render_clear();
		se_debug_trace_end("20_debug_frame");
		se_window_render_screen(window);
	}

	const se_debug_trace_event* traces = NULL;
	sz trace_count = 0;
	se_debug_get_trace_events(&traces, &trace_count);
	printf("20_debug_tools :: captured trace events=%zu\n", trace_count);
	(void)traces;
	c8 timing_ms_text[512] = {0};
	se_debug_dump_last_frame_timing_lines(timing_ms_text, sizeof(timing_ms_text));
	printf("20_debug_tools :: frame timing ms per system:\n%s\n", timing_ms_text);

	se_debug_set_overlay_enabled(false);
	se_context_destroy(ctx);
	return 0;
}
