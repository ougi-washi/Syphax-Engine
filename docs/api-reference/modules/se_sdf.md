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

### `se_sdf_shutdown`

<div class="api-signature">

```c
extern void se_sdf_shutdown(void);
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
typedef struct se_sdf { s_mat4 transform; se_sdf_type type; union { struct { f32 radius; } sphere; struct { s_vec3 size; } box; }; se_sdf_noise noise_0; se_sdf_noise noise_1; se_sdf_noise noise_2; se_sdf_noise noise_3; // nodes/scene // don't set manually se_sdf_handle parent; s_array(se_sdf_handle, children); se_quad quad; se_shader_handle shader; se_framebuffer_handle output; se_texture_handle volume; } se_sdf;
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
