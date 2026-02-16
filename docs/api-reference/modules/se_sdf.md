<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_sdf.h

Source header: `include/se_sdf.h`

## Overview

Signed distance field scene graph, renderer, and material controls.

This page is generated from `include/se_sdf.h` and is deterministic.

## Read the Playbook

- [Deep dive Playbook](../../playbooks/se-sdf.md)

## Functions

### `se_sdf_node_add_child`

<div class="api-signature">

```c
extern b8 se_sdf_node_add_child(se_sdf_scene_handle scene, se_sdf_node_handle parent, se_sdf_node_handle child);
```

</div>

No inline description found in header comments.

### `se_sdf_node_create_group`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_create_group(se_sdf_scene_handle scene, const se_sdf_node_group_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_node_create_primitive`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_create_primitive(se_sdf_scene_handle scene, const se_sdf_node_primitive_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_node_destroy`

<div class="api-signature">

```c
extern void se_sdf_node_destroy(se_sdf_scene_handle scene, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_get_transform`

<div class="api-signature">

```c
extern s_mat4 se_sdf_node_get_transform(se_sdf_scene_handle scene, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_node_remove_child`

<div class="api-signature">

```c
extern b8 se_sdf_node_remove_child(se_sdf_scene_handle scene, se_sdf_node_handle parent, se_sdf_node_handle child);
```

</div>

No inline description found in header comments.

### `se_sdf_node_set_operation`

<div class="api-signature">

```c
extern b8 se_sdf_node_set_operation(se_sdf_scene_handle scene, se_sdf_node_handle node, se_sdf_operation operation);
```

</div>

No inline description found in header comments.

### `se_sdf_node_set_transform`

<div class="api-signature">

```c
extern b8 se_sdf_node_set_transform(se_sdf_scene_handle scene, se_sdf_node_handle node, const s_mat4* transform);
```

</div>

No inline description found in header comments.

### `se_sdf_node_spawn_primitive`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_node_spawn_primitive( se_sdf_scene_handle scene, se_sdf_node_handle parent, const se_sdf_primitive_desc* primitive, const s_mat4* transform, se_sdf_operation operation );
```

</div>

No inline description found in header comments.

### `se_sdf_scene_build_grid_preset`

<div class="api-signature">

```c
extern b8 se_sdf_scene_build_grid_preset( se_sdf_scene_handle scene, const se_sdf_primitive_desc* primitive, i32 columns, i32 rows, f32 spacing, se_sdf_node_handle* out_root, se_sdf_node_handle* out_first );
```

</div>

No inline description found in header comments.

### `se_sdf_scene_build_orbit_showcase_preset`

<div class="api-signature">

```c
extern b8 se_sdf_scene_build_orbit_showcase_preset( se_sdf_scene_handle scene, const se_sdf_primitive_desc* center_primitive, const se_sdf_primitive_desc* orbit_primitive, i32 orbit_count, f32 orbit_radius, se_sdf_node_handle* out_root, se_sdf_node_handle* out_center );
```

</div>

No inline description found in header comments.

### `se_sdf_scene_build_primitive_gallery_preset`

<div class="api-signature">

```c
extern b8 se_sdf_scene_build_primitive_gallery_preset( se_sdf_scene_handle scene, i32 columns, i32 rows, f32 spacing, se_sdf_node_handle* out_root, se_sdf_node_handle* out_focus );
```

</div>

No inline description found in header comments.

### `se_sdf_scene_build_single_object_preset`

<div class="api-signature">

```c
extern b8 se_sdf_scene_build_single_object_preset( se_sdf_scene_handle scene, const se_sdf_primitive_desc* primitive, const s_mat4* transform, se_sdf_node_handle* out_root, se_sdf_node_handle* out_object );
```

</div>

No inline description found in header comments.

### `se_sdf_scene_clear`

<div class="api-signature">

```c
extern void se_sdf_scene_clear(se_sdf_scene_handle scene);
```

</div>

No inline description found in header comments.

### `se_sdf_scene_create`

<div class="api-signature">

```c
extern se_sdf_scene_handle se_sdf_scene_create(const se_sdf_scene_desc* desc);
```

</div>

No inline description found in header comments.

### `se_sdf_scene_destroy`

<div class="api-signature">

```c
extern void se_sdf_scene_destroy(se_sdf_scene_handle scene);
```

</div>

No inline description found in header comments.

### `se_sdf_scene_get_root`

<div class="api-signature">

```c
extern se_sdf_node_handle se_sdf_scene_get_root(se_sdf_scene_handle scene);
```

</div>

No inline description found in header comments.

### `se_sdf_scene_set_root`

<div class="api-signature">

```c
extern b8 se_sdf_scene_set_root(se_sdf_scene_handle scene, se_sdf_node_handle node);
```

</div>

No inline description found in header comments.

### `se_sdf_scene_validate`

<div class="api-signature">

```c
extern b8 se_sdf_scene_validate(se_sdf_scene_handle scene, char* error_message, sz error_message_size);
```

</div>

No inline description found in header comments.

### `se_sdf_transform_grid_cell`

<div class="api-signature">

```c
extern s_mat4 se_sdf_transform_grid_cell(i32 index, i32 columns, i32 rows, f32 spacing, f32 y, f32 yaw, f32 sx, f32 sy, f32 sz);
```

</div>

No inline description found in header comments.

### `se_sdf_transform_trs`

<div class="api-signature">

```c
extern s_mat4 se_sdf_transform_trs(f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz);
```

</div>

No inline description found in header comments.

## Enums

### `se_sdf_object_type`

<div class="api-signature">

```c
typedef enum { SE_SDF_NONE, // Container/group node with no shape SE_SDF_SPHERE, SE_SDF_BOX, SE_SDF_ROUND_BOX, SE_SDF_BOX_FRAME, SE_SDF_TORUS, SE_SDF_CAPPED_TORUS, SE_SDF_LINK, SE_SDF_CYLINDER, SE_SDF_CONE, SE_SDF_CONE_INFINITE, SE_SDF_PLANE, SE_SDF_HEX_PRISM, SE_SDF_CAPSULE, SE_SDF_VERTICAL_CAPSULE, SE_SDF_CAPPED_CYLINDER, SE_SDF_CAPPED_CYLINDER_ARBITRARY, SE_SDF_ROUNDED_CYLINDER, SE_SDF_CAPPED_CONE, SE_SDF_CAPPED_CONE_ARBITRARY, SE_SDF_SOLID_ANGLE, SE_SDF_CUT_SPHERE, SE_SDF_CUT_HOLLOW_SPHERE, SE_SDF_DEATH_STAR, SE_SDF_ROUND_CONE, SE_SDF_ROUND_CONE_ARBITRARY, SE_SDF_VESICA_SEGMENT, SE_SDF_RHOMBUS, SE_SDF_OCTAHEDRON, SE_SDF_OCTAHEDRON_BOUND, SE_SDF_PYRAMID, SE_SDF_TRIANGLE, SE_SDF_QUAD } se_sdf_object_type;
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
typedef enum { SE_SDF_PRIMITIVE_SPHERE = SE_SDF_SPHERE, SE_SDF_PRIMITIVE_BOX = SE_SDF_BOX, SE_SDF_PRIMITIVE_ROUND_BOX = SE_SDF_ROUND_BOX, SE_SDF_PRIMITIVE_BOX_FRAME = SE_SDF_BOX_FRAME, SE_SDF_PRIMITIVE_TORUS = SE_SDF_TORUS, SE_SDF_PRIMITIVE_CAPPED_TORUS = SE_SDF_CAPPED_TORUS, SE_SDF_PRIMITIVE_LINK = SE_SDF_LINK, SE_SDF_PRIMITIVE_CYLINDER = SE_SDF_CYLINDER, SE_SDF_PRIMITIVE_CONE = SE_SDF_CONE, SE_SDF_PRIMITIVE_CONE_INFINITE = SE_SDF_CONE_INFINITE, SE_SDF_PRIMITIVE_PLANE = SE_SDF_PLANE, SE_SDF_PRIMITIVE_HEX_PRISM = SE_SDF_HEX_PRISM, SE_SDF_PRIMITIVE_CAPSULE = SE_SDF_CAPSULE, SE_SDF_PRIMITIVE_VERTICAL_CAPSULE = SE_SDF_VERTICAL_CAPSULE, SE_SDF_PRIMITIVE_CAPPED_CYLINDER = SE_SDF_CAPPED_CYLINDER, SE_SDF_PRIMITIVE_CAPPED_CYLINDER_ARBITRARY = SE_SDF_CAPPED_CYLINDER_ARBITRARY, SE_SDF_PRIMITIVE_ROUNDED_CYLINDER = SE_SDF_ROUNDED_CYLINDER, SE_SDF_PRIMITIVE_CAPPED_CONE = SE_SDF_CAPPED_CONE, SE_SDF_PRIMITIVE_CAPPED_CONE_ARBITRARY = SE_SDF_CAPPED_CONE_ARBITRARY, SE_SDF_PRIMITIVE_SOLID_ANGLE = SE_SDF_SOLID_ANGLE, SE_SDF_PRIMITIVE_CUT_SPHERE = SE_SDF_CUT_SPHERE, SE_SDF_PRIMITIVE_CUT_HOLLOW_SPHERE = SE_SDF_CUT_HOLLOW_SPHERE, SE_SDF_PRIMITIVE_DEATH_STAR = SE_SDF_DEATH_STAR, SE_SDF_PRIMITIVE_ROUND_CONE = SE_SDF_ROUND_CONE, SE_SDF_PRIMITIVE_ROUND_CONE_ARBITRARY = SE_SDF_ROUND_CONE_ARBITRARY, SE_SDF_PRIMITIVE_VESICA_SEGMENT = SE_SDF_VESICA_SEGMENT, SE_SDF_PRIMITIVE_RHOMBUS = SE_SDF_RHOMBUS, SE_SDF_PRIMITIVE_OCTAHEDRON = SE_SDF_OCTAHEDRON, SE_SDF_PRIMITIVE_OCTAHEDRON_BOUND = SE_SDF_OCTAHEDRON_BOUND, SE_SDF_PRIMITIVE_PYRAMID = SE_SDF_PYRAMID, SE_SDF_PRIMITIVE_TRIANGLE = SE_SDF_TRIANGLE, SE_SDF_PRIMITIVE_QUAD = SE_SDF_QUAD } se_sdf_primitive_type;
```

</div>

No inline description found in header comments.

## Typedefs

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

### `se_sdf_control_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_control_handle;
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

### `se_sdf_material_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_material_handle;
```

</div>

No inline description found in header comments.

### `se_sdf_node_group_desc`

<div class="api-signature">

```c
typedef struct { s_mat4 transform; se_sdf_operation operation; } se_sdf_node_group_desc;
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
typedef struct { s_mat4 transform; se_sdf_operation operation; se_sdf_primitive_desc primitive; } se_sdf_node_primitive_desc;
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

### `se_sdf_object`

<div class="api-signature">

```c
typedef struct se_sdf_object { s_mat4 transform; se_sdf_scene_handle source_scene; se_sdf_node_handle source_node; se_sdf_object_type type; union { struct { f32 radius; } sphere; struct { s_vec3 size; } box; struct { s_vec3 size; f32 roundness; } round_box; struct { s_vec3 size; f32 thickness; } box_frame; struct { s_vec2 radii; } torus; struct { s_vec2 axis_sin_cos; f32 major_radius; f32 minor_radius; } capped_torus; struct { f32 half_length; f32 outer_radius; f32 inner_radius; } link; struct { s_vec3 axis_and_radius; } cylinder; struct { s_vec2 angle_sin_cos; f32 height; } cone; struct { s_vec2 angle_sin_cos; } cone_infinite; struct { s_vec3 normal; f32 offset; } plane; struct { s_vec2 half_size; } hex_prism; struct { s_vec3 a; s_vec3 b; f32 radius; } capsule; struct { f32 height; f32 radius; } vertical_capsule; struct { f32 radius; f32 half_height; } capped_cylinder; struct { s_vec3 a; s_vec3 b; f32 radius; } capped_cylinder_arbitrary; struct { f32 outer_radius; f32 corner_radius; f32 half_height; } rounded_cylinder; struct { f32 height; f32 radius_base; f32 radius_top; } capped_cone; struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } capped_cone_arbitrary; struct { s_vec2 angle_sin_cos; f32 radius; } solid_angle; struct { f32 radius; f32 cut_height; } cut_sphere; struct { f32 radius; f32 cut_height; f32 thickness; } cut_hollow_sphere; struct { f32 radius_a; f32 radius_b; f32 separation; } death_star; struct { f32 radius_base; f32 radius_top; f32 height; } round_cone; struct { s_vec3 a; s_vec3 b; f32 radius_a; f32 radius_b; } round_cone_arbitrary; struct { s_vec3 a; s_vec3 b; f32 width; } vesica_segment; struct { f32 length_a; f32 length_b; f32 height; f32 corner_radius; } rhombus; struct { f32 size; } octahedron; struct { f32 size; } octahedron_bound; struct { f32 height; } pyramid; struct { s_vec3 a; s_vec3 b; s_vec3 c; } triangle; struct { s_vec3 a; s_vec3 b; s_vec3 c; s_vec3 d; } quad; }; se_sdf_operation operation; se_sdf_noise noise; s_array(struct se_sdf_object, children); } se_sdf_object;
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

### `se_sdf_plane_desc`

<div class="api-signature">

```c
typedef struct { s_vec3 normal; f32 offset; } se_sdf_plane_desc;
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

### `se_sdf_renderer_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_renderer_handle;
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

### `se_sdf_scene_bounds`

<div class="api-signature">

```c
typedef struct { s_vec3 min; s_vec3 max; s_vec3 center; f32 radius; b8 valid; b8 has_unbounded_primitives; } se_sdf_scene_bounds;
```

</div>

No inline description found in header comments.

### `se_sdf_scene_handle`

<div class="api-signature">

```c
typedef s_handle se_sdf_scene_handle;
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
