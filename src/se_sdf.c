// Syphax-Engine - Ougi Washi

#include "se_sdf.h"

#include "render/se_gl.h"
#include "se_camera.h"
#include "se_debug.h"
#include "se_framebuffer.h"
#include "se_shader.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SE_SDF_VERTEX_SHADER_CAPACITY 2048u
#define SE_SDF_FRAGMENT_SHADER_CAPACITY (256u * 1024u)
#define SE_SDF_FAR_DISTANCE 256.0f
#define SE_SDF_MAX_TRACE_STEPS 128u
#define SE_SDF_HIT_EPSILON 0.001f
#define SE_SDF_MATRIX_EPSILON 0.00001f
#define SE_SDF_DEFAULT_SMOOTH_UNION_AMOUNT 0.35f
#define SE_SDF_MIN_SMOOTH_UNION_AMOUNT 0.0001f
#define SE_SDF_DEFAULT_NOISE_AMOUNT 0.2f
#define SE_SDF_MIN_NOISE_FREQUENCY 0.0001f
#define SE_SDF_DEFAULT_BASE_R 0.84f
#define SE_SDF_DEFAULT_BASE_G 0.86f
#define SE_SDF_DEFAULT_BASE_B 0.90f
#define SE_SDF_DEFAULT_AMBIENT_FACTOR 0.22f
#define SE_SDF_DEFAULT_SPECULAR_R 0.18f
#define SE_SDF_DEFAULT_SPECULAR_G 0.19f
#define SE_SDF_DEFAULT_SPECULAR_B 0.22f
#define SE_SDF_DEFAULT_ROUGHNESS 0.35f
#define SE_SDF_DEFAULT_SHADING_BIAS 0.5f
#define SE_SDF_DEFAULT_SHADING_SMOOTHNESS 1.0f
#define SE_SDF_DEFAULT_SHADOW_SOFTNESS 8.0f
#define SE_SDF_DEFAULT_SHADOW_BIAS 0.02f
#define SE_SDF_DEFAULT_SHADOW_SAMPLES 48u
#define SE_SDF_MAX_SHADOW_SAMPLES 128u
#define SE_SDF_DEFAULT_POINT_LIGHT_RADIUS 8.0f
#define SE_SDF_DEFAULT_LIGHT_DIR_X 0.45f
#define SE_SDF_DEFAULT_LIGHT_DIR_Y 0.85f
#define SE_SDF_DEFAULT_LIGHT_DIR_Z 0.35f
#define SE_SDF_DEFAULT_LOD_HIGH_DISTANCE 8.0f
#define SE_SDF_DEFAULT_LOD_MEDIUM_DISTANCE 20.0f
#define SE_SDF_DEFAULT_LOD_HIGH_STEPS SE_SDF_MAX_TRACE_STEPS
#define SE_SDF_DEFAULT_LOD_MEDIUM_STEPS 48u
#define SE_SDF_DEFAULT_LOD_LOW_STEPS 20u
#define SE_SDF_LOD_HIGH_HIT_EPSILON SE_SDF_HIT_EPSILON
#define SE_SDF_LOD_MEDIUM_HIT_EPSILON 0.002f
#define SE_SDF_LOD_LOW_HIT_EPSILON 0.004f
#define SE_SDF_HANDLE_FMT "%llu"
#define SE_SDF_HANDLE_ARG(_handle) ((unsigned long long)(_handle))

static se_sdf* se_sdf_from_handle(se_context* ctx, const se_sdf_handle sdf);
static se_sdf_noise* se_sdf_noise_from_handle(se_context* ctx, const se_sdf_noise_handle noise);
static se_sdf_point_light* se_sdf_point_light_from_handle(se_context* ctx, const se_sdf_point_light_handle point_light);
static se_sdf_directional_light* se_sdf_directional_light_from_handle(se_context* ctx, const se_sdf_directional_light_handle directional_light);
static b8 se_sdf_noise_is_referenced(se_context* ctx, const se_sdf_noise_handle noise);
static b8 se_sdf_point_light_is_referenced(se_context* ctx, const se_sdf_point_light_handle point_light);
static b8 se_sdf_directional_light_is_referenced(se_context* ctx, const se_sdf_directional_light_handle directional_light);
static void se_sdf_remove_child_reference(se_sdf* parent_ptr, const se_sdf_handle child);
static void se_sdf_release_noise_references(se_context* ctx, se_sdf* sdf_ptr);
static void se_sdf_release_point_light_references(se_context* ctx, se_sdf* sdf_ptr);
static void se_sdf_release_directional_light_references(se_context* ctx, se_sdf* sdf_ptr);
static b8 se_sdf_transform_is_zero(const s_mat4* transform);
static b8 se_sdf_vec3_is_zero(const s_vec3* value);
static b8 se_sdf_shading_is_zero(const se_sdf_shading* shading);
static b8 se_sdf_shadow_is_zero(const se_sdf_shadow* shadow);
static s_vec4 se_sdf_mul_mat4_vec4(const s_mat4* matrix, const s_vec4* vector);
static s_vec3 se_sdf_transform_point(const s_mat4* transform, const s_vec3* point);
static s_vec3 se_sdf_transform_direction(const s_mat4* transform, const s_vec3* direction);
static b8 se_sdf_noise_is_active(const se_sdf_noise* noise);
static b8 se_sdf_has_primitive(const se_sdf* sdf);
static b8 se_sdf_has_noise(const se_sdf* sdf);
static b8 se_sdf_needs_inverse_transform(const se_sdf* sdf);
static b8 se_sdf_has_lights_recursive(const se_sdf_handle sdf);
static se_sdf_shading se_sdf_get_default_shading(void);
static se_sdf_shading se_sdf_get_shading_defaulted(const se_sdf* sdf);
static se_sdf_shadow se_sdf_get_default_shadow(void);
static se_sdf_shadow se_sdf_get_shadow_defaulted(const se_sdf* sdf);
static b8 se_sdf_lod_is_zero(const se_sdf_lod* lod);
static b8 se_sdf_lods_are_zero(const se_sdf_lods* lods);
static u16 se_sdf_clamp_lod_steps(u16 steps);
static se_sdf_lods se_sdf_get_default_lods(void);
static se_sdf_lods se_sdf_get_lods_defaulted(const se_sdf* sdf);
static f32 se_sdf_get_operation_amount(const se_sdf* sdf);
static f32 se_sdf_get_noise_frequency_defaulted(const se_sdf_noise* noise);
static f32 se_sdf_get_point_light_radius_defaulted(const se_sdf_point_light* point_light);
static void se_sdf_append(c8* out, const sz capacity, const c8* fmt, ...);
static const c8* se_sdf_get_operation_function_name(const se_sdf_operator operation);
static const c8* se_sdf_get_noise_function_name(const se_sdf_noise_type type);
static void se_sdf_destroy_shader_runtime(se_sdf* sdf_ptr);
static void se_sdf_invalidate_shader_chain(const se_sdf_handle sdf);
static void se_sdf_gen_lod_helpers(c8* out, const sz capacity);
static void se_sdf_gen_operator_functions(c8* out, const sz capacity);
static void se_sdf_gen_noise_functions(c8* out, const sz capacity);
static void se_sdf_gen_shading_helpers(c8* out, const sz capacity);
static void se_sdf_gen_light_functions(c8* out, const sz capacity);
static void se_sdf_gen_uniform_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_function_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_surface_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_light_apply_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_upload_lod_uniforms(const se_shader_handle shader, const se_sdf* sdf_ptr);
static void se_sdf_upload_uniforms_recursive(const se_shader_handle shader, const se_sdf_handle sdf);

static se_sdf* se_sdf_from_handle(se_context* ctx, const se_sdf_handle sdf) {
	if (!ctx || sdf == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->sdfs, sdf);
}

static se_sdf_noise* se_sdf_noise_from_handle(se_context* ctx, const se_sdf_noise_handle noise) {
	if (!ctx || noise == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->sdf_noises, noise);
}

static se_sdf_point_light* se_sdf_point_light_from_handle(se_context* ctx, const se_sdf_point_light_handle point_light) {
	if (!ctx || point_light == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->sdf_point_lights, point_light);
}

static se_sdf_directional_light* se_sdf_directional_light_from_handle(se_context* ctx, const se_sdf_directional_light_handle directional_light) {
	if (!ctx || directional_light == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->sdf_directional_lights, directional_light);
}

static b8 se_sdf_noise_is_referenced(se_context* ctx, const se_sdf_noise_handle noise) {
	se_sdf* sdf_ptr = NULL;
	se_sdf_noise_handle* noise_handle_ptr = NULL;
	if (!ctx || noise == S_HANDLE_NULL) {
		return false;
	}
	s_foreach(&ctx->sdfs, sdf_ptr) {
		s_foreach(&sdf_ptr->noises, noise_handle_ptr) {
			if (*noise_handle_ptr == noise) {
				return true;
			}
		}
	}
	return false;
}

static b8 se_sdf_point_light_is_referenced(se_context* ctx, const se_sdf_point_light_handle point_light) {
	se_sdf* sdf_ptr = NULL;
	se_sdf_point_light_handle* point_light_handle_ptr = NULL;
	if (!ctx || point_light == S_HANDLE_NULL) {
		return false;
	}
	s_foreach(&ctx->sdfs, sdf_ptr) {
		s_foreach(&sdf_ptr->point_lights, point_light_handle_ptr) {
			if (*point_light_handle_ptr == point_light) {
				return true;
			}
		}
	}
	return false;
}

static b8 se_sdf_directional_light_is_referenced(se_context* ctx, const se_sdf_directional_light_handle directional_light) {
	se_sdf* sdf_ptr = NULL;
	se_sdf_directional_light_handle* directional_light_handle_ptr = NULL;
	if (!ctx || directional_light == S_HANDLE_NULL) {
		return false;
	}
	s_foreach(&ctx->sdfs, sdf_ptr) {
		s_foreach(&sdf_ptr->directional_lights, directional_light_handle_ptr) {
			if (*directional_light_handle_ptr == directional_light) {
				return true;
			}
		}
	}
	return false;
}

static void se_sdf_remove_child_reference(se_sdf* parent_ptr, const se_sdf_handle child) {
	u32 i = 0;
	if (!parent_ptr || child == S_HANDLE_NULL) {
		return;
	}
	for (i = 0; i < (u32)s_array_get_size(&parent_ptr->children); ++i) {
		const s_handle local_handle = s_array_handle_at(&parent_ptr->children.b, i);
		se_sdf_handle* child_handle_ptr = s_array_get(&parent_ptr->children, local_handle);
		if (!child_handle_ptr || *child_handle_ptr != child) {
			continue;
		}
		s_array_remove(&parent_ptr->children, local_handle);
		return;
	}
}

static void se_sdf_release_noise_references(se_context* ctx, se_sdf* sdf_ptr) {
	while (ctx && sdf_ptr && s_array_get_size(&sdf_ptr->noises) > 0) {
		const s_handle local_handle = s_array_handle(&sdf_ptr->noises, (u32)(s_array_get_size(&sdf_ptr->noises) - 1u));
		se_sdf_noise_handle* noise_handle_ptr = s_array_get(&sdf_ptr->noises, local_handle);
		se_sdf_noise_handle noise = S_HANDLE_NULL;
		if (!noise_handle_ptr) {
			s_array_remove(&sdf_ptr->noises, local_handle);
			continue;
		}
		noise = *noise_handle_ptr;
		s_array_remove(&sdf_ptr->noises, local_handle);
		if (!se_sdf_noise_is_referenced(ctx, noise)) {
			s_array_remove(&ctx->sdf_noises, noise);
		}
	}
}

static void se_sdf_release_point_light_references(se_context* ctx, se_sdf* sdf_ptr) {
	while (ctx && sdf_ptr && s_array_get_size(&sdf_ptr->point_lights) > 0) {
		const s_handle local_handle = s_array_handle(&sdf_ptr->point_lights, (u32)(s_array_get_size(&sdf_ptr->point_lights) - 1u));
		se_sdf_point_light_handle* point_light_handle_ptr = s_array_get(&sdf_ptr->point_lights, local_handle);
		se_sdf_point_light_handle point_light = S_HANDLE_NULL;
		if (!point_light_handle_ptr) {
			s_array_remove(&sdf_ptr->point_lights, local_handle);
			continue;
		}
		point_light = *point_light_handle_ptr;
		s_array_remove(&sdf_ptr->point_lights, local_handle);
		if (!se_sdf_point_light_is_referenced(ctx, point_light)) {
			s_array_remove(&ctx->sdf_point_lights, point_light);
		}
	}
}

static void se_sdf_release_directional_light_references(se_context* ctx, se_sdf* sdf_ptr) {
	while (ctx && sdf_ptr && s_array_get_size(&sdf_ptr->directional_lights) > 0) {
		const s_handle local_handle = s_array_handle(&sdf_ptr->directional_lights, (u32)(s_array_get_size(&sdf_ptr->directional_lights) - 1u));
		se_sdf_directional_light_handle* directional_light_handle_ptr = s_array_get(&sdf_ptr->directional_lights, local_handle);
		se_sdf_directional_light_handle directional_light = S_HANDLE_NULL;
		if (!directional_light_handle_ptr) {
			s_array_remove(&sdf_ptr->directional_lights, local_handle);
			continue;
		}
		directional_light = *directional_light_handle_ptr;
		s_array_remove(&sdf_ptr->directional_lights, local_handle);
		if (!se_sdf_directional_light_is_referenced(ctx, directional_light)) {
			s_array_remove(&ctx->sdf_directional_lights, directional_light);
		}
	}
}

static b8 se_sdf_transform_is_zero(const s_mat4* transform) {
	if (!transform) {
		return true;
	}
	for (u32 row = 0; row < 4u; ++row) {
		for (u32 col = 0; col < 4u; ++col) {
			if (fabsf(transform->m[row][col]) > SE_SDF_MATRIX_EPSILON) {
				return false;
			}
		}
	}
	return true;
}

static b8 se_sdf_vec3_is_zero(const s_vec3* value) {
	return !value || s_vec3_length(value) <= SE_SDF_MATRIX_EPSILON;
}

static b8 se_sdf_shading_is_zero(const se_sdf_shading* shading) {
	return !shading
		|| (se_sdf_vec3_is_zero(&shading->ambient)
		&& se_sdf_vec3_is_zero(&shading->diffuse)
		&& se_sdf_vec3_is_zero(&shading->specular)
		&& fabsf(shading->roughness) <= SE_SDF_MATRIX_EPSILON
		&& fabsf(shading->bias) <= SE_SDF_MATRIX_EPSILON
		&& fabsf(shading->smoothness) <= SE_SDF_MATRIX_EPSILON);
}

static b8 se_sdf_shadow_is_zero(const se_sdf_shadow* shadow) {
	return !shadow
		|| (fabsf(shadow->softness) <= SE_SDF_MATRIX_EPSILON
		&& fabsf(shadow->bias) <= SE_SDF_MATRIX_EPSILON
		&& shadow->samples == 0u);
}

static s_vec4 se_sdf_mul_mat4_vec4(const s_mat4* matrix, const s_vec4* vector) {
	return s_vec4(
		matrix->m[0][0] * vector->x + matrix->m[1][0] * vector->y + matrix->m[2][0] * vector->z + matrix->m[3][0] * vector->w,
		matrix->m[0][1] * vector->x + matrix->m[1][1] * vector->y + matrix->m[2][1] * vector->z + matrix->m[3][1] * vector->w,
		matrix->m[0][2] * vector->x + matrix->m[1][2] * vector->y + matrix->m[2][2] * vector->z + matrix->m[3][2] * vector->w,
		matrix->m[0][3] * vector->x + matrix->m[1][3] * vector->y + matrix->m[2][3] * vector->z + matrix->m[3][3] * vector->w);
}

static s_vec3 se_sdf_transform_point(const s_mat4* transform, const s_vec3* point) {
	const s_vec4 transformed = se_sdf_mul_mat4_vec4(transform, &s_vec4(point->x, point->y, point->z, 1.0f));
	return s_vec3(transformed.x, transformed.y, transformed.z);
}

static s_vec3 se_sdf_transform_direction(const s_mat4* transform, const s_vec3* direction) {
	const s_vec4 transformed = se_sdf_mul_mat4_vec4(transform, &s_vec4(direction->x, direction->y, direction->z, 0.0f));
	return s_vec3_normalize(&s_vec3(transformed.x, transformed.y, transformed.z));
}

static b8 se_sdf_noise_is_active(const se_sdf_noise* noise) {
	return noise && noise->type != SE_SDF_NOISE_NONE;
}

static b8 se_sdf_has_primitive(const se_sdf* sdf) {
	return sdf && (sdf->type == SE_SDF_SPHERE || sdf->type == SE_SDF_BOX);
}

static b8 se_sdf_has_noise(const se_sdf* sdf) {
	se_context* ctx = se_current_context();
	if (!sdf) {
		return false;
	}
	for (u32 i = 0; i < (u32)s_array_get_size(&sdf->noises); ++i) {
		const s_handle noise_handle = s_array_handle_at(&sdf->noises.b, i);
		const se_sdf_noise_handle* noise_handle_ptr = s_array_get_ptr(&sdf->noises.b, sizeof(se_sdf_noise_handle), noise_handle);
		const se_sdf_noise* noise = noise_handle_ptr ? se_sdf_noise_from_handle(ctx, *noise_handle_ptr) : NULL;
		if (se_sdf_noise_is_active(noise)) {
			return true;
		}
	}
	return false;
}

static b8 se_sdf_needs_inverse_transform(const se_sdf* sdf) {
	return se_sdf_has_primitive(sdf) || se_sdf_has_noise(sdf);
}

static b8 se_sdf_has_lights_recursive(const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	if (!sdf_ptr) {
		return false;
	}
	if (s_array_get_size(&sdf_ptr->point_lights) > 0 || s_array_get_size(&sdf_ptr->directional_lights) > 0) {
		return true;
	}
	s_foreach(&sdf_ptr->children, child) {
		if (se_sdf_has_lights_recursive(*child)) {
			return true;
		}
	}
	return false;
}

static se_sdf_shading se_sdf_get_default_shading(void) {
	return (se_sdf_shading){
		.ambient = s_vec3(
			SE_SDF_DEFAULT_BASE_R * SE_SDF_DEFAULT_AMBIENT_FACTOR,
			SE_SDF_DEFAULT_BASE_G * SE_SDF_DEFAULT_AMBIENT_FACTOR,
			SE_SDF_DEFAULT_BASE_B * SE_SDF_DEFAULT_AMBIENT_FACTOR),
		.diffuse = s_vec3(SE_SDF_DEFAULT_BASE_R, SE_SDF_DEFAULT_BASE_G, SE_SDF_DEFAULT_BASE_B),
		.specular = s_vec3(SE_SDF_DEFAULT_SPECULAR_R, SE_SDF_DEFAULT_SPECULAR_G, SE_SDF_DEFAULT_SPECULAR_B),
		.roughness = SE_SDF_DEFAULT_ROUGHNESS,
		.bias = SE_SDF_DEFAULT_SHADING_BIAS,
		.smoothness = SE_SDF_DEFAULT_SHADING_SMOOTHNESS,
	};
}

static se_sdf_shading se_sdf_get_shading_defaulted(const se_sdf* sdf) {
	se_sdf_shading shading = se_sdf_get_default_shading();
	if (!sdf) {
		return shading;
	}
	if (!se_sdf_shading_is_zero(&sdf->shading)) {
		shading = sdf->shading;
	}
	shading.roughness = fminf(fmaxf(shading.roughness, 0.0f), 1.0f);
	shading.bias = fminf(fmaxf(shading.bias, 0.0f), 1.0f);
	shading.smoothness = fmaxf(shading.smoothness, 0.0f);
	return shading;
}

static se_sdf_shadow se_sdf_get_default_shadow(void) {
	return (se_sdf_shadow){
		.softness = SE_SDF_DEFAULT_SHADOW_SOFTNESS,
		.bias = SE_SDF_DEFAULT_SHADOW_BIAS,
		.samples = SE_SDF_DEFAULT_SHADOW_SAMPLES,
	};
}

static se_sdf_shadow se_sdf_get_shadow_defaulted(const se_sdf* sdf) {
	se_sdf_shadow shadow = se_sdf_get_default_shadow();
	if (!sdf) {
		return shadow;
	}
	if (!se_sdf_shadow_is_zero(&sdf->shadow)) {
		shadow = sdf->shadow;
	}
	shadow.softness = fmaxf(shadow.softness, 0.0f);
	shadow.bias = fmaxf(shadow.bias, 0.0f);
	shadow.samples = shadow.samples == 0u
		? SE_SDF_DEFAULT_SHADOW_SAMPLES
		: (u16)((shadow.samples > SE_SDF_MAX_SHADOW_SAMPLES) ? SE_SDF_MAX_SHADOW_SAMPLES : shadow.samples);
	return shadow;
}

static b8 se_sdf_lod_is_zero(const se_sdf_lod* lod) {
	return !lod
		|| (lod->distance <= 0.0f
		&& lod->steps == 0u
		&& !lod->noise
		&& !lod->point_lights);
}

static b8 se_sdf_lods_are_zero(const se_sdf_lods* lods) {
	return !lods
		|| (se_sdf_lod_is_zero(&lods->high)
		&& se_sdf_lod_is_zero(&lods->medium)
		&& se_sdf_lod_is_zero(&lods->low));
}

static u16 se_sdf_clamp_lod_steps(u16 steps) {
	if (steps == 0u) {
		return 1u;
	}
	if (steps > SE_SDF_MAX_TRACE_STEPS) {
		return SE_SDF_MAX_TRACE_STEPS;
	}
	return steps;
}

static se_sdf_lods se_sdf_get_default_lods(void) {
	return (se_sdf_lods){
		.high = {
			.distance = SE_SDF_DEFAULT_LOD_HIGH_DISTANCE,
			.steps = SE_SDF_DEFAULT_LOD_HIGH_STEPS,
			.noise = true,
			.point_lights = true,
		},
		.medium = {
			.distance = SE_SDF_DEFAULT_LOD_MEDIUM_DISTANCE,
			.steps = SE_SDF_DEFAULT_LOD_MEDIUM_STEPS,
			.noise = true,
			.point_lights = true,
		},
		.low = {
			.distance = SE_SDF_FAR_DISTANCE,
			.steps = SE_SDF_DEFAULT_LOD_LOW_STEPS,
			.noise = false,
			.point_lights = false,
		},
	};
}

static se_sdf_lods se_sdf_get_lods_defaulted(const se_sdf* sdf) {
	se_sdf_lods lods = se_sdf_get_default_lods();
	if (!sdf || se_sdf_lods_are_zero(&sdf->lods)) {
		return lods;
	}

	if (sdf->lods.high.distance > 0.0f) {
		lods.high.distance = sdf->lods.high.distance;
	}
	if (sdf->lods.medium.distance > 0.0f) {
		lods.medium.distance = sdf->lods.medium.distance;
	}
	if (sdf->lods.low.distance > 0.0f) {
		lods.low.distance = sdf->lods.low.distance;
	}
	if (sdf->lods.high.steps > 0u) {
		lods.high.steps = sdf->lods.high.steps;
	}
	if (sdf->lods.medium.steps > 0u) {
		lods.medium.steps = sdf->lods.medium.steps;
	}
	if (sdf->lods.low.steps > 0u) {
		lods.low.steps = sdf->lods.low.steps;
	}

	lods.high.noise = sdf->lods.high.noise;
	lods.high.point_lights = sdf->lods.high.point_lights;
	lods.medium.noise = sdf->lods.medium.noise;
	lods.medium.point_lights = sdf->lods.medium.point_lights;
	lods.low.noise = sdf->lods.low.noise;
	lods.low.point_lights = sdf->lods.low.point_lights;

	lods.low.distance = fminf(fmaxf(lods.low.distance, 0.0f), SE_SDF_FAR_DISTANCE);
	lods.medium.distance = fminf(fmaxf(lods.medium.distance, 0.0f), lods.low.distance);
	lods.high.distance = fminf(fmaxf(lods.high.distance, 0.0f), lods.medium.distance);
	lods.high.steps = se_sdf_clamp_lod_steps(lods.high.steps);
	lods.medium.steps = se_sdf_clamp_lod_steps(lods.medium.steps);
	lods.low.steps = se_sdf_clamp_lod_steps(lods.low.steps);
	return lods;
}

static f32 se_sdf_get_operation_amount(const se_sdf* sdf) {
	if (!sdf) {
		return 0.0f;
	}
	if (sdf->operation == SE_SDF_SMOOTH_UNION && sdf->operation_amount <= 0.0f) {
		return SE_SDF_DEFAULT_SMOOTH_UNION_AMOUNT;
	}
	return sdf->operation_amount;
}

static f32 se_sdf_get_noise_frequency_defaulted(const se_sdf_noise* noise) {
	if (!noise || noise->frequency <= 0.0f) {
		return 1.0f;
	}
	if (noise->frequency < SE_SDF_MIN_NOISE_FREQUENCY) {
		return SE_SDF_MIN_NOISE_FREQUENCY;
	}
	return noise->frequency;
}

static f32 se_sdf_get_point_light_radius_defaulted(const se_sdf_point_light* point_light) {
	if (!point_light || point_light->radius <= 0.0f) {
		return SE_SDF_DEFAULT_POINT_LIGHT_RADIUS;
	}
	return point_light->radius;
}

static void se_sdf_append(c8* out, const sz capacity, const c8* fmt, ...) {
	if (!out || capacity == 0 || !fmt) {
		return;
	}
	const sz current_length = strlen(out);
	if (current_length >= capacity - 1u) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	vsnprintf(out + current_length, capacity - current_length, fmt, args);
	va_end(args);
}

static const c8* se_sdf_get_operation_function_name(const se_sdf_operator operation) {
	switch (operation) {
		case SE_SDF_SMOOTH_UNION:
			return "sdf_op_smooth_union";
		case SE_SDF_UNION:
		default:
			return "sdf_op_union";
	}
}

static const c8* se_sdf_get_noise_function_name(const se_sdf_noise_type type) {
	switch (type) {
		case SE_SDF_NOISE_PERLIN:
			return "sdf_noise_perlin";
		case SE_SDF_NOISE_VORNOI:
			return "sdf_noise_vornoi";
		case SE_SDF_NOISE_NONE:
		default:
			return NULL;
	}
}

static void se_sdf_destroy_shader_runtime(se_sdf* sdf_ptr) {
	if (!sdf_ptr || sdf_ptr->shader == S_HANDLE_NULL) {
		return;
	}
	se_shader_destroy(sdf_ptr->shader);
	sdf_ptr->shader = S_HANDLE_NULL;
}

static void se_sdf_invalidate_shader_chain(const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf_handle current = sdf;
	while (ctx && current != S_HANDLE_NULL) {
		se_sdf* sdf_ptr = se_sdf_from_handle(ctx, current);
		se_sdf_handle parent = S_HANDLE_NULL;
		if (!sdf_ptr) {
			return;
		}
		parent = sdf_ptr->parent;
		se_sdf_destroy_shader_runtime(sdf_ptr);
		current = parent;
	}
}

static void se_sdf_gen_operator_functions(c8* out, const sz capacity) {
	se_sdf_append(
		out,
		capacity,
		"float sdf_op_smooth_union_factor(float a, float b, float amount) {\n"
		"\tfloat k = max(amount, %.4f);\n"
		"\treturn clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);\n"
		"}\n\n",
		SE_SDF_MIN_SMOOTH_UNION_AMOUNT);
	se_sdf_append(
		out,
		capacity,
		"float sdf_op_union(float a, float b, float amount) {\n"
		"\treturn min(a, b);\n"
		"}\n\n");
	se_sdf_append(
		out,
		capacity,
		"float sdf_op_smooth_union(float a, float b, float amount) {\n"
		"\tfloat k = max(amount, %.4f);\n"
		"\tfloat h = sdf_op_smooth_union_factor(a, b, amount);\n"
		"\treturn mix(b, a, h) - k * h * (1.0 - h);\n"
		"}\n\n",
		SE_SDF_MIN_SMOOTH_UNION_AMOUNT);
}

static void se_sdf_gen_noise_functions(c8* out, const sz capacity) {
	se_sdf_append(
		out,
		capacity,
		"float sdf_hash13(vec3 p) {\n"
		"\treturn fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);\n"
		"}\n\n"
		"vec3 sdf_hash33(vec3 p) {\n"
		"\treturn fract(sin(vec3(\n"
		"\t\tdot(p, vec3(127.1, 311.7, 74.7)),\n"
		"\t\tdot(p, vec3(269.5, 183.3, 246.1)),\n"
		"\t\tdot(p, vec3(113.5, 271.9, 124.6)))) * 43758.5453123) * 2.0 - 1.0;\n"
		"}\n\n"
		"float sdf_noise_perlin(vec3 p) {\n"
		"\tvec3 cell = floor(p);\n"
		"\tvec3 local = fract(p);\n"
		"\tvec3 fade = local * local * (3.0 - 2.0 * local);\n"
		"\tfloat n000 = dot(normalize(sdf_hash33(cell + vec3(0.0, 0.0, 0.0))), local - vec3(0.0, 0.0, 0.0));\n"
		"\tfloat n100 = dot(normalize(sdf_hash33(cell + vec3(1.0, 0.0, 0.0))), local - vec3(1.0, 0.0, 0.0));\n"
		"\tfloat n010 = dot(normalize(sdf_hash33(cell + vec3(0.0, 1.0, 0.0))), local - vec3(0.0, 1.0, 0.0));\n"
		"\tfloat n110 = dot(normalize(sdf_hash33(cell + vec3(1.0, 1.0, 0.0))), local - vec3(1.0, 1.0, 0.0));\n"
		"\tfloat n001 = dot(normalize(sdf_hash33(cell + vec3(0.0, 0.0, 1.0))), local - vec3(0.0, 0.0, 1.0));\n"
		"\tfloat n101 = dot(normalize(sdf_hash33(cell + vec3(1.0, 0.0, 1.0))), local - vec3(1.0, 0.0, 1.0));\n"
		"\tfloat n011 = dot(normalize(sdf_hash33(cell + vec3(0.0, 1.0, 1.0))), local - vec3(0.0, 1.0, 1.0));\n"
		"\tfloat n111 = dot(normalize(sdf_hash33(cell + vec3(1.0, 1.0, 1.0))), local - vec3(1.0, 1.0, 1.0));\n"
		"\tfloat nx00 = mix(n000, n100, fade.x);\n"
		"\tfloat nx10 = mix(n010, n110, fade.x);\n"
		"\tfloat nx01 = mix(n001, n101, fade.x);\n"
		"\tfloat nx11 = mix(n011, n111, fade.x);\n"
		"\tfloat nxy0 = mix(nx00, nx10, fade.y);\n"
		"\tfloat nxy1 = mix(nx01, nx11, fade.y);\n"
		"\treturn mix(nxy0, nxy1, fade.z);\n"
		"}\n\n"
		"float sdf_noise_vornoi(vec3 p) {\n"
		"\tvec3 cell = floor(p);\n"
		"\tvec3 local = fract(p);\n"
		"\tfloat min_distance = 8.0;\n"
		"\tfor (int z = -1; z <= 1; ++z) {\n"
		"\t\tfor (int y = -1; y <= 1; ++y) {\n"
		"\t\t\tfor (int x = -1; x <= 1; ++x) {\n"
		"\t\t\t\tvec3 neighbor = vec3(float(x), float(y), float(z));\n"
		"\t\t\t\tvec3 point = neighbor + 0.5 + 0.45 * sdf_hash33(cell + neighbor);\n"
		"\t\t\t\tmin_distance = min(min_distance, length(local - point));\n"
		"\t\t\t}\n"
		"\t\t}\n"
		"\t}\n"
		"\treturn 0.5 - min_distance;\n"
		"}\n\n");
}

static void se_sdf_gen_shading_helpers(c8* out, const sz capacity) {
	se_sdf_append(
		out,
		capacity,
		"struct sdf_shading_data {\n"
		"\tvec3 ambient;\n"
		"\tvec3 diffuse;\n"
		"\tvec3 specular;\n"
		"\tfloat roughness;\n"
		"\tfloat bias;\n"
		"\tfloat smoothness;\n"
		"};\n\n"
		"struct sdf_shadow_data {\n"
		"\tfloat softness;\n"
		"\tfloat bias;\n"
		"\tfloat samples;\n"
		"};\n\n"
		"struct sdf_surface_data {\n"
		"\tfloat distance;\n"
		"\tsdf_shading_data shading;\n"
		"\tsdf_shadow_data shadow;\n"
		"};\n\n"
		"sdf_shading_data sdf_make_shading(vec3 ambient, vec3 diffuse, vec3 specular, float roughness, float bias, float smoothness) {\n"
		"\tsdf_shading_data shading;\n"
		"\tshading.ambient = ambient;\n"
		"\tshading.diffuse = diffuse;\n"
		"\tshading.specular = specular;\n"
		"\tshading.roughness = clamp(roughness, 0.0, 1.0);\n"
		"\tshading.bias = clamp(bias, 0.0, 1.0);\n"
		"\tshading.smoothness = max(smoothness, 0.0);\n"
		"\treturn shading;\n"
		"}\n\n"
		"sdf_shadow_data sdf_make_shadow(float softness, float bias, int samples) {\n"
		"\tsdf_shadow_data shadow;\n"
		"\tshadow.softness = max(softness, 0.0);\n"
		"\tshadow.bias = max(bias, 0.0);\n"
		"\tshadow.samples = clamp(float(max(samples, 1)), 1.0, 128.0);\n"
		"\treturn shadow;\n"
		"}\n\n"
		"sdf_shading_data sdf_mix_shading(sdf_shading_data a, sdf_shading_data b, float t) {\n"
		"\tsdf_shading_data shading;\n"
		"\tshading.ambient = mix(a.ambient, b.ambient, t);\n"
		"\tshading.diffuse = mix(a.diffuse, b.diffuse, t);\n"
		"\tshading.specular = mix(a.specular, b.specular, t);\n"
		"\tshading.roughness = mix(a.roughness, b.roughness, t);\n"
		"\tshading.bias = mix(a.bias, b.bias, t);\n"
		"\tshading.smoothness = mix(a.smoothness, b.smoothness, t);\n"
		"\treturn shading;\n"
		"}\n\n"
		"sdf_shadow_data sdf_mix_shadow(sdf_shadow_data a, sdf_shadow_data b, float t) {\n"
		"\tsdf_shadow_data shadow;\n"
		"\tshadow.softness = mix(a.softness, b.softness, t);\n"
		"\tshadow.bias = mix(a.bias, b.bias, t);\n"
		"\tshadow.samples = mix(a.samples, b.samples, t);\n"
		"\treturn shadow;\n"
		"}\n\n"
		"float sdf_shading_band(float value, float bias, float smoothness) {\n"
		"\tfloat center = clamp(bias, 0.0, 1.0);\n"
		"\tfloat half_width = max(smoothness * 0.5, 0.0001);\n"
		"\tfloat band_min = clamp(center - half_width, 0.0, 1.0);\n"
		"\tfloat band_max = clamp(center + half_width, 0.0, 1.0);\n"
		"\tfloat band_value = clamp(value, 0.0, 1.0);\n"
		"\tif (band_max <= band_min + 0.0001) {\n"
		"\t\treturn band_value >= center ? 1.0 : 0.0;\n"
		"\t}\n"
		"\treturn smoothstep(band_min, band_max, band_value);\n"
		"}\n\n");
}

static void se_sdf_gen_lod_helpers(c8* out, const sz capacity) {
	se_sdf_append(
		out,
		capacity,
		"struct sdf_lod_data {\n"
		"\tint band;\n"
		"\tfloat max_distance;\n"
		"\tint steps;\n"
		"\tint noise_enabled;\n"
		"\tint point_lights_enabled;\n"
		"\tfloat hit_epsilon;\n"
		"\tfloat normal_epsilon;\n"
		"};\n\n"
		"sdf_lod_data sdf_make_lod(int band, float max_distance, int steps, int noise_enabled, int point_lights_enabled, float hit_epsilon, float normal_epsilon) {\n"
		"\tsdf_lod_data lod;\n"
		"\tlod.band = band;\n"
		"\tlod.max_distance = max(max_distance, 0.0);\n"
		"\tlod.steps = clamp(steps, 1, 128);\n"
		"\tlod.noise_enabled = noise_enabled != 0 ? 1 : 0;\n"
		"\tlod.point_lights_enabled = point_lights_enabled != 0 ? 1 : 0;\n"
		"\tlod.hit_epsilon = max(hit_epsilon, 0.0005);\n"
		"\tlod.normal_epsilon = max(normal_epsilon, lod.hit_epsilon);\n"
		"\treturn lod;\n"
		"}\n\n"
		"int sdf_lod_shadow_samples(sdf_lod_data lod, int requested_samples) {\n"
		"\tint sample_cap = max(lod.steps / 2, 1);\n"
		"\treturn clamp(min(requested_samples, sample_cap), 1, 128);\n"
		"}\n\n"
		"sdf_lod_data sdf_select_lod(float distance_from_camera) {\n"
		"\tif (distance_from_camera <= u_sdf_lod_high_distance) {\n"
		"\t\treturn sdf_make_lod(0, u_sdf_lod_high_distance, u_sdf_lod_high_steps, u_sdf_lod_high_noise, u_sdf_lod_high_point_lights, %.4ff, %.4ff);\n"
		"\t}\n"
		"\tif (distance_from_camera <= u_sdf_lod_medium_distance) {\n"
		"\t\treturn sdf_make_lod(1, u_sdf_lod_medium_distance, u_sdf_lod_medium_steps, u_sdf_lod_medium_noise, u_sdf_lod_medium_point_lights, %.4ff, %.4ff);\n"
		"\t}\n"
		"\treturn sdf_make_lod(2, u_sdf_lod_low_distance, u_sdf_lod_low_steps, u_sdf_lod_low_noise, u_sdf_lod_low_point_lights, %.4ff, %.4ff);\n"
		"}\n\n",
		SE_SDF_LOD_HIGH_HIT_EPSILON,
		SE_SDF_LOD_HIGH_HIT_EPSILON,
		SE_SDF_LOD_MEDIUM_HIT_EPSILON,
		SE_SDF_LOD_MEDIUM_HIT_EPSILON,
		SE_SDF_LOD_LOW_HIT_EPSILON,
		SE_SDF_LOD_LOW_HIT_EPSILON);
}

static void se_sdf_gen_light_functions(c8* out, const sz capacity) {
	se_sdf_append(
		out,
		capacity,
		"float sdf_shadow_visibility(vec3 ray_origin, vec3 ray_direction, float min_distance, float max_distance, float shadow_softness, int shadow_samples, sdf_lod_data lod) {\n"
		"\tfloat visibility = 1.0;\n"
		"\tfloat travel = min_distance;\n"
		"\tfloat safe_shadow_softness = max(shadow_softness, 0.0001);\n"
		"\tint sample_count = sdf_lod_shadow_samples(lod, shadow_samples);\n"
		"\tfor (int i = 0; i < 128; ++i) {\n"
		"\t\tif (i >= sample_count || travel >= max_distance) {\n"
		"\t\t\tbreak;\n"
		"\t\t}\n"
		"\t\tfloat distance_to_surface = scene_sdf(ray_origin + ray_direction * travel, lod);\n"
		"\t\tif (distance_to_surface < lod.hit_epsilon) {\n"
		"\t\t\treturn 0.0;\n"
		"\t\t}\n"
		"\t\tvisibility = min(visibility, safe_shadow_softness * distance_to_surface / max(travel, 0.0001));\n"
		"\t\ttravel += clamp(distance_to_surface, 0.01, 0.35);\n"
		"\t}\n"
		"\treturn clamp(visibility, 0.0, 1.0);\n"
		"}\n\n"
		"void sdf_apply_directional_light(\n"
		"\tvec3 normal,\n"
		"\tvec3 view_direction,\n"
		"\tvec3 world_position,\n"
		"\tvec3 light_direction,\n"
		"\tvec3 light_color,\n"
		"\tfloat roughness,\n"
		"\tfloat shading_bias,\n"
		"\tfloat shading_smoothness,\n"
		"\tfloat shadow_softness,\n"
		"\tfloat shadow_bias,\n"
		"\tint shadow_samples,\n"
		"\tsdf_lod_data lod,\n"
		"\tout vec3 diffuse_light,\n"
		"\tout vec3 specular_light) {\n"
		"\tvec3 dir = normalize(light_direction);\n"
		"\tfloat diffuse = max(dot(normal, dir), 0.0);\n"
		"\tif (diffuse <= 0.0) {\n"
		"\t\tdiffuse_light = vec3(0.0);\n"
		"\t\tspecular_light = vec3(0.0);\n"
		"\t\treturn;\n"
		"\t}\n"
		"\tfloat diffuse_band = sdf_shading_band(diffuse, shading_bias, shading_smoothness);\n"
		"\tfloat shadow_visibility = sdf_shadow_visibility(world_position + normal * max(shadow_bias, 0.0005), dir, max(shadow_bias * 2.0, lod.hit_epsilon), 32.0, shadow_softness, shadow_samples, lod);\n"
		"\tvec3 half_dir = normalize(dir + view_direction);\n"
		"\tfloat shininess = mix(96.0, 8.0, clamp(roughness, 0.0, 1.0));\n"
		"\tfloat specular = pow(max(dot(normal, half_dir), 0.0), shininess) * mix(1.0, 0.18, clamp(roughness, 0.0, 1.0));\n"
		"\tfloat specular_band = sdf_shading_band(specular, shading_bias, shading_smoothness);\n"
		"\tdiffuse_light = light_color * diffuse * diffuse_band * shadow_visibility * 0.78;\n"
		"\tspecular_light = light_color * specular * specular_band * shadow_visibility * 0.35;\n"
		"}\n\n"
		"void sdf_apply_point_light(\n"
		"\tvec3 normal,\n"
		"\tvec3 view_direction,\n"
		"\tvec3 world_position,\n"
		"\tvec3 light_position,\n"
		"\tvec3 light_color,\n"
		"\tfloat radius,\n"
		"\tfloat roughness,\n"
		"\tfloat shading_bias,\n"
		"\tfloat shading_smoothness,\n"
		"\tfloat shadow_softness,\n"
		"\tfloat shadow_bias,\n"
		"\tint shadow_samples,\n"
		"\tsdf_lod_data lod,\n"
		"\tout vec3 diffuse_light,\n"
		"\tout vec3 specular_light) {\n"
		"\tvec3 to_light = light_position - world_position;\n"
		"\tfloat distance_to_light = length(to_light);\n"
		"\tif (distance_to_light <= 0.0001) {\n"
		"\t\tdiffuse_light = vec3(0.0);\n"
		"\t\tspecular_light = vec3(0.0);\n"
		"\t\treturn;\n"
		"\t}\n"
		"\tvec3 dir = to_light / distance_to_light;\n"
		"\tfloat diffuse = max(dot(normal, dir), 0.0);\n"
		"\tfloat safe_radius = max(radius, 0.0001);\n"
		"\tfloat attenuation = clamp(1.0 - distance_to_light / safe_radius, 0.0, 1.0);\n"
		"\tattenuation *= attenuation;\n"
		"\tif (diffuse <= 0.0 || attenuation <= 0.0) {\n"
		"\t\tdiffuse_light = vec3(0.0);\n"
		"\t\tspecular_light = vec3(0.0);\n"
		"\t\treturn;\n"
		"\t}\n"
		"\tfloat diffuse_band = sdf_shading_band(diffuse, shading_bias, shading_smoothness);\n"
		"\tfloat shadow_visibility = sdf_shadow_visibility(world_position + normal * max(shadow_bias, 0.0005), dir, max(shadow_bias * 2.0, lod.hit_epsilon), max(distance_to_light - shadow_bias, 0.01), shadow_softness, shadow_samples, lod);\n"
		"\tvec3 half_dir = normalize(dir + view_direction);\n"
		"\tfloat shininess = mix(96.0, 8.0, clamp(roughness, 0.0, 1.0));\n"
		"\tfloat specular = pow(max(dot(normal, half_dir), 0.0), shininess) * mix(1.0, 0.18, clamp(roughness, 0.0, 1.0));\n"
		"\tfloat specular_band = sdf_shading_band(specular, shading_bias, shading_smoothness);\n"
		"\tdiffuse_light = light_color * diffuse * diffuse_band * attenuation * shadow_visibility * 0.78;\n"
		"\tspecular_light = light_color * specular * specular_band * attenuation * shadow_visibility * 0.35;\n"
		"}\n\n");
}

static void se_sdf_gen_uniform_recursive(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	se_sdf_noise_handle* noise_handle_ptr = NULL;
	se_sdf_point_light_handle* point_light_handle_ptr = NULL;
	se_sdf_directional_light_handle* directional_light_handle_ptr = NULL;
	if (!sdf_ptr) {
		return;
	}
	se_sdf_append(out, capacity, "uniform float _" SE_SDF_HANDLE_FMT "_operation_amount;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform vec3 _" SE_SDF_HANDLE_FMT "_shading_ambient;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform vec3 _" SE_SDF_HANDLE_FMT "_shading_diffuse;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform vec3 _" SE_SDF_HANDLE_FMT "_shading_specular;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform float _" SE_SDF_HANDLE_FMT "_shading_roughness;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform float _" SE_SDF_HANDLE_FMT "_shading_bias;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform float _" SE_SDF_HANDLE_FMT "_shading_smoothness;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform float _" SE_SDF_HANDLE_FMT "_shadow_softness;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform float _" SE_SDF_HANDLE_FMT "_shadow_bias;\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "uniform int _" SE_SDF_HANDLE_FMT "_shadow_samples;\n", SE_SDF_HANDLE_ARG(sdf));
	if (se_sdf_needs_inverse_transform(sdf_ptr)) {
		se_sdf_append(out, capacity, "uniform mat4 _" SE_SDF_HANDLE_FMT "_inv_transform;\n", SE_SDF_HANDLE_ARG(sdf));
	}
	if (sdf_ptr->type == SE_SDF_SPHERE) {
		se_sdf_append(out, capacity, "uniform float _" SE_SDF_HANDLE_FMT "_radius;\n", SE_SDF_HANDLE_ARG(sdf));
	} else if (sdf_ptr->type == SE_SDF_BOX) {
		se_sdf_append(out, capacity, "uniform vec3 _" SE_SDF_HANDLE_FMT "_size;\n", SE_SDF_HANDLE_ARG(sdf));
	}
	s_foreach(&sdf_ptr->noises, noise_handle_ptr) {
		const se_sdf_noise_handle noise_id = *noise_handle_ptr;
		const se_sdf_noise* noise = se_sdf_noise_from_handle(ctx, noise_id);
		if (!se_sdf_noise_is_active(noise)) {
			continue;
		}
		se_sdf_append(
			out,
			capacity,
			"uniform float _noise_" SE_SDF_HANDLE_FMT "_frequency;\n",
			SE_SDF_HANDLE_ARG(noise_id));
		se_sdf_append(
			out,
			capacity,
			"uniform vec3 _noise_" SE_SDF_HANDLE_FMT "_offset;\n",
			SE_SDF_HANDLE_ARG(noise_id));
	}
	s_foreach(&sdf_ptr->point_lights, point_light_handle_ptr) {
		const se_sdf_point_light_handle point_light_id = *point_light_handle_ptr;
		if (!se_sdf_point_light_from_handle(ctx, point_light_id)) {
			continue;
		}
		se_sdf_append(
			out,
			capacity,
			"uniform vec3 _point_light_" SE_SDF_HANDLE_FMT "_position;\n",
			SE_SDF_HANDLE_ARG(point_light_id));
		se_sdf_append(
			out,
			capacity,
			"uniform vec3 _point_light_" SE_SDF_HANDLE_FMT "_color;\n",
			SE_SDF_HANDLE_ARG(point_light_id));
		se_sdf_append(
			out,
			capacity,
			"uniform float _point_light_" SE_SDF_HANDLE_FMT "_radius;\n",
			SE_SDF_HANDLE_ARG(point_light_id));
	}
	s_foreach(&sdf_ptr->directional_lights, directional_light_handle_ptr) {
		const se_sdf_directional_light_handle directional_light_id = *directional_light_handle_ptr;
		if (!se_sdf_directional_light_from_handle(ctx, directional_light_id)) {
			continue;
		}
		se_sdf_append(
			out,
			capacity,
			"uniform vec3 _directional_light_" SE_SDF_HANDLE_FMT "_direction;\n",
			SE_SDF_HANDLE_ARG(directional_light_id));
		se_sdf_append(
			out,
			capacity,
			"uniform vec3 _directional_light_" SE_SDF_HANDLE_FMT "_color;\n",
			SE_SDF_HANDLE_ARG(directional_light_id));
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_gen_uniform_recursive(out, capacity, *child);
	}
}

static void se_sdf_gen_function_recursive(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	se_sdf_noise_handle* noise_handle_ptr = NULL;
	const c8* operation_function = NULL;
	if (!sdf_ptr) {
		return;
	}
	operation_function = se_sdf_get_operation_function_name(sdf_ptr->operation);
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_gen_function_recursive(out, capacity, *child);
	}
	if (sdf_ptr->type == SE_SDF_SPHERE) {
		se_sdf_append(
			out,
			capacity,
			"float sdf_" SE_SDF_HANDLE_FMT "_primitive(vec3 p) {\n"
			"\tvec3 local = (_" SE_SDF_HANDLE_FMT "_inv_transform * vec4(p, 1.0)).xyz;\n"
			"\treturn length(local) - _" SE_SDF_HANDLE_FMT "_radius;\n"
			"}\n\n",
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf));
	} else if (sdf_ptr->type == SE_SDF_BOX) {
		se_sdf_append(
			out,
			capacity,
			"float sdf_" SE_SDF_HANDLE_FMT "_primitive(vec3 p) {\n"
			"\tvec3 local = (_" SE_SDF_HANDLE_FMT "_inv_transform * vec4(p, 1.0)).xyz;\n"
			"\tvec3 q = abs(local) - _" SE_SDF_HANDLE_FMT "_size;\n"
			"\treturn length(max(q, vec3(0.0))) + min(max(q.x, max(q.y, q.z)), 0.0);\n"
			"}\n\n",
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf));
	}
	se_sdf_append(out, capacity, "float sdf_" SE_SDF_HANDLE_FMT "(vec3 p, bool noise_enabled) {\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "\tfloat d = %.1f;\n", SE_SDF_FAR_DISTANCE);
	if (se_sdf_has_primitive(sdf_ptr)) {
		se_sdf_append(
			out,
			capacity,
			"\td = %s(d, sdf_" SE_SDF_HANDLE_FMT "_primitive(p), _" SE_SDF_HANDLE_FMT "_operation_amount);\n",
			operation_function,
			SE_SDF_HANDLE_ARG(sdf),
			SE_SDF_HANDLE_ARG(sdf));
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_append(
			out,
			capacity,
			"\td = %s(d, sdf_" SE_SDF_HANDLE_FMT "(p, noise_enabled), _" SE_SDF_HANDLE_FMT "_operation_amount);\n",
			operation_function,
			SE_SDF_HANDLE_ARG(*child),
			SE_SDF_HANDLE_ARG(sdf));
	}
	if (se_sdf_has_noise(sdf_ptr)) {
		se_sdf_append(
			out,
			capacity,
			"\tif (noise_enabled) {\n"
			"\t\tvec3 noise_local = (_" SE_SDF_HANDLE_FMT "_inv_transform * vec4(p, 1.0)).xyz;\n",
			SE_SDF_HANDLE_ARG(sdf));
		s_foreach(&sdf_ptr->noises, noise_handle_ptr) {
			const c8* noise_function = NULL;
			const se_sdf_noise_handle noise_id = *noise_handle_ptr;
			const se_sdf_noise* noise = se_sdf_noise_from_handle(ctx, noise_id);
			if (!noise) {
				continue;
			}
			noise_function = se_sdf_get_noise_function_name(noise->type);
			if (!noise_function) {
				continue;
			}
			se_sdf_append(
				out,
				capacity,
				"\t\td += %s(noise_local * _noise_" SE_SDF_HANDLE_FMT "_frequency + _noise_" SE_SDF_HANDLE_FMT "_offset) * %.3ff;\n",
				noise_function,
				SE_SDF_HANDLE_ARG(noise_id),
				SE_SDF_HANDLE_ARG(noise_id),
				SE_SDF_DEFAULT_NOISE_AMOUNT);
		}
		se_sdf_append(out, capacity, "\t}\n");
	}
	se_sdf_append(out, capacity, "\treturn d;\n}\n\n");
}

static void se_sdf_gen_surface_recursive(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	se_sdf_noise_handle* noise_handle_ptr = NULL;
	if (!sdf_ptr) {
		return;
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_gen_surface_recursive(out, capacity, *child);
	}
	se_sdf_append(
		out,
		capacity,
		"sdf_surface_data sdf_" SE_SDF_HANDLE_FMT "_surface(vec3 p, bool noise_enabled) {\n"
		"\tsdf_surface_data surface;\n"
		"\tsurface.distance = %.1f;\n"
		"\tsurface.shading = sdf_make_shading(\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shading_ambient,\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shading_diffuse,\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shading_specular,\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shading_roughness,\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shading_bias,\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shading_smoothness);\n"
		"\tsurface.shadow = sdf_make_shadow(\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shadow_softness,\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shadow_bias,\n"
		"\t\t_" SE_SDF_HANDLE_FMT "_shadow_samples);\n"
		"\tbool has_surface = false;\n",
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_FAR_DISTANCE,
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_HANDLE_ARG(sdf),
		SE_SDF_HANDLE_ARG(sdf));
	if (se_sdf_has_primitive(sdf_ptr)) {
		se_sdf_append(
			out,
			capacity,
			"\tsurface.distance = sdf_" SE_SDF_HANDLE_FMT "_primitive(p);\n"
			"\thas_surface = true;\n",
			SE_SDF_HANDLE_ARG(sdf));
	}
	s_foreach(&sdf_ptr->children, child) {
		switch (sdf_ptr->operation) {
			case SE_SDF_SMOOTH_UNION:
				se_sdf_append(
					out,
					capacity,
					"\t{\n"
					"\t\tsdf_surface_data child_surface = sdf_" SE_SDF_HANDLE_FMT "_surface(p, noise_enabled);\n"
					"\t\tif (!has_surface) {\n"
					"\t\t\tsurface = child_surface;\n"
					"\t\t\thas_surface = true;\n"
					"\t\t} else {\n"
					"\t\t\tfloat blend = sdf_op_smooth_union_factor(surface.distance, child_surface.distance, _" SE_SDF_HANDLE_FMT "_operation_amount);\n"
					"\t\t\tsurface.shading = sdf_mix_shading(child_surface.shading, surface.shading, blend);\n"
					"\t\t\tsurface.shadow = sdf_mix_shadow(child_surface.shadow, surface.shadow, blend);\n"
					"\t\t\tsurface.distance = sdf_op_smooth_union(surface.distance, child_surface.distance, _" SE_SDF_HANDLE_FMT "_operation_amount);\n"
					"\t\t}\n"
					"\t}\n",
					SE_SDF_HANDLE_ARG(*child),
					SE_SDF_HANDLE_ARG(sdf),
					SE_SDF_HANDLE_ARG(sdf));
				break;
			case SE_SDF_UNION:
			default:
				se_sdf_append(
					out,
					capacity,
					"\t{\n"
					"\t\tsdf_surface_data child_surface = sdf_" SE_SDF_HANDLE_FMT "_surface(p, noise_enabled);\n"
					"\t\tif (!has_surface || child_surface.distance < surface.distance) {\n"
					"\t\t\tsurface.shading = child_surface.shading;\n"
					"\t\t\tsurface.shadow = child_surface.shadow;\n"
					"\t\t}\n"
					"\t\tsurface.distance = has_surface ? sdf_op_union(surface.distance, child_surface.distance, _" SE_SDF_HANDLE_FMT "_operation_amount) : child_surface.distance;\n"
					"\t\thas_surface = true;\n"
					"\t}\n",
					SE_SDF_HANDLE_ARG(*child),
					SE_SDF_HANDLE_ARG(sdf));
				break;
		}
	}
	if (se_sdf_has_noise(sdf_ptr)) {
		se_sdf_append(
			out,
			capacity,
			"\tif (noise_enabled) {\n"
			"\t\tvec3 noise_local = (_" SE_SDF_HANDLE_FMT "_inv_transform * vec4(p, 1.0)).xyz;\n",
			SE_SDF_HANDLE_ARG(sdf));
		s_foreach(&sdf_ptr->noises, noise_handle_ptr) {
			const c8* noise_function = NULL;
			const se_sdf_noise_handle noise_id = *noise_handle_ptr;
			const se_sdf_noise* noise = se_sdf_noise_from_handle(ctx, noise_id);
			if (!noise) {
				continue;
			}
			noise_function = se_sdf_get_noise_function_name(noise->type);
			if (!noise_function) {
				continue;
			}
			se_sdf_append(
				out,
				capacity,
				"\t\tsurface.distance += %s(noise_local * _noise_" SE_SDF_HANDLE_FMT "_frequency + _noise_" SE_SDF_HANDLE_FMT "_offset) * %.3ff;\n",
				noise_function,
				SE_SDF_HANDLE_ARG(noise_id),
				SE_SDF_HANDLE_ARG(noise_id),
				SE_SDF_DEFAULT_NOISE_AMOUNT);
		}
		se_sdf_append(out, capacity, "\t}\n");
	}
	se_sdf_append(out, capacity, "\treturn surface;\n}\n\n");
}

static void se_sdf_gen_light_apply_recursive(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	se_sdf_point_light_handle* point_light_handle_ptr = NULL;
	se_sdf_directional_light_handle* directional_light_handle_ptr = NULL;
	if (!sdf_ptr) {
		return;
	}
	s_foreach(&sdf_ptr->directional_lights, directional_light_handle_ptr) {
		const se_sdf_directional_light_handle directional_light_id = *directional_light_handle_ptr;
		if (!se_sdf_directional_light_from_handle(ctx, directional_light_id)) {
			continue;
		}
		se_sdf_append(
			out,
			capacity,
			"\t{\n"
			"\t\tvec3 light_diffuse = vec3(0.0);\n"
			"\t\tvec3 light_specular = vec3(0.0);\n"
			"\t\tsdf_apply_directional_light(normal, view_direction, hit_position, _directional_light_" SE_SDF_HANDLE_FMT "_direction, _directional_light_" SE_SDF_HANDLE_FMT "_color, shading.roughness, shading.bias, shading.smoothness, shadow.softness, shadow.bias, int(max(round(shadow.samples), 1.0)), lod, light_diffuse, light_specular);\n"
			"\t\tdiffuse_lighting += light_diffuse;\n"
			"\t\tspecular_lighting += light_specular;\n"
			"\t}\n",
			SE_SDF_HANDLE_ARG(directional_light_id),
			SE_SDF_HANDLE_ARG(directional_light_id));
	}
	s_foreach(&sdf_ptr->point_lights, point_light_handle_ptr) {
		const se_sdf_point_light_handle point_light_id = *point_light_handle_ptr;
		if (!se_sdf_point_light_from_handle(ctx, point_light_id)) {
			continue;
		}
		se_sdf_append(
			out,
			capacity,
			"\tif (lod.point_lights_enabled != 0) {\n"
			"\t\tvec3 light_diffuse = vec3(0.0);\n"
			"\t\tvec3 light_specular = vec3(0.0);\n"
			"\t\tsdf_apply_point_light(normal, view_direction, hit_position, _point_light_" SE_SDF_HANDLE_FMT "_position, _point_light_" SE_SDF_HANDLE_FMT "_color, _point_light_" SE_SDF_HANDLE_FMT "_radius, shading.roughness, shading.bias, shading.smoothness, shadow.softness, shadow.bias, int(max(round(shadow.samples), 1.0)), lod, light_diffuse, light_specular);\n"
			"\t\tdiffuse_lighting += light_diffuse;\n"
			"\t\tspecular_lighting += light_specular;\n"
			"\t}\n",
			SE_SDF_HANDLE_ARG(point_light_id),
			SE_SDF_HANDLE_ARG(point_light_id),
			SE_SDF_HANDLE_ARG(point_light_id));
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_gen_light_apply_recursive(out, capacity, *child);
	}
}

void se_sdf_gen_uniform(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_sdf_append(out, capacity, "uniform mat4 u_inv_view_projection;\n");
	se_sdf_append(out, capacity, "uniform vec3 u_camera_position;\n");
	se_sdf_append(out, capacity, "uniform int u_use_orthographic;\n");
	se_sdf_append(out, capacity, "uniform float u_sdf_lod_high_distance;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_high_steps;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_high_noise;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_high_point_lights;\n");
	se_sdf_append(out, capacity, "uniform float u_sdf_lod_medium_distance;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_medium_steps;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_medium_noise;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_medium_point_lights;\n");
	se_sdf_append(out, capacity, "uniform float u_sdf_lod_low_distance;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_low_steps;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_low_noise;\n");
	se_sdf_append(out, capacity, "uniform int u_sdf_lod_low_point_lights;\n");
	se_sdf_gen_uniform_recursive(out, capacity, sdf);
}

void se_sdf_gen_function(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_sdf_gen_function_recursive(out, capacity, sdf);
}

void se_sdf_gen_fragment(c8* out, const sz capacity, const se_sdf_handle sdf) {
	if (!out || capacity == 0) {
		return;
	}
	out[0] = '\0';
	se_sdf_append(out, capacity, "#version 330 core\n");
	se_sdf_append(out, capacity, "in vec2 v_uv;\n");
	se_sdf_append(out, capacity, "out vec4 frag_color;\n\n");
	se_sdf_gen_uniform(out, capacity, sdf);
	se_sdf_append(out, capacity, "\n");
	se_sdf_gen_operator_functions(out, capacity);
	se_sdf_gen_noise_functions(out, capacity);
	se_sdf_gen_shading_helpers(out, capacity);
	se_sdf_gen_lod_helpers(out, capacity);
	se_sdf_gen_function(out, capacity, sdf);
	se_sdf_gen_surface_recursive(out, capacity, sdf);
	se_sdf_append(out, capacity, "float scene_sdf(vec3 p, sdf_lod_data lod) {\n");
	se_sdf_append(out, capacity, "\treturn sdf_" SE_SDF_HANDLE_FMT "(p, lod.noise_enabled != 0);\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "}\n\n");
	se_sdf_append(out, capacity, "sdf_surface_data scene_surface(vec3 p, sdf_lod_data lod) {\n");
	se_sdf_append(out, capacity, "\treturn sdf_" SE_SDF_HANDLE_FMT "_surface(p, lod.noise_enabled != 0);\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "}\n\n");
	se_sdf_append(
		out,
		capacity,
		"vec3 sdf_estimate_normal(vec3 p, sdf_lod_data lod) {\n"
		"\tfloat e = lod.normal_epsilon;\n"
		"\tfloat dx = scene_sdf(p + vec3(e, 0.0, 0.0), lod) - scene_sdf(p - vec3(e, 0.0, 0.0), lod);\n"
		"\tfloat dy = scene_sdf(p + vec3(0.0, e, 0.0), lod) - scene_sdf(p - vec3(0.0, e, 0.0), lod);\n"
		"\tfloat dz = scene_sdf(p + vec3(0.0, 0.0, e), lod) - scene_sdf(p - vec3(0.0, 0.0, e), lod);\n"
		"\treturn normalize(vec3(dx, dy, dz));\n"
		"}\n\n");
	se_sdf_gen_light_functions(out, capacity);
	se_sdf_append(
		out,
		capacity,
		"void main() {\n"
		"\tvec2 ndc = vec2(v_uv.x * 2.0 - 1.0, 1.0 - v_uv.y * 2.0);\n"
		"\tvec4 near_clip = vec4(ndc, -1.0, 1.0);\n"
		"\tvec4 far_clip = vec4(ndc, 1.0, 1.0);\n"
		"\tvec4 near_world_h = u_inv_view_projection * near_clip;\n"
		"\tvec4 far_world_h = u_inv_view_projection * far_clip;\n"
		"\tvec3 near_world = near_world_h.xyz / max(near_world_h.w, 0.00001);\n"
		"\tvec3 far_world = far_world_h.xyz / max(far_world_h.w, 0.00001);\n"
		"\tvec3 ray_origin = near_world;\n"
		"\tvec3 ray_dir = normalize(far_world - near_world);\n"
		"\tif (u_use_orthographic == 0) {\n"
		"\t\tray_origin = u_camera_position;\n"
		"\t\tray_dir = normalize(far_world - u_camera_position);\n"
		"\t}\n"
		"\tfloat travel = 0.0;\n"
		"\tvec3 hit_position = ray_origin;\n"
		"\tbool hit = false;\n"
		"\tsdf_lod_data lod = sdf_select_lod(0.0);\n"
		"\tfor (int i = 0; i < %u; ++i) {\n"
		"\t\tlod = sdf_select_lod(travel);\n"
		"\t\tif (i >= lod.steps || travel > lod.max_distance) {\n"
		"\t\t\tbreak;\n"
		"\t\t}\n"
		"\t\thit_position = ray_origin + ray_dir * travel;\n"
		"\t\tfloat distance_to_surface = scene_sdf(hit_position, lod);\n"
		"\t\tif (distance_to_surface < lod.hit_epsilon) {\n"
		"\t\t\thit = true;\n"
		"\t\t\tbreak;\n"
		"\t\t}\n"
		"\t\ttravel += distance_to_surface;\n"
		"\t\tif (travel > %.1f) {\n"
		"\t\t\tbreak;\n"
		"\t\t}\n"
		"\t}\n"
		"\tvec3 sky = mix(vec3(0.05, 0.06, 0.08), vec3(0.16, 0.19, 0.24), clamp(ray_dir.y * 0.5 + 0.5, 0.0, 1.0));\n"
		"\tif (!hit) {\n"
		"\t\tfrag_color = vec4(sky, 1.0);\n"
		"\t\treturn;\n"
		"\t}\n"
		"\tlod = sdf_select_lod(travel);\n"
		"\tvec3 normal = sdf_estimate_normal(hit_position, lod);\n"
		"\tsdf_surface_data surface = scene_surface(hit_position, lod);\n"
		"\tsdf_shading_data shading = surface.shading;\n"
		"\tsdf_shadow_data shadow = surface.shadow;\n"
		"\tvec3 view_direction = normalize(u_use_orthographic != 0 ? -ray_dir : (u_camera_position - hit_position));\n"
		"\tvec3 diffuse_lighting = vec3(0.0);\n"
		"\tvec3 specular_lighting = vec3(0.0);\n",
		SE_SDF_MAX_TRACE_STEPS,
		SE_SDF_FAR_DISTANCE);
	if (se_sdf_has_lights_recursive(sdf)) {
		se_sdf_gen_light_apply_recursive(out, capacity, sdf);
	} else {
		se_sdf_append(
			out,
			capacity,
			"\t{\n"
			"\t\tvec3 light_diffuse = vec3(0.0);\n"
			"\t\tvec3 light_specular = vec3(0.0);\n"
			"\t\tsdf_apply_directional_light(normal, view_direction, hit_position, vec3(%.2f, %.2f, %.2f), vec3(1.0), shading.roughness, shading.bias, shading.smoothness, shadow.softness, shadow.bias, int(max(round(shadow.samples), 1.0)), lod, light_diffuse, light_specular);\n"
			"\t\tdiffuse_lighting += light_diffuse;\n"
			"\t\tspecular_lighting += light_specular;\n"
			"\t}\n",
			SE_SDF_DEFAULT_LIGHT_DIR_X,
			SE_SDF_DEFAULT_LIGHT_DIR_Y,
			SE_SDF_DEFAULT_LIGHT_DIR_Z);
	}
	se_sdf_append(
		out,
		capacity,
		"\tvec3 color = shading.ambient + shading.diffuse * diffuse_lighting + shading.specular * specular_lighting;\n"
		"\tfloat fog = clamp(travel / 48.0, 0.0, 1.0);\n"
		"\tfrag_color = vec4(mix(color, sky, fog), 1.0);\n"
		"}\n");
}

void se_sdf_gen_vertex(c8* out, const sz capacity) {
	if (!out || capacity == 0) {
		return;
	}
	out[0] = '\0';
	se_sdf_append(
		out,
		capacity,
		"#version 330 core\n"
		"layout(location = 0) in vec2 in_position;\n"
		"layout(location = 1) in vec2 in_uv;\n"
		"out vec2 v_uv;\n\n"
		"void main() {\n"
		"\tv_uv = in_uv;\n"
		"\tgl_Position = vec4(in_position, 0.0, 1.0);\n"
		"}\n");
}

static void se_sdf_upload_lod_uniforms(const se_shader_handle shader, const se_sdf* sdf_ptr) {
	const se_sdf_lods lods = se_sdf_get_lods_defaulted(sdf_ptr);
	se_shader_set_float(shader, "u_sdf_lod_high_distance", lods.high.distance);
	se_shader_set_int(shader, "u_sdf_lod_high_steps", (i32)lods.high.steps);
	se_shader_set_int(shader, "u_sdf_lod_high_noise", lods.high.noise ? 1 : 0);
	se_shader_set_int(shader, "u_sdf_lod_high_point_lights", lods.high.point_lights ? 1 : 0);
	se_shader_set_float(shader, "u_sdf_lod_medium_distance", lods.medium.distance);
	se_shader_set_int(shader, "u_sdf_lod_medium_steps", (i32)lods.medium.steps);
	se_shader_set_int(shader, "u_sdf_lod_medium_noise", lods.medium.noise ? 1 : 0);
	se_shader_set_int(shader, "u_sdf_lod_medium_point_lights", lods.medium.point_lights ? 1 : 0);
	se_shader_set_float(shader, "u_sdf_lod_low_distance", lods.low.distance);
	se_shader_set_int(shader, "u_sdf_lod_low_steps", (i32)lods.low.steps);
	se_shader_set_int(shader, "u_sdf_lod_low_noise", lods.low.noise ? 1 : 0);
	se_shader_set_int(shader, "u_sdf_lod_low_point_lights", lods.low.point_lights ? 1 : 0);
}

static void se_sdf_upload_uniforms_recursive(const se_shader_handle shader, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	se_sdf_noise_handle* noise_handle_ptr = NULL;
	se_sdf_point_light_handle* point_light_handle_ptr = NULL;
	se_sdf_directional_light_handle* directional_light_handle_ptr = NULL;
	if (!sdf_ptr) {
		return;
	}
	c8 uniform_name[64] = {0};
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_operation_amount", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_float(shader, uniform_name, se_sdf_get_operation_amount(sdf_ptr));
	const se_sdf_shading shading = se_sdf_get_shading_defaulted(sdf_ptr);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shading_ambient", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_vec3(shader, uniform_name, &shading.ambient);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shading_diffuse", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_vec3(shader, uniform_name, &shading.diffuse);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shading_specular", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_vec3(shader, uniform_name, &shading.specular);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shading_roughness", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_float(shader, uniform_name, shading.roughness);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shading_bias", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_float(shader, uniform_name, shading.bias);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shading_smoothness", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_float(shader, uniform_name, shading.smoothness);
	const se_sdf_shadow shadow = se_sdf_get_shadow_defaulted(sdf_ptr);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shadow_softness", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_float(shader, uniform_name, shadow.softness);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shadow_bias", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_float(shader, uniform_name, shadow.bias);
	snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_shadow_samples", SE_SDF_HANDLE_ARG(sdf));
	se_shader_set_int(shader, uniform_name, (i32)shadow.samples);
	if (se_sdf_needs_inverse_transform(sdf_ptr)) {
		const s_mat4 inverse = s_mat4_inverse(&sdf_ptr->transform);
		snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_inv_transform", SE_SDF_HANDLE_ARG(sdf));
		se_shader_set_mat4(shader, uniform_name, &inverse);
	}
	if (sdf_ptr->type == SE_SDF_SPHERE || sdf_ptr->type == SE_SDF_BOX) {
		if (sdf_ptr->type == SE_SDF_SPHERE) {
			snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_radius", SE_SDF_HANDLE_ARG(sdf));
			se_shader_set_float(shader, uniform_name, sdf_ptr->sphere.radius);
		} else {
			snprintf(uniform_name, sizeof(uniform_name), "_" SE_SDF_HANDLE_FMT "_size", SE_SDF_HANDLE_ARG(sdf));
			se_shader_set_vec3(shader, uniform_name, &sdf_ptr->box.size);
		}
	}
	s_foreach(&sdf_ptr->noises, noise_handle_ptr) {
		const se_sdf_noise_handle noise_id = *noise_handle_ptr;
		const se_sdf_noise* noise = se_sdf_noise_from_handle(ctx, noise_id);
		if (!se_sdf_noise_is_active(noise)) {
			continue;
		}
		snprintf(uniform_name, sizeof(uniform_name), "_noise_" SE_SDF_HANDLE_FMT "_frequency", SE_SDF_HANDLE_ARG(noise_id));
		se_shader_set_float(shader, uniform_name, se_sdf_get_noise_frequency_defaulted(noise));
		snprintf(uniform_name, sizeof(uniform_name), "_noise_" SE_SDF_HANDLE_FMT "_offset", SE_SDF_HANDLE_ARG(noise_id));
		se_shader_set_vec3(shader, uniform_name, &noise->offset);
	}
	s_foreach(&sdf_ptr->point_lights, point_light_handle_ptr) {
		s_vec3 world_position = s_vec3(0.0f, 0.0f, 0.0f);
		const se_sdf_point_light_handle point_light_id = *point_light_handle_ptr;
		const se_sdf_point_light* point_light = se_sdf_point_light_from_handle(ctx, point_light_id);
		if (!point_light) {
			continue;
		}
		world_position = se_sdf_transform_point(&sdf_ptr->transform, &point_light->position);
		snprintf(uniform_name, sizeof(uniform_name), "_point_light_" SE_SDF_HANDLE_FMT "_position", SE_SDF_HANDLE_ARG(point_light_id));
		se_shader_set_vec3(shader, uniform_name, &world_position);
		snprintf(uniform_name, sizeof(uniform_name), "_point_light_" SE_SDF_HANDLE_FMT "_color", SE_SDF_HANDLE_ARG(point_light_id));
		se_shader_set_vec3(shader, uniform_name, &point_light->color);
		snprintf(uniform_name, sizeof(uniform_name), "_point_light_" SE_SDF_HANDLE_FMT "_radius", SE_SDF_HANDLE_ARG(point_light_id));
		se_shader_set_float(shader, uniform_name, point_light->radius);
	}
	s_foreach(&sdf_ptr->directional_lights, directional_light_handle_ptr) {
		s_vec3 world_direction = s_vec3(0.0f, 0.0f, 0.0f);
		const se_sdf_directional_light_handle directional_light_id = *directional_light_handle_ptr;
		const se_sdf_directional_light* directional_light = se_sdf_directional_light_from_handle(ctx, directional_light_id);
		if (!directional_light) {
			continue;
		}
		world_direction = se_sdf_transform_direction(&sdf_ptr->transform, &directional_light->direction);
		snprintf(uniform_name, sizeof(uniform_name), "_directional_light_" SE_SDF_HANDLE_FMT "_direction", SE_SDF_HANDLE_ARG(directional_light_id));
		se_shader_set_vec3(shader, uniform_name, &world_direction);
		snprintf(uniform_name, sizeof(uniform_name), "_directional_light_" SE_SDF_HANDLE_FMT "_color", SE_SDF_HANDLE_ARG(directional_light_id));
		se_shader_set_vec3(shader, uniform_name, &directional_light->color);
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_upload_uniforms_recursive(shader, *child);
	}
}

se_sdf_handle se_sdf_create_internal(const se_sdf* sdf) {
	se_context* ctx = se_current_context();
	se_sdf_handle new_sdf = S_HANDLE_NULL;
	se_sdf* sdf_ptr = NULL;
	if (!ctx) {
		return S_HANDLE_NULL;
	}
	new_sdf = s_array_increment(&ctx->sdfs);
	sdf_ptr = se_sdf_from_handle(ctx, new_sdf);
	if (!sdf_ptr) {
		return S_HANDLE_NULL;
	}
	memset(sdf_ptr, 0, sizeof(*sdf_ptr));
	if (sdf) {
		memcpy(sdf_ptr, sdf, sizeof(*sdf_ptr));
	}
	if (se_sdf_transform_is_zero(&sdf_ptr->transform)) {
		sdf_ptr->transform = s_mat4_identity;
	}
	sdf_ptr->parent = S_HANDLE_NULL;
	s_array_init(&sdf_ptr->children);
	s_array_init(&sdf_ptr->noises);
	s_array_init(&sdf_ptr->point_lights);
	s_array_init(&sdf_ptr->directional_lights);
	memset(&sdf_ptr->quad, 0, sizeof(sdf_ptr->quad));
	sdf_ptr->shader = S_HANDLE_NULL;
	sdf_ptr->output = S_HANDLE_NULL;
	sdf_ptr->volume = S_HANDLE_NULL;
	return new_sdf;
}

void se_sdf_destroy(se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf* parent_ptr = NULL;
	if (!ctx || !sdf_ptr) {
		return;
	}

	while (s_array_get_size(&sdf_ptr->children) > 0) {
		const s_handle local_child_handle = s_array_handle(&sdf_ptr->children, (u32)(s_array_get_size(&sdf_ptr->children) - 1u));
		se_sdf_handle* child_handle_ptr = s_array_get(&sdf_ptr->children, local_child_handle);
		if (!child_handle_ptr || *child_handle_ptr == S_HANDLE_NULL || *child_handle_ptr == sdf) {
			s_array_remove(&sdf_ptr->children, local_child_handle);
			continue;
		}
		se_sdf_destroy(*child_handle_ptr);
	}

	if (sdf_ptr->parent != S_HANDLE_NULL) {
		parent_ptr = se_sdf_from_handle(ctx, sdf_ptr->parent);
		if (parent_ptr) {
			se_sdf_remove_child_reference(parent_ptr, sdf);
			se_sdf_invalidate_shader_chain(sdf_ptr->parent);
		}
		sdf_ptr->parent = S_HANDLE_NULL;
	}

	se_sdf_release_noise_references(ctx, sdf_ptr);
	se_sdf_release_point_light_references(ctx, sdf_ptr);
	se_sdf_release_directional_light_references(ctx, sdf_ptr);

	se_sdf_destroy_shader_runtime(sdf_ptr);
	if (sdf_ptr->output != S_HANDLE_NULL) {
		se_framebuffer_destroy(sdf_ptr->output);
		sdf_ptr->output = S_HANDLE_NULL;
	}
	se_quad_destroy(&sdf_ptr->quad);
	s_array_clear(&sdf_ptr->children);
	s_array_clear(&sdf_ptr->noises);
	s_array_clear(&sdf_ptr->point_lights);
	s_array_clear(&sdf_ptr->directional_lights);
	s_array_remove(&ctx->sdfs, sdf);
}

void se_sdf_add_child(se_sdf_handle parent, se_sdf_handle child) {
	se_context* ctx = se_current_context();
	se_sdf* parent_ptr = se_sdf_from_handle(ctx, parent);
	se_sdf* child_ptr = se_sdf_from_handle(ctx, child);
	if (!parent_ptr || !child_ptr) {
		return;
	}
	child_ptr->parent = parent;
	s_array_add(&parent_ptr->children, child);
	se_sdf_invalidate_shader_chain(parent);
}

se_sdf_noise_handle se_sdf_add_noise_internal(se_sdf_handle sdf, const se_sdf_noise noise) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_noise_handle new_noise = S_HANDLE_NULL;
	se_sdf_noise* noise_ptr = NULL;
	if (!sdf_ptr || !se_sdf_noise_is_active(&noise)) {
		return S_HANDLE_NULL;
	}
	new_noise = s_array_add(&ctx->sdf_noises, noise);
	noise_ptr = se_sdf_noise_from_handle(ctx, new_noise);
	if (!noise_ptr) {
		return S_HANDLE_NULL;
	}
	*noise_ptr = noise;
	noise_ptr->frequency = se_sdf_get_noise_frequency_defaulted(&noise);
	s_array_add(&sdf_ptr->noises, new_noise);
	se_sdf_invalidate_shader_chain(sdf);
	return new_noise;
}

f32 se_sdf_get_noise_frequency(se_sdf_noise_handle noise) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return 0.0f;
	}
	return noise_ptr->frequency;
}

void se_sdf_noise_set_frequency(se_sdf_noise_handle noise, f32 frequency) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return;
	}
	noise_ptr->frequency = se_sdf_get_noise_frequency_defaulted(&(se_sdf_noise){
		.type = noise_ptr->type,
		.frequency = frequency,
		.offset = noise_ptr->offset,
	});
}

s_vec3 se_sdf_get_noise_offset(se_sdf_noise_handle noise) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	return noise_ptr->offset;
}

void se_sdf_noise_set_offset(se_sdf_noise_handle noise, const s_vec3* offset) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr || !offset) {
		return;
	}
	noise_ptr->offset = *offset;
}

se_sdf_point_light_handle se_sdf_add_point_light_internal(se_sdf_handle sdf, const se_sdf_point_light point_light) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_point_light_handle new_point_light = S_HANDLE_NULL;
	se_sdf_point_light* point_light_ptr = NULL;
	if (!sdf_ptr) {
		return S_HANDLE_NULL;
	}
	new_point_light = s_array_add(&ctx->sdf_point_lights, point_light);
	point_light_ptr = se_sdf_point_light_from_handle(ctx, new_point_light);
	if (!point_light_ptr) {
		return S_HANDLE_NULL;
	}
	*point_light_ptr = point_light;
	if (se_sdf_vec3_is_zero(&point_light_ptr->color)) {
		point_light_ptr->color = s_vec3(1.0f, 1.0f, 1.0f);
	}
	point_light_ptr->radius = se_sdf_get_point_light_radius_defaulted(point_light_ptr);
	s_array_add(&sdf_ptr->point_lights, new_point_light);
	se_sdf_invalidate_shader_chain(sdf);
	return new_point_light;
}

void se_sdf_point_light_remove(se_sdf_handle sdf, se_sdf_point_light_handle point_light) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	for (u32 i = 0; sdf_ptr && i < (u32)s_array_get_size(&sdf_ptr->point_lights); ++i) {
		const s_handle local_handle = s_array_handle_at(&sdf_ptr->point_lights.b, i);
		se_sdf_point_light_handle* point_light_handle_ptr = s_array_get(&sdf_ptr->point_lights, local_handle);
		if (!point_light_handle_ptr || *point_light_handle_ptr != point_light) {
			continue;
		}
		s_array_remove(&sdf_ptr->point_lights, local_handle);
		if (!se_sdf_point_light_is_referenced(ctx, point_light)) {
			s_array_remove(&ctx->sdf_point_lights, point_light);
		}
		se_sdf_invalidate_shader_chain(sdf);
		return;
	}
}

s_vec3 se_sdf_get_point_light_position(se_sdf_point_light_handle point_light) {
	se_context* ctx = se_current_context();
	se_sdf_point_light* point_light_ptr = se_sdf_point_light_from_handle(ctx, point_light);
	if (!point_light_ptr) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	return point_light_ptr->position;
}

void se_sdf_point_light_set_position(se_sdf_point_light_handle point_light, const s_vec3* position) {
	se_context* ctx = se_current_context();
	se_sdf_point_light* point_light_ptr = se_sdf_point_light_from_handle(ctx, point_light);
	if (!point_light_ptr || !position) {
		return;
	}
	point_light_ptr->position = *position;
}

s_vec3 se_sdf_get_point_light_color(se_sdf_point_light_handle point_light) {
	se_context* ctx = se_current_context();
	se_sdf_point_light* point_light_ptr = se_sdf_point_light_from_handle(ctx, point_light);
	if (!point_light_ptr) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	return point_light_ptr->color;
}

void se_sdf_point_light_set_color(se_sdf_point_light_handle point_light, const s_vec3* color) {
	se_context* ctx = se_current_context();
	se_sdf_point_light* point_light_ptr = se_sdf_point_light_from_handle(ctx, point_light);
	if (!point_light_ptr || !color) {
		return;
	}
	point_light_ptr->color = *color;
}

f32 se_sdf_get_point_light_radius(se_sdf_point_light_handle point_light) {
	se_context* ctx = se_current_context();
	se_sdf_point_light* point_light_ptr = se_sdf_point_light_from_handle(ctx, point_light);
	if (!point_light_ptr) {
		return 0.0f;
	}
	return point_light_ptr->radius;
}

void se_sdf_point_light_set_radius(se_sdf_point_light_handle point_light, f32 radius) {
	se_context* ctx = se_current_context();
	se_sdf_point_light* point_light_ptr = se_sdf_point_light_from_handle(ctx, point_light);
	if (!point_light_ptr) {
		return;
	}
	point_light_ptr->radius = radius < 0.0f ? 0.0f : radius;
}

se_sdf_directional_light_handle se_sdf_add_directional_light_internal(se_sdf_handle sdf, const se_sdf_directional_light directional_light) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_directional_light_handle new_directional_light = S_HANDLE_NULL;
	se_sdf_directional_light* directional_light_ptr = NULL;
	if (!sdf_ptr) {
		return S_HANDLE_NULL;
	}
	new_directional_light = s_array_add(&ctx->sdf_directional_lights, directional_light);
	directional_light_ptr = se_sdf_directional_light_from_handle(ctx, new_directional_light);
	if (!directional_light_ptr) {
		return S_HANDLE_NULL;
	}
	*directional_light_ptr = directional_light;
	if (se_sdf_vec3_is_zero(&directional_light_ptr->direction)) {
		directional_light_ptr->direction = s_vec3(
			SE_SDF_DEFAULT_LIGHT_DIR_X,
			SE_SDF_DEFAULT_LIGHT_DIR_Y,
			SE_SDF_DEFAULT_LIGHT_DIR_Z);
	}
	directional_light_ptr->direction = s_vec3_normalize(&directional_light_ptr->direction);
	if (se_sdf_vec3_is_zero(&directional_light_ptr->color)) {
		directional_light_ptr->color = s_vec3(1.0f, 1.0f, 1.0f);
	}
	s_array_add(&sdf_ptr->directional_lights, new_directional_light);
	se_sdf_invalidate_shader_chain(sdf);
	return new_directional_light;
}

s_vec3 se_sdf_get_directional_light_direction(se_sdf_directional_light_handle directional_light) {
	se_context* ctx = se_current_context();
	se_sdf_directional_light* directional_light_ptr = se_sdf_directional_light_from_handle(ctx, directional_light);
	if (!directional_light_ptr) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	return directional_light_ptr->direction;
}

void se_sdf_directional_light_set_direction(se_sdf_directional_light_handle directional_light, const s_vec3* direction) {
	se_context* ctx = se_current_context();
	se_sdf_directional_light* directional_light_ptr = se_sdf_directional_light_from_handle(ctx, directional_light);
	if (!directional_light_ptr || !direction) {
		return;
	}
	if (se_sdf_vec3_is_zero(direction)) {
		directional_light_ptr->direction = s_vec3(
			SE_SDF_DEFAULT_LIGHT_DIR_X,
			SE_SDF_DEFAULT_LIGHT_DIR_Y,
			SE_SDF_DEFAULT_LIGHT_DIR_Z);
		return;
	}
	directional_light_ptr->direction = s_vec3_normalize(direction);
}

s_vec3 se_sdf_get_directional_light_color(se_sdf_directional_light_handle directional_light) {
	se_context* ctx = se_current_context();
	se_sdf_directional_light* directional_light_ptr = se_sdf_directional_light_from_handle(ctx, directional_light);
	if (!directional_light_ptr) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	return directional_light_ptr->color;
}

void se_sdf_directional_light_set_color(se_sdf_directional_light_handle directional_light, const s_vec3* color) {
	se_context* ctx = se_current_context();
	se_sdf_directional_light* directional_light_ptr = se_sdf_directional_light_from_handle(ctx, directional_light);
	if (!directional_light_ptr || !color) {
		return;
	}
	directional_light_ptr->color = *color;
}

se_sdf_lods se_sdf_get_lods(se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr) {
		return (se_sdf_lods){0};
	}
	return se_sdf_get_lods_defaulted(sdf_ptr);
}

void se_sdf_set_lods(se_sdf_handle sdf, const se_sdf_lods* lods) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr || !lods) {
		return;
	}
	sdf_ptr->lods = se_sdf_get_lods_defaulted(&(se_sdf){
		.lods = *lods,
	});
}

se_sdf_shading se_sdf_get_shading(se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr) {
		return (se_sdf_shading){0};
	}
	return se_sdf_get_shading_defaulted(sdf_ptr);
}

void se_sdf_set_shading(se_sdf_handle sdf, const se_sdf_shading* shading) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr || !shading) {
		return;
	}
	sdf_ptr->shading = *shading;
}

s_vec3 se_sdf_get_shading_ambient(se_sdf_handle sdf) {
	return se_sdf_get_shading(sdf).ambient;
}

void se_sdf_set_shading_ambient(se_sdf_handle sdf, const s_vec3* ambient) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shading shading = se_sdf_get_shading(sdf);
	if (!sdf_ptr || !ambient) {
		return;
	}
	shading.ambient = *ambient;
	sdf_ptr->shading = shading;
}

s_vec3 se_sdf_get_shading_diffuse(se_sdf_handle sdf) {
	return se_sdf_get_shading(sdf).diffuse;
}

void se_sdf_set_shading_diffuse(se_sdf_handle sdf, const s_vec3* diffuse) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shading shading = se_sdf_get_shading(sdf);
	if (!sdf_ptr || !diffuse) {
		return;
	}
	shading.diffuse = *diffuse;
	sdf_ptr->shading = shading;
}

s_vec3 se_sdf_get_shading_specular(se_sdf_handle sdf) {
	return se_sdf_get_shading(sdf).specular;
}

void se_sdf_set_shading_specular(se_sdf_handle sdf, const s_vec3* specular) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shading shading = se_sdf_get_shading(sdf);
	if (!sdf_ptr || !specular) {
		return;
	}
	shading.specular = *specular;
	sdf_ptr->shading = shading;
}

f32 se_sdf_get_shading_roughness(se_sdf_handle sdf) {
	return se_sdf_get_shading(sdf).roughness;
}

void se_sdf_set_shading_roughness(se_sdf_handle sdf, f32 roughness) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shading shading = se_sdf_get_shading(sdf);
	if (!sdf_ptr) {
		return;
	}
	shading.roughness = roughness;
	sdf_ptr->shading = shading;
}

f32 se_sdf_get_shading_bias(se_sdf_handle sdf) {
	return se_sdf_get_shading(sdf).bias;
}

void se_sdf_set_shading_bias(se_sdf_handle sdf, f32 bias) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shading shading = se_sdf_get_shading(sdf);
	if (!sdf_ptr) {
		return;
	}
	shading.bias = bias;
	sdf_ptr->shading = shading;
}

f32 se_sdf_get_shading_smoothness(se_sdf_handle sdf) {
	return se_sdf_get_shading(sdf).smoothness;
}

void se_sdf_set_shading_smoothness(se_sdf_handle sdf, f32 smoothness) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shading shading = se_sdf_get_shading(sdf);
	if (!sdf_ptr) {
		return;
	}
	shading.smoothness = smoothness;
	sdf_ptr->shading = shading;
}

se_sdf_shadow se_sdf_get_shadow(se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr) {
		return (se_sdf_shadow){0};
	}
	return se_sdf_get_shadow_defaulted(sdf_ptr);
}

void se_sdf_set_shadow(se_sdf_handle sdf, const se_sdf_shadow* shadow) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr || !shadow) {
		return;
	}
	sdf_ptr->shadow = *shadow;
}

f32 se_sdf_get_shadow_softness(se_sdf_handle sdf) {
	return se_sdf_get_shadow(sdf).softness;
}

void se_sdf_set_shadow_softness(se_sdf_handle sdf, f32 softness) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shadow shadow = se_sdf_get_shadow(sdf);
	if (!sdf_ptr) {
		return;
	}
	shadow.softness = softness;
	sdf_ptr->shadow = shadow;
}

f32 se_sdf_get_shadow_bias(se_sdf_handle sdf) {
	return se_sdf_get_shadow(sdf).bias;
}

void se_sdf_set_shadow_bias(se_sdf_handle sdf, f32 bias) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shadow shadow = se_sdf_get_shadow(sdf);
	if (!sdf_ptr) {
		return;
	}
	shadow.bias = bias;
	sdf_ptr->shadow = shadow;
}

u16 se_sdf_get_shadow_samples(se_sdf_handle sdf) {
	return se_sdf_get_shadow(sdf).samples;
}

void se_sdf_set_shadow_samples(se_sdf_handle sdf, u16 samples) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_shadow shadow = se_sdf_get_shadow(sdf);
	if (!sdf_ptr) {
		return;
	}
	shadow.samples = samples;
	sdf_ptr->shadow = shadow;
}

void se_sdf_render_raw(se_sdf_handle sdf, se_camera_handle camera) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_camera* camera_ptr = se_camera_get(camera);
	s_mat4 view_projection = s_mat4_identity;
	s_mat4 inverse_view_projection = s_mat4_identity;
	if (!sdf_ptr || !camera_ptr || sdf_ptr->shader == S_HANDLE_NULL) {
		return;
	}
	view_projection = se_camera_get_view_projection_matrix(camera);
	inverse_view_projection = s_mat4_inverse(&view_projection);
	se_shader_set_mat4(sdf_ptr->shader, "u_inv_view_projection", &inverse_view_projection);
	se_shader_set_vec3(sdf_ptr->shader, "u_camera_position", &camera_ptr->position);
	se_shader_set_int(sdf_ptr->shader, "u_use_orthographic", camera_ptr->use_orthographic ? 1 : 0);
	se_sdf_upload_lod_uniforms(sdf_ptr->shader, sdf_ptr);
	se_sdf_upload_uniforms_recursive(sdf_ptr->shader, sdf);
	se_shader_use(sdf_ptr->shader, true, true);
	se_quad_render(&sdf_ptr->quad, 0);
}

void se_sdf_render(se_sdf_handle sdf, se_camera_handle camera) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!ctx || !sdf_ptr || camera == S_HANDLE_NULL) {
		return;
	}
	if (sdf_ptr->quad.vao == 0) {
		se_quad_2d_create(&sdf_ptr->quad, 0);
	}
	if (sdf_ptr->shader == S_HANDLE_NULL) {
		c8* vs = calloc(SE_SDF_VERTEX_SHADER_CAPACITY, sizeof(c8));
		c8* fs = calloc(SE_SDF_FRAGMENT_SHADER_CAPACITY, sizeof(c8));
		if (!vs || !fs) {
			free(vs);
			free(fs);
			return;
		}
		se_sdf_gen_vertex(vs, SE_SDF_VERTEX_SHADER_CAPACITY);
		se_sdf_gen_fragment(fs, SE_SDF_FRAGMENT_SHADER_CAPACITY, sdf);
		sdf_ptr->shader = se_shader_load_from_memory(vs, fs);
		free(vs);
		free(fs);
		if (sdf_ptr->shader == S_HANDLE_NULL) {
			se_log("se_sdf_render :: failed to create shader for sdf: " SE_SDF_HANDLE_FMT, SE_SDF_HANDLE_ARG(sdf));
			return;
		}
	}
	const GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
	const GLboolean cull_face_enabled = glIsEnabled(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	se_sdf_render_raw(sdf, camera);
	if (depth_test_enabled) {
		glEnable(GL_DEPTH_TEST);
	}
	if (cull_face_enabled) {
		glEnable(GL_CULL_FACE);
	}
}

void se_sdf_bake(se_sdf_handle sdf) {
	(void)sdf;
}

void se_sdf_set_position(se_sdf_handle sdf, const s_vec3* position) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr || !position) {
		return;
	}
	s_mat4_set_translation(&sdf_ptr->transform, position);
}
