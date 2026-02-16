<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_curve.h

Source header: `include/se_curve.h`

## Overview

Curve keyframe data and interpolation helpers.

This page is generated from `include/se_curve.h` and is deterministic.

## Functions

### `se_curve_float_add_key`

<div class="api-signature">

```c
extern b8 se_curve_float_add_key(se_curve_float_keys* keys, const f32 t, const f32 value);
```

</div>

No inline description found in header comments.

### `se_curve_float_clear`

<div class="api-signature">

```c
extern void se_curve_float_clear(se_curve_float_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_float_eval`

<div class="api-signature">

```c
extern b8 se_curve_float_eval(const se_curve_float_keys* keys, const se_curve_mode mode, const f32 t, f32* out_value);
```

</div>

No inline description found in header comments.

### `se_curve_float_init`

<div class="api-signature">

```c
extern void se_curve_float_init(se_curve_float_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_float_reset`

<div class="api-signature">

```c
extern void se_curve_float_reset(se_curve_float_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_float_sort`

<div class="api-signature">

```c
extern void se_curve_float_sort(se_curve_float_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec2_add_key`

<div class="api-signature">

```c
extern b8 se_curve_vec2_add_key(se_curve_vec2_keys* keys, const f32 t, const s_vec2* value);
```

</div>

No inline description found in header comments.

### `se_curve_vec2_clear`

<div class="api-signature">

```c
extern void se_curve_vec2_clear(se_curve_vec2_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec2_eval`

<div class="api-signature">

```c
extern b8 se_curve_vec2_eval(const se_curve_vec2_keys* keys, const se_curve_mode mode, const f32 t, s_vec2* out_value);
```

</div>

No inline description found in header comments.

### `se_curve_vec2_init`

<div class="api-signature">

```c
extern void se_curve_vec2_init(se_curve_vec2_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec2_reset`

<div class="api-signature">

```c
extern void se_curve_vec2_reset(se_curve_vec2_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec2_sort`

<div class="api-signature">

```c
extern void se_curve_vec2_sort(se_curve_vec2_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec3_add_key`

<div class="api-signature">

```c
extern b8 se_curve_vec3_add_key(se_curve_vec3_keys* keys, const f32 t, const s_vec3* value);
```

</div>

No inline description found in header comments.

### `se_curve_vec3_clear`

<div class="api-signature">

```c
extern void se_curve_vec3_clear(se_curve_vec3_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec3_eval`

<div class="api-signature">

```c
extern b8 se_curve_vec3_eval(const se_curve_vec3_keys* keys, const se_curve_mode mode, const f32 t, s_vec3* out_value);
```

</div>

No inline description found in header comments.

### `se_curve_vec3_init`

<div class="api-signature">

```c
extern void se_curve_vec3_init(se_curve_vec3_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec3_reset`

<div class="api-signature">

```c
extern void se_curve_vec3_reset(se_curve_vec3_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec3_sort`

<div class="api-signature">

```c
extern void se_curve_vec3_sort(se_curve_vec3_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec4_add_key`

<div class="api-signature">

```c
extern b8 se_curve_vec4_add_key(se_curve_vec4_keys* keys, const f32 t, const s_vec4* value);
```

</div>

No inline description found in header comments.

### `se_curve_vec4_clear`

<div class="api-signature">

```c
extern void se_curve_vec4_clear(se_curve_vec4_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec4_eval`

<div class="api-signature">

```c
extern b8 se_curve_vec4_eval(const se_curve_vec4_keys* keys, const se_curve_mode mode, const f32 t, s_vec4* out_value);
```

</div>

No inline description found in header comments.

### `se_curve_vec4_init`

<div class="api-signature">

```c
extern void se_curve_vec4_init(se_curve_vec4_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec4_reset`

<div class="api-signature">

```c
extern void se_curve_vec4_reset(se_curve_vec4_keys* keys);
```

</div>

No inline description found in header comments.

### `se_curve_vec4_sort`

<div class="api-signature">

```c
extern void se_curve_vec4_sort(se_curve_vec4_keys* keys);
```

</div>

No inline description found in header comments.

## Enums

### `se_curve_mode`

<div class="api-signature">

```c
typedef enum { SE_CURVE_STEP = 0, SE_CURVE_LINEAR, SE_CURVE_SMOOTH, SE_CURVE_EASE_IN, SE_CURVE_EASE_OUT, SE_CURVE_EASE_IN_OUT } se_curve_mode;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_curve_float_key`

<div class="api-signature">

```c
typedef struct { f32 t; f32 value; } se_curve_float_key;
```

</div>

No inline description found in header comments.

### `se_curve_vec2_key`

<div class="api-signature">

```c
typedef struct { f32 t; s_vec2 value; } se_curve_vec2_key;
```

</div>

No inline description found in header comments.

### `se_curve_vec3_key`

<div class="api-signature">

```c
typedef struct { f32 t; s_vec3 value; } se_curve_vec3_key;
```

</div>

No inline description found in header comments.

### `se_curve_vec4_key`

<div class="api-signature">

```c
typedef struct { f32 t; s_vec4 value; } se_curve_vec4_key;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_curve_float_key, se_curve_float_keys);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_curve_vec2_key, se_curve_vec2_keys);
```

</div>

No inline description found in header comments.

### `typedef_3`

<div class="api-signature">

```c
typedef s_array(se_curve_vec3_key, se_curve_vec3_keys);
```

</div>

No inline description found in header comments.

### `typedef_4`

<div class="api-signature">

```c
typedef s_array(se_curve_vec4_key, se_curve_vec4_keys);
```

</div>

No inline description found in header comments.
