// Syphax Engine - Ougi Washi

#include "se_framebuffer.h"
#include "render/se_gl.h"
#include "se_defines.h"

se_framebuffer *se_framebuffer_create(se_context *ctx, const s_vec2 *size) {
	if (!ctx || !size) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_capacity(&ctx->framebuffers) == s_array_get_size(&ctx->framebuffers)) {
		const sz current_capacity = s_array_get_capacity(&ctx->framebuffers);
		const sz next_capacity = (current_capacity == 0) ? 2 : (current_capacity + 2);
		s_array_resize(&ctx->framebuffers, next_capacity);
	}
	se_framebuffer *framebuffer = s_array_increment(&ctx->framebuffers);
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
	se_framebuffer_cleanup(framebuffer);
	memset(framebuffer, 0, sizeof(*framebuffer));
	se_set_last_error(SE_RESULT_BACKEND_FAILURE);
	return NULL;
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	printf("se_framebuffer_create :: created framebuffer %p\n", framebuffer);
	se_set_last_error(SE_RESULT_OK);
	return framebuffer;
}

void se_framebuffer_destroy(se_context *ctx, se_framebuffer *framebuffer) {
	s_assertf(ctx, "se_framebuffer_destroy :: ctx is null");
	s_assertf(framebuffer, "se_framebuffer_destroy :: framebuffer is null");
	se_framebuffer_cleanup(framebuffer);
	for (sz i = 0; i < s_array_get_size(&ctx->framebuffers); i++) {
		se_framebuffer *slot = s_array_get(&ctx->framebuffers, i);
		if (slot == framebuffer) {
			s_array_remove_at(&ctx->framebuffers, i);
			break;
		}
	}
}

void se_framebuffer_set_size(se_framebuffer *framebuffer, const s_vec2 *size) {
	s_assertf(framebuffer, "se_framebuffer_set_size :: framebuffer is null");
	s_assertf(size, "se_framebuffer_set_size :: size is null");

	framebuffer->size = *size;

	glBindTexture(GL_TEXTURE_2D, framebuffer->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (i32)size->x, (i32)size->y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, framebuffer->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (i32)size->x, (i32)size->y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->depth_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void se_framebuffer_get_size(se_framebuffer *framebuffer, s_vec2 *out_size) {
	s_assertf(framebuffer, "se_framebuffer_get_size :: framebuffer is null");
	s_assertf(out_size, "se_framebuffer_get_size :: out_size is null");
	*out_size = framebuffer->size;
}

void se_framebuffer_bind(se_framebuffer *framebuffer) {
	glViewport(0, 0, framebuffer->size.x, framebuffer->size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);
}

void se_framebuffer_unbind(se_framebuffer *framebuffer) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void se_framebuffer_cleanup(se_framebuffer *framebuffer) {
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
