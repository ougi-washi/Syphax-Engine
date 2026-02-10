// Syphax-Engine - Ougi Washi

#include "se_physics.h"
#include "se_render.h"
#include "se_scene.h"
#include "se_window.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BODY_COUNT 8

typedef struct {
	se_physics_body_2d *body;
	se_object_2d *object;
	f32 size;
	b8 is_circle;
} physics_2d_slot;

typedef struct {
	u32 hits;
} physics_2d_contact_state;

void physics_2d_on_contact(const se_physics_contact_2d *contact, void *user_data) {
	physics_2d_contact_state *state = (physics_2d_contact_state *)user_data;
	state->hits++;
	if (state->hits <= 5) {
		printf("physics_2d contact: normal=(%.2f, %.2f) penetration=%.4f\n", contact->normal.x, contact->normal.y, contact->penetration);
	}
}

int main(void) {
	se_render_handle *render_handle = se_render_handle_create(NULL);

	se_window *window = se_window_create(render_handle, "Syphax-Engine - Physics 2D", WINDOW_WIDTH, WINDOW_HEIGHT);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.05f, 0.06f, 0.08f, 1.0f));

	se_scene_handle *scene_handle = se_scene_handle_create(render_handle, NULL);

	se_scene_2d *scene = se_scene_2d_create(scene_handle, &s_vec2(WINDOW_WIDTH, WINDOW_HEIGHT), BODY_COUNT + 1);
	se_scene_2d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));

	se_physics_world_params_2d world_params = SE_PHYSICS_WORLD_PARAMS_2D_DEFAULTS;
	world_params.gravity = s_vec2(0.0f, -1.6f);
	physics_2d_contact_state contact_state = {0};
	world_params.on_contact = physics_2d_on_contact;
	world_params.user_data = &contact_state;
	se_physics_world_2d *world = se_physics_world_2d_create(&world_params);

	se_physics_body_params_2d floor_params = SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS;
	floor_params.type = SE_PHYSICS_BODY_STATIC;
	floor_params.position = s_vec2(0.0f, -0.9f);
	se_physics_body_2d *floor_body = se_physics_body_2d_create(world, &floor_params);
	se_physics_body_2d_add_aabb(floor_body, &s_vec2(0.0f, 0.0f), &s_vec2(0.95f, 0.06f), false);
	se_object_2d *floor_object = se_object_2d_create(scene_handle, SE_RESOURCE_EXAMPLE("ui/button.glsl"), &s_mat3_identity, 0);
	se_shader_set_vec3(floor_object->shader, "u_color", &s_vec3(0.2f, 0.25f, 0.32f));
	se_object_2d_set_position(floor_object, &floor_body->position);
	se_object_2d_set_scale(floor_object, &s_vec2(0.95f, 0.06f));
	se_scene_2d_add_object(scene, floor_object);

	physics_2d_slot slots[BODY_COUNT] = {0};
	for (u32 i = 0; i < BODY_COUNT; i++) {
		se_physics_body_params_2d body_params = SE_PHYSICS_BODY_PARAMS_2D_DEFAULTS;
		body_params.position = s_vec2(-0.7f + 0.2f * (f32)i, 0.6f + 0.12f * (f32)i);
		se_physics_body_2d *body = se_physics_body_2d_create(world, &body_params);
		b8 is_circle = (i % 2) == 0;
		f32 size = is_circle ? 0.06f : 0.07f;
		if (is_circle) {
			se_physics_body_2d_add_circle(body, &s_vec2(0.0f, 0.0f), size, false);
		} else {
			se_physics_body_2d_add_box(body, &s_vec2(0.0f, 0.0f), &s_vec2(size, size), 0.0f, false);
		}
		se_object_2d *object = se_object_2d_create(scene_handle, SE_RESOURCE_EXAMPLE("ui/button.glsl"), &s_mat3_identity, 0);
		se_shader_set_vec3(object->shader, "u_color", is_circle ? &s_vec3(0.45f, 0.75f, 0.9f) : &s_vec3(0.9f, 0.6f, 0.35f));
		se_object_2d_set_position(object, &body->position);
		se_object_2d_set_scale(object, &s_vec2(size, size));
		se_scene_2d_add_object(scene, object);
		slots[i].body = body;
		slots[i].object = object;
		slots[i].size = size;
		slots[i].is_circle = is_circle;
	}

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_handle_reload_changed_shaders(render_handle);

		const f32 dt = (f32)se_window_get_delta_time(window);
		se_physics_world_2d_step(world, dt);
		for (u32 i = 0; i < BODY_COUNT; i++) {
			se_object_2d_set_position(slots[i].object, &slots[i].body->position);
		}

		se_scene_2d_draw(scene, render_handle, window);
	}

	se_physics_world_2d_destroy(world);
	se_scene_handle_destroy(scene_handle);
	se_render_handle_destroy(render_handle);
	se_window_destroy(window);
	return 0;
}
