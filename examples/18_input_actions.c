// Syphax-Engine - Ougi Washi

#include "se_input.h"
#include "se_debug.h"

#include <stdio.h>

typedef struct {
	u32 text_events;
} input_text_state;

static void input_text_on_text(const c8* utf8_text, void* user_data) {
	input_text_state* state = (input_text_state*)user_data;
	state->text_events++;
	printf("18_input_actions :: text input \"%s\"\n", utf8_text);
}

int main(void) {
	se_context* ctx = se_context_create();
	se_window_handle window = se_window_create("Syphax-Engine - Input Actions", 960, 540);
	if (window == S_HANDLE_NULL) {
		printf("18_input_actions :: window unavailable, skipping runtime\n");
		se_context_destroy(ctx);
		return 0;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background_color(s_vec4(0.07f, 0.08f, 0.10f, 1.0f));

	se_input_handle* input = se_input_create(window, 24);
	input_text_state text_state = {0};
	se_input_set_text_callback(input, input_text_on_text, &text_state);
	se_input_attach_window_text(input);

	se_input_context_create(input, 0, "gameplay", true);
	se_input_context_create(input, 1, "ui", false);
	se_input_context_push(input, 0);

	se_input_action_create(input, 10, "jump", 0);
	se_input_action_create(input, 20, "look_x", 0);
	se_input_action_bind(input, 10, &(se_input_action_binding){
		.source_id = SE_KEY_SPACE,
		.source_type = SE_INPUT_SOURCE_KEY,
		.state = SE_INPUT_PRESS
	});
	se_input_action_bind(input, 20, &(se_input_action_binding){
		.source_id = SE_INPUT_MOUSE_DELTA_X,
		.source_type = SE_INPUT_SOURCE_AXIS,
		.state = SE_INPUT_AXIS,
		.axis = {
			.deadzone = 0.0f,
			.sensitivity = 0.02f,
			.exponent = 1.0f,
			.smoothing = 0.45f
		}
	});

	se_input_record_start(input);
	for (i32 frame = 0; frame < 12; ++frame) {
		se_window_tick(window);
		se_render_clear();
		if (frame == 1) {
			se_window_inject_key_state(window, SE_KEY_SPACE, true);
		}
		if (frame == 2) {
			se_window_inject_key_state(window, SE_KEY_SPACE, false);
		}
		se_window_inject_mouse_position(window, 220.0f + frame * 16.0f, 190.0f);
		if (frame == 4) {
			se_window_emit_text(window, "x");
		}
		se_input_tick(input);
		if (se_input_action_is_pressed(input, 10)) {
			printf("18_input_actions :: jump pressed\n");
		}
		se_window_render_screen(window);
	}
	se_input_record_stop(input);

	se_input_replay_start(input, false);
	for (i32 frame = 0; frame < 10; ++frame) {
		se_window_tick(window);
		se_render_clear();
		se_input_tick(input);
		se_window_render_screen(window);
	}
	se_input_replay_stop(input);

	se_input_diagnostics diagnostics = {0};
	se_input_get_diagnostics(input, &diagnostics);
	printf(
		"18_input_actions :: actions=%u contexts=%u queue=%u recorded=%u text_events=%u\n",
		diagnostics.action_count,
		diagnostics.context_count,
		diagnostics.queue_size,
		diagnostics.recorded_count,
		text_state.text_events);

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_clear();
		se_input_tick(input);
		if (se_input_action_is_pressed(input, 10)) {
			printf("18_input_actions :: jump pressed\n");
		}
		se_window_render_screen(window);
	}

	se_input_destroy(input);
	se_context_destroy(ctx);
	return 0;
}
