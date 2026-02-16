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
typedef se_framebuffer_handle se_framebuffer_ptr;
typedef s_array(se_framebuffer_handle, se_framebuffers_ptr);

extern se_framebuffer_handle se_framebuffer_create(const s_vec2 *size);
extern void se_framebuffer_destroy(const se_framebuffer_handle framebuffer);
extern void se_framebuffer_set_size(const se_framebuffer_handle framebuffer, const s_vec2 *size);
extern void se_framebuffer_get_size(const se_framebuffer_handle framebuffer, s_vec2 *out_size);
extern b8 se_framebuffer_get_texture_id(const se_framebuffer_handle framebuffer, u32 *out_texture);
extern void se_framebuffer_bind(const se_framebuffer_handle framebuffer);
extern void se_framebuffer_unbind(const se_framebuffer_handle framebuffer);
extern void se_framebuffer_cleanup(const se_framebuffer_handle framebuffer);

#endif // SE_FRAMEBUFFER_H	
