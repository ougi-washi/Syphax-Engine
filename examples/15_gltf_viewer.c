// Syphax-Engine - Ougi Washi

#include "loader/se_loader.h"
#include "se_camera_controller.h"

#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main(void) {
	char gltf_path[SE_MAX_PATH_LENGTH] = {0};
	se_paths_resolve_resource_path(gltf_path, SE_MAX_PATH_LENGTH, SE_RESOURCE_EXAMPLE("gltf/Sponza/Sponza.gltf"));

	se_context* ctx = se_context_create();

	se_window* window = se_window_create(ctx, "Syphax-Engine - glTF Viewer", WINDOW_WIDTH, WINDOW_HEIGHT);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	const u16 scene_objects_capacity = 4096;
	se_scene_3d* scene = se_scene_3d_create_for_window(ctx, window, scene_objects_capacity);
	se_scene_3d_set_culling(scene, false);

	se_shader* mesh_shader = se_shader_load(ctx, SE_RESOURCE_EXAMPLE("gltf_viewer/gltf_vertex.glsl"), SE_RESOURCE_EXAMPLE("gltf_viewer/gltf_fragment.glsl"));

	se_texture* default_texture = se_texture_load(ctx, SE_RESOURCE_EXAMPLE("gltf/Sponza/white.png"), SE_REPEAT);

	se_uniforms *global_uniforms = se_context_get_global_uniforms(ctx);
	const s_vec3 light_direction_0 = s_vec3(-0.25f, 0.90f, 0.34f);
	const s_vec3 light_direction_1 = s_vec3(0.74f, 0.48f, -0.31f);
	const s_vec3 light_direction_2 = s_vec3(-0.08f, -0.62f, -0.78f);
	const s_vec3 light_color_0 = s_vec3(8.5f, 8.0f, 7.3f);
	const s_vec3 light_color_1 = s_vec3(2.2f, 2.8f, 3.4f);
	const s_vec3 light_color_2 = s_vec3(0.55f, 0.65f, 0.78f);
	const s_vec3 ambient_sky = s_vec3(0.18f, 0.22f, 0.27f);
	const s_vec3 ambient_ground = s_vec3(0.02f, 0.017f, 0.014f);
	se_uniform_set_vec3(global_uniforms, "u_light_directions[0]", &light_direction_0);
	se_uniform_set_vec3(global_uniforms, "u_light_directions[1]", &light_direction_1);
	se_uniform_set_vec3(global_uniforms, "u_light_directions[2]", &light_direction_2);
	se_uniform_set_vec3(global_uniforms, "u_light_colors[0]", &light_color_0);
	se_uniform_set_vec3(global_uniforms, "u_light_colors[1]", &light_color_1);
	se_uniform_set_vec3(global_uniforms, "u_light_colors[2]", &light_color_2);
	se_uniform_set_vec3(global_uniforms, "u_ambient_sky", &ambient_sky);
	se_uniform_set_vec3(global_uniforms, "u_ambient_ground", &ambient_ground);
	se_uniform_set_float(global_uniforms, "u_ambient_strength", 1.1f);
	se_uniform_set_float(global_uniforms, "u_exposure", 1.08f);

	sz objects_added = 0;
	se_gltf_asset* asset = se_gltf_scene_load(ctx, scene, gltf_path, NULL, mesh_shader, default_texture, SE_REPEAT, &objects_added);

	printf("15_gltf_viewer :: loaded gltf asset, meshes=%zu materials=%zu textures=%zu images=%zu objects=%zu\n",
		asset->meshes.size,
		asset->materials.size,
		asset->textures.size,
		asset->images.size,
		objects_added);
	printf("15_gltf_viewer :: added %zu objects\n", objects_added);
	se_gltf_scene_fit_camera(scene, asset);

	se_camera_controller_params camera_controller_params = SE_CAMERA_CONTROLLER_PARAMS_DEFAULTS;
	camera_controller_params.window = window;
	camera_controller_params.camera = scene->camera;
	camera_controller_params.movement_speed = 10.;
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
		se_context_reload_changed_shaders(ctx);
		se_camera_controller_tick(camera_controller, (f32)se_window_get_delta_time(window));
		se_scene_3d_draw(scene, ctx, window);
	}

	se_camera_controller_destroy(camera_controller);
	se_gltf_free(asset);
	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
