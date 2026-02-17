<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_render_thread.h

Source header: `include/se_render_thread.h`

## Overview

Dedicated render-thread lifecycle and diagnostics APIs.

This page is generated from `include/se_render_thread.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/window.md)

## Functions

### `se_render_thread_get_diagnostics`

<div class="api-signature">

```c
extern b8 se_render_thread_get_diagnostics(se_render_thread_handle thread, se_render_thread_diagnostics* out_diag);
```

</div>

No inline description found in header comments.

### `se_render_thread_is_running`

<div class="api-signature">

```c
extern b8 se_render_thread_is_running(se_render_thread_handle thread);
```

</div>

No inline description found in header comments.

### `se_render_thread_start`

<div class="api-signature">

```c
extern se_render_thread_handle se_render_thread_start(se_window_handle window, const se_render_thread_config* config);
```

</div>

No inline description found in header comments.

### `se_render_thread_stop`

<div class="api-signature">

```c
extern void se_render_thread_stop(se_render_thread_handle thread);
```

</div>

No inline description found in header comments.

### `se_render_thread_wait_idle`

<div class="api-signature">

```c
extern void se_render_thread_wait_idle(se_render_thread_handle thread);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_render_thread_config`

<div class="api-signature">

```c
typedef struct { u32 max_commands_per_frame; u32 max_command_bytes_per_frame; b8 wait_on_submit; } se_render_thread_config;
```

</div>

No inline description found in header comments.

### `se_render_thread_diagnostics`

<div class="api-signature">

```c
typedef struct { b8 running; b8 stopping; b8 failed; u64 submitted_frames; u64 presented_frames; u64 submit_stalls; u32 queue_depth; u64 last_command_count; u64 last_command_bytes; f64 last_submit_wait_ms; f64 last_execute_ms; f64 last_present_ms; } se_render_thread_diagnostics;
```

</div>

No inline description found in header comments.

### `se_render_thread_handle`

<div class="api-signature">

```c
typedef s_handle se_render_thread_handle;
```

</div>

No inline description found in header comments.
