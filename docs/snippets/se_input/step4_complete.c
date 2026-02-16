#include "se_input.h"

int main(void) {
	se_input_handle* input = se_input_create(S_HANDLE_NULL, 48);
	se_input_tick(input);
	se_input_action_create(input, 1, "move_x", 0);
	f32 move = se_input_action_get_value(input, 1);
	(void)move;
	if (se_input_action_is_pressed(input, 1)) {
		se_input_set_enabled(input, 0);
	}
	se_input_set_enabled(input, 1);
	se_input_context_create(input, 2, "editor", 1);
	se_input_context_set_enabled(input, 2, 1);
	se_input_destroy(input);
	return 0;
}
