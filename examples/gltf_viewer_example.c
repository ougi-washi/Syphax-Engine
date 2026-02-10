// Syphax-Engine - Ougi Washi

#include "loader/se_loader.h"

#include <math.h>
#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main(void) {
	char gltf_path[SE_MAX_PATH_LENGTH] = {0};
	se_paths_resolve_resource_path(gltf_path, SE_MAX_PATH_LENGTH, SE_RESOURCE_EXAMPLE("gltf/Sponza/Sponza.gltf"));

	se_render_handle_params render_params = SE_RENDER_HANDLE_PARAMS_DEFAULTS;
	render_params.framebuffers_count = 4;
	render_params.render_buffers_count = 2;
	render_params.textures_count = 512;
	render_params.shaders_count = 16;
	render_params.models_count = 512;
	render_params.cameras_count = 2;

	se_render_handle *render_handle = se_render_handle_create(&render_params);

	se_window *window = se_window_create(render_handle, "Syphax-Engine - glTF Viewer", WINDOW_WIDTH, WINDOW_HEIGHT);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	se_scene_handle_params scene_params = {0};
	scene_params.objects_2d_count = 0;
	scene_params.objects_3d_count = 4096;
	scene_params.scenes_2d_count = 0;
	scene_params.scenes_3d_count = 1;

	se_scene_handle *scene_handle = se_scene_handle_create(render_handle, &scene_params);

	se_scene_3d *scene = se_scene_3d_create(scene_handle, &s_vec2(WINDOW_WIDTH, WINDOW_HEIGHT), scene_params.objects_3d_count);

	se_scene_3d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));
	se_scene_3d_set_culling(scene, false);

	se_shader *mesh_shader = se_shader_load(render_handle, SE_RESOURCE_PUBLIC("shaders/gltf_3d_vertex.glsl"), SE_RESOURCE_PUBLIC("shaders/gltf_3d_fragment.glsl"));

	se_texture *default_texture = se_texture_load(render_handle, SE_RESOURCE_EXAMPLE("gltf/Sponza/white.png"), SE_REPEAT);

	sz objects_added = 0;
	se_gltf_asset *asset = se_gltf_scene_load(render_handle, scene_handle, scene, gltf_path, NULL, mesh_shader, default_texture, SE_REPEAT, &objects_added);

	printf("gltf_viewer_example :: loaded gltf asset, meshes=%zu materials=%zu textures=%zu images=%zu objects=%zu\n",
		asset->meshes.size,
		asset->materials.size,
		asset->textures.size,
		asset->images.size,
		objects_added);
	printf("gltf_viewer_example :: added %zu objects\n", objects_added);
	se_gltf_scene_fit_camera(scene, asset);

	f32 base_speed = 600.0f;
	f32 fast_speed = 1400.0f;
	se_gltf_scene_get_navigation_speeds(asset, &base_speed, &fast_speed);

	f32 camera_yaw = 0.0f;
	f32 camera_pitch = 0.0f;
	s_vec3 forward = s_vec3_sub(&scene->camera->target, &scene->camera->position);
	forward = s_vec3_normalize(&forward);
	camera_yaw = atan2f(forward.x, forward.z);
	camera_pitch = asinf(forward.y);

	s_vec2 mouse_delta_clicked = {0};
	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_handle_reload_changed_shaders(render_handle);

		se_camera *camera = scene->camera;
		const f32 dt = (f32)se_window_get_delta_time(window);
		const f32 look_sensitivity = 15.0f;
		const b8 fast = se_window_is_key_down(window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(window, SE_KEY_RIGHT_SHIFT);
		const f32 move_speed = fast ? fast_speed : base_speed;

		if (se_window_is_mouse_down(window, SE_MOUSE_RIGHT)) {
			se_window_get_mouse_delta_normalized(window, &mouse_delta_clicked);
			camera_yaw += mouse_delta_clicked.x * look_sensitivity;
			camera_pitch -= mouse_delta_clicked.y * look_sensitivity;
			if (camera_pitch > 1.55f) camera_pitch = 1.55f;
			if (camera_pitch < -1.55f) camera_pitch = -1.55f;
		} else {
			mouse_delta_clicked = s_vec2(0.0f, 0.0f);
		}

		s_vec3 forward = s_vec3(
			sinf(camera_yaw) * cosf(camera_pitch),
			sinf(camera_pitch),
			cosf(camera_yaw) * cosf(camera_pitch));
		forward = s_vec3_normalize(&forward);
		s_vec3 right = s_vec3_cross(&forward, &camera->up);
		right = s_vec3_normalize(&right);
		s_vec3 up = s_vec3_normalize(&camera->up);

		s_vec3 move = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 forward_neg = s_vec3_muls(&forward, -1.0f);
		s_vec3 right_neg = s_vec3_muls(&right, -1.0f);
		s_vec3 up_neg = s_vec3_muls(&up, -1.0f);
		if (se_window_is_key_down(window, SE_KEY_W)) move = s_vec3_add(&move, &forward);
		if (se_window_is_key_down(window, SE_KEY_S)) move = s_vec3_add(&move, &forward_neg);
		if (se_window_is_key_down(window, SE_KEY_D)) move = s_vec3_add(&move, &right);
		if (se_window_is_key_down(window, SE_KEY_A)) move = s_vec3_add(&move, &right_neg);
		if (se_window_is_key_down(window, SE_KEY_E)) move = s_vec3_add(&move, &up);
		if (se_window_is_key_down(window, SE_KEY_Q)) move = s_vec3_add(&move, &up_neg);

		if (s_vec3_length(&move) > 0.0f) {
			move = s_vec3_normalize(&move);
			move = s_vec3_muls(&move, move_speed * dt);
			camera->position = s_vec3_add(&camera->position, &move);
		}
		camera->target = s_vec3_add(&camera->position, &forward);

		se_scene_3d_draw(scene, render_handle, window);
	}

	se_gltf_free(asset);
	se_scene_handle_destroy(scene_handle);
	se_render_handle_destroy(render_handle);
	se_window_destroy(window);
	return 0;
}
