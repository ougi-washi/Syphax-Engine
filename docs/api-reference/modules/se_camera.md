<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_camera.h

Source header: `include/se_camera.h`

## Overview

Camera data, projection setup, and camera-controller helpers.

This page is generated from `include/se_camera.h` and is deterministic.

## Functions

### `se_camera_clamp_target`

<div class="api-signature">

```c
extern void se_camera_clamp_target(const se_camera_handle camera, const s_vec3 *min_bounds, const s_vec3 *max_bounds);
```

</div>

No inline description found in header comments.

### `se_camera_controller_create`

<div class="api-signature">

```c
extern se_camera_controller* se_camera_controller_create(const se_camera_controller_params* params);
```

</div>

No inline description found in header comments.

### `se_camera_controller_destroy`

<div class="api-signature">

```c
extern void se_camera_controller_destroy(se_camera_controller* controller);
```

</div>

No inline description found in header comments.

### `se_camera_controller_focus_bounds`

<div class="api-signature">

```c
extern void se_camera_controller_focus_bounds(se_camera_controller* controller);
```

</div>

No inline description found in header comments.

### `se_camera_controller_get_invert_y`

<div class="api-signature">

```c
extern b8 se_camera_controller_get_invert_y(const se_camera_controller* controller);
```

</div>

No inline description found in header comments.

### `se_camera_controller_get_look_toggle`

<div class="api-signature">

```c
extern b8 se_camera_controller_get_look_toggle(const se_camera_controller* controller);
```

</div>

No inline description found in header comments.

### `se_camera_controller_get_preset`

<div class="api-signature">

```c
extern se_camera_controller_preset se_camera_controller_get_preset(const se_camera_controller* controller);
```

</div>

No inline description found in header comments.

### `se_camera_controller_is_enabled`

<div class="api-signature">

```c
extern b8 se_camera_controller_is_enabled(const se_camera_controller* controller);
```

</div>

No inline description found in header comments.

### `se_camera_controller_set_enabled`

<div class="api-signature">

```c
extern void se_camera_controller_set_enabled(se_camera_controller* controller, const b8 enabled);
```

</div>

No inline description found in header comments.

### `se_camera_controller_set_focus_limits`

<div class="api-signature">

```c
extern void se_camera_controller_set_focus_limits(se_camera_controller* controller, const f32 min_distance, const f32 max_distance);
```

</div>

No inline description found in header comments.

### `se_camera_controller_set_invert_y`

<div class="api-signature">

```c
extern void se_camera_controller_set_invert_y(se_camera_controller* controller, const b8 invert_y);
```

</div>

No inline description found in header comments.

### `se_camera_controller_set_look_toggle`

<div class="api-signature">

```c
extern void se_camera_controller_set_look_toggle(se_camera_controller* controller, const b8 look_toggle);
```

</div>

No inline description found in header comments.

### `se_camera_controller_set_preset`

<div class="api-signature">

```c
extern b8 se_camera_controller_set_preset(se_camera_controller* controller, const se_camera_controller_preset preset);
```

</div>

No inline description found in header comments.

### `se_camera_controller_set_scene_bounds`

<div class="api-signature">

```c
extern void se_camera_controller_set_scene_bounds(se_camera_controller* controller, const s_vec3* center, const f32 radius);
```

</div>

No inline description found in header comments.

### `se_camera_controller_set_speeds`

<div class="api-signature">

```c
extern void se_camera_controller_set_speeds(se_camera_controller* controller, const f32 movement_speed, const f32 mouse_x_speed, const f32 mouse_y_speed);
```

</div>

No inline description found in header comments.

### `se_camera_controller_tick`

<div class="api-signature">

```c
extern void se_camera_controller_tick(se_camera_controller* controller, const f32 dt);
```

</div>

No inline description found in header comments.

### `se_camera_create`

<div class="api-signature">

```c
extern se_camera_handle se_camera_create(void);
```

</div>

No inline description found in header comments.

### `se_camera_destroy`

<div class="api-signature">

```c
extern void se_camera_destroy(const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_camera_dolly`

<div class="api-signature">

```c
extern void se_camera_dolly(const se_camera_handle camera, const s_vec3 *pivot, const f32 distance_delta, const f32 min_distance, const f32 max_distance);
```

</div>

No inline description found in header comments.

### `se_camera_get`

<div class="api-signature">

```c
extern se_camera *se_camera_get(const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_camera_get_forward_vector`

<div class="api-signature">

```c
extern s_vec3 se_camera_get_forward_vector(const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_camera_get_projection_matrix`

<div class="api-signature">

```c
extern s_mat4 se_camera_get_projection_matrix(const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_camera_get_right_vector`

<div class="api-signature">

```c
extern s_vec3 se_camera_get_right_vector(const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_camera_get_up_vector`

<div class="api-signature">

```c
extern s_vec3 se_camera_get_up_vector(const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_camera_get_view_matrix`

<div class="api-signature">

```c
extern s_mat4 se_camera_get_view_matrix(const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_camera_get_view_projection_matrix`

<div class="api-signature">

```c
extern s_mat4 se_camera_get_view_projection_matrix(const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_camera_orbit`

<div class="api-signature">

```c
extern void se_camera_orbit(const se_camera_handle camera, const s_vec3 *pivot, const f32 yaw_delta, const f32 pitch_delta, const f32 min_pitch, const f32 max_pitch);
```

</div>

No inline description found in header comments.

### `se_camera_pan_local`

<div class="api-signature">

```c
extern void se_camera_pan_local(const se_camera_handle camera, const f32 right_delta, const f32 up_delta, const f32 forward_delta);
```

</div>

No inline description found in header comments.

### `se_camera_pan_world`

<div class="api-signature">

```c
extern void se_camera_pan_world(const se_camera_handle camera, const s_vec3 *delta);
```

</div>

No inline description found in header comments.

### `se_camera_screen_to_plane`

<div class="api-signature">

```c
extern b8 se_camera_screen_to_plane(const se_camera_handle camera, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, const s_vec3 *plane_point, const s_vec3 *plane_normal, s_vec3 *out_world);
```

</div>

No inline description found in header comments.

### `se_camera_screen_to_ray`

<div class="api-signature">

```c
extern b8 se_camera_screen_to_ray(const se_camera_handle camera, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, s_vec3 *out_origin, s_vec3 *out_direction);
```

</div>

No inline description found in header comments.

### `se_camera_set_aspect`

<div class="api-signature">

```c
extern void se_camera_set_aspect(const se_camera_handle camera, const f32 width, const f32 height);
```

</div>

No inline description found in header comments.

### `se_camera_set_aspect_from_window`

<div class="api-signature">

```c
extern void se_camera_set_aspect_from_window(const se_camera_handle camera, const se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_camera_set_orbit_defaults`

<div class="api-signature">

```c
extern void se_camera_set_orbit_defaults(const se_camera_handle camera, const se_window_handle window, const s_vec3* pivot, const f32 distance);
```

</div>

No inline description found in header comments.

### `se_camera_set_orthographic`

<div class="api-signature">

```c
extern void se_camera_set_orthographic(const se_camera_handle camera, const f32 ortho_height, const f32 near_plane, const f32 far_plane);
```

</div>

No inline description found in header comments.

### `se_camera_set_perspective`

<div class="api-signature">

```c
extern void se_camera_set_perspective(const se_camera_handle camera, const f32 fov_degrees, const f32 near_plane, const f32 far_plane);
```

</div>

No inline description found in header comments.

### `se_camera_smooth_position`

<div class="api-signature">

```c
extern void se_camera_smooth_position(const se_camera_handle camera, const s_vec3 *position, const f32 smoothing, const f32 dt);
```

</div>

No inline description found in header comments.

### `se_camera_smooth_target`

<div class="api-signature">

```c
extern void se_camera_smooth_target(const se_camera_handle camera, const s_vec3 *target, const f32 smoothing, const f32 dt);
```

</div>

No inline description found in header comments.

### `se_camera_world_to_ndc`

<div class="api-signature">

```c
extern b8 se_camera_world_to_ndc(const se_camera_handle camera, const s_vec3 *world, s_vec3 *out_ndc);
```

</div>

No inline description found in header comments.

### `se_camera_world_to_screen`

<div class="api-signature">

```c
extern b8 se_camera_world_to_screen(const se_camera_handle camera, const s_vec3 *world, const f32 viewport_width, const f32 viewport_height, s_vec2 *out_screen);
```

</div>

No inline description found in header comments.

## Enums

### `se_camera_controller_preset`

<div class="api-signature">

```c
typedef enum { SE_CAMERA_CONTROLLER_PRESET_UE = 0, SE_CAMERA_CONTROLLER_PRESET_BLENDER, SE_CAMERA_CONTROLLER_PRESET_TRACKPAD } se_camera_controller_preset;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_camera`

<div class="api-signature">

```c
typedef struct se_camera { s_vec3 position; s_vec3 target; s_vec3 up; s_vec3 right; f32 fov; f32 near; f32 far; f32 aspect; f32 ortho_height; b8 use_orthographic : 1; } se_camera;
```

</div>

No inline description found in header comments.

### `se_camera_controller`

<div class="api-signature">

```c
typedef struct se_camera_controller se_camera_controller;
```

</div>

No inline description found in header comments.

### `se_camera_controller_params`

<div class="api-signature">

```c
typedef struct { se_window_handle window; se_camera_handle camera; f32 movement_speed; f32 mouse_x_speed; f32 mouse_y_speed; f32 min_pitch; f32 max_pitch; f32 min_focus_distance; f32 max_focus_distance; b8 enabled : 1; b8 invert_y : 1; b8 look_toggle : 1; b8 lock_cursor_while_active : 1; b8 use_raw_mouse_motion : 1; se_camera_controller_preset preset; } se_camera_controller_params;
```

</div>

No inline description found in header comments.

### `se_camera_ptr`

<div class="api-signature">

```c
typedef se_camera_handle se_camera_ptr;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_camera, se_cameras);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_camera_handle, se_cameras_ptr);
```

</div>

No inline description found in header comments.
