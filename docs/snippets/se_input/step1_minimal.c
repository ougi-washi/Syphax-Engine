#include "se_input.h"

int main(void) {
	se_context* context = se_context_create();
	if (!context) {
		return 1;
	}

	se_window_handle window = se_window_create("se_input_minimal", 640, 360);
	if (window == S_HANDLE_NULL) {
		se_context_destroy(context);
		return 1;
	}

	se_input_handle* input = se_input_create(window, 16);
	if (!input) {
		se_context_destroy(context);
		return 1;
	}

	se_window_begin_frame(window);
	se_input_tick(input);
	se_window_end_frame(window);

	se_input_destroy(input);
	se_context_destroy(context);
	return 0;
}
