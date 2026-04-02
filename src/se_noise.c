// Syphax-Engine - Ougi Washi

#include "se_noise.h"

#include "se_texture.h"
#include "render/se_gl.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#define SE_NOISE_2D_DEFAULT_SIZE 256u
#define SE_NOISE_MIN_FREQUENCY 0.0001f
#define SE_NOISE_SCALE_EPSILON 0.0001f
#define SE_NOISE_SIMPLEX_F2 0.36602540378f
#define SE_NOISE_SIMPLEX_G2 0.21132486540f
#define SE_NOISE_INV_SQRT2 0.70710678118f
#define SE_NOISE_SQRT2 1.41421356237f

typedef f32 (*se_noise_sample_fn)(const s_vec2* point, u32 seed);

static se_texture* se_noise_texture_from_handle(se_context* ctx, const se_texture_handle texture) {
	return ctx ? s_array_get(&ctx->textures, texture) : NULL;
}

static void se_noise_upload_texture_2d(se_texture* texture, const u8* pixels, const u32 width, const u32 height) {
	if (!texture || !pixels || width == 0u || height == 0u) {
		return;
	}
	if (texture->id == 0u) {
		glGenTextures(1, &texture->id);
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static f32 se_noise_clampf(const f32 value, const f32 min_value, const f32 max_value) {
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

static f32 se_noise_lerp(const f32 a, const f32 b, const f32 t) {
	return a + (b - a) * t;
}

static f32 se_noise_frequency_defaulted(const se_noise_2d* noise) {
	if (!noise || noise->frequency <= 0.0f) {
		return 1.0f;
	}
	return s_max(noise->frequency, SE_NOISE_MIN_FREQUENCY);
}

static f32 se_noise_scale_component_defaulted(const f32 value) {
	if (fabsf(value) <= SE_NOISE_SCALE_EPSILON) {
		return 1.0f;
	}
	return value;
}

static s_vec2 se_noise_scale_defaulted(const se_noise_2d* noise) {
	if (!noise) {
		return s_vec2(1.0f, 1.0f);
	}
	return s_vec2(
		se_noise_scale_component_defaulted(noise->scale.x),
		se_noise_scale_component_defaulted(noise->scale.y));
}

static u8 se_noise_unit_to_u8(const f32 value) {
	const f32 clamped = se_noise_clampf(value, 0.0f, 1.0f);
	return (u8)lroundf(clamped * 255.0f);
}

static u32 se_noise_hash_u32(u32 value) {
	value ^= value >> 16;
	value *= 0x7FEB352Du;
	value ^= value >> 15;
	value *= 0x846CA68Bu;
	value ^= value >> 16;
	return value;
}

static u32 se_noise_hash_2d(const i32 x, const i32 y, const u32 seed) {
	u32 value = seed ^ 0x9E3779B9u;
	value ^= (u32)x * 0x85EBCA6Bu;
	value = se_noise_hash_u32(value);
	value ^= (u32)y * 0xC2B2AE35u;
	return se_noise_hash_u32(value);
}

static f32 se_noise_hash_unit(const u32 value) {
	return (f32)(value & 0x00FFFFFFu) / (f32)0x00FFFFFFu;
}

static s_vec2 se_noise_feature_point(const i32 cell_x, const i32 cell_y, const u32 seed) {
	const f32 jitter_x = se_noise_hash_unit(se_noise_hash_2d(cell_x, cell_y, seed));
	const f32 jitter_y = se_noise_hash_unit(se_noise_hash_2d(cell_x, cell_y, seed ^ 0xA511E9B3u));
	return s_vec2((f32)cell_x + jitter_x, (f32)cell_y + jitter_y);
}

static f32 se_noise_sample_perlin(const s_vec2* point, const u32 seed) {
	if (!point) {
		return 0.5f;
	}
	return se_noise_clampf(stb_perlin_noise3_seed(point->x, point->y, 0.0f, 0, 0, 0, (i32)seed) * 0.5f + 0.5f, 0.0f, 1.0f);
}

static s_vec2 se_noise_simplex_gradient(const u32 hash) {
	switch (hash & 7u) {
		case 0u:
			return s_vec2(SE_NOISE_INV_SQRT2, SE_NOISE_INV_SQRT2);
		case 1u:
			return s_vec2(-SE_NOISE_INV_SQRT2, SE_NOISE_INV_SQRT2);
		case 2u:
			return s_vec2(SE_NOISE_INV_SQRT2, -SE_NOISE_INV_SQRT2);
		case 3u:
			return s_vec2(-SE_NOISE_INV_SQRT2, -SE_NOISE_INV_SQRT2);
		case 4u:
			return s_vec2(1.0f, 0.0f);
		case 5u:
			return s_vec2(-1.0f, 0.0f);
		case 6u:
			return s_vec2(0.0f, 1.0f);
		default:
			return s_vec2(0.0f, -1.0f);
	}
}

static f32 se_noise_simplex_corner(const i32 lattice_x, const i32 lattice_y, const f32 x, const f32 y, const u32 seed) {
	f32 t = 0.5f - x * x - y * y;
	if (t <= 0.0f) {
		return 0.0f;
	}

	const s_vec2 gradient = se_noise_simplex_gradient(se_noise_hash_2d(lattice_x, lattice_y, seed));
	t *= t;
	return t * t * (gradient.x * x + gradient.y * y);
}

static f32 se_noise_sample_simplex(const s_vec2* point, const u32 seed) {
	if (!point) {
		return 0.5f;
	}

	const f32 skew = (point->x + point->y) * SE_NOISE_SIMPLEX_F2;
	const i32 cell_x = (i32)floorf(point->x + skew);
	const i32 cell_y = (i32)floorf(point->y + skew);
	const f32 unskew = (f32)(cell_x + cell_y) * SE_NOISE_SIMPLEX_G2;
	const f32 origin_x = (f32)cell_x - unskew;
	const f32 origin_y = (f32)cell_y - unskew;
	const f32 x0 = point->x - origin_x;
	const f32 y0 = point->y - origin_y;

	const i32 offset_x = x0 > y0 ? 1 : 0;
	const i32 offset_y = x0 > y0 ? 0 : 1;
	const f32 x1 = x0 - (f32)offset_x + SE_NOISE_SIMPLEX_G2;
	const f32 y1 = y0 - (f32)offset_y + SE_NOISE_SIMPLEX_G2;
	const f32 x2 = x0 - 1.0f + 2.0f * SE_NOISE_SIMPLEX_G2;
	const f32 y2 = y0 - 1.0f + 2.0f * SE_NOISE_SIMPLEX_G2;

	const f32 n0 = se_noise_simplex_corner(cell_x, cell_y, x0, y0, seed);
	const f32 n1 = se_noise_simplex_corner(cell_x + offset_x, cell_y + offset_y, x1, y1, seed);
	const f32 n2 = se_noise_simplex_corner(cell_x + 1, cell_y + 1, x2, y2, seed);
	const f32 value = 70.0f * (n0 + n1 + n2);

	return se_noise_clampf(value * 0.5f + 0.5f, 0.0f, 1.0f);
}

static f32 se_noise_sample_vornoi(const s_vec2* point, const u32 seed) {
	if (!point) {
		return 0.0f;
	}

	const i32 base_x = (i32)floorf(point->x);
	const i32 base_y = (i32)floorf(point->y);
	f32 best_distance_sq = 1e30f;
	i32 best_cell_x = base_x;
	i32 best_cell_y = base_y;

	for (i32 dy = -1; dy <= 1; ++dy) {
		for (i32 dx = -1; dx <= 1; ++dx) {
			const i32 cell_x = base_x + dx;
			const i32 cell_y = base_y + dy;
			const s_vec2 feature = se_noise_feature_point(cell_x, cell_y, seed);
			const f32 delta_x = feature.x - point->x;
			const f32 delta_y = feature.y - point->y;
			const f32 distance_sq = delta_x * delta_x + delta_y * delta_y;
			if (distance_sq < best_distance_sq) {
				best_distance_sq = distance_sq;
				best_cell_x = cell_x;
				best_cell_y = cell_y;
			}
		}
	}

	return se_noise_hash_unit(se_noise_hash_2d(best_cell_x, best_cell_y, seed ^ 0x3C6EF372u));
}

static f32 se_noise_sample_worley(const s_vec2* point, const u32 seed) {
	if (!point) {
		return 0.0f;
	}

	const i32 base_x = (i32)floorf(point->x);
	const i32 base_y = (i32)floorf(point->y);
	f32 best_distance_sq = 1e30f;

	for (i32 dy = -1; dy <= 1; ++dy) {
		for (i32 dx = -1; dx <= 1; ++dx) {
			const s_vec2 feature = se_noise_feature_point(base_x + dx, base_y + dy, seed);
			const f32 delta_x = feature.x - point->x;
			const f32 delta_y = feature.y - point->y;
			const f32 distance_sq = delta_x * delta_x + delta_y * delta_y;
			if (distance_sq < best_distance_sq) {
				best_distance_sq = distance_sq;
			}
		}
	}

	return 1.0f - se_noise_clampf(sqrtf(best_distance_sq) / SE_NOISE_SQRT2, 0.0f, 1.0f);
}

static se_noise_sample_fn se_noise_sample_fn_from_type(const se_noise_type type) {
	switch (type) {
		case SE_NOISE_SIMPLEX:
			return se_noise_sample_simplex;
		case SE_NOISE_PERLIN:
			return se_noise_sample_perlin;
		case SE_NOISE_VORNOI:
			return se_noise_sample_vornoi;
		case SE_NOISE_WORLEY:
			return se_noise_sample_worley;
		default:
			return NULL;
	}
}

static f32 se_noise_sample_tileable(const se_noise_2d* noise, se_noise_sample_fn sample_fn, const f32 u, const f32 v) {
	const f32 frequency = se_noise_frequency_defaulted(noise);
	const s_vec2 scale = se_noise_scale_defaulted(noise);
	const s_vec2 period = s_vec2(scale.x * frequency, scale.y * frequency);
	const s_vec2 p00 = s_vec2(noise->offset.x + u * period.x, noise->offset.y + v * period.y);

	if (fabsf(period.x) <= SE_NOISE_SCALE_EPSILON && fabsf(period.y) <= SE_NOISE_SCALE_EPSILON) {
		return sample_fn(&p00, noise->seed);
	}

	const s_vec2 p10 = s_vec2(p00.x - period.x, p00.y);
	const s_vec2 p01 = s_vec2(p00.x, p00.y - period.y);
	const s_vec2 p11 = s_vec2(p00.x - period.x, p00.y - period.y);

	const f32 n00 = sample_fn(&p00, noise->seed);
	const f32 n10 = sample_fn(&p10, noise->seed);
	const f32 n01 = sample_fn(&p01, noise->seed);
	const f32 n11 = sample_fn(&p11, noise->seed);
	const f32 nx0 = se_noise_lerp(n00, n10, u);
	const f32 nx1 = se_noise_lerp(n01, n11, u);
	return se_noise_lerp(nx0, nx1, v);
}

static u8* se_noise_2d_build_pixels(const se_noise_2d* noise, const u32 width, const u32 height) {
	const se_noise_sample_fn sample_fn = se_noise_sample_fn_from_type(noise->type);
	u8* pixels = NULL;

	if (!sample_fn || width == 0u || height == 0u) {
		return NULL;
	}

	pixels = malloc((sz)width * (sz)height * 4u);
	if (!pixels) {
		return NULL;
	}

	for (u32 y = 0u; y < height; ++y) {
		for (u32 x = 0u; x < width; ++x) {
			const f32 u = ((f32)x + 0.5f) / (f32)width;
			const f32 v = ((f32)y + 0.5f) / (f32)height;
			const f32 noise_value = se_noise_sample_tileable(noise, sample_fn, u, v);
			const u8 value = se_noise_unit_to_u8(noise_value);
			const sz index = ((sz)y * (sz)width + (sz)x) * 4u;

			pixels[index + 0u] = value;
			pixels[index + 1u] = value;
			pixels[index + 2u] = value;
			pixels[index + 3u] = 255u;
		}
	}

	return pixels;
}

se_texture_handle se_noise_2d_create(se_context* ctx, const se_noise_2d* noise) {
	se_context* previous_context = NULL;
	se_texture_handle texture_handle = S_HANDLE_NULL;
	se_texture* texture = NULL;
	u8* pixels = NULL;
	const u32 width = SE_NOISE_2D_DEFAULT_SIZE;
	const u32 height = SE_NOISE_2D_DEFAULT_SIZE;

	if (!ctx || !noise) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (!se_noise_sample_fn_from_type(noise->type)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	previous_context = se_push_tls_context(ctx);
	pixels = se_noise_2d_build_pixels(noise, width, height);
	if (!pixels) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		se_pop_tls_context(previous_context);
		return S_HANDLE_NULL;
	}

	if (s_array_get_capacity(&ctx->textures) == 0u) {
		s_array_init(&ctx->textures);
	}

	texture_handle = s_array_increment(&ctx->textures);
	texture = se_noise_texture_from_handle(ctx, texture_handle);
	if (!texture) {
		free(pixels);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		se_pop_tls_context(previous_context);
		return S_HANDLE_NULL;
	}

	memset(texture, 0, sizeof(*texture));
	texture->width = (i32)width;
	texture->height = (i32)height;
	texture->depth = 1;
	texture->channels = 4;
	texture->cpu_channels = 4;
	texture->target = GL_TEXTURE_2D;
	texture->wrap = SE_REPEAT;
	texture->cpu_pixels = pixels;
	texture->cpu_pixels_size = (sz)width * (sz)height * 4u;
	texture->path[0] = '\0';
	se_noise_upload_texture_2d(texture, pixels, width, height);

	se_set_last_error(SE_RESULT_OK);
	se_pop_tls_context(previous_context);
	return texture_handle;
}

void se_noise_update(se_context* ctx, se_texture_handle texture_handle, const se_noise_2d* noise) {
	se_context* previous_context = NULL;
	se_texture* texture = NULL;
	u8* pixels = NULL;
	const u32 width = SE_NOISE_2D_DEFAULT_SIZE;
	const u32 height = SE_NOISE_2D_DEFAULT_SIZE;

	if (!ctx || texture_handle == S_HANDLE_NULL || !noise) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}
	if (!se_noise_sample_fn_from_type(noise->type)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}

	texture = se_noise_texture_from_handle(ctx, texture_handle);
	if (!texture) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	previous_context = se_push_tls_context(ctx);
	pixels = se_noise_2d_build_pixels(noise, width, height);
	if (!pixels) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		se_pop_tls_context(previous_context);
		return;
	}

	if (texture->cpu_pixels) {
		free(texture->cpu_pixels);
	}
	texture->width = (i32)width;
	texture->height = (i32)height;
	texture->depth = 1;
	texture->channels = 4;
	texture->cpu_channels = 4;
	texture->target = GL_TEXTURE_2D;
	texture->wrap = SE_REPEAT;
	texture->cpu_pixels = pixels;
	texture->cpu_pixels_size = (sz)width * (sz)height * 4u;
	texture->path[0] = '\0';
	se_noise_upload_texture_2d(texture, pixels, width, height);

	se_set_last_error(SE_RESULT_OK);
	se_pop_tls_context(previous_context);
}

void se_noise_2d_destroy(se_context* ctx, se_texture_handle texture) {
	se_context* previous_context = NULL;
	se_texture* texture_ptr = NULL;

	if (!ctx || texture == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return;
	}

	texture_ptr = se_noise_texture_from_handle(ctx, texture);
	if (!texture_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return;
	}

	previous_context = se_push_tls_context(ctx);
	se_texture_destroy(texture);
	se_set_last_error(SE_RESULT_OK);
	se_pop_tls_context(previous_context);
}
