// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_physics.h"
#include "se_scene.h"

#include <stdio.h>

// What you will learn: simulate 2D physics bodies and draw their motion.
// Try this: change gravity, body count, or the jump impulse amount.
#define BODY_COUNT 8

typedef struct {
	se_physics_body_2d* body;
	se_object_2d_handle object;
} physics_slot_2d;
typedef s_array(physics_slot_2d, physics_slots_2d);

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Physics2D Playground", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("physics2d_playground :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.05f, 0.06f, 0.08f, 1.0f));

	se_scene_2d_handle scene = se_scene_2d_create(&s_vec2(1280.0f, 720.0f), BODY_COUNT + 1);
	se_scene_2d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));

	se_physics_world_params_2d world_params = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS;
	world_params.gravity = s_vec2(0.0f, -2.0f);
	se_physics_world_2d* world = se_physics_world_2d_create(&world_params);

	se_physics_body_params_2d floor_params = SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS;
	floor_params.type = SE_PHYSICS_BODY_STATIC;
	floor_params.position = s_vec2(0.0f, -0.90f);
	se_physics_body_2d* floor = se_physics_body_2d_create(world, &floor_params);
	se_physics_body_2d_add_aabb(floor, &s_vec2(0.0f, 0.0f), &s_vec2(0.95f, 0.05f), false);
	se_object_2d_handle floor_object = se_object_2d_create(SE_RESOURCE_EXAMPLE("physics2d/box.glsl"), &s_mat3_identity, 0);
	se_shader_handle obj_shader = se_object_2d_get_shader(floor_object);
	se_shader_set_vec3(obj_shader, "u_color", &s_vec3(0.6f, 0.6f, .6f));
	se_object_2d_set_position(floor_object, &floor->position);
	se_object_2d_set_scale(floor_object, &s_vec2(0.95f, 0.05f));
	se_scene_2d_add_object(scene, floor_object);

	physics_slots_2d slots = {0};
	s_array_init(&slots);
	for (i32 i = 0; i < BODY_COUNT; ++i) {
		se_physics_body_params_2d body_params = SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS;
		body_params.position = s_vec2(-0.75f + i * 0.20f, 0.45f + i * 0.12f);
		se_physics_body_2d* body = se_physics_body_2d_create(world, &body_params);
		se_physics_body_2d_add_box(body, &s_vec2(0.0f, 0.0f), &s_vec2(0.06f, 0.06f), 0.0f, false);
		se_object_2d_handle object = se_object_2d_create(SE_RESOURCE_EXAMPLE("physics2d/box.glsl"), &s_mat3_identity, 0);
		se_shader_handle obj_shader = se_object_2d_get_shader(object);
		if (i % 2 == 0) {
			se_shader_set_vec3(obj_shader, "u_color", &s_vec3(1.0f, 0.0f, 0.0f));
		} else {
			se_shader_set_vec3(obj_shader, "u_color", &s_vec3(0.0f, 1.0f, 0.0f));
		}
		se_object_2d_set_position(object, &body->position);
		se_object_2d_set_scale(object, &s_vec2(0.06f, 0.06f));
		se_scene_2d_add_object(scene, object);
		physics_slot_2d slot = { .body = body, .object = object };
		s_array_add(&slots, slot);
	}

	printf("physics2d_playground controls:\n");
	printf("  Space: push all balls upward\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		if (se_window_is_key_pressed(window, SE_KEY_SPACE)) {
			for (sz i = 0; i < s_array_get_size(&slots); ++i) {
				physics_slot_2d* slot = s_array_get(&slots, s_array_handle(&slots, (u32)i));
				if (slot && slot->body) {
					se_physics_body_2d_apply_impulse(slot->body, &s_vec2(0.0f, 1.8f), &slot->body->position);
				}
			}
		}

		se_physics_world_2d_step(world, (f32)se_window_get_delta_time(window));
		for (sz i = 0; i < s_array_get_size(&slots); ++i) {
			physics_slot_2d* slot = s_array_get(&slots, s_array_handle(&slots, (u32)i));
			if (slot && slot->body) {
				se_object_2d_set_position(slot->object, &slot->body->position);
			}
		}

		se_render_clear();
		se_scene_2d_draw(scene, window);
		se_window_end_frame(window);
	}

	s_array_clear(&slots);
	se_physics_world_2d_destroy(world);
	se_scene_2d_destroy_full(scene, true);
	se_context_destroy(context);
	return 0;
}
