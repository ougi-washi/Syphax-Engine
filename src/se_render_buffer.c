// Syphax Engine - Ougi Washi

#include "se_render_buffer.h"
#include "render/se_gl.h"

se_render_buffer *se_render_buffer_create(se_context *ctx, const u32 width, const u32 height, const c8 *fragment_shader_path) {
	if (!ctx || !fragment_shader_path || width == 0 || height == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_capacity(&ctx->render_buffers) == s_array_get_size(&ctx->render_buffers)) {
		const sz current_capacity = s_array_get_capacity(&ctx->render_buffers);
		const sz next_capacity = (current_capacity == 0) ? 2 : (current_capacity + 2);
		s_array_resize(&ctx->render_buffers, next_capacity);
	}
	se_render_buffer *buffer = s_array_increment(&ctx->render_buffers);
	memset(buffer, 0, sizeof(*buffer));

	buffer->texture_size = s_vec2(width, height);
	buffer->scale = s_vec2(1., 1.);
	buffer->position = s_vec2(0., 0.);

	// Create framebuffer
	glGenFramebuffers(1, &buffer->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->framebuffer);

	// Create color texture
	glGenTextures(1, &buffer->texture);
	glBindTexture(GL_TEXTURE_2D, buffer->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer->texture, 0);

	// Create depth buffer
	glGenRenderbuffers(1, &buffer->depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, buffer->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer->depth_buffer);

	// Create previous texture
	glGenTextures(1, &buffer->prev_texture);
	glBindTexture(GL_TEXTURE_2D, buffer->prev_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create framebuffer for previous frame (for easy copying)
	glGenFramebuffers(1, &buffer->prev_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->prev_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer->prev_texture, 0);

	se_shader *shader = se_shader_load(ctx, SE_RESOURCE_INTERNAL("shaders/render_buffer_vert.glsl"), fragment_shader_path);
	if (!shader) {
		se_render_buffer_cleanup(buffer);
		s_array_remove_at(&ctx->render_buffers, s_array_get_size(&ctx->render_buffers) - 1);
		return NULL;
	}
	buffer->shader = shader;

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	se_render_buffer_cleanup(buffer);
	s_array_remove_at(&ctx->render_buffers, s_array_get_size(&ctx->render_buffers) - 1);
	se_set_last_error(SE_RESULT_BACKEND_FAILURE);
	return NULL;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	se_set_last_error(SE_RESULT_OK);
	return buffer;
}

void se_render_buffer_destroy(se_context *ctx, se_render_buffer *buffer) {
	s_assertf(ctx, "se_render_buffer_destroy :: ctx is null");
	s_assertf(buffer, "se_render_buffer_destroy :: buffer is null");
	se_render_buffer_cleanup(buffer);
	for (sz i = 0; i < s_array_get_size(&ctx->render_buffers); i++) {
		se_render_buffer *slot = s_array_get(&ctx->render_buffers, i);
		if (slot == buffer) {
			s_array_remove_at(&ctx->render_buffers, i);
			break;
		}
	}
}

void se_render_buffer_copy_to_previous(se_render_buffer *buffer) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, buffer->framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer->prev_framebuffer);

	glBlitFramebuffer(0, 0, buffer->texture_size.x,
					buffer->texture_size.y, // Source rectangle
					0, 0, buffer->texture_size.x,
					buffer->texture_size.y, // Destination rectangle
					GL_COLOR_BUFFER_BIT,	// Copy color buffer
					GL_NEAREST // Use nearest filtering for exact copy
	);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void se_render_buffer_set_shader(se_render_buffer *buffer, se_shader *shader) {
	s_assert(buffer && shader);
	buffer->shader = shader;
}

void se_render_buffer_unset_shader(se_render_buffer *buffer) {
	s_assert(buffer);
	buffer->shader = NULL;
}

void se_render_buffer_bind(se_render_buffer *buffer) {
	se_render_buffer_copy_to_previous(buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->framebuffer);
	glViewport(0, 0, buffer->texture_size.x, buffer->texture_size.y);
	se_shader_set_texture(buffer->shader, "u_prev", buffer->prev_texture);
	se_shader_set_vec2(buffer->shader, "u_scale", &buffer->scale);
	se_shader_set_vec2(buffer->shader, "u_position", &buffer->position);
	se_shader_set_vec2(buffer->shader, "u_texture_size", &buffer->texture_size);
}

void se_render_buffer_unbind(se_render_buffer *buf) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void se_render_buffer_set_scale(se_render_buffer *buffer, const s_vec2 *scale) {
	buffer->scale = *scale;
}

void se_render_buffer_set_position(se_render_buffer *buffer, const s_vec2 *position) {
	buffer->position = *position;
}

void se_render_buffer_cleanup(se_render_buffer *buffer) {
	printf("se_render_buffer_cleanup :: buffer: %p\n", buffer);
	if (buffer->framebuffer) {
	glDeleteFramebuffers(1, &buffer->framebuffer);
	buffer->framebuffer = 0;
	}
	if (buffer->prev_framebuffer) {
		glDeleteFramebuffers(1, &buffer->prev_framebuffer);
		buffer->prev_framebuffer = 0;
	}
	if (buffer->texture) {
	glDeleteTextures(1, &buffer->texture);
	buffer->texture = 0;
	}
	if (buffer->prev_texture) {
		glDeleteTextures(1, &buffer->prev_texture);
		buffer->prev_texture = 0;
	}
	if (buffer->depth_buffer) {
	glDeleteRenderbuffers(1, &buffer->depth_buffer);
	buffer->depth_buffer = 0;
	}
}
