<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_camera.h

Source header: `include/se_camera.h`

## Overview

Camera data, projection setup, and camera-controller helpers.

This page is generated from `include/se_camera.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/camera.md)

## Functions

### `se_camera_add_location`

<div class="api-signature">

```c
extern void se_camera_add_location(const se_camera_handle camera, const s_vec3* delta_location);
```

</div>

No inline description found in header comments.

### `se_camera_add_rotation`

<div class="api-signature">

```c
extern void se_camera_add_rotation(const se_camera_handle camera, const s_vec3* delta_rotation);
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

### `se_camera_get_location`

<div class="api-signature">

```c
extern b8 se_camera_get_location(const se_camera_handle camera, s_vec3* out_location);
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

### `se_camera_get_rotation`

<div class="api-signature">

```c
extern b8 se_camera_get_rotation(const se_camera_handle camera, s_vec3* out_rotation);
```

</div>

No inline description found in header comments.

### `se_camera_get_target`

<div class="api-signature">

```c
extern b8 se_camera_get_target(const se_camera_handle camera, s_vec3* out_target);
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

### `se_camera_is_target_mode`

<div class="api-signature">

```c
extern b8 se_camera_is_target_mode(const se_camera_handle camera);
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

### `se_camera_set_location`

<div class="api-signature">

```c
extern void se_camera_set_location(const se_camera_handle camera, const s_vec3* location);
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

### `se_camera_set_rotation`

<div class="api-signature">

```c
extern void se_camera_set_rotation(const se_camera_handle camera, const s_vec3* rotation);
```

</div>

No inline description found in header comments.

### `se_camera_set_target`

<div class="api-signature">

```c
extern void se_camera_set_target(const se_camera_handle camera, const s_vec3* target);
```

</div>

No inline description found in header comments.

### `se_camera_set_target_mode`

<div class="api-signature">

```c
extern void se_camera_set_target_mode(const se_camera_handle camera, const b8 enabled);
```

</div>

No inline description found in header comments.

### `se_camera_set_window_aspect`

<div class="api-signature">

```c
extern void se_camera_set_window_aspect(const se_camera_handle camera, const se_window_handle window);
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

No enums found in this header.

## Typedefs

### `se_camera`

<div class="api-signature">

```c
typedef struct se_camera { s_vec3 position; s_vec3 target; s_vec3 up; s_vec3 right; // Rotation is (pitch, yaw, roll) in radians. s_vec3 rotation; f32 target_distance; f32 fov; f32 near; f32 far; f32 aspect; f32 ortho_height; s_mat4 cached_view_matrix; s_mat4 cached_projection_matrix; s_mat4 cached_view_projection_matrix; b8 use_orthographic : 1; b8 target_mode : 1; b8 view_dirty : 1; b8 projection_dirty : 1; b8 view_projection_dirty : 1; } se_camera;
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
