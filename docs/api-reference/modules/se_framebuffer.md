<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_framebuffer.h

Source header: `include/se_framebuffer.h`

## Overview

Framebuffer creation, binding, and resize lifecycle.

This page is generated from `include/se_framebuffer.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/framebuffer.md)

## Functions

### `se_framebuffer_bind`

<div class="api-signature">

```c
extern void se_framebuffer_bind(const se_framebuffer_handle framebuffer);
```

</div>

No inline description found in header comments.

### `se_framebuffer_cleanup`

<div class="api-signature">

```c
extern void se_framebuffer_cleanup(const se_framebuffer_handle framebuffer);
```

</div>

No inline description found in header comments.

### `se_framebuffer_create`

<div class="api-signature">

```c
extern se_framebuffer_handle se_framebuffer_create(const s_vec2 *size);
```

</div>

No inline description found in header comments.

### `se_framebuffer_destroy`

<div class="api-signature">

```c
extern void se_framebuffer_destroy(const se_framebuffer_handle framebuffer);
```

</div>

No inline description found in header comments.

### `se_framebuffer_get_size`

<div class="api-signature">

```c
extern void se_framebuffer_get_size(const se_framebuffer_handle framebuffer, s_vec2 *out_size);
```

</div>

No inline description found in header comments.

### `se_framebuffer_get_texture_id`

<div class="api-signature">

```c
extern b8 se_framebuffer_get_texture_id(const se_framebuffer_handle framebuffer, u32 *out_texture);
```

</div>

No inline description found in header comments.

### `se_framebuffer_set_size`

<div class="api-signature">

```c
extern void se_framebuffer_set_size(const se_framebuffer_handle framebuffer, const s_vec2 *size);
```

</div>

No inline description found in header comments.

### `se_framebuffer_unbind`

<div class="api-signature">

```c
extern void se_framebuffer_unbind(const se_framebuffer_handle framebuffer);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_framebuffer`

<div class="api-signature">

```c
typedef struct se_framebuffer { u32 framebuffer; u32 texture; u32 depth_buffer; s_vec2 size; s_vec2 ratio; b8 auto_resize : 1; } se_framebuffer;
```

</div>

No inline description found in header comments.

### `se_framebuffer_ptr`

<div class="api-signature">

```c
typedef se_framebuffer_handle se_framebuffer_ptr;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_framebuffer, se_framebuffers);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_framebuffer_handle, se_framebuffers_ptr);
```

</div>

No inline description found in header comments.
