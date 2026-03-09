<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_texture.h

Source header: `include/se_texture.h`

## Overview

Texture loading from files or memory and texture parameter control.

This page is generated from `include/se_texture.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/texture.md)

## Functions

### `se_texture_create_3d_r16f`

<div class="api-signature">

```c
extern se_texture_handle se_texture_create_3d_r16f(const f32 *data, const u32 width, const u32 height, const u32 depth, const se_texture_wrap wrap);
```

</div>

No inline description found in header comments.

### `se_texture_create_3d_rgba16f`

<div class="api-signature">

```c
extern se_texture_handle se_texture_create_3d_rgba16f(const f32 *data, const u32 width, const u32 height, const u32 depth, const se_texture_wrap wrap);
```

</div>

No inline description found in header comments.

### `se_texture_destroy`

<div class="api-signature">

```c
extern void se_texture_destroy(const se_texture_handle texture);
```

</div>

No inline description found in header comments.

### `se_texture_find_by_id`

<div class="api-signature">

```c
extern se_texture_handle se_texture_find_by_id(const u32 texture_id);
```

</div>

No inline description found in header comments.

### `se_texture_load`

<div class="api-signature">

```c
extern se_texture_handle se_texture_load(const char *path, const se_texture_wrap wrap);
```

</div>

No inline description found in header comments.

### `se_texture_load_from_memory`

<div class="api-signature">

```c
extern se_texture_handle se_texture_load_from_memory(const u8 *data, const sz size, const se_texture_wrap wrap);
```

</div>

No inline description found in header comments.

### `se_texture_sample_rgb`

<div class="api-signature">

```c
extern b8 se_texture_sample_rgb(const se_texture_handle texture, const s_vec2 *uv, s_vec3 *out_color);
```

</div>

No inline description found in header comments.

## Enums

### `se_texture_wrap`

<div class="api-signature">

```c
typedef enum { SE_REPEAT, SE_CLAMP } se_texture_wrap;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_texture`

<div class="api-signature">

```c
typedef struct se_texture { char path[SE_MAX_PATH_LENGTH]; u32 id; i32 width; i32 height; i32 depth; i32 channels; i32 cpu_channels; u32 target; se_texture_wrap wrap; u8 *cpu_pixels; sz cpu_pixels_size; } se_texture;
```

</div>

No inline description found in header comments.

### `se_texture_ptr`

<div class="api-signature">

```c
typedef se_texture_handle se_texture_ptr;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_texture, se_textures);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_texture_handle, se_textures_ptr);
```

</div>

No inline description found in header comments.
