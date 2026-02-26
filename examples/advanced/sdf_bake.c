// Syphax-Engine - Ougi Washi

#include "loader/se_loader.h"
#include "se_camera.h"
#include "se_graphics.h"
#include "se_model.h"
#include "se_sdf.h"
#include "se_shader.h"
#include "se_texture.h"

#include <math.h>
#include <stdio.h>

static void sdf_bake_set_volume_shader_uniforms(
	se_context* context,
	const se_model_handle volume_model,
	const se_sdf_model_texture3d_result* baked,
	const s_vec3* camera_position
) {
	if (!context || volume_model == S_HANDLE_NULL || !baked || !camera_position) {
		return;
	}

	se_model* model_ptr = s_array_get(&context->models, volume_model);
	if (!model_ptr || s_array_get_size(&model_ptr->meshes) == 0) {
		return;
	}

	se_mesh* first_mesh = s_array_get(&model_ptr->meshes, s_array_handle(&model_ptr->meshes, 0u));
	const s_mat4 model_matrix = first_mesh ? first_mesh->matrix : s_mat4_identity;
	const s_mat4 model_inverse = s_mat4_inverse(&model_matrix);

	se_texture* volume_texture = s_array_get(&context->textures, baked->texture);
	const u32 volume_texture_id = volume_texture ? volume_texture->id : 0u;
	const s_vec3 volume_size = s_vec3_sub(&baked->bounds_max, &baked->bounds_min);
	const f32 max_extent = s_max(volume_size.x, s_max(volume_size.y, volume_size.z));
	const f32 surface_epsilon = s_max(0.0005f, max_extent / 512.0f);

	for (sz i = 0; i < s_array_get_size(&model_ptr->meshes); ++i) {
		const se_shader_handle shader = se_model_get_mesh_shader(volume_model, i);
		if (shader == S_HANDLE_NULL) {
			continue;
		}
		se_shader_set_texture(shader, "u_volume_texture", volume_texture_id);
		se_shader_set_vec3(shader, "u_volume_size", &volume_size);
		se_shader_set_vec3(shader, "u_voxel_size", &baked->voxel_size);
		se_shader_set_vec3(shader, "u_camera_position", camera_position);
		se_shader_set_mat4(shader, "u_model_inverse", &model_inverse);
		se_shader_set_float(shader, "u_surface_epsilon", surface_epsilon);
		se_shader_set_float(shader, "u_min_step", 0.0012f);
		se_shader_set_int(shader, "u_max_steps", 220);
	}
}

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - SDF Bake Volume", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("sdf_bake :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.03f, 0.04f, 0.06f, 1.0f));

	char gltf_path[SE_MAX_PATH_LENGTH] = {0};
	se_paths_resolve_resource_path(gltf_path, SE_MAX_PATH_LENGTH, SE_RESOURCE_EXAMPLE("gltf/triangle.gltf"));
	se_gltf_asset* asset = se_gltf_load(gltf_path, NULL);
	if (!asset) {
		printf("sdf_bake :: failed to load gltf '%s'\n", gltf_path);
		se_context_destroy(context);
		return 1;
	}

	const i32 gltf_mesh_index = 0;
	se_model_handle source_model = se_gltf_model_load_ex(asset, gltf_mesh_index, SE_MESH_DATA_CPU_GPU);
	if (source_model == S_HANDLE_NULL) {
		printf("sdf_bake :: failed to load gltf model (%s)\n", se_result_str(se_get_last_error()));
		se_gltf_free(asset);
		se_context_destroy(context);
		return 1;
	}

	se_sdf_model_texture3d_desc bake_desc = SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS;
	bake_desc.resolution_x = 96u;
	bake_desc.resolution_y = 96u;
	bake_desc.resolution_z = 96u;
	bake_desc.padding = 0.12f;
	bake_desc.max_distance = 0.0f;
	se_sdf_model_texture3d_result bake_result = {0};
	if (!se_sdf_bake_model_texture3d(source_model, &bake_desc, &bake_result)) {
		printf("sdf_bake :: bake failed (%s)\n", se_result_str(se_get_last_error()));
		se_gltf_free(asset);
		se_context_destroy(context);
		return 1;
	}

	printf("sdf_bake :: baked texture3d = %u x %u x %u\n", bake_desc.resolution_x, bake_desc.resolution_y, bake_desc.resolution_z);

	se_gltf_free(asset);

	se_model_handle volume_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("sdf_bake/volume_raymarch_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("sdf_bake/volume_raymarch_fragment.glsl"));
	if (volume_model == S_HANDLE_NULL) {
		printf("sdf_bake :: failed to load cube model\n");
		se_context_destroy(context);
		return 1;
	}

	const s_vec3 volume_size = s_vec3_sub(&bake_result.bounds_max, &bake_result.bounds_min);
	const s_vec3 volume_center = s_vec3(
		(bake_result.bounds_min.x + bake_result.bounds_max.x) * 0.5f,
		(bake_result.bounds_min.y + bake_result.bounds_max.y) * 0.5f,
		(bake_result.bounds_min.z + bake_result.bounds_max.z) * 0.5f);
	const s_vec3 volume_half = s_vec3(
		s_max(volume_size.x * 0.5f, 0.001f),
		s_max(volume_size.y * 0.5f, 0.001f),
		s_max(volume_size.z * 0.5f, 0.001f));
	const f32 max_extent = s_max(volume_size.x, s_max(volume_size.y, volume_size.z));
	const f32 stable_extent = s_max(max_extent, 0.35f);

	se_model_translate(volume_model, &volume_center);
	se_model_scale(volume_model, &volume_half);

	const se_camera_handle camera = se_camera_create();
	se_camera_set_target_mode(camera, true);
	se_camera_set_perspective(camera, 55.0f, 0.02f, 300.0f);
	se_camera_set_aspect_from_window(camera, window);
	s_vec3 camera_target = volume_center;
	se_camera_set_target(camera, &camera_target);

	f32 camera_yaw = 0.35f;
	f32 camera_pitch = 0.22f;
	f32 camera_distance = s_max(1.8f, stable_extent * 2.4f);

	printf("sdf_bake controls:\n");
	printf("  LMB drag: orbit\n");
	printf("  Wheel: zoom\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_camera_set_aspect_from_window(camera, window);

		s_vec2 mouse_delta = s_vec2(0.0f, 0.0f);
		s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
		se_window_get_mouse_delta(window, &mouse_delta);
		se_window_get_scroll_delta(window, &scroll_delta);

		if (se_window_is_mouse_down(window, SE_MOUSE_LEFT)) {
			camera_yaw += mouse_delta.x * 0.01f;
			camera_pitch = s_max(-1.40f, s_min(1.40f, camera_pitch - mouse_delta.y * 0.01f));
		}
		if (fabsf(scroll_delta.y) > 0.0001f) {
			const f32 min_distance = s_max(0.6f, stable_extent * 0.8f);
			const f32 max_distance = s_max(min_distance + 0.5f, stable_extent * 8.0f);
			camera_distance = s_max(min_distance, s_min(max_distance, camera_distance - scroll_delta.y * stable_extent * 0.16f));
		}

		const s_vec3 rotation = s_vec3(camera_pitch, camera_yaw, 0.0f);
		se_camera_set_rotation(camera, &rotation);
		const s_vec3 forward = se_camera_get_forward_vector(camera);
		const s_vec3 orbit_offset = s_vec3_muls(&forward, camera_distance);
		const s_vec3 camera_position = s_vec3_sub(&camera_target, &orbit_offset);
		se_camera_set_location(camera, &camera_position);
		se_camera_set_target(camera, &camera_target);

		sdf_bake_set_volume_shader_uniforms(context, volume_model, &bake_result, &camera_position);

		se_render_clear();
		se_model_render(volume_model, camera);
		se_window_end_frame(window);
	}

	se_context_destroy(context);
	return 0;
}
