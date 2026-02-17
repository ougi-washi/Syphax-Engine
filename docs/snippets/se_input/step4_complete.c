#include "se_input.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_input_complete", 640, 360);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_input_handle* input = se_input_create(window, 64);
	if (!input) {
		se_context_destroy(context);
		return 1;
	}

	const i32 gameplay_context = 1;
	const i32 editor_context = 2;
	const i32 action_move_forward = 10;
	const i32 action_toggle_editor = 11;

	if (!se_input_context_create(input, gameplay_context, "gameplay", 1)) {
		se_input_destroy(input);
		se_context_destroy(context);
		return 1;
	}
	if (!se_input_context_create(input, editor_context, "editor", 0)) {
		se_input_destroy(input);
		se_context_destroy(context);
		return 1;
	}
	if (!se_input_action_create(input, action_move_forward, "move_forward", gameplay_context)) {
		se_input_destroy(input);
		se_context_destroy(context);
		return 1;
	}
	if (!se_input_action_create(input, action_toggle_editor, "toggle_editor", gameplay_context)) {
		se_input_destroy(input);
		se_context_destroy(context);
		return 1;
	}

	se_input_action_binding move_forward_binding = {
		.source_id = SE_KEY_W,
		.source_type = SE_INPUT_SOURCE_KEY,
		.state = SE_INPUT_DOWN
	};
	if (!se_input_action_bind(input, action_move_forward, &move_forward_binding)) {
		se_input_destroy(input);
		se_context_destroy(context);
		return 1;
	}

	se_input_action_binding toggle_editor_binding = {
		.source_id = SE_KEY_TAB,
		.source_type = SE_INPUT_SOURCE_KEY,
		.state = SE_INPUT_PRESS
	};
	if (!se_input_action_bind(input, action_toggle_editor, &toggle_editor_binding)) {
		se_input_destroy(input);
		se_context_destroy(context);
		return 1;
	}

	se_window_begin_frame(window);
	se_input_tick(input);
	if (se_input_action_is_pressed(input, action_toggle_editor)) {
		const b8 editor_enabled = se_input_context_is_enabled(input, editor_context);
		se_input_context_set_enabled(input, editor_context, (b8)!editor_enabled);
	}
	f32 forward = se_input_action_get_value(input, action_move_forward);
	(void)forward;
	se_window_end_frame(window);

	se_input_destroy(input);
	se_context_destroy(context);
	return 0;
}
