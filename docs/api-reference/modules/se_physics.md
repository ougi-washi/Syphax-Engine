<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_physics.h

Source header: `include/se_physics.h`

## Overview

2D and 3D rigid body worlds, shapes, stepping, and queries.

This page is generated from `include/se_physics.h` and is deterministic.

## Read the path walkthrough

- [Deep dive path page](../../path/physics.md)

## Functions

### `se_physics_body_2d_add_aabb`

<div class="api-signature">

```c
extern se_physics_shape_2d_handle se_physics_body_2d_add_aabb(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const s_vec2 *offset, const s_vec2 *half_extents, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_add_box`

<div class="api-signature">

```c
extern se_physics_shape_2d_handle se_physics_body_2d_add_box(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const s_vec2 *offset, const s_vec2 *half_extents, const f32 rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_add_circle`

<div class="api-signature">

```c
extern se_physics_shape_2d_handle se_physics_body_2d_add_circle(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const s_vec2 *offset, const f32 radius, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_add_mesh`

<div class="api-signature">

```c
extern se_physics_shape_2d_handle se_physics_body_2d_add_mesh(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const se_physics_mesh_2d *mesh, const s_vec2 *offset, const f32 rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_apply_force`

<div class="api-signature">

```c
extern void se_physics_body_2d_apply_force(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const s_vec2 *force);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_apply_impulse`

<div class="api-signature">

```c
extern void se_physics_body_2d_apply_impulse(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const s_vec2 *impulse, const s_vec2 *point);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_create`

<div class="api-signature">

```c
extern se_physics_body_2d_handle se_physics_body_2d_create(se_physics_world_2d_handle world, const se_physics_body_params_2d *params);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_destroy`

<div class="api-signature">

```c
extern void se_physics_body_2d_destroy(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_angular_damping`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_angular_damping(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_angular_velocity`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_angular_velocity(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_force`

<div class="api-signature">

```c
extern s_vec2 se_physics_body_2d_get_force(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_friction`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_friction(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_inertia`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_inertia(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_inv_inertia`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_inv_inertia(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_inv_mass`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_inv_mass(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_linear_damping`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_linear_damping(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_mass`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_mass(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_position`

<div class="api-signature">

```c
extern s_vec2 se_physics_body_2d_get_position(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_restitution`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_restitution(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_rotation`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_rotation(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_shape_count`

<div class="api-signature">

```c
extern u32 se_physics_body_2d_get_shape_count(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_torque`

<div class="api-signature">

```c
extern f32 se_physics_body_2d_get_torque(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_type`

<div class="api-signature">

```c
extern se_physics_body_type se_physics_body_2d_get_type(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_get_velocity`

<div class="api-signature">

```c
extern s_vec2 se_physics_body_2d_get_velocity(se_physics_world_2d_handle world, se_physics_body_2d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_angular_damping`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_angular_damping(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const f32 damping);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_angular_velocity`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_angular_velocity(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const f32 angular_velocity);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_force`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_force(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const s_vec2 *force);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_friction`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_friction(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const f32 friction);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_linear_damping`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_linear_damping(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const f32 damping);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_mass`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_mass(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const f32 mass);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_position`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_position(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const s_vec2 *position);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_restitution`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_restitution(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const f32 restitution);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_rotation`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_rotation(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const f32 rotation);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_torque`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_torque(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const f32 torque);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_type`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_type(se_physics_world_2d_handle world, se_physics_body_2d_handle body, se_physics_body_type type);
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_set_velocity`

<div class="api-signature">

```c
extern void se_physics_body_2d_set_velocity(se_physics_world_2d_handle world, se_physics_body_2d_handle body, const s_vec2 *velocity);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_aabb`

<div class="api-signature">

```c
extern se_physics_shape_3d_handle se_physics_body_3d_add_aabb(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *offset, const s_vec3 *half_extents, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_box`

<div class="api-signature">

```c
extern se_physics_shape_3d_handle se_physics_body_3d_add_box(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *offset, const s_vec3 *half_extents, const s_vec3 *rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_mesh`

<div class="api-signature">

```c
extern se_physics_shape_3d_handle se_physics_body_3d_add_mesh(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const se_physics_mesh_3d *mesh, const s_vec3 *offset, const s_vec3 *rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_sdf`

<div class="api-signature">

```c
extern se_physics_shape_3d_handle se_physics_body_3d_add_sdf(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const se_physics_sdf_3d *sdf, const s_vec3 *offset, const s_vec3 *rotation, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_add_sphere`

<div class="api-signature">

```c
extern se_physics_shape_3d_handle se_physics_body_3d_add_sphere(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *offset, const f32 radius, const b8 is_trigger);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_apply_force`

<div class="api-signature">

```c
extern void se_physics_body_3d_apply_force(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *force);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_apply_impulse`

<div class="api-signature">

```c
extern void se_physics_body_3d_apply_impulse(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *impulse, const s_vec3 *point);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_create`

<div class="api-signature">

```c
extern se_physics_body_3d_handle se_physics_body_3d_create(se_physics_world_3d_handle world, const se_physics_body_params_3d *params);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_destroy`

<div class="api-signature">

```c
extern void se_physics_body_3d_destroy(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_angular_damping`

<div class="api-signature">

```c
extern f32 se_physics_body_3d_get_angular_damping(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_angular_velocity`

<div class="api-signature">

```c
extern s_vec3 se_physics_body_3d_get_angular_velocity(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_force`

<div class="api-signature">

```c
extern s_vec3 se_physics_body_3d_get_force(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_friction`

<div class="api-signature">

```c
extern f32 se_physics_body_3d_get_friction(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_inertia`

<div class="api-signature">

```c
extern f32 se_physics_body_3d_get_inertia(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_inv_inertia`

<div class="api-signature">

```c
extern f32 se_physics_body_3d_get_inv_inertia(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_inv_mass`

<div class="api-signature">

```c
extern f32 se_physics_body_3d_get_inv_mass(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_linear_damping`

<div class="api-signature">

```c
extern f32 se_physics_body_3d_get_linear_damping(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_mass`

<div class="api-signature">

```c
extern f32 se_physics_body_3d_get_mass(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_position`

<div class="api-signature">

```c
extern s_vec3 se_physics_body_3d_get_position(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_restitution`

<div class="api-signature">

```c
extern f32 se_physics_body_3d_get_restitution(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_rotation`

<div class="api-signature">

```c
extern s_vec3 se_physics_body_3d_get_rotation(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_shape_count`

<div class="api-signature">

```c
extern u32 se_physics_body_3d_get_shape_count(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_torque`

<div class="api-signature">

```c
extern s_vec3 se_physics_body_3d_get_torque(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_type`

<div class="api-signature">

```c
extern se_physics_body_type se_physics_body_3d_get_type(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_get_velocity`

<div class="api-signature">

```c
extern s_vec3 se_physics_body_3d_get_velocity(se_physics_world_3d_handle world, se_physics_body_3d_handle body);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_angular_damping`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_angular_damping(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const f32 damping);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_angular_velocity`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_angular_velocity(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *angular_velocity);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_force`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_force(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *force);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_friction`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_friction(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const f32 friction);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_linear_damping`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_linear_damping(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const f32 damping);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_mass`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_mass(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const f32 mass);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_position`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_position(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *position);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_restitution`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_restitution(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const f32 restitution);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_rotation`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_rotation(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *rotation);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_torque`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_torque(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *torque);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_type`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_type(se_physics_world_3d_handle world, se_physics_body_3d_handle body, se_physics_body_type type);
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_set_velocity`

<div class="api-signature">

```c
extern void se_physics_body_3d_set_velocity(se_physics_world_3d_handle world, se_physics_body_3d_handle body, const s_vec3 *velocity);
```

</div>

No inline description found in header comments.

### `se_physics_example_2d_create`

<div class="api-signature">

```c
extern se_physics_world_2d_handle se_physics_example_2d_create(void);
```

</div>

No inline description found in header comments.

### `se_physics_example_3d_create`

<div class="api-signature">

```c
extern se_physics_world_3d_handle se_physics_example_3d_create(void);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_create`

<div class="api-signature">

```c
extern se_physics_world_2d_handle se_physics_world_2d_create(const se_physics_world_params_2d *params);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_destroy`

<div class="api-signature">

```c
extern void se_physics_world_2d_destroy(se_physics_world_2d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_get_body`

<div class="api-signature">

```c
extern se_physics_body_2d_handle se_physics_world_2d_get_body(se_physics_world_2d_handle world, const u32 index);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_get_body_count`

<div class="api-signature">

```c
extern u32 se_physics_world_2d_get_body_count(se_physics_world_2d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_get_contact_count`

<div class="api-signature">

```c
extern u32 se_physics_world_2d_get_contact_count(se_physics_world_2d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_get_gravity`

<div class="api-signature">

```c
extern s_vec2 se_physics_world_2d_get_gravity(se_physics_world_2d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_get_iterations`

<div class="api-signature">

```c
extern u32 se_physics_world_2d_get_iterations(se_physics_world_2d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_get_shape_limit`

<div class="api-signature">

```c
extern u32 se_physics_world_2d_get_shape_limit(se_physics_world_2d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_raycast`

<div class="api-signature">

```c
extern b8 se_physics_world_2d_raycast(se_physics_world_2d_handle world, const s_vec2 *origin, const s_vec2 *direction, const f32 max_distance, se_physics_raycast_hit_2d *out_hit);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_set_gravity`

<div class="api-signature">

```c
extern void se_physics_world_2d_set_gravity(se_physics_world_2d_handle world, const s_vec2 *gravity);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_set_iterations`

<div class="api-signature">

```c
extern void se_physics_world_2d_set_iterations(se_physics_world_2d_handle world, const u32 iterations);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_set_shape_limit`

<div class="api-signature">

```c
extern void se_physics_world_2d_set_shape_limit(se_physics_world_2d_handle world, const u32 count);
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_step`

<div class="api-signature">

```c
extern void se_physics_world_2d_step(se_physics_world_2d_handle world, const f32 dt);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_create`

<div class="api-signature">

```c
extern se_physics_world_3d_handle se_physics_world_3d_create(const se_physics_world_params_3d *params);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_destroy`

<div class="api-signature">

```c
extern void se_physics_world_3d_destroy(se_physics_world_3d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_get_body`

<div class="api-signature">

```c
extern se_physics_body_3d_handle se_physics_world_3d_get_body(se_physics_world_3d_handle world, const u32 index);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_get_body_count`

<div class="api-signature">

```c
extern u32 se_physics_world_3d_get_body_count(se_physics_world_3d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_get_contact_count`

<div class="api-signature">

```c
extern u32 se_physics_world_3d_get_contact_count(se_physics_world_3d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_get_gravity`

<div class="api-signature">

```c
extern s_vec3 se_physics_world_3d_get_gravity(se_physics_world_3d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_get_iterations`

<div class="api-signature">

```c
extern u32 se_physics_world_3d_get_iterations(se_physics_world_3d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_get_shape_limit`

<div class="api-signature">

```c
extern u32 se_physics_world_3d_get_shape_limit(se_physics_world_3d_handle world);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_raycast`

<div class="api-signature">

```c
extern b8 se_physics_world_3d_raycast(se_physics_world_3d_handle world, const s_vec3 *origin, const s_vec3 *direction, const f32 max_distance, se_physics_raycast_hit_3d *out_hit);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_set_gravity`

<div class="api-signature">

```c
extern void se_physics_world_3d_set_gravity(se_physics_world_3d_handle world, const s_vec3 *gravity);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_set_iterations`

<div class="api-signature">

```c
extern void se_physics_world_3d_set_iterations(se_physics_world_3d_handle world, const u32 iterations);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_set_shape_limit`

<div class="api-signature">

```c
extern void se_physics_world_3d_set_shape_limit(se_physics_world_3d_handle world, const u32 count);
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_step`

<div class="api-signature">

```c
extern void se_physics_world_3d_step(se_physics_world_3d_handle world, const f32 dt);
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
typedef enum { SE_PHYSICS_SHAPE_3D_AABB = 0, SE_PHYSICS_SHAPE_3D_BOX, SE_PHYSICS_SHAPE_3D_SPHERE, SE_PHYSICS_SHAPE_3D_MESH, SE_PHYSICS_SHAPE_3D_SDF } se_physics_shape_type_3d;
```

</div>

No inline description found in header comments.

## Typedefs

### `se_physics_body_2d`

<div class="api-signature">

```c
typedef struct se_physics_body_2d se_physics_body_2d;
```

</div>

No inline description found in header comments.

### `se_physics_body_2d_handle`

<div class="api-signature">

```c
typedef s_handle se_physics_body_2d_handle;
```

</div>

No inline description found in header comments.

### `se_physics_body_3d`

<div class="api-signature">

```c
typedef struct se_physics_body_3d se_physics_body_3d;
```

</div>

No inline description found in header comments.

### `se_physics_body_3d_handle`

<div class="api-signature">

```c
typedef s_handle se_physics_body_3d_handle;
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
typedef struct { se_physics_body_2d_handle a; se_physics_body_2d_handle b; se_physics_shape_2d_handle shape_a; se_physics_shape_2d_handle shape_b; s_vec2 normal; f32 penetration; s_vec2 contact_point; f32 restitution; f32 friction; b8 is_trigger : 1; } se_physics_contact_2d;
```

</div>

No inline description found in header comments.

### `se_physics_contact_3d`

<div class="api-signature">

```c
typedef struct { se_physics_body_3d_handle a; se_physics_body_3d_handle b; se_physics_shape_3d_handle shape_a; se_physics_shape_3d_handle shape_b; s_vec3 normal; f32 penetration; s_vec3 contact_point; f32 restitution; f32 friction; b8 is_trigger : 1; } se_physics_contact_3d;
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
typedef struct { se_physics_body_2d_handle body; se_physics_shape_2d_handle shape; s_vec2 point; s_vec2 normal; f32 distance; } se_physics_raycast_hit_2d;
```

</div>

No inline description found in header comments.

### `se_physics_raycast_hit_3d`

<div class="api-signature">

```c
typedef struct { se_physics_body_3d_handle body; se_physics_shape_3d_handle shape; s_vec3 point; s_vec3 normal; f32 distance; } se_physics_raycast_hit_3d;
```

</div>

No inline description found in header comments.

### `se_physics_sdf_3d`

<div class="api-signature">

```c
typedef struct { void *user_data; se_box_3d local_bounds; se_physics_sdf_sample_fn_3d sample; } se_physics_sdf_3d;
```

</div>

No inline description found in header comments.

### `se_physics_sdf_sample_fn_3d`

<div class="api-signature">

```c
typedef b8 (*se_physics_sdf_sample_fn_3d)(void *user_data, const s_vec3 *local_point, f32 *out_distance, s_vec3 *out_normal);
```

</div>

No inline description found in header comments.

### `se_physics_shape_2d`

<div class="api-signature">

```c
typedef struct se_physics_shape_2d se_physics_shape_2d;
```

</div>

No inline description found in header comments.

### `se_physics_shape_2d_handle`

<div class="api-signature">

```c
typedef s_handle se_physics_shape_2d_handle;
```

</div>

No inline description found in header comments.

### `se_physics_shape_3d`

<div class="api-signature">

```c
typedef struct se_physics_shape_3d se_physics_shape_3d;
```

</div>

No inline description found in header comments.

### `se_physics_shape_3d_handle`

<div class="api-signature">

```c
typedef s_handle se_physics_shape_3d_handle;
```

</div>

No inline description found in header comments.

### `se_physics_world_2d`

<div class="api-signature">

```c
typedef struct se_physics_world_2d se_physics_world_2d;
```

</div>

No inline description found in header comments.

### `se_physics_world_2d_handle`

<div class="api-signature">

```c
typedef s_handle se_physics_world_2d_handle;
```

</div>

No inline description found in header comments.

### `se_physics_world_3d`

<div class="api-signature">

```c
typedef struct se_physics_world_3d se_physics_world_3d;
```

</div>

No inline description found in header comments.

### `se_physics_world_3d_handle`

<div class="api-signature">

```c
typedef s_handle se_physics_world_3d_handle;
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
typedef s_array(se_physics_bvh_node_2d, se_physics_bvh_nodes_2d);
```

</div>

No inline description found in header comments.

### `typedef_2`

<div class="api-signature">

```c
typedef s_array(se_physics_bvh_node_3d, se_physics_bvh_nodes_3d);
```

</div>

No inline description found in header comments.
