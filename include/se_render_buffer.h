// Syphax-Engine - Ougi Washi

#ifndef SE_RENDER_BUFFER_H
#define SE_RENDER_BUFFER_H

#include "se_shader.h"

typedef struct se_render_buffer {
	u32 framebuffer;
	u32 texture;
	u32 prev_framebuffer;
	u32 prev_texture;
	u32 depth_buffer;
	s_vec2 texture_size;
	s_vec2 scale;
	s_vec2 position;
	se_shader_ptr shader;
} se_render_buffer;
typedef s_array(se_render_buffer, se_render_buffers);
typedef se_render_buffer *se_render_buffer_ptr;
typedef s_array(se_render_buffer_ptr, se_render_buffers_ptr);

extern se_render_buffer *se_render_buffer_create(se_context *ctx, const u32 width, const u32 height, const c8 *fragment_shader_path);
extern void se_render_buffer_destroy(se_context *ctx, se_render_buffer *buffer);
extern void se_render_buffer_set_shader(se_render_buffer *buffer, se_shader *shader);
extern void se_render_buffer_unset_shader(se_render_buffer *buffer);
extern void se_render_buffer_bind(se_render_buffer *buffer);
extern void se_render_buffer_unbind(se_render_buffer *buf);
extern void se_render_buffer_set_scale(se_render_buffer *buffer, const s_vec2 *scale);
extern void se_render_buffer_set_position(se_render_buffer *buffer, const s_vec2 *position);
extern void se_render_buffer_cleanup(se_render_buffer *buffer);

#endif // SE_RENDER_BUFFER_H
