// Syphax-Engine - Ougi Washi

#include "se.h"
#include "se_window.h"
#include "se_render.h"
#include "se_camera.h"
#include "se_sdf.h"

int main(void) {
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

	se_camera_handle camera = se_camera_create();
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

	se_camera_controller_params cam_ctrl_params = SE_CAMERA_CONTROLLER_PARAMS_DEFAULTS;
	cam_ctrl_params.window = window;
	cam_ctrl_params.camera = camera;
	cam_ctrl_params.preset = SE_CAMERA_CONTROLLER_PRESET_UE;
	cam_ctrl_params.movement_speed = 8.0f;
	cam_ctrl_params.mouse_x_speed = 0.0045f;
	cam_ctrl_params.mouse_y_speed = 0.0045f;

	se_camera_controller* camera_controller = se_camera_controller_create(&cam_ctrl_params);
	if (camera_controller && scene_bounds.valid) {
		const f32 radius = scene_bounds.radius > 1.0f ? scene_bounds.radius : 1.0f;
		se_camera_controller_set_scene_bounds(camera_controller, &scene_bounds.center, radius);
	}

	se_window_loop(window,
		u32 w = 0;
		u32 h = 0;
		se_window_get_size(window, &w, &h);

		f32 t = (f32)se_window_get_time(window);
		s_vec2 res = s_vec2(w > 0 ? (f32)w : 1280.0f, h > 0 ? (f32)h : 720.0f);

		if (camera_controller) {
			se_camera_controller_tick(camera_controller, (f32)se_window_get_delta_time(window));
		}

		se_sdf_frame_desc frame = SE_SDF_FRAME_DESC_DEFAULTS;
		frame.resolution = res;
		frame.time_seconds = t;

		se_camera_set_aspect_from_window(camera, window);
		se_sdf_frame_set_camera(&frame, camera);
		se_window_get_mouse_position_normalized(window, &frame.mouse_normalized);

		se_render_clear();
		se_sdf_renderer_render(renderer, &frame);
	);

	if (camera_controller) {
		se_camera_controller_destroy(camera_controller);
	}

	se_camera_destroy(camera);
	se_sdf_renderer_destroy(renderer);
	se_sdf_scene_destroy(scene);
	se_sdf_shutdown();
	se_context_destroy(context);
	return 0;
}
