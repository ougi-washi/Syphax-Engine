<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_audio.h

Source header: `include/se_audio.h`

## Overview

Audio engine setup, clips, streams, buses, and capture APIs.

This page is generated from `include/se_audio.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/audio.md)

## Functions

### `se_audio_buffer_analyze_bands`

<div class="api-signature">

```c
extern b8 se_audio_buffer_analyze_bands(const f32* samples, sz frame_count, u32 channels, u32 sample_rate, se_audio_band_levels* out_levels);
```

</div>

No inline description found in header comments.

### `se_audio_bus_get_volume`

<div class="api-signature">

```c
extern f32 se_audio_bus_get_volume(se_audio_engine* engine, se_audio_bus bus);
```

</div>

No inline description found in header comments.

### `se_audio_bus_set_volume`

<div class="api-signature">

```c
extern void se_audio_bus_set_volume(se_audio_engine* engine, se_audio_bus bus, f32 volume);
```

</div>

No inline description found in header comments.

### `se_audio_capture_get_bands`

<div class="api-signature">

```c
extern b8 se_audio_capture_get_bands(se_audio_capture* capture, se_audio_band_levels* out_levels);
```

</div>

No inline description found in header comments.

### `se_audio_capture_list_devices`

<div class="api-signature">

```c
extern u32 se_audio_capture_list_devices(se_audio_engine* engine, se_audio_device_info* out_devices, u32 max_devices);
```

</div>

No inline description found in header comments.

### `se_audio_capture_read`

<div class="api-signature">

```c
extern sz se_audio_capture_read(se_audio_capture* capture, f32* dst_frames, sz max_frames);
```

</div>

No inline description found in header comments.

### `se_audio_capture_start`

<div class="api-signature">

```c
extern se_audio_capture* se_audio_capture_start(se_audio_engine* engine, const se_audio_capture_config* config);
```

</div>

No inline description found in header comments.

### `se_audio_capture_stop`

<div class="api-signature">

```c
extern void se_audio_capture_stop(se_audio_capture* capture);
```

</div>

No inline description found in header comments.

### `se_audio_clip_get_bands`

<div class="api-signature">

```c
extern b8 se_audio_clip_get_bands(const se_audio_clip* clip, se_audio_band_levels* out_levels, sz max_frames);
```

</div>

No inline description found in header comments.

### `se_audio_clip_is_playing`

<div class="api-signature">

```c
extern b8 se_audio_clip_is_playing(const se_audio_clip* clip);
```

</div>

No inline description found in header comments.

### `se_audio_clip_load`

<div class="api-signature">

```c
extern se_audio_clip* se_audio_clip_load(se_audio_engine* engine, const char* relative_path);
```

</div>

No inline description found in header comments.

### `se_audio_clip_play`

<div class="api-signature">

```c
extern b8 se_audio_clip_play(se_audio_clip* clip, const se_audio_play_params* params);
```

</div>

No inline description found in header comments.

### `se_audio_clip_set_pan`

<div class="api-signature">

```c
extern void se_audio_clip_set_pan(se_audio_clip* clip, f32 pan);
```

</div>

No inline description found in header comments.

### `se_audio_clip_set_volume`

<div class="api-signature">

```c
extern void se_audio_clip_set_volume(se_audio_clip* clip, f32 volume);
```

</div>

No inline description found in header comments.

### `se_audio_clip_stop`

<div class="api-signature">

```c
extern void se_audio_clip_stop(se_audio_clip* clip);
```

</div>

No inline description found in header comments.

### `se_audio_clip_unload`

<div class="api-signature">

```c
extern void se_audio_clip_unload(se_audio_engine* engine, se_audio_clip* clip);
```

</div>

No inline description found in header comments.

### `se_audio_init`

<div class="api-signature">

```c
extern se_audio_engine* se_audio_init(const se_audio_config* config);
```

</div>

No inline description found in header comments.

### `se_audio_shutdown`

<div class="api-signature">

```c
extern void se_audio_shutdown(se_audio_engine* engine);
```

</div>

No inline description found in header comments.

### `se_audio_stop_all`

<div class="api-signature">

```c
extern void se_audio_stop_all(se_audio_engine* engine);
```

</div>

No inline description found in header comments.

### `se_audio_stream_close`

<div class="api-signature">

```c
extern void se_audio_stream_close(se_audio_stream* stream);
```

</div>

No inline description found in header comments.

### `se_audio_stream_get_cursor`

<div class="api-signature">

```c
extern f64 se_audio_stream_get_cursor(const se_audio_stream* stream);
```

</div>

No inline description found in header comments.

### `se_audio_stream_open`

<div class="api-signature">

```c
extern se_audio_stream* se_audio_stream_open(se_audio_engine* engine, const char* relative_path, const se_audio_play_params* params);
```

</div>

No inline description found in header comments.

### `se_audio_stream_seek`

<div class="api-signature">

```c
extern void se_audio_stream_seek(se_audio_stream* stream, f64 seconds);
```

</div>

No inline description found in header comments.

### `se_audio_stream_set_looping`

<div class="api-signature">

```c
extern void se_audio_stream_set_looping(se_audio_stream* stream, b8 looping);
```

</div>

No inline description found in header comments.

### `se_audio_stream_set_volume`

<div class="api-signature">

```c
extern void se_audio_stream_set_volume(se_audio_stream* stream, f32 volume);
```

</div>

No inline description found in header comments.

### `se_audio_update`

<div class="api-signature">

```c
extern void se_audio_update(se_audio_engine* engine);
```

</div>

No inline description found in header comments.

## Enums

### `se_audio_bus`

<div class="api-signature">

```c
typedef enum { SE_AUDIO_BUS_MASTER = 0, SE_AUDIO_BUS_MUSIC, SE_AUDIO_BUS_SFX, SE_AUDIO_BUS_COUNT } se_audio_bus;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_audio_band_levels`

<div class="api-signature">

```c
typedef struct { f32 low; f32 mid; f32 high; } se_audio_band_levels;
```

</div>

No inline description found in header comments.

### `se_audio_capture`

<div class="api-signature">

```c
typedef struct se_audio_capture se_audio_capture;
```

</div>

No inline description found in header comments.

### `se_audio_capture_config`

<div class="api-signature">

```c
typedef struct { u32 device_index; u32 sample_rate; u32 channels; u32 frames_per_buffer; } se_audio_capture_config;
```

</div>

No inline description found in header comments.

### `se_audio_clip`

<div class="api-signature">

```c
typedef struct se_audio_clip se_audio_clip;
```

</div>

No inline description found in header comments.

### `se_audio_config`

<div class="api-signature">

```c
typedef struct { u32 sample_rate; u32 channels; u32 max_clips; u32 max_streams; u32 max_captures; b8 enable_capture : 1; } se_audio_config;
```

</div>

No inline description found in header comments.

### `se_audio_device_info`

<div class="api-signature">

```c
typedef struct { char name[SE_AUDIO_DEVICE_NAME_MAX]; u32 index; b8 is_default : 1; } se_audio_device_info;
```

</div>

No inline description found in header comments.

### `se_audio_engine`

<div class="api-signature">

```c
typedef struct se_audio_engine se_audio_engine;
```

</div>

No inline description found in header comments.

### `se_audio_play_params`

<div class="api-signature">

```c
typedef struct { f32 volume; f32 pan; f32 pitch; b8 looping : 1; se_audio_bus bus; } se_audio_play_params;
```

</div>

No inline description found in header comments.

### `se_audio_stream`

<div class="api-signature">

```c
typedef struct se_audio_stream se_audio_stream;
```

</div>

No inline description found in header comments.
