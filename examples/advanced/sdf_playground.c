// Syphax-Engine - Ougi Washi

#include "se.h"
#include "se_window.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_input.h"
#include "se_scene.h"
#include "se_sdf.h"
#include <math.h>
#include <stdio.h>

enum {
	INPUT_CONTEXT_CAMERA = 0,
	ACTION_CAMERA_YAW = 200,
	ACTION_CAMERA_ZOOM = 201
};

int main(void) {
	printf("advanced/sdf_playground :: Advanced example (reference)\n");
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("Syphax-Engine - 16_sdf", 1280, 720);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.03f, 0.04f, 0.05f, 1.0f));

	se_scene_3d_handle raster_scene = se_scene_3d_create_for_window(window, 16);
	if (raster_scene == S_HANDLE_NULL) {
		return 1;
	}
	se_model_handle cube_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
	if (cube_model != S_HANDLE_NULL) {
		se_object_3d_handle cube_object = se_scene_3d_add_model(raster_scene, cube_model, &s_mat4_identity);
		if (cube_object != S_HANDLE_NULL) {
			s_mat4 transform = s_mat4_identity;
			s_mat4_set_translation(&transform, &s_vec3(-1.9f, -0.1f, 0.2f));
			(void)se_object_3d_add_instance(cube_object, &transform, &s_mat4_identity);
			s_mat4_set_translation(&transform, &s_vec3(1.7f, -0.1f, -0.9f));
			(void)se_object_3d_add_instance(cube_object, &transform, &s_mat4_identity);
		}
	}

	se_sdf_scene_handle scene = se_sdf_scene_create(NULL);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &root_desc);
	se_sdf_scene_set_root(scene, root);

	se_sdf_node_primitive_desc sphere = {
		.transform = se_sdf_transform_trs(0.0f, 0.10f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
		.operation = SE_SDF_OP_UNION,
		.primitive = {
			.type = SE_SDF_PRIMITIVE_SPHERE,
			.sphere = {
				.radius = 0.90f
			}
		}
	};

	se_sdf_node_primitive_desc ground = {
		.transform = se_sdf_transform_trs(0.0f, -1.10f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
		.operation = SE_SDF_OP_UNION,
		.primitive = {
			.type = SE_SDF_PRIMITIVE_BOX,
			.box = {
				.size = {9.0f, 0.20f, 9.0f}
			}
		}
	};

	se_sdf_node_handle sphere_node = se_sdf_node_create_primitive(scene, &sphere);
	se_sdf_node_handle ground_node = se_sdf_node_create_primitive(scene, &ground);

	if (sphere_node == SE_SDF_NODE_NULL ||
		ground_node == SE_SDF_NODE_NULL ||
		!se_sdf_node_add_child(scene, root, sphere_node) ||
		!se_sdf_node_add_child(scene, root, ground_node)) {
		return 1;
	}

	se_sdf_renderer_handle renderer = se_sdf_renderer_create(NULL);
	if (renderer == SE_SDF_RENDERER_NULL || !se_sdf_renderer_set_scene(renderer, scene)) {
		return 1;
	}

	se_sdf_material_desc material = SE_SDF_MATERIAL_DESC_DEFAULTS;
	material.model = SE_SDF_SHADING_STYLIZED;
	material.base_color = s_vec3(0.70f, 0.72f, 0.75f);

	se_sdf_stylized_desc stylized = SE_SDF_STYLIZED_DESC_DEFAULTS;
	stylized.band_levels = 4.0f;
	stylized.rim_power = 2.2f;

	se_sdf_renderer_set_material(renderer, &material);
	se_sdf_renderer_set_stylized(renderer, &stylized);

	se_input_handle* input = se_input_create(window, 16);
	if (!input) {
		return 1;
	}
	se_input_context_create(input, INPUT_CONTEXT_CAMERA, "camera", true);
	se_input_context_push(input, INPUT_CONTEXT_CAMERA);
	se_input_action_create(input, ACTION_CAMERA_YAW, "camera_yaw", INPUT_CONTEXT_CAMERA);
	se_input_action_bind(input, ACTION_CAMERA_YAW, &(se_input_action_binding){
		.source_id = SE_INPUT_MOUSE_DELTA_X,
		.source_type = SE_INPUT_SOURCE_AXIS,
		.state = SE_INPUT_AXIS,
		.axis = (se_input_axis_options){
			.deadzone = 0.0f,
			.sensitivity = 0.007f,
			.exponent = 1.0f,
			.smoothing = 0.0f
		}
	});
	se_input_action_create(input, ACTION_CAMERA_ZOOM, "camera_zoom", INPUT_CONTEXT_CAMERA);
	se_input_action_bind(input, ACTION_CAMERA_ZOOM, &(se_input_action_binding){
		.source_id = SE_INPUT_MOUSE_SCROLL_Y,
		.source_type = SE_INPUT_SOURCE_AXIS,
		.state = SE_INPUT_AXIS,
		.axis = SE_INPUT_AXIS_OPTIONS_DEFAULTS
	});

	se_camera_handle camera = se_scene_3d_get_camera(raster_scene);
	if (camera == S_HANDLE_NULL) {
		return 1;
	}

	se_camera_set_aspect_from_window(camera, window);
	se_camera_set_perspective(camera, 52.0f, 0.05f, 200.0f);

	se_sdf_camera_align_desc align = SE_SDF_CAMERA_ALIGN_DESC_DEFAULTS;
	align.padding = 1.35f;
	align.min_distance = 3.0f;
	align.far_margin = 36.0f;

	se_sdf_scene_bounds scene_bounds = SE_SDF_SCENE_BOUNDS_DEFAULTS;
	se_sdf_scene_align_camera(scene, camera, &align, &scene_bounds);
	se_camera_set_target_mode(camera, true);
	s_vec3 camera_target = scene_bounds.valid ? scene_bounds.center : s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 camera_position = s_vec3(0.0f, 0.0f, 6.0f);
	(void)se_camera_get_location(camera, &camera_position);
	(void)se_camera_get_target(camera, &camera_target);
	s_vec3 forward = se_camera_get_forward_vector(camera);
	f32 camera_pitch = asinf(s_max(-1.0f, s_min(1.0f, forward.y)));
	f32 camera_yaw = atan2f(forward.x, -forward.z);
	f32 camera_distance = s_vec3_length(&s_vec3_sub(&camera_position, &camera_target));
	if (camera_distance < 0.5f) {
		camera_distance = 6.0f;
	}

	printf("sdf_playground controls:\n");
	printf("  hold left mouse: orbit around Y axis\n");
	printf("  mouse wheel: zoom\n");
	printf("depth demo:\n");
	printf("  rasterized cubes in the scene should occlude the SDF sphere where closer.\n");
	fflush(stdout);

	se_window_loop(window,
		se_input_tick(input);
		u32 w = 0;
		u32 h = 0;
		se_window_get_size(window, &w, &h);

		f32 t = (f32)se_window_get_time(window);
		s_vec2 res = s_vec2(w > 0 ? (f32)w : 1280.0f, h > 0 ? (f32)h : 720.0f);
		const f32 yaw_input = se_input_action_get_value(input, ACTION_CAMERA_YAW);
		if (se_window_is_mouse_down(window, SE_MOUSE_LEFT)) {
			camera_yaw += yaw_input;
		}
		const f32 zoom_input = se_input_action_get_value(input, ACTION_CAMERA_ZOOM);
		if (fabsf(zoom_input) > 0.0001f) {
			camera_distance = s_max(1.2f, s_min(80.0f, camera_distance - zoom_input * 0.8f));
		}

		const s_vec3 rotation = s_vec3(camera_pitch, camera_yaw, 0.0f);
		se_camera_set_rotation(camera, &rotation);
		forward = se_camera_get_forward_vector(camera);
		camera_position = s_vec3_sub(&camera_target, &s_vec3_muls(&forward, camera_distance));
		se_camera_set_location(camera, &camera_position);
		se_camera_set_target(camera, &camera_target);

		se_sdf_frame_desc frame = SE_SDF_FRAME_DESC_DEFAULTS;
		frame.resolution = res;
		frame.time_seconds = t;

		se_camera_set_aspect_from_window(camera, window);
		se_sdf_frame_set_camera(&frame, camera);
		u32 depth_texture = 0;
		if (se_scene_3d_get_output_depth_texture(raster_scene, &depth_texture) && depth_texture != 0u) {
			se_sdf_frame_set_scene_depth_texture(&frame, depth_texture);
		}
		se_window_get_mouse_position_normalized(window, &frame.mouse_normalized);

		se_render_clear();
		se_scene_3d_render_to_buffer(raster_scene);
		se_scene_3d_render_to_screen(raster_scene, window);
		se_sdf_renderer_render(renderer, &frame);
	);

	se_input_destroy(input);
	se_scene_3d_destroy_full(raster_scene, true, true);
	se_sdf_renderer_destroy(renderer);
	se_sdf_scene_destroy(scene);
	se_sdf_shutdown();
	se_context_destroy(context);
	return 0;
}
