// Syphax-Engine - Ougi Washi

#ifndef SE_WINDOW_BACKEND_INTERNAL_H
#define SE_WINDOW_BACKEND_INTERNAL_H

#include "se_window.h"
#include <time.h>

extern b8 se_window_backend_render_thread_attach(se_window_handle window);
extern void se_window_backend_render_thread_detach(void);
extern void se_window_backend_render_thread_present(se_window_handle window);
extern void se_window_backend_render_thread_set_vsync(se_window_handle window, b8 enabled);
extern void* se_window_backend_get_gl_proc_address(const c8* name);
extern b8 se_window_backend_has_current_context(void);
extern b8 se_window_backend_read_asset_text(const c8* path, c8** out_data, sz* out_size);
extern b8 se_window_backend_read_asset_binary(const c8* path, u8** out_data, sz* out_size);
extern b8 se_window_backend_get_asset_mtime(const c8* path, time_t* out_mtime);
extern b8 se_window_backend_resolve_writable_path(const c8* path, c8* out_path, sz out_path_size);

#endif // SE_WINDOW_BACKEND_INTERNAL_H
