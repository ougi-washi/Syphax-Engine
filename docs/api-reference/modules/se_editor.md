<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_editor.h

Source header: `include/se_editor.h`

## Overview

Editor inspection, diagnostics, and runtime command APIs.

This page is generated from `include/se_editor.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/utilities.md)

## Functions

### `se_editor_apply_command`

<div class="api-signature">

```c
extern b8 se_editor_apply_command(se_editor* editor, const se_editor_command* command);
```

</div>

No inline description found in header comments.

### `se_editor_apply_commands`

<div class="api-signature">

```c
extern b8 se_editor_apply_commands(se_editor* editor, const se_editor_command* commands, sz command_count, u32* out_applied);
```

</div>

No inline description found in header comments.

### `se_editor_apply_queue`

<div class="api-signature">

```c
extern b8 se_editor_apply_queue(se_editor* editor, u32* out_applied, b8 clear_after_apply);
```

</div>

No inline description found in header comments.

### `se_editor_category_name`

<div class="api-signature">

```c
extern const c8* se_editor_category_name(se_editor_category category);
```

</div>

No inline description found in header comments.

### `se_editor_clear_queue`

<div class="api-signature">

```c
extern void se_editor_clear_queue(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_collect_counts`

<div class="api-signature">

```c
extern b8 se_editor_collect_counts(se_editor* editor, se_editor_counts* out_counts);
```

</div>

No inline description found in header comments.

### `se_editor_collect_diagnostics`

<div class="api-signature">

```c
extern b8 se_editor_collect_diagnostics(se_editor* editor, se_editor_diagnostics* out_diagnostics);
```

</div>

No inline description found in header comments.

### `se_editor_collect_items`

<div class="api-signature">

```c
extern b8 se_editor_collect_items(se_editor* editor, se_editor_category_mask category_mask, const se_editor_item** out_items, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_editor_collect_properties`

<div class="api-signature">

```c
extern b8 se_editor_collect_properties(se_editor* editor, const se_editor_item* item, const se_editor_property** out_properties, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_editor_create`

<div class="api-signature">

```c
extern se_editor* se_editor_create(const se_editor_config* config);
```

</div>

No inline description found in header comments.

### `se_editor_destroy`

<div class="api-signature">

```c
extern void se_editor_destroy(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_audio_engine`

<div class="api-signature">

```c
extern se_audio_engine* se_editor_get_audio_engine(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_context`

<div class="api-signature">

```c
extern se_context* se_editor_get_context(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_focused_simulation`

<div class="api-signature">

```c
extern se_simulation_handle se_editor_get_focused_simulation(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_focused_vfx_2d`

<div class="api-signature">

```c
extern se_vfx_2d_handle se_editor_get_focused_vfx_2d(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_focused_vfx_3d`

<div class="api-signature">

```c
extern se_vfx_3d_handle se_editor_get_focused_vfx_3d(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_input`

<div class="api-signature">

```c
extern se_input_handle* se_editor_get_input(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_navigation`

<div class="api-signature">

```c
extern se_navigation_grid* se_editor_get_navigation(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_physics_2d`

<div class="api-signature">

```c
extern se_physics_world_2d* se_editor_get_physics_2d(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_physics_3d`

<div class="api-signature">

```c
extern se_physics_world_3d* se_editor_get_physics_3d(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_get_queue`

<div class="api-signature">

```c
extern b8 se_editor_get_queue(const se_editor* editor, const se_editor_command** out_commands, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_editor_get_window`

<div class="api-signature">

```c
extern se_window_handle se_editor_get_window(const se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_queue_command`

<div class="api-signature">

```c
extern b8 se_editor_queue_command(se_editor* editor, const se_editor_command* command);
```

</div>

No inline description found in header comments.

### `se_editor_reset`

<div class="api-signature">

```c
extern void se_editor_reset(se_editor* editor);
```

</div>

No inline description found in header comments.

### `se_editor_set_audio_engine`

<div class="api-signature">

```c
extern void se_editor_set_audio_engine(se_editor* editor, se_audio_engine* audio);
```

</div>

No inline description found in header comments.

### `se_editor_set_context`

<div class="api-signature">

```c
extern void se_editor_set_context(se_editor* editor, se_context* context);
```

</div>

No inline description found in header comments.

### `se_editor_set_focused_simulation`

<div class="api-signature">

```c
extern void se_editor_set_focused_simulation(se_editor* editor, se_simulation_handle simulation);
```

</div>

No inline description found in header comments.

### `se_editor_set_focused_vfx_2d`

<div class="api-signature">

```c
extern void se_editor_set_focused_vfx_2d(se_editor* editor, se_vfx_2d_handle vfx);
```

</div>

No inline description found in header comments.

### `se_editor_set_focused_vfx_3d`

<div class="api-signature">

```c
extern void se_editor_set_focused_vfx_3d(se_editor* editor, se_vfx_3d_handle vfx);
```

</div>

No inline description found in header comments.

### `se_editor_set_input`

<div class="api-signature">

```c
extern void se_editor_set_input(se_editor* editor, se_input_handle* input);
```

</div>

No inline description found in header comments.

### `se_editor_set_navigation`

<div class="api-signature">

```c
extern void se_editor_set_navigation(se_editor* editor, se_navigation_grid* navigation);
```

</div>

No inline description found in header comments.

### `se_editor_set_physics_2d`

<div class="api-signature">

```c
extern void se_editor_set_physics_2d(se_editor* editor, se_physics_world_2d* world);
```

</div>

No inline description found in header comments.

### `se_editor_set_physics_3d`

<div class="api-signature">

```c
extern void se_editor_set_physics_3d(se_editor* editor, se_physics_world_3d* world);
```

</div>

No inline description found in header comments.

### `se_editor_set_window`

<div class="api-signature">

```c
extern void se_editor_set_window(se_editor* editor, se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_editor_track_audio_capture`

<div class="api-signature">

```c
extern b8 se_editor_track_audio_capture(se_editor* editor, se_audio_capture* capture, const c8* label);
```

</div>

No inline description found in header comments.

### `se_editor_track_audio_clip`

<div class="api-signature">

```c
extern b8 se_editor_track_audio_clip(se_editor* editor, se_audio_clip* clip, const c8* label);
```

</div>

No inline description found in header comments.

### `se_editor_track_audio_stream`

<div class="api-signature">

```c
extern b8 se_editor_track_audio_stream(se_editor* editor, se_audio_stream* stream, const c8* label);
```

</div>

No inline description found in header comments.

### `se_editor_untrack_audio_capture`

<div class="api-signature">

```c
extern b8 se_editor_untrack_audio_capture(se_editor* editor, se_audio_capture* capture);
```

</div>

No inline description found in header comments.

### `se_editor_untrack_audio_clip`

<div class="api-signature">

```c
extern b8 se_editor_untrack_audio_clip(se_editor* editor, se_audio_clip* clip);
```

</div>

No inline description found in header comments.

### `se_editor_untrack_audio_stream`

<div class="api-signature">

```c
extern b8 se_editor_untrack_audio_stream(se_editor* editor, se_audio_stream* stream);
```

</div>

No inline description found in header comments.

### `se_editor_value_type_name`

<div class="api-signature">

```c
extern const c8* se_editor_value_type_name(se_editor_value_type type);
```

</div>

No inline description found in header comments.

## Enums

### `se_editor_category`

<div class="api-signature">

```c
typedef enum { SE_EDITOR_CATEGORY_BACKEND = 0, SE_EDITOR_CATEGORY_DEBUG, SE_EDITOR_CATEGORY_WINDOW, SE_EDITOR_CATEGORY_INPUT, SE_EDITOR_CATEGORY_CAMERA, SE_EDITOR_CATEGORY_SCENE_2D, SE_EDITOR_CATEGORY_OBJECT_2D, SE_EDITOR_CATEGORY_SCENE_3D, SE_EDITOR_CATEGORY_OBJECT_3D, SE_EDITOR_CATEGORY_UI, SE_EDITOR_CATEGORY_UI_WIDGET, SE_EDITOR_CATEGORY_VFX_2D, SE_EDITOR_CATEGORY_VFX_3D, SE_EDITOR_CATEGORY_SIMULATION, SE_EDITOR_CATEGORY_MODEL, SE_EDITOR_CATEGORY_SHADER, SE_EDITOR_CATEGORY_TEXTURE, SE_EDITOR_CATEGORY_FRAMEBUFFER, SE_EDITOR_CATEGORY_RENDER_BUFFER, SE_EDITOR_CATEGORY_FONT, SE_EDITOR_CATEGORY_AUDIO, SE_EDITOR_CATEGORY_AUDIO_CLIP, SE_EDITOR_CATEGORY_AUDIO_STREAM, SE_EDITOR_CATEGORY_AUDIO_CAPTURE, SE_EDITOR_CATEGORY_NAVIGATION, SE_EDITOR_CATEGORY_PHYSICS_2D, SE_EDITOR_CATEGORY_PHYSICS_2D_BODY, SE_EDITOR_CATEGORY_PHYSICS_3D, SE_EDITOR_CATEGORY_PHYSICS_3D_BODY, SE_EDITOR_CATEGORY_COUNT } se_editor_category;
```

</div>

No inline description found in header comments.

### `se_editor_command_type`

<div class="api-signature">

```c
typedef enum { SE_EDITOR_COMMAND_SET = 0, SE_EDITOR_COMMAND_TOGGLE, SE_EDITOR_COMMAND_ACTION } se_editor_command_type;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_editor`

<div class="api-signature">

```c
typedef struct se_editor se_editor;
```

</div>

No inline description found in header comments.

### `se_editor_category_mask`

<div class="api-signature">

```c
typedef u64 se_editor_category_mask;
```

</div>

No inline description found in header comments.

### `se_editor_command`

<div class="api-signature">

```c
typedef struct { se_editor_command_type type; se_editor_item item; c8 name[SE_MAX_NAME_LENGTH]; se_editor_value value; se_editor_value aux_value; } se_editor_command;
```

</div>

No inline description found in header comments.

### `se_editor_config`

<div class="api-signature">

```c
typedef struct { se_context* context; se_window_handle window; se_input_handle* input; se_audio_engine* audio; se_navigation_grid* navigation; se_physics_world_2d* physics_2d; se_physics_world_3d* physics_3d; se_simulation_handle focused_simulation; se_vfx_2d_handle focused_vfx_2d; se_vfx_3d_handle focused_vfx_3d; } se_editor_config;
```

</div>

No inline description found in header comments.

### `se_editor_counts`

<div class="api-signature">

```c
typedef struct { u32 windows; u32 inputs; u32 cameras; u32 scenes_2d; u32 objects_2d; u32 scenes_3d; u32 objects_3d; u32 uis; u32 ui_widgets; u32 vfx_2d; u32 vfx_3d; u32 simulations; u32 models; u32 shaders; u32 textures; u32 framebuffers; u32 render_buffers; u32 fonts; u32 audio_clips; u32 audio_streams; u32 audio_captures; u32 physics_2d_bodies; u32 physics_3d_bodies; } se_editor_counts;
```

</div>

No inline description found in header comments.

### `se_editor_diagnostics`

<div class="api-signature">

```c
typedef struct { se_backend_info backend_info; se_capabilities capabilities; se_debug_frame_timing frame_timing; se_window_diagnostics window_diagnostics; se_input_diagnostics input_diagnostics; se_simulation_diagnostics simulation_diagnostics; se_vfx_diagnostics vfx_2d_diagnostics; se_vfx_diagnostics vfx_3d_diagnostics; se_audio_band_levels audio_capture_bands; b8 has_frame_timing : 1; b8 has_window_diagnostics : 1; b8 has_input_diagnostics : 1; b8 has_simulation_diagnostics : 1; b8 has_vfx_2d_diagnostics : 1; b8 has_vfx_3d_diagnostics : 1; b8 has_audio_capture_bands : 1; } se_editor_diagnostics;
```

</div>

No inline description found in header comments.

### `se_editor_item`

<div class="api-signature">

```c
typedef struct { se_editor_category category; s_handle handle; s_handle owner_handle; void* pointer; void* owner_pointer; u32 index; c8 label[SE_MAX_NAME_LENGTH]; } se_editor_item;
```

</div>

No inline description found in header comments.

### `se_editor_property`

<div class="api-signature">

```c
typedef struct { c8 name[SE_MAX_NAME_LENGTH]; se_editor_value value; b8 editable : 1; } se_editor_property;
```

</div>

No inline description found in header comments.

### `se_editor_value`

<div class="api-signature">

```c
typedef struct { se_editor_value_type type; union { b8 bool_value; i32 int_value; u32 uint_value; u64 u64_value; f32 float_value; f64 double_value; s_vec2 vec2_value; s_vec3 vec3_value; s_vec4 vec4_value; s_mat3 mat3_value; s_mat4 mat4_value; s_handle handle_value; void* pointer_value; c8 text_value[SE_MAX_PATH_LENGTH]; }; } se_editor_value;
```

</div>

No inline description found in header comments.
