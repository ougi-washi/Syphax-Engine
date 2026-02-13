// Syphax Engine - Ougi Washi

#include "se_framebuffer.h"
#include "render/se_gl.h"
#include "se_defines.h"

static void se_framebuffer_cleanup_raw(se_framebuffer *framebuffer) {
	printf("se_framebuffer_cleanup :: framebuffer: %p\n", framebuffer);
	if (framebuffer->framebuffer) {
		glDeleteFramebuffers(1, &framebuffer->framebuffer);
		framebuffer->framebuffer = 0;
	}
	if (framebuffer->texture) {
		glDeleteTextures(1, &framebuffer->texture);
		framebuffer->texture = 0;
	}
	if (framebuffer->depth_buffer) {
		glDeleteRenderbuffers(1, &framebuffer->depth_buffer);
		framebuffer->depth_buffer = 0;
	}
}

static se_framebuffer* se_framebuffer_from_handle(se_context *ctx, const se_framebuffer_handle framebuffer) {
	return s_array_get(&ctx->framebuffers, framebuffer);
}

se_framebuffer_handle se_framebuffer_create(const s_vec2 *size) {
	se_context *ctx = se_current_context();
	if (!ctx || !size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->framebuffers) == 0) {
		s_array_init(&ctx->framebuffers);
	}
	se_framebuffer_handle framebuffer_handle = s_array_increment(&ctx->framebuffers);
	se_framebuffer *framebuffer = s_array_get(&ctx->framebuffers, framebuffer_handle);
	memset(framebuffer, 0, sizeof(*framebuffer));
	framebuffer->size = *size;

	// Create framebuffer
	glGenFramebuffers(1, &framebuffer->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);

	// Create color texture
	glGenTextures(1, &framebuffer->texture);
	glBindTexture(GL_TEXTURE_2D, framebuffer->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size->x, size->y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->texture, 0);

	// Create depth buffer
	glGenRenderbuffers(1, &framebuffer->depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, framebuffer->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size->x, size->y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->depth_buffer);

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	se_framebuffer_cleanup_raw(framebuffer);
	memset(framebuffer, 0, sizeof(*framebuffer));
	se_set_last_error(SE_RESULT_BACKEND_FAILURE);
	s_array_remove(&ctx->framebuffers, framebuffer_handle);
	return S_HANDLE_NULL;
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	printf("se_framebuffer_create :: created framebuffer %p\n", framebuffer);
	se_set_last_error(SE_RESULT_OK);
	return framebuffer_handle;
}

void se_framebuffer_destroy(const se_framebuffer_handle framebuffer) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_framebuffer_destroy :: ctx is null");
	se_framebuffer *framebuffer_ptr = se_framebuffer_from_handle(ctx, framebuffer);
	s_assertf(framebuffer_ptr, "se_framebuffer_destroy :: framebuffer is null");
	se_framebuffer_cleanup_raw(framebuffer_ptr);
	s_array_remove(&ctx->framebuffers, framebuffer);
}

void se_framebuffer_set_size(const se_framebuffer_handle framebuffer, const s_vec2 *size) {
	se_context *ctx = se_current_context();
	se_framebuffer *framebuffer_ptr = se_framebuffer_from_handle(ctx, framebuffer);
	s_assertf(framebuffer_ptr, "se_framebuffer_set_size :: framebuffer is null");
	s_assertf(size, "se_framebuffer_set_size :: size is null");

	framebuffer_ptr->size = *size;
	GLint previous_framebuffer = 0;
	GLint previous_texture = 0;
	GLint previous_renderbuffer = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_framebuffer);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &previous_renderbuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_ptr->framebuffer);

	glBindTexture(GL_TEXTURE_2D, framebuffer_ptr->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (i32)size->x, (i32)size->y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer_ptr->texture, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, framebuffer_ptr->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (i32)size->x, (i32)size->y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer_ptr->depth_buffer);

	glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)previous_renderbuffer);
	glBindTexture(GL_TEXTURE_2D, (GLuint)previous_texture);
	glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous_framebuffer);
}

void se_framebuffer_get_size(const se_framebuffer_handle framebuffer, s_vec2 *out_size) {
	se_context *ctx = se_current_context();
	se_framebuffer *framebuffer_ptr = se_framebuffer_from_handle(ctx, framebuffer);
	s_assertf(framebuffer_ptr, "se_framebuffer_get_size :: framebuffer is null");
	s_assertf(out_size, "se_framebuffer_get_size :: out_size is null");
	*out_size = framebuffer_ptr->size;
}

void se_framebuffer_bind(const se_framebuffer_handle framebuffer) {
	se_context *ctx = se_current_context();
	se_framebuffer *framebuffer_ptr = se_framebuffer_from_handle(ctx, framebuffer);
	glViewport(0, 0, framebuffer_ptr->size.x, framebuffer_ptr->size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_ptr->framebuffer);
}

void se_framebuffer_unbind(const se_framebuffer_handle framebuffer) {
	(void)framebuffer;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void se_framebuffer_cleanup(const se_framebuffer_handle framebuffer) {
	se_context *ctx = se_current_context();
	se_framebuffer *framebuffer_ptr = se_framebuffer_from_handle(ctx, framebuffer);
	if (!framebuffer_ptr) {
		return;
	}
	se_framebuffer_cleanup_raw(framebuffer_ptr);
}
