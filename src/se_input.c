// Syphax-Engine - Ougi Washi

#include "se_input.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SE_INPUT_AXIS_THRESHOLD 0.0001f

static b8 se_input_is_key_id(const i32 id) {
	return id >= 0 && id < SE_KEY_COUNT;
}

static b8 se_input_is_mouse_button_id(const i32 id) {
	return id >= SE_INPUT_MOUSE_LEFT && id <= SE_INPUT_MOUSE_BUTTON_8;
}

static b8 se_input_is_axis_id(const i32 id) {
	return id >= SE_INPUT_MOUSE_DELTA_X && id <= SE_INPUT_MOUSE_SCROLL_Y;
}

static se_mouse_button se_input_id_to_mouse_button(const i32 id) {
	return (se_mouse_button)(id - SE_INPUT_MOUSE_LEFT);
}

static b8 se_input_binding_is_valid(const se_input_binding* binding) {
	if (!binding) {
		return false;
	}
	if (se_input_is_key_id(binding->id) || se_input_is_mouse_button_id(binding->id)) {
		return binding->state == SE_INPUT_DOWN || binding->state == SE_INPUT_PRESS || binding->state == SE_INPUT_RELEASE;
	}
	if (se_input_is_axis_id(binding->id)) {
		return binding->state == SE_INPUT_AXIS;
	}
	return false;
}

se_input_handle* se_input_create(se_window* window, const u16 bindings_capacity) {
	if (!window || bindings_capacity == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_input_handle* input_handle = calloc(1, sizeof(*input_handle));
	if (!input_handle) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	input_handle->window = window;
	s_array_init(&input_handle->bindings, bindings_capacity);
	input_handle->enabled = true;
	se_set_last_error(SE_RESULT_OK);
	return input_handle;
}

void se_input_destroy(se_input_handle* input_handle) {
	if (!input_handle) {
		return;
	}
	s_array_clear(&input_handle->bindings);
	free(input_handle);
}

b8 se_input_bind(se_input_handle* input_handle, const i32 id, const se_input_state state, se_input_callback callback, void* user_data) {
	if (!input_handle) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (input_handle->bindings.size >= input_handle->bindings.capacity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	se_input_binding binding = {0};
	binding.id = id;
	binding.state = state;
	binding.value = 0.0f;
	binding.callback = callback;
	binding.user_data = user_data;
	binding.is_valid = true;

	if (!se_input_binding_is_valid(&binding)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	*s_array_increment(&input_handle->bindings) = binding;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_input_unbind_all(se_input_handle* input_handle) {
	if (!input_handle) {
		return;
	}
	if (input_handle->bindings.data && input_handle->bindings.capacity > 0) {
		memset(input_handle->bindings.data, 0, sizeof(*input_handle->bindings.data) * input_handle->bindings.capacity);
	}
	input_handle->bindings.size = 0;
}

void se_input_set_enabled(se_input_handle* input_handle, const b8 enabled) {
	if (!input_handle) {
		return;
	}
	input_handle->enabled = enabled;
}

b8 se_input_is_enabled(const se_input_handle* input_handle) {
	if (!input_handle) {
		return false;
	}
	return input_handle->enabled;
}

static b8 se_input_binding_check_digital(se_window* window, se_input_binding* binding) {
	b8 fired = false;
	binding->value = 0.0f;

	if (se_input_is_key_id(binding->id)) {
		const se_key key = (se_key)binding->id;
		if (binding->state == SE_INPUT_DOWN) fired = se_window_is_key_down(window, key);
		if (binding->state == SE_INPUT_PRESS) fired = se_window_is_key_pressed(window, key);
		if (binding->state == SE_INPUT_RELEASE) fired = se_window_is_key_released(window, key);
	} else if (se_input_is_mouse_button_id(binding->id)) {
		const se_mouse_button button = se_input_id_to_mouse_button(binding->id);
		if (binding->state == SE_INPUT_DOWN) fired = se_window_is_mouse_down(window, button);
		if (binding->state == SE_INPUT_PRESS) fired = se_window_is_mouse_pressed(window, button);
		if (binding->state == SE_INPUT_RELEASE) fired = se_window_is_mouse_released(window, button);
	}

	if (fired) {
		binding->value = 1.0f;
	}
	return fired;
}

static b8 se_input_binding_check_axis(se_input_binding* binding, const s_vec2* mouse_delta, const s_vec2* scroll_delta) {
	binding->value = 0.0f;
	if (binding->id == SE_INPUT_MOUSE_DELTA_X) binding->value = mouse_delta->x;
	if (binding->id == SE_INPUT_MOUSE_DELTA_Y) binding->value = mouse_delta->y;
	if (binding->id == SE_INPUT_MOUSE_SCROLL_X) binding->value = scroll_delta->x;
	if (binding->id == SE_INPUT_MOUSE_SCROLL_Y) binding->value = scroll_delta->y;
	return fabsf(binding->value) > SE_INPUT_AXIS_THRESHOLD;
}

void se_input_tick(se_input_handle* input_handle) {
	if (!input_handle || !input_handle->window || !input_handle->enabled) {
		return;
	}

	s_vec2 mouse_delta = s_vec2(0.0f, 0.0f);
	s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
	se_window_get_mouse_delta(input_handle->window, &mouse_delta);
	se_window_get_scroll_delta(input_handle->window, &scroll_delta);

	for (sz i = 0; i < input_handle->bindings.size; i++) {
		se_input_binding* binding = &input_handle->bindings.data[i];
		if (!binding->is_valid) {
			continue;
		}

		b8 fired = false;
		if (binding->state == SE_INPUT_AXIS) {
			fired = se_input_binding_check_axis(binding, &mouse_delta, &scroll_delta);
		} else {
			fired = se_input_binding_check_digital(input_handle->window, binding);
		}

		if (fired && binding->callback) {
			binding->callback(binding->user_data);
		}
	}
}

f32 se_input_get_value(const se_input_handle* input_handle, const i32 id, const se_input_state state) {
	if (!input_handle) {
		return 0.0f;
	}

	f32 value = 0.0f;
	for (sz i = 0; i < input_handle->bindings.size; i++) {
		const se_input_binding* binding = &input_handle->bindings.data[i];
		if (!binding->is_valid) {
			continue;
		}
		if (binding->id == id && binding->state == state) {
			value += binding->value;
		}
	}
	return value;
}
