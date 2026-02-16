// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_input.h"
#include "se_scene.h"
#include "se_shader.h"

#include <stdio.h>

// What you will learn: map actions to keys/mouse and move a 2D object.
// Try this: change move speed, color, or bind different action keys.
enum {
	INPUT_CONTEXT_GAME = 0,
	ACTION_FORWARD = 10,
	ACTION_BACKWARD = 11,
	ACTION_LEFT = 12,
	ACTION_RIGHT = 13,
	ACTION_LOOK_X = 14,
	ACTION_LOOK_Y = 15,
	ACTION_BOOST = 16,
	ACTION_COLOR = 17
};

int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Input Actions", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("input_actions :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.06f, 0.07f, 0.10f, 1.0f));

	se_scene_2d_handle scene = se_scene_2d_create(&s_vec2(1280.0f, 720.0f), 4);
	se_scene_2d_set_auto_resize(scene, window, &s_vec2(1.0f, 1.0f));
	se_object_2d_handle actor = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/button.glsl"), &s_mat3_identity, 0);
	se_object_2d_set_scale(actor, &s_vec2(0.10f, 0.10f));
	se_scene_2d_add_object(scene, actor);

	se_input_handle* input = se_input_create(window, 32);
	se_input_context_create(input, INPUT_CONTEXT_GAME, "game", true);
	se_input_context_push(input, INPUT_CONTEXT_GAME);
	se_input_bind_wasd_mouse_look(
		input,
		INPUT_CONTEXT_GAME,
		ACTION_FORWARD,
		ACTION_BACKWARD,
		ACTION_LEFT,
		ACTION_RIGHT,
		ACTION_LOOK_X,
		ACTION_LOOK_Y,
		0.01f);
	se_input_action_create(input, ACTION_BOOST, "boost", INPUT_CONTEXT_GAME);
	se_input_action_bind(input, ACTION_BOOST, &(se_input_action_binding){ .source_id = SE_KEY_LEFT_SHIFT, .source_type = SE_INPUT_SOURCE_KEY, .state = SE_INPUT_DOWN });
	se_input_action_create(input, ACTION_COLOR, "color", INPUT_CONTEXT_GAME);
	se_input_action_bind(input, ACTION_COLOR, &(se_input_action_binding){ .source_id = SE_MOUSE_LEFT, .source_type = SE_INPUT_SOURCE_MOUSE_BUTTON, .state = SE_INPUT_PRESS });

	b8 alt_color = false;
	printf("input_actions controls:\n");
	printf("  W A S D: move square\n");
	printf("  Left Shift: move faster\n");
	printf("  Left click: change square color\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_input_tick(input);

		s_vec2 position = se_object_2d_get_position(actor);
		const f32 dt = (f32)se_window_get_delta_time(window);
		const f32 speed = se_input_action_is_down(input, ACTION_BOOST) ? 1.2f : 0.7f;
		position.y += (se_input_action_get_value(input, ACTION_FORWARD) - se_input_action_get_value(input, ACTION_BACKWARD)) * speed * dt;
		position.x += (se_input_action_get_value(input, ACTION_RIGHT) - se_input_action_get_value(input, ACTION_LEFT)) * speed * dt;
		position.x = s_max(-0.92f, s_min(0.92f, position.x));
		position.y = s_max(-0.88f, s_min(0.88f, position.y));
		se_object_2d_set_position(actor, &position);

		if (se_input_action_is_pressed(input, ACTION_COLOR)) {
			alt_color = !alt_color;
			const se_shader_handle shader = se_object_2d_get_shader(actor);
			if (shader != S_HANDLE_NULL) {
				se_shader_set_vec3(shader, "u_color", alt_color ? &s_vec3(0.95f, 0.42f, 0.28f) : &s_vec3(0.28f, 0.76f, 0.92f));
			}
		}

		se_render_clear();
		se_scene_2d_draw(scene, window);
		se_window_end_frame(window);
	}

	se_input_destroy(input);
	se_scene_2d_destroy_full(scene, true);
	se_context_destroy(context);
	return 0;
}
