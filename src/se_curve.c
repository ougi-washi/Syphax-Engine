// Syphax-Engine - Ougi Washi

#include "se_curve.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	sz left;
	sz right;
	f32 local_t;
} se_curve_segment;

static f32 se_curve_clamp01(const f32 t) {
	return s_max(0.0f, s_min(1.0f, t));
}

static f32 se_curve_apply_mode(const se_curve_mode mode, const f32 t) {
	const f32 clamped = se_curve_clamp01(t);
	switch (mode) {
		case SE_CURVE_STEP:
			return 0.0f;
		case SE_CURVE_LINEAR:
			return clamped;
		case SE_CURVE_SMOOTH:
			return clamped * clamped * (3.0f - 2.0f * clamped);
		case SE_CURVE_EASE_IN:
			return clamped * clamped;
		case SE_CURVE_EASE_OUT:
			return 1.0f - (1.0f - clamped) * (1.0f - clamped);
		case SE_CURVE_EASE_IN_OUT:
			if (clamped < 0.5f) {
				return 2.0f * clamped * clamped;
			}
			return 1.0f - powf(-2.0f * clamped + 2.0f, 2.0f) * 0.5f;
		default:
			return clamped;
	}
}

static f32 se_curve_key_t_at(const u8* bytes, const sz stride, const sz index) {
	f32 t = 0.0f;
	memcpy(&t, bytes + (index * stride), sizeof(f32));
	return t;
}

static void se_curve_sort_keys_raw(void* data, const sz count, const sz stride) {
	if (!data || count < 2 || stride == 0) {
		return;
	}

	u8 stack_buffer[64] = {0};
	u8* temp = stack_buffer;
	if (stride > sizeof(stack_buffer)) {
		temp = malloc(stride);
		if (!temp) {
			return;
		}
	}

	u8* bytes = (u8*)data;
	for (sz i = 1; i < count; ++i) {
		memcpy(temp, bytes + (i * stride), stride);
		const f32 temp_t = se_curve_key_t_at(temp, stride, 0);
		sz j = i;
		while (j > 0) {
			const f32 prev_t = se_curve_key_t_at(bytes, stride, j - 1);
			if (prev_t <= temp_t) {
				break;
			}
			memcpy(bytes + (j * stride), bytes + ((j - 1) * stride), stride);
			j--;
		}
		memcpy(bytes + (j * stride), temp, stride);
	}

	if (temp != stack_buffer) {
		free(temp);
	}
}

static b8 se_curve_find_segment(const void* data, const sz count, const sz stride, const f32 t, se_curve_segment* out_segment) {
	if (!data || count == 0 || stride == 0 || !out_segment) {
		return false;
	}

	const u8* bytes = (const u8*)data;
	if (count == 1) {
		out_segment->left = 0;
		out_segment->right = 0;
		out_segment->local_t = 0.0f;
		return true;
	}

	const f32 first_t = se_curve_key_t_at(bytes, stride, 0);
	const f32 last_t = se_curve_key_t_at(bytes, stride, count - 1);
	if (t <= first_t) {
		out_segment->left = 0;
		out_segment->right = 0;
		out_segment->local_t = 0.0f;
		return true;
	}
	if (t >= last_t) {
		out_segment->left = count - 1;
		out_segment->right = count - 1;
		out_segment->local_t = 0.0f;
		return true;
	}

	for (sz i = 0; i + 1 < count; ++i) {
		const f32 a_t = se_curve_key_t_at(bytes, stride, i);
		const f32 b_t = se_curve_key_t_at(bytes, stride, i + 1);
		if (t > b_t) {
			continue;
		}
		out_segment->left = i;
		out_segment->right = i + 1;
		const f32 denom = b_t - a_t;
		if (fabsf(denom) <= 0.000001f) {
			out_segment->local_t = 0.0f;
		} else {
			out_segment->local_t = se_curve_clamp01((t - a_t) / denom);
		}
		return true;
	}

	out_segment->left = count - 1;
	out_segment->right = count - 1;
	out_segment->local_t = 0.0f;
	return true;
}

void se_curve_float_init(se_curve_float_keys* keys) {
	if (!keys) {
		return;
	}
	s_array_init(keys);
}

void se_curve_vec2_init(se_curve_vec2_keys* keys) {
	if (!keys) {
		return;
	}
	s_array_init(keys);
}

void se_curve_vec3_init(se_curve_vec3_keys* keys) {
	if (!keys) {
		return;
	}
	s_array_init(keys);
}

void se_curve_vec4_init(se_curve_vec4_keys* keys) {
	if (!keys) {
		return;
	}
	s_array_init(keys);
}

b8 se_curve_float_add_key(se_curve_float_keys* keys, const f32 t, const f32 value) {
	if (!keys) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_curve_float_key key = {0};
	key.t = se_curve_clamp01(t);
	key.value = value;
	s_array_add(keys, key);
	se_curve_float_sort(keys);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_curve_vec2_add_key(se_curve_vec2_keys* keys, const f32 t, const s_vec2* value) {
	if (!keys || !value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_curve_vec2_key key = {0};
	key.t = se_curve_clamp01(t);
	key.value = *value;
	s_array_add(keys, key);
	se_curve_vec2_sort(keys);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_curve_vec3_add_key(se_curve_vec3_keys* keys, const f32 t, const s_vec3* value) {
	if (!keys || !value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_curve_vec3_key key = {0};
	key.t = se_curve_clamp01(t);
	key.value = *value;
	s_array_add(keys, key);
	se_curve_vec3_sort(keys);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_curve_vec4_add_key(se_curve_vec4_keys* keys, const f32 t, const s_vec4* value) {
	if (!keys || !value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_curve_vec4_key key = {0};
	key.t = se_curve_clamp01(t);
	key.value = *value;
	s_array_add(keys, key);
	se_curve_vec4_sort(keys);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_curve_float_clear(se_curve_float_keys* keys) {
	if (!keys) {
		return;
	}
	while (s_array_get_size(keys) > 0) {
		s_array_remove(keys, s_array_handle(keys, (u32)(s_array_get_size(keys) - 1)));
	}
}

void se_curve_vec2_clear(se_curve_vec2_keys* keys) {
	if (!keys) {
		return;
	}
	while (s_array_get_size(keys) > 0) {
		s_array_remove(keys, s_array_handle(keys, (u32)(s_array_get_size(keys) - 1)));
	}
}

void se_curve_vec3_clear(se_curve_vec3_keys* keys) {
	if (!keys) {
		return;
	}
	while (s_array_get_size(keys) > 0) {
		s_array_remove(keys, s_array_handle(keys, (u32)(s_array_get_size(keys) - 1)));
	}
}

void se_curve_vec4_clear(se_curve_vec4_keys* keys) {
	if (!keys) {
		return;
	}
	while (s_array_get_size(keys) > 0) {
		s_array_remove(keys, s_array_handle(keys, (u32)(s_array_get_size(keys) - 1)));
	}
}

void se_curve_float_reset(se_curve_float_keys* keys) {
	if (!keys) {
		return;
	}
	s_array_clear(keys);
	s_array_init(keys);
}

void se_curve_vec2_reset(se_curve_vec2_keys* keys) {
	if (!keys) {
		return;
	}
	s_array_clear(keys);
	s_array_init(keys);
}

void se_curve_vec3_reset(se_curve_vec3_keys* keys) {
	if (!keys) {
		return;
	}
	s_array_clear(keys);
	s_array_init(keys);
}

void se_curve_vec4_reset(se_curve_vec4_keys* keys) {
	if (!keys) {
		return;
	}
	s_array_clear(keys);
	s_array_init(keys);
}

void se_curve_float_sort(se_curve_float_keys* keys) {
	if (!keys || s_array_get_size(keys) < 2) {
		return;
	}
	se_curve_sort_keys_raw(s_array_get_data(keys), s_array_get_size(keys), sizeof(se_curve_float_key));
}

void se_curve_vec2_sort(se_curve_vec2_keys* keys) {
	if (!keys || s_array_get_size(keys) < 2) {
		return;
	}
	se_curve_sort_keys_raw(s_array_get_data(keys), s_array_get_size(keys), sizeof(se_curve_vec2_key));
}

void se_curve_vec3_sort(se_curve_vec3_keys* keys) {
	if (!keys || s_array_get_size(keys) < 2) {
		return;
	}
	se_curve_sort_keys_raw(s_array_get_data(keys), s_array_get_size(keys), sizeof(se_curve_vec3_key));
}

void se_curve_vec4_sort(se_curve_vec4_keys* keys) {
	if (!keys || s_array_get_size(keys) < 2) {
		return;
	}
	se_curve_sort_keys_raw(s_array_get_data(keys), s_array_get_size(keys), sizeof(se_curve_vec4_key));
}

b8 se_curve_float_eval(const se_curve_float_keys* keys, const se_curve_mode mode, const f32 t, f32* out_value) {
	if (!keys || !out_value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const sz count = s_array_get_size((se_curve_float_keys*)keys);
	if (count == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const se_curve_float_key* values = s_array_get_data((se_curve_float_keys*)keys);
	const f32 clamped = se_curve_clamp01(t);
	se_curve_segment segment = {0};
	if (!se_curve_find_segment(values, count, sizeof(se_curve_float_key), clamped, &segment)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (segment.left == segment.right) {
		*out_value = values[segment.left].value;
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	const f32 blend = se_curve_apply_mode(mode, segment.local_t);
	const f32 a = values[segment.left].value;
	const f32 b = values[segment.right].value;
	*out_value = a + (b - a) * blend;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_curve_vec2_eval(const se_curve_vec2_keys* keys, const se_curve_mode mode, const f32 t, s_vec2* out_value) {
	if (!keys || !out_value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const sz count = s_array_get_size((se_curve_vec2_keys*)keys);
	if (count == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const se_curve_vec2_key* values = s_array_get_data((se_curve_vec2_keys*)keys);
	const f32 clamped = se_curve_clamp01(t);
	se_curve_segment segment = {0};
	if (!se_curve_find_segment(values, count, sizeof(se_curve_vec2_key), clamped, &segment)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (segment.left == segment.right) {
		*out_value = values[segment.left].value;
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	const f32 blend = se_curve_apply_mode(mode, segment.local_t);
	*out_value = s_vec2_lerp(&values[segment.left].value, &values[segment.right].value, blend);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_curve_vec3_eval(const se_curve_vec3_keys* keys, const se_curve_mode mode, const f32 t, s_vec3* out_value) {
	if (!keys || !out_value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const sz count = s_array_get_size((se_curve_vec3_keys*)keys);
	if (count == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const se_curve_vec3_key* values = s_array_get_data((se_curve_vec3_keys*)keys);
	const f32 clamped = se_curve_clamp01(t);
	se_curve_segment segment = {0};
	if (!se_curve_find_segment(values, count, sizeof(se_curve_vec3_key), clamped, &segment)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (segment.left == segment.right) {
		*out_value = values[segment.left].value;
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	const f32 blend = se_curve_apply_mode(mode, segment.local_t);
	*out_value = s_vec3_lerp(&values[segment.left].value, &values[segment.right].value, blend);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_curve_vec4_eval(const se_curve_vec4_keys* keys, const se_curve_mode mode, const f32 t, s_vec4* out_value) {
	if (!keys || !out_value) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const sz count = s_array_get_size((se_curve_vec4_keys*)keys);
	if (count == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	const se_curve_vec4_key* values = s_array_get_data((se_curve_vec4_keys*)keys);
	const f32 clamped = se_curve_clamp01(t);
	se_curve_segment segment = {0};
	if (!se_curve_find_segment(values, count, sizeof(se_curve_vec4_key), clamped, &segment)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (segment.left == segment.right) {
		*out_value = values[segment.left].value;
		se_set_last_error(SE_RESULT_OK);
		return true;
	}
	const f32 blend = se_curve_apply_mode(mode, segment.local_t);
	*out_value = s_vec4_lerp(&values[segment.left].value, &values[segment.right].value, blend);
	se_set_last_error(SE_RESULT_OK);
	return true;
}
