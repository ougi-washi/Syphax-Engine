// Syphax-Engine - Ougi Washi

#include "se_debug.h"
#include "se_graphics.h"
#include "se_window.h"

#include <stdlib.h>
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
	se_window_set_vsync(window, false);
	se_render_set_background_color(s_vec4(0.04f, 0.05f, 0.07f, 1.0f));

	se_debug_set_level(SE_DEBUG_LEVEL_TRACE);
	se_debug_set_category_mask(SE_DEBUG_CATEGORY_ALL);
	se_debug_set_overlay_enabled(true);
	se_debug_log(SE_DEBUG_LEVEL_INFO, SE_DEBUG_CATEGORY_CORE, "20_debug_tools started");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_context_reload_changed_shaders();
		se_debug_trace_begin("20_debug_frame");
		se_render_clear();
		se_debug_trace_end("20_debug_frame");
		se_window_end_frame(window);
	}

	const se_debug_trace_event* traces = NULL;
	sz trace_count = 0;
	se_debug_get_trace_events(&traces, &trace_count);
	printf("20_debug_tools :: captured trace events=%zu\n", trace_count);
	(void)traces;
	u32 trace_stats_count = 0u;
	f64 cpu_ms = 0.0;
	f64 gpu_ms = 0.0;
	if (se_debug_get_trace_stats(NULL, 0u, &trace_stats_count, true) && trace_stats_count > 0u) {
		se_debug_trace_stat* trace_stats = (se_debug_trace_stat*)calloc(trace_stats_count, sizeof(*trace_stats));
		if (trace_stats && se_debug_get_trace_stats(trace_stats, trace_stats_count, &trace_stats_count, true)) {
			for (u32 i = 0; i < trace_stats_count; ++i) {
				const se_debug_trace_stat* stat = &trace_stats[i];
				if (stat->channel == (u32)SE_DEBUG_TRACE_CHANNEL_CPU) {
					cpu_ms += stat->total_ms;
				} else if (stat->channel == (u32)SE_DEBUG_TRACE_CHANNEL_GPU) {
					gpu_ms += stat->total_ms;
				}
			}
		}
		free(trace_stats);
	}
	printf(
		"20_debug_tools :: trace totals last frame cpu=%.3fms gpu=%.3fms\n",
		cpu_ms,
		gpu_ms);

	c8 trace_stats_text[2048] = {0};
	se_debug_dump_trace_stats(trace_stats_text, sizeof(trace_stats_text), 12u, true);
	printf("20_debug_tools :: trace stats last frame:\n%s\n", trace_stats_text);

	se_debug_set_overlay_enabled(false);
	se_context_destroy(ctx);
	return 0;
}
