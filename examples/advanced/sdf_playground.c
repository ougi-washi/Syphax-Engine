// Syphax-Engine - Ougi Washi

#include "se.h"
#include "se_window.h"
#include "se_graphics.h"
#include "se_camera.h"
#include "se_scene.h"
#include "se_physics.h"
#include "se_sdf.h"

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - sdf_playground", 1280, 720);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.03f, 0.04f, 0.05f, 1.0f));

	se_scene_3d_handle camera_scene = se_scene_3d_create_for_window(window, 1);
	se_camera_handle camera = se_scene_3d_get_camera(camera_scene);
	se_camera_set_target_mode(camera, true);
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 52.0f, 0.05f, 200.0f);
	se_camera_set_location(camera, &s_vec3(6.0f, 4.5f, 6.0f));
	se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));

	const f32 ball_radius = 0.45f;
	const s_vec3 ground_size = s_vec3(8.0f, 0.20f, 8.0f);
	const s_vec3 ball_start = s_vec3(0.0f, 2.2f, 0.0f);
	s_mat4 ground_transform = s_mat4_identity;
	s_mat4_set_translation(&ground_transform, &s_vec3(0.0f, -1.0f, 0.0f));

	se_sdf_scene_handle sdf_scene = se_sdf_scene_create(NULL);
	se_sdf_node_group_desc root_desc = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	root_desc.operation = SE_SDF_OP_SMOOTH_UNION;
	root_desc.operation_amount = 0.5f;
	se_sdf_node_handle root = se_sdf_node_create_group(sdf_scene, &root_desc);
	se_sdf_scene_set_root(sdf_scene, root);

	se_sdf_node_primitive_desc ground_node_desc = {
		.transform = ground_transform,
		.operation = SE_SDF_OP_UNION,
		.primitive = {
			.type = SE_SDF_PRIMITIVE_BOX,
			.box = {
				.size = ground_size
			}
		}
	};
	se_sdf_node_primitive_desc ball_node_desc = {
		.transform = s_mat4_identity,
		.operation = SE_SDF_OP_UNION,
		.primitive = {
			.type = SE_SDF_PRIMITIVE_SPHERE,
			.sphere = {
				.radius = ball_radius
			}
		}
	};
	se_sdf_node_handle ground_node = se_sdf_node_create_primitive(sdf_scene, &ground_node_desc);
	se_sdf_node_handle ball_node = se_sdf_node_create_primitive(sdf_scene, &ball_node_desc);
	se_sdf_node_add_child(sdf_scene, root, ground_node);
	se_sdf_node_add_child(sdf_scene, root, ball_node);

	se_sdf_renderer_handle renderer = se_sdf_renderer_create(NULL);
	se_sdf_renderer_set_scene(renderer, sdf_scene);
	se_sdf_control_handle ball_translation_control = se_sdf_control_define_vec3(renderer, "ball_translation", &ball_start);
	se_sdf_control_bind_node_position(renderer, sdf_scene, ball_node, ball_translation_control);

	se_physics_world_params_3d world_params = SE_PHYSICS_WORLD_PARAMS_3D_DEFAULTS;
	world_params.gravity = s_vec3(0.0f, -9.81f, 0.0f);
	world_params.shapes_per_body = 24;
	se_physics_world_3d_handle physics_world = se_physics_world_3d_create(&world_params);

	const s_vec3 reference_position = s_vec3(6.0f, 4.5f, 6.0f);

	se_sdf_object ground_object = se_sdf_box(
		ground_transform,
		.box = {
			.size = ground_size
		}
	);
	se_physics_body_params_3d ground_body_params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	ground_body_params.type = SE_PHYSICS_BODY_STATIC;
	ground_body_params.position = s_vec3(0.0f, 0.0f, 0.0f);
	(void)se_sdf_object_create_physics_body_3d(
		physics_world,
		&ground_object,
		&ground_body_params,
		&reference_position,
		false
	);

	se_sdf_object ball_object = se_sdf_sphere(
		s_mat4_identity,
		.sphere = {
			.radius = ball_radius
		}
	);
	se_physics_body_params_3d ball_body_params = SE_PHYSICS_BODY_PARAMS_3D_DEFAULTS;
	ball_body_params.type = SE_PHYSICS_BODY_DYNAMIC;
	ball_body_params.position = ball_start;
	ball_body_params.mass = 1.0f;
	ball_body_params.friction = 0.6f;
	ball_body_params.restitution = 0.95f;
	se_physics_body_3d_handle ball_body = se_sdf_object_create_physics_body_3d(
		physics_world,
		&ball_object,
		&ball_body_params,
		&reference_position,
		false
	);

	se_window_loop(window,
		const f32 dt = (f32)se_window_get_delta_time(window);
		if (dt > 0.0f) {
			se_physics_world_3d_step(physics_world, dt);
		}

		const s_vec3 ball_position = se_physics_body_3d_get_position(physics_world, ball_body);
		se_sdf_control_set_vec3(renderer, ball_translation_control, &ball_position);

		u32 fb_w = 0;
		u32 fb_h = 0;
		se_window_get_framebuffer_size(window, &fb_w, &fb_h);
		const f32 aspect_w = fb_w > 0 ? (f32)fb_w : 1280.0f;
		const f32 aspect_h = fb_h > 0 ? (f32)fb_h : 720.0f;
		se_camera_set_aspect(camera, aspect_w, aspect_h);

		se_sdf_frame_desc frame = SE_SDF_FRAME_DESC_DEFAULTS;
		frame.resolution = s_vec2(aspect_w, aspect_h);
		frame.time_seconds = (f32)se_window_get_time(window);
		se_sdf_frame_set_camera(&frame, camera);

		se_render_clear();
		se_sdf_renderer_render(renderer, &frame);
	);

	se_physics_world_3d_destroy(physics_world);
	se_sdf_renderer_destroy(renderer);
	se_sdf_scene_destroy(sdf_scene);
	se_scene_3d_destroy(camera_scene);
	se_sdf_shutdown();
	se_context_destroy(context);
	return 0;
}
