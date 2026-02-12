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
	se_shader_handle shader;
} se_render_buffer;
typedef s_array(se_render_buffer, se_render_buffers);
typedef se_render_buffer_handle se_render_buffer_ptr;
typedef s_array(se_render_buffer_handle, se_render_buffers_ptr);

extern se_render_buffer_handle se_render_buffer_create(const u32 width, const u32 height, const c8 *fragment_shader_path);
extern void se_render_buffer_destroy(const se_render_buffer_handle buffer);
extern void se_render_buffer_set_shader(const se_render_buffer_handle buffer, const se_shader_handle shader);
extern void se_render_buffer_unset_shader(const se_render_buffer_handle buffer);
extern void se_render_buffer_bind(const se_render_buffer_handle buffer);
extern void se_render_buffer_unbind(const se_render_buffer_handle buffer);
extern void se_render_buffer_set_scale(const se_render_buffer_handle buffer, const s_vec2 *scale);
extern void se_render_buffer_set_position(const se_render_buffer_handle buffer, const s_vec2 *position);
extern void se_render_buffer_cleanup(const se_render_buffer_handle buffer);

#endif // SE_RENDER_BUFFER_H
