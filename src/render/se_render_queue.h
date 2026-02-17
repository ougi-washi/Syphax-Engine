// Syphax-Engine - Ougi Washi

#ifndef SE_RENDER_QUEUE_H
#define SE_RENDER_QUEUE_H

#include "se_render_frame.h"
#include "se_render_thread.h"

typedef void (*se_render_queue_sync_fn)(const void* payload, void* out_result);

#define SE_RENDER_QUEUE_POINTER_PATCH_NONE 0xFFFFFFFFu

extern b8 se_render_queue_start(se_window_handle window, const se_render_thread_config* config);
extern void se_render_queue_stop(se_window_handle window);
extern b8 se_render_queue_is_running_for_window(se_window_handle window);
extern b8 se_render_queue_is_running(void);
extern b8 se_render_queue_is_render_thread(void);
extern se_window_handle se_render_queue_active_window(void);
extern void se_render_queue_wait_idle(se_window_handle window);
extern b8 se_render_queue_get_diagnostics(se_window_handle window, se_render_thread_diagnostics* out_diag);

extern void se_render_queue_begin_frame(se_window_handle window);
extern void se_render_queue_submit_frame(se_window_handle window);
extern void se_render_queue_wait_presented(se_window_handle window);
extern b8 se_render_queue_get_frame_stats(se_window_handle window, se_render_frame_stats* out_stats);

extern b8 se_render_queue_call_sync(se_render_queue_sync_fn fn, const void* payload, void* out_result);
extern b8 se_render_queue_call_sync_sized(se_render_queue_sync_fn fn, const void* payload, void* out_result, u32 payload_bytes);
extern b8 se_render_queue_record_async(se_render_queue_sync_fn fn, const void* payload, u32 payload_bytes);
extern b8 se_render_queue_record_async_blob(se_render_queue_sync_fn fn,
	const void* payload,
	u32 payload_bytes,
	u32 pointer_patch_offset,
	const void* blob,
	u32 blob_bytes);

#endif // SE_RENDER_QUEUE_H
