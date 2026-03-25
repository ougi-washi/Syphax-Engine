// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_scene.h"
#include "se_window.h"

#include <math.h>
#include <stdio.h>

typedef struct {
	se_scene_3d_handle scene;
	se_camera_handle camera;
	s_vec3 camera_target;
	f32 camera_yaw;
	f32 camera_pitch;
	f32 camera_distance;
} civilization_world;

static void civilization_apply_camera(civilization_world* world, se_window_handle window) {
	if (!world || world->camera == S_HANDLE_NULL) {
		return;
	}
	const s_vec3 rotation = s_vec3(world->camera_pitch, world->camera_yaw, 0.0f);
	se_camera_set_rotation(world->camera, &rotation);
	const s_vec3 forward = se_camera_get_forward_vector(world->camera);
	const s_vec3 position = s_vec3_sub(&world->camera_target, &s_vec3_muls(&forward, world->camera_distance));
	se_camera_set_location(world->camera, &position);
	se_camera_set_target(world->camera, &world->camera_target);
	se_camera_set_window_aspect(world->camera, window);
}

static b8 civilization_init_world(civilization_world* world, se_window_handle window) {
	se_model_handle cube_model = S_HANDLE_NULL;
	se_object_3d_handle blocks = S_HANDLE_NULL;
	if (!world) {
		return false;
	}

	world->scene = se_scene_3d_create_for_window(window, 64);
	if (world->scene == S_HANDLE_NULL) {
		return false;
	}
	world->camera = se_scene_3d_get_camera(world->scene);
	world->camera_target = s_vec3(0.0f, 0.3f, 0.0f);
	world->camera_yaw = 0.65f;
	world->camera_pitch = 0.55f;
	world->camera_distance = 14.0f;
	se_camera_set_target_mode(world->camera, true);
	se_camera_set_perspective(world->camera, 52.0f, 0.05f, 200.0f);
	civilization_apply_camera(world, window);

	cube_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
	if (cube_model == S_HANDLE_NULL) {
		return false;
	}
	blocks = se_scene_3d_add_model(world->scene, cube_model, &s_mat4_identity);
	if (blocks == S_HANDLE_NULL) {
		return false;
	}

	for (i32 z = -4; z <= 4; ++z) {
		for (i32 x = -4; x <= 4; ++x) {
			s_mat4 tile = s_mat4_identity;
			tile = s_mat4_scale(&tile, &s_vec3(0.9f, 0.08f, 0.9f));
			s_mat4_set_translation(&tile, &s_vec3((f32)x * 1.1f, -0.55f, (f32)z * 1.1f));
			se_object_3d_add_instance(blocks, &tile, &s_mat4_identity);
		}
	}

	for (i32 i = 0; i < 6; ++i) {
		s_mat4 building = s_mat4_identity;
		const f32 height = 0.6f + (f32)(i % 3) * 0.45f;
		building = s_mat4_scale(&building, &s_vec3(0.55f, height, 0.55f));
		s_mat4_set_translation(&building, &s_vec3(-3.0f + (f32)i * 1.2f, -0.55f + height, -0.4f + (f32)(i % 2) * 1.4f));
		se_object_3d_add_instance(blocks, &building, &s_mat4_identity);
	}

	return true;
}

static void civilization_update_camera(civilization_world* world, se_window_handle window, const f32 dt) {
	s_vec2 mouse_delta = s_vec2(0.0f, 0.0f);
	s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
	s_vec3 move = s_vec3(0.0f, 0.0f, 0.0f);
	if (!world) {
		return;
	}

	se_window_get_mouse_delta(window, &mouse_delta);
	se_window_get_scroll(window, &scroll_delta);

	if (se_window_is_mouse_down(window, SE_MOUSE_LEFT)) {
		world->camera_yaw += mouse_delta.x * 0.008f;
		world->camera_pitch = s_max(-1.35f, s_min(1.35f, world->camera_pitch - mouse_delta.y * 0.008f));
	}
	if (fabsf(scroll_delta.y) > 0.0001f) {
		world->camera_distance = s_max(4.0f, s_min(40.0f, world->camera_distance - scroll_delta.y));
	}

	{
		const s_vec3 forward_xz = s_vec3(sinf(world->camera_yaw), 0.0f, -cosf(world->camera_yaw));
		const s_vec3 right_xz = s_vec3(cosf(world->camera_yaw), 0.0f, sinf(world->camera_yaw));
		const f32 move_speed = se_window_is_key_down(window, SE_KEY_LEFT_SHIFT) ? 8.0f : 4.0f;
		if (se_window_is_key_down(window, SE_KEY_W)) move = s_vec3_add(&move, &forward_xz);
		if (se_window_is_key_down(window, SE_KEY_S)) move = s_vec3_sub(&move, &forward_xz);
		if (se_window_is_key_down(window, SE_KEY_D)) move = s_vec3_add(&move, &right_xz);
		if (se_window_is_key_down(window, SE_KEY_A)) move = s_vec3_sub(&move, &right_xz);
		if (se_window_is_key_down(window, SE_KEY_E)) move.y += 1.0f;
		if (se_window_is_key_down(window, SE_KEY_Q)) move.y -= 1.0f;
		if (s_vec3_length(&move) > 0.0001f) {
			const s_vec3 move_dir = s_vec3_normalize(&move);
			move = s_vec3_muls(&move_dir, move_speed * s_max(dt, 0.0f));
			world->camera_target = s_vec3_add(&world->camera_target, &move);
		}
	}

	if (se_window_is_key_pressed(window, SE_KEY_R)) {
		world->camera_target = s_vec3(0.0f, 0.3f, 0.0f);
		world->camera_yaw = 0.65f;
		world->camera_pitch = 0.55f;
		world->camera_distance = 14.0f;
	}

	civilization_apply_camera(world, window);
}

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Civilization", 1280, 720);
	civilization_world world = {0};
	if (window == S_HANDLE_NULL) {
		printf("civilization :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.05f, 0.06f, 0.08f, 1.0f));

	if (!civilization_init_world(&world, window)) {
		printf("civilization :: world setup failed (%s)\n", se_result_str(se_get_last_error()));
		if (world.scene != S_HANDLE_NULL) {
			se_scene_3d_destroy_full(world.scene, true, true);
		}
		se_context_destroy(context);
		return 1;
	}

	printf("civilization controls:\n");
	printf("  Left Mouse: orbit camera\n");
	printf("  Mouse Wheel: zoom\n");
	printf("  WASD + QE: pan target\n");
	printf("  Shift: faster movement\n");
	printf("  R: reset camera\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		const f32 dt = (f32)se_window_get_delta_time(window);
		se_window_begin_frame(window);
		civilization_update_camera(&world, window, dt);
		se_render_clear();
		se_scene_3d_draw(world.scene, window);
		se_window_end_frame(window);
	}

	se_scene_3d_destroy_full(world.scene, true, true);
	se_context_destroy(context);
	return 0;
}
