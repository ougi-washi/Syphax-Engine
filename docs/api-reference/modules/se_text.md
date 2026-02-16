<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_text.h

Source header: `include/se_text.h`

## Overview

Font loading and text drawing APIs.

This page is generated from `include/se_text.h` and is deterministic.

## Read the Playbook

- [Deep dive Playbook](../../playbooks/se-text.md)

## Functions

### `se_font_destroy`

<div class="api-signature">

```c
extern void se_font_destroy(const se_font_handle font);
```

</div>

No inline description found in header comments.

### `se_font_load`

<div class="api-signature">

```c
extern se_font_handle se_font_load(se_text_handle* text_handle, const char* path, const f32 size);
```

</div>

No inline description found in header comments.

### `se_text_handle_create`

<div class="api-signature">

```c
extern se_text_handle* se_text_handle_create(const u32 fonts_count);
```

</div>

Font && text functions

### `se_text_handle_destroy`

<div class="api-signature">

```c
extern void se_text_handle_destroy(se_text_handle* text_handle);
```

</div>

No inline description found in header comments.

### `se_text_render`

<div class="api-signature">

```c
extern void se_text_render(se_text_handle* text_handle, const se_font_handle font, const c8* text, const s_vec2* position, const s_vec2* size, const f32 new_line_offset);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_font`

<div class="api-signature">

```c
typedef struct se_font { s_array(c8, path); u32 atlas_texture; f32 size; u16 atlas_width, atlas_height; u16 first_character, characters_count; s_array(stbtt_packedchar, packed_characters); s_array(stbtt_aligned_quad, aligned_quads); } se_font;
```

</div>

No inline description found in header comments.

### `se_text_handle`

<div class="api-signature">

```c
typedef struct se_text_handle { se_quad quad; s_mat4 buffer[SE_TEXT_CHAR_COUNT]; se_shader_handle text_shader; i32 text_vertex_index; // ? se_context* ctx; } se_text_handle;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_font, se_fonts);
```

</div>

No inline description found in header comments.
