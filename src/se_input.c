// Syphax-Engine - Ougi Washi

#include "se_input.h"
#include "se_debug.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef s_array(se_input_action, se_input_actions);
typedef s_array(se_input_context, se_input_contexts);
typedef s_array(se_input_event_record, se_input_event_records);
typedef s_array(i32, se_input_context_stack);

struct se_input_runtime {
	se_input_actions actions;
	se_input_contexts contexts;
	se_input_context_stack context_stack;
	se_input_event_records queue;
	se_input_event_records recorded;
	f64 record_start_time;
	f64 replay_time;
	sz replay_index;
	b8 recording_enabled : 1;
	b8 replay_enabled : 1;
	b8 replay_loop : 1;
};

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

static se_input_action* se_input_find_action(se_input_handle* input_handle, const i32 action_id) {
	if (!input_handle || !input_handle->runtime) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(&input_handle->runtime->actions); ++i) {
		se_input_action* action = s_array_get(&input_handle->runtime->actions, s_array_handle(&input_handle->runtime->actions, (u32)i));
		if (action && action->valid && action->action_id == action_id) {
			return action;
		}
	}
	return NULL;
}

static const se_input_action* se_input_find_action_const(const se_input_handle* input_handle, const i32 action_id) {
	if (!input_handle || !input_handle->runtime) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(&input_handle->runtime->actions); ++i) {
		const se_input_action* action = s_array_get(&input_handle->runtime->actions, s_array_handle(&input_handle->runtime->actions, (u32)i));
		if (action && action->valid && action->action_id == action_id) {
			return action;
		}
	}
	return NULL;
}

static se_input_context* se_input_find_context(se_input_handle* input_handle, const i32 context_id) {
	if (!input_handle || !input_handle->runtime) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(&input_handle->runtime->contexts); ++i) {
		se_input_context* context = s_array_get(&input_handle->runtime->contexts, s_array_handle(&input_handle->runtime->contexts, (u32)i));
		if (context && context->context_id == context_id) {
			return context;
		}
	}
	return NULL;
}

static b8 se_input_context_enabled(const se_input_handle* input_handle, const i32 context_id) {
	if (!input_handle || !input_handle->runtime) {
		return false;
	}
	if (context_id < 0) {
		return true;
	}
	b8 context_enabled = false;
	for (sz i = 0; i < s_array_get_size(&input_handle->runtime->contexts); ++i) {
		const se_input_context* context = s_array_get(&input_handle->runtime->contexts, s_array_handle(&input_handle->runtime->contexts, (u32)i));
		if (context && context->context_id == context_id) {
			context_enabled = context->enabled;
			break;
		}
	}
	if (!context_enabled) {
		return false;
	}
	if (s_array_get_size(&input_handle->runtime->context_stack) == 0) {
		return true;
	}
	i32* top_context = s_array_get(
		&input_handle->runtime->context_stack,
		s_array_handle(&input_handle->runtime->context_stack, (u32)(s_array_get_size(&input_handle->runtime->context_stack) - 1)));
	if (!top_context || *top_context < 0) {
		return true;
	}
	return *top_context == context_id;
}

static f32 se_input_apply_axis_options(const f32 raw_value, const se_input_axis_options* options, const f32 previous_value) {
	if (!options) {
		return raw_value;
	}
	f32 value = raw_value;
	const f32 sign = value < 0.0f ? -1.0f : 1.0f;
	value = fabsf(value);
	if (value < s_max(options->deadzone, 0.0f)) {
		value = 0.0f;
	}
	if (value > 0.0f) {
		const f32 exponent = s_max(options->exponent, 0.0001f);
		value = powf(value, exponent);
	}
	value *= s_max(options->sensitivity, 0.0f);
	value *= sign;
	const f32 smoothing = s_max(options->smoothing, 0.0f);
	if (smoothing > 0.0f) {
		value = previous_value + (value - previous_value) * s_min(smoothing, 1.0f);
	}
	return value;
}

static b8 se_input_action_chord_ok(const se_input_handle* input_handle, const se_input_action_binding* binding) {
	if (!input_handle || !binding) {
		return false;
	}
	for (u8 i = 0; i < binding->chord_count; ++i) {
		const i32 key_id = binding->chord_keys[i];
		if (!se_input_is_key_id(key_id)) {
			return false;
		}
		if (!se_window_is_key_down(input_handle->window, (se_key)key_id)) {
			return false;
		}
	}
	return true;
}

static void se_input_runtime_push_event(se_input_handle* input_handle, const se_input_action* action) {
	if (!input_handle || !input_handle->runtime || !action) {
		return;
	}
	const f64 now = se_window_get_time(input_handle->window);
	se_input_event_record queue_record = {0};
	queue_record.timestamp = now;
	queue_record.action_id = action->action_id;
	queue_record.value = action->value;
	queue_record.down = action->down;
	queue_record.pressed = action->pressed;
	queue_record.released = action->released;
	s_array_add(&input_handle->runtime->queue, queue_record);
	if (input_handle->runtime->recording_enabled) {
		se_input_event_record recorded = queue_record;
		recorded.timestamp = s_max(0.0, now - input_handle->runtime->record_start_time);
		s_array_add(&input_handle->runtime->recorded, recorded);
	}
}

static void se_input_window_text_bridge(const se_window_handle window, const c8* utf8_text, void* user_data) {
	(void)window;
	se_input_handle* input_handle = (se_input_handle*)user_data;
	se_input_emit_text(input_handle, utf8_text);
}

static b8 se_input_binding_check_digital(const se_window_handle window, se_input_binding* binding) {
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

static f32 se_input_read_axis(const i32 id, const s_vec2* mouse_delta, const s_vec2* scroll_delta) {
	if (id == SE_INPUT_MOUSE_DELTA_X) return mouse_delta->x;
	if (id == SE_INPUT_MOUSE_DELTA_Y) return mouse_delta->y;
	if (id == SE_INPUT_MOUSE_SCROLL_X) return scroll_delta->x;
	if (id == SE_INPUT_MOUSE_SCROLL_Y) return scroll_delta->y;
	return 0.0f;
}

static void se_input_tick_actions(se_input_handle* input_handle, const s_vec2* mouse_delta, const s_vec2* scroll_delta) {
	if (!input_handle || !input_handle->runtime) {
		return;
	}

	for (sz i = 0; i < s_array_get_size(&input_handle->runtime->actions); ++i) {
		se_input_action* action = s_array_get(&input_handle->runtime->actions, s_array_handle(&input_handle->runtime->actions, (u32)i));
		if (!action || !action->valid) {
			continue;
		}

		action->previous_value = action->value;
		action->pressed = false;
		action->released = false;

		if (!se_input_context_enabled(input_handle, action->context_id)) {
			action->value = 0.0f;
			action->down = false;
			continue;
		}

		const se_input_action_binding* binding = &action->binding;
		f32 value = 0.0f;
		b8 active = false;
		if (!se_input_action_chord_ok(input_handle, binding)) {
			active = false;
			value = 0.0f;
		} else if (binding->source_type == SE_INPUT_SOURCE_KEY && se_input_is_key_id(binding->source_id)) {
			const se_key key = (se_key)binding->source_id;
			if (binding->state == SE_INPUT_DOWN) active = se_window_is_key_down(input_handle->window, key);
			if (binding->state == SE_INPUT_PRESS) active = se_window_is_key_pressed(input_handle->window, key);
			if (binding->state == SE_INPUT_RELEASE) active = se_window_is_key_released(input_handle->window, key);
			value = active ? 1.0f : 0.0f;
		} else if (binding->source_type == SE_INPUT_SOURCE_MOUSE_BUTTON && se_input_is_mouse_button_id(binding->source_id)) {
			const se_mouse_button button = se_input_id_to_mouse_button(binding->source_id);
			if (binding->state == SE_INPUT_DOWN) active = se_window_is_mouse_down(input_handle->window, button);
			if (binding->state == SE_INPUT_PRESS) active = se_window_is_mouse_pressed(input_handle->window, button);
			if (binding->state == SE_INPUT_RELEASE) active = se_window_is_mouse_released(input_handle->window, button);
			value = active ? 1.0f : 0.0f;
		} else if (binding->source_type == SE_INPUT_SOURCE_AXIS && se_input_is_axis_id(binding->source_id)) {
			const f32 raw = se_input_read_axis(binding->source_id, mouse_delta, scroll_delta);
			value = se_input_apply_axis_options(raw, &binding->axis, action->previous_value);
			active = fabsf(value) > SE_INPUT_AXIS_THRESHOLD;
		}

		action->value = value;
		action->down = active;
		action->pressed = (!action->down ? false : action->previous_value == 0.0f && action->value != 0.0f);
		action->released = (action->previous_value != 0.0f && action->value == 0.0f);

		if (action->pressed || action->released || fabsf(action->value - action->previous_value) > SE_INPUT_AXIS_THRESHOLD) {
			se_input_runtime_push_event(input_handle, action);
		}
	}
}

static void se_input_tick_replay(se_input_handle* input_handle) {
	if (!input_handle || !input_handle->runtime || !input_handle->runtime->replay_enabled) {
		return;
	}
	const f64 dt = se_window_get_delta_time(input_handle->window);
	input_handle->runtime->replay_time += s_max(dt, 0.0);

	while (input_handle->runtime->replay_index < s_array_get_size(&input_handle->runtime->recorded)) {
		se_input_event_record* record = s_array_get(&input_handle->runtime->recorded, s_array_handle(&input_handle->runtime->recorded, (u32)input_handle->runtime->replay_index));
		if (!record || record->timestamp > input_handle->runtime->replay_time) {
			break;
		}
		se_input_action* action = se_input_find_action(input_handle, record->action_id);
		if (action) {
			action->value = record->value;
			action->down = record->down;
			action->pressed = record->pressed;
			action->released = record->released;
			s_array_add(&input_handle->runtime->queue, *record);
		}
		input_handle->runtime->replay_index++;
	}

	if (input_handle->runtime->replay_index >= s_array_get_size(&input_handle->runtime->recorded)) {
		if (input_handle->runtime->replay_loop) {
			input_handle->runtime->replay_index = 0;
			input_handle->runtime->replay_time = 0.0;
		} else {
			input_handle->runtime->replay_enabled = false;
		}
	}
}

se_input_handle* se_input_create(const se_window_handle window, const u16 bindings_capacity) {
	se_context *ctx = se_current_context();
	if (!ctx || window == S_HANDLE_NULL || bindings_capacity == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_input_handle* input_handle = calloc(1, sizeof(*input_handle));
	if (!input_handle) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	input_handle->ctx = ctx;
	input_handle->window = window;
	input_handle->bindings_capacity = bindings_capacity;
	s_array_init(&input_handle->bindings);
	input_handle->enabled = true;
	input_handle->runtime = calloc(1, sizeof(*input_handle->runtime));
	if (!input_handle->runtime) {
		s_array_clear(&input_handle->bindings);
		free(input_handle);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	s_array_init(&input_handle->runtime->actions);
	s_array_init(&input_handle->runtime->contexts);
	s_array_init(&input_handle->runtime->context_stack);
	s_array_init(&input_handle->runtime->queue);
	s_array_init(&input_handle->runtime->recorded);
	se_set_last_error(SE_RESULT_OK);
	return input_handle;
}

void se_input_destroy(se_input_handle* input_handle) {
	if (!input_handle) {
		return;
	}
	s_array_clear(&input_handle->bindings);
	if (input_handle->runtime) {
		s_array_clear(&input_handle->runtime->actions);
		s_array_clear(&input_handle->runtime->contexts);
		s_array_clear(&input_handle->runtime->context_stack);
		s_array_clear(&input_handle->runtime->queue);
		s_array_clear(&input_handle->runtime->recorded);
		free(input_handle->runtime);
		input_handle->runtime = NULL;
	}
	free(input_handle);
}

b8 se_input_bind(se_input_handle* input_handle, const i32 id, const se_input_state state, se_input_callback callback, void* user_data) {
	if (!input_handle) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (s_array_get_size(&input_handle->bindings) >= input_handle->bindings_capacity) {
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

	s_handle binding_handle = s_array_increment(&input_handle->bindings);
	se_input_binding* binding_slot = s_array_get(&input_handle->bindings, binding_handle);
	*binding_slot = binding;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_input_unbind_all(se_input_handle* input_handle) {
	if (!input_handle) {
		return;
	}
	if (s_array_get_size(&input_handle->bindings) > 0) {
		s_array_clear(&input_handle->bindings);
		s_array_init(&input_handle->bindings);
	}
}

void se_input_set_enabled(se_input_handle* input_handle, const b8 enabled) {
	if (!input_handle) {
		return;
	}
	input_handle->enabled = enabled;
}

b8 se_input_is_enabled(se_input_handle* input_handle) {
	if (!input_handle) {
		return false;
	}
	return input_handle->enabled;
}

void se_input_tick(se_input_handle* input_handle) {
	if (!input_handle || input_handle->window == S_HANDLE_NULL || !input_handle->enabled) {
		return;
	}
	se_debug_trace_begin("input_tick");

	s_vec2 mouse_delta = s_vec2(0.0f, 0.0f);
	s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
	se_window_get_mouse_delta(input_handle->window, &mouse_delta);
	se_window_get_scroll_delta(input_handle->window, &scroll_delta);

	for (sz i = 0; i < s_array_get_size(&input_handle->bindings); i++) {
		se_input_binding* binding = s_array_get(&input_handle->bindings, s_array_handle(&input_handle->bindings, (u32)i));
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

	if (input_handle->runtime) {
		if (input_handle->runtime->replay_enabled) {
			se_input_tick_replay(input_handle);
		} else {
			se_input_tick_actions(input_handle, &mouse_delta, &scroll_delta);
		}
	}
	se_debug_trace_end("input_tick");
}

f32 se_input_get_value(se_input_handle* input_handle, const i32 id, const se_input_state state) {
	if (!input_handle) {
		return 0.0f;
	}

	f32 value = 0.0f;
	for (sz i = 0; i < s_array_get_size(&input_handle->bindings); i++) {
		const se_input_binding* binding = s_array_get(&input_handle->bindings, s_array_handle(&input_handle->bindings, (u32)i));
		if (!binding->is_valid) {
			continue;
		}
		if (binding->id == id && binding->state == state) {
			value += binding->value;
		}
	}
	return value;
}

b8 se_input_action_create(se_input_handle* input_handle, const i32 action_id, const c8* name, const i32 context_id) {
	if (!input_handle || !input_handle->runtime || !name || action_id < 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (se_input_find_action(input_handle, action_id)) {
		se_debug_log(SE_DEBUG_LEVEL_WARN, SE_DEBUG_CATEGORY_INPUT, "se_input_action_create :: action id %d already exists", action_id);
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}
	se_input_action action = {0};
	action.action_id = action_id;
	strncpy(action.name, name, sizeof(action.name) - 1);
	action.context_id = context_id;
	action.binding.axis = SE_INPUT_AXIS_OPTIONS_DEFAULTS;
	action.valid = true;
	s_array_add(&input_handle->runtime->actions, action);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_action_bind(se_input_handle* input_handle, const i32 action_id, const se_input_action_binding* binding) {
	if (!input_handle || !binding) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_input_action* action = se_input_find_action(input_handle, action_id);
	if (!action) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	action->binding = *binding;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_action_rebind_source(se_input_handle* input_handle, const i32 action_id, const i32 source_id, const se_input_source_type source_type, const se_input_state state) {
	se_input_action* action = se_input_find_action(input_handle, action_id);
	if (!action) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	action->binding.source_id = source_id;
	action->binding.source_type = source_type;
	action->binding.state = state;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_action_unbind(se_input_handle* input_handle, const i32 action_id) {
	se_input_action* action = se_input_find_action(input_handle, action_id);
	if (!action) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	memset(&action->binding, 0, sizeof(action->binding));
	action->binding.axis = SE_INPUT_AXIS_OPTIONS_DEFAULTS;
	action->value = 0.0f;
	action->down = false;
	action->pressed = false;
	action->released = false;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

f32 se_input_action_get_value(se_input_handle* input_handle, const i32 action_id) {
	const se_input_action* action = se_input_find_action_const(input_handle, action_id);
	if (!action) {
		return 0.0f;
	}
	return action->value;
}

b8 se_input_action_is_down(se_input_handle* input_handle, const i32 action_id) {
	const se_input_action* action = se_input_find_action_const(input_handle, action_id);
	return action ? action->down : false;
}

b8 se_input_action_is_pressed(se_input_handle* input_handle, const i32 action_id) {
	const se_input_action* action = se_input_find_action_const(input_handle, action_id);
	return action ? action->pressed : false;
}

b8 se_input_action_is_released(se_input_handle* input_handle, const i32 action_id) {
	const se_input_action* action = se_input_find_action_const(input_handle, action_id);
	return action ? action->released : false;
}

b8 se_input_bind_wasd_mouse_look(
	se_input_handle* input_handle,
	const i32 context_id,
	const i32 action_forward,
	const i32 action_backward,
	const i32 action_left,
	const i32 action_right,
	const i32 action_look_x,
	const i32 action_look_y,
	const f32 look_sensitivity) {
	if (!input_handle) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	const f32 sensitivity = look_sensitivity > 0.0f ? look_sensitivity : 0.01f;

	const b8 ok =
		se_input_action_create(input_handle, action_forward, "move_forward", context_id) &&
		se_input_action_create(input_handle, action_backward, "move_backward", context_id) &&
		se_input_action_create(input_handle, action_left, "move_left", context_id) &&
		se_input_action_create(input_handle, action_right, "move_right", context_id) &&
		se_input_action_create(input_handle, action_look_x, "look_x", context_id) &&
		se_input_action_create(input_handle, action_look_y, "look_y", context_id) &&
		se_input_action_bind(input_handle, action_forward, &(se_input_action_binding){
			.source_id = SE_KEY_W,
			.source_type = SE_INPUT_SOURCE_KEY,
			.state = SE_INPUT_DOWN
		}) &&
		se_input_action_bind(input_handle, action_backward, &(se_input_action_binding){
			.source_id = SE_KEY_S,
			.source_type = SE_INPUT_SOURCE_KEY,
			.state = SE_INPUT_DOWN
		}) &&
		se_input_action_bind(input_handle, action_left, &(se_input_action_binding){
			.source_id = SE_KEY_A,
			.source_type = SE_INPUT_SOURCE_KEY,
			.state = SE_INPUT_DOWN
		}) &&
		se_input_action_bind(input_handle, action_right, &(se_input_action_binding){
			.source_id = SE_KEY_D,
			.source_type = SE_INPUT_SOURCE_KEY,
			.state = SE_INPUT_DOWN
		}) &&
		se_input_action_bind(input_handle, action_look_x, &(se_input_action_binding){
			.source_id = SE_INPUT_MOUSE_DELTA_X,
			.source_type = SE_INPUT_SOURCE_AXIS,
			.state = SE_INPUT_AXIS,
			.axis = {
				.deadzone = 0.0f,
				.sensitivity = sensitivity,
				.exponent = 1.0f,
				.smoothing = 0.0f
			}
		}) &&
		se_input_action_bind(input_handle, action_look_y, &(se_input_action_binding){
			.source_id = SE_INPUT_MOUSE_DELTA_Y,
			.source_type = SE_INPUT_SOURCE_AXIS,
			.state = SE_INPUT_AXIS,
			.axis = {
				.deadzone = 0.0f,
				.sensitivity = sensitivity,
				.exponent = 1.0f,
				.smoothing = 0.0f
			}
		});
	if (!ok) {
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_context_create(se_input_handle* input_handle, const i32 context_id, const c8* name, const b8 enabled) {
	if (!input_handle || !input_handle->runtime || !name) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (se_input_find_context(input_handle, context_id)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}
	se_input_context context = {0};
	context.context_id = context_id;
	strncpy(context.name, name, sizeof(context.name) - 1);
	context.enabled = enabled;
	s_array_add(&input_handle->runtime->contexts, context);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_context_set_enabled(se_input_handle* input_handle, const i32 context_id, const b8 enabled) {
	se_input_context* context = se_input_find_context(input_handle, context_id);
	if (!context) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	context->enabled = enabled;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_context_is_enabled(se_input_handle* input_handle, const i32 context_id) {
	return se_input_context_enabled(input_handle, context_id);
}

b8 se_input_context_push(se_input_handle* input_handle, const i32 context_id) {
	if (!input_handle || !input_handle->runtime || !se_input_find_context(input_handle, context_id)) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	s_array_add(&input_handle->runtime->context_stack, context_id);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_context_pop(se_input_handle* input_handle, i32* out_context_id) {
	if (!input_handle || !input_handle->runtime || s_array_get_size(&input_handle->runtime->context_stack) == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	s_handle top = s_array_handle(&input_handle->runtime->context_stack, (u32)(s_array_get_size(&input_handle->runtime->context_stack) - 1));
	i32* context_id = s_array_get(&input_handle->runtime->context_stack, top);
	if (out_context_id) {
		*out_context_id = context_id ? *context_id : -1;
	}
	s_array_remove(&input_handle->runtime->context_stack, top);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_context_peek(se_input_handle* input_handle, i32* out_context_id) {
	if (!input_handle || !input_handle->runtime || !out_context_id || s_array_get_size(&input_handle->runtime->context_stack) == 0) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	i32* context_id = s_array_get(
		&input_handle->runtime->context_stack,
		s_array_handle(&input_handle->runtime->context_stack, (u32)(s_array_get_size(&input_handle->runtime->context_stack) - 1)));
	*out_context_id = context_id ? *context_id : -1;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_input_context_clear_stack(se_input_handle* input_handle) {
	if (!input_handle || !input_handle->runtime) {
		return;
	}
	s_array_clear(&input_handle->runtime->context_stack);
	s_array_init(&input_handle->runtime->context_stack);
}

void se_input_set_text_callback(se_input_handle* input_handle, se_input_text_callback callback, void* user_data) {
	if (!input_handle) {
		return;
	}
	input_handle->text_callback = callback;
	input_handle->text_user_data = user_data;
}

void se_input_attach_window_text(se_input_handle* input_handle) {
	if (!input_handle || input_handle->window == S_HANDLE_NULL) {
		return;
	}
	se_window_set_text_callback(input_handle->window, se_input_window_text_bridge, input_handle);
}

void se_input_emit_text(se_input_handle* input_handle, const c8* utf8_text) {
	if (!input_handle || !utf8_text || !input_handle->text_callback) {
		return;
	}
	input_handle->text_callback(utf8_text, input_handle->text_user_data);
}

b8 se_input_get_event_queue(se_input_handle* input_handle, const se_input_event_record** out_events, sz* out_count) {
	if (!input_handle || !input_handle->runtime || !out_events || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	*out_events = s_array_get_data(&input_handle->runtime->queue);
	*out_count = s_array_get_size(&input_handle->runtime->queue);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_input_clear_event_queue(se_input_handle* input_handle) {
	if (!input_handle || !input_handle->runtime) {
		return;
	}
	s_array_clear(&input_handle->runtime->queue);
	s_array_init(&input_handle->runtime->queue);
}

void se_input_record_start(se_input_handle* input_handle) {
	if (!input_handle || !input_handle->runtime) {
		return;
	}
	input_handle->runtime->record_start_time = se_window_get_time(input_handle->window);
	input_handle->runtime->recording_enabled = true;
}

void se_input_record_stop(se_input_handle* input_handle) {
	if (!input_handle || !input_handle->runtime) {
		return;
	}
	input_handle->runtime->recording_enabled = false;
}

void se_input_record_clear(se_input_handle* input_handle) {
	if (!input_handle || !input_handle->runtime) {
		return;
	}
	s_array_clear(&input_handle->runtime->recorded);
	s_array_init(&input_handle->runtime->recorded);
}

b8 se_input_replay_start(se_input_handle* input_handle, const b8 loop) {
	if (!input_handle || !input_handle->runtime || s_array_get_size(&input_handle->runtime->recorded) == 0) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	input_handle->runtime->replay_enabled = true;
	input_handle->runtime->replay_loop = loop;
	input_handle->runtime->replay_index = 0;
	input_handle->runtime->replay_time = 0.0;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_input_replay_stop(se_input_handle* input_handle) {
	if (!input_handle || !input_handle->runtime) {
		return;
	}
	input_handle->runtime->replay_enabled = false;
	input_handle->runtime->replay_index = 0;
	input_handle->runtime->replay_time = 0.0;
}

b8 se_input_save_mappings(se_input_handle* input_handle, const c8* path) {
	if (!input_handle || !input_handle->runtime || !path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	FILE* fp = fopen(path, "w");
	if (!fp) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	for (sz i = 0; i < s_array_get_size(&input_handle->runtime->actions); ++i) {
		const se_input_action* action = s_array_get(&input_handle->runtime->actions, s_array_handle(&input_handle->runtime->actions, (u32)i));
		if (!action || !action->valid) {
			continue;
		}
		c8 line[512] = {0};
		const i32 line_len = snprintf(
			line,
			sizeof(line),
			"%d,%s,%d,%d,%d,%d,%u,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f\n",
			action->action_id,
			action->name,
			action->context_id,
			action->binding.source_id,
			action->binding.source_type,
			action->binding.state,
			(u32)action->binding.chord_count,
			action->binding.chord_keys[0],
			action->binding.chord_keys[1],
			action->binding.chord_keys[2],
			action->binding.chord_keys[3],
			action->binding.axis.deadzone,
			action->binding.axis.sensitivity,
			action->binding.axis.exponent,
			action->binding.axis.smoothing);
		if (line_len > 0) {
			fputs(line, fp);
		}
	}
	fclose(fp);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_load_mappings(se_input_handle* input_handle, const c8* path) {
	if (!input_handle || !input_handle->runtime || !path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	FILE* fp = fopen(path, "r");
	if (!fp) {
		se_set_last_error(SE_RESULT_IO);
		return false;
	}
	s_array_clear(&input_handle->runtime->actions);
	s_array_init(&input_handle->runtime->actions);
	char line[512] = {0};
	while (fgets(line, sizeof(line), fp)) {
		se_input_action action = {0};
		i32 chord_count = 0;
		if (sscanf(
				line,
				"%d,%63[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f,%f",
				&action.action_id,
				action.name,
				&action.context_id,
				&action.binding.source_id,
				(i32*)&action.binding.source_type,
				(i32*)&action.binding.state,
				&chord_count,
				&action.binding.chord_keys[0],
				&action.binding.chord_keys[1],
				&action.binding.chord_keys[2],
				&action.binding.chord_keys[3],
				&action.binding.axis.deadzone,
				&action.binding.axis.sensitivity,
				&action.binding.axis.exponent,
				&action.binding.axis.smoothing) != 15) {
			continue;
		}
		action.binding.chord_count = (u8)s_max(0, s_min(chord_count, 4));
		action.valid = true;
		s_array_add(&input_handle->runtime->actions, action);
	}
	fclose(fp);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_input_dump_diagnostics(se_input_handle* input_handle, c8* out_buffer, const sz out_buffer_size) {
	if (!out_buffer || out_buffer_size == 0) {
		return;
	}
	if (!input_handle || !input_handle->runtime) {
		snprintf(out_buffer, out_buffer_size, "se_input diagnostics unavailable");
		return;
	}
	snprintf(
		out_buffer,
		out_buffer_size,
		"bindings=%zu actions=%zu contexts=%zu queue=%zu recorded=%zu recording=%d replay=%d",
		s_array_get_size(&input_handle->bindings),
		s_array_get_size(&input_handle->runtime->actions),
		s_array_get_size(&input_handle->runtime->contexts),
		s_array_get_size(&input_handle->runtime->queue),
		s_array_get_size(&input_handle->runtime->recorded),
		input_handle->runtime->recording_enabled ? 1 : 0,
		input_handle->runtime->replay_enabled ? 1 : 0);
}

void se_input_get_diagnostics(se_input_handle* input_handle, se_input_diagnostics* out_diagnostics) {
	if (!out_diagnostics) {
		return;
	}
	memset(out_diagnostics, 0, sizeof(*out_diagnostics));
	if (!input_handle || !input_handle->runtime) {
		return;
	}
	out_diagnostics->action_count = (u32)s_array_get_size(&input_handle->runtime->actions);
	out_diagnostics->context_count = (u32)s_array_get_size(&input_handle->runtime->contexts);
	out_diagnostics->queue_size = (u32)s_array_get_size(&input_handle->runtime->queue);
	out_diagnostics->queue_capacity = (u32)s_array_get_capacity(&input_handle->runtime->queue);
	out_diagnostics->recorded_count = (u32)s_array_get_size(&input_handle->runtime->recorded);
	out_diagnostics->recording_enabled = input_handle->runtime->recording_enabled;
	out_diagnostics->replay_enabled = input_handle->runtime->replay_enabled;
}
