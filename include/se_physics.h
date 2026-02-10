// Syphax-Engine - Ougi Washi

#ifndef SE_PHYSICS_H
#define SE_PHYSICS_H

#include "se_defines.h"
#include "se_math.h"
#include "syphax/s_array.h"

typedef enum {
	SE_PHYSICS_BODY_STATIC = 0,
	SE_PHYSICS_BODY_KINEMATIC,
	SE_PHYSICS_BODY_DYNAMIC
} se_physics_body_type;

typedef enum {
	SE_PHYSICS_SHAPE_2D_AABB = 0,
	SE_PHYSICS_SHAPE_2D_BOX,
	SE_PHYSICS_SHAPE_2D_CIRCLE,
	SE_PHYSICS_SHAPE_2D_MESH
} se_physics_shape_type_2d;

typedef enum {
	SE_PHYSICS_SHAPE_3D_AABB = 0,
	SE_PHYSICS_SHAPE_3D_BOX,
	SE_PHYSICS_SHAPE_3D_SPHERE,
	SE_PHYSICS_SHAPE_3D_MESH
} se_physics_shape_type_3d;

typedef struct {
	const s_vec2 *vertices;
	sz vertex_count;
	const u32 *indices;
	sz index_count;
} se_physics_mesh_2d;

typedef struct {
	const s_vec3 *vertices;
	sz vertex_count;
	const u32 *indices;
	sz index_count;
} se_physics_mesh_3d;

typedef struct {
	se_box_2d bounds;
	u32 left;
	u32 right;
	u32 start;
	u32 count;
	b8 is_leaf : 1;
} se_physics_bvh_node_2d;

typedef struct {
	se_box_3d bounds;
	u32 left;
	u32 right;
	u32 start;
	u32 count;
	b8 is_leaf : 1;
} se_physics_bvh_node_3d;

typedef s_array(se_physics_bvh_node_2d, se_physics_bvh_nodes_2d);
typedef s_array(se_physics_bvh_node_3d, se_physics_bvh_nodes_3d);

typedef struct {
	se_physics_shape_type_2d type;
	s_vec2 offset;
	f32 rotation;
	union {
		struct {
			f32 radius;
		} circle;
		struct {
			s_vec2 half_extents;
			b8 is_aabb;
		} box;
		se_physics_mesh_2d mesh;
	};
	se_box_2d local_bounds;
	se_physics_bvh_nodes_2d bvh_nodes;
	u32 *bvh_triangles;
	sz bvh_triangle_count;
	b8 bvh_built : 1;
	b8 is_trigger : 1;
} se_physics_shape_2d;

typedef struct {
	se_physics_shape_type_3d type;
	s_vec3 offset;
	s_vec3 rotation;
	union {
		struct {
			f32 radius;
		} sphere;
		struct {
			s_vec3 half_extents;
			b8 is_aabb;
		} box;
		se_physics_mesh_3d mesh;
	};
	se_box_3d local_bounds;
	se_physics_bvh_nodes_3d bvh_nodes;
	u32 *bvh_triangles;
	sz bvh_triangle_count;
	b8 bvh_built : 1;
	b8 is_trigger : 1;
} se_physics_shape_3d;

typedef s_array(se_physics_shape_2d, se_physics_shapes_2d);
typedef s_array(se_physics_shape_3d, se_physics_shapes_3d);

typedef struct {
	se_physics_body_type type;
	s_vec2 position;
	f32 rotation;
	s_vec2 velocity;
	f32 angular_velocity;
	s_vec2 force;
	f32 torque;
	f32 mass;
	f32 inv_mass;
	f32 inertia;
	f32 inv_inertia;
	f32 restitution;
	f32 friction;
	f32 linear_damping;
	f32 angular_damping;
	se_physics_shapes_2d shapes;
	b8 is_valid : 1;
} se_physics_body_2d;

typedef struct {
	se_physics_body_type type;
	s_vec3 position;
	s_vec3 rotation;
	s_vec3 velocity;
	s_vec3 angular_velocity;
	s_vec3 force;
	s_vec3 torque;
	f32 mass;
	f32 inv_mass;
	f32 inertia;
	f32 inv_inertia;
	f32 restitution;
	f32 friction;
	f32 linear_damping;
	f32 angular_damping;
	se_physics_shapes_3d shapes;
	b8 is_valid : 1;
} se_physics_body_3d;

typedef struct {
	se_physics_body_2d *a;
	se_physics_body_2d *b;
	se_physics_shape_2d *shape_a;
	se_physics_shape_2d *shape_b;
	s_vec2 normal;
	f32 penetration;
	s_vec2 contact_point;
	f32 restitution;
	f32 friction;
	b8 is_trigger : 1;
} se_physics_contact_2d;

typedef struct {
	se_physics_body_3d *a;
	se_physics_body_3d *b;
	se_physics_shape_3d *shape_a;
	se_physics_shape_3d *shape_b;
	s_vec3 normal;
	f32 penetration;
	s_vec3 contact_point;
	f32 restitution;
	f32 friction;
	b8 is_trigger : 1;
} se_physics_contact_3d;

typedef struct {
	se_physics_body_2d *body;
	se_physics_shape_2d *shape;
	s_vec2 point;
	s_vec2 normal;
	f32 distance;
} se_physics_raycast_hit_2d;

typedef struct {
	se_physics_body_3d *body;
	se_physics_shape_3d *shape;
	s_vec3 point;
	s_vec3 normal;
	f32 distance;
} se_physics_raycast_hit_3d;

typedef s_array(se_physics_body_2d, se_physics_bodies_2d);
typedef s_array(se_physics_body_3d, se_physics_bodies_3d);
typedef s_array(se_physics_contact_2d, se_physics_contacts_2d);
typedef s_array(se_physics_contact_3d, se_physics_contacts_3d);

typedef void (*se_physics_contact_callback_2d)(const se_physics_contact_2d *contact, void *user_data);
typedef void (*se_physics_contact_callback_3d)(const se_physics_contact_3d *contact, void *user_data);

typedef struct {
	s_vec2 gravity;
	u32 bodies_count;
	u32 shapes_per_body;
	u32 contacts_count;
	u32 solver_iterations;
	se_physics_contact_callback_2d on_contact;
	void *user_data;
} se_physics_world_params_2d;

typedef struct {
	s_vec3 gravity;
	u32 bodies_count;
	u32 shapes_per_body;
	u32 contacts_count;
	u32 solver_iterations;
	se_physics_contact_callback_3d on_contact;
	void *user_data;
} se_physics_world_params_3d;

#define SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS ((se_physics_world_params_2d){ .gravity = s_vec2(0.0f, -9.81f), .bodies_count = 128, .shapes_per_body = 8, .contacts_count = 256, .solver_iterations = 8, .on_contact = NULL, .user_data = NULL })
#define SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS ((se_physics_world_params_3d){ .gravity = s_vec3(0.0f, -9.81f, 0.0f), .bodies_count = 128, .shapes_per_body = 8, .contacts_count = 256, .solver_iterations = 8, .on_contact = NULL, .user_data = NULL })

typedef struct {
	se_physics_body_type type;
	s_vec2 position;
	f32 rotation;
	s_vec2 velocity;
	f32 angular_velocity;
	f32 mass;
	f32 restitution;
	f32 friction;
	f32 linear_damping;
	f32 angular_damping;
} se_physics_body_params_2d;

typedef struct {
	se_physics_body_type type;
	s_vec3 position;
	s_vec3 rotation;
	s_vec3 velocity;
	s_vec3 angular_velocity;
	f32 mass;
	f32 restitution;
	f32 friction;
	f32 linear_damping;
	f32 angular_damping;
} se_physics_body_params_3d;

#define SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS ((se_physics_body_params_2d){ .type = SE_PHYSICS_BODY_DYNAMIC, .position = s_vec2(0.0f, 0.0f), .rotation = 0.0f, .velocity = s_vec2(0.0f, 0.0f), .angular_velocity = 0.0f, .mass = 1.0f, .restitution = 0.2f, .friction = 0.6f, .linear_damping = 0.01f, .angular_damping = 0.02f })
#define SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS ((se_physics_body_params_3d){ .type = SE_PHYSICS_BODY_DYNAMIC, .position = s_vec3(0.0f, 0.0f, 0.0f), .rotation = s_vec3(0.0f, 0.0f, 0.0f), .velocity = s_vec3(0.0f, 0.0f, 0.0f), .angular_velocity = s_vec3(0.0f, 0.0f, 0.0f), .mass = 1.0f, .restitution = 0.2f, .friction = 0.6f, .linear_damping = 0.01f, .angular_damping = 0.02f })

typedef struct {
	s_vec2 gravity;
	u32 solver_iterations;
	u32 shapes_per_body;
	se_physics_bodies_2d bodies;
	se_physics_contacts_2d contacts;
	se_physics_contact_callback_2d on_contact;
	void *user_data;
} se_physics_world_2d;

typedef struct {
	s_vec3 gravity;
	u32 solver_iterations;
	u32 shapes_per_body;
	se_physics_bodies_3d bodies;
	se_physics_contacts_3d contacts;
	se_physics_contact_callback_3d on_contact;
	void *user_data;
} se_physics_world_3d;

extern se_physics_world_2d *se_physics_world_2d_create(const se_physics_world_params_2d *params);
extern void se_physics_world_2d_destroy(se_physics_world_2d *world);
extern se_physics_world_3d *se_physics_world_3d_create(const se_physics_world_params_3d *params);
extern void se_physics_world_3d_destroy(se_physics_world_3d *world);

extern se_physics_body_2d *se_physics_body_2d_create(se_physics_world_2d *world, const se_physics_body_params_2d *params);
extern void se_physics_body_2d_destroy(se_physics_world_2d *world, se_physics_body_2d *body);
extern se_physics_body_3d *se_physics_body_3d_create(se_physics_world_3d *world, const se_physics_body_params_3d *params);
extern void se_physics_body_3d_destroy(se_physics_world_3d *world, se_physics_body_3d *body);

extern se_physics_shape_2d *se_physics_body_2d_add_circle(se_physics_body_2d *body, const s_vec2 *offset, const f32 radius, const b8 is_trigger);
extern se_physics_shape_2d *se_physics_body_2d_add_aabb(se_physics_body_2d *body, const s_vec2 *offset, const s_vec2 *half_extents, const b8 is_trigger);
extern se_physics_shape_2d *se_physics_body_2d_add_box(se_physics_body_2d *body, const s_vec2 *offset, const s_vec2 *half_extents, const f32 rotation, const b8 is_trigger);
extern se_physics_shape_2d *se_physics_body_2d_add_mesh(se_physics_body_2d *body, const se_physics_mesh_2d *mesh, const s_vec2 *offset, const f32 rotation, const b8 is_trigger);

extern se_physics_shape_3d *se_physics_body_3d_add_sphere(se_physics_body_3d *body, const s_vec3 *offset, const f32 radius, const b8 is_trigger);
extern se_physics_shape_3d *se_physics_body_3d_add_aabb(se_physics_body_3d *body, const s_vec3 *offset, const s_vec3 *half_extents, const b8 is_trigger);
extern se_physics_shape_3d *se_physics_body_3d_add_box(se_physics_body_3d *body, const s_vec3 *offset, const s_vec3 *half_extents, const s_vec3 *rotation, const b8 is_trigger);
extern se_physics_shape_3d *se_physics_body_3d_add_mesh(se_physics_body_3d *body, const se_physics_mesh_3d *mesh, const s_vec3 *offset, const s_vec3 *rotation, const b8 is_trigger);

extern void se_physics_body_2d_set_position(se_physics_body_2d *body, const s_vec2 *position);
extern void se_physics_body_2d_set_velocity(se_physics_body_2d *body, const s_vec2 *velocity);
extern void se_physics_body_2d_set_rotation(se_physics_body_2d *body, const f32 rotation);
extern void se_physics_body_2d_apply_force(se_physics_body_2d *body, const s_vec2 *force);
extern void se_physics_body_2d_apply_impulse(se_physics_body_2d *body, const s_vec2 *impulse, const s_vec2 *point);

extern void se_physics_body_3d_set_position(se_physics_body_3d *body, const s_vec3 *position);
extern void se_physics_body_3d_set_velocity(se_physics_body_3d *body, const s_vec3 *velocity);
extern void se_physics_body_3d_set_rotation(se_physics_body_3d *body, const s_vec3 *rotation);
extern void se_physics_body_3d_apply_force(se_physics_body_3d *body, const s_vec3 *force);
extern void se_physics_body_3d_apply_impulse(se_physics_body_3d *body, const s_vec3 *impulse, const s_vec3 *point);

extern void se_physics_world_2d_set_gravity(se_physics_world_2d *world, const s_vec2 *gravity);
extern void se_physics_world_3d_set_gravity(se_physics_world_3d *world, const s_vec3 *gravity);
extern void se_physics_world_2d_step(se_physics_world_2d *world, const f32 dt);
extern void se_physics_world_3d_step(se_physics_world_3d *world, const f32 dt);

extern b8 se_physics_world_2d_raycast(se_physics_world_2d *world, const s_vec2 *origin, const s_vec2 *direction, const f32 max_distance, se_physics_raycast_hit_2d *out_hit);
extern b8 se_physics_world_3d_raycast(se_physics_world_3d *world, const s_vec3 *origin, const s_vec3 *direction, const f32 max_distance, se_physics_raycast_hit_3d *out_hit);

extern se_physics_world_2d *se_physics_example_2d_create(void);
extern se_physics_world_3d *se_physics_example_3d_create(void);

#endif // SE_PHYSICS_H
