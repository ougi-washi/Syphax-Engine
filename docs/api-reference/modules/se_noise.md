<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_noise.h

Source header: `include/se_noise.h`

## Overview

Public declarations exposed by this header.

This page is generated from `include/se_noise.h` and is deterministic.

## Functions

### `se_noise_2d_create`

<div class="api-signature">

```c
extern se_texture_handle se_noise_2d_create(se_context* ctx, const se_noise_2d* noise);
```

</div>

No inline description found in header comments.

### `se_noise_2d_destroy`

<div class="api-signature">

```c
extern void se_noise_2d_destroy(se_context* ctx, se_texture_handle texture);
```

</div>

No inline description found in header comments.

### `se_noise_update`

<div class="api-signature">

```c
extern void se_noise_update(se_context* ctx, se_texture_handle texture, const se_noise_2d* noise);
```

</div>

No inline description found in header comments.

## Enums

### `se_noise_type`

<div class="api-signature">

```c
typedef enum se_noise_type { SE_NOISE_SIMPLEX, SE_NOISE_PERLIN, SE_NOISE_VORNOI, SE_NOISE_WORLEY, } se_noise_type;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_noise_2d`

<div class="api-signature">

```c
typedef struct se_noise_2d { se_noise_type type; f32 frequency; s_vec2 offset; s_vec2 scale; u32 seed; } se_noise_2d;
```

</div>

No inline description found in header comments.

### `se_noise_3d`

<div class="api-signature">

```c
typedef struct se_noise_3d { se_noise_type type; f32 frequency; s_vec3 offset; s_vec3 scale; u32 seed; } se_noise_3d;
```

</div>

No inline description found in header comments.
