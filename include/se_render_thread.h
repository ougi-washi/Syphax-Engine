// Syphax-Engine - Ougi Washi

#ifndef SE_RENDER_THREAD_H
#define SE_RENDER_THREAD_H

#include "se.h"

#define SE_RENDER_THREAD_NULL S_HANDLE_NULL

typedef s_handle se_render_thread_handle;

typedef struct {
	u32 max_commands_per_frame;
	u32 max_command_bytes_per_frame;
	b8 wait_on_submit;
} se_render_thread_config;

#define SE_RENDER_THREAD_CONFIG_DEFAULTS ((se_render_thread_config){ \
	.max_commands_per_frame = 4096u, \
	.max_command_bytes_per_frame = 4u * 1024u * 1024u, \
	.wait_on_submit = true \
})

typedef struct {
	b8 running;
	b8 stopping;
	b8 failed;
	u64 submitted_frames;
	u64 presented_frames;
	u64 submit_stalls;
	u32 queue_depth;
	u64 last_command_count;
	u64 last_command_bytes;
	f64 last_submit_wait_ms;
	f64 last_execute_ms;
	f64 last_present_ms;
} se_render_thread_diagnostics;

extern se_render_thread_handle se_render_thread_start(se_window_handle window, const se_render_thread_config* config);
extern void se_render_thread_stop(se_render_thread_handle thread);
extern b8 se_render_thread_is_running(se_render_thread_handle thread);
extern void se_render_thread_wait_idle(se_render_thread_handle thread);
extern b8 se_render_thread_get_diagnostics(se_render_thread_handle thread, se_render_thread_diagnostics* out_diag);

#endif // SE_RENDER_THREAD_H
