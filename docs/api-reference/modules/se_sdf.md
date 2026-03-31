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
extern se_sdf_noise_handle se_sdf_add_noise_internal(se_sdf_handle sdf, const se_sdf_noise noise);
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

### `se_sdf_render`

<div class="api-signature">

```c
extern void se_sdf_render(se_sdf_handle sdf, se_camera_handle camera);
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

## Enums

### `se_sdf_noise_type`

<div class="api-signature">

```c
typedef enum se_sdf_noise_type { SE_SDF_NOISE_NONE, SE_SDF_NOISE_PERLIN, SE_SDF_NOISE_VORNOI, //... } se_sdf_noise_type;
```

</div>

No inline description found in header comments.

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

### `se_sdf`

<div class="api-signature">

```c
typedef struct se_sdf { s_mat4 transform; se_sdf_type type; se_sdf_operator operation; f32 operation_amount; se_sdf_shading shading; se_sdf_shadow shadow; union { struct { f32 radius; } sphere; struct { s_vec3 size; } box; }; // nodes/scene // don't set manually se_sdf_handle parent; s_array(se_sdf_handle, children); s_array(se_sdf_noise_handle, noises); s_array(se_sdf_point_light_handle, point_lights); s_array(se_sdf_directional_light_handle, directional_lights); se_quad quad; se_shader_handle shader; se_texture_handle volume; } se_sdf;
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

### `se_sdf_noise`

<div class="api-signature">

```c
typedef struct se_sdf_noise { se_sdf_noise_type type; f32 frequency; s_vec3 offset; } se_sdf_noise;
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
