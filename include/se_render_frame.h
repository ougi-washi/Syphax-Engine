// Syphax-Engine - Ougi Washi

#ifndef SE_RENDER_FRAME_H
#define SE_RENDER_FRAME_H

#include "se.h"

typedef struct {
	u64 submitted_frames;
	u64 presented_frames;
	u64 submit_stalls;
	u32 queue_depth;
	u64 last_command_count;
	u64 last_command_bytes;
	f64 last_submit_wait_ms;
	f64 last_execute_ms;
	f64 last_present_ms;
} se_render_frame_stats;

extern void se_render_frame_begin(se_window_handle window);
extern void se_render_frame_submit(se_window_handle window);
extern void se_render_frame_wait_presented(se_window_handle window);
extern b8 se_render_frame_get_stats(se_window_handle window, se_render_frame_stats* out_stats);

#endif // SE_RENDER_FRAME_H
