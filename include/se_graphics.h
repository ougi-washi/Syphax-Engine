// Syphax-Engine - Ougi Washi

#ifndef SE_GRAPHICS_H
#define SE_GRAPHICS_H

#include "se_defines.h"

// Graphics helper functions
extern b8 se_render_init(void);
extern void se_render_shutdown(void);
extern b8 se_render_has_context(void);
extern u64 se_render_get_generation(void);
extern void se_render_set_blending(const b8 active);
extern void se_render_unbind_framebuffer(void); // window framebuffer
extern void se_render_clear(void);
extern void se_render_set_background_color(const s_vec4 color);

// Utility functions
extern void se_render_print_mat4(const s_mat4 *mat);

#endif // SE_GRAPHICS_H
