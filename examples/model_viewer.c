// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_scene.h"

#include <stdio.h>

// What you will learn: load a 3D model, add a camera controller, and render it.
// Try this: switch model asset, camera speed, or add extra model instances.
int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Model Viewer", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("model_viewer :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.05f, 0.06f, 0.09f, 1.0f));

	se_scene_3d_handle scene = se_scene_3d_create_for_window(window, 16);
	se_model_handle model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/sphere.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
	se_object_3d_handle object = se_scene_3d_add_model(scene, model, &s_mat4_identity);

	for (i32 i = 0; i < 5; ++i) {
		s_mat4 transform = s_mat4_identity;
		s_mat4_set_translation(&transform, &s_vec3(-4.0f + i * 2.0f, 0.0f, -2.0f));
		se_object_3d_add_instance(object, &transform, &s_mat4_identity);
	}

	const se_camera_handle camera = se_scene_3d_get_camera(scene);
	se_camera_set_orbit_defaults(camera, window, &s_vec3(0.0f, 0.0f, 0.0f), 9.0f);
	se_camera_controller_params camera_params = SE_CAMERA_CONTROLLER_PARAMS_DEFAULTS;
	camera_params.window = window;
	camera_params.camera = camera;
	camera_params.preset = SE_CAMERA_CONTROLLER_PRESET_UE;
	camera_params.movement_speed = 8.0f;
	camera_params.mouse_x_speed = 0.004f;
	camera_params.mouse_y_speed = 0.004f;
	se_camera_controller* controller = se_camera_controller_create(&camera_params);

	printf("model_viewer controls:\n");
	printf("  Mouse + WASD: move camera\n");
	printf("  R: reset camera\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		if (se_window_is_key_pressed(window, SE_KEY_R)) {
			se_camera_set_orbit_defaults(camera, window, &s_vec3(0.0f, 0.0f, 0.0f), 9.0f);
		}
		if (controller) {
			se_camera_controller_tick(controller, (f32)se_window_get_delta_time(window));
		}
		se_render_clear();
		se_scene_3d_draw(scene, window);
		se_window_end_frame(window);
	}

	if (controller) {
		se_camera_controller_destroy(controller);
	}
	se_scene_3d_destroy_full(scene, true, true);
	se_context_destroy(context);
	return 0;
}
