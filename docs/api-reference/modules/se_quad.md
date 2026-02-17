<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_quad.h

Source header: `include/se_quad.h`

## Overview

Quad and mesh instance utility structs.

This page is generated from `include/se_quad.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/utilities.md)

## Functions

### `se_quad_2d_add_instance_buffer`

<div class="api-signature">

```c
extern void se_quad_2d_add_instance_buffer(se_quad *quad, const s_mat4 *buffer, const sz instance_count);
```

</div>

No inline description found in header comments.

### `se_quad_2d_add_instance_buffer_mat3`

<div class="api-signature">

```c
extern void se_quad_2d_add_instance_buffer_mat3(se_quad *quad, const s_mat3 *buffer, const sz instance_count);
```

</div>

No inline description found in header comments.

### `se_quad_2d_create`

<div class="api-signature">

```c
extern void se_quad_2d_create(se_quad *out_quad, const u32 instance_count);
```

</div>

No inline description found in header comments.

### `se_quad_3d_create`

<div class="api-signature">

```c
extern void se_quad_3d_create(se_quad *out_quad);
```

</div>

Quad functions

### `se_quad_destroy`

<div class="api-signature">

```c
extern void se_quad_destroy(se_quad *quad);
```

</div>

No inline description found in header comments.

### `se_quad_render`

<div class="api-signature">

```c
extern void se_quad_render(se_quad *quad, const sz instance_count);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_instance_buffer`

<div class="api-signature">

```c
typedef struct { u32 vbo; const void *buffer_ptr; sz buffer_size; } se_instance_buffer;
```

</div>

No inline description found in header comments.

### `se_mesh_instance`

<div class="api-signature">

```c
typedef struct { u32 vao; s_array(se_instance_buffer, instance_buffers); b8 instance_buffers_dirty; } se_mesh_instance;
```

</div>

No inline description found in header comments.

### `se_quad`

<div class="api-signature">

```c
typedef struct { u32 vao; u32 ebo; u32 vbo; s_array(se_instance_buffer, instance_buffers); b8 instance_buffers_dirty; u32 last_attribute; } se_quad;
```

</div>

No inline description found in header comments.

### `se_vertex_2d`

<div class="api-signature">

```c
typedef struct { s_vec2 position; s_vec2 uv; } se_vertex_2d;
```

</div>

No inline description found in header comments.

### `se_vertex_3d`

<div class="api-signature">

```c
typedef struct { s_vec3 position; s_vec3 normal; s_vec2 uv; } se_vertex_3d;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_mesh_instance, se_mesh_instances);
```

</div>

No inline description found in header comments.
