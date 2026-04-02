// Syphax-Engine - Ougi Washi

#ifndef SE_NOISE_H
#define SE_NOISE_H

#include "se.h"

typedef enum se_noise_type {
	SE_NOISE_SIMPLEX,
	SE_NOISE_PERLIN,
	SE_NOISE_VORNOI,
	SE_NOISE_WORLEY,
} se_noise_type;

typedef struct se_noise_2d {
	se_noise_type type;
	f32 frequency;
	s_vec2 offset;
	s_vec2 scale;
	u32 seed;
} se_noise_2d;

typedef struct se_noise_3d {
	se_noise_type type;
	f32 frequency;
	s_vec3 offset;
	s_vec3 scale;
	u32 seed;
} se_noise_3d;

extern se_texture_handle se_noise_2d_create(se_context* ctx, const se_noise_2d* noise);
extern void se_noise_update(se_context* ctx, se_texture_handle texture, const se_noise_2d* noise);
extern void se_noise_2d_destroy(se_context* ctx, se_texture_handle texture);

#endif // SE_NOISE_H
