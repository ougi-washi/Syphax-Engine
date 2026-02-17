// Syphax-Engine - Ougi Washi

#ifndef SE_WINDOW_BACKEND_INTERNAL_H
#define SE_WINDOW_BACKEND_INTERNAL_H

#include "se_window.h"

extern b8 se_window_backend_render_thread_attach(se_window_handle window);
extern void se_window_backend_render_thread_detach(void);
extern void se_window_backend_render_thread_present(se_window_handle window);
extern void se_window_backend_render_thread_set_vsync(se_window_handle window, b8 enabled);

#endif // SE_WINDOW_BACKEND_INTERNAL_H
