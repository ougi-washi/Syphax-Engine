#include "se_input.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_input_core", 640, 360);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_input_handle* input = se_input_create(window, 32);
	if (!input) {
		se_context_destroy(context);
		return 1;
	}

	const i32 action_move_x = 1;
	if (!se_input_action_create(input, action_move_x, "move_x", 0)) {
		se_input_destroy(input);
		se_context_destroy(context);
		return 1;
	}

	se_input_action_binding move_x_binding = {
		.source_id = SE_INPUT_MOUSE_DELTA_X,
		.source_type = SE_INPUT_SOURCE_AXIS,
		.state = SE_INPUT_AXIS,
		.axis = SE_INPUT_AXIS_OPTIONS_DEFAULTS
	};
	if (!se_input_action_bind(input, action_move_x, &move_x_binding)) {
		se_input_destroy(input);
		se_context_destroy(context);
		return 1;
	}

	se_window_begin_frame(window);
	se_input_tick(input);
	f32 move = se_input_action_get_value(input, action_move_x);
	se_window_end_frame(window);

	(void)move;
	se_input_destroy(input);
	se_context_destroy(context);
	return 0;
}
