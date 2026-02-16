<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_scene.h

Source header: `include/se_scene.h`

## Overview

2D/3D scenes, objects, instance management, and picking.

This page is generated from `include/se_scene.h` and is deterministic.

## Functions

### `se_object_2d_add_instance`

<div class="api-signature">

```c
extern se_instance_id se_object_2d_add_instance(const se_object_2d_handle object, const s_mat3 *transform, const s_mat4 *buffer);
```

</div>

No inline description found in header comments.

### `se_object_2d_are_instances_dirty`

<div class="api-signature">

```c
extern b8 se_object_2d_are_instances_dirty(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_2d_create`

<div class="api-signature">

```c
extern se_object_2d_handle se_object_2d_create(const c8 *fragment_shader_path, const s_mat3 *transform, const sz max_instances_count);
```

</div>

2D objects functions

### `se_object_2d_create_custom`

<div class="api-signature">

```c
extern se_object_2d_handle se_object_2d_create_custom(se_object_custom *custom, const s_mat3 *transform);
```

</div>

No inline description found in header comments.

### `se_object_2d_destroy`

<div class="api-signature">

```c
extern void se_object_2d_destroy(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_box_2d`

<div class="api-signature">

```c
extern void se_object_2d_get_box_2d(const se_object_2d_handle object, se_box_2d *out_box);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_inactive_slot_count`

<div class="api-signature">

```c
extern sz se_object_2d_get_inactive_slot_count(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_instance_count`

<div class="api-signature">

```c
extern sz se_object_2d_get_instance_count(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_instance_index`

<div class="api-signature">

```c
extern i32 se_object_2d_get_instance_index(const se_object_2d_handle object, const se_instance_id instance_id);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_instance_metadata`

<div class="api-signature">

```c
extern b8 se_object_2d_get_instance_metadata(const se_object_2d_handle object, const se_instance_id instance_id, s_mat4* out_metadata);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_position`

<div class="api-signature">

```c
extern s_vec2 se_object_2d_get_position(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_scale`

<div class="api-signature">

```c
extern s_vec2 se_object_2d_get_scale(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_shader`

<div class="api-signature">

```c
extern se_shader_handle se_object_2d_get_shader(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_2d_get_transform`

<div class="api-signature">

```c
extern s_mat3 se_object_2d_get_transform(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_2d_is_instance_active`

<div class="api-signature">

```c
extern b8 se_object_2d_is_instance_active(const se_object_2d_handle object, const se_instance_id instance_id);
```

</div>

No inline description found in header comments.

### `se_object_2d_remove_instance`

<div class="api-signature">

```c
extern b8 se_object_2d_remove_instance(const se_object_2d_handle object, const se_instance_id instance_id);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instance_active`

<div class="api-signature">

```c
extern b8 se_object_2d_set_instance_active(const se_object_2d_handle object, const se_instance_id instance_id, const b8 active);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instance_buffer`

<div class="api-signature">

```c
extern void se_object_2d_set_instance_buffer(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat4 *buffer);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instance_metadata`

<div class="api-signature">

```c
extern b8 se_object_2d_set_instance_metadata(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat4* metadata);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instance_transform`

<div class="api-signature">

```c
extern void se_object_2d_set_instance_transform(const se_object_2d_handle object, const se_instance_id instance_id, const s_mat3 *transform);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instances_buffers`

<div class="api-signature">

```c
extern void se_object_2d_set_instances_buffers(const se_object_2d_handle object, const se_buffers *buffers);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instances_buffers_bulk`

<div class="api-signature">

```c
extern void se_object_2d_set_instances_buffers_bulk(const se_object_2d_handle object, const se_instance_id* instance_ids, const s_mat4* buffers, const sz count);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instances_dirty`

<div class="api-signature">

```c
extern void se_object_2d_set_instances_dirty(const se_object_2d_handle object, const b8 dirty);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instances_transforms`

<div class="api-signature">

```c
extern void se_object_2d_set_instances_transforms(const se_object_2d_handle object, const se_transforms_2d *transforms);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_instances_transforms_bulk`

<div class="api-signature">

```c
extern void se_object_2d_set_instances_transforms_bulk(const se_object_2d_handle object, const se_instance_id* instance_ids, const s_mat3* transforms, const sz count);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_position`

<div class="api-signature">

```c
extern void se_object_2d_set_position(const se_object_2d_handle object, const s_vec2 *position);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_scale`

<div class="api-signature">

```c
extern void se_object_2d_set_scale(const se_object_2d_handle object, const s_vec2 *scale);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_shader`

<div class="api-signature">

```c
extern void se_object_2d_set_shader(const se_object_2d_handle object, const se_shader_handle shader);
```

</div>

No inline description found in header comments.

### `se_object_2d_set_transform`

<div class="api-signature">

```c
extern void se_object_2d_set_transform(const se_object_2d_handle object, const s_mat3 *transform);
```

</div>

No inline description found in header comments.

### `se_object_2d_update_uniforms`

<div class="api-signature">

```c
extern void se_object_2d_update_uniforms(const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_3d_add_instance`

<div class="api-signature">

```c
extern se_instance_id se_object_3d_add_instance(const se_object_3d_handle object, const s_mat4 *transform, const s_mat4 *buffer);
```

</div>

No inline description found in header comments.

### `se_object_3d_are_instances_dirty`

<div class="api-signature">

```c
extern b8 se_object_3d_are_instances_dirty(const se_object_3d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_3d_create`

<div class="api-signature">

```c
extern se_object_3d_handle se_object_3d_create(const se_model_handle model, const s_mat4 *transform, const sz max_instances_count);
```

</div>

3D objects functions

### `se_object_3d_destroy`

<div class="api-signature">

```c
extern void se_object_3d_destroy(const se_object_3d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_3d_get_inactive_slot_count`

<div class="api-signature">

```c
extern sz se_object_3d_get_inactive_slot_count(const se_object_3d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_3d_get_instance_count`

<div class="api-signature">

```c
extern sz se_object_3d_get_instance_count(const se_object_3d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_3d_get_instance_index`

<div class="api-signature">

```c
extern i32 se_object_3d_get_instance_index(const se_object_3d_handle object, const se_instance_id instance_id);
```

</div>

No inline description found in header comments.

### `se_object_3d_get_instance_metadata`

<div class="api-signature">

```c
extern b8 se_object_3d_get_instance_metadata(const se_object_3d_handle object, const se_instance_id instance_id, s_mat4* out_metadata);
```

</div>

No inline description found in header comments.

### `se_object_3d_get_transform`

<div class="api-signature">

```c
extern s_mat4 se_object_3d_get_transform(const se_object_3d_handle object);
```

</div>

No inline description found in header comments.

### `se_object_3d_is_instance_active`

<div class="api-signature">

```c
extern b8 se_object_3d_is_instance_active(const se_object_3d_handle object, const se_instance_id instance_id);
```

</div>

No inline description found in header comments.

### `se_object_3d_remove_instance`

<div class="api-signature">

```c
extern b8 se_object_3d_remove_instance(const se_object_3d_handle object, const se_instance_id instance_id);
```

</div>

No inline description found in header comments.

### `se_object_3d_set_instance_active`

<div class="api-signature">

```c
extern b8 se_object_3d_set_instance_active(const se_object_3d_handle object, const se_instance_id instance_id, const b8 active);
```

</div>

No inline description found in header comments.

### `se_object_3d_set_instance_buffer`

<div class="api-signature">

```c
extern void se_object_3d_set_instance_buffer(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *buffer);
```

</div>

No inline description found in header comments.

### `se_object_3d_set_instance_metadata`

<div class="api-signature">

```c
extern b8 se_object_3d_set_instance_metadata(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4* metadata);
```

</div>

No inline description found in header comments.

### `se_object_3d_set_instance_transform`

<div class="api-signature">

```c
extern void se_object_3d_set_instance_transform(const se_object_3d_handle object, const se_instance_id instance_id, const s_mat4 *transform);
```

</div>

No inline description found in header comments.

### `se_object_3d_set_instances_buffers_bulk`

<div class="api-signature">

```c
extern void se_object_3d_set_instances_buffers_bulk(const se_object_3d_handle object, const se_instance_id* instance_ids, const s_mat4* buffers, const sz count);
```

</div>

No inline description found in header comments.

### `se_object_3d_set_instances_dirty`

<div class="api-signature">

```c
extern void se_object_3d_set_instances_dirty(const se_object_3d_handle object, const b8 dirty);
```

</div>

No inline description found in header comments.

### `se_object_3d_set_instances_transforms_bulk`

<div class="api-signature">

```c
extern void se_object_3d_set_instances_transforms_bulk(const se_object_3d_handle object, const se_instance_id* instance_ids, const s_mat4* transforms, const sz count);
```

</div>

No inline description found in header comments.

### `se_object_3d_set_transform`

<div class="api-signature">

```c
extern void se_object_3d_set_transform(const se_object_3d_handle object, const s_mat4 *transform);
```

</div>

No inline description found in header comments.

### `se_object_custom_set_data`

<div class="api-signature">

```c
extern void se_object_custom_set_data(se_object_custom *custom, const void *data, const sz size);
```

</div>

No inline description found in header comments.

### `se_scene_2d_add_object`

<div class="api-signature">

```c
extern void se_scene_2d_add_object(const se_scene_2d_handle scene, const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_scene_2d_bind`

<div class="api-signature">

```c
extern void se_scene_2d_bind(const se_scene_2d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_2d_create`

<div class="api-signature">

```c
extern se_scene_2d_handle se_scene_2d_create(const s_vec2 *size, const u16 object_count);
```

</div>

2D scene functions

### `se_scene_2d_destroy`

<div class="api-signature">

```c
extern void se_scene_2d_destroy(const se_scene_2d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_2d_destroy_full`

<div class="api-signature">

```c
extern void se_scene_2d_destroy_full(const se_scene_2d_handle scene, const b8 destroy_object_shaders);
```

</div>

Destroys every object referenced by the scene, optionally destroys their shaders, then destroys the scene itself.

### `se_scene_2d_draw`

<div class="api-signature">

```c
extern void se_scene_2d_draw(const se_scene_2d_handle scene, const se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_scene_2d_pick_object`

<div class="api-signature">

```c
extern b8 se_scene_2d_pick_object(const se_scene_2d_handle scene, const s_vec2* point_ndc, se_scene_pick_filter_2d filter, void* user_data, se_object_2d_handle* out_object);
```

</div>

No inline description found in header comments.

### `se_scene_2d_remove_object`

<div class="api-signature">

```c
extern void se_scene_2d_remove_object(const se_scene_2d_handle scene, const se_object_2d_handle object);
```

</div>

No inline description found in header comments.

### `se_scene_2d_render_raw`

<div class="api-signature">

```c
extern void se_scene_2d_render_raw(const se_scene_2d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_2d_render_to_buffer`

<div class="api-signature">

```c
extern void se_scene_2d_render_to_buffer(const se_scene_2d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_2d_render_to_screen`

<div class="api-signature">

```c
extern void se_scene_2d_render_to_screen(const se_scene_2d_handle scene, const se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_scene_2d_set_auto_resize`

<div class="api-signature">

```c
extern void se_scene_2d_set_auto_resize(const se_scene_2d_handle scene, const se_window_handle window, const s_vec2 *ratio);
```

</div>

No inline description found in header comments.

### `se_scene_2d_unbind`

<div class="api-signature">

```c
extern void se_scene_2d_unbind(const se_scene_2d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_3d_add_model`

<div class="api-signature">

```c
extern se_object_3d_handle se_scene_3d_add_model(const se_scene_3d_handle scene, const se_model_handle model, const s_mat4 *transform);
```

</div>

No inline description found in header comments.

### `se_scene_3d_add_object`

<div class="api-signature">

```c
extern void se_scene_3d_add_object(const se_scene_3d_handle scene, const se_object_3d_handle object);
```

</div>

No inline description found in header comments.

### `se_scene_3d_add_post_process_buffer`

<div class="api-signature">

```c
extern void se_scene_3d_add_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer);
```

</div>

No inline description found in header comments.

### `se_scene_3d_clear_debug_markers`

<div class="api-signature">

```c
extern void se_scene_3d_clear_debug_markers(const se_scene_3d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_3d_create`

<div class="api-signature">

```c
extern se_scene_3d_handle se_scene_3d_create(const s_vec2 *size, const u16 object_count);
```

</div>

3D scene functions

### `se_scene_3d_create_for_window`

<div class="api-signature">

```c
extern se_scene_3d_handle se_scene_3d_create_for_window(const se_window_handle window, const u16 object_count);
```

</div>

No inline description found in header comments.

### `se_scene_3d_debug_box`

<div class="api-signature">

```c
extern void se_scene_3d_debug_box(const se_scene_3d_handle scene, const s_vec3* min_corner, const s_vec3* max_corner, const s_vec4* color);
```

</div>

No inline description found in header comments.

### `se_scene_3d_debug_line`

<div class="api-signature">

```c
extern void se_scene_3d_debug_line(const se_scene_3d_handle scene, const s_vec3* start, const s_vec3* end, const s_vec4* color);
```

</div>

No inline description found in header comments.

### `se_scene_3d_debug_sphere`

<div class="api-signature">

```c
extern void se_scene_3d_debug_sphere(const se_scene_3d_handle scene, const s_vec3* center, const f32 radius, const s_vec4* color);
```

</div>

No inline description found in header comments.

### `se_scene_3d_debug_text`

<div class="api-signature">

```c
extern void se_scene_3d_debug_text(const se_scene_3d_handle scene, const s_vec3* position, const c8* text, const s_vec4* color);
```

</div>

No inline description found in header comments.

### `se_scene_3d_destroy`

<div class="api-signature">

```c
extern void se_scene_3d_destroy(const se_scene_3d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_3d_destroy_full`

<div class="api-signature">

```c
extern void se_scene_3d_destroy_full(const se_scene_3d_handle scene, const b8 destroy_models, const b8 destroy_model_shaders);
```

</div>

Destroys every object referenced by the scene. Optional flags can also destroy referenced models and their shaders before destroying the scene itself.

### `se_scene_3d_draw`

<div class="api-signature">

```c
extern void se_scene_3d_draw(const se_scene_3d_handle scene, const se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_scene_3d_get_camera`

<div class="api-signature">

```c
extern se_camera_handle se_scene_3d_get_camera(const se_scene_3d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_3d_get_debug_markers`

<div class="api-signature">

```c
extern b8 se_scene_3d_get_debug_markers(const se_scene_3d_handle scene, const se_scene_debug_marker** out_markers, sz* out_count);
```

</div>

No inline description found in header comments.

### `se_scene_3d_pick_object_screen`

<div class="api-signature">

```c
extern b8 se_scene_3d_pick_object_screen(const se_scene_3d_handle scene, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, const f32 pick_radius, se_scene_pick_filter_3d filter, void* user_data, se_object_3d_handle* out_object, f32* out_distance);
```

</div>

No inline description found in header comments.

### `se_scene_3d_register_custom_render`

<div class="api-signature">

```c
extern se_scene_3d_custom_render_handle se_scene_3d_register_custom_render(const se_scene_3d_handle scene, se_scene_3d_custom_render_callback callback, void* user_data);
```

</div>

No inline description found in header comments.

### `se_scene_3d_remove_object`

<div class="api-signature">

```c
extern void se_scene_3d_remove_object(const se_scene_3d_handle scene, const se_object_3d_handle object);
```

</div>

No inline description found in header comments.

### `se_scene_3d_remove_post_process_buffer`

<div class="api-signature">

```c
extern void se_scene_3d_remove_post_process_buffer(const se_scene_3d_handle scene, const se_render_buffer_handle buffer);
```

</div>

No inline description found in header comments.

### `se_scene_3d_render_to_buffer`

<div class="api-signature">

```c
extern void se_scene_3d_render_to_buffer(const se_scene_3d_handle scene);
```

</div>

No inline description found in header comments.

### `se_scene_3d_render_to_screen`

<div class="api-signature">

```c
extern void se_scene_3d_render_to_screen(const se_scene_3d_handle scene, const se_window_handle window);
```

</div>

No inline description found in header comments.

### `se_scene_3d_set_auto_resize`

<div class="api-signature">

```c
extern void se_scene_3d_set_auto_resize(const se_scene_3d_handle scene, const se_window_handle window, const s_vec2 *ratio);
```

</div>

No inline description found in header comments.

### `se_scene_3d_set_camera`

<div class="api-signature">

```c
extern void se_scene_3d_set_camera(const se_scene_3d_handle scene, const se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_scene_3d_set_culling`

<div class="api-signature">

```c
extern void se_scene_3d_set_culling(const se_scene_3d_handle scene, const b8 enabled);
```

</div>

No inline description found in header comments.

### `se_scene_3d_unregister_custom_render`

<div class="api-signature">

```c
extern b8 se_scene_3d_unregister_custom_render(const se_scene_3d_handle scene, const se_scene_3d_custom_render_handle callback_handle);
```

</div>

No inline description found in header comments.

## Enums

### `se_scene_debug_marker_type`

<div class="api-signature">

```c
typedef enum { SE_SCENE_DEBUG_MARKER_LINE = 0, SE_SCENE_DEBUG_MARKER_BOX, SE_SCENE_DEBUG_MARKER_SPHERE, SE_SCENE_DEBUG_MARKER_TEXT } se_scene_debug_marker_type;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_instance_id`

<div class="api-signature">

```c
typedef i32 se_instance_id;
```

</div>

No inline description found in header comments.

### `se_instances`

<div class="api-signature">

```c
typedef struct { se_instance_ids ids; se_transforms transforms; se_buffers buffers; se_instance_actives actives; se_instance_ids free_indices; se_buffers metadata; se_instance_id next_id; } se_instances;
```

</div>

No inline description found in header comments.

### `se_instances_2d`

<div class="api-signature">

```c
typedef struct { se_instance_ids ids; se_transforms_2d transforms; se_buffers buffers; se_instance_actives actives; se_instance_ids free_indices; se_buffers metadata; se_instance_id next_id; } se_instances_2d;
```

</div>

No inline description found in header comments.

### `se_object_2d`

<div class="api-signature">

```c
typedef struct se_object_2d { s_mat3 transform; union { struct { se_quad quad; se_shader_handle shader; se_instances_2d instances; se_transforms_2d render_transforms; se_buffers render_buffers; }; se_object_custom custom; }; b8 is_custom : 1; b8 is_visible : 1; } se_object_2d;
```

</div>

No inline description found in header comments.

### `se_object_2d_ptr`

<div class="api-signature">

```c
typedef se_object_2d_handle se_object_2d_ptr;
```

</div>

No inline description found in header comments.

### `se_object_3d`

<div class="api-signature">

```c
typedef struct se_object_3d { se_model_handle model; s_mat4 transform; se_instances instances; se_mesh_instances mesh_instances; se_transforms render_transforms; b8 is_visible : 1; } se_object_3d;
```

</div>

No inline description found in header comments.

### `se_object_3d_ptr`

<div class="api-signature">

```c
typedef se_object_3d_handle se_object_3d_ptr;
```

</div>

No inline description found in header comments.

### `se_object_custom`

<div class="api-signature">

```c
typedef struct { se_object_custom_callback render; sz data_size; u8 data[SE_OBJECT_CUSTOM_DATA_SIZE]; } se_object_custom;
```

</div>

No inline description found in header comments.

### `se_object_custom_callback`

<div class="api-signature">

```c
typedef void (*se_object_custom_callback)(void *data);
```

</div>

No inline description found in header comments.

### `se_scene_2d`

<div class="api-signature">

```c
typedef struct se_scene_2d { se_objects_2d_ptr objects; se_framebuffer_handle output; } se_scene_2d;
```

</div>

No inline description found in header comments.

### `se_scene_2d_ptr`

<div class="api-signature">

```c
typedef se_scene_2d_handle se_scene_2d_ptr;
```

</div>

No inline description found in header comments.

### `se_scene_3d`

<div class="api-signature">

```c
typedef struct se_scene_3d { se_objects_3d_ptr objects; se_camera_handle camera; se_render_buffers_ptr post_process; se_shader_handle output_shader; se_framebuffer_handle output; se_scene_debug_markers debug_markers; se_scene_3d_custom_render_entries custom_renders; s_mat4 last_vp; b8 enable_culling : 1; b8 has_last_vp : 1; } se_scene_3d;
```

</div>

No inline description found in header comments.

### `se_scene_3d_custom_render_callback`

<div class="api-signature">

```c
typedef void (*se_scene_3d_custom_render_callback)(se_scene_3d_handle scene, void* user_data);
```

</div>

No inline description found in header comments.

### `se_scene_3d_custom_render_entry`

<div class="api-signature">

```c
typedef struct { se_scene_3d_custom_render_callback callback; void* user_data; } se_scene_3d_custom_render_entry;
```

</div>

No inline description found in header comments.

### `se_scene_3d_custom_render_handle`

<div class="api-signature">

```c
typedef s_handle se_scene_3d_custom_render_handle;
```

</div>

No inline description found in header comments.

### `se_scene_3d_ptr`

<div class="api-signature">

```c
typedef se_scene_3d_handle se_scene_3d_ptr;
```

</div>

No inline description found in header comments.

### `se_scene_debug_marker`

<div class="api-signature">

```c
typedef struct { se_scene_debug_marker_type type; s_vec3 a; s_vec3 b; s_vec4 color; f32 radius; c8 text[64]; } se_scene_debug_marker;
```

</div>

No inline description found in header comments.

### `se_scene_pick_filter_2d`

<div class="api-signature">

```c
typedef b8 (*se_scene_pick_filter_2d)(se_object_2d_handle object, void* user_data);
```

</div>

No inline description found in header comments.

### `se_scene_pick_filter_3d`

<div class="api-signature">

```c
typedef b8 (*se_scene_pick_filter_3d)(se_object_3d_handle object, void* user_data);
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(b8, se_instance_actives);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(s_mat3, se_transforms_2d);
```

</div>

No inline description found in header comments.

### `typedef_3`

<div class="api-signature">

```c
typedef s_array(s_mat4, se_buffers);
```

</div>

No inline description found in header comments.

### `typedef_4`

<div class="api-signature">

```c
typedef s_array(s_mat4, se_transforms);
```

</div>

No inline description found in header comments.

### `typedef_5`

<div class="api-signature">

```c
typedef s_array(se_instance_id, se_instance_ids);
```

</div>

No inline description found in header comments.

### `typedef_6`

<div class="api-signature">

```c
typedef s_array(se_object_2d, se_objects_2d);
```

</div>

No inline description found in header comments.

### `typedef_7`

<div class="api-signature">

```c
typedef s_array(se_object_2d_handle, se_objects_2d_ptr);
```

</div>

No inline description found in header comments.

### `typedef_8`

<div class="api-signature">

```c
typedef s_array(se_object_3d, se_objects_3d);
```

</div>

No inline description found in header comments.

### `typedef_9`

<div class="api-signature">

```c
typedef s_array(se_object_3d_handle, se_objects_3d_ptr);
```

</div>

No inline description found in header comments.

### `typedef_10`

<div class="api-signature">

```c
typedef s_array(se_scene_2d, se_scenes_2d);
```

</div>

No inline description found in header comments.

### `typedef_11`

<div class="api-signature">

```c
typedef s_array(se_scene_2d_handle, se_scenes_2d_ptr);
```

</div>

No inline description found in header comments.

### `typedef_12`

<div class="api-signature">

```c
typedef s_array(se_scene_3d, se_scenes_3d);
```

</div>

No inline description found in header comments.

### `typedef_13`

<div class="api-signature">

```c
typedef s_array(se_scene_3d_custom_render_entry, se_scene_3d_custom_render_entries);
```

</div>

No inline description found in header comments.

### `typedef_14`

<div class="api-signature">

```c
typedef s_array(se_scene_3d_handle, se_scenes_3d_ptr);
```

</div>

No inline description found in header comments.

### `typedef_15`

<div class="api-signature">

```c
typedef s_array(se_scene_debug_marker, se_scene_debug_markers);
```

</div>

No inline description found in header comments.
