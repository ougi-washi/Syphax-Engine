<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_render_buffer.h

Source header: `include/se_render_buffer.h`

## Overview

Render-to-texture buffers and offscreen draw orchestration.

This page is generated from `include/se_render_buffer.h` and is deterministic.

## Read the Playbook

- [Deep dive Playbook](../../playbooks/se-render-buffer.md)

## Functions

### `se_render_buffer_bind`

<div class="api-signature">

```c
extern void se_render_buffer_bind(const se_render_buffer_handle buffer);
```

</div>

No inline description found in header comments.

### `se_render_buffer_cleanup`

<div class="api-signature">

```c
extern void se_render_buffer_cleanup(const se_render_buffer_handle buffer);
```

</div>

No inline description found in header comments.

### `se_render_buffer_create`

<div class="api-signature">

```c
extern se_render_buffer_handle se_render_buffer_create(const u32 width, const u32 height, const c8 *fragment_shader_path);
```

</div>

No inline description found in header comments.

### `se_render_buffer_destroy`

<div class="api-signature">

```c
extern void se_render_buffer_destroy(const se_render_buffer_handle buffer);
```

</div>

No inline description found in header comments.

### `se_render_buffer_set_position`

<div class="api-signature">

```c
extern void se_render_buffer_set_position(const se_render_buffer_handle buffer, const s_vec2 *position);
```

</div>

No inline description found in header comments.

### `se_render_buffer_set_scale`

<div class="api-signature">

```c
extern void se_render_buffer_set_scale(const se_render_buffer_handle buffer, const s_vec2 *scale);
```

</div>

No inline description found in header comments.

### `se_render_buffer_set_shader`

<div class="api-signature">

```c
extern void se_render_buffer_set_shader(const se_render_buffer_handle buffer, const se_shader_handle shader);
```

</div>

No inline description found in header comments.

### `se_render_buffer_unbind`

<div class="api-signature">

```c
extern void se_render_buffer_unbind(const se_render_buffer_handle buffer);
```

</div>

No inline description found in header comments.

### `se_render_buffer_unset_shader`

<div class="api-signature">

```c
extern void se_render_buffer_unset_shader(const se_render_buffer_handle buffer);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_render_buffer`

<div class="api-signature">

```c
typedef struct se_render_buffer { u32 framebuffer; u32 texture; u32 prev_framebuffer; u32 prev_texture; u32 depth_buffer; s_vec2 texture_size; s_vec2 scale; s_vec2 position; se_shader_handle shader; } se_render_buffer;
```

</div>

No inline description found in header comments.

### `se_render_buffer_ptr`

<div class="api-signature">

```c
typedef se_render_buffer_handle se_render_buffer_ptr;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_render_buffer, se_render_buffers);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_render_buffer_handle, se_render_buffers_ptr);
```

</div>

No inline description found in header comments.
