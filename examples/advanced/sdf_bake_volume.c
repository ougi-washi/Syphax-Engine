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
#include <stdlib.h>
#include <string.h>

typedef enum {
	SDF_BAKE_SOURCE_CUBE,
	SDF_BAKE_SOURCE_SPHERE,
	SDF_BAKE_SOURCE_SPONZA
} sdf_bake_source;

static const c8* sdf_bake_source_name(const sdf_bake_source source) {
	switch (source) {
		case SDF_BAKE_SOURCE_CUBE: return "cube";
		case SDF_BAKE_SOURCE_SPHERE: return "sphere";
		case SDF_BAKE_SOURCE_SPONZA: return "sponza";
		default: return "cube";
	}
}

static b8 sdf_bake_parse_u32(const c8* text, u32* out_value) {
	if (!text || !out_value || text[0] == '\0') {
		return false;
	}
	char* end_ptr = NULL;
	const long parsed = strtol(text, &end_ptr, 10);
	if (end_ptr == text || *end_ptr != '\0' || parsed < 0 || parsed > 4096) {
		return false;
	}
	*out_value = (u32)parsed;
	return true;
}

static b8 sdf_bake_parse_f32(const c8* text, f32* out_value) {
	if (!text || !out_value || text[0] == '\0') {
		return false;
	}
	char* end_ptr = NULL;
	const double parsed = strtod(text, &end_ptr);
	if (end_ptr == text || *end_ptr != '\0') {
		return false;
	}
	*out_value = (f32)parsed;
	return true;
}

static b8 sdf_bake_parse_source(const c8* text, sdf_bake_source* out_source) {
	if (!text || !out_source) {
		return false;
	}
	if (strcmp(text, "cube") == 0) {
		*out_source = SDF_BAKE_SOURCE_CUBE;
		return true;
	}
	if (strcmp(text, "sphere") == 0) {
		*out_source = SDF_BAKE_SOURCE_SPHERE;
		return true;
	}
	if (strcmp(text, "sponza") == 0) {
		*out_source = SDF_BAKE_SOURCE_SPONZA;
		return true;
	}
	return false;
}

static void sdf_bake_print_usage(void) {
	printf("sdf_bake_volume usage:\n");
	printf("  ./bin/sdf_bake_volume [--source=cube|sphere|sponza] [--resolution=<n>] [--yaw-deg=<n>] [--pitch-deg=<n>]\n");
	printf("examples:\n");
	printf("  ./bin/sdf_bake_volume\n");
	printf("  ./bin/sdf_bake_volume --source=sphere\n");
	printf("  ./bin/sdf_bake_volume --source=sponza --resolution=20\n");
	printf("  ./bin/sdf_bake_volume --source=sphere --yaw-deg=45 --pitch-deg=20\n");
}

static b8 sdf_bake_load_source_model(
	const sdf_bake_source source,
	se_model_handle* out_model,
	se_gltf_asset** out_asset
) {
	if (!out_model || !out_asset) {
		return false;
	}
	*out_model = S_HANDLE_NULL;
	*out_asset = NULL;

	if (source == SDF_BAKE_SOURCE_CUBE) {
		*out_model = se_model_load_obj_ex(SE_RESOURCE_PUBLIC("models/cube.obj"), NULL, SE_MESH_DATA_CPU_GPU);
		return *out_model != S_HANDLE_NULL;
	}
	if (source == SDF_BAKE_SOURCE_SPHERE) {
		*out_model = se_model_load_obj_ex(SE_RESOURCE_PUBLIC("models/sphere.obj"), NULL, SE_MESH_DATA_CPU_GPU);
		return *out_model != S_HANDLE_NULL;
	}

	char gltf_path[SE_MAX_PATH_LENGTH] = {0};
	se_paths_resolve_resource_path(gltf_path, SE_MAX_PATH_LENGTH, SE_RESOURCE_EXAMPLE("gltf/Sponza/Sponza.gltf"));
	*out_asset = se_gltf_load(gltf_path, NULL);
	if (!*out_asset) {
		return false;
	}

	*out_model = se_gltf_model_load_ex(*out_asset, -1, SE_MESH_DATA_CPU_GPU);
	if (*out_model == S_HANDLE_NULL) {
		se_gltf_free(*out_asset);
		*out_asset = NULL;
		return false;
	}
	return true;
}

static void sdf_bake_set_volume_shader_uniforms(
	se_context* context,
	const se_window_handle window,
	const se_camera_handle camera,
	const se_model_handle volume_model,
	const se_sdf_model_texture3d_result* baked,
	const s_vec3* camera_position,
	const s_vec3* camera_target
) {
	if (!context || window == S_HANDLE_NULL || camera == S_HANDLE_NULL || volume_model == S_HANDLE_NULL || !baked || !camera_position || !camera_target) {
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
	se_camera* camera_ptr = se_camera_get(camera);
	const f32 fov_radians = ((camera_ptr ? camera_ptr->fov : 55.0f) * PI) / 180.0f;
	const f32 tan_half_fov = tanf(fov_radians * 0.5f);
	u32 framebuffer_width = 0u;
	u32 framebuffer_height = 0u;
	se_window_get_framebuffer_size(window, &framebuffer_width, &framebuffer_height);
	if (framebuffer_width == 0u || framebuffer_height == 0u) {
		se_window_get_size(window, &framebuffer_width, &framebuffer_height);
	}
	const s_vec2 viewport_size = s_vec2(
		(f32)s_max(1u, framebuffer_width),
		(f32)s_max(1u, framebuffer_height));
	const f32 camera_aspect = viewport_size.x / s_max(viewport_size.y, 1.0f);

	s_vec3 camera_forward = s_vec3_sub(camera_target, camera_position);
	if (s_vec3_length(&camera_forward) <= 0.00001f) {
		camera_forward = se_camera_get_forward_vector(camera);
	}
	camera_forward = s_vec3_normalize(&camera_forward);
	const s_vec3 world_up = s_vec3(0.0f, 1.0f, 0.0f);
	s_vec3 camera_right = s_vec3_cross(&camera_forward, &world_up);
	if (s_vec3_length(&camera_right) <= 0.00001f) {
		camera_right = se_camera_get_right_vector(camera);
	}
	camera_right = s_vec3_normalize(&camera_right);
	s_vec3 camera_up = s_vec3_cross(&camera_right, &camera_forward);
	if (s_vec3_length(&camera_up) <= 0.00001f) {
		camera_up = se_camera_get_up_vector(camera);
	}
	camera_up = s_vec3_normalize(&camera_up);

	for (sz i = 0; i < s_array_get_size(&model_ptr->meshes); ++i) {
		const se_shader_handle shader = se_model_get_mesh_shader(volume_model, i);
		if (shader == S_HANDLE_NULL) {
			continue;
		}
		se_shader_set_texture(shader, "u_volume_texture", volume_texture_id);
		se_shader_set_vec3(shader, "u_volume_size", &volume_size);
		se_shader_set_vec3(shader, "u_voxel_size", &baked->voxel_size);
		se_shader_set_vec3(shader, "u_camera_position", camera_position);
		se_shader_set_vec3(shader, "u_camera_forward", &camera_forward);
		se_shader_set_vec3(shader, "u_camera_right", &camera_right);
		se_shader_set_vec3(shader, "u_camera_up", &camera_up);
		se_shader_set_vec2(shader, "u_viewport_size", &viewport_size);
		se_shader_set_float(shader, "u_camera_tan_half_fov", tan_half_fov);
		se_shader_set_float(shader, "u_camera_aspect", camera_aspect);
		se_shader_set_mat4(shader, "u_model_inverse", &model_inverse);
		se_shader_set_float(shader, "u_surface_epsilon", surface_epsilon);
		se_shader_set_float(shader, "u_min_step", 0.0012f);
		se_shader_set_int(shader, "u_max_steps", 220);
	}
}

static void sdf_bake_fit_proxy_model_to_bounds(
	const se_model_handle volume_model,
	const se_sdf_model_texture3d_result* baked,
	s_vec3* out_center,
	f32* out_stable_extent
) {
	if (out_center) {
		*out_center = s_vec3(0.0f, 0.0f, 0.0f);
	}
	if (out_stable_extent) {
		*out_stable_extent = 1.0f;
	}
	if (volume_model == S_HANDLE_NULL || !baked) {
		return;
	}

	const s_vec3 volume_size = s_vec3_sub(&baked->bounds_max, &baked->bounds_min);
	const s_vec3 volume_half = s_vec3(
		s_max(volume_size.x * 0.5f, 0.0005f),
		s_max(volume_size.y * 0.5f, 0.0005f),
		s_max(volume_size.z * 0.5f, 0.0005f));
	const s_vec3 volume_center = s_vec3_muls(&s_vec3_add(&baked->bounds_min, &baked->bounds_max), 0.5f);

	/* Matrix composition in this engine is right-multiplied. Translate first, then scale => M = T*S. */
	se_model_translate(volume_model, &volume_center);
	se_model_scale(volume_model, &volume_half);

	if (out_center) {
		*out_center = volume_center;
	}
	if (out_stable_extent) {
		*out_stable_extent = s_max(volume_size.x, s_max(volume_size.y, volume_size.z));
	}
}

int main(int argc, char** argv) {
	sdf_bake_source source = SDF_BAKE_SOURCE_CUBE;
	u32 resolution_override = 0u;
	f32 camera_yaw_deg = 20.0f;
	f32 camera_pitch_deg = 12.0f;
	for (i32 i = 1; i < argc; ++i) {
		const c8* arg = argv[i];
		if (!arg) {
			continue;
		}
		if (strcmp(arg, "--help") == 0) {
			sdf_bake_print_usage();
			return 0;
		}
		if (strncmp(arg, "--source=", 9) == 0) {
			const c8* value = arg + 9;
			if (!sdf_bake_parse_source(value, &source)) {
				printf("sdf_bake_volume :: unknown source '%s'\n", value);
				sdf_bake_print_usage();
				return 1;
			}
			continue;
		}
		if (strncmp(arg, "--resolution=", 13) == 0) {
			const c8* value = arg + 13;
			if (!sdf_bake_parse_u32(value, &resolution_override) || resolution_override < 8u || resolution_override > 256u) {
				printf("sdf_bake_volume :: invalid resolution '%s' (expected 8..256)\n", value);
				return 1;
			}
			continue;
		}
		if (strncmp(arg, "--yaw-deg=", 10) == 0) {
			const c8* value = arg + 10;
			if (!sdf_bake_parse_f32(value, &camera_yaw_deg)) {
				printf("sdf_bake_volume :: invalid yaw '%s'\n", value);
				return 1;
			}
			continue;
		}
		if (strncmp(arg, "--pitch-deg=", 12) == 0) {
			const c8* value = arg + 12;
			if (!sdf_bake_parse_f32(value, &camera_pitch_deg)) {
				printf("sdf_bake_volume :: invalid pitch '%s'\n", value);
				return 1;
			}
			continue;
		}
		if (arg[0] != '-') {
			if (!sdf_bake_parse_source(arg, &source)) {
				printf("sdf_bake_volume :: unknown source '%s'\n", arg);
				sdf_bake_print_usage();
				return 1;
			}
			continue;
		}

		printf("sdf_bake_volume :: unknown argument '%s'\n", arg);
		sdf_bake_print_usage();
		return 1;
	}

	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - SDF Bake Volume", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("sdf_bake_volume :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.03f, 0.04f, 0.06f, 1.0f));

	se_gltf_asset* asset = NULL;
	se_model_handle source_model = S_HANDLE_NULL;
	if (!sdf_bake_load_source_model(source, &source_model, &asset)) {
		printf(
			"sdf_bake_volume :: failed to load source model (source=%s, error=%s)\n",
			sdf_bake_source_name(source),
			se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	printf("sdf_bake_volume :: source=%s\n", sdf_bake_source_name(source));

	se_sdf_model_texture3d_desc bake_desc = SE_SDF_MODEL_TEXTURE3D_DESC_DEFAULTS;
	u32 default_resolution = 96u;
	if (source == SDF_BAKE_SOURCE_SPHERE) {
		default_resolution = 96u;
	} else if (source == SDF_BAKE_SOURCE_SPONZA) {
		default_resolution = 20u;
	}
	const u32 bake_resolution = (resolution_override > 0u) ? resolution_override : default_resolution;
	bake_desc.resolution_x = bake_resolution;
	bake_desc.resolution_y = bake_resolution;
	bake_desc.resolution_z = bake_resolution;
	if (source == SDF_BAKE_SOURCE_SPHERE) {
		bake_desc.padding = 0.02f;
	} else if (source == SDF_BAKE_SOURCE_SPONZA) {
		bake_desc.padding = 0.03f;
	} else {
		bake_desc.padding = 0.08f;
	}
	bake_desc.max_distance = 0.0f;
	se_sdf_model_texture3d_result bake_result = {0};
	if (!se_sdf_bake_model_texture3d(source_model, &bake_desc, &bake_result)) {
		printf("sdf_bake_volume :: bake failed (%s)\n", se_result_str(se_get_last_error()));
		if (asset) {
			se_gltf_free(asset);
		}
		se_context_destroy(context);
		return 1;
	}

	printf(
		"sdf_bake_volume :: baked texture3d = %u x %u x %u\n",
		bake_desc.resolution_x,
		bake_desc.resolution_y,
		bake_desc.resolution_z);
	printf(
		"sdf_bake_volume :: bounds min=(%.3f, %.3f, %.3f) max=(%.3f, %.3f, %.3f) max-distance=%.3f\n",
		(double)bake_result.bounds_min.x,
		(double)bake_result.bounds_min.y,
		(double)bake_result.bounds_min.z,
		(double)bake_result.bounds_max.x,
		(double)bake_result.bounds_max.y,
		(double)bake_result.bounds_max.z,
		(double)bake_result.max_distance);

	if (asset) {
		se_gltf_free(asset);
	}

	se_model_handle volume_model = se_model_load_obj_simple(
		SE_RESOURCE_EXAMPLE("sdf_bake/volume_proxy_cube.obj"),
		SE_RESOURCE_EXAMPLE("sdf_bake/volume_raymarch_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("sdf_bake/volume_raymarch_fragment.glsl"));
	if (volume_model == S_HANDLE_NULL) {
		printf("sdf_bake_volume :: failed to load cube model\n");
		se_context_destroy(context);
		return 1;
	}

	s_vec3 camera_target = s_vec3(0.0f, 0.0f, 0.0f);
	f32 stable_extent = 2.0f;
	sdf_bake_fit_proxy_model_to_bounds(volume_model, &bake_result, &camera_target, &stable_extent);

	const se_camera_handle camera = se_camera_create();
	se_camera_set_target_mode(camera, true);
	const f32 near_plane = s_max(0.02f, stable_extent * 0.0005f);
	const f32 far_plane = s_max(120.0f, stable_extent * 16.0f);
	se_camera_set_perspective(camera, 55.0f, near_plane, far_plane);
	se_camera_set_aspect_from_window(camera, window);
	se_camera_set_target(camera, &camera_target);

	f32 camera_yaw = camera_yaw_deg * (PI / 180.0f);
	f32 camera_pitch = camera_pitch_deg * (PI / 180.0f);
	f32 camera_distance = s_max(1.2f, stable_extent * 1.4f);

	printf("sdf_bake_volume controls:\n");
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
			const f32 min_distance = s_max(0.6f, stable_extent * 0.2f);
			const f32 max_distance = s_max(14.0f, stable_extent * 8.0f);
			camera_distance = s_max(min_distance, s_min(max_distance, camera_distance - scroll_delta.y * 0.35f));
		}

		const f32 cos_pitch = cosf(camera_pitch);
		s_vec3 forward = s_vec3(
			cos_pitch * sinf(camera_yaw),
			sinf(camera_pitch),
			-cos_pitch * cosf(camera_yaw));
		forward = s_vec3_normalize(&forward);
		const s_vec3 orbit_offset = s_vec3_muls(&forward, camera_distance);
		const s_vec3 camera_position = s_vec3_sub(&camera_target, &orbit_offset);
		se_camera_set_location(camera, &camera_position);
		se_camera_set_target(camera, &camera_target);

		sdf_bake_set_volume_shader_uniforms(context, window, camera, volume_model, &bake_result, &camera_position, &camera_target);

		se_render_clear();
		se_model_render(volume_model, camera);
		se_window_end_frame(window);
	}

	se_context_destroy(context);
	return 0;
}
