// Syphax-Engine - Ougi Washi

#ifndef SE_FRAMEBUFFER_H
#define SE_FRAMEBUFFER_H

#include "se.h"

typedef struct se_framebuffer {
	u32 framebuffer;
	u32 texture;
	u32 depth_buffer;
	s_vec2 size;
	s_vec2 ratio;
	b8 auto_resize : 1;
} se_framebuffer;
typedef s_array(se_framebuffer, se_framebuffers);
typedef se_framebuffer *se_framebuffer_ptr;
typedef s_array(se_framebuffer_ptr, se_framebuffers_ptr);

extern se_framebuffer *se_framebuffer_create(se_context *ctx, const s_vec2 *size);
extern void se_framebuffer_destroy(se_context *ctx, se_framebuffer *framebuffer);
extern void se_framebuffer_set_size(se_framebuffer *framebuffer, const s_vec2 *size);
extern void se_framebuffer_get_size(se_framebuffer *framebuffer, s_vec2 *out_size);
extern void se_framebuffer_bind(se_framebuffer *framebuffer);
extern void se_framebuffer_unbind(se_framebuffer *framebuffer);
extern void se_framebuffer_cleanup(se_framebuffer *framebuffer);

#endif // SE_FRAMEBUFFER_H	
