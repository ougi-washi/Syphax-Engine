<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_render_frame.h

Source header: `include/se_render_frame.h`

## Overview

Dedicated render-frame submission and frame-queue statistics APIs.

This page is generated from `include/se_render_frame.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/window.md)

## Functions

### `se_render_frame_begin`

<div class="api-signature">

```c
extern void se_render_frame_begin(se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_render_frame_get_stats`

<div class="api-signature">

```c
extern b8 se_render_frame_get_stats(se_window_handle window, se_render_frame_stats* out_stats);
```

</div>

No inline description found in header comments.

### `se_render_frame_submit`

<div class="api-signature">

```c
extern void se_render_frame_submit(se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_render_frame_wait_presented`

<div class="api-signature">

```c
extern void se_render_frame_wait_presented(se_window_handle window);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_render_frame_stats`

<div class="api-signature">

```c
typedef struct { u64 submitted_frames; u64 presented_frames; u64 submit_stalls; u32 queue_depth; u64 last_command_count; u64 last_command_bytes; f64 last_submit_wait_ms; f64 last_execute_ms; f64 last_present_ms; } se_render_frame_stats;
```

</div>

No inline description found in header comments.
