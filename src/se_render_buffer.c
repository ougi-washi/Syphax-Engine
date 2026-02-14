// Syphax Engine - Ougi Washi

#include "se_render_buffer.h"
#include "render/se_gl.h"
#include "se_debug.h"

static se_render_buffer* se_render_buffer_from_handle(se_context *ctx, const se_render_buffer_handle buffer) {
	return s_array_get(&ctx->render_buffers, buffer);
}

static void se_render_buffer_cleanup_raw(se_render_buffer *buffer) {
	se_log("se_render_buffer_cleanup :: buffer: %p", (void*)buffer);
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

static void se_render_buffer_copy_to_previous(se_render_buffer *buffer) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, buffer->framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer->prev_framebuffer);

	glBlitFramebuffer(0, 0, buffer->texture_size.x,
					buffer->texture_size.y,
					0, 0, buffer->texture_size.x,
					buffer->texture_size.y,
					GL_COLOR_BUFFER_BIT,
					GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

se_render_buffer_handle se_render_buffer_create(const u32 width, const u32 height, const c8 *fragment_shader_path) {
	se_context *ctx = se_current_context();
	if (!ctx || !fragment_shader_path || width == 0 || height == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->render_buffers) == 0) {
		s_array_init(&ctx->render_buffers);
	}
	se_render_buffer_handle buffer_handle = s_array_increment(&ctx->render_buffers);
	se_render_buffer *buffer = s_array_get(&ctx->render_buffers, buffer_handle);
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

	se_shader_handle shader = se_shader_load(SE_RESOURCE_INTERNAL("shaders/render_buffer_vert.glsl"), fragment_shader_path);
	if (shader == S_HANDLE_NULL) {
		se_render_buffer_cleanup_raw(buffer);
		s_array_remove(&ctx->render_buffers, buffer_handle);
		return S_HANDLE_NULL;
	}
	buffer->shader = shader;

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	se_render_buffer_cleanup_raw(buffer);
	s_array_remove(&ctx->render_buffers, buffer_handle);
	se_set_last_error(SE_RESULT_BACKEND_FAILURE);
	return S_HANDLE_NULL;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	se_set_last_error(SE_RESULT_OK);
	return buffer_handle;
}

void se_render_buffer_destroy(const se_render_buffer_handle buffer) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_render_buffer_destroy :: ctx is null");
	se_render_buffer *buffer_ptr = se_render_buffer_from_handle(ctx, buffer);
	s_assertf(buffer_ptr, "se_render_buffer_destroy :: buffer is null");
	se_render_buffer_cleanup_raw(buffer_ptr);
	s_array_remove(&ctx->render_buffers, buffer);
}

void se_render_buffer_set_shader(const se_render_buffer_handle buffer, const se_shader_handle shader) {
	se_context *ctx = se_current_context();
	se_render_buffer *buffer_ptr = se_render_buffer_from_handle(ctx, buffer);
	s_assert(buffer_ptr);
	buffer_ptr->shader = shader;
}

void se_render_buffer_unset_shader(const se_render_buffer_handle buffer) {
	se_context *ctx = se_current_context();
	se_render_buffer *buffer_ptr = se_render_buffer_from_handle(ctx, buffer);
	s_assert(buffer_ptr);
	buffer_ptr->shader = S_HANDLE_NULL;
}

void se_render_buffer_bind(const se_render_buffer_handle buffer) {
	se_context *ctx = se_current_context();
	se_render_buffer *buffer_ptr = se_render_buffer_from_handle(ctx, buffer);
	se_render_buffer_copy_to_previous(buffer_ptr);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer_ptr->framebuffer);
	glViewport(0, 0, buffer_ptr->texture_size.x, buffer_ptr->texture_size.y);
	se_shader_set_texture(buffer_ptr->shader, "u_prev", buffer_ptr->prev_texture);
	se_shader_set_vec2(buffer_ptr->shader, "u_scale", &buffer_ptr->scale);
	se_shader_set_vec2(buffer_ptr->shader, "u_position", &buffer_ptr->position);
	se_shader_set_vec2(buffer_ptr->shader, "u_texture_size", &buffer_ptr->texture_size);
}

void se_render_buffer_unbind(const se_render_buffer_handle buffer) {
	(void)buffer;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void se_render_buffer_set_scale(const se_render_buffer_handle buffer, const s_vec2 *scale) {
	se_context *ctx = se_current_context();
	se_render_buffer *buffer_ptr = se_render_buffer_from_handle(ctx, buffer);
	buffer_ptr->scale = *scale;
}

void se_render_buffer_set_position(const se_render_buffer_handle buffer, const s_vec2 *position) {
	se_context *ctx = se_current_context();
	se_render_buffer *buffer_ptr = se_render_buffer_from_handle(ctx, buffer);
	buffer_ptr->position = *position;
}

void se_render_buffer_cleanup(const se_render_buffer_handle buffer) {
	se_context *ctx = se_current_context();
	se_render_buffer *buffer_ptr = se_render_buffer_from_handle(ctx, buffer);
	if (!buffer_ptr) {
		return;
	}
	se_render_buffer_cleanup_raw(buffer_ptr);
}
