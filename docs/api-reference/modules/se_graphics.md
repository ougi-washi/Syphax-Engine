<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_graphics.h

Source header: `include/se_graphics.h`

## Overview

Render lifecycle helpers such as clear, blending, and context state.

This page is generated from `include/se_graphics.h` and is deterministic.

## Functions

### `se_render_clear`

<div class="api-signature">

```c
extern void se_render_clear(void);
```

</div>

No inline description found in header comments.

### `se_render_get_generation`

<div class="api-signature">

```c
extern u64 se_render_get_generation(void);
```

</div>

No inline description found in header comments.

### `se_render_has_context`

<div class="api-signature">

```c
extern b8 se_render_has_context(void);
```

</div>

No inline description found in header comments.

### `se_render_init`

<div class="api-signature">

```c
extern b8 se_render_init(void);
```

</div>

Graphics helper functions

### `se_render_print_mat4`

<div class="api-signature">

```c
extern void se_render_print_mat4(const s_mat4 *mat);
```

</div>

Utility functions

### `se_render_set_background_color`

<div class="api-signature">

```c
extern void se_render_set_background_color(const s_vec4 color);
```

</div>

No inline description found in header comments.

### `se_render_set_blending`

<div class="api-signature">

```c
extern void se_render_set_blending(const b8 active);
```

</div>

No inline description found in header comments.

### `se_render_shutdown`

<div class="api-signature">

```c
extern void se_render_shutdown(void);
```

</div>

No inline description found in header comments.

### `function`

<div class="api-signature">

```c
extern void se_render_unbind_framebuffer(void); // window framebuffer
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

No typedefs found in this header.
