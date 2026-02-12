// Syphax Engine - Ougi Washi

#include "se_texture.h"
#include "render/se_gl.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void se_texture_cleanup(se_texture *texture);
static se_texture* se_texture_from_handle(se_context *ctx, const se_texture_handle texture) {
	return s_array_get(&ctx->textures, texture);
}

se_texture_handle se_texture_load(const char *file_path, const se_texture_wrap wrap) {
	se_context *ctx = se_current_context();
	if (!ctx || !file_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	stbi_set_flip_vertically_on_load(1);
	if (s_array_get_capacity(&ctx->textures) == 0) {
		s_array_init(&ctx->textures);
	}
	se_texture_handle texture_handle = s_array_increment(&ctx->textures);
	se_texture *texture = s_array_get(&ctx->textures, texture_handle);
	memset(texture, 0, sizeof(*texture));

	char full_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_paths_resolve_resource_path(full_path, SE_MAX_PATH_LENGTH, file_path)) {
		s_array_remove(&ctx->textures, texture_handle);
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	unsigned char *pixels = stbi_load(full_path, &texture->width, &texture->height, &texture->channels, 0);
	if (!pixels) {
		s_array_remove(&ctx->textures, texture_handle);
		se_set_last_error(SE_RESULT_IO);
		return S_HANDLE_NULL;
	}

	glGenTextures(1, &texture->id);
	glActiveTexture(GL_TEXTURE0); // always bind to unit 0 by default
	glBindTexture(GL_TEXTURE_2D, texture->id);

	// Upload to GPU
	GLenum format = (texture->channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Set filtering/wrapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (wrap == SE_CLAMP) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else if (wrap == SE_REPEAT) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	stbi_image_free(pixels);

	se_set_last_error(SE_RESULT_OK);
	return texture_handle;
}

se_texture_handle se_texture_load_from_memory(const u8 *data, const sz size, const se_texture_wrap wrap) {
	se_context *ctx = se_current_context();
	if (!ctx || !data || size == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	stbi_set_flip_vertically_on_load(1);
	if (s_array_get_capacity(&ctx->textures) == 0) {
		s_array_init(&ctx->textures);
	}
	se_texture_handle texture_handle = s_array_increment(&ctx->textures);
	se_texture *texture = s_array_get(&ctx->textures, texture_handle);
	memset(texture, 0, sizeof(*texture));

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 0);
	if (!pixels) {
		s_array_remove(&ctx->textures, texture_handle);
		se_set_last_error(SE_RESULT_IO);
		return S_HANDLE_NULL;
	}

	texture->width = width;
	texture->height = height;
	texture->channels = channels;
	texture->path[0] = '\0';

	glGenTextures(1, &texture->id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->id);

	GLenum format = (texture->channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (wrap == SE_CLAMP) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else if (wrap == SE_REPEAT) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	stbi_image_free(pixels);
	se_set_last_error(SE_RESULT_OK);
	return texture_handle;
}

static void se_texture_cleanup(se_texture *texture) {
	glDeleteTextures(1, &texture->id);
	texture->id = 0;
	texture->width = 0;
	texture->height = 0;
	texture->channels = 0;
	texture->path[0] = '\0';
}

void se_texture_destroy(const se_texture_handle texture) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_texture_destroy :: ctx is null");
	se_texture *texture_ptr = se_texture_from_handle(ctx, texture);
	s_assertf(texture_ptr, "se_texture_destroy :: texture is null");
	se_texture_cleanup(texture_ptr);
	s_array_remove(&ctx->textures, texture);
}
