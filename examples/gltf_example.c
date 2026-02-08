// Syphax-Engine - Ougi Washi

#include "se_gltf.h"
#include "se_scene.h"
#include "syphax/s_files.h"

#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main() {
	se_render_handle_params render_params = {0};
	render_params.framebuffers_count = 4;
	render_params.render_buffers_count = 2;
	render_params.textures_count = 8;
	render_params.shaders_count = 8;
	render_params.models_count = 8;
	render_params.cameras_count = 2;
	se_render_handle *render_handle = se_render_handle_create(&render_params);

	se_window *window = se_window_create(render_handle, "Syphax-Engine - glTF Example", WINDOW_WIDTH, WINDOW_HEIGHT);
	se_key_combo exit_keys = {0};
	s_array_init(&exit_keys, 1);
	s_array_add(&exit_keys, GLFW_KEY_ESCAPE);
	se_window_set_exit_keys(window, &exit_keys);

	se_scene_handle_params scene_params = {0};
	scene_params.objects_2d_count = 0;
	scene_params.objects_3d_count = 4;
	scene_params.scenes_2d_count = 0;
	scene_params.scenes_3d_count = 1;
	se_scene_handle *scene_handle = se_scene_handle_create(render_handle, &scene_params);

	se_scene_3d *scene = se_scene_3d_create(scene_handle, &s_vec2(WINDOW_WIDTH, WINDOW_HEIGHT), 4);
	se_scene_3d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));

	se_shader *mesh_shader = se_shader_load(render_handle, "shaders/scene_3d_vertex.glsl", "shaders/scene_3d_fragment.glsl");

	char gltf_path[SE_MAX_PATH_LENGTH] = {0};
	if (!s_path_join(gltf_path, SE_MAX_PATH_LENGTH, RESOURCES_DIR, "examples/gltf/triangle.gltf")) {
		printf("gltf_example :: failed to build gltf path\n");
		return 1;
	}

	se_gltf_load_params load_params = {0};
	load_params.load_buffers = true;
	load_params.load_images = true;
	load_params.decode_data_uris = true;
	se_gltf_asset *asset = se_gltf_load(gltf_path, &load_params);
	if (asset == NULL) {
		printf("gltf_example :: failed to load gltf asset\n");
		return 1;
	}
	printf("gltf_example :: loaded gltf asset\n");

	se_model *model = se_gltf_to_model(render_handle, asset, 0);
	if (model == NULL) {
		printf("gltf_example :: failed to convert gltf to model\n");
		se_gltf_free(asset);
		return 1;
	}
	printf("gltf_example :: model meshes=%zu\n", model->meshes.size);

	s_foreach(&model->meshes, i) {
		se_mesh *mesh = s_array_get(&model->meshes, i);
		mesh->shader = mesh_shader;
	}

	s_mat4 identity = s_mat4_identity;
	se_object_3d *object = se_object_3d_create(scene_handle, model, &identity, 1);
	se_object_3d_add_instance(object, &identity, &identity);
	se_scene_3d_add_object(scene, object);
	printf("gltf_example :: object created and instance added\n");
	printf("gltf_example :: mesh index_count=%u vertex_count=%u\n",
		model->meshes.data[0].index_count,
		model->meshes.data[0].vertex_count);

	if (scene->camera) {
		scene->camera->position = s_vec3(0.0f, 0.0f, 3.0f);
		scene->camera->target = s_vec3(0.0f, 0.0f, 0.0f);
		scene->camera->up = s_vec3(0.0f, 1.0f, 0.0f);
	}

	while (!se_window_should_close(window)) {
		se_window_poll_events();
		se_window_update(window);
		se_render_handle_reload_changed_shaders(render_handle);

		se_scene_3d_render(scene, render_handle);
		se_render_clear();
		se_scene_3d_render_to_screen(scene, render_handle, window);
		se_window_render_screen(window);
	}

	se_gltf_free(asset);
	se_scene_handle_cleanup(scene_handle);
	se_render_handle_cleanup(render_handle);
	se_window_destroy(window);
	return 0;
}
