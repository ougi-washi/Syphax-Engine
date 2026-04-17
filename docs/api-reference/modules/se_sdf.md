<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_sdf.h

Source header: `include/se_sdf.h`

## Overview

Signed distance field scene graph, renderer, and material controls.

This page is generated from `include/se_sdf.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/sdf.md)

## Functions

### `se_sdf_add_child`

<div class="api-signature">

```c
extern void se_sdf_add_child(se_sdf_handle parent, se_sdf_handle child);
```

</div>

No inline description found in header comments.

### `se_sdf_add_directional_light_internal`

<div class="api-signature">

```c
extern se_sdf_directional_light_handle se_sdf_add_directional_light_internal(se_sdf_handle sdf, const se_sdf_directional_light directional_light);
```

</div>

No inline description found in header comments.

### `se_sdf_add_noise_internal`

<div class="api-signature">

```c
extern se_sdf_noise_handle se_sdf_add_noise_internal(se_sdf_handle sdf, const se_noise_2d* noise);
```

</div>

No inline description found in header comments.

### `se_sdf_add_point_light_internal`

<div class="api-signature">

```c
extern se_sdf_point_light_handle se_sdf_add_point_light_internal(se_sdf_handle sdf, const se_sdf_point_light point_light);
```

</div>

No inline description found in header comments.

### `se_sdf_bake`

<div class="api-signature">

```c
extern void se_sdf_bake(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_create_internal`

<div class="api-signature">

```c
extern se_sdf_handle se_sdf_create_internal(const se_sdf* sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_destroy`

<div class="api-signature">

```c
extern void se_sdf_destroy(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_directional_light_set_color`

<div class="api-signature">

```c
extern void se_sdf_directional_light_set_color(se_sdf_directional_light_handle directional_light, const s_vec3* color);
```

</div>

No inline description found in header comments.

### `se_sdf_directional_light_set_direction`

<div class="api-signature">

```c
extern void se_sdf_directional_light_set_direction(se_sdf_directional_light_handle directional_light, const s_vec3* direction);
```

</div>

No inline description found in header comments.

### `se_sdf_from_json`

<div class="api-signature">

```c
extern b8 se_sdf_from_json(se_sdf_handle sdf, const s_json* root);
```

</div>

No inline description found in header comments.

### `se_sdf_from_json_file`

<div class="api-signature">

```c
extern b8 se_sdf_from_json_file(se_sdf_handle sdf, const c8* path);
```

</div>

Loads an SDF scene from either a normal file path or a packaged asset path.

### `se_sdf_get_children`

<div class="api-signature">

```c
extern b8 se_sdf_get_children(se_sdf_handle sdf, const se_sdf_handle** out_children, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_sdf_get_directional_light_color`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_directional_light_color(se_sdf_directional_light_handle directional_light);
```

</div>

No inline description found in header comments.

### `se_sdf_get_directional_light_direction`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_directional_light_direction(se_sdf_directional_light_handle directional_light);
```

</div>

No inline description found in header comments.

### `se_sdf_get_directional_lights`

<div class="api-signature">

```c
extern b8 se_sdf_get_directional_lights(se_sdf_handle sdf, const se_sdf_directional_light_handle** out_directional_lights, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_sdf_get_noise_frequency`

<div class="api-signature">

```c
extern f32 se_sdf_get_noise_frequency(se_sdf_noise_handle noise);
```

</div>

No inline description found in header comments.

### `se_sdf_get_noise_offset`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_noise_offset(se_sdf_noise_handle noise);
```

</div>

No inline description found in header comments.

### `se_sdf_get_noise_scale`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_noise_scale(se_sdf_noise_handle noise);
```

</div>

No inline description found in header comments.

### `se_sdf_get_noise_seed`

<div class="api-signature">

```c
extern u32 se_sdf_get_noise_seed(se_sdf_noise_handle noise);
```

</div>

No inline description found in header comments.

### `se_sdf_get_noise_texture`

<div class="api-signature">

```c
extern se_texture_handle se_sdf_get_noise_texture(se_sdf_noise_handle noise);
```

</div>

No inline description found in header comments.

### `se_sdf_get_noises`

<div class="api-signature">

```c
extern b8 se_sdf_get_noises(se_sdf_handle sdf, const se_sdf_noise_handle** out_noises, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_sdf_get_point_light_color`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_point_light_color(se_sdf_point_light_handle point_light);
```

</div>

No inline description found in header comments.

### `se_sdf_get_point_light_position`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_point_light_position(se_sdf_point_light_handle point_light);
```

</div>

No inline description found in header comments.

### `se_sdf_get_point_light_radius`

<div class="api-signature">

```c
extern f32 se_sdf_get_point_light_radius(se_sdf_point_light_handle point_light);
```

</div>

No inline description found in header comments.

### `se_sdf_get_point_lights`

<div class="api-signature">

```c
extern b8 se_sdf_get_point_lights(se_sdf_handle sdf, const se_sdf_point_light_handle** out_point_lights, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shading`

<div class="api-signature">

```c
extern se_sdf_shading se_sdf_get_shading(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shading_ambient`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_shading_ambient(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shading_bias`

<div class="api-signature">

```c
extern f32 se_sdf_get_shading_bias(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shading_diffuse`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_shading_diffuse(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shading_roughness`

<div class="api-signature">

```c
extern f32 se_sdf_get_shading_roughness(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shading_smoothness`

<div class="api-signature">

```c
extern f32 se_sdf_get_shading_smoothness(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shading_specular`

<div class="api-signature">

```c
extern s_vec3 se_sdf_get_shading_specular(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shadow`

<div class="api-signature">

```c
extern se_sdf_shadow se_sdf_get_shadow(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shadow_bias`

<div class="api-signature">

```c
extern f32 se_sdf_get_shadow_bias(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shadow_samples`

<div class="api-signature">

```c
extern u16 se_sdf_get_shadow_samples(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_shadow_softness`

<div class="api-signature">

```c
extern f32 se_sdf_get_shadow_softness(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_noise_set_frequency`

<div class="api-signature">

```c
extern void se_sdf_noise_set_frequency(se_sdf_noise_handle noise, f32 frequency);
```

</div>

No inline description found in header comments.

### `se_sdf_noise_set_offset`

<div class="api-signature">

```c
extern void se_sdf_noise_set_offset(se_sdf_noise_handle noise, const s_vec3* offset);
```

</div>

No inline description found in header comments.

### `se_sdf_noise_set_scale`

<div class="api-signature">

```c
extern void se_sdf_noise_set_scale(se_sdf_noise_handle noise, const s_vec3* scale);
```

</div>

No inline description found in header comments.

### `se_sdf_noise_set_seed`

<div class="api-signature">

```c
extern void se_sdf_noise_set_seed(se_sdf_noise_handle noise, u32 seed);
```

</div>

No inline description found in header comments.

### `se_sdf_noise_set_texture`

<div class="api-signature">

```c
extern void se_sdf_noise_set_texture(se_sdf_noise_handle noise, se_texture_handle texture);
```

</div>

No inline description found in header comments.

### `se_sdf_point_light_remove`

<div class="api-signature">

```c
extern void se_sdf_point_light_remove(se_sdf_handle sdf, se_sdf_point_light_handle point_light);
```

</div>

No inline description found in header comments.

### `se_sdf_point_light_set_color`

<div class="api-signature">

```c
extern void se_sdf_point_light_set_color(se_sdf_point_light_handle point_light, const s_vec3* color);
```

</div>

No inline description found in header comments.

### `se_sdf_point_light_set_position`

<div class="api-signature">

```c
extern void se_sdf_point_light_set_position(se_sdf_point_light_handle point_light, const s_vec3* position);
```

</div>

No inline description found in header comments.

### `se_sdf_point_light_set_radius`

<div class="api-signature">

```c
extern void se_sdf_point_light_set_radius(se_sdf_point_light_handle point_light, f32 radius);
```

</div>

No inline description found in header comments.

### `se_sdf_render_framebuffer_to_window`

<div class="api-signature">

```c
extern void se_sdf_render_framebuffer_to_window(se_sdf_handle sdf, se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_sdf_render_to_framebuffer`

<div class="api-signature">

```c
extern void se_sdf_render_to_framebuffer(se_sdf_handle sdf, se_camera_handle camera, const s_vec2* resolution);
```

</div>

No inline description found in header comments.

### `se_sdf_render_to_window`

<div class="api-signature">

```c
extern void se_sdf_render_to_window(se_sdf_handle sdf, se_camera_handle camera, se_window_handle window, const f32 ratio);
```

</div>

No inline description found in header comments.

### `se_sdf_set_operation_amount`

<div class="api-signature">

```c
extern void se_sdf_set_operation_amount(se_sdf_handle sdf, f32 amount);
```

</div>

No inline description found in header comments.

### `se_sdf_set_position`

<div class="api-signature">

```c
extern void se_sdf_set_position(se_sdf_handle sdf, const s_vec3* position);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shading`

<div class="api-signature">

```c
extern void se_sdf_set_shading(se_sdf_handle sdf, const se_sdf_shading* shading);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shading_ambient`

<div class="api-signature">

```c
extern void se_sdf_set_shading_ambient(se_sdf_handle sdf, const s_vec3* ambient);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shading_bias`

<div class="api-signature">

```c
extern void se_sdf_set_shading_bias(se_sdf_handle sdf, f32 bias);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shading_diffuse`

<div class="api-signature">

```c
extern void se_sdf_set_shading_diffuse(se_sdf_handle sdf, const s_vec3* diffuse);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shading_roughness`

<div class="api-signature">

```c
extern void se_sdf_set_shading_roughness(se_sdf_handle sdf, f32 roughness);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shading_smoothness`

<div class="api-signature">

```c
extern void se_sdf_set_shading_smoothness(se_sdf_handle sdf, f32 smoothness);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shading_specular`

<div class="api-signature">

```c
extern void se_sdf_set_shading_specular(se_sdf_handle sdf, const s_vec3* specular);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shadow`

<div class="api-signature">

```c
extern void se_sdf_set_shadow(se_sdf_handle sdf, const se_sdf_shadow* shadow);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shadow_bias`

<div class="api-signature">

```c
extern void se_sdf_set_shadow_bias(se_sdf_handle sdf, f32 bias);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shadow_samples`

<div class="api-signature">

```c
extern void se_sdf_set_shadow_samples(se_sdf_handle sdf, u16 samples);
```

</div>

No inline description found in header comments.

### `se_sdf_set_shadow_softness`

<div class="api-signature">

```c
extern void se_sdf_set_shadow_softness(se_sdf_handle sdf, f32 softness);
```

</div>

No inline description found in header comments.

### `se_sdf_set_transform`

<div class="api-signature">

```c
extern void se_sdf_set_transform(se_sdf_handle sdf, const s_mat4* transform);
```

</div>

No inline description found in header comments.

### `se_sdf_to_json`

<div class="api-signature">

```c
extern s_json* se_sdf_to_json(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

## Enums

### `se_sdf_operator`

<div class="api-signature">

```c
typedef enum se_sdf_operator { SE_SDF_UNION, SE_SDF_SMOOTH_UNION, //... } se_sdf_operator;
```

</div>

No inline description found in header comments.

### `se_sdf_type`

<div class="api-signature">

```c
typedef enum se_sdf_type { SE_SDF_CUSTOM, SE_SDF_SPHERE, SE_SDF_BOX, //... } se_sdf_type;
```

</div>

No inline description found in header comments.

## Typedefs

### `s_json`

<div class="api-signature">

```c
typedef struct s_json s_json;
```

</div>

No inline description found in header comments.

### `se_sdf`

<div class="api-signature">

```c
typedef struct se_sdf { s_mat4 transform; se_sdf_type type; se_sdf_operator operation; f32 operation_amount; se_sdf_shading shading; se_sdf_shadow shadow; union { struct { f32 radius; } sphere; struct { s_vec3 size; } box; }; // nodes/scene // don't set manually se_sdf_handle parent; s_array(se_sdf_handle, children); s_array(se_texture_handle, noises); s_array(se_sdf_point_light_handle, point_lights); s_array(se_sdf_directional_light_handle, directional_lights); se_texture_handle volume; se_quad quad; se_shader_handle shader; se_framebuffer_handle output; } se_sdf;
```

</div>

No inline description found in header comments.

### `se_sdf_directional_light`

<div class="api-signature">

```c
typedef struct se_sdf_directional_light { s_vec3 direction; s_vec3 color; } se_sdf_directional_light;
```

</div>

No inline description found in header comments.

### `se_sdf_point_light`

<div class="api-signature">

```c
typedef struct se_sdf_point_light { s_vec3 position; s_vec3 color; f32 radius; } se_sdf_point_light;
```

</div>

No inline description found in header comments.

### `se_sdf_shading`

<div class="api-signature">

```c
typedef struct se_sdf_shading { s_vec3 ambient; s_vec3 diffuse; s_vec3 specular; f32 roughness; f32 bias; f32 smoothness; } se_sdf_shading;
```

</div>

No inline description found in header comments.

### `se_sdf_shadow`

<div class="api-signature">

```c
typedef struct se_sdf_shadow { f32 softness; f32 bias; u16 samples; } se_sdf_shadow;
```

</div>

No inline description found in header comments.
