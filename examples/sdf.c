// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_sdf.h"
#include "se_window.h"

i32 main() {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - SDF", 1280, 720);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.05f, 0.06f, 0.08f, 1.0f));

	se_camera_handle camera = se_camera_create();
	se_camera_set_window_aspect(camera, window);
	se_camera_set_perspective(camera, 50.0f, 0.05f, 100.0f);
	se_camera_set_location(camera, &s_vec3(0.0f, 5.0f, -10.0f));	
	se_camera_set_target(camera, &s_vec3(0.0f, 0.0f, 0.0f));

	se_sdf_handle scene = se_sdf_create(
		.operation = SE_SDF_SMOOTH_UNION,
		.operation_amount = 0.35f);

	se_sdf_handle sphere = se_sdf_create(
		.type = SE_SDF_SPHERE,
		.sphere = {.radius = 1.0f});

	se_sdf_handle ground = se_sdf_create(
		.type = SE_SDF_BOX,
		.box = {.size = {10.0f, 0.01f, 10.0f}});

	se_sdf_add_child(scene, ground);
	se_sdf_add_child(scene, sphere);

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_camera_set_window_aspect(camera, window);

		se_render_clear();
		se_sdf_render(scene, camera);

		se_window_end_frame(window);
	}

	se_context_destroy(context);
	return 0;
}
