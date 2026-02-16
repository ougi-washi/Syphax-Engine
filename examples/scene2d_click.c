// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_scene.h"
#include "se_shader.h"
#include "se_window.h"

#include <stdio.h>

// What you will learn: pick a 2D object with the mouse and react to clicks.
// Try this: change object positions/colors or add a second clickable object.
int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Scene2D Click", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("scene2d_click :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.07f, 0.08f, 0.10f, 1.0f));

	se_scene_2d_handle scene = se_scene_2d_create(&s_vec2(1280.0f, 720.0f), 4);
	se_scene_2d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));

	se_object_2d_handle panel = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/panel.glsl"), &s_mat3_identity, 0);
	se_object_2d_set_scale(panel, &s_vec2(0.60f, 0.44f));
	se_scene_2d_add_object(scene, panel);

	se_object_2d_handle button = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/button.glsl"), &s_mat3_identity, 0);
	se_object_2d_set_position(button, &s_vec2(0.00f, -0.02f));
	se_object_2d_set_scale(button, &s_vec2(0.22f, 0.13f));
	se_scene_2d_add_object(scene, button);

	b8 active_color = false;
	u32 click_count = 0;
	printf("scene2d_click controls:\n");
	printf("  Left click on the center button: toggle color\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);

		if (se_window_is_mouse_pressed(window, SE_MOUSE_LEFT)) {
			s_vec2 pointer = s_vec2(0.0f, 0.0f);
			se_window_get_mouse_position_normalized(window, &pointer);
			se_object_2d_handle picked = S_HANDLE_NULL;
			if (se_scene_2d_pick_object(scene, &pointer, NULL, NULL, &picked) && picked == button) {
				active_color = !active_color;
				click_count++;
				const se_shader_handle shader = se_object_2d_get_shader(button);
				if (shader != S_HANDLE_NULL) {
					se_shader_set_vec3(shader, "u_color", active_color ? &s_vec3(0.30f, 0.92f, 0.56f) : &s_vec3(0.93f, 0.34f, 0.42f));
				}
				printf("scene2d_click :: button clicks = %u\n", click_count);
			}
		}

		se_render_clear();
		se_scene_2d_draw(scene, window);
		se_window_end_frame(window);
	}

	se_scene_2d_destroy_full(scene, true);
	se_context_destroy(context);
	return 0;
}
