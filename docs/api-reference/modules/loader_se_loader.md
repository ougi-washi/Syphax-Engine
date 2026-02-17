<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# loader/se_loader.h

Source header: `include/loader/se_loader.h`

## Overview

General loader helpers used by model and asset loading paths.

This page is generated from `include/loader/se_loader.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/loader.md)

## Functions

### `se_gltf_image_load`

<div class="api-signature">

```c
extern se_texture_handle se_gltf_image_load(const se_gltf_asset *asset, const i32 image_index, const se_texture_wrap wrap);
```

</div>

No inline description found in header comments.

### `se_gltf_model_load`

<div class="api-signature">

```c
extern se_model_handle se_gltf_model_load(const se_gltf_asset *asset, const i32 mesh_index);
```

</div>

No inline description found in header comments.

### `se_gltf_model_load_ex`

<div class="api-signature">

```c
extern se_model_handle se_gltf_model_load_ex(const se_gltf_asset *asset, const i32 mesh_index, const se_mesh_data_flags mesh_data_flags);
```

</div>

No inline description found in header comments.

### `se_gltf_model_load_first`

<div class="api-signature">

```c
extern se_model_handle se_gltf_model_load_first(const char *path, const se_gltf_load_params *params);
```

</div>

No inline description found in header comments.

### `se_gltf_scene_compute_bounds`

<div class="api-signature">

```c
extern b8 se_gltf_scene_compute_bounds(const se_gltf_asset *asset, s_vec3 *out_center, f32 *out_radius);
```

</div>

No inline description found in header comments.

### `se_gltf_scene_fit_camera`

<div class="api-signature">

```c
extern void se_gltf_scene_fit_camera(const se_scene_3d_handle scene, const se_gltf_asset *asset);
```

</div>

No inline description found in header comments.

### `se_gltf_scene_get_navigation_speeds`

<div class="api-signature">

```c
extern void se_gltf_scene_get_navigation_speeds(const se_gltf_asset *asset, f32 *out_base_speed, f32 *out_fast_speed);
```

</div>

No inline description found in header comments.

### `se_gltf_scene_load`

<div class="api-signature">

```c
extern se_gltf_asset *se_gltf_scene_load(const se_scene_3d_handle scene, const char *path, const se_gltf_load_params *load_params, const se_shader_handle mesh_shader, const se_texture_handle default_texture, const se_texture_wrap wrap, sz *out_objects_loaded);
```

</div>

No inline description found in header comments.

### `se_gltf_scene_load_from_asset`

<div class="api-signature">

```c
extern sz se_gltf_scene_load_from_asset(const se_scene_3d_handle scene, const se_gltf_asset *asset, const se_shader_handle mesh_shader, const se_texture_handle default_texture, const se_texture_wrap wrap);
```

</div>

No inline description found in header comments.

### `se_gltf_texture_load`

<div class="api-signature">

```c
extern se_texture_handle se_gltf_texture_load(const se_gltf_asset *asset, const i32 texture_index, const se_texture_wrap wrap);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

No typedefs found in this header.
