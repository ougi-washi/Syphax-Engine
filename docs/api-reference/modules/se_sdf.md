<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_sdf.h

Source header: `include/se_sdf.h`

## Overview

Signed distance field scene graph, renderer, and material controls.

This page is generated from `include/se_sdf.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/sdf.md)

## Functions

### `se_sdf_align_camera`

<div class="api-signature">

```c
extern b8 se_sdf_align_camera( se_sdf_handle sdf, se_camera_handle camera, const se_sdf_camera_align_desc* desc, se_sdf_bounds* out_bounds );
```

</div>

No inline description found in header comments.

### `se_sdf_bake`

<div class="api-signature">

```c
extern b8 se_sdf_bake(se_sdf_handle sdf, const se_sdf_bake_desc* desc, se_sdf_bake_result* out_result);
```

</div>

No inline description found in header comments.

### `se_sdf_bake_model_texture3d`

<div class="api-signature">

```c
extern b8 se_sdf_bake_model_texture3d( se_model_handle model, const se_sdf_model_texture3d_desc* desc, se_sdf_model_texture3d_result* out_result );
```

</div>

No inline description found in header comments.

### `se_sdf_build_grid_preset`

<div class="api-signature">

```c
extern b8 se_sdf_build_grid_preset( se_sdf_handle sdf, const se_sdf_primitive_desc* primitive, i32 columns, i32 rows, f32 spacing, se_sdf_node_handle* out_root, se_sdf_node_handle* out_first );
```

</div>

No inline description found in header comments.

### `se_sdf_build_orbit_showcase_preset`

<div class="api-signature">

```c
extern b8 se_sdf_build_orbit_showcase_preset( se_sdf_handle sdf, const se_sdf_primitive_desc* center_primitive, const se_sdf_primitive_desc* orbit_primitive, i32 orbit_count, f32 orbit_radius, se_sdf_node_handle* out_root, se_sdf_node_handle* out_center );
```

</div>

No inline description found in header comments.

### `se_sdf_build_primitive_gallery_preset`

<div class="api-signature">

```c
extern b8 se_sdf_build_primitive_gallery_preset( se_sdf_handle sdf, i32 columns, i32 rows, f32 spacing, se_sdf_node_handle* out_root, se_sdf_node_handle* out_focus );
```

</div>

No inline description found in header comments.

### `se_sdf_build_single_object_preset`

<div class="api-signature">

```c
extern b8 se_sdf_build_single_object_preset( se_sdf_handle sdf, const se_sdf_primitive_desc* primitive, const s_mat4* transform, se_sdf_node_handle* out_root, se_sdf_node_handle* out_object );
```

</div>

No inline description found in header comments.

### `se_sdf_calculate_bounds`

<div class="api-signature">

```c
extern b8 se_sdf_calculate_bounds(se_sdf_handle sdf, se_sdf_bounds* out_bounds);
```

</div>

No inline description found in header comments.

### `se_sdf_clear`

<div class="api-signature">

```c
extern void se_sdf_clear(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_create`

<div class="api-signature">

```c
extern se_sdf_handle se_sdf_create(const se_sdf_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_destroy`

<div class="api-signature">

```c
extern void se_sdf_destroy(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_frame_set_camera`

<div class="api-signature">

```c
extern b8 se_sdf_frame_set_camera(se_sdf_frame_desc* frame, se_camera_handle camera);
```

</div>

No inline description found in header comments.

### `se_sdf_frame_set_scene_depth_texture`

<div class="api-signature">

```c
extern b8 se_sdf_frame_set_scene_depth_texture(se_sdf_frame_desc* frame, u32 depth_texture);
```

</div>

No inline description found in header comments.

### `se_sdf_from_json`

<div class="api-signature">

```c
extern b8 se_sdf_from_json(se_sdf_handle sdf, const s_json* root);
```

</div>

No inline description found in header comments.

### `se_sdf_from_json_file`

<div class="api-signature">

```c
extern b8 se_sdf_from_json_file(se_sdf_handle sdf, const c8* path);
```

</div>

No inline description found in header comments.

### `se_sdf_get_generation`

<div class="api-signature">

```c
extern u64 se_sdf_get_generation(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_node`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_get_node(se_sdf_handle sdf, u32 index);
```

</div>

No inline description found in header comments.

### `se_sdf_get_node_count`

<div class="api-signature">

```c
extern u32 se_sdf_get_node_count(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_get_root`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_get_root(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_node_add_child`

<div class="api-signature">

```c
extern b8 se_sdf_node_add_child(se_sdf_handle sdf, se_sdf_node_handle parent, se_sdf_node_handle child);
```

</div>

No inline description found in header comments.

### `se_sdf_node_create_group`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_create_group(se_sdf_handle sdf, const se_sdf_node_group_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_node_create_primitive`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_create_primitive(se_sdf_handle sdf, const se_sdf_node_primitive_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_node_create_ref`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_create_ref(se_sdf_handle sdf, const se_sdf_node_ref_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_node_destroy`

<div class="api-signature">

```c
extern void se_sdf_node_destroy(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_child`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_get_child(se_sdf_handle sdf, se_sdf_node_handle node, u32 index);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_child_count`

<div class="api-signature">

```c
extern u32 se_sdf_node_get_child_count(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_color`

<div class="api-signature">

```c
extern b8 se_sdf_node_get_color(se_sdf_handle sdf, se_sdf_node_handle node, s_vec3* out_color);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_noise`

<div class="api-signature">

```c
extern se_sdf_noise se_sdf_node_get_noise(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_operation`

<div class="api-signature">

```c
extern se_sdf_operation se_sdf_node_get_operation(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_operation_amount`

<div class="api-signature">

```c
extern f32 se_sdf_node_get_operation_amount(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_parent`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_get_parent(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_primitive`

<div class="api-signature">

```c
extern b8 se_sdf_node_get_primitive(se_sdf_handle sdf, se_sdf_node_handle node, se_sdf_primitive_desc* out_primitive);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_ref_source`

<div class="api-signature">

```c
extern se_sdf_handle se_sdf_node_get_ref_source(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_transform`

<div class="api-signature">

```c
extern s_mat4 se_sdf_node_get_transform(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_type`

<div class="api-signature">

```c
extern se_sdf_node_type se_sdf_node_get_type(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_has_color`

<div class="api-signature">

```c
extern b8 se_sdf_node_has_color(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_remove_child`

<div class="api-signature">

```c
extern b8 se_sdf_node_remove_child(se_sdf_handle sdf, se_sdf_node_handle parent, se_sdf_node_handle child);
```

</div>

No inline description found in header comments.

### `se_sdf_node_set_color`

<div class="api-signature">

```c
extern b8 se_sdf_node_set_color(se_sdf_handle sdf, se_sdf_node_handle node, const s_vec3* color);
```

</div>

No inline description found in header comments.

### `se_sdf_node_set_noise`

<div class="api-signature">

```c
extern b8 se_sdf_node_set_noise(se_sdf_handle sdf, se_sdf_node_handle node, const se_sdf_noise* noise);
```

</div>

No inline description found in header comments.

### `se_sdf_node_set_operation`

<div class="api-signature">

```c
extern b8 se_sdf_node_set_operation(se_sdf_handle sdf, se_sdf_node_handle node, se_sdf_operation operation);
```

</div>

No inline description found in header comments.

### `se_sdf_node_set_operation_amount`

<div class="api-signature">

```c
extern b8 se_sdf_node_set_operation_amount(se_sdf_handle sdf, se_sdf_node_handle node, f32 operation_amount);
```

</div>

No inline description found in header comments.

### `se_sdf_node_set_primitive`

<div class="api-signature">

```c
extern b8 se_sdf_node_set_primitive(se_sdf_handle sdf, se_sdf_node_handle node, const se_sdf_primitive_desc* primitive);
```

</div>

No inline description found in header comments.

### `se_sdf_node_set_transform`

<div class="api-signature">

```c
extern b8 se_sdf_node_set_transform(se_sdf_handle sdf, se_sdf_node_handle node, const s_mat4* transform);
```

</div>

No inline description found in header comments.

### `se_sdf_node_spawn_primitive`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_spawn_primitive( se_sdf_handle sdf, se_sdf_node_handle parent, const se_sdf_primitive_desc* primitive, const s_mat4* transform, se_sdf_operation operation );
```

</div>

No inline description found in header comments.

### `se_sdf_physics_add_shape_3d`

<div class="api-signature">

```c
extern se_physics_shape_3d_handle se_sdf_physics_add_shape_3d( se_sdf_physics_handle physics, se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3* offset, const s_vec3* rotation, b8 is_trigger );
```

</div>

No inline description found in header comments.

### `se_sdf_physics_create`

<div class="api-signature">

```c
extern se_sdf_physics_handle se_sdf_physics_create(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_physics_destroy`

<div class="api-signature">

```c
extern void se_sdf_physics_destroy(se_sdf_physics_handle physics);
```

</div>

No inline description found in header comments.

### `se_sdf_physics_refresh`

<div class="api-signature">

```c
extern b8 se_sdf_physics_refresh(se_sdf_physics_handle physics);
```

</div>

No inline description found in header comments.

### `se_sdf_prepare`

<div class="api-signature">

```c
extern b8 se_sdf_prepare(se_sdf_handle sdf, const se_sdf_prepare_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_prepare_async`

<div class="api-signature">

```c
extern b8 se_sdf_prepare_async(se_sdf_handle sdf, const se_sdf_prepare_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_prepare_poll`

<div class="api-signature">

```c
extern b8 se_sdf_prepare_poll(se_sdf_handle sdf, se_sdf_prepare_status* out_status);
```

</div>

No inline description found in header comments.

### `se_sdf_project_point`

<div class="api-signature">

```c
extern b8 se_sdf_project_point(se_sdf_handle sdf, const s_vec3* point, const s_vec3* direction, f32 max_distance, se_sdf_surface_hit* out_hit);
```

</div>

No inline description found in header comments.

### `se_sdf_raycast`

<div class="api-signature">

```c
extern b8 se_sdf_raycast(se_sdf_handle sdf, const s_vec3* origin, const s_vec3* direction, f32 max_distance, se_sdf_surface_hit* out_hit);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_create`

<div class="api-signature">

```c
extern se_sdf_renderer_handle se_sdf_renderer_create(const se_sdf_renderer_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_destroy`

<div class="api-signature">

```c
extern void se_sdf_renderer_destroy(se_sdf_renderer_handle renderer);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_get_build_diagnostics`

<div class="api-signature">

```c
extern se_sdf_build_diagnostics se_sdf_renderer_get_build_diagnostics(se_sdf_renderer_handle renderer);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_get_stats`

<div class="api-signature">

```c
extern se_sdf_renderer_stats se_sdf_renderer_get_stats(se_sdf_renderer_handle renderer);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_get_stylized`

<div class="api-signature">

```c
extern se_sdf_stylized_desc se_sdf_renderer_get_stylized(se_sdf_renderer_handle renderer);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_render`

<div class="api-signature">

```c
extern b8 se_sdf_renderer_render(se_sdf_renderer_handle renderer, const se_sdf_frame_desc* frame);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_set_base_color`

<div class="api-signature">

```c
extern b8 se_sdf_renderer_set_base_color(se_sdf_renderer_handle renderer, const s_vec3* base_color);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_set_debug`

<div class="api-signature">

```c
extern b8 se_sdf_renderer_set_debug(se_sdf_renderer_handle renderer, const se_sdf_renderer_debug* debug);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_set_light_rig`

<div class="api-signature">

```c
extern b8 se_sdf_renderer_set_light_rig( se_sdf_renderer_handle renderer, const s_vec3* light_direction, const s_vec3* light_color, const s_vec3* fog_color, f32 fog_density );
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_set_material`

<div class="api-signature">

```c
extern b8 se_sdf_renderer_set_material(se_sdf_renderer_handle renderer, const se_sdf_material_desc* material);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_set_quality`

<div class="api-signature">

```c
extern b8 se_sdf_renderer_set_quality(se_sdf_renderer_handle renderer, const se_sdf_raymarch_quality* quality);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_set_sdf`

<div class="api-signature">

```c
extern b8 se_sdf_renderer_set_sdf(se_sdf_renderer_handle renderer, se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_set_stylized`

<div class="api-signature">

```c
extern b8 se_sdf_renderer_set_stylized(se_sdf_renderer_handle renderer, const se_sdf_stylized_desc* stylized);
```

</div>

No inline description found in header comments.

### `se_sdf_sample_distance`

<div class="api-signature">

```c
extern b8 se_sdf_sample_distance(se_sdf_handle sdf, const s_vec3* point, f32* out_distance, se_sdf_node_handle* out_node);
```

</div>

No inline description found in header comments.

### `se_sdf_sample_height`

<div class="api-signature">

```c
extern b8 se_sdf_sample_height(se_sdf_handle sdf, f32 x, f32 z, f32 start_y, f32 max_distance, se_sdf_surface_hit* out_hit);
```

</div>

No inline description found in header comments.

### `se_sdf_set_root`

<div class="api-signature">

```c
extern b8 se_sdf_set_root(se_sdf_handle sdf, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_shutdown`

<div class="api-signature">

```c
extern void se_sdf_shutdown(void);
```

</div>

No inline description found in header comments.

### `se_sdf_to_json`

<div class="api-signature">

```c
extern s_json* se_sdf_to_json(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_to_object_2d`

<div class="api-signature">

```c
extern se_object_2d_handle se_sdf_to_object_2d(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_to_object_3d`

<div class="api-signature">

```c
extern se_object_3d_handle se_sdf_to_object_3d(se_sdf_handle sdf);
```

</div>

No inline description found in header comments.

### `se_sdf_transform`

<div class="api-signature">

```c
extern s_mat4 se_sdf_transform(s_vec3 translation, s_vec3 rotation, s_vec3 scale);
```

</div>

No inline description found in header comments.

### `se_sdf_validate`

<div class="api-signature">

```c
extern b8 se_sdf_validate(se_sdf_handle sdf, char* error_message, sz error_message_size);
```

</div>

No inline description found in header comments.

## Enums

### `se_sdf_node_type`

<div class="api-signature">

```c
typedef enum { SE_SDF_NODE_GROUP, SE_SDF_NODE_PRIMITIVE, SE_SDF_NODE_REF } se_sdf_node_type;
```

</div>

No inline description found in header comments.

### `se_sdf_operation`

<div class="api-signature">

```c
typedef enum { SE_SDF_OP_NONE, SE_SDF_OP_UNION, SE_SDF_OP_INTERSECTION, SE_SDF_OP_SUBTRACTION, SE_SDF_OP_SMOOTH_UNION, SE_SDF_OP_SMOOTH_INTERSECTION, SE_SDF_OP_SMOOTH_SUBTRACTION } se_sdf_operation;
```

</div>

No inline description found in header comments.

### `se_sdf_primitive_type`

<div class="api-signature">

```c
typedef enum { SE_SDF_PRIMITIVE_SPHERE = 1, SE_SDF_PRIMITIVE_BOX = 2, SE_SDF_PRIMITIVE_ROUND_BOX = 3, SE_SDF_PRIMITIVE_BOX_FRAME = 4, SE_SDF_PRIMITIVE_TORUS = 5, SE_SDF_PRIMITIVE_CAPPED_TORUS = 6, SE_SDF_PRIMITIVE_LINK = 7, SE_SDF_PRIMITIVE_CYLINDER = 8, SE_SDF_PRIMITIVE_CONE = 9, SE_SDF_PRIMITIVE_CONE_INFINITE = 10, SE_SDF_PRIMITIVE_PLANE = 11, SE_SDF_PRIMITIVE_HEX_PRISM = 12, SE_SDF_PRIMITIVE_CAPSULE = 13, SE_SDF_PRIMITIVE_VERTICAL_CAPSULE = 14, SE_SDF_PRIMITIVE_CAPPED_CYLINDER = 15, SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY = 16, SE_SDF_PRIMITIVE_ROUNDED_CYLINDER = 17, SE_SDF_PRIMITIVE_CAPPED_CONE = 18, SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY = 19, SE_SDF_PRIMITIVE_SOLID_ANGLE = 20, SE_SDF_PRIMITIVE_CUT_SPHERE = 21, SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE = 22, SE_SDF_PRIMITIVE_DEATH_STAR = 23, SE_SDF_PRIMITIVE_ROUND_CONE = 24, SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY = 25, SE_SDF_PRIMITIVE_VESICA_SEGMENT = 26, SE_SDF_PRIMITIVE_RHOMBUS = 27, SE_SDF_PRIMITIVE_OCTAHEDRON = 28, SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND = 29, SE_SDF_PRIMITIVE_PYRAMID = 30, SE_SDF_PRIMITIVE_TRIANGLE = 31, SE_SDF_PRIMITIVE_QUAD = 32 } se_sdf_primitive_type;
```

</div>

No inline description found in header comments.

### `se_sdf_shading_model`

<div class="api-signature">

```c
typedef enum { SE_SDF_SHADING_STYLIZED, SE_SDF_SHADING_LIT_PBR, SE_SDF_SHADING_UNLIT } se_sdf_shading_model;
```

</div>

No inline description found in header comments.

## Typedefs

### `s_json`

<div class="api-signature">

```c
typedef struct s_json s_json;
```

</div>

No inline description found in header comments.

### `se_sdf_bake_desc`

<div class="api-signature">

```c
typedef struct { se_sdf_prepare_desc prepare; } se_sdf_bake_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_bake_result`

<div class="api-signature">

```c
typedef struct { se_sdf_bounds bounds; u32 lod_count; se_sdf_lod_volume lods[4]; } se_sdf_bake_result;
```

</div>

No inline description found in header comments.

### `se_sdf_bounds`

<div class="api-signature">

```c
typedef struct { s_vec3 min; s_vec3 max; s_vec3 center; f32 radius; b8 valid; b8 has_unbounded_primitives; } se_sdf_bounds;
```

</div>

No inline description found in header comments.

### `se_sdf_box_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 size; } se_sdf_box_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_box_frame_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 size; f32 thickness; } se_sdf_box_frame_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_build_diagnostics`

<div class="api-signature">

```c
typedef struct { b8 success; char stage[64]; char message[512]; } se_sdf_build_diagnostics;
```

</div>

No inline description found in header comments.

### `se_sdf_camera_align_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 view_direction; s_vec3 up; f32 padding; f32 min_radius; f32 min_distance; f32 near_margin; f32 far_margin; b8 update_clip_planes; } se_sdf_camera_align_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_capped_cone_arbitrary_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } se_sdf_capped_cone_arbitrary_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_capped_cone_desc`

<div class="api-signature">

```c
typedef struct { f32 height; f32 radius_base; f32 radius_top; } se_sdf_capped_cone_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_capped_cylinder_arbitrary_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 a; s_vec3 b; f32 radius; } se_sdf_capped_cylinder_arbitrary_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_capped_cylinder_desc`

<div class="api-signature">

```c
typedef struct { f32 radius; f32 half_height; } se_sdf_capped_cylinder_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_capped_torus_desc`

<div class="api-signature">

```c
typedef struct { s_vec2 axis_sin_cos; f32 major_radius; f32 minor_radius; } se_sdf_capped_torus_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_capsule_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 a; s_vec3 b; f32 radius; } se_sdf_capsule_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_cone_desc`

<div class="api-signature">

```c
typedef struct { s_vec2 angle_sin_cos; f32 height; } se_sdf_cone_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_cone_infinite_desc`

<div class="api-signature">

```c
typedef struct { s_vec2 angle_sin_cos; } se_sdf_cone_infinite_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_cut_hollow_sphere_desc`

<div class="api-signature">

```c
typedef struct { f32 radius; f32 cut_height; f32 thickness; } se_sdf_cut_hollow_sphere_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_cut_sphere_desc`

<div class="api-signature">

```c
typedef struct { f32 radius; f32 cut_height; } se_sdf_cut_sphere_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_cylinder_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 axis_and_radius; } se_sdf_cylinder_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_death_star_desc`

<div class="api-signature">

```c
typedef struct { f32 radius_a; f32 radius_b; f32 separation; } se_sdf_death_star_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_desc`

<div class="api-signature">

```c
typedef struct { sz initial_node_capacity; } se_sdf_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_frame_desc`

<div class="api-signature">

```c
typedef struct { s_vec2 resolution; f32 time_seconds; s_vec2 mouse_normalized; se_camera_handle camera; b8 has_scene_depth_texture; u32 scene_depth_texture; } se_sdf_frame_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_handle;
```

</div>

No inline description found in header comments.

### `se_sdf_hex_prism_desc`

<div class="api-signature">

```c
typedef struct { s_vec2 half_size; } se_sdf_hex_prism_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_link_desc`

<div class="api-signature">

```c
typedef struct { f32 half_length; f32 outer_radius; f32 inner_radius; } se_sdf_link_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_lod_volume`

<div class="api-signature">

```c
typedef struct { u32 resolution; se_texture_handle texture; s_vec3 voxel_size; f32 max_distance; b8 valid; } se_sdf_lod_volume;
```

</div>

No inline description found in header comments.

### `se_sdf_material_desc`

<div class="api-signature">

```c
typedef struct { se_sdf_shading_model model; s_vec3 base_color; s_vec3 emissive_color; f32 roughness; f32 metalness; f32 opacity; } se_sdf_material_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_material_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_material_handle;
```

</div>

No inline description found in header comments.

### `se_sdf_model_texture3d_desc`

<div class="api-signature">

```c
typedef struct { u32 resolution_x; u32 resolution_y; u32 resolution_z; f32 padding; f32 max_distance; const c8* base_color_texture_uniform; const c8* base_color_factor_uniform; } se_sdf_model_texture3d_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_model_texture3d_result`

<div class="api-signature">

```c
typedef struct { se_texture_handle texture; s_vec3 bounds_min; s_vec3 bounds_max; s_vec3 voxel_size; f32 max_distance; } se_sdf_model_texture3d_result;
```

</div>

No inline description found in header comments.

### `se_sdf_node_group_desc`

<div class="api-signature">

```c
typedef struct { s_mat4 transform; se_sdf_operation operation; f32 operation_amount; } se_sdf_node_group_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_node_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_node_handle;
```

</div>

No inline description found in header comments.

### `se_sdf_node_primitive_desc`

<div class="api-signature">

```c
typedef struct { s_mat4 transform; se_sdf_operation operation; f32 operation_amount; se_sdf_primitive_desc primitive; se_sdf_noise noise; } se_sdf_node_primitive_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_node_ref_desc`

<div class="api-signature">

```c
typedef struct { se_sdf_handle source; s_mat4 transform; se_sdf_operation operation; f32 operation_amount; } se_sdf_node_ref_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_noise`

<div class="api-signature">

```c
typedef struct { b8 active; f32 scale; f32 offset; f32 frequency; } se_sdf_noise;
```

</div>

No inline description found in header comments.

### `se_sdf_octahedron_bound_desc`

<div class="api-signature">

```c
typedef struct { f32 size; } se_sdf_octahedron_bound_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_octahedron_desc`

<div class="api-signature">

```c
typedef struct { f32 size; } se_sdf_octahedron_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_physics_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_physics_handle;
```

</div>

No inline description found in header comments.

### `se_sdf_plane_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 normal; f32 offset; } se_sdf_plane_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_prepare_desc`

<div class="api-signature">

```c
typedef struct { u32 lod_count; u32 lod_resolutions[4]; u32 brick_size; u32 brick_border; f32 max_distance_scale; b8 force_rebuild; } se_sdf_prepare_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_prepare_status`

<div class="api-signature">

```c
typedef struct { b8 pending; b8 applied; b8 failed; b8 ready; se_result result; } se_sdf_prepare_status;
```

</div>

No inline description found in header comments.

### `se_sdf_primitive_desc`

<div class="api-signature">

```c
typedef struct { se_sdf_primitive_type type; union { se_sdf_sphere_desc sphere; se_sdf_box_desc box; se_sdf_round_box_desc round_box; se_sdf_box_frame_desc box_frame; se_sdf_torus_desc torus; se_sdf_capped_torus_desc capped_torus; se_sdf_link_desc link; se_sdf_cylinder_desc cylinder; se_sdf_cone_desc cone; se_sdf_cone_infinite_desc cone_infinite; se_sdf_plane_desc plane; se_sdf_hex_prism_desc hex_prism; se_sdf_capsule_desc capsule; se_sdf_vertical_capsule_desc vertical_capsule; se_sdf_capped_cylinder_desc capped_cylinder; se_sdf_capped_cylinder_arbitrary_desc capped_cylinder_arbitrary; se_sdf_rounded_cylinder_desc rounded_cylinder; se_sdf_capped_cone_desc capped_cone; se_sdf_capped_cone_arbitrary_desc capped_cone_arbitrary; se_sdf_solid_angle_desc solid_angle; se_sdf_cut_sphere_desc cut_sphere; se_sdf_cut_hollow_sphere_desc cut_hollow_sphere; se_sdf_death_star_desc death_star; se_sdf_round_cone_desc round_cone; se_sdf_round_cone_arbitrary_desc round_cone_arbitrary; se_sdf_vesica_segment_desc vesica_segment; se_sdf_rhombus_desc rhombus; se_sdf_octahedron_desc octahedron; se_sdf_octahedron_bound_desc octahedron_bound; se_sdf_pyramid_desc pyramid; se_sdf_triangle_desc triangle; se_sdf_quad_desc quad; }; } se_sdf_primitive_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_pyramid_desc`

<div class="api-signature">

```c
typedef struct { f32 height; } se_sdf_pyramid_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_quad_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 a; s_vec3 b; s_vec3 c; s_vec3 d; } se_sdf_quad_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_raymarch_quality`

<div class="api-signature">

```c
typedef struct { i32 max_steps; f32 hit_epsilon; f32 max_distance; b8 enable_shadows; i32 shadow_steps; f32 shadow_strength; } se_sdf_raymarch_quality;
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_debug`

<div class="api-signature">

```c
typedef struct { b8 show_normals; b8 show_distance; b8 show_steps; b8 freeze_time; } se_sdf_renderer_debug;
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_desc`

<div class="api-signature">

```c
typedef struct { u32 brick_budget; u32 brick_upload_budget; u32 max_visible_instances; f32 lod_fade_ratio; b8 use_compute_if_available; b8 front_to_back_sort; } se_sdf_renderer_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_renderer_handle;
```

</div>

No inline description found in header comments.

### `se_sdf_renderer_stats`

<div class="api-signature">

```c
typedef struct { u32 visible_refs; u32 resident_bricks; u32 page_misses; f32 average_march_steps; u32 selected_lod_counts[4]; } se_sdf_renderer_stats;
```

</div>

No inline description found in header comments.

### `se_sdf_rhombus_desc`

<div class="api-signature">

```c
typedef struct { f32 length_a; f32 length_b; f32 height; f32 corner_radius; } se_sdf_rhombus_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_round_box_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 size; f32 roundness; } se_sdf_round_box_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_round_cone_arbitrary_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } se_sdf_round_cone_arbitrary_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_round_cone_desc`

<div class="api-signature">

```c
typedef struct { f32 radius_base; f32 radius_top; f32 height; } se_sdf_round_cone_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_rounded_cylinder_desc`

<div class="api-signature">

```c
typedef struct { f32 outer_radius; f32 corner_radius; f32 half_height; } se_sdf_rounded_cylinder_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_solid_angle_desc`

<div class="api-signature">

```c
typedef struct { s_vec2 angle_sin_cos; f32 radius; } se_sdf_solid_angle_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_sphere_desc`

<div class="api-signature">

```c
typedef struct { f32 radius; } se_sdf_sphere_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_stylized_desc`

<div class="api-signature">

```c
typedef struct { f32 band_levels; f32 rim_power; f32 rim_strength; f32 fill_strength; f32 back_strength; f32 checker_scale; f32 checker_strength; f32 ground_blend; f32 desaturate; f32 gamma; } se_sdf_stylized_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_surface_hit`

<div class="api-signature">

```c
typedef struct { s_vec3 position; s_vec3 normal; f32 distance; se_sdf_handle sdf; se_sdf_node_handle node; b8 hit; } se_sdf_surface_hit;
```

</div>

No inline description found in header comments.

### `se_sdf_torus_desc`

<div class="api-signature">

```c
typedef struct { s_vec2 radii; } se_sdf_torus_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_triangle_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 a; s_vec3 b; s_vec3 c; } se_sdf_triangle_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_vertical_capsule_desc`

<div class="api-signature">

```c
typedef struct { f32 height; f32 radius; } se_sdf_vertical_capsule_desc;
```

</div>

No inline description found in header comments.

### `se_sdf_vesica_segment_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 a; s_vec3 b; f32 width; } se_sdf_vesica_segment_desc;
```

</div>

No inline description found in header comments.
