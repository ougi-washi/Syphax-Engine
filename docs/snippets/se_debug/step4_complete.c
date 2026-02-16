#include "se_debug.h"

int main(void) {
	se_debug_set_level(SE_DEBUG_LEVEL_TRACE);
	se_debug_frame_begin();
	se_debug_trace_begin("physics_step");
	se_debug_trace_end("physics_step");
	se_debug_frame_end();
	se_debug_set_overlay_enabled(1);
	se_debug_frame_timing timing = {0};
	se_debug_get_last_frame_timing(&timing);
	se_debug_set_overlay_enabled(0);
	char lines[512] = {0};
	se_debug_dump_last_frame_timing_lines(lines, sizeof(lines));
	(void)lines;
	return 0;
}
