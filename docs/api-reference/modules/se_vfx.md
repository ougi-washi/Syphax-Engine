<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_vfx.h

Source header: `include/se_vfx.h`

## Overview

Particle system APIs for 2D and 3D emitters.

This page is generated from `include/se_vfx.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/vfx.md)

## Functions

No functions found in this header.

## Enums

### `se_vfx_blend_mode`

<div class="api-signature">

```c
typedef enum { SE_VFX_BLEND_ALPHA = 0, SE_VFX_BLEND_ADDITIVE } se_vfx_blend_mode;
```

</div>

No inline description found in header comments.

### `se_vfx_builtin_target`

<div class="api-signature">

```c
typedef enum { SE_VFX_TARGET_VELOCITY = 0, SE_VFX_TARGET_COLOR, SE_VFX_TARGET_SIZE, SE_VFX_TARGET_ROTATION, SE_VFX_TARGET_COUNT } se_vfx_builtin_target;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_vfx_2d_params`

<div class="api-signature">

```c
typedef struct { u32 max_emitters; s_vec2 render_size; b8 auto_resize_with_window : 1; } se_vfx_2d_params;
```

</div>

No inline description found in header comments.

### `se_vfx_particle_2d`

<div class="api-signature">

```c
typedef struct { s_vec2 position; s_vec2 velocity; s_vec4 color; f32 size; f32 rotation; f32 age; f32 lifetime; b8 alive : 1; } se_vfx_particle_2d;
```

</div>

No inline description found in header comments.

### `se_vfx_particle_3d`

<div class="api-signature">

```c
typedef struct { s_vec3 position; s_vec3 velocity; s_vec4 color; f32 size; f32 rotation; f32 age; f32 lifetime; b8 alive : 1; } se_vfx_particle_3d;
```

</div>

No inline description found in header comments.

### `se_vfx_particle_update_2d_callback`

<div class="api-signature">

```c
typedef void (*se_vfx_particle_update_2d_callback)(se_vfx_particle_2d* particle, f32 dt, void* user_data);
```

</div>

No inline description found in header comments.

### `se_vfx_particle_update_3d_callback`

<div class="api-signature">

```c
typedef void (*se_vfx_particle_update_3d_callback)(se_vfx_particle_3d* particle, f32 dt, void* user_data);
```

</div>

No inline description found in header comments.

### `se_vfx_uniform_2d_callback`

<div class="api-signature">

```c
typedef void (*se_vfx_uniform_2d_callback)(se_vfx_emitter_2d_handle emitter, se_shader_handle shader, f32 dt, void* user_data);
```

</div>

No inline description found in header comments.

### `se_vfx_uniform_3d_callback`

<div class="api-signature">

```c
typedef void (*se_vfx_uniform_3d_callback)(se_vfx_emitter_3d_handle emitter, se_shader_handle shader, f32 dt, void* user_data);
```

</div>

No inline description found in header comments.
