<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_physics.h

Source header: `include/se_physics.h`

## Overview

2D and 3D rigid body worlds, shapes, stepping, and queries.

This page is generated from `include/se_physics.h` and is deterministic.

## Read the Playbook

- [Deep dive Playbook](../../playbooks/se-physics.md)

## Functions

### `se_physics_body_2d_add_aabb`

<div class="api-signature">

```c
extern se_physics_shape_2d *se_physics_body_2d_add_aabb(se_physics_body_2d *body, const s_vec2 *offset, const s_vec2 *half_extents, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_add_box`

<div class="api-signature">

```c
extern se_physics_shape_2d *se_physics_body_2d_add_box(se_physics_body_2d *body, const s_vec2 *offset, const s_vec2 *half_extents, const f32 rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_add_circle`

<div class="api-signature">

```c
extern se_physics_shape_2d *se_physics_body_2d_add_circle(se_physics_body_2d *body, const s_vec2 *offset, const f32 radius, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_add_mesh`

<div class="api-signature">

```c
extern se_physics_shape_2d *se_physics_body_2d_add_mesh(se_physics_body_2d *body, const se_physics_mesh_2d *mesh, const s_vec2 *offset, const f32 rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_apply_force`

<div class="api-signature">

```c
extern void se_physics_body_2d_apply_force(se_physics_body_2d *body, const s_vec2 *force);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_apply_impulse`

<div class="api-signature">

```c
extern void se_physics_body_2d_apply_impulse(se_physics_body_2d *body, const s_vec2 *impulse, const s_vec2 *point);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_create`

<div class="api-signature">

```c
extern se_physics_body_2d *se_physics_body_2d_create(se_physics_world_2d *world, const se_physics_body_params_2d *params);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_destroy`

<div class="api-signature">

```c
extern void se_physics_body_2d_destroy(se_physics_world_2d *world, se_physics_body_2d *body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_position`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_position(se_physics_body_2d *body, const s_vec2 *position);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_rotation`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_rotation(se_physics_body_2d *body, const f32 rotation);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_velocity`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_velocity(se_physics_body_2d *body, const s_vec2 *velocity);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_aabb`

<div class="api-signature">

```c
extern se_physics_shape_3d *se_physics_body_3d_add_aabb(se_physics_body_3d *body, const s_vec3 *offset, const s_vec3 *half_extents, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_box`

<div class="api-signature">

```c
extern se_physics_shape_3d *se_physics_body_3d_add_box(se_physics_body_3d *body, const s_vec3 *offset, const s_vec3 *half_extents, const s_vec3 *rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_mesh`

<div class="api-signature">

```c
extern se_physics_shape_3d *se_physics_body_3d_add_mesh(se_physics_body_3d *body, const se_physics_mesh_3d *mesh, const s_vec3 *offset, const s_vec3 *rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_sphere`

<div class="api-signature">

```c
extern se_physics_shape_3d *se_physics_body_3d_add_sphere(se_physics_body_3d *body, const s_vec3 *offset, const f32 radius, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_apply_force`

<div class="api-signature">

```c
extern void se_physics_body_3d_apply_force(se_physics_body_3d *body, const s_vec3 *force);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_apply_impulse`

<div class="api-signature">

```c
extern void se_physics_body_3d_apply_impulse(se_physics_body_3d *body, const s_vec3 *impulse, const s_vec3 *point);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_create`

<div class="api-signature">

```c
extern se_physics_body_3d *se_physics_body_3d_create(se_physics_world_3d *world, const se_physics_body_params_3d *params);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_destroy`

<div class="api-signature">

```c
extern void se_physics_body_3d_destroy(se_physics_world_3d *world, se_physics_body_3d *body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_position`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_position(se_physics_body_3d *body, const s_vec3 *position);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_rotation`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_rotation(se_physics_body_3d *body, const s_vec3 *rotation);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_velocity`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_velocity(se_physics_body_3d *body, const s_vec3 *velocity);
```

</div>

No inline description found in header comments.

### `se_physics_example_2d_create`

<div class="api-signature">

```c
extern se_physics_world_2d *se_physics_example_2d_create(void);
```

</div>

No inline description found in header comments.

### `se_physics_example_3d_create`

<div class="api-signature">

```c
extern se_physics_world_3d *se_physics_example_3d_create(void);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_create`

<div class="api-signature">

```c
extern se_physics_world_2d *se_physics_world_2d_create(const se_physics_world_params_2d *params);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_destroy`

<div class="api-signature">

```c
extern void se_physics_world_2d_destroy(se_physics_world_2d *world);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_raycast`

<div class="api-signature">

```c
extern b8 se_physics_world_2d_raycast(se_physics_world_2d *world, const s_vec2 *origin, const s_vec2 *direction, const f32 max_distance, se_physics_raycast_hit_2d *out_hit);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_set_gravity`

<div class="api-signature">

```c
extern void se_physics_world_2d_set_gravity(se_physics_world_2d *world, const s_vec2 *gravity);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_step`

<div class="api-signature">

```c
extern void se_physics_world_2d_step(se_physics_world_2d *world, const f32 dt);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_create`

<div class="api-signature">

```c
extern se_physics_world_3d *se_physics_world_3d_create(const se_physics_world_params_3d *params);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_destroy`

<div class="api-signature">

```c
extern void se_physics_world_3d_destroy(se_physics_world_3d *world);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_raycast`

<div class="api-signature">

```c
extern b8 se_physics_world_3d_raycast(se_physics_world_3d *world, const s_vec3 *origin, const s_vec3 *direction, const f32 max_distance, se_physics_raycast_hit_3d *out_hit);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_set_gravity`

<div class="api-signature">

```c
extern void se_physics_world_3d_set_gravity(se_physics_world_3d *world, const s_vec3 *gravity);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_step`

<div class="api-signature">

```c
extern void se_physics_world_3d_step(se_physics_world_3d *world, const f32 dt);
```

</div>

No inline description found in header comments.

## Enums

### `se_physics_body_type`

<div class="api-signature">

```c
typedef enum { SE_PHYSICS_BODY_STATIC = 0, SE_PHYSICS_BODY_KINEMATIC, SE_PHYSICS_BODY_DYNAMIC } se_physics_body_type;
```

</div>

No inline description found in header comments.

### `se_physics_shape_type_2d`

<div class="api-signature">

```c
typedef enum { SE_PHYSICS_SHAPE_2D_AABB = 0, SE_PHYSICS_SHAPE_2D_BOX, SE_PHYSICS_SHAPE_2D_CIRCLE, SE_PHYSICS_SHAPE_2D_MESH } se_physics_shape_type_2d;
```

</div>

No inline description found in header comments.

### `se_physics_shape_type_3d`

<div class="api-signature">

```c
typedef enum { SE_PHYSICS_SHAPE_3D_AABB = 0, SE_PHYSICS_SHAPE_3D_BOX, SE_PHYSICS_SHAPE_3D_SPHERE, SE_PHYSICS_SHAPE_3D_MESH } se_physics_shape_type_3d;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_physics_body_2d`

<div class="api-signature">

```c
typedef struct { se_physics_body_type type; s_vec2 position; f32 rotation; s_vec2 velocity; f32 angular_velocity; s_vec2 force; f32 torque; f32 mass; f32 inv_mass; f32 inertia; f32 inv_inertia; f32 restitution; f32 friction; f32 linear_damping; f32 angular_damping; se_physics_shapes_2d shapes; b8 is_valid : 1; } se_physics_body_2d;
```

</div>

No inline description found in header comments.

### `se_physics_body_3d`

<div class="api-signature">

```c
typedef struct { se_physics_body_type type; s_vec3 position; s_vec3 rotation; s_vec3 velocity; s_vec3 angular_velocity; s_vec3 force; s_vec3 torque; f32 mass; f32 inv_mass; f32 inertia; f32 inv_inertia; f32 restitution; f32 friction; f32 linear_damping; f32 angular_damping; se_physics_shapes_3d shapes; b8 is_valid : 1; } se_physics_body_3d;
```

</div>

No inline description found in header comments.

### `se_physics_body_params_2d`

<div class="api-signature">

```c
typedef struct { se_physics_body_type type; s_vec2 position; f32 rotation; s_vec2 velocity; f32 angular_velocity; f32 mass; f32 restitution; f32 friction; f32 linear_damping; f32 angular_damping; } se_physics_body_params_2d;
```

</div>

No inline description found in header comments.

### `se_physics_body_params_3d`

<div class="api-signature">

```c
typedef struct { se_physics_body_type type; s_vec3 position; s_vec3 rotation; s_vec3 velocity; s_vec3 angular_velocity; f32 mass; f32 restitution; f32 friction; f32 linear_damping; f32 angular_damping; } se_physics_body_params_3d;
```

</div>

No inline description found in header comments.

### `se_physics_bvh_node_2d`

<div class="api-signature">

```c
typedef struct { se_box_2d bounds; u32 left; u32 right; u32 start; u32 count; b8 is_leaf : 1; } se_physics_bvh_node_2d;
```

</div>

No inline description found in header comments.

### `se_physics_bvh_node_3d`

<div class="api-signature">

```c
typedef struct { se_box_3d bounds; u32 left; u32 right; u32 start; u32 count; b8 is_leaf : 1; } se_physics_bvh_node_3d;
```

</div>

No inline description found in header comments.

### `se_physics_contact_2d`

<div class="api-signature">

```c
typedef struct { se_physics_body_2d *a; se_physics_body_2d *b; se_physics_shape_2d *shape_a; se_physics_shape_2d *shape_b; s_vec2 normal; f32 penetration; s_vec2 contact_point; f32 restitution; f32 friction; b8 is_trigger : 1; } se_physics_contact_2d;
```

</div>

No inline description found in header comments.

### `se_physics_contact_3d`

<div class="api-signature">

```c
typedef struct { se_physics_body_3d *a; se_physics_body_3d *b; se_physics_shape_3d *shape_a; se_physics_shape_3d *shape_b; s_vec3 normal; f32 penetration; s_vec3 contact_point; f32 restitution; f32 friction; b8 is_trigger : 1; } se_physics_contact_3d;
```

</div>

No inline description found in header comments.

### `se_physics_contact_callback_2d`

<div class="api-signature">

```c
typedef void (*se_physics_contact_callback_2d)(const se_physics_contact_2d *contact, void *user_data);
```

</div>

No inline description found in header comments.

### `se_physics_contact_callback_3d`

<div class="api-signature">

```c
typedef void (*se_physics_contact_callback_3d)(const se_physics_contact_3d *contact, void *user_data);
```

</div>

No inline description found in header comments.

### `se_physics_mesh_2d`

<div class="api-signature">

```c
typedef struct { const s_vec2 *vertices; sz vertex_count; const u32 *indices; sz index_count; } se_physics_mesh_2d;
```

</div>

No inline description found in header comments.

### `se_physics_mesh_3d`

<div class="api-signature">

```c
typedef struct { const s_vec3 *vertices; sz vertex_count; const u32 *indices; sz index_count; } se_physics_mesh_3d;
```

</div>

No inline description found in header comments.

### `se_physics_raycast_hit_2d`

<div class="api-signature">

```c
typedef struct { se_physics_body_2d *body; se_physics_shape_2d *shape; s_vec2 point; s_vec2 normal; f32 distance; } se_physics_raycast_hit_2d;
```

</div>

No inline description found in header comments.

### `se_physics_raycast_hit_3d`

<div class="api-signature">

```c
typedef struct { se_physics_body_3d *body; se_physics_shape_3d *shape; s_vec3 point; s_vec3 normal; f32 distance; } se_physics_raycast_hit_3d;
```

</div>

No inline description found in header comments.

### `se_physics_shape_2d`

<div class="api-signature">

```c
typedef struct { se_physics_shape_type_2d type; s_vec2 offset; f32 rotation; union { struct { f32 radius; } circle; struct { s_vec2 half_extents; b8 is_aabb; } box; se_physics_mesh_2d mesh; }; se_box_2d local_bounds; se_physics_bvh_nodes_2d bvh_nodes; u32 *bvh_triangles; sz bvh_triangle_count; b8 bvh_built : 1; b8 is_trigger : 1; } se_physics_shape_2d;
```

</div>

No inline description found in header comments.

### `se_physics_shape_3d`

<div class="api-signature">

```c
typedef struct { se_physics_shape_type_3d type; s_vec3 offset; s_vec3 rotation; union { struct { f32 radius; } sphere; struct { s_vec3 half_extents; b8 is_aabb; } box; se_physics_mesh_3d mesh; }; se_box_3d local_bounds; se_physics_bvh_nodes_3d bvh_nodes; u32 *bvh_triangles; sz bvh_triangle_count; b8 bvh_built : 1; b8 is_trigger : 1; } se_physics_shape_3d;
```

</div>

No inline description found in header comments.

### `se_physics_world_2d`

<div class="api-signature">

```c
typedef struct { s_vec2 gravity; u32 solver_iterations; u32 shapes_per_body; se_physics_bodies_2d bodies; se_physics_contacts_2d contacts; se_physics_contact_callback_2d on_contact; void *user_data; } se_physics_world_2d;
```

</div>

No inline description found in header comments.

### `se_physics_world_3d`

<div class="api-signature">

```c
typedef struct { s_vec3 gravity; u32 solver_iterations; u32 shapes_per_body; se_physics_bodies_3d bodies; se_physics_contacts_3d contacts; se_physics_contact_callback_3d on_contact; void *user_data; } se_physics_world_3d;
```

</div>

No inline description found in header comments.

### `se_physics_world_params_2d`

<div class="api-signature">

```c
typedef struct { s_vec2 gravity; u32 bodies_count; u32 shapes_per_body; u32 contacts_count; u32 solver_iterations; se_physics_contact_callback_2d on_contact; void *user_data; } se_physics_world_params_2d;
```

</div>

No inline description found in header comments.

### `se_physics_world_params_3d`

<div class="api-signature">

```c
typedef struct { s_vec3 gravity; u32 bodies_count; u32 shapes_per_body; u32 contacts_count; u32 solver_iterations; se_physics_contact_callback_3d on_contact; void *user_data; } se_physics_world_params_3d;
```

</div>

No inline description found in header comments.

### `typedef`

<div class="api-signature">

```c
typedef s_array(se_physics_body_2d, se_physics_bodies_2d);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_physics_body_3d, se_physics_bodies_3d);
```

</div>

No inline description found in header comments.

### `typedef_3`

<div class="api-signature">

```c
typedef s_array(se_physics_bvh_node_2d, se_physics_bvh_nodes_2d);
```

</div>

No inline description found in header comments.

### `typedef_4`

<div class="api-signature">

```c
typedef s_array(se_physics_bvh_node_3d, se_physics_bvh_nodes_3d);
```

</div>

No inline description found in header comments.

### `typedef_5`

<div class="api-signature">

```c
typedef s_array(se_physics_contact_2d, se_physics_contacts_2d);
```

</div>

No inline description found in header comments.

### `typedef_6`

<div class="api-signature">

```c
typedef s_array(se_physics_contact_3d, se_physics_contacts_3d);
```

</div>

No inline description found in header comments.

### `typedef_7`

<div class="api-signature">

```c
typedef s_array(se_physics_shape_2d, se_physics_shapes_2d);
```

</div>

No inline description found in header comments.

### `typedef_8`

<div class="api-signature">

```c
typedef s_array(se_physics_shape_3d, se_physics_shapes_3d);
```

</div>

No inline description found in header comments.
