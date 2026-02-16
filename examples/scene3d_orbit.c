// Syphax-Engine - Ougi Washi

#include "se_camera.h"
#include "se_graphics.h"
#include "se_scene.h"

#include <math.h>
#include <stdio.h>

// What you will learn: load a 3D model and orbit camera with mouse input.
// Try this: add more cubes, tweak orbit speed, or change dolly limits.
int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Scene3D Orbit", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("scene3d_orbit :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.05f, 0.06f, 0.09f, 1.0f));

	se_scene_3d_handle scene = se_scene_3d_create_for_window(window, 64);
	se_model_handle cube_model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/cube.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl"));
	se_object_3d_handle cubes = se_scene_3d_add_model(scene, cube_model, &s_mat4_identity);

	for (i32 x = -1; x <= 1; ++x) {
		for (i32 z = -1; z <= 1; ++z) {
			s_mat4 transform = s_mat4_identity;
			s_mat4_set_translation(&transform, &s_vec3((f32)x * 2.0f, 0.0f, (f32)z * 2.0f));
			se_object_3d_add_instance(cubes, &transform, &s_mat4_identity);
		}
	}

	const s_vec3 pivot = s_vec3(0.0f, 0.0f, 0.0f);
	const se_camera_handle camera = se_scene_3d_get_camera(scene);
	se_camera_set_orbit_defaults(camera, window, &pivot, 10.0f);

	printf("scene3d_orbit controls:\n");
	printf("  Hold left mouse and move: orbit camera\n");
	printf("  Mouse wheel: zoom\n");
	printf("  R: reset camera\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_camera_set_aspect_from_window(camera, window);

		s_vec2 mouse_delta = s_vec2(0.0f, 0.0f);
		s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
		se_window_get_mouse_delta(window, &mouse_delta);
		se_window_get_scroll_delta(window, &scroll_delta);

		if (se_window_is_mouse_down(window, SE_MOUSE_LEFT)) {
			se_camera_orbit(camera, &pivot, mouse_delta.x * 0.01f, -mouse_delta.y * 0.01f, -1.45f, 1.45f);
		}
		if (fabsf(scroll_delta.y) > 0.0001f) {
			se_camera_dolly(camera, &pivot, -scroll_delta.y, 3.0f, 26.0f);
		}
		if (se_window_is_key_pressed(window, SE_KEY_R)) {
			se_camera_set_orbit_defaults(camera, window, &pivot, 10.0f);
		}

		se_render_clear();
		se_scene_3d_draw(scene, window);
		se_window_end_frame(window);
	}

	se_scene_3d_destroy_full(scene, true, true);
	se_context_destroy(context);
	return 0;
}
