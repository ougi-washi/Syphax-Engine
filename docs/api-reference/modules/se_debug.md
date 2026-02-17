<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_debug.h

Source header: `include/se_debug.h`

## Overview

Logging, trace spans, overlays, and frame timing diagnostics.

This page is generated from `include/se_debug.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/debug.md)

## Functions

### `se_debug_clear_trace_events`

<div class="api-signature">

```c
extern void se_debug_clear_trace_events(void);
```

</div>

No inline description found in header comments.

### `se_debug_collect_stats`

<div class="api-signature">

```c
extern b8 se_debug_collect_stats(const se_window_handle window, se_input_handle* input, se_debug_system_stats* out_stats);
```

</div>

No inline description found in header comments.

### `se_debug_dump_last_frame_timing`

<div class="api-signature">

```c
extern void se_debug_dump_last_frame_timing(c8* out_buffer, const sz out_buffer_size);
```

</div>

No inline description found in header comments.

### `se_debug_dump_last_frame_timing_lines`

<div class="api-signature">

```c
extern void se_debug_dump_last_frame_timing_lines(c8* out_buffer, const sz out_buffer_size);
```

</div>

No inline description found in header comments.

### `se_debug_frame_begin`

<div class="api-signature">

```c
extern void se_debug_frame_begin(void);
```

</div>

No inline description found in header comments.

### `se_debug_frame_end`

<div class="api-signature">

```c
extern void se_debug_frame_end(void);
```

</div>

No inline description found in header comments.

### `se_debug_get_category_mask`

<div class="api-signature">

```c
extern u32 se_debug_get_category_mask(void);
```

</div>

No inline description found in header comments.

### `se_debug_get_last_frame_timing`

<div class="api-signature">

```c
extern b8 se_debug_get_last_frame_timing(se_debug_frame_timing* out_timing);
```

</div>

No inline description found in header comments.

### `se_debug_get_level`

<div class="api-signature">

```c
extern se_debug_level se_debug_get_level(void);
```

</div>

No inline description found in header comments.

### `se_debug_get_trace_events`

<div class="api-signature">

```c
extern b8 se_debug_get_trace_events(const se_debug_trace_event** out_events, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_debug_is_overlay_enabled`

<div class="api-signature">

```c
extern b8 se_debug_is_overlay_enabled(void);
```

</div>

No inline description found in header comments.

### `se_debug_log`

<div class="api-signature">

```c
extern void se_debug_log(const se_debug_level level, const se_debug_category category, const c8* fmt, ...);
```

</div>

No inline description found in header comments.

### `se_debug_render_overlay`

<div class="api-signature">

```c
extern void se_debug_render_overlay(const se_window_handle window, se_input_handle* input);
```

</div>

No inline description found in header comments.

### `se_debug_set_category_mask`

<div class="api-signature">

```c
extern void se_debug_set_category_mask(const u32 category_mask);
```

</div>

No inline description found in header comments.

### `se_debug_set_level`

<div class="api-signature">

```c
extern void se_debug_set_level(const se_debug_level level);
```

</div>

No inline description found in header comments.

### `se_debug_set_log_callback`

<div class="api-signature">

```c
extern void se_debug_set_log_callback(se_debug_log_callback callback, void* user_data);
```

</div>

No inline description found in header comments.

### `se_debug_set_overlay_enabled`

<div class="api-signature">

```c
extern void se_debug_set_overlay_enabled(const b8 enabled);
```

</div>

No inline description found in header comments.

### `se_debug_trace_begin`

<div class="api-signature">

```c
extern void se_debug_trace_begin(const c8* name);
```

</div>

No inline description found in header comments.

### `se_debug_trace_end`

<div class="api-signature">

```c
extern void se_debug_trace_end(const c8* name);
```

</div>

No inline description found in header comments.

### `se_debug_validate`

<div class="api-signature">

```c
extern b8 se_debug_validate(const b8 condition, const se_result error_code, const c8* expression, const c8* file, const i32 line);
```

</div>

No inline description found in header comments.

### `se_log`

<div class="api-signature">

```c
extern void se_log(const c8* fmt, ...);
```

</div>

No inline description found in header comments.

## Enums

### `se_debug_category`

<div class="api-signature">

```c
typedef enum { SE_DEBUG_CATEGORY_WINDOW = 1 << 0, SE_DEBUG_CATEGORY_INPUT = 1 << 1, SE_DEBUG_CATEGORY_CAMERA = 1 << 2, SE_DEBUG_CATEGORY_UI = 1 << 3, SE_DEBUG_CATEGORY_RENDER = 1 << 4, SE_DEBUG_CATEGORY_NAVIGATION = 1 << 5, SE_DEBUG_CATEGORY_SCENE = 1 << 6, SE_DEBUG_CATEGORY_CORE = 1 << 7, SE_DEBUG_CATEGORY_ALL = 0xFFFFFFFFu } se_debug_category;
```

</div>

No inline description found in header comments.

### `se_debug_level`

<div class="api-signature">

```c
typedef enum { SE_DEBUG_LEVEL_TRACE = 0, SE_DEBUG_LEVEL_DEBUG, SE_DEBUG_LEVEL_INFO, SE_DEBUG_LEVEL_WARN, SE_DEBUG_LEVEL_ERROR, SE_DEBUG_LEVEL_NONE } se_debug_level;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_debug_frame_timing`

<div class="api-signature">

```c
typedef struct { u64 frame_index; f64 frame_ms; f64 window_update_ms; f64 input_ms; f64 scene2d_ms; f64 scene3d_ms; f64 text_ms; f64 ui_ms; f64 navigation_ms; f64 window_present_ms; f64 other_ms; } se_debug_frame_timing;
```

</div>

No inline description found in header comments.

### `se_debug_log_callback`

<div class="api-signature">

```c
typedef void (*se_debug_log_callback)(se_debug_level level, se_debug_category category, const c8* message, void* user_data);
```

</div>

No inline description found in header comments.

### `se_debug_system_stats`

<div class="api-signature">

```c
typedef struct { se_window_diagnostics window; se_input_diagnostics input; u32 window_count; u32 camera_count; u32 scene2d_count; u32 scene3d_count; u32 object2d_count; u32 object3d_count; u32 ui_element_count; u32 navigation_trace_count; } se_debug_system_stats;
```

</div>

No inline description found in header comments.

### `se_debug_trace_event`

<div class="api-signature">

```c
typedef struct { c8 name[64]; f64 begin_time; f64 end_time; f64 duration; b8 active : 1; } se_debug_trace_event;
```

</div>

No inline description found in header comments.
