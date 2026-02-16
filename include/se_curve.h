// Syphax-Engine - Ougi Washi

#ifndef SE_CURVE_H
#define SE_CURVE_H

#include "se_defines.h"
#include "syphax/s_array.h"

typedef enum {
	SE_CURVE_STEP = 0,
	SE_CURVE_LINEAR,
	SE_CURVE_SMOOTH,
	SE_CURVE_EASE_IN,
	SE_CURVE_EASE_OUT,
	SE_CURVE_EASE_IN_OUT
} se_curve_mode;

typedef struct {
	f32 t;
	f32 value;
} se_curve_float_key;

typedef struct {
	f32 t;
	s_vec2 value;
} se_curve_vec2_key;

typedef struct {
	f32 t;
	s_vec3 value;
} se_curve_vec3_key;

typedef struct {
	f32 t;
	s_vec4 value;
} se_curve_vec4_key;

typedef s_array(se_curve_float_key, se_curve_float_keys);
typedef s_array(se_curve_vec2_key, se_curve_vec2_keys);
typedef s_array(se_curve_vec3_key, se_curve_vec3_keys);
typedef s_array(se_curve_vec4_key, se_curve_vec4_keys);

extern void se_curve_float_init(se_curve_float_keys* keys);
extern void se_curve_vec2_init(se_curve_vec2_keys* keys);
extern void se_curve_vec3_init(se_curve_vec3_keys* keys);
extern void se_curve_vec4_init(se_curve_vec4_keys* keys);

extern b8 se_curve_float_add_key(se_curve_float_keys* keys, const f32 t, const f32 value);
extern b8 se_curve_vec2_add_key(se_curve_vec2_keys* keys, const f32 t, const s_vec2* value);
extern b8 se_curve_vec3_add_key(se_curve_vec3_keys* keys, const f32 t, const s_vec3* value);
extern b8 se_curve_vec4_add_key(se_curve_vec4_keys* keys, const f32 t, const s_vec4* value);

extern void se_curve_float_clear(se_curve_float_keys* keys);
extern void se_curve_vec2_clear(se_curve_vec2_keys* keys);
extern void se_curve_vec3_clear(se_curve_vec3_keys* keys);
extern void se_curve_vec4_clear(se_curve_vec4_keys* keys);

extern void se_curve_float_reset(se_curve_float_keys* keys);
extern void se_curve_vec2_reset(se_curve_vec2_keys* keys);
extern void se_curve_vec3_reset(se_curve_vec3_keys* keys);
extern void se_curve_vec4_reset(se_curve_vec4_keys* keys);

extern void se_curve_float_sort(se_curve_float_keys* keys);
extern void se_curve_vec2_sort(se_curve_vec2_keys* keys);
extern void se_curve_vec3_sort(se_curve_vec3_keys* keys);
extern void se_curve_vec4_sort(se_curve_vec4_keys* keys);

extern b8 se_curve_float_eval(const se_curve_float_keys* keys, const se_curve_mode mode, const f32 t, f32* out_value);
extern b8 se_curve_vec2_eval(const se_curve_vec2_keys* keys, const se_curve_mode mode, const f32 t, s_vec2* out_value);
extern b8 se_curve_vec3_eval(const se_curve_vec3_keys* keys, const se_curve_mode mode, const f32 t, s_vec3* out_value);
extern b8 se_curve_vec4_eval(const se_curve_vec4_keys* keys, const se_curve_mode mode, const f32 t, s_vec4* out_value);

#endif // SE_CURVE_H
