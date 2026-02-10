// Syphax-Engine - Ougi Washi

#include "se_physics.h"
#include "se_render.h"
#include "se_scene.h"
#include "se_window.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BODY_COUNT 8

typedef struct {
	se_physics_body_3d *body;
	se_instance_id instance;
	s_vec3 half_extents;
} physics_3d_slot;

s_mat4 physics_3d_build_transform(const s_vec3 *position, const s_vec3 *half_extents) {
	s_vec3 scale = s_vec3(half_extents->x * 2.0f, half_extents->y * 2.0f, half_extents->z * 2.0f);
	s_mat4 transform = s_mat4_identity;
	transform = s_mat4_scale(&transform, &scale);
	s_mat4_set_translation(&transform, position);
	return transform;
}

int main(void) {
	se_render_handle *render_handle = se_render_handle_create(NULL);
	if (!render_handle) {
		return 1;
	}

	se_window *window = se_window_create(render_handle, "Syphax-Engine - Physics 3D", WINDOW_WIDTH, WINDOW_HEIGHT);
	if (!window) {
		se_render_handle_destroy(render_handle);
		return 1;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.04f, 0.05f, 0.07f, 1.0f));

	se_scene_handle *scene_handle = se_scene_handle_create(render_handle, NULL);
	if (!scene_handle) {
		se_window_destroy(window);
		se_render_handle_destroy(render_handle);
		return 1;
	}

	se_scene_3d *scene = se_scene_3d_create(scene_handle, &s_vec2(WINDOW_WIDTH, WINDOW_HEIGHT), BODY_COUNT + 1);
	if (!scene) {
		se_scene_handle_destroy(scene_handle);
		se_window_destroy(window);
		se_render_handle_destroy(render_handle);
		return 1;
	}
	se_scene_3d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));

	se_shader *mesh_shader = se_shader_load(render_handle, "shaders/scene_3d_vertex.glsl", "shaders/scene_3d_fragment.glsl");
	if (!mesh_shader) {
		se_scene_handle_destroy(scene_handle);
		se_window_destroy(window);
		se_render_handle_destroy(render_handle);
		return 1;
	}

	se_shaders_ptr shader_list = {0};
	s_array_init(&shader_list, 1);
	*s_array_increment(&shader_list) = mesh_shader;

	se_model *cube_model = se_model_load_obj(render_handle, "cube.obj", &shader_list);
	if (!cube_model) {
		s_array_clear(&shader_list);
		se_scene_handle_destroy(scene_handle);
		se_window_destroy(window);
		se_render_handle_destroy(render_handle);
		return 1;
	}

	se_object_3d *cubes = se_object_3d_create(scene_handle, cube_model, &s_mat4_identity, BODY_COUNT + 1);
	if (!cubes) {
		s_array_clear(&shader_list);
		se_scene_handle_destroy(scene_handle);
		se_window_destroy(window);
		se_render_handle_destroy(render_handle);
		return 1;
	}
	se_scene_3d_add_object(scene, cubes);

	se_physics_world_params_3d world_params = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS;
	world_params.gravity = s_vec3(0.0f, -2.8f, 0.0f);
	se_physics_world_3d *world = se_physics_world_3d_create(&world_params);
	if (!world) {
		s_array_clear(&shader_list);
		se_scene_handle_destroy(scene_handle);
		se_window_destroy(window);
		se_render_handle_destroy(render_handle);
		return 1;
	}

	se_physics_body_params_3d ground_params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	ground_params.type = SE_PHYSICS_BODY_STATIC;
	ground_params.position = s_vec3(0.0f, -3.0f, 0.0f);
	se_physics_body_3d *ground_body = se_physics_body_3d_create(world, &ground_params);
	s_vec3 ground_half = s_vec3(6.0f, 0.4f, 6.0f);
	se_instance_id ground_instance = -1;
	if (ground_body) {
		se_physics_body_3d_add_aabb(ground_body, &s_vec3(0.0f, 0.0f, 0.0f), &ground_half, false);
		s_mat4 ground_transform_init = physics_3d_build_transform(&ground_body->position, &ground_half);
		ground_instance = se_object_3d_add_instance(cubes, &ground_transform_init, &s_mat4_identity);
	}

	physics_3d_slot slots[BODY_COUNT] = {0};
	for (u32 i = 0; i < BODY_COUNT; i++) {
		se_physics_body_params_3d body_params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
		body_params.position = s_vec3(-3.0f + 1.0f * (f32)i, 1.0f + 0.6f * (f32)i, 0.0f);
		se_physics_body_3d *body = se_physics_body_3d_create(world, &body_params);
		if (!body) {
			continue;
		}
		b8 is_sphere = (i % 2) == 0;
		s_vec3 half_extents = is_sphere ? s_vec3(0.35f, 0.35f, 0.35f) : s_vec3(0.45f, 0.3f, 0.45f);
		if (is_sphere) {
			se_physics_body_3d_add_sphere(body, &s_vec3(0.0f, 0.0f, 0.0f), half_extents.x, false);
		} else {
			se_physics_body_3d_add_box(body, &s_vec3(0.0f, 0.0f, 0.0f), &half_extents, &s_vec3(0.0f, 0.0f, 0.0f), false);
		}
		s_mat4 transform = physics_3d_build_transform(&body->position, &half_extents);
		se_instance_id instance = se_object_3d_add_instance(cubes, &transform, &s_mat4_identity);
		slots[i].body = body;
		slots[i].instance = instance;
		slots[i].half_extents = half_extents;
	}

	if (scene->camera) {
		scene->camera->position = s_vec3(0.0f, 4.0f, 10.0f);
		scene->camera->target = s_vec3(0.0f, 0.0f, 0.0f);
		scene->camera->up = s_vec3(0.0f, 1.0f, 0.0f);
		scene->camera->near = 0.1f;
		scene->camera->far = 100.0f;
	}

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_handle_reload_changed_shaders(render_handle);

		const f32 dt = (f32)se_window_get_delta_time(window);
		se_physics_world_3d_step(world, dt);

		if (ground_body && ground_instance >= 0) {
			s_mat4 ground_transform = physics_3d_build_transform(&ground_body->position, &ground_half);
			se_object_3d_set_instance_transform(cubes, ground_instance, &ground_transform);
		}
		for (u32 i = 0; i < BODY_COUNT; i++) {
			if (!slots[i].body) {
				continue;
			}
			s_mat4 transform = physics_3d_build_transform(&slots[i].body->position, &slots[i].half_extents);
			se_object_3d_set_instance_transform(cubes, slots[i].instance, &transform);
		}

		se_scene_3d_draw(scene, render_handle, window);
	}

	se_physics_world_3d_destroy(world);
	s_array_clear(&shader_list);
	se_scene_handle_destroy(scene_handle);
	se_render_handle_destroy(render_handle);
	se_window_destroy(window);
	return 0;
}
