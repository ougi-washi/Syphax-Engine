// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_physics.h"
#include "se_scene.h"

#include <stdio.h>

// What you will learn: run 3D rigid-body simulation and sync it to instances.
// Try this: increase body count, gravity, or the bounce impulse strength.
#define BODY_COUNT 8

typedef struct {
	se_physics_body_3d* body;
	se_instance_id instance;
	s_vec3 half_extents;
} physics_slot_3d;
typedef s_array(physics_slot_3d, physics_slots_3d);

static s_mat4 physics_make_transform(const s_vec3* position, const s_vec3* half_extents) {
	s_mat4 transform = s_mat4_identity;
	transform = s_mat4_scale(&transform, &s_vec3(half_extents->x * 2.0f, half_extents->y * 2.0f, half_extents->z * 2.0f));
	s_mat4_set_translation(&transform, position);
	return transform;
}

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Physics3D Playground", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("physics3d_playground :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.04f, 0.05f, 0.07f, 1.0f));

	se_scene_3d_handle scene = se_scene_3d_create_for_window(window, BODY_COUNT + 2);
	se_model_handle cube_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("physics3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("physics3d/scene3d_fragment.glsl"));
	se_object_3d_handle cubes = se_object_3d_create(cube_model, &s_mat4_identity, BODY_COUNT + 2);
	se_scene_3d_add_object(scene, cubes);

	se_physics_world_params_3d world_params = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS;
	world_params.gravity = s_vec3(0.0f, -3.2f, 0.0f);
	se_physics_world_3d* world = se_physics_world_3d_create(&world_params);

	se_physics_body_params_3d ground_params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	ground_params.type = SE_PHYSICS_BODY_STATIC;
	ground_params.position = s_vec3(0.0f, -2.5f, 0.0f);
	se_physics_body_3d* ground = se_physics_body_3d_create(world, &ground_params);
	s_vec3 ground_half = s_vec3(6.0f, 0.5f, 6.0f);
	se_physics_body_3d_add_aabb(ground, &s_vec3(0.0f, 0.0f, 0.0f), &ground_half, false);
	s_mat4 ground_transform_init = physics_make_transform(&ground->position, &ground_half);
	se_instance_id ground_instance = se_object_3d_add_instance(cubes, &ground_transform_init, &s_mat4_identity);

	physics_slots_3d slots = {0};
	s_array_init(&slots);
	for (i32 i = 0; i < BODY_COUNT; ++i) {
		se_physics_body_params_3d params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
		params.position = s_vec3(-3.0f + i * 0.9f, 0.8f + i * 0.5f, 0.0f);
		se_physics_body_3d* body = se_physics_body_3d_create(world, &params);
		s_vec3 half_extents = s_vec3(0.32f, 0.32f, 0.32f);
		se_physics_body_3d_add_box(body, &s_vec3(0.0f, 0.0f, 0.0f), &half_extents, &s_vec3(0.0f, 0.0f, 0.0f), false);
		s_mat4 body_transform = physics_make_transform(&body->position, &half_extents);
		se_instance_id instance = se_object_3d_add_instance(cubes, &body_transform, &s_mat4_identity);
		physics_slot_3d slot = { .body = body, .instance = instance, .half_extents = half_extents };
		s_array_add(&slots, slot);
	}

	se_camera_handle camera = se_scene_3d_get_camera(scene);
	se_camera_set_orbit_defaults(camera, window, &s_vec3(40.0f, 30.0f, .0f), 30.0f);
	printf("physics3d_playground controls:\n");
	printf("  Space: launch all cubes upward\n  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		if (se_window_is_key_pressed(window, SE_KEY_SPACE)) {
			for (sz i = 0; i < s_array_get_size(&slots); ++i) {
				physics_slot_3d* slot = s_array_get(&slots, s_array_handle(&slots, (u32)i));
				if (slot && slot->body) {
					se_physics_body_3d_apply_impulse(slot->body, &s_vec3(0.0f, 3.5f, 0.0f), &slot->body->position);
				}
			}
		}

		se_physics_world_3d_step(world, (f32)se_window_get_delta_time(window));
		s_mat4 ground_transform = physics_make_transform(&ground->position, &ground_half);
		se_object_3d_set_instance_transform(cubes, ground_instance, &ground_transform);
		for (sz i = 0; i < s_array_get_size(&slots); ++i) {
			physics_slot_3d* slot = s_array_get(&slots, s_array_handle(&slots, (u32)i));
			if (slot && slot->body) {
				s_mat4 transform = physics_make_transform(&slot->body->position, &slot->half_extents);
				se_object_3d_set_instance_transform(cubes, slot->instance, &transform);
			}
		}

		se_render_clear();
		se_scene_3d_draw(scene, window);
		se_window_end_frame(window);
	}

	s_array_clear(&slots);
	se_physics_world_3d_destroy(world);
	se_scene_3d_destroy_full(scene, true, true);
	se_context_destroy(context);
	return 0;
}
