<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_vfx.h

Source header: `include/se_vfx.h`

## Overview

Particle system APIs for 2D and 3D emitters.

This page is generated from `include/se_vfx.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/vfx.md)

## Functions

### `se_vfx_2d_add_emitter`

<div class="api-signature">

```c
extern se_vfx_emitter_2d_handle se_vfx_2d_add_emitter(se_vfx_2d_handle vfx, const se_vfx_emitter_2d_params* params);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_create`

<div class="api-signature">

```c
extern se_vfx_2d_handle se_vfx_2d_create(const se_vfx_2d_params* params);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_destroy`

<div class="api-signature">

```c
extern void se_vfx_2d_destroy(se_vfx_2d_handle vfx);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_draw`

<div class="api-signature">

```c
extern b8 se_vfx_2d_draw(se_vfx_2d_handle vfx, se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_add_builtin_float_key`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_add_builtin_float_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, f32 value);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_add_builtin_vec2_key`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_add_builtin_vec2_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec2* value);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_add_builtin_vec3_key`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_add_builtin_vec3_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec3* value);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_add_builtin_vec4_key`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_add_builtin_vec4_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec4* value);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_add_uniform_float_key`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_add_uniform_float_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, f32 value);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_add_uniform_vec2_key`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_add_uniform_vec2_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec2* value);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_add_uniform_vec3_key`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_add_uniform_vec3_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec3* value);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_add_uniform_vec4_key`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_add_uniform_vec4_key(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec4* value);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_burst`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_burst(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, u32 count);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_clear_builtin_track`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_clear_builtin_track(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_builtin_target target);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_clear_tracks`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_clear_tracks(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_clear_uniform_track`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_clear_uniform_track(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* uniform_name);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_set_blend_mode`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_set_blend_mode(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_blend_mode blend_mode);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_set_particle_callback`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_set_particle_callback(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_particle_update_2d_callback callback, void* user_data);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_set_shader`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_set_shader(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, const c8* vertex_shader_path, const c8* fragment_shader_path);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_set_spawn`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_set_spawn(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, f32 spawn_rate, u32 burst_count, f32 lifetime_min, f32 lifetime_max);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_set_texture`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_set_texture(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_texture_handle texture);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_set_uniform_callback`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_set_uniform_callback(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter, se_vfx_uniform_2d_callback callback, void* user_data);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_start`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_start(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_emitter_stop`

<div class="api-signature">

```c
extern b8 se_vfx_2d_emitter_stop(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_get_diagnostics`

<div class="api-signature">

```c
extern b8 se_vfx_2d_get_diagnostics(se_vfx_2d_handle vfx, se_vfx_diagnostics* out_diagnostics);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_get_framebuffer`

<div class="api-signature">

```c
extern b8 se_vfx_2d_get_framebuffer(se_vfx_2d_handle vfx, se_framebuffer_handle* out_fb);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_get_texture_id`

<div class="api-signature">

```c
extern b8 se_vfx_2d_get_texture_id(se_vfx_2d_handle vfx, u32* out_texture);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_remove_emitter`

<div class="api-signature">

```c
extern b8 se_vfx_2d_remove_emitter(se_vfx_2d_handle vfx, se_vfx_emitter_2d_handle emitter);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_render`

<div class="api-signature">

```c
extern b8 se_vfx_2d_render(se_vfx_2d_handle vfx, se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_tick`

<div class="api-signature">

```c
extern b8 se_vfx_2d_tick(se_vfx_2d_handle vfx, f32 dt);
```

</div>

No inline description found in header comments.

### `se_vfx_2d_tick_window`

<div class="api-signature">

```c
extern b8 se_vfx_2d_tick_window(se_vfx_2d_handle vfx, se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_add_emitter`

<div class="api-signature">

```c
extern se_vfx_emitter_3d_handle se_vfx_3d_add_emitter(se_vfx_3d_handle vfx, const se_vfx_emitter_3d_params* params);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_create`

<div class="api-signature">

```c
extern se_vfx_3d_handle se_vfx_3d_create(const se_vfx_3d_params* params);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_destroy`

<div class="api-signature">

```c
extern void se_vfx_3d_destroy(se_vfx_3d_handle vfx);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_draw`

<div class="api-signature">

```c
extern b8 se_vfx_3d_draw(se_vfx_3d_handle vfx, se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_add_builtin_float_key`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_add_builtin_float_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, f32 value);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_add_builtin_vec2_key`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_add_builtin_vec2_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec2* value);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_add_builtin_vec3_key`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_add_builtin_vec3_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec3* value);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_add_builtin_vec4_key`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_add_builtin_vec4_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target, se_curve_mode mode, f32 t, const s_vec4* value);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_add_uniform_float_key`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_add_uniform_float_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, f32 value);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_add_uniform_vec2_key`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_add_uniform_vec2_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec2* value);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_add_uniform_vec3_key`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_add_uniform_vec3_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec3* value);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_add_uniform_vec4_key`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_add_uniform_vec4_key(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name, se_curve_mode mode, f32 t, const s_vec4* value);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_burst`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_burst(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, u32 count);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_clear_builtin_track`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_clear_builtin_track(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_builtin_target target);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_clear_tracks`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_clear_tracks(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_clear_uniform_track`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_clear_uniform_track(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* uniform_name);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_set_blend_mode`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_set_blend_mode(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_blend_mode blend_mode);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_set_model`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_set_model(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_model_handle model);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_set_particle_callback`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_set_particle_callback(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_particle_update_3d_callback callback, void* user_data);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_set_shader`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_set_shader(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, const c8* vertex_shader_path, const c8* fragment_shader_path);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_set_spawn`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_set_spawn(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, f32 spawn_rate, u32 burst_count, f32 lifetime_min, f32 lifetime_max);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_set_texture`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_set_texture(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_texture_handle texture);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_set_uniform_callback`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_set_uniform_callback(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter, se_vfx_uniform_3d_callback callback, void* user_data);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_start`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_start(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_emitter_stop`

<div class="api-signature">

```c
extern b8 se_vfx_3d_emitter_stop(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_get_diagnostics`

<div class="api-signature">

```c
extern b8 se_vfx_3d_get_diagnostics(se_vfx_3d_handle vfx, se_vfx_diagnostics* out_diagnostics);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_get_framebuffer`

<div class="api-signature">

```c
extern b8 se_vfx_3d_get_framebuffer(se_vfx_3d_handle vfx, se_framebuffer_handle* out_fb);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_get_texture_id`

<div class="api-signature">

```c
extern b8 se_vfx_3d_get_texture_id(se_vfx_3d_handle vfx, u32* out_texture);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_remove_emitter`

<div class="api-signature">

```c
extern b8 se_vfx_3d_remove_emitter(se_vfx_3d_handle vfx, se_vfx_emitter_3d_handle emitter);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_render`

<div class="api-signature">

```c
extern b8 se_vfx_3d_render(se_vfx_3d_handle vfx, se_window_handle window, se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_tick`

<div class="api-signature">

```c
extern b8 se_vfx_3d_tick(se_vfx_3d_handle vfx, f32 dt);
```

</div>

No inline description found in header comments.

### `se_vfx_3d_tick_window`

<div class="api-signature">

```c
extern b8 se_vfx_3d_tick_window(se_vfx_3d_handle vfx, se_window_handle window);
```

</div>

No inline description found in header comments.

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

### `se_vfx_3d_params`

<div class="api-signature">

```c
typedef struct { u32 max_emitters; s_vec2 render_size; b8 auto_resize_with_window : 1; } se_vfx_3d_params;
```

</div>

No inline description found in header comments.

### `se_vfx_diagnostics`

<div class="api-signature">

```c
typedef struct { u32 emitter_count; u32 alive_particles; u64 spawned_particles; u64 expired_particles; } se_vfx_diagnostics;
```

</div>

No inline description found in header comments.

### `se_vfx_emitter_2d_params`

<div class="api-signature">

```c
typedef struct { s_vec2 position; s_vec2 velocity; s_vec4 color; f32 size; f32 rotation; f32 spawn_rate; u32 burst_count; f32 lifetime_min; f32 lifetime_max; u32 max_particles; se_texture_handle texture; const c8* vertex_shader_path; const c8* fragment_shader_path; se_vfx_blend_mode blend_mode; se_vfx_particle_update_2d_callback particle_callback; void* particle_user_data; se_vfx_uniform_2d_callback uniform_callback; void* uniform_user_data; b8 start_active : 1; } se_vfx_emitter_2d_params;
```

</div>

No inline description found in header comments.

### `se_vfx_emitter_3d_params`

<div class="api-signature">

```c
typedef struct { s_vec3 position; s_vec3 velocity; s_vec4 color; f32 size; f32 rotation; f32 spawn_rate; u32 burst_count; f32 lifetime_min; f32 lifetime_max; u32 max_particles; se_texture_handle texture; se_model_handle model; const c8* vertex_shader_path; const c8* fragment_shader_path; se_vfx_blend_mode blend_mode; se_vfx_particle_update_3d_callback particle_callback; void* particle_user_data; se_vfx_uniform_3d_callback uniform_callback; void* uniform_user_data; b8 start_active : 1; } se_vfx_emitter_3d_params;
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
