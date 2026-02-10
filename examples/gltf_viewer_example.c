// Syphax-Engine - Ougi Washi

#include "loader/se_loader.h"
#include "se_camera_controller.h"
#include "se_input.h"

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

	se_render_handle* render_handle = se_render_handle_create(&render_params);

	se_window* window = se_window_create(render_handle, "Syphax-Engine - glTF Viewer", WINDOW_WIDTH, WINDOW_HEIGHT);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	se_scene_handle_params scene_params = {0};
	scene_params.objects_2d_count = 0;
	scene_params.objects_3d_count = 4096;
	scene_params.scenes_2d_count = 0;
	scene_params.scenes_3d_count = 1;

	se_scene_handle* scene_handle = se_scene_handle_create(render_handle, &scene_params);
	
	se_scene_3d* scene = se_scene_3d_create(scene_handle, &s_vec2(WINDOW_WIDTH, WINDOW_HEIGHT), scene_params.objects_3d_count);

	se_scene_3d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));
	se_scene_3d_set_culling(scene, false);

	se_shader* mesh_shader = se_shader_load(render_handle, SE_RESOURCE_PUBLIC("shaders/gltf_3d_vertex.glsl"), SE_RESOURCE_PUBLIC("shaders/gltf_3d_fragment.glsl"));

	se_texture* default_texture = se_texture_load(render_handle, SE_RESOURCE_EXAMPLE("gltf/Sponza/white.png"), SE_REPEAT);

	sz objects_added = 0;
	se_gltf_asset* asset = se_gltf_scene_load(render_handle, scene_handle, scene, gltf_path, NULL, mesh_shader, default_texture, SE_REPEAT, &objects_added);

	printf("gltf_viewer_example :: loaded gltf asset, meshes=%zu materials=%zu textures=%zu images=%zu objects=%zu\n",
		asset->meshes.size,
		asset->materials.size,
		asset->textures.size,
		asset->images.size,
		objects_added);
	printf("gltf_viewer_example :: added %zu objects\n", objects_added);
	se_gltf_scene_fit_camera(scene, asset);

	se_input_params input_params = SE_INPUT_PARAMS_DEFAULTS;
	input_params.actions_count = 64;
	se_input* input = se_input_create(&input_params);

	se_camera_controller_params camera_controller_params = SE_CAMERA_CONTROLLER_PARAMS_DEFAULTS;
	camera_controller_params.input = input;
	camera_controller_params.window = window;
	camera_controller_params.camera = scene->camera;
	camera_controller_params.movement_speed = 20.f;
	camera_controller_params.mouse_x_speed = 0.01f;
	camera_controller_params.mouse_y_speed = 0.01f;
	camera_controller_params.preset = SE_CAMERA_CONTROLLER_PRESET_UE;
	se_camera_controller* camera_controller = se_camera_controller_create(&camera_controller_params);
	se_camera_controller_set_invert_y(camera_controller, true);

	s_vec3 scene_center = scene->camera->target;
	f32 scene_radius = 1.0f;
	if (!se_gltf_scene_compute_bounds(asset, &scene_center, &scene_radius)) {
		scene_radius = 1.0f;
	}
	if (scene_radius < 1.0f) {
		scene_radius = 1.0f;
	}
	se_camera_controller_set_scene_bounds(camera_controller, &scene_center, scene_radius);
	
	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_handle_reload_changed_shaders(render_handle);
		se_input_tick(input, window);
		se_camera_controller_tick(camera_controller, (f32)se_window_get_delta_time(window));
		se_scene_3d_draw(scene, render_handle, window);
	}

	se_camera_controller_destroy(camera_controller);
	se_input_destroy(input);
	se_gltf_free(asset);
	se_scene_handle_destroy(scene_handle);
	se_render_handle_destroy(render_handle);
	se_window_destroy(window);
	return 0;
}
