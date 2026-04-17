// Syphax-Engine - Ougi Washi

#include "se_sdf.h"

#include "render/se_gl.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_defines.h"
#include "se_debug.h"
#include "se_math.h"
#include "se_resource_io.h"
#include "se_shader.h"
#include "se_framebuffer.h"
#include "se_texture.h"
#include "syphax/s_files.h"
#include "syphax/s_json.h"

#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SE_SDF_FRAGMENT_SHADER_CAPACITY (256u * 1024u)
#define SE_SDF_FAR_DISTANCE 1000.0f
#define SE_SDF_MAX_TRACE_STEPS 128u
#define SE_SDF_HIT_EPSILON 0.01f
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
#define SE_SDF_VERTEX_SHADER_TEMPLATE_PATH SE_RESOURCE_INTERNAL("shaders/sdf/sdf_vert.glsl")
#define SE_SDF_COMMON_SHADER_TEMPLATE_PATH SE_RESOURCE_INTERNAL("shaders/sdf/sdf_common.glsl")
#define SE_SDF_FRAGMENT_SHADER_TEMPLATE_PATH SE_RESOURCE_INTERNAL("shaders/sdf/sdf_fragment_template.glsl")
#define SE_SDF_JSON_FORMAT "se_sdf_json"
#define SE_SDF_JSON_MAX_SAFE_INTEGER_U64 9007199254740991ULL
#define SE_SDF_JSON_PRECISION_SCALE 1000.0f
#define SE_SDF_HANDLE_FMT "%llu"
#define SE_SDF_HANDLE_ARG(_handle) ((unsigned long long)(_handle))

enum {
	SE_SDF_JSON_VERSION = 1u
};

typedef struct se_sdf_template_part {
	const c8* token;
	const c8* value;
} se_sdf_template_part;

typedef struct se_sdf_shader_template_state {
	time_t vertex_mtime;
	time_t common_mtime;
	time_t fragment_mtime;
	b8 initialized;
} se_sdf_shader_template_state;

typedef struct se_sdf_noise {
	se_texture_handle texture;
	se_noise_2d descriptor;
} se_sdf_noise;

static se_sdf_shader_template_state g_se_sdf_shader_templates = {0};

static se_sdf* se_sdf_from_handle(se_context* ctx, const se_sdf_handle sdf);
static se_sdf_noise* se_sdf_noise_from_handle(se_context* ctx, const se_sdf_noise_handle noise);
static s_handle se_sdf_noise_record_handle_from_texture(se_context* ctx, const se_texture_handle texture);
static se_sdf_point_light* se_sdf_point_light_from_handle(se_context* ctx, const se_sdf_point_light_handle point_light);
static se_sdf_directional_light* se_sdf_directional_light_from_handle(se_context* ctx, const se_sdf_directional_light_handle directional_light);
static se_texture* se_sdf_texture_from_handle(se_context* ctx, const se_texture_handle texture);
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
static se_sdf_directional_light_handle se_sdf_get_first_directional_light_recursive(const se_sdf_handle sdf);
static se_sdf_shading se_sdf_get_default_shading(void);
static se_sdf_shading se_sdf_get_shading_defaulted(const se_sdf* sdf);
static se_sdf_shadow se_sdf_get_default_shadow(void);
static se_sdf_shadow se_sdf_get_shadow_defaulted(const se_sdf* sdf);
static f32 se_sdf_get_operation_amount(const se_sdf* sdf);
static f32 se_sdf_get_noise_frequency_defaulted(const se_sdf_noise* noise);
static s_vec3 se_sdf_get_noise_offset_defaulted(const se_sdf_noise* noise);
static f32 se_sdf_get_point_light_radius_defaulted(const se_sdf_point_light* point_light);
static GLenum se_sdf_texture_format_from_channel_count(i32 channels);
static b8 se_sdf_copy_texture_content(se_context* ctx, se_texture_handle dst_handle, se_texture_handle src_handle);
static void se_sdf_append(c8* out, const sz capacity, const c8* fmt, ...);
static void se_sdf_append_range(c8* out, const sz capacity, const c8* text, const sz text_size);
static const c8* se_sdf_get_operation_function_name(const se_sdf_operator operation);
static i32 se_sdf_base64_value(const c8 c);
static c8* se_sdf_base64_encode(const u8* data, const sz size);
static b8 se_sdf_base64_decode(const c8* in, u8** out_data, sz* out_size);
static b8 se_sdf_json_add_child(s_json* parent, s_json* child);
static s_json* se_sdf_json_add_object(s_json* parent, const c8* name);
static s_json* se_sdf_json_add_array(s_json* parent, const c8* name);
static b8 se_sdf_json_add_u32(s_json* parent, const c8* name, const u32 value);
static b8 se_sdf_json_add_f32(s_json* parent, const c8* name, const f32 value);
static b8 se_sdf_json_add_string(s_json* parent, const c8* name, const c8* value);
static b8 se_sdf_json_add_vec2(s_json* parent, const c8* name, const s_vec2* value);
static b8 se_sdf_json_add_vec3(s_json* parent, const c8* name, const s_vec3* value);
static b8 se_sdf_json_add_mat4(s_json* parent, const c8* name, const s_mat4* value);
static b8 se_sdf_json_number_to_u64(const s_json* node, u64* out_value);
static b8 se_sdf_json_number_to_u32(const s_json* node, u32* out_value);
static b8 se_sdf_json_number_to_f32(const s_json* node, f32* out_value);
static b8 se_sdf_json_string_to_c8(const s_json* node, const c8** out_value);
static const s_json* se_sdf_json_get_required(const s_json* object, const c8* key, const s_json_type type);
static b8 se_sdf_json_read_vec2(const s_json* node, s_vec2* out_value);
static b8 se_sdf_json_read_vec3(const s_json* node, s_vec3* out_value);
static b8 se_sdf_json_read_mat4(const s_json* node, s_mat4* out_value);
static const c8* se_sdf_json_type_name(const se_sdf_type type);
static b8 se_sdf_json_read_type(const s_json* node, se_sdf_type* out_value);
static const c8* se_sdf_json_operation_name(const se_sdf_operator operation);
static b8 se_sdf_json_read_operation(const s_json* node, se_sdf_operator* out_value);
static const c8* se_sdf_json_noise_type_name(const se_noise_type type);
static b8 se_sdf_json_read_noise_type(const s_json* node, se_noise_type* out_value);
static const c8* se_sdf_json_texture_wrap_name(const se_texture_wrap wrap);
static b8 se_sdf_json_read_texture_wrap(const s_json* node, se_texture_wrap* out_value);
static b8 se_sdf_json_apply_texture_pixels(se_context* ctx, se_texture_handle texture_handle, const u8* pixels, i32 width, i32 height, se_texture_wrap wrap);
static b8 se_sdf_json_populate_object(se_context* ctx, se_sdf_handle sdf, s_json* object);
static se_sdf_handle se_sdf_build_from_json_node(const s_json* node);
static void se_sdf_destroy_shader_runtime(se_sdf* sdf_ptr);
static void se_sdf_destroy_runtime_resources(se_context* ctx, se_sdf* sdf_ptr);
static void se_sdf_reset_keep_handle(se_context* ctx, const se_sdf_handle sdf);
static b8 se_sdf_transfer_state(se_context* ctx, const se_sdf_handle dst, const se_sdf_handle src);
static void se_sdf_invalidate_shader_chain(const se_sdf_handle sdf);
static void se_sdf_invalidate_all_shaders(se_context* ctx);
static b8 se_sdf_read_resource_text(c8** out_text, const c8* resource_path);
static b8 se_sdf_read_resource_mtime(time_t* out_mtime, const c8* resource_path);
static void se_sdf_refresh_shader_templates(se_context* ctx);
static const c8* se_sdf_find_template_value(const se_sdf_template_part* parts, const sz part_count, const c8* token, const sz token_size);
static void se_sdf_apply_template(c8* out, const sz capacity, const c8* template_source, const se_sdf_template_part* parts, const sz part_count);
static void se_sdf_gen_common_constants(c8* out, const sz capacity);
static void se_sdf_gen_uniform(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_uniform_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_function(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_function_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_surface_recursive(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_composite_light_apply_recursive(c8* out, const sz capacity, const se_sdf_handle sdf, const se_sdf_directional_light_handle shadow_light, b8* shadow_light_used);
static void se_sdf_gen_scene_wrappers(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_gen_light_apply(c8* out, const sz capacity, const se_sdf_handle sdf);
static b8 se_sdf_gen_fragment(c8* out, const sz capacity, const se_sdf_handle sdf);
static void se_sdf_upload_uniforms_recursive(const se_shader_handle shader, const se_sdf_handle sdf);

static se_sdf* se_sdf_from_handle(se_context* ctx, const se_sdf_handle sdf) {
	if (!ctx || sdf == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->sdfs, sdf);
}

static s_handle se_sdf_noise_record_handle_from_texture(se_context* ctx, const se_texture_handle texture) {
	if (!ctx || texture == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	for (u32 i = 0; i < (u32)s_array_get_size(&ctx->sdf_noises); ++i) {
		const s_handle record_handle = s_array_handle_at(&ctx->sdf_noises.b, i);
		se_sdf_noise* noise_ptr = s_array_get(&ctx->sdf_noises, record_handle);
		if (noise_ptr && noise_ptr->texture == texture) {
			return record_handle;
		}
	}
	return S_HANDLE_NULL;
}

static se_sdf_noise* se_sdf_noise_from_handle(se_context* ctx, const se_sdf_noise_handle noise) {
	const s_handle record_handle = se_sdf_noise_record_handle_from_texture(ctx, (se_texture_handle)noise);
	if (record_handle == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->sdf_noises, record_handle);
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

static se_texture* se_sdf_texture_from_handle(se_context* ctx, const se_texture_handle texture) {
	if (!ctx || texture == S_HANDLE_NULL) {
		return NULL;
	}
	return s_array_get(&ctx->textures, texture);
}

static b8 se_sdf_noise_is_referenced(se_context* ctx, const se_sdf_noise_handle noise) {
	se_sdf* sdf_ptr = NULL;
	se_texture_handle* noise_handle_ptr = NULL;
	if (!ctx || noise == S_HANDLE_NULL) {
		return false;
	}
	s_foreach(&ctx->sdfs, sdf_ptr) {
		s_foreach(&sdf_ptr->noises, noise_handle_ptr) {
			if (*noise_handle_ptr == (se_texture_handle)noise) {
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
		se_texture_handle* noise_handle_ptr = s_array_get(&sdf_ptr->noises, local_handle);
		se_texture_handle noise = S_HANDLE_NULL;
		if (!noise_handle_ptr) {
			s_array_remove(&sdf_ptr->noises, local_handle);
			continue;
		}
		noise = *noise_handle_ptr;
		s_array_remove(&sdf_ptr->noises, local_handle);
		if (!se_sdf_noise_is_referenced(ctx, (se_sdf_noise_handle)noise)) {
			const s_handle record_handle = se_sdf_noise_record_handle_from_texture(ctx, noise);
			if (record_handle != S_HANDLE_NULL) {
				se_noise_2d_destroy(ctx, noise);
				s_array_remove(&ctx->sdf_noises, record_handle);
			}
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
	return noise && noise->texture != S_HANDLE_NULL;
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
		const se_texture_handle* noise_handle_ptr = s_array_get_ptr(&sdf->noises.b, sizeof(se_texture_handle), noise_handle);
		const se_sdf_noise* noise = noise_handle_ptr ? se_sdf_noise_from_handle(ctx, (se_sdf_noise_handle)*noise_handle_ptr) : NULL;
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

static se_sdf_directional_light_handle se_sdf_get_first_directional_light_recursive(const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	if (!sdf_ptr) {
		return S_HANDLE_NULL;
	}
	if (s_array_get_size(&sdf_ptr->directional_lights) > 0) {
		se_sdf_directional_light_handle* directional_light = s_array_get(&sdf_ptr->directional_lights, s_array_handle(&sdf_ptr->directional_lights, 0u));
		return directional_light ? *directional_light : S_HANDLE_NULL;
	}
	s_foreach(&sdf_ptr->children, child) {
		const se_sdf_directional_light_handle found = se_sdf_get_first_directional_light_recursive(*child);
		if (found != S_HANDLE_NULL) {
			return found;
		}
	}
	return S_HANDLE_NULL;
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
	if (!noise || noise->descriptor.frequency <= 0.0f) {
		return 1.0f;
	}
	if (noise->descriptor.frequency < SE_SDF_MIN_NOISE_FREQUENCY) {
		return SE_SDF_MIN_NOISE_FREQUENCY;
	}
	return noise->descriptor.frequency;
}

static s_vec3 se_sdf_get_noise_offset_defaulted(const se_sdf_noise* noise) {
	if (!noise) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	return s_vec3(noise->descriptor.offset.x, noise->descriptor.offset.y, 0.0f);
}

static f32 se_sdf_get_point_light_radius_defaulted(const se_sdf_point_light* point_light) {
	if (!point_light || point_light->radius <= 0.0f) {
		return SE_SDF_DEFAULT_POINT_LIGHT_RADIUS;
	}
	return point_light->radius;
}

static GLenum se_sdf_texture_format_from_channel_count(i32 channels) {
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

static b8 se_sdf_copy_texture_content(se_context* ctx, se_texture_handle dst_handle, se_texture_handle src_handle) {
	se_texture* dst = se_sdf_texture_from_handle(ctx, dst_handle);
	se_texture* src = se_sdf_texture_from_handle(ctx, src_handle);
	u8* copied_pixels = NULL;
	GLenum format = 0u;

	if (!dst || !src || src->target != GL_TEXTURE_2D || src->width <= 0 || src->height <= 0 || !src->cpu_pixels || src->cpu_pixels_size == 0u) {
		return false;
	}

	copied_pixels = malloc(src->cpu_pixels_size);
	if (!copied_pixels) {
		return false;
	}
	memcpy(copied_pixels, src->cpu_pixels, src->cpu_pixels_size);

	if (dst->cpu_pixels) {
		free(dst->cpu_pixels);
	}

	dst->width = src->width;
	dst->height = src->height;
	dst->depth = src->depth;
	dst->channels = src->channels;
	dst->cpu_channels = src->cpu_channels;
	dst->target = src->target;
	dst->wrap = src->wrap;
	dst->cpu_pixels = copied_pixels;
	dst->cpu_pixels_size = src->cpu_pixels_size;
	dst->path[0] = '\0';

	if (dst->id == 0u) {
		glGenTextures(1, &dst->id);
	}
	format = se_sdf_texture_format_from_channel_count(dst->channels);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, dst->id);
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, dst->width, dst->height, 0, format, GL_UNSIGNED_BYTE, dst->cpu_pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, dst->wrap == SE_CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, dst->wrap == SE_CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	return true;
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

static void se_sdf_append_range(c8* out, const sz capacity, const c8* text, const sz text_size) {
	if (!out || capacity == 0 || !text || text_size == 0) {
		return;
	}
	const sz current_length = strlen(out);
	if (current_length >= capacity - 1u) {
		return;
	}
	const sz available = capacity - current_length - 1u;
	const sz copy_size = text_size < available ? text_size : available;
	memcpy(out + current_length, text, copy_size);
	out[current_length + copy_size] = '\0';
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

static i32 se_sdf_base64_value(const c8 c) {
	if (c >= 'A' && c <= 'Z') {
		return (i32)(c - 'A');
	}
	if (c >= 'a' && c <= 'z') {
		return (i32)(c - 'a' + 26);
	}
	if (c >= '0' && c <= '9') {
		return (i32)(c - '0' + 52);
	}
	if (c == '+') {
		return 62;
	}
	if (c == '/') {
		return 63;
	}
	return -1;
}

static c8* se_sdf_base64_encode(const u8* data, const sz size) {
	static const c8 table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (!data && size > 0u) {
		return NULL;
	}
	if (size == 0u) {
		c8* out = malloc(1u);
		if (!out) {
			return NULL;
		}
		out[0] = '\0';
		return out;
	}
	const sz encoded_len = ((size + 2u) / 3u) * 4u;
	c8* out = malloc(encoded_len + 1u);
	if (!out) {
		return NULL;
	}
	sz j = 0u;
	for (sz i = 0u; i < size; i += 3u) {
		const u32 octet_a = i < size ? data[i] : 0u;
		const u32 octet_b = (i + 1u) < size ? data[i + 1u] : 0u;
		const u32 octet_c = (i + 2u) < size ? data[i + 2u] : 0u;
		const u32 triple = (octet_a << 16u) | (octet_b << 8u) | octet_c;
		out[j++] = table[(triple >> 18u) & 0x3Fu];
		out[j++] = table[(triple >> 12u) & 0x3Fu];
		out[j++] = (i + 1u) < size ? table[(triple >> 6u) & 0x3Fu] : '=';
		out[j++] = (i + 2u) < size ? table[triple & 0x3Fu] : '=';
	}
	out[j] = '\0';
	return out;
}

static b8 se_sdf_base64_decode(const c8* in, u8** out_data, sz* out_size) {
	if (!in || !out_data || !out_size) {
		return false;
	}
	const sz len = strlen(in);
	if ((len % 4u) != 0u) {
		return false;
	}
	if (len == 0u) {
		u8* empty = malloc(1u);
		if (!empty) {
			return false;
		}
		*out_data = empty;
		*out_size = 0u;
		return true;
	}
	sz padding = 0u;
	if (in[len - 1u] == '=') {
		padding++;
	}
	if (in[len - 2u] == '=') {
		padding++;
	}
	const sz max_out_size = (len / 4u) * 3u - padding;
	u8* out = malloc(max_out_size > 0u ? max_out_size : 1u);
	if (!out) {
		return false;
	}
	sz out_pos = 0u;
	for (sz i = 0u; i < len; i += 4u) {
		const c8 c0 = in[i + 0u];
		const c8 c1 = in[i + 1u];
		const c8 c2 = in[i + 2u];
		const c8 c3 = in[i + 3u];
		const b8 c2_pad = (c2 == '=');
		const b8 c3_pad = (c3 == '=');
		if (c0 == '=' || c1 == '=') {
			free(out);
			return false;
		}
		if (c2_pad && !c3_pad) {
			free(out);
			return false;
		}
		if ((c2_pad || c3_pad) && (i + 4u) != len) {
			free(out);
			return false;
		}
		const i32 v0 = se_sdf_base64_value(c0);
		const i32 v1 = se_sdf_base64_value(c1);
		const i32 v2 = c2_pad ? 0 : se_sdf_base64_value(c2);
		const i32 v3 = c3_pad ? 0 : se_sdf_base64_value(c3);
		if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) {
			free(out);
			return false;
		}
		const u32 triple =
			((u32)v0 << 18u) |
			((u32)v1 << 12u) |
			((u32)v2 << 6u) |
			(u32)v3;
		if (out_pos < max_out_size) {
			out[out_pos++] = (u8)((triple >> 16u) & 0xFFu);
		}
		if (!c2_pad && out_pos < max_out_size) {
			out[out_pos++] = (u8)((triple >> 8u) & 0xFFu);
		}
		if (!c3_pad && out_pos < max_out_size) {
			out[out_pos++] = (u8)(triple & 0xFFu);
		}
	}
	*out_data = out;
	*out_size = max_out_size;
	return true;
}

static b8 se_sdf_json_add_child(s_json* parent, s_json* child) {
	if (!parent || !child || !s_json_add(parent, child)) {
		if (child) {
			s_json_free(child);
		}
		return false;
	}
	return true;
}

static s_json* se_sdf_json_add_object(s_json* parent, const c8* name) {
	s_json* object = s_json_object_empty(name);
	if (!object) {
		return NULL;
	}
	if (!se_sdf_json_add_child(parent, object)) {
		return NULL;
	}
	return object;
}

static s_json* se_sdf_json_add_array(s_json* parent, const c8* name) {
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return NULL;
	}
	if (!se_sdf_json_add_child(parent, array)) {
		return NULL;
	}
	return array;
}

static f32 se_sdf_json_round_f32(const f32 value) {
	if (!isfinite((f64)value)) {
		return value;
	}
	const f32 rounded = roundf(value * SE_SDF_JSON_PRECISION_SCALE) / SE_SDF_JSON_PRECISION_SCALE;
	return fabsf(rounded) < (0.5f / SE_SDF_JSON_PRECISION_SCALE) ? 0.0f : rounded;
}

static b8 se_sdf_json_add_u32(s_json* parent, const c8* name, const u32 value) {
	return se_sdf_json_add_child(parent, s_json_num(name, (f64)value));
}

static b8 se_sdf_json_add_f32(s_json* parent, const c8* name, const f32 value) {
	if (!isfinite((f64)value)) {
		return false;
	}
	return se_sdf_json_add_child(parent, s_json_num(name, (f64)se_sdf_json_round_f32(value)));
}

static b8 se_sdf_json_add_string(s_json* parent, const c8* name, const c8* value) {
	return se_sdf_json_add_child(parent, s_json_str(name, value ? value : ""));
}

static b8 se_sdf_json_add_vec2(s_json* parent, const c8* name, const s_vec2* value) {
	if (!parent || !value) {
		return false;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return false;
	}
	if (!se_sdf_json_add_f32(array, NULL, value->x) ||
		!se_sdf_json_add_f32(array, NULL, value->y)) {
		s_json_free(array);
		return false;
	}
	if (!se_sdf_json_add_child(parent, array)) {
		return false;
	}
	return true;
}

static b8 se_sdf_json_add_vec3(s_json* parent, const c8* name, const s_vec3* value) {
	if (!parent || !value) {
		return false;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return false;
	}
	if (!se_sdf_json_add_f32(array, NULL, value->x) ||
		!se_sdf_json_add_f32(array, NULL, value->y) ||
		!se_sdf_json_add_f32(array, NULL, value->z)) {
		s_json_free(array);
		return false;
	}
	if (!se_sdf_json_add_child(parent, array)) {
		return false;
	}
	return true;
}

static b8 se_sdf_json_add_mat4(s_json* parent, const c8* name, const s_mat4* value) {
	if (!parent || !value) {
		return false;
	}
	s_json* array = s_json_array_empty(name);
	if (!array) {
		return false;
	}
	for (u32 row = 0u; row < 4u; ++row) {
		for (u32 col = 0u; col < 4u; ++col) {
			if (!se_sdf_json_add_f32(array, NULL, value->m[row][col])) {
				s_json_free(array);
				return false;
			}
		}
	}
	if (!se_sdf_json_add_child(parent, array)) {
		return false;
	}
	return true;
}

static b8 se_sdf_json_number_to_u64(const s_json* node, u64* out_value) {
	if (!node || node->type != S_JSON_NUMBER || !out_value) {
		return false;
	}
	const f64 value = node->as.number;
	if (!isfinite(value) || value < 0.0 || value > (f64)SE_SDF_JSON_MAX_SAFE_INTEGER_U64) {
		return false;
	}
	const u64 integer = (u64)value;
	if ((f64)integer != value) {
		return false;
	}
	*out_value = integer;
	return true;
}

static b8 se_sdf_json_number_to_u32(const s_json* node, u32* out_value) {
	u64 value = 0u;
	if (!se_sdf_json_number_to_u64(node, &value) || value > (u64)UINT32_MAX) {
		return false;
	}
	*out_value = (u32)value;
	return true;
}

static b8 se_sdf_json_number_to_f32(const s_json* node, f32* out_value) {
	if (!node || node->type != S_JSON_NUMBER || !out_value) {
		return false;
	}
	const f64 value = node->as.number;
	if (!isfinite(value) || value < -(f64)FLT_MAX || value > (f64)FLT_MAX) {
		return false;
	}
	*out_value = se_sdf_json_round_f32((f32)value);
	return true;
}

static b8 se_sdf_json_string_to_c8(const s_json* node, const c8** out_value) {
	if (!node || node->type != S_JSON_STRING || !out_value) {
		return false;
	}
	*out_value = node->as.string ? node->as.string : "";
	return true;
}

static const s_json* se_sdf_json_get_required(const s_json* object, const c8* key, const s_json_type type) {
	if (!object || object->type != S_JSON_OBJECT || !key) {
		return NULL;
	}
	const s_json* child = s_json_get(object, key);
	if (!child || child->type != type) {
		return NULL;
	}
	return child;
}

static b8 se_sdf_json_read_vec2(const s_json* node, s_vec2* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 2u) {
		return false;
	}
	f32 values[2] = {0.0f, 0.0f};
	for (u32 i = 0u; i < 2u; ++i) {
		if (!se_sdf_json_number_to_f32(s_json_at(node, i), &values[i])) {
			return false;
		}
	}
	*out_value = s_vec2(values[0], values[1]);
	return true;
}

static b8 se_sdf_json_read_vec3(const s_json* node, s_vec3* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 3u) {
		return false;
	}
	f32 values[3] = {0.0f, 0.0f, 0.0f};
	for (u32 i = 0u; i < 3u; ++i) {
		if (!se_sdf_json_number_to_f32(s_json_at(node, i), &values[i])) {
			return false;
		}
	}
	*out_value = s_vec3(values[0], values[1], values[2]);
	return true;
}

static b8 se_sdf_json_read_mat4(const s_json* node, s_mat4* out_value) {
	if (!node || node->type != S_JSON_ARRAY || !out_value || node->as.children.count != 16u) {
		return false;
	}
	s_mat4 parsed = s_mat4_identity;
	u32 index = 0u;
	for (u32 row = 0u; row < 4u; ++row) {
		for (u32 col = 0u; col < 4u; ++col) {
			if (!se_sdf_json_number_to_f32(s_json_at(node, index++), &parsed.m[row][col])) {
				return false;
			}
		}
	}
	*out_value = parsed;
	return true;
}

static const c8* se_sdf_json_type_name(const se_sdf_type type) {
	switch (type) {
		case SE_SDF_CUSTOM:
			return "custom";
		case SE_SDF_SPHERE:
			return "sphere";
		case SE_SDF_BOX:
			return "box";
		default:
			return NULL;
	}
}

static b8 se_sdf_json_read_type(const s_json* node, se_sdf_type* out_value) {
	if (!node || !out_value) {
		return false;
	}
	if (node->type == S_JSON_STRING) {
		const c8* value = node->as.string ? node->as.string : "";
		if (strcmp(value, "custom") == 0) {
			*out_value = SE_SDF_CUSTOM;
			return true;
		}
		if (strcmp(value, "sphere") == 0) {
			*out_value = SE_SDF_SPHERE;
			return true;
		}
		if (strcmp(value, "box") == 0) {
			*out_value = SE_SDF_BOX;
			return true;
		}
		return false;
	}
	if (node->type == S_JSON_NUMBER) {
		u32 value = 0u;
		if (!se_sdf_json_number_to_u32(node, &value) || value > (u32)SE_SDF_BOX) {
			return false;
		}
		*out_value = (se_sdf_type)value;
		return true;
	}
	return false;
}

static const c8* se_sdf_json_operation_name(const se_sdf_operator operation) {
	switch (operation) {
		case SE_SDF_UNION:
			return "union";
		case SE_SDF_SMOOTH_UNION:
			return "smooth_union";
		default:
			return NULL;
	}
}

static b8 se_sdf_json_read_operation(const s_json* node, se_sdf_operator* out_value) {
	if (!node || !out_value) {
		return false;
	}
	if (node->type == S_JSON_STRING) {
		const c8* value = node->as.string ? node->as.string : "";
		if (strcmp(value, "union") == 0) {
			*out_value = SE_SDF_UNION;
			return true;
		}
		if (strcmp(value, "smooth_union") == 0) {
			*out_value = SE_SDF_SMOOTH_UNION;
			return true;
		}
		return false;
	}
	if (node->type == S_JSON_NUMBER) {
		u32 value = 0u;
		if (!se_sdf_json_number_to_u32(node, &value) || value > (u32)SE_SDF_SMOOTH_UNION) {
			return false;
		}
		*out_value = (se_sdf_operator)value;
		return true;
	}
	return false;
}

static const c8* se_sdf_json_noise_type_name(const se_noise_type type) {
	switch (type) {
		case SE_NOISE_SIMPLEX:
			return "simplex";
		case SE_NOISE_PERLIN:
			return "perlin";
		case SE_NOISE_VORNOI:
			return "voronoi";
		case SE_NOISE_WORLEY:
			return "worley";
		default:
			return NULL;
	}
}

static b8 se_sdf_json_read_noise_type(const s_json* node, se_noise_type* out_value) {
	if (!node || !out_value) {
		return false;
	}
	if (node->type == S_JSON_STRING) {
		const c8* value = node->as.string ? node->as.string : "";
		if (strcmp(value, "simplex") == 0) {
			*out_value = SE_NOISE_SIMPLEX;
			return true;
		}
		if (strcmp(value, "perlin") == 0) {
			*out_value = SE_NOISE_PERLIN;
			return true;
		}
		if (strcmp(value, "voronoi") == 0 || strcmp(value, "vornoi") == 0) {
			*out_value = SE_NOISE_VORNOI;
			return true;
		}
		if (strcmp(value, "worley") == 0) {
			*out_value = SE_NOISE_WORLEY;
			return true;
		}
		return false;
	}
	if (node->type == S_JSON_NUMBER) {
		u32 value = 0u;
		if (!se_sdf_json_number_to_u32(node, &value) || value > (u32)SE_NOISE_WORLEY) {
			return false;
		}
		*out_value = (se_noise_type)value;
		return true;
	}
	return false;
}

static const c8* se_sdf_json_texture_wrap_name(const se_texture_wrap wrap) {
	switch (wrap) {
		case SE_REPEAT:
			return "repeat";
		case SE_CLAMP:
			return "clamp";
		default:
			return NULL;
	}
}

static b8 se_sdf_json_read_texture_wrap(const s_json* node, se_texture_wrap* out_value) {
	if (!node || !out_value) {
		return false;
	}
	if (node->type == S_JSON_STRING) {
		const c8* value = node->as.string ? node->as.string : "";
		if (strcmp(value, "repeat") == 0) {
			*out_value = SE_REPEAT;
			return true;
		}
		if (strcmp(value, "clamp") == 0) {
			*out_value = SE_CLAMP;
			return true;
		}
		return false;
	}
	if (node->type == S_JSON_NUMBER) {
		u32 value = 0u;
		if (!se_sdf_json_number_to_u32(node, &value) || value > (u32)SE_CLAMP) {
			return false;
		}
		*out_value = (se_texture_wrap)value;
		return true;
	}
	return false;
}

static b8 se_sdf_json_apply_texture_pixels(se_context* ctx, se_texture_handle texture_handle, const u8* pixels, i32 width, i32 height, se_texture_wrap wrap) {
	se_texture* texture = se_sdf_texture_from_handle(ctx, texture_handle);
	if (!texture || !pixels || width <= 0 || height <= 0) {
		return false;
	}

	const sz pixel_size = (sz)width * (sz)height * 4u;
	u8* copy = malloc(pixel_size);
	if (!copy) {
		return false;
	}
	memcpy(copy, pixels, pixel_size);

	if (texture->cpu_pixels) {
		free(texture->cpu_pixels);
	}
	texture->width = width;
	texture->height = height;
	texture->depth = 1;
	texture->channels = 4;
	texture->cpu_channels = 4;
	texture->target = GL_TEXTURE_2D;
	texture->wrap = wrap;
	texture->cpu_pixels = copy;
	texture->cpu_pixels_size = pixel_size;
	texture->path[0] = '\0';

	if (texture->id == 0u) {
		glGenTextures(1, &texture->id);
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->cpu_pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap == SE_CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap == SE_CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	return true;
}

static b8 se_sdf_json_populate_object(se_context* ctx, se_sdf_handle sdf, s_json* object) {
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!ctx || !sdf_ptr || !object || object->type != S_JSON_OBJECT) {
		se_set_last_error(!ctx || !object ? SE_RESULT_INVALID_ARGUMENT : SE_RESULT_NOT_FOUND);
		return false;
	}

	const c8* type_name = se_sdf_json_type_name(sdf_ptr->type);
	const c8* operation_name = se_sdf_json_operation_name(sdf_ptr->operation);
	const b8 write_operation = sdf_ptr->operation != SE_SDF_UNION;
	const b8 write_operation_amount = fabsf(sdf_ptr->operation_amount) > SE_SDF_MATRIX_EPSILON;
	s_json* shading_json = NULL;
	s_json* shadow_json = NULL;
	s_json* noises_json = NULL;
	s_json* point_lights_json = NULL;
	s_json* directional_lights_json = NULL;
	s_json* children_json = NULL;
	if (!type_name || (write_operation && !operation_name) ||
		!se_sdf_json_add_mat4(object, "transform", &sdf_ptr->transform) ||
		!se_sdf_json_add_string(object, "type", type_name) ||
		(write_operation && !se_sdf_json_add_string(object, "operation", operation_name)) ||
		(write_operation_amount && !se_sdf_json_add_f32(object, "operation_amount", sdf_ptr->operation_amount))) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}

	if (!se_sdf_shading_is_zero(&sdf_ptr->shading)) {
		shading_json = se_sdf_json_add_object(object, "shading");
		if (!shading_json ||
			!se_sdf_json_add_vec3(shading_json, "ambient", &sdf_ptr->shading.ambient) ||
			!se_sdf_json_add_vec3(shading_json, "diffuse", &sdf_ptr->shading.diffuse) ||
			!se_sdf_json_add_vec3(shading_json, "specular", &sdf_ptr->shading.specular) ||
			!se_sdf_json_add_f32(shading_json, "roughness", sdf_ptr->shading.roughness) ||
			!se_sdf_json_add_f32(shading_json, "bias", sdf_ptr->shading.bias) ||
			!se_sdf_json_add_f32(shading_json, "smoothness", sdf_ptr->shading.smoothness)) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	}

	if (!se_sdf_shadow_is_zero(&sdf_ptr->shadow)) {
		shadow_json = se_sdf_json_add_object(object, "shadow");
		if (!shadow_json ||
			!se_sdf_json_add_f32(shadow_json, "softness", sdf_ptr->shadow.softness) ||
			!se_sdf_json_add_f32(shadow_json, "bias", sdf_ptr->shadow.bias) ||
			!se_sdf_json_add_u32(shadow_json, "samples", sdf_ptr->shadow.samples)) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	}

	if (sdf_ptr->type == SE_SDF_SPHERE) {
		s_json* sphere_json = se_sdf_json_add_object(object, "sphere");
		if (!sphere_json || !se_sdf_json_add_f32(sphere_json, "radius", sdf_ptr->sphere.radius)) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	} else if (sdf_ptr->type == SE_SDF_BOX) {
		s_json* box_json = se_sdf_json_add_object(object, "box");
		if (!box_json || !se_sdf_json_add_vec3(box_json, "size", &sdf_ptr->box.size)) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	}

	if (s_array_get_size(&sdf_ptr->noises) > 0u) {
		noises_json = se_sdf_json_add_array(object, "noises");
		if (!noises_json) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	}

	for (u32 i = 0; i < (u32)s_array_get_size(&sdf_ptr->noises); ++i) {
		const s_handle local_handle = s_array_handle_at(&sdf_ptr->noises.b, i);
		se_texture_handle* noise_handle_ptr = s_array_get(&sdf_ptr->noises, local_handle);
		se_sdf_noise* noise_ptr = noise_handle_ptr ? se_sdf_noise_from_handle(ctx, (se_sdf_noise_handle)*noise_handle_ptr) : NULL;
		se_texture* texture_ptr = noise_handle_ptr ? se_sdf_texture_from_handle(ctx, *noise_handle_ptr) : NULL;
		if (!noise_handle_ptr || !noise_ptr || !texture_ptr || !texture_ptr->cpu_pixels || texture_ptr->width <= 0 || texture_ptr->height <= 0) {
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return false;
		}
		const sz expected_size = (sz)texture_ptr->width * (sz)texture_ptr->height * 4u;
		if (texture_ptr->cpu_pixels_size != expected_size) {
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			return false;
		}
		c8* pixels_b64 = se_sdf_base64_encode(texture_ptr->cpu_pixels, texture_ptr->cpu_pixels_size);
		if (!pixels_b64) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
		const c8* noise_type_name = se_sdf_json_noise_type_name(noise_ptr->descriptor.type);
		const c8* wrap_name = se_sdf_json_texture_wrap_name(texture_ptr->wrap);
		s_json* noise_json = se_sdf_json_add_object(noises_json, NULL);
		s_json* descriptor_json = noise_json ? se_sdf_json_add_object(noise_json, "descriptor") : NULL;
		s_json* texture_json = noise_json ? se_sdf_json_add_object(noise_json, "texture") : NULL;
		const s_vec2 offset = noise_ptr->descriptor.offset;
		const s_vec2 scale = noise_ptr->descriptor.scale;
		const b8 ok = noise_type_name && wrap_name &&
			noise_json && descriptor_json && texture_json &&
			se_sdf_json_add_string(descriptor_json, "type", noise_type_name) &&
			se_sdf_json_add_f32(descriptor_json, "frequency", noise_ptr->descriptor.frequency) &&
			se_sdf_json_add_vec2(descriptor_json, "offset", &offset) &&
			se_sdf_json_add_vec2(descriptor_json, "scale", &scale) &&
			se_sdf_json_add_u32(descriptor_json, "seed", noise_ptr->descriptor.seed) &&
			se_sdf_json_add_string(texture_json, "wrap", wrap_name) &&
			se_sdf_json_add_u32(texture_json, "width", (u32)texture_ptr->width) &&
			se_sdf_json_add_u32(texture_json, "height", (u32)texture_ptr->height) &&
			se_sdf_json_add_string(texture_json, "pixels_rgba_b64", pixels_b64);
		free(pixels_b64);
		if (!ok) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	}

	if (s_array_get_size(&sdf_ptr->point_lights) > 0u) {
		point_lights_json = se_sdf_json_add_array(object, "point_lights");
		if (!point_lights_json) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	}

	for (u32 i = 0; i < (u32)s_array_get_size(&sdf_ptr->point_lights); ++i) {
		const s_handle local_handle = s_array_handle_at(&sdf_ptr->point_lights.b, i);
		se_sdf_point_light_handle* point_light_handle_ptr = s_array_get(&sdf_ptr->point_lights, local_handle);
		se_sdf_point_light* point_light_ptr = point_light_handle_ptr ? se_sdf_point_light_from_handle(ctx, *point_light_handle_ptr) : NULL;
		s_json* point_light_json = se_sdf_json_add_object(point_lights_json, NULL);
		if (!point_light_ptr || !point_light_json ||
			!se_sdf_json_add_vec3(point_light_json, "position", &point_light_ptr->position) ||
			!se_sdf_json_add_vec3(point_light_json, "color", &point_light_ptr->color) ||
			!se_sdf_json_add_f32(point_light_json, "radius", point_light_ptr->radius)) {
			se_set_last_error(point_light_ptr ? SE_RESULT_OUT_OF_MEMORY : SE_RESULT_NOT_FOUND);
			return false;
		}
	}

	if (s_array_get_size(&sdf_ptr->directional_lights) > 0u) {
		directional_lights_json = se_sdf_json_add_array(object, "directional_lights");
		if (!directional_lights_json) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	}

	for (u32 i = 0; i < (u32)s_array_get_size(&sdf_ptr->directional_lights); ++i) {
		const s_handle local_handle = s_array_handle_at(&sdf_ptr->directional_lights.b, i);
		se_sdf_directional_light_handle* directional_light_handle_ptr = s_array_get(&sdf_ptr->directional_lights, local_handle);
		se_sdf_directional_light* directional_light_ptr = directional_light_handle_ptr ? se_sdf_directional_light_from_handle(ctx, *directional_light_handle_ptr) : NULL;
		s_json* directional_light_json = se_sdf_json_add_object(directional_lights_json, NULL);
		if (!directional_light_ptr || !directional_light_json ||
			!se_sdf_json_add_vec3(directional_light_json, "direction", &directional_light_ptr->direction) ||
			!se_sdf_json_add_vec3(directional_light_json, "color", &directional_light_ptr->color)) {
			se_set_last_error(directional_light_ptr ? SE_RESULT_OUT_OF_MEMORY : SE_RESULT_NOT_FOUND);
			return false;
		}
	}

	if (s_array_get_size(&sdf_ptr->children) > 0u) {
		children_json = se_sdf_json_add_array(object, "children");
		if (!children_json) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
	}

	for (u32 i = 0; i < (u32)s_array_get_size(&sdf_ptr->children); ++i) {
		const s_handle local_handle = s_array_handle_at(&sdf_ptr->children.b, i);
		se_sdf_handle* child_handle_ptr = s_array_get(&sdf_ptr->children, local_handle);
		s_json* child_json = child_handle_ptr ? se_sdf_json_add_object(children_json, NULL) : NULL;
		if (!child_handle_ptr || !child_json || !se_sdf_json_populate_object(ctx, *child_handle_ptr, child_json)) {
			if (se_get_last_error() == SE_RESULT_OK) {
				se_set_last_error(child_handle_ptr ? SE_RESULT_OUT_OF_MEMORY : SE_RESULT_NOT_FOUND);
			}
			return false;
		}
	}

	return true;
}

static se_sdf_handle se_sdf_build_from_json_node(const s_json* node) {
	se_context* ctx = se_current_context();
	se_sdf_handle sdf = S_HANDLE_NULL;
	if (!ctx || !node || node->type != S_JSON_OBJECT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	const s_json* transform_json = s_json_get(node, "transform");
	const s_json* type_json = s_json_get(node, "type");
	const s_json* operation_json = s_json_get(node, "operation");
	const s_json* operation_amount_json = s_json_get(node, "operation_amount");
	const s_json* shading_json = s_json_get(node, "shading");
	const s_json* shadow_json = s_json_get(node, "shadow");
	const s_json* noises_json = s_json_get(node, "noises");
	const s_json* point_lights_json = s_json_get(node, "point_lights");
	const s_json* directional_lights_json = s_json_get(node, "directional_lights");
	const s_json* children_json = s_json_get(node, "children");
	s_mat4 transform = s_mat4_identity;
	se_sdf_type type = SE_SDF_CUSTOM;
	se_sdf_operator operation = SE_SDF_UNION;
	f32 operation_amount = 0.0f;
	se_sdf_shading shading = {0};
	se_sdf_shadow shadow = {0};
	f32 sphere_radius = 0.0f;
	s_vec3 box_size = s_vec3(0.0f, 0.0f, 0.0f);
	u32 shadow_samples = 0u;

	if (!type_json || !se_sdf_json_read_type(type_json, &type)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	if (transform_json) {
		if (transform_json->type != S_JSON_ARRAY || !se_sdf_json_read_mat4(transform_json, &transform)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return S_HANDLE_NULL;
		}
	}

	if (operation_json && !se_sdf_json_read_operation(operation_json, &operation)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (operation_amount_json && !se_sdf_json_number_to_f32(operation_amount_json, &operation_amount)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	if (shading_json) {
		if (shading_json->type != S_JSON_OBJECT ||
			!se_sdf_json_read_vec3(se_sdf_json_get_required(shading_json, "ambient", S_JSON_ARRAY), &shading.ambient) ||
			!se_sdf_json_read_vec3(se_sdf_json_get_required(shading_json, "diffuse", S_JSON_ARRAY), &shading.diffuse) ||
			!se_sdf_json_read_vec3(se_sdf_json_get_required(shading_json, "specular", S_JSON_ARRAY), &shading.specular) ||
			!se_sdf_json_number_to_f32(se_sdf_json_get_required(shading_json, "roughness", S_JSON_NUMBER), &shading.roughness) ||
			!se_sdf_json_number_to_f32(se_sdf_json_get_required(shading_json, "bias", S_JSON_NUMBER), &shading.bias) ||
			!se_sdf_json_number_to_f32(se_sdf_json_get_required(shading_json, "smoothness", S_JSON_NUMBER), &shading.smoothness)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return S_HANDLE_NULL;
		}
	}

	if (shadow_json) {
		if (shadow_json->type != S_JSON_OBJECT ||
			!se_sdf_json_number_to_f32(se_sdf_json_get_required(shadow_json, "softness", S_JSON_NUMBER), &shadow.softness) ||
			!se_sdf_json_number_to_f32(se_sdf_json_get_required(shadow_json, "bias", S_JSON_NUMBER), &shadow.bias) ||
			!se_sdf_json_number_to_u32(se_sdf_json_get_required(shadow_json, "samples", S_JSON_NUMBER), &shadow_samples) ||
			shadow_samples > (u32)UINT16_MAX) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return S_HANDLE_NULL;
		}
		shadow.samples = (u16)shadow_samples;
	}

	if ((noises_json && noises_json->type != S_JSON_ARRAY) ||
		(point_lights_json && point_lights_json->type != S_JSON_ARRAY) ||
		(directional_lights_json && directional_lights_json->type != S_JSON_ARRAY) ||
		(children_json && children_json->type != S_JSON_ARRAY)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}

	if (type == SE_SDF_SPHERE) {
		const s_json* sphere_json = se_sdf_json_get_required(node, "sphere", S_JSON_OBJECT);
		if (!sphere_json || !se_sdf_json_number_to_f32(se_sdf_json_get_required(sphere_json, "radius", S_JSON_NUMBER), &sphere_radius)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return S_HANDLE_NULL;
		}
	} else if (type == SE_SDF_BOX) {
		const s_json* box_json = se_sdf_json_get_required(node, "box", S_JSON_OBJECT);
		if (!box_json || !se_sdf_json_read_vec3(se_sdf_json_get_required(box_json, "size", S_JSON_ARRAY), &box_size)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return S_HANDLE_NULL;
		}
	}

	se_sdf descriptor = {
		.transform = transform,
		.type = type,
		.operation = operation,
		.operation_amount = operation_amount,
		.shading = shading,
		.shadow = shadow,
	};
	if (type == SE_SDF_SPHERE) {
		descriptor.sphere.radius = sphere_radius;
	} else if (type == SE_SDF_BOX) {
		descriptor.box.size = box_size;
	}
	sdf = se_sdf_create_internal(&descriptor);
	if (sdf == S_HANDLE_NULL) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		}
		return S_HANDLE_NULL;
	}

	for (sz i = 0; noises_json && i < noises_json->as.children.count; ++i) {
		const s_json* noise_json = s_json_at(noises_json, i);
		const s_json* descriptor_json = se_sdf_json_get_required(noise_json, "descriptor", S_JSON_OBJECT);
		se_noise_2d noise_descriptor = {0};
		if (!noise_json || !descriptor_json ||
			!se_sdf_json_read_noise_type(s_json_get(descriptor_json, "type"), &noise_descriptor.type) ||
			!se_sdf_json_number_to_f32(se_sdf_json_get_required(descriptor_json, "frequency", S_JSON_NUMBER), &noise_descriptor.frequency) ||
			!se_sdf_json_read_vec2(se_sdf_json_get_required(descriptor_json, "offset", S_JSON_ARRAY), &noise_descriptor.offset) ||
			!se_sdf_json_read_vec2(se_sdf_json_get_required(descriptor_json, "scale", S_JSON_ARRAY), &noise_descriptor.scale) ||
			!se_sdf_json_number_to_u32(se_sdf_json_get_required(descriptor_json, "seed", S_JSON_NUMBER), &noise_descriptor.seed)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			goto fail;
		}

		se_sdf_noise_handle noise_handle = se_sdf_add_noise_internal(sdf, &noise_descriptor);
		if (noise_handle == S_HANDLE_NULL) {
			if (se_get_last_error() == SE_RESULT_OK) {
				se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			}
			goto fail;
		}

		const s_json* texture_json = s_json_get(noise_json, "texture");
		if (texture_json) {
			u32 width = 0u;
			u32 height = 0u;
			se_texture_wrap wrap = SE_REPEAT;
			const c8* pixels_b64 = NULL;
			u8* pixels = NULL;
			sz pixels_size = 0u;
			if (texture_json->type != S_JSON_OBJECT ||
				!se_sdf_json_read_texture_wrap(s_json_get(texture_json, "wrap"), &wrap) ||
				!se_sdf_json_number_to_u32(se_sdf_json_get_required(texture_json, "width", S_JSON_NUMBER), &width) ||
				!se_sdf_json_number_to_u32(se_sdf_json_get_required(texture_json, "height", S_JSON_NUMBER), &height) ||
				!se_sdf_json_string_to_c8(se_sdf_json_get_required(texture_json, "pixels_rgba_b64", S_JSON_STRING), &pixels_b64) ||
				width == 0u || height == 0u ||
				!se_sdf_base64_decode(pixels_b64, &pixels, &pixels_size) ||
				pixels_size != ((sz)width * (sz)height * 4u) ||
				!se_sdf_json_apply_texture_pixels(ctx, se_sdf_get_noise_texture(noise_handle), pixels, (i32)width, (i32)height, wrap)) {
				free(pixels);
				if (se_get_last_error() == SE_RESULT_OK) {
					se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
				}
				goto fail;
			}
			free(pixels);
		}
	}

	for (sz i = 0; point_lights_json && i < point_lights_json->as.children.count; ++i) {
		const s_json* point_light_json = s_json_at(point_lights_json, i);
		se_sdf_point_light point_light = {0};
		if (!point_light_json ||
			!se_sdf_json_read_vec3(se_sdf_json_get_required(point_light_json, "position", S_JSON_ARRAY), &point_light.position) ||
			!se_sdf_json_read_vec3(se_sdf_json_get_required(point_light_json, "color", S_JSON_ARRAY), &point_light.color) ||
			!se_sdf_json_number_to_f32(se_sdf_json_get_required(point_light_json, "radius", S_JSON_NUMBER), &point_light.radius) ||
			se_sdf_add_point_light_internal(sdf, point_light) == S_HANDLE_NULL) {
			if (se_get_last_error() == SE_RESULT_OK) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			}
			goto fail;
		}
	}

	for (sz i = 0; directional_lights_json && i < directional_lights_json->as.children.count; ++i) {
		const s_json* directional_light_json = s_json_at(directional_lights_json, i);
		se_sdf_directional_light directional_light = {0};
		if (!directional_light_json ||
			!se_sdf_json_read_vec3(se_sdf_json_get_required(directional_light_json, "direction", S_JSON_ARRAY), &directional_light.direction) ||
			!se_sdf_json_read_vec3(se_sdf_json_get_required(directional_light_json, "color", S_JSON_ARRAY), &directional_light.color) ||
			se_sdf_add_directional_light_internal(sdf, directional_light) == S_HANDLE_NULL) {
			if (se_get_last_error() == SE_RESULT_OK) {
				se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			}
			goto fail;
		}
	}

	for (sz i = 0; children_json && i < children_json->as.children.count; ++i) {
		const s_json* child_json = s_json_at(children_json, i);
		se_sdf_handle child = se_sdf_build_from_json_node(child_json);
		if (child == S_HANDLE_NULL) {
			goto fail;
		}
		se_sdf_add_child(sdf, child);
	}

	return sdf;

fail:
	if (sdf != S_HANDLE_NULL) {
		se_sdf_destroy(sdf);
	}
	return S_HANDLE_NULL;
}

static void se_sdf_destroy_shader_runtime(se_sdf* sdf_ptr) {
	if (!sdf_ptr) {
		return;
	}
	if (sdf_ptr->shader != S_HANDLE_NULL) {
		se_shader_destroy(sdf_ptr->shader);
		sdf_ptr->shader = S_HANDLE_NULL;
	}
}

static void se_sdf_destroy_runtime_resources(se_context* ctx, se_sdf* sdf_ptr) {
	if (!ctx || !sdf_ptr) {
		return;
	}
	if (sdf_ptr->output != S_HANDLE_NULL) {
		se_framebuffer_destroy(sdf_ptr->output);
		sdf_ptr->output = S_HANDLE_NULL;
	}
	if (sdf_ptr->volume != S_HANDLE_NULL) {
		se_texture_destroy(sdf_ptr->volume);
		sdf_ptr->volume = S_HANDLE_NULL;
	}
	se_sdf_destroy_shader_runtime(sdf_ptr);
	se_quad_destroy(&sdf_ptr->quad);
	memset(&sdf_ptr->quad, 0, sizeof(sdf_ptr->quad));
}

static void se_sdf_reset_keep_handle(se_context* ctx, const se_sdf_handle sdf) {
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!ctx || !sdf_ptr) {
		return;
	}
	const se_sdf_handle parent = sdf_ptr->parent;

	while (s_array_get_size(&sdf_ptr->children) > 0) {
		const s_handle local_child_handle = s_array_handle(&sdf_ptr->children, (u32)(s_array_get_size(&sdf_ptr->children) - 1u));
		se_sdf_handle* child_handle_ptr = s_array_get(&sdf_ptr->children, local_child_handle);
		if (!child_handle_ptr || *child_handle_ptr == S_HANDLE_NULL || *child_handle_ptr == sdf) {
			s_array_remove(&sdf_ptr->children, local_child_handle);
			continue;
		}
		se_sdf_destroy(*child_handle_ptr);
	}

	se_sdf_release_noise_references(ctx, sdf_ptr);
	se_sdf_release_point_light_references(ctx, sdf_ptr);
	se_sdf_release_directional_light_references(ctx, sdf_ptr);
	se_sdf_destroy_runtime_resources(ctx, sdf_ptr);
	s_array_clear(&sdf_ptr->children);
	s_array_clear(&sdf_ptr->noises);
	s_array_clear(&sdf_ptr->point_lights);
	s_array_clear(&sdf_ptr->directional_lights);
	s_array_init(&sdf_ptr->children);
	s_array_init(&sdf_ptr->noises);
	s_array_init(&sdf_ptr->point_lights);
	s_array_init(&sdf_ptr->directional_lights);
	sdf_ptr->transform = s_mat4_identity;
	sdf_ptr->type = SE_SDF_CUSTOM;
	sdf_ptr->operation = SE_SDF_UNION;
	sdf_ptr->operation_amount = 0.0f;
	sdf_ptr->shading = (se_sdf_shading){0};
	sdf_ptr->shadow = (se_sdf_shadow){0};
	sdf_ptr->sphere.radius = 0.0f;
	sdf_ptr->parent = parent;
}

static b8 se_sdf_transfer_state(se_context* ctx, const se_sdf_handle dst, const se_sdf_handle src) {
	se_sdf* dst_ptr = se_sdf_from_handle(ctx, dst);
	se_sdf* src_ptr = se_sdf_from_handle(ctx, src);
	if (!ctx || !dst_ptr || !src_ptr || dst == src) {
		return false;
	}

	const se_sdf_handle preserved_parent = dst_ptr->parent;
	se_sdf_reset_keep_handle(ctx, dst);
	dst_ptr = se_sdf_from_handle(ctx, dst);
	src_ptr = se_sdf_from_handle(ctx, src);
	if (!dst_ptr || !src_ptr) {
		return false;
	}

	*dst_ptr = *src_ptr;
	dst_ptr->parent = preserved_parent;
	for (u32 i = 0; i < (u32)s_array_get_size(&dst_ptr->children); ++i) {
		const s_handle local_handle = s_array_handle_at(&dst_ptr->children.b, i);
		se_sdf_handle* child_handle_ptr = s_array_get(&dst_ptr->children, local_handle);
		se_sdf* child_ptr = child_handle_ptr ? se_sdf_from_handle(ctx, *child_handle_ptr) : NULL;
		if (child_ptr) {
			child_ptr->parent = dst;
		}
	}

	memset(src_ptr, 0, sizeof(*src_ptr));
	s_array_remove(&ctx->sdfs, src);
	se_sdf_invalidate_shader_chain(dst);
	return true;
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

static void se_sdf_invalidate_all_shaders(se_context* ctx) {
	se_sdf* sdf_ptr = NULL;
	if (!ctx) {
		return;
	}
	s_foreach(&ctx->sdfs, sdf_ptr) {
		se_sdf_destroy_shader_runtime(sdf_ptr);
	}
}

static b8 se_sdf_read_resource_text(c8** out_text, const c8* resource_path) {
	c8 absolute_path[SE_MAX_PATH_LENGTH] = {0};
	if (!out_text || !resource_path) {
		return false;
	}
	*out_text = NULL;
	if (!se_paths_resolve_resource_path(absolute_path, sizeof(absolute_path), resource_path)) {
		se_log("se_sdf :: failed to resolve shader template path: %s", resource_path);
		return false;
	}
	if (!se_resource_read_text_file(absolute_path, out_text, NULL)) {
		se_log("se_sdf :: failed to read shader template: %s", absolute_path);
		return false;
	}
	return true;
}

static b8 se_sdf_read_resource_mtime(time_t* out_mtime, const c8* resource_path) {
	c8 absolute_path[SE_MAX_PATH_LENGTH] = {0};
	if (!out_mtime || !resource_path) {
		return false;
	}
	*out_mtime = 0;
	if (!se_paths_resolve_resource_path(absolute_path, sizeof(absolute_path), resource_path)) {
		se_log("se_sdf :: failed to resolve shader template path: %s", resource_path);
		return false;
	}
	if (!se_resource_file_mtime(absolute_path, out_mtime)) {
		se_log("se_sdf :: failed to stat shader template: %s", absolute_path);
		return false;
	}
	return true;
}

static void se_sdf_refresh_shader_templates(se_context* ctx) {
	time_t vertex_mtime = 0;
	time_t common_mtime = 0;
	time_t fragment_mtime = 0;
	if (!ctx) {
		return;
	}
	if (!se_sdf_read_resource_mtime(&vertex_mtime, SE_SDF_VERTEX_SHADER_TEMPLATE_PATH)
		|| !se_sdf_read_resource_mtime(&common_mtime, SE_SDF_COMMON_SHADER_TEMPLATE_PATH)
		|| !se_sdf_read_resource_mtime(&fragment_mtime, SE_SDF_FRAGMENT_SHADER_TEMPLATE_PATH)) {
		return;
	}
	if (!g_se_sdf_shader_templates.initialized) {
		g_se_sdf_shader_templates.vertex_mtime = vertex_mtime;
		g_se_sdf_shader_templates.common_mtime = common_mtime;
		g_se_sdf_shader_templates.fragment_mtime = fragment_mtime;
		g_se_sdf_shader_templates.initialized = true;
		return;
	}
	if (g_se_sdf_shader_templates.vertex_mtime == vertex_mtime
		&& g_se_sdf_shader_templates.common_mtime == common_mtime
		&& g_se_sdf_shader_templates.fragment_mtime == fragment_mtime) {
		return;
	}
	g_se_sdf_shader_templates.vertex_mtime = vertex_mtime;
	g_se_sdf_shader_templates.common_mtime = common_mtime;
	g_se_sdf_shader_templates.fragment_mtime = fragment_mtime;
	se_sdf_invalidate_all_shaders(ctx);
}

static const c8* se_sdf_find_template_value(const se_sdf_template_part* parts, const sz part_count, const c8* token, const sz token_size) {
	if (!parts || !token || token_size == 0) {
		return NULL;
	}
	for (sz i = 0; i < part_count; ++i) {
		const c8* current_token = parts[i].token;
		if (!current_token) {
			continue;
		}
		if (strlen(current_token) == token_size && strncmp(current_token, token, token_size) == 0) {
			return parts[i].value ? parts[i].value : "";
		}
	}
	return NULL;
}

static void se_sdf_apply_template(c8* out, const sz capacity, const c8* template_source, const se_sdf_template_part* parts, const sz part_count) {
	const c8* cursor = template_source;
	if (!out || capacity == 0 || !template_source) {
		return;
	}
	out[0] = '\0';
	while (cursor && *cursor != '\0') {
		const c8* token_start = strstr(cursor, "{{");
		if (!token_start) {
			se_sdf_append_range(out, capacity, cursor, strlen(cursor));
			break;
		}
		se_sdf_append_range(out, capacity, cursor, (sz)(token_start - cursor));
		const c8* token_end = strstr(token_start, "}}");
		if (!token_end) {
			se_sdf_append_range(out, capacity, token_start, strlen(token_start));
			break;
		}
		token_end += 2;
		const c8* replacement = se_sdf_find_template_value(parts, part_count, token_start, (sz)(token_end - token_start));
		if (replacement) {
			se_sdf_append_range(out, capacity, replacement, strlen(replacement));
		} else {
			se_sdf_append_range(out, capacity, token_start, (sz)(token_end - token_start));
		}
		cursor = token_end;
	}
}

static void se_sdf_gen_common_constants(c8* out, const sz capacity) {
	if (!out || capacity == 0) {
		return;
	}
	out[0] = '\0';
	se_sdf_append(out, capacity, "const float SDF_FAR_DISTANCE = %.1ff;\n", SE_SDF_FAR_DISTANCE);
	se_sdf_append(out, capacity, "const int SDF_MAX_TRACE_STEPS = %u;\n", SE_SDF_MAX_TRACE_STEPS);
	se_sdf_append(out, capacity, "const float SDF_HIT_EPSILON = %.4ff;\n", SE_SDF_HIT_EPSILON);
	se_sdf_append(out, capacity, "const float SDF_MIN_SMOOTH_UNION_AMOUNT = %.4ff;\n", SE_SDF_MIN_SMOOTH_UNION_AMOUNT);
	se_sdf_append(out, capacity, "const float SDF_DEFAULT_NOISE_AMOUNT = %.3ff;\n", SE_SDF_DEFAULT_NOISE_AMOUNT);
	se_sdf_append(out, capacity, "const int SDF_MAX_SHADOW_SAMPLES = %u;\n", SE_SDF_MAX_SHADOW_SAMPLES);
	se_sdf_append(
		out,
		capacity,
		"const vec3 SDF_DEFAULT_LIGHT_DIRECTION = vec3(%.2f, %.2f, %.2f);\n\n",
		SE_SDF_DEFAULT_LIGHT_DIR_X,
		SE_SDF_DEFAULT_LIGHT_DIR_Y,
		SE_SDF_DEFAULT_LIGHT_DIR_Z);
}

static void se_sdf_gen_uniform_recursive(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	se_texture_handle* noise_handle_ptr = NULL;
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
		const se_texture_handle noise_id = *noise_handle_ptr;
		const se_sdf_noise* noise = se_sdf_noise_from_handle(ctx, (se_sdf_noise_handle)noise_id);
		if (!se_sdf_noise_is_active(noise)) {
			continue;
		}
		se_sdf_append(
			out,
			capacity,
			"uniform sampler2D _noise_" SE_SDF_HANDLE_FMT "_texture;\n",
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
	se_texture_handle* noise_handle_ptr = NULL;
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
	se_sdf_append(out, capacity, "float sdf_" SE_SDF_HANDLE_FMT "(vec3 p) {\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "\tfloat d = SDF_FAR_DISTANCE;\n");
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
			"\td = %s(d, sdf_" SE_SDF_HANDLE_FMT "(p), _" SE_SDF_HANDLE_FMT "_operation_amount);\n",
			operation_function,
			SE_SDF_HANDLE_ARG(*child),
			SE_SDF_HANDLE_ARG(sdf));
	}
	if (se_sdf_has_noise(sdf_ptr)) {
		se_sdf_append(
			out,
			capacity,
			"\tvec3 noise_local = (_" SE_SDF_HANDLE_FMT "_inv_transform * vec4(p, 1.0)).xyz;\n",
			SE_SDF_HANDLE_ARG(sdf));
		s_foreach(&sdf_ptr->noises, noise_handle_ptr) {
			const se_texture_handle noise_id = *noise_handle_ptr;
			if (!se_sdf_texture_from_handle(ctx, noise_id)) {
				continue;
			}
			se_sdf_append(
				out,
				capacity,
				"\t\td += sdf_noise_texture_triplanar(_noise_" SE_SDF_HANDLE_FMT "_texture, noise_local) * SDF_DEFAULT_NOISE_AMOUNT;\n",
				SE_SDF_HANDLE_ARG(noise_id));
		}
	}
	se_sdf_append(out, capacity, "\treturn d;\n}\n\n");
}

static void se_sdf_gen_surface_recursive(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	se_texture_handle* noise_handle_ptr = NULL;
	if (!sdf_ptr) {
		return;
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_gen_surface_recursive(out, capacity, *child);
	}
	se_sdf_append(
		out,
		capacity,
		"sdf_surface_data sdf_" SE_SDF_HANDLE_FMT "_surface(vec3 p) {\n"
		"\tsdf_surface_data surface;\n"
		"\tsurface.distance = SDF_FAR_DISTANCE;\n"
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
					"\t\tsdf_surface_data child_surface = sdf_" SE_SDF_HANDLE_FMT "_surface(p);\n"
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
					"\t\tsdf_surface_data child_surface = sdf_" SE_SDF_HANDLE_FMT "_surface(p);\n"
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
			"\tvec3 noise_local = (_" SE_SDF_HANDLE_FMT "_inv_transform * vec4(p, 1.0)).xyz;\n",
			SE_SDF_HANDLE_ARG(sdf));
		s_foreach(&sdf_ptr->noises, noise_handle_ptr) {
			const se_texture_handle noise_id = *noise_handle_ptr;
			if (!se_sdf_texture_from_handle(ctx, noise_id)) {
				continue;
			}
			se_sdf_append(
				out,
				capacity,
				"\t\tsurface.distance += sdf_noise_texture_triplanar(_noise_" SE_SDF_HANDLE_FMT "_texture, noise_local) * SDF_DEFAULT_NOISE_AMOUNT;\n",
				SE_SDF_HANDLE_ARG(noise_id));
		}
	}
	se_sdf_append(out, capacity, "\treturn surface;\n}\n\n");
}

static void se_sdf_gen_composite_light_apply_recursive(c8* out, const sz capacity, const se_sdf_handle sdf, const se_sdf_directional_light_handle shadow_light, b8* shadow_light_used) {
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
		const b8 use_shadow = shadow_light_used && directional_light_id == shadow_light && !(*shadow_light_used);
		if (!se_sdf_directional_light_from_handle(ctx, directional_light_id)) {
			continue;
		}
		se_sdf_append(
			out,
			capacity,
			"\t{\n"
			"\t\tvec3 light_diffuse = vec3(0.0);\n"
			"\t\tvec3 light_specular = vec3(0.0);\n"
			"\t\tsdf_apply_directional_light_visibility(normal, view_direction, _directional_light_" SE_SDF_HANDLE_FMT "_direction, _directional_light_" SE_SDF_HANDLE_FMT "_color, shading.roughness, shading.bias, shading.smoothness, %s, light_diffuse, light_specular);\n"
			"\t\tdiffuse_lighting += light_diffuse;\n"
			"\t\tspecular_lighting += light_specular;\n"
			"\t}\n",
			SE_SDF_HANDLE_ARG(directional_light_id),
			SE_SDF_HANDLE_ARG(directional_light_id),
			use_shadow ? "shadow_visibility" : "1.0");
		if (use_shadow && shadow_light_used) {
			*shadow_light_used = true;
		}
	}
	s_foreach(&sdf_ptr->point_lights, point_light_handle_ptr) {
		const se_sdf_point_light_handle point_light_id = *point_light_handle_ptr;
		if (!se_sdf_point_light_from_handle(ctx, point_light_id)) {
			continue;
		}
		se_sdf_append(
			out,
			capacity,
			"\t{\n"
			"\t\tvec3 light_diffuse = vec3(0.0);\n"
			"\t\tvec3 light_specular = vec3(0.0);\n"
			"\t\tsdf_apply_point_light_visibility(normal, view_direction, hit_position, _point_light_" SE_SDF_HANDLE_FMT "_position, _point_light_" SE_SDF_HANDLE_FMT "_color, _point_light_" SE_SDF_HANDLE_FMT "_radius, shading.roughness, shading.bias, shading.smoothness, 1.0, light_diffuse, light_specular);\n"
			"\t\tdiffuse_lighting += light_diffuse;\n"
			"\t\tspecular_lighting += light_specular;\n"
			"\t}\n",
			SE_SDF_HANDLE_ARG(point_light_id),
			SE_SDF_HANDLE_ARG(point_light_id),
			SE_SDF_HANDLE_ARG(point_light_id));
	}
	s_foreach(&sdf_ptr->children, child) {
		se_sdf_gen_composite_light_apply_recursive(out, capacity, *child, shadow_light, shadow_light_used);
	}
}

static void se_sdf_gen_scene_wrappers(c8* out, const sz capacity, const se_sdf_handle sdf) {
	if (!out || capacity == 0) {
		return;
	}
	out[0] = '\0';
	se_sdf_append(out, capacity, "float scene_sdf(vec3 p) {\n");
	se_sdf_append(out, capacity, "\treturn sdf_" SE_SDF_HANDLE_FMT "(p);\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "}\n\n");
	se_sdf_append(out, capacity, "sdf_surface_data scene_surface(vec3 p) {\n");
	se_sdf_append(out, capacity, "\treturn sdf_" SE_SDF_HANDLE_FMT "_surface(p);\n", SE_SDF_HANDLE_ARG(sdf));
	se_sdf_append(out, capacity, "}\n\n");
}

static void se_sdf_gen_light_apply(c8* out, const sz capacity, const se_sdf_handle sdf) {
	const se_sdf_directional_light_handle shadow_light = se_sdf_get_first_directional_light_recursive(sdf);
	b8 shadow_light_used = false;
	if (!out || capacity == 0) {
		return;
	}
	out[0] = '\0';
	if (shadow_light != S_HANDLE_NULL) {
		se_sdf_append(
			out,
			capacity,
			"\tshadow_visibility = sdf_shadow_visibility(hit_position + normal * max(surface.shadow.bias, 0.0005), normalize(_directional_light_" SE_SDF_HANDLE_FMT "_direction), max(surface.shadow.bias * 2.0, SDF_HIT_EPSILON), 32.0, surface.shadow.softness, int(max(round(surface.shadow.samples), 1.0)));\n",
			SE_SDF_HANDLE_ARG(shadow_light));
	}
	if (se_sdf_has_lights_recursive(sdf)) {
		se_sdf_gen_composite_light_apply_recursive(out, capacity, sdf, shadow_light, &shadow_light_used);
	} else {
		se_sdf_append(
			out,
			capacity,
			"\t{\n"
			"\t\tvec3 light_diffuse = vec3(0.0);\n"
			"\t\tvec3 light_specular = vec3(0.0);\n"
			"\t\tsdf_apply_directional_light_visibility(normal, view_direction, SDF_DEFAULT_LIGHT_DIRECTION, vec3(1.0), shading.roughness, shading.bias, shading.smoothness, 1.0, light_diffuse, light_specular);\n"
			"\t\tdiffuse_lighting += light_diffuse;\n"
			"\t\tspecular_lighting += light_specular;\n"
			"\t}\n");
	}
}

static void se_sdf_gen_uniform(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_sdf_append(out, capacity, "uniform mat4 u_inv_view_projection;\n");
	se_sdf_append(out, capacity, "uniform vec3 u_camera_position;\n");
	se_sdf_append(out, capacity, "uniform int u_use_orthographic;\n");
	se_sdf_gen_uniform_recursive(out, capacity, sdf);
}

static void se_sdf_gen_function(c8* out, const sz capacity, const se_sdf_handle sdf) {
	se_sdf_gen_function_recursive(out, capacity, sdf);
}

static b8 se_sdf_gen_fragment(c8* out, const sz capacity, const se_sdf_handle sdf) {
	c8* common_template = NULL;
	c8* fragment_template = NULL;
	c8* uniforms = NULL;
	c8* common_constants = NULL;
	c8* distance_functions = NULL;
	c8* surface_functions = NULL;
	c8* scene_wrappers = NULL;
	c8* light_apply = NULL;
	b8 success = false;
	if (!out || capacity == 0) {
		return false;
	}
	out[0] = '\0';
	if (!se_sdf_read_resource_text(&common_template, SE_SDF_COMMON_SHADER_TEMPLATE_PATH)
		|| !se_sdf_read_resource_text(&fragment_template, SE_SDF_FRAGMENT_SHADER_TEMPLATE_PATH)) {
		goto cleanup;
	}
	uniforms = calloc(SE_SDF_FRAGMENT_SHADER_CAPACITY, sizeof(c8));
	common_constants = calloc(2048u, sizeof(c8));
	distance_functions = calloc(SE_SDF_FRAGMENT_SHADER_CAPACITY, sizeof(c8));
	surface_functions = calloc(SE_SDF_FRAGMENT_SHADER_CAPACITY, sizeof(c8));
	scene_wrappers = calloc(2048u, sizeof(c8));
	light_apply = calloc(SE_SDF_FRAGMENT_SHADER_CAPACITY, sizeof(c8));
	if (!uniforms || !common_constants || !distance_functions || !surface_functions || !scene_wrappers || !light_apply) {
		goto cleanup;
	}
	se_sdf_gen_uniform(uniforms, SE_SDF_FRAGMENT_SHADER_CAPACITY, sdf);
	se_sdf_gen_common_constants(common_constants, 2048u);
	se_sdf_gen_function(distance_functions, SE_SDF_FRAGMENT_SHADER_CAPACITY, sdf);
	se_sdf_gen_surface_recursive(surface_functions, SE_SDF_FRAGMENT_SHADER_CAPACITY, sdf);
	se_sdf_gen_scene_wrappers(scene_wrappers, 2048u, sdf);
	se_sdf_gen_light_apply(light_apply, SE_SDF_FRAGMENT_SHADER_CAPACITY, sdf);
	const se_sdf_template_part parts[] = {
		{ "{{SDF_UNIFORMS}}", uniforms },
		{ "{{SDF_COMMON_CONSTANTS}}", common_constants },
		{ "{{SDF_COMMON}}", common_template },
		{ "{{SDF_DISTANCE_FUNCTIONS}}", distance_functions },
		{ "{{SDF_SURFACE_FUNCTIONS}}", surface_functions },
		{ "{{SDF_SCENE_WRAPPERS}}", scene_wrappers },
		{ "{{SDF_LIGHT_APPLY}}", light_apply },
	};
	se_sdf_apply_template(out, capacity, fragment_template, parts, sizeof(parts) / sizeof(parts[0]));
	success = true;

cleanup:
	free(common_template);
	free(fragment_template);
	free(uniforms);
	free(common_constants);
	free(distance_functions);
	free(surface_functions);
	free(scene_wrappers);
	free(light_apply);
	return success;
}

static void se_sdf_upload_uniforms_recursive(const se_shader_handle shader, const se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_sdf_handle* child = NULL;
	se_texture_handle* noise_handle_ptr = NULL;
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
		const se_texture_handle noise_id = *noise_handle_ptr;
		const se_sdf_noise* noise = se_sdf_noise_from_handle(ctx, (se_sdf_noise_handle)noise_id);
		const se_texture* texture = se_sdf_texture_from_handle(ctx, noise_id);
		if (!se_sdf_noise_is_active(noise)) {
			continue;
		}
		if (texture && texture->id != 0u) {
			snprintf(uniform_name, sizeof(uniform_name), "_noise_" SE_SDF_HANDLE_FMT "_texture", SE_SDF_HANDLE_ARG(noise_id));
			se_shader_set_texture(shader, uniform_name, texture->id);
		}
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

	se_sdf_destroy_runtime_resources(ctx, sdf_ptr);
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

b8 se_sdf_get_children(se_sdf_handle sdf, const se_sdf_handle** out_children, sz* out_count) {
	se_context* ctx = se_current_context();
	const se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!out_children || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_children = NULL;
	*out_count = 0u;
	if (!ctx || sdf == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!sdf_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_children = (const se_sdf_handle*)sdf_ptr->children.b.data;
	*out_count = s_array_get_size(&sdf_ptr->children);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_sdf_get_noises(se_sdf_handle sdf, const se_sdf_noise_handle** out_noises, sz* out_count) {
	se_context* ctx = se_current_context();
	const se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!out_noises || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_noises = NULL;
	*out_count = 0u;
	if (!ctx || sdf == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!sdf_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_noises = (const se_sdf_noise_handle*)sdf_ptr->noises.b.data;
	*out_count = s_array_get_size(&sdf_ptr->noises);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_sdf_get_point_lights(se_sdf_handle sdf, const se_sdf_point_light_handle** out_point_lights, sz* out_count) {
	se_context* ctx = se_current_context();
	const se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!out_point_lights || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_point_lights = NULL;
	*out_count = 0u;
	if (!ctx || sdf == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!sdf_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_point_lights = (const se_sdf_point_light_handle*)sdf_ptr->point_lights.b.data;
	*out_count = s_array_get_size(&sdf_ptr->point_lights);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_sdf_get_directional_lights(se_sdf_handle sdf, const se_sdf_directional_light_handle** out_directional_lights, sz* out_count) {
	se_context* ctx = se_current_context();
	const se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!out_directional_lights || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_directional_lights = NULL;
	*out_count = 0u;
	if (!ctx || sdf == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!sdf_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	*out_directional_lights = (const se_sdf_directional_light_handle*)sdf_ptr->directional_lights.b.data;
	*out_count = s_array_get_size(&sdf_ptr->directional_lights);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

se_sdf_noise_handle se_sdf_add_noise_internal(se_sdf_handle sdf, const se_noise_2d* noise) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	se_texture_handle new_noise = S_HANDLE_NULL;
	se_sdf_noise* noise_ptr = NULL;
	se_noise_2d descriptor = {0};
	if (!ctx || !sdf_ptr || !noise) {
		return S_HANDLE_NULL;
	}
	descriptor = *noise;
	if (descriptor.frequency <= 0.0f) {
		descriptor.frequency = 1.0f;
	}
	new_noise = se_noise_2d_create(ctx, &descriptor);
	if (new_noise == S_HANDLE_NULL) {
		return S_HANDLE_NULL;
	}
	noise_ptr = s_array_get(&ctx->sdf_noises, s_array_add(&ctx->sdf_noises, ((se_sdf_noise){
		.texture = new_noise,
		.descriptor = descriptor,
	})));
	if (!noise_ptr) {
		se_noise_2d_destroy(ctx, new_noise);
		return S_HANDLE_NULL;
	}
	s_array_add(&sdf_ptr->noises, new_noise);
	se_sdf_invalidate_shader_chain(sdf);
	return (se_sdf_noise_handle)new_noise;
}

f32 se_sdf_get_noise_frequency(se_sdf_noise_handle noise) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return 0.0f;
	}
	return se_sdf_get_noise_frequency_defaulted(noise_ptr);
}

void se_sdf_noise_set_frequency(se_sdf_noise_handle noise, f32 frequency) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return;
	}
	noise_ptr->descriptor.frequency = frequency;
	noise_ptr->descriptor.frequency = se_sdf_get_noise_frequency_defaulted(noise_ptr);
	se_noise_update(ctx, noise_ptr->texture, &noise_ptr->descriptor);
}

s_vec3 se_sdf_get_noise_offset(se_sdf_noise_handle noise) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	return se_sdf_get_noise_offset_defaulted(noise_ptr);
}

void se_sdf_noise_set_offset(se_sdf_noise_handle noise, const s_vec3* offset) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr || !offset) {
		return;
	}
	noise_ptr->descriptor.offset = s_vec2(offset->x, offset->y);
	se_noise_update(ctx, noise_ptr->texture, &noise_ptr->descriptor);
}

s_vec3 se_sdf_get_noise_scale(se_sdf_noise_handle noise) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return s_vec3(0.0f, 0.0f, 0.0f);
	}
	return s_vec3(noise_ptr->descriptor.scale.x, noise_ptr->descriptor.scale.y, 0.0f);
}

void se_sdf_noise_set_scale(se_sdf_noise_handle noise, const s_vec3* scale) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr || !scale) {
		return;
	}
	noise_ptr->descriptor.scale = s_vec2(scale->x, scale->y);
	se_noise_update(ctx, noise_ptr->texture, &noise_ptr->descriptor);
}

u32 se_sdf_get_noise_seed(se_sdf_noise_handle noise) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return 0u;
	}
	return noise_ptr->descriptor.seed;
}

void se_sdf_noise_set_seed(se_sdf_noise_handle noise, u32 seed) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return;
	}
	noise_ptr->descriptor.seed = seed;
	se_noise_update(ctx, noise_ptr->texture, &noise_ptr->descriptor);
}

se_texture_handle se_sdf_get_noise_texture(se_sdf_noise_handle noise) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!noise_ptr) {
		return S_HANDLE_NULL;
	}
	return noise_ptr->texture;
}

void se_sdf_noise_set_texture(se_sdf_noise_handle noise, se_texture_handle texture) {
	se_context* ctx = se_current_context();
	se_sdf_noise* noise_ptr = se_sdf_noise_from_handle(ctx, noise);
	if (!ctx || !noise_ptr || texture == S_HANDLE_NULL) {
		return;
	}
	if (se_sdf_noise_from_handle(ctx, (se_sdf_noise_handle)texture)) {
		noise_ptr->descriptor = se_sdf_noise_from_handle(ctx, (se_sdf_noise_handle)texture)->descriptor;
		se_noise_update(ctx, noise_ptr->texture, &noise_ptr->descriptor);
		return;
	}
	se_sdf_copy_texture_content(ctx, noise_ptr->texture, texture);
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

s_json* se_sdf_to_json(se_sdf_handle sdf) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!ctx || sdf == S_HANDLE_NULL || !sdf_ptr) {
		se_set_last_error(!ctx || sdf == S_HANDLE_NULL ? SE_RESULT_INVALID_ARGUMENT : SE_RESULT_NOT_FOUND);
		return NULL;
	}

	s_json* root = s_json_object_empty(NULL);
	if (!root) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	if (!se_sdf_json_populate_object(ctx, sdf, root)) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		}
		s_json_free(root);
		return NULL;
	}

	se_set_last_error(SE_RESULT_OK);
	return root;
}

b8 se_sdf_from_json(se_sdf_handle sdf, const s_json* root) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!ctx || sdf == S_HANDLE_NULL || !root) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (!sdf_ptr) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (root->type != S_JSON_OBJECT) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	const s_json* format_node = s_json_get(root, "format");
	const s_json* version_node = s_json_get(root, "version");
	if ((format_node && !version_node) || (!format_node && version_node)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (format_node && version_node) {
		const c8* format = NULL;
		u32 version = 0u;
		if (!se_sdf_json_string_to_c8(se_sdf_json_get_required(root, "format", S_JSON_STRING), &format) ||
			!se_sdf_json_number_to_u32(se_sdf_json_get_required(root, "version", S_JSON_NUMBER), &version)) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		if (strcmp(format, SE_SDF_JSON_FORMAT) != 0 || version != SE_SDF_JSON_VERSION) {
			se_set_last_error(SE_RESULT_UNSUPPORTED);
			return false;
		}
	}

	se_sdf_handle staged = se_sdf_build_from_json_node(root);
	if (staged == S_HANDLE_NULL) {
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		}
		return false;
	}
	if (!se_sdf_transfer_state(ctx, sdf, staged)) {
		if (se_sdf_from_handle(ctx, staged)) {
			se_sdf_destroy(staged);
		}
		if (se_get_last_error() == SE_RESULT_OK) {
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		}
		return false;
	}

	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void se_sdf_upload_common_uniforms(
	const se_shader_handle shader,
	const se_sdf_handle sdf,
	se_sdf* sdf_ptr,
	se_camera* camera_ptr,
	const s_mat4* inverse_view_projection) {
	if (!sdf_ptr || shader == S_HANDLE_NULL || !camera_ptr || !inverse_view_projection) {
		return;
	}
	se_shader_use(shader, true, true);
	se_shader_set_mat4(shader, "u_inv_view_projection", inverse_view_projection);
	se_shader_set_vec3(shader, "u_camera_position", &camera_ptr->position);
	se_shader_set_int(shader, "u_use_orthographic", camera_ptr->use_orthographic ? 1 : 0);
	se_sdf_upload_uniforms_recursive(shader, sdf);
}

static b8 se_sdf_compile_render_shaders(se_context* ctx, const se_sdf_handle sdf, se_sdf* sdf_ptr) {
	c8* vs = NULL;
	c8* fs = NULL;
	if (!ctx || !sdf_ptr) {
		return false;
	}
	if (sdf_ptr->shader != S_HANDLE_NULL) {
		return true;
	}
	se_debug_trace_begin("sdf_shader_compile");
	se_sdf_destroy_shader_runtime(sdf_ptr);
	fs = calloc(SE_SDF_FRAGMENT_SHADER_CAPACITY, sizeof(c8));
	if (!fs) {
		free(vs);
		free(fs);
		se_debug_trace_end("sdf_shader_compile");
		return false;
	}
	if (!se_sdf_read_resource_text(&vs, SE_SDF_VERTEX_SHADER_TEMPLATE_PATH)
		|| !se_sdf_gen_fragment(fs, SE_SDF_FRAGMENT_SHADER_CAPACITY, sdf)) {
		free(vs);
		free(fs);
		se_debug_trace_end("sdf_shader_compile");
		return false;
	}
	sdf_ptr->shader = se_shader_load_from_memory(vs, fs);
	free(vs);
	free(fs);
	if (sdf_ptr->shader == S_HANDLE_NULL) {
		se_log("se_sdf_render :: failed to create shader for sdf: " SE_SDF_HANDLE_FMT, SE_SDF_HANDLE_ARG(sdf));
		se_sdf_destroy_shader_runtime(sdf_ptr);
		se_debug_trace_end("sdf_shader_compile");
		return false;
	}
	se_debug_trace_end("sdf_shader_compile");
	return true;
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
	se_debug_trace_begin("sdf_render");
	view_projection = se_camera_get_view_projection_matrix(camera);
	inverse_view_projection = s_mat4_inverse(&view_projection);
	se_sdf_upload_common_uniforms(sdf_ptr->shader, sdf, sdf_ptr, camera_ptr, &inverse_view_projection);
	se_quad_render(&sdf_ptr->quad, 0);
	se_debug_trace_end("sdf_render");
}

void se_sdf_render_to_framebuffer(se_sdf_handle sdf, se_camera_handle camera, const s_vec2* resolution) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!ctx || !sdf_ptr || camera == S_HANDLE_NULL) {
		return;
	}
	se_sdf_refresh_shader_templates(ctx);
	if (sdf_ptr->quad.vao == 0) {
		se_quad_2d_create(&sdf_ptr->quad, 0);
	}
	if (!se_sdf_compile_render_shaders(ctx, sdf, sdf_ptr)) {
		return;
	}
	if (sdf_ptr->output == S_HANDLE_NULL) {
		sdf_ptr->output = se_framebuffer_create(resolution);
	}

	// adjust framebuffer size if needed
	s_vec2 current_framebuffer_size = {};
	se_framebuffer_get_size(sdf_ptr->output, &current_framebuffer_size);
	if (current_framebuffer_size.x != resolution->x || current_framebuffer_size.y != resolution->y) {
		se_framebuffer_set_size(sdf_ptr->output, resolution);
	}

	se_framebuffer_bind(sdf_ptr->output);
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
	se_framebuffer_unbind(sdf_ptr->output);
}

void se_sdf_render_framebuffer_to_window(se_sdf_handle sdf, se_window_handle window) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr || sdf_ptr->output == S_HANDLE_NULL) {
		return;
	}
	se_window* window_ptr = s_array_get(&ctx->windows, window);
	se_framebuffer* framebuffer_ptr = s_array_get(&ctx->framebuffers, sdf_ptr->output);
	s_assertf(window_ptr, "se_sdf_render_framebuffer_to_window :: window is null");
	s_assertf(framebuffer_ptr, "se_sdf_render_framebuffer_to_window :: framebuffer is null");
	se_render_unbind_framebuffer();
	se_render_clear();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, window_ptr->width, window_ptr->height);
	se_shader_set_texture(window_ptr->shader, "u_texture", framebuffer_ptr->texture);
	se_window_render_quad(window);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	se_render_set_blending(false);
}

void se_sdf_render_to_window(se_sdf_handle sdf, se_camera_handle camera, se_window_handle window, const f32 ratio) {
	se_context* ctx = se_current_context();
	se_window* window_ptr = s_array_get(&ctx->windows, window);
	se_sdf_render_to_framebuffer(sdf, camera, &s_vec2(window_ptr->width * ratio, window_ptr->height * ratio));
	se_sdf_render_framebuffer_to_window(sdf, window);
}

void se_sdf_bake(se_sdf_handle sdf) {
	(void)sdf;
}

void se_sdf_set_transform(se_sdf_handle sdf, const s_mat4* transform) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr || !transform) {
		return;
	}
	sdf_ptr->transform = *transform;
}

void se_sdf_set_operation_amount(se_sdf_handle sdf, f32 amount) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr) {
		return;
	}
	sdf_ptr->operation_amount = amount;
}

void se_sdf_set_position(se_sdf_handle sdf, const s_vec3* position) {
	se_context* ctx = se_current_context();
	se_sdf* sdf_ptr = se_sdf_from_handle(ctx, sdf);
	if (!sdf_ptr || !position) {
		return;
	}
	s_mat4_set_translation(&sdf_ptr->transform, position);
}
