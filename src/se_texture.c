// Syphax Engine - Ougi Washi

#include "se_texture.h"
#include "render/se_gl.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void se_texture_cleanup(se_texture *texture);
static se_texture* se_texture_from_handle(se_context *ctx, const se_texture_handle texture) {
	return s_array_get(&ctx->textures, texture);
}

static void se_texture_set_wrap_params(const GLenum target, const se_texture_wrap wrap) {
	if (wrap == SE_CLAMP) {
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (target == GL_TEXTURE_3D) {
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}
	} else {
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if (target == GL_TEXTURE_3D) {
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);
		}
	}
}

static GLenum se_texture_format_from_channel_count(const i32 channels) {
	if (channels == 4) {
		return GL_RGBA;
	}
	if (channels == 3) {
		return GL_RGB;
	}
	if (channels == 2) {
		return GL_RG;
	}
	return GL_RED;
}

static b8 se_texture_store_cpu_pixels_rgba(se_texture *texture, const u8 *pixels) {
	if (!texture || !pixels || texture->width <= 0 || texture->height <= 0 || texture->channels <= 0) {
		return false;
	}

	const sz pixel_count = (sz)texture->width * (sz)texture->height;
	const sz dst_size = pixel_count * 4u;
	u8 *dst = malloc(dst_size);
	if (!dst) {
		return false;
	}

	for (sz i = 0; i < pixel_count; ++i) {
		const sz src_index = i * (sz)texture->channels;
		const sz dst_index = i * 4u;
		if (texture->channels == 4) {
			dst[dst_index + 0] = pixels[src_index + 0];
			dst[dst_index + 1] = pixels[src_index + 1];
			dst[dst_index + 2] = pixels[src_index + 2];
			dst[dst_index + 3] = pixels[src_index + 3];
			continue;
		}
		if (texture->channels == 3) {
			dst[dst_index + 0] = pixels[src_index + 0];
			dst[dst_index + 1] = pixels[src_index + 1];
			dst[dst_index + 2] = pixels[src_index + 2];
			dst[dst_index + 3] = 255u;
			continue;
		}
		if (texture->channels == 2) {
			dst[dst_index + 0] = pixels[src_index + 0];
			dst[dst_index + 1] = pixels[src_index + 0];
			dst[dst_index + 2] = pixels[src_index + 0];
			dst[dst_index + 3] = pixels[src_index + 1];
			continue;
		}
		dst[dst_index + 0] = pixels[src_index + 0];
		dst[dst_index + 1] = pixels[src_index + 0];
		dst[dst_index + 2] = pixels[src_index + 0];
		dst[dst_index + 3] = 255u;
	}

	texture->cpu_pixels = dst;
	texture->cpu_pixels_size = dst_size;
	texture->cpu_channels = 4;
	return true;
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
	texture->depth = 1;
	texture->target = GL_TEXTURE_2D;
	texture->wrap = wrap;

	char full_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_paths_resolve_resource_path(full_path, SE_MAX_PATH_LENGTH, file_path)) {
		s_array_remove(&ctx->textures, texture_handle);
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	u8 *pixels = stbi_load(full_path, &texture->width, &texture->height, &texture->channels, 0);
	if (!pixels) {
		s_array_remove(&ctx->textures, texture_handle);
		se_set_last_error(SE_RESULT_IO);
		return S_HANDLE_NULL;
	}

	glGenTextures(1, &texture->id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->id);

	const GLenum format = se_texture_format_from_channel_count(texture->channels);
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	se_texture_set_wrap_params(GL_TEXTURE_2D, wrap);

	strncpy(texture->path, file_path, sizeof(texture->path) - 1);
	texture->path[sizeof(texture->path) - 1] = '\0';

	if (!se_texture_store_cpu_pixels_rgba(texture, pixels)) {
		stbi_image_free(pixels);
		se_texture_cleanup(texture);
		s_array_remove(&ctx->textures, texture_handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
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
	texture->depth = 1;
	texture->target = GL_TEXTURE_2D;
	texture->wrap = wrap;

	int width = 0;
	int height = 0;
	int channels = 0;
	u8 *pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 0);
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

	const GLenum format = se_texture_format_from_channel_count(texture->channels);
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	se_texture_set_wrap_params(GL_TEXTURE_2D, wrap);

	if (!se_texture_store_cpu_pixels_rgba(texture, pixels)) {
		stbi_image_free(pixels);
		se_texture_cleanup(texture);
		s_array_remove(&ctx->textures, texture_handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return S_HANDLE_NULL;
	}

	stbi_image_free(pixels);
	se_set_last_error(SE_RESULT_OK);
	return texture_handle;
}

se_texture_handle se_texture_create_3d_rgba16f(const f32 *data, const u32 width, const u32 height, const u32 depth, const se_texture_wrap wrap) {
	se_context *ctx = se_current_context();
	if (!ctx || !data || width == 0u || height == 0u || depth == 0u) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->textures) == 0) {
		s_array_init(&ctx->textures);
	}

	se_texture_handle texture_handle = s_array_increment(&ctx->textures);
	se_texture *texture = s_array_get(&ctx->textures, texture_handle);
	memset(texture, 0, sizeof(*texture));

	texture->width = (i32)width;
	texture->height = (i32)height;
	texture->depth = (i32)depth;
	texture->channels = 4;
	texture->target = GL_TEXTURE_3D;
	texture->wrap = wrap;
	texture->path[0] = '\0';

	glGenTextures(1, &texture->id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, texture->id);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, (GLsizei)width, (GLsizei)height, (GLsizei)depth, 0, GL_RGBA, GL_FLOAT, data);

	// Use single-level linear filtering for stable raymarch interpolation.
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	se_texture_set_wrap_params(GL_TEXTURE_3D, wrap);

	se_set_last_error(SE_RESULT_OK);
	return texture_handle;
}

se_texture_handle se_texture_find_by_id(const u32 texture_id) {
	se_context *ctx = se_current_context();
	if (!ctx || texture_id == 0u) {
		return S_HANDLE_NULL;
	}

	for (sz i = 0; i < s_array_get_size(&ctx->textures); ++i) {
		const se_texture_handle handle = s_array_handle(&ctx->textures, (u32)i);
		se_texture *texture = s_array_get(&ctx->textures, handle);
		if (texture && texture->id == texture_id) {
			return handle;
		}
	}
	return S_HANDLE_NULL;
}

static f32 se_texture_wrap_coordinate(const f32 value, const se_texture_wrap wrap) {
	if (wrap == SE_REPEAT) {
		return value - floorf(value);
	}
	return s_max(0.0f, s_min(1.0f, value));
}

static i32 se_texture_wrap_index(const i32 value, const i32 limit, const se_texture_wrap wrap) {
	if (limit <= 0) {
		return 0;
	}
	if (wrap == SE_REPEAT) {
		i32 idx = value % limit;
		if (idx < 0) {
			idx += limit;
		}
		return idx;
	}
	return s_max(0, s_min(limit - 1, value));
}

static s_vec3 se_texture_read_rgb(const se_texture *texture, const i32 x, const i32 y) {
	const i32 safe_x = se_texture_wrap_index(x, texture->width, texture->wrap);
	const i32 safe_y = se_texture_wrap_index(y, texture->height, texture->wrap);
	const sz idx = ((sz)safe_y * (sz)texture->width + (sz)safe_x) * (sz)texture->cpu_channels;
	const u8 *pixel = texture->cpu_pixels + idx;
	return s_vec3(
		(f32)pixel[0] / 255.0f,
		(f32)pixel[1] / 255.0f,
		(f32)pixel[2] / 255.0f
	);
}

b8 se_texture_sample_rgb(const se_texture_handle texture_handle, const s_vec2 *uv, s_vec3 *out_color) {
	se_context *ctx = se_current_context();
	if (!ctx || !uv || !out_color) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const se_texture *texture = se_texture_from_handle(ctx, texture_handle);
	if (!texture || texture->id == 0u || texture->target != GL_TEXTURE_2D || texture->width <= 0 || texture->height <= 0 || texture->cpu_channels < 3 || !texture->cpu_pixels) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	const f32 u = se_texture_wrap_coordinate(uv->x, texture->wrap);
	const f32 v = se_texture_wrap_coordinate(uv->y, texture->wrap);
	const f32 fx = u * (f32)(texture->width - 1);
	const f32 fy = v * (f32)(texture->height - 1);
	const i32 x0 = (i32)floorf(fx);
	const i32 y0 = (i32)floorf(fy);
	const i32 x1 = x0 + 1;
	const i32 y1 = y0 + 1;
	const f32 tx = fx - (f32)x0;
	const f32 ty = fy - (f32)y0;

	const s_vec3 c00 = se_texture_read_rgb(texture, x0, y0);
	const s_vec3 c10 = se_texture_read_rgb(texture, x1, y0);
	const s_vec3 c01 = se_texture_read_rgb(texture, x0, y1);
	const s_vec3 c11 = se_texture_read_rgb(texture, x1, y1);

	const s_vec3 cx0 = s_vec3(
		c00.x + (c10.x - c00.x) * tx,
		c00.y + (c10.y - c00.y) * tx,
		c00.z + (c10.z - c00.z) * tx
	);
	const s_vec3 cx1 = s_vec3(
		c01.x + (c11.x - c01.x) * tx,
		c01.y + (c11.y - c01.y) * tx,
		c01.z + (c11.z - c01.z) * tx
	);

	*out_color = s_vec3(
		cx0.x + (cx1.x - cx0.x) * ty,
		cx0.y + (cx1.y - cx0.y) * ty,
		cx0.z + (cx1.z - cx0.z) * ty
	);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void se_texture_cleanup(se_texture *texture) {
	if (!texture) {
		return;
	}
	if (texture->id != 0u) {
		glDeleteTextures(1, &texture->id);
	}
	if (texture->cpu_pixels) {
		free(texture->cpu_pixels);
	}
	texture->id = 0u;
	texture->width = 0;
	texture->height = 0;
	texture->depth = 0;
	texture->channels = 0;
	texture->cpu_channels = 0;
	texture->target = 0u;
	texture->wrap = SE_REPEAT;
	texture->cpu_pixels = NULL;
	texture->cpu_pixels_size = 0;
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
