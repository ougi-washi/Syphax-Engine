// Syphax-Engine - Ougi Washi

#include "se_gltf.h"
#include "se_scene.h"
#include "syphax/s_files.h"

#include <math.h>
#include <stdio.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main() {
	char gltf_path[SE_MAX_PATH_LENGTH] = {0};
	if (!s_path_join(gltf_path, SE_MAX_PATH_LENGTH, RESOURCES_DIR, "examples/gltf/Sponza/Sponza.gltf")) {
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
	printf("gltf_example :: loaded gltf asset, meshes=%zu materials=%zu textures=%zu images=%zu\n",
		asset->meshes.size,
		asset->materials.size,
		asset->textures.size,
		asset->images.size);

	sz total_primitives = 0;
	s_foreach(&asset->meshes, i) {
		se_gltf_mesh *mesh = s_array_get(&asset->meshes, i);
		total_primitives += mesh->primitives.size;
	}

	se_render_handle_params render_params = {0};
	render_params.framebuffers_count = 4;
	render_params.render_buffers_count = 2;
	render_params.textures_count = (u16)(asset->images.size + 8);
	render_params.shaders_count = (u16)(total_primitives + 8);
	render_params.models_count = (u16)(asset->meshes.size + 8);
	render_params.cameras_count = 2;
	se_render_handle *render_handle = se_render_handle_create(&render_params);

	se_window *window = se_window_create(render_handle, "Syphax-Engine - glTF Example", WINDOW_WIDTH, WINDOW_HEIGHT);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	se_scene_handle_params scene_params = {0};
	scene_params.objects_2d_count = 0;
	scene_params.objects_3d_count = (u16)s_array_get_size(&asset->meshes);
	scene_params.scenes_2d_count = 0;
	scene_params.scenes_3d_count = 1;
	se_scene_handle *scene_handle = se_scene_handle_create(render_handle, &scene_params);

	se_scene_3d *scene = se_scene_3d_create(scene_handle, &s_vec2(WINDOW_WIDTH, WINDOW_HEIGHT), scene_params.objects_3d_count);
	se_scene_3d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));
	se_scene_3d_set_culling(scene, false);

	se_shader *mesh_shader = se_shader_load(render_handle, "shaders/gltf_3d_vertex.glsl", "shaders/gltf_3d_fragment.glsl");
	if (!mesh_shader) {
		printf("gltf_example :: failed to load mesh shader\n");
		se_gltf_free(asset);
		return 1;
	}

	f32 bounds_min[3] = {0.0f, 0.0f, 0.0f};
	f32 bounds_max[3] = {0.0f, 0.0f, 0.0f};
	b8 has_bounds = false;
	s_vec3 bounds_center = s_vec3(0.0f, 0.0f, 0.0f);
	f32 bounds_radius = 1.0f;
	s_foreach(&asset->meshes, mesh_index) {
		se_gltf_mesh *gltf_mesh = s_array_get(&asset->meshes, mesh_index);
		s_foreach(&gltf_mesh->primitives, prim_index) {
			se_gltf_primitive *prim = s_array_get(&gltf_mesh->primitives, prim_index);
			se_gltf_attribute *pos_attr = NULL;
			s_foreach(&prim->attributes.attributes, attr_index) {
				se_gltf_attribute *attr = s_array_get(&prim->attributes.attributes, attr_index);
				if (strcmp(attr->name, "POSITION") == 0) {
					pos_attr = attr;
					break;
				}
			}
			if (!pos_attr) {
				continue;
			}
			if (pos_attr->accessor < 0 || (sz)pos_attr->accessor >= asset->accessors.size) {
				continue;
			}
			se_gltf_accessor *pos_acc = s_array_get(&asset->accessors, pos_attr->accessor);
			if (!pos_acc->has_min || !pos_acc->has_max) {
				continue;
			}
			if (!has_bounds) {
				bounds_min[0] = pos_acc->min_values[0];
				bounds_min[1] = pos_acc->min_values[1];
				bounds_min[2] = pos_acc->min_values[2];
				bounds_max[0] = pos_acc->max_values[0];
				bounds_max[1] = pos_acc->max_values[1];
				bounds_max[2] = pos_acc->max_values[2];
				has_bounds = true;
			} else {
				bounds_min[0] = (pos_acc->min_values[0] < bounds_min[0]) ? pos_acc->min_values[0] : bounds_min[0];
				bounds_min[1] = (pos_acc->min_values[1] < bounds_min[1]) ? pos_acc->min_values[1] : bounds_min[1];
				bounds_min[2] = (pos_acc->min_values[2] < bounds_min[2]) ? pos_acc->min_values[2] : bounds_min[2];
				bounds_max[0] = (pos_acc->max_values[0] > bounds_max[0]) ? pos_acc->max_values[0] : bounds_max[0];
				bounds_max[1] = (pos_acc->max_values[1] > bounds_max[1]) ? pos_acc->max_values[1] : bounds_max[1];
				bounds_max[2] = (pos_acc->max_values[2] > bounds_max[2]) ? pos_acc->max_values[2] : bounds_max[2];
			}
		}
	}
	if (has_bounds) {
		bounds_center.x = 0.5f * (bounds_min[0] + bounds_max[0]);
		bounds_center.y = 0.5f * (bounds_min[1] + bounds_max[1]);
		bounds_center.z = 0.5f * (bounds_min[2] + bounds_max[2]);
		f32 size_x = bounds_max[0] - bounds_min[0];
		f32 size_y = bounds_max[1] - bounds_min[1];
		f32 size_z = bounds_max[2] - bounds_min[2];
		bounds_radius = size_x;
		if (size_y > bounds_radius) bounds_radius = size_y;
		if (size_z > bounds_radius) bounds_radius = size_z;
		bounds_radius = bounds_radius > 1.0f ? bounds_radius : 1.0f;
	}

	se_texture *default_texture = se_texture_load(render_handle, "examples/gltf/Sponza/white.png", SE_REPEAT);
	if (!default_texture) {
		printf("gltf_example :: failed to load default texture\n");
		se_gltf_free(asset);
		return 1;
	}

	se_textures_ptr texture_cache = {0};
	const sz texture_count = s_array_get_size(&asset->textures);
	s_array_init(&texture_cache, texture_count);
	for (sz i = 0; i < texture_count; i++) {
		se_texture **slot = s_array_increment(&texture_cache);
		*slot = NULL;
	}

	s_mat4 identity = s_mat4_identity;
	s_mat4 model_transform = s_mat4_identity;
	if (has_bounds) {
		s_vec3 offset = s_vec3(-bounds_center.x, -bounds_center.y, -bounds_center.z);
		s_mat4_set_translation(&model_transform, &offset);
	}
	s_foreach(&asset->meshes, mesh_index) {
		se_model *model = se_gltf_to_model(render_handle, asset, (i32)mesh_index);
		if (!model) {
			printf("gltf_example :: failed to convert mesh %zu\n", mesh_index);
			continue;
		}
		se_gltf_mesh *gltf_mesh = s_array_get(&asset->meshes, mesh_index);
		for (sz prim_index = 0; prim_index < s_array_get_size(&model->meshes); prim_index++) {
			se_mesh *mesh = s_array_get(&model->meshes, prim_index);
			se_texture *mesh_texture = default_texture;
			if (prim_index < gltf_mesh->primitives.size) {
				se_gltf_primitive *prim = s_array_get(&gltf_mesh->primitives, prim_index);
				if (prim->has_material && prim->material >= 0 && (sz)prim->material < asset->materials.size) {
					se_gltf_material *material = s_array_get(&asset->materials, prim->material);
					if (material->has_pbr_metallic_roughness && material->pbr_metallic_roughness.has_base_color_texture) {
						i32 tex_index = material->pbr_metallic_roughness.base_color_texture.index;
						if (tex_index >= 0 && (sz)tex_index < texture_cache.size) {
							se_texture **cached = s_array_get(&texture_cache, tex_index);
							if (*cached == NULL) {
								*cached = se_gltf_texture_load(render_handle, asset, tex_index, SE_REPEAT);
							}
							if (*cached) {
								mesh_texture = *cached;
							}
						}
					}
				}
			}
			mesh->shader = mesh_shader;
			mesh->texture_id = mesh_texture->id;
		}

		se_object_3d *object = se_object_3d_create(scene_handle, model, &model_transform, 1);
		se_object_3d_add_instance(object, &identity, &identity);
		se_scene_3d_add_object(scene, object);
	}

	if (scene->camera) {
		s_vec3 target = s_vec3(0.0f, 0.0f, 0.0f);
		s_vec3 position = s_vec3(0.0f, 300.0f, 1500.0f);
		if (has_bounds) {
			target = s_vec3(0.0f, 0.0f, 0.0f);
			position = s_vec3(0.0f, bounds_radius * 0.35f, bounds_radius * 1.5f);
			scene->camera->far = bounds_radius * 10.0f;
		} else {
			scene->camera->far = 5000.0f;
		}
		scene->camera->position = position;
		scene->camera->target = target;
		scene->camera->up = s_vec3(0.0f, 1.0f, 0.0f);
		scene->camera->near = 0.1f;
	}

	f32 camera_yaw = 0.0f;
	f32 camera_pitch = 0.0f;
	if (scene->camera) {
		s_vec3 forward = s_vec3_sub(&scene->camera->target, &scene->camera->position);
		forward = s_vec3_normalize(&forward);
		camera_yaw = atan2f(forward.x, forward.z);
		camera_pitch = asinf(forward.y);
	}

	s_vec2 mouse_delta_clicked = {0};

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_handle_reload_changed_shaders(render_handle);

		se_camera* camera = scene->camera;
		const f32 dt = (f32)se_window_get_delta_time(window);
		const f32 base_speed = 600.0f;
		const f32 fast_speed = 1400.0f;
		const f32 look_sensitivity = 15.0f;
		const b8 fast = se_window_is_key_down(window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(window, SE_KEY_RIGHT_SHIFT);
		const f32 move_speed = fast ? fast_speed : base_speed;

		if (se_window_is_mouse_down(window, SE_MOUSE_RIGHT)) {
			se_window_get_mouse_delta_normalized(window, &mouse_delta_clicked);
			camera_yaw 		+= mouse_delta_clicked.x * look_sensitivity;
			camera_pitch 	-= mouse_delta_clicked.y * look_sensitivity;
			if (camera_pitch > 1.55f) camera_pitch = 1.55f;
			if (camera_pitch < -1.55f) camera_pitch = -1.55f;
		}
		else {
			mouse_delta_clicked = s_vec2(0.0f, 0.0f);
		}
		s_vec3 forward = s_vec3(
			sinf(camera_yaw) * cosf(camera_pitch),
			sinf(camera_pitch),
			cosf(camera_yaw) * cosf(camera_pitch));
		forward = s_vec3_normalize(&forward);
		s_vec3 right = s_vec3_cross(&forward, &camera->up);
		right = s_vec3_normalize(&right);
		s_vec3 up = camera->up;
		up = s_vec3_normalize(&up);

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
	s_array_clear(&texture_cache);
	se_scene_handle_cleanup(scene_handle);
	se_render_handle_destroy(render_handle);
	se_window_destroy(window);
	return 0;
}
