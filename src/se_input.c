// Syphax-Engine - Ougi Washi

#include "se_input.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SE_INPUT_ACTION_THRESHOLD 0.0001f

typedef struct {
	se_input_device device;
	se_input_trigger trigger;
	se_input_modifiers modifiers;
	f32 scale;
	se_key key;
	se_mouse_button mouse_button;
	b8 invert : 1;
	b8 exact_modifiers : 1;
	b8 is_valid : 1;
} se_input_binding;

typedef struct {
	se_input_binding* bindings;
	u16 count;
	u16 capacity;
} se_input_action_map;

typedef struct {
	c8 name[SE_INPUT_CONTEXT_NAME_LENGTH];
	i32 priority;
	b8 enabled : 1;
	b8 is_valid : 1;
	se_input_action_map* actions;
} se_input_context_slot;

typedef struct {
	f32 value;
	b8 down : 1;
	b8 pressed : 1;
	b8 released : 1;
} se_input_action_state;

typedef struct {
	f32 value;
	b8 down : 1;
	b8 pressed : 1;
	b8 released : 1;
} se_input_eval;

struct se_input {
	se_input_params params;
	se_input_context_slot* contexts;
	se_input_action_state* action_states;
};

static b8 se_input_valid_action(const se_input* input, const u16 action) {
	return input && action < input->params.actions_count;
}

static b8 se_input_valid_context_id(const se_input* input, const se_input_context_id context_id) {
	if (!input || context_id < 0) {
		return false;
	}
	return (u16)context_id < input->params.contexts_count;
}

static b8 se_input_valid_context(const se_input* input, const se_input_context_id context_id) {
	if (!se_input_valid_context_id(input, context_id)) {
		return false;
	}
	return input->contexts[context_id].is_valid;
}

static se_input_modifiers se_input_read_modifiers(se_window* window) {
	se_input_modifiers modifiers = SE_INPUT_MODIFIERS_NONE;
	modifiers.shift = se_window_is_key_down(window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(window, SE_KEY_RIGHT_SHIFT);
	modifiers.ctrl = se_window_is_key_down(window, SE_KEY_LEFT_CONTROL) || se_window_is_key_down(window, SE_KEY_RIGHT_CONTROL);
	modifiers.alt = se_window_is_key_down(window, SE_KEY_LEFT_ALT) || se_window_is_key_down(window, SE_KEY_RIGHT_ALT);
	modifiers.super = se_window_is_key_down(window, SE_KEY_LEFT_SUPER) || se_window_is_key_down(window, SE_KEY_RIGHT_SUPER);
	return modifiers;
}

static b8 se_input_modifiers_match(const se_input_modifiers* required, const se_input_modifiers* current, const b8 exact) {
	if (exact) {
		if (required->shift != current->shift) return false;
		if (required->ctrl != current->ctrl) return false;
		if (required->alt != current->alt) return false;
		if (required->super != current->super) return false;
		return true;
	}
	if (required->shift && !current->shift) return false;
	if (required->ctrl && !current->ctrl) return false;
	if (required->alt && !current->alt) return false;
	if (required->super && !current->super) return false;
	return true;
}

static void se_input_accumulate_digital(se_input_eval* eval, const se_input_trigger trigger, const f32 scale, const b8 down, const b8 pressed, const b8 released) {
	if (trigger == SE_INPUT_TRIGGER_DOWN || trigger == SE_INPUT_TRIGGER_AXIS) {
		if (down) {
			eval->value += scale;
			eval->down = true;
		}
		return;
	}
	if (trigger == SE_INPUT_TRIGGER_PRESSED) {
		if (pressed) {
			eval->value += scale;
			eval->pressed = true;
		}
		return;
	}
	if (trigger == SE_INPUT_TRIGGER_RELEASED) {
		if (released) {
			eval->value += scale;
			eval->released = true;
		}
	}
}

static se_input_eval se_input_evaluate_action(const se_input* input, se_window* window, const se_input_action_map* action_map, const se_input_modifiers* modifiers, const s_vec2* mouse_delta, const s_vec2* scroll_delta) {
	se_input_eval eval = {0};
	if (!action_map) {
		return eval;
	}

	for (u16 i = 0; i < action_map->count; i++) {
		const se_input_binding* binding = &action_map->bindings[i];
		if (!binding->is_valid) {
			continue;
		}
		if (!se_input_modifiers_match(&binding->modifiers, modifiers, binding->exact_modifiers)) {
			continue;
		}

		if (binding->device == SE_INPUT_DEVICE_KEY) {
			const b8 down = se_window_is_key_down(window, binding->key);
			const b8 pressed = se_window_is_key_pressed(window, binding->key);
			const b8 released = se_window_is_key_released(window, binding->key);
			se_input_accumulate_digital(&eval, binding->trigger, binding->scale, down, pressed, released);
			continue;
		}

		if (binding->device == SE_INPUT_DEVICE_MOUSE_BUTTON) {
			const b8 down = se_window_is_mouse_down(window, binding->mouse_button);
			const b8 pressed = se_window_is_mouse_pressed(window, binding->mouse_button);
			const b8 released = se_window_is_mouse_released(window, binding->mouse_button);
			se_input_accumulate_digital(&eval, binding->trigger, binding->scale, down, pressed, released);
			continue;
		}

		if (binding->trigger != SE_INPUT_TRIGGER_AXIS) {
			continue;
		}

		f32 axis_value = 0.0f;
		switch (binding->device) {
			case SE_INPUT_DEVICE_MOUSE_DELTA_X:
				axis_value = mouse_delta->x;
				break;
			case SE_INPUT_DEVICE_MOUSE_DELTA_Y:
				axis_value = mouse_delta->y;
				break;
			case SE_INPUT_DEVICE_MOUSE_SCROLL_X:
				axis_value = scroll_delta->x;
				break;
			case SE_INPUT_DEVICE_MOUSE_SCROLL_Y:
				axis_value = scroll_delta->y;
				break;
			default:
				break;
		}

		if (binding->invert) {
			axis_value = -axis_value;
		}
		axis_value *= binding->scale;
		eval.value += axis_value;
		if (fabsf(axis_value) > SE_INPUT_ACTION_THRESHOLD) {
			eval.down = true;
		}
	}

	if (!eval.down && fabsf(eval.value) > SE_INPUT_ACTION_THRESHOLD) {
		eval.down = true;
	}

	return eval;
}

static b8 se_input_binding_desc_valid(const se_input_binding_desc* binding) {
	if (!binding) {
		return false;
	}
	if (binding->device == SE_INPUT_DEVICE_KEY) {
		return binding->key >= 0 && binding->key < SE_KEY_COUNT;
	}
	if (binding->device == SE_INPUT_DEVICE_MOUSE_BUTTON) {
		return binding->mouse_button >= 0 && binding->mouse_button < SE_MOUSE_BUTTON_COUNT;
	}
	if (binding->device == SE_INPUT_DEVICE_MOUSE_DELTA_X ||
		binding->device == SE_INPUT_DEVICE_MOUSE_DELTA_Y ||
		binding->device == SE_INPUT_DEVICE_MOUSE_SCROLL_X ||
		binding->device == SE_INPUT_DEVICE_MOUSE_SCROLL_Y) {
		return binding->trigger == SE_INPUT_TRIGGER_AXIS;
	}
	return false;
}

se_input* se_input_create(const se_input_params* params) {
	se_input_params resolved = SE_INPUT_PARAMS_DEFAULTS;
	if (params) {
		resolved = *params;
	}
	if (resolved.actions_count == 0) resolved.actions_count = SE_INPUT_PARAMS_DEFAULTS.actions_count;
	if (resolved.contexts_count == 0) resolved.contexts_count = SE_INPUT_PARAMS_DEFAULTS.contexts_count;
	if (resolved.bindings_per_action == 0) resolved.bindings_per_action = SE_INPUT_PARAMS_DEFAULTS.bindings_per_action;

	se_input* input = calloc(1, sizeof(*input));
	if (!input) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	input->params = resolved;

	input->contexts = calloc(input->params.contexts_count, sizeof(*input->contexts));
	input->action_states = calloc(input->params.actions_count, sizeof(*input->action_states));
	if (!input->contexts || !input->action_states) {
		se_input_destroy(input);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	for (u16 context_idx = 0; context_idx < input->params.contexts_count; context_idx++) {
		se_input_context_slot* context = &input->contexts[context_idx];
		context->actions = calloc(input->params.actions_count, sizeof(*context->actions));
		if (!context->actions) {
			se_input_destroy(input);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return NULL;
		}

		for (u16 action_idx = 0; action_idx < input->params.actions_count; action_idx++) {
			se_input_action_map* action = &context->actions[action_idx];
			action->capacity = input->params.bindings_per_action;
			action->bindings = calloc(action->capacity, sizeof(*action->bindings));
			if (!action->bindings) {
				se_input_destroy(input);
				se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
				return NULL;
			}
		}
	}

	se_set_last_error(SE_RESULT_OK);
	return input;
}

void se_input_destroy(se_input* input) {
	if (!input) {
		return;
	}

	if (input->contexts) {
		for (u16 context_idx = 0; context_idx < input->params.contexts_count; context_idx++) {
			se_input_context_slot* context = &input->contexts[context_idx];
			if (context->actions) {
				for (u16 action_idx = 0; action_idx < input->params.actions_count; action_idx++) {
					se_input_action_map* action = &context->actions[action_idx];
					if (action->bindings) {
						free(action->bindings);
						action->bindings = NULL;
					}
					action->count = 0;
					action->capacity = 0;
				}
				free(context->actions);
				context->actions = NULL;
			}
		}
		free(input->contexts);
		input->contexts = NULL;
	}

	if (input->action_states) {
		free(input->action_states);
		input->action_states = NULL;
	}

	free(input);
}

void se_input_tick(se_input* input, se_window* window) {
	if (!input || !window) {
		return;
	}

	s_vec2 mouse_delta = s_vec2(0.0f, 0.0f);
	s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
	se_window_get_mouse_delta(window, &mouse_delta);
	se_window_get_scroll_delta(window, &scroll_delta);
	const se_input_modifiers modifiers = se_input_read_modifiers(window);

	for (u16 action_idx = 0; action_idx < input->params.actions_count; action_idx++) {
		se_input_action_state* state = &input->action_states[action_idx];
		const b8 was_down = state->down;

		i32 selected_context_idx = SE_INPUT_CONTEXT_INVALID;
		i32 selected_priority = INT_MIN;
		for (u16 context_idx = 0; context_idx < input->params.contexts_count; context_idx++) {
			se_input_context_slot* context = &input->contexts[context_idx];
			if (!context->is_valid || !context->enabled) {
				continue;
			}
			se_input_action_map* action_map = &context->actions[action_idx];
			if (action_map->count == 0) {
				continue;
			}
			if (context->priority > selected_priority) {
				selected_priority = context->priority;
				selected_context_idx = (i32)context_idx;
			}
		}

		se_input_eval eval = {0};
		if (selected_context_idx != SE_INPUT_CONTEXT_INVALID) {
			se_input_context_slot* context = &input->contexts[selected_context_idx];
			se_input_action_map* action_map = &context->actions[action_idx];
			eval = se_input_evaluate_action(input, window, action_map, &modifiers, &mouse_delta, &scroll_delta);
		}

		state->value = eval.value;
		state->down = eval.down;
		state->pressed = eval.pressed || (state->down && !was_down);
		state->released = eval.released || (!state->down && was_down);
	}
}

se_input_context_id se_input_context_create(se_input* input, const c8* name, const i32 priority) {
	if (!input) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return SE_INPUT_CONTEXT_INVALID;
	}

	for (u16 context_idx = 0; context_idx < input->params.contexts_count; context_idx++) {
		se_input_context_slot* context = &input->contexts[context_idx];
		if (context->is_valid) {
			continue;
		}

		memset(context->name, 0, sizeof(context->name));
		if (name && name[0] != '\0') {
			strncpy(context->name, name, sizeof(context->name) - 1);
		}
		context->priority = priority;
		context->enabled = true;
		context->is_valid = true;
		for (u16 action_idx = 0; action_idx < input->params.actions_count; action_idx++) {
			context->actions[action_idx].count = 0;
		}

		se_set_last_error(SE_RESULT_OK);
		return (se_input_context_id)context_idx;
	}

	se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
	return SE_INPUT_CONTEXT_INVALID;
}

b8 se_input_context_destroy(se_input* input, const se_input_context_id context_id) {
	if (!se_input_valid_context(input, context_id)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_input_context_slot* context = &input->contexts[context_id];
	for (u16 action_idx = 0; action_idx < input->params.actions_count; action_idx++) {
		context->actions[action_idx].count = 0;
	}
	context->enabled = false;
	context->is_valid = false;
	context->priority = 0;
	memset(context->name, 0, sizeof(context->name));

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_set_context_active(se_input* input, const se_input_context_id context_id, const b8 active) {
	if (!se_input_valid_context(input, context_id)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	input->contexts[context_id].enabled = active;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_is_context_active(const se_input* input, const se_input_context_id context_id) {
	if (!se_input_valid_context(input, context_id)) {
		return false;
	}
	return input->contexts[context_id].enabled;
}

b8 se_input_set_context_priority(se_input* input, const se_input_context_id context_id, const i32 priority) {
	if (!se_input_valid_context(input, context_id)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	input->contexts[context_id].priority = priority;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_context_clear_bindings(se_input* input, const se_input_context_id context_id) {
	if (!se_input_valid_context(input, context_id)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_input_context_slot* context = &input->contexts[context_id];
	for (u16 action_idx = 0; action_idx < input->params.actions_count; action_idx++) {
		context->actions[action_idx].count = 0;
	}

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_action_clear_bindings(se_input* input, const se_input_context_id context_id, const u16 action) {
	if (!se_input_valid_context(input, context_id) || !se_input_valid_action(input, action)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	input->contexts[context_id].actions[action].count = 0;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_bind(se_input* input, const se_input_context_id context_id, const u16 action, const se_input_binding_desc* binding) {
	if (!se_input_valid_context(input, context_id) || !se_input_valid_action(input, action) || !se_input_binding_desc_valid(binding)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_input_action_map* action_map = &input->contexts[context_id].actions[action];
	if (action_map->count >= action_map->capacity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	se_input_binding* out_binding = &action_map->bindings[action_map->count++];
	out_binding->device = binding->device;
	out_binding->trigger = binding->trigger;
	out_binding->modifiers = binding->modifiers;
	out_binding->scale = binding->scale;
	out_binding->key = binding->key;
	out_binding->mouse_button = binding->mouse_button;
	out_binding->invert = binding->invert;
	out_binding->exact_modifiers = binding->exact_modifiers;
	out_binding->is_valid = true;

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_input_bind_key(se_input* input, const se_input_context_id context_id, const u16 action, const se_key key, const se_input_trigger trigger, const f32 scale, const se_input_modifiers modifiers, const b8 exact_modifiers) {
	se_input_binding_desc binding = SE_INPUT_BINDING_DESC_DEFAULTS;
	binding.device = SE_INPUT_DEVICE_KEY;
	binding.trigger = trigger;
	binding.scale = scale;
	binding.modifiers = modifiers;
	binding.key = key;
	binding.exact_modifiers = exact_modifiers;
	return se_input_bind(input, context_id, action, &binding);
}

b8 se_input_bind_mouse_button(se_input* input, const se_input_context_id context_id, const u16 action, const se_mouse_button button, const se_input_trigger trigger, const f32 scale, const se_input_modifiers modifiers, const b8 exact_modifiers) {
	se_input_binding_desc binding = SE_INPUT_BINDING_DESC_DEFAULTS;
	binding.device = SE_INPUT_DEVICE_MOUSE_BUTTON;
	binding.trigger = trigger;
	binding.scale = scale;
	binding.modifiers = modifiers;
	binding.mouse_button = button;
	binding.exact_modifiers = exact_modifiers;
	return se_input_bind(input, context_id, action, &binding);
}

b8 se_input_bind_mouse_delta(se_input* input, const se_input_context_id context_id, const u16 action, const se_input_axis axis, const f32 scale, const se_input_modifiers modifiers, const b8 invert, const b8 exact_modifiers) {
	se_input_binding_desc binding = SE_INPUT_BINDING_DESC_DEFAULTS;
	binding.device = axis == SE_INPUT_AXIS_X ? SE_INPUT_DEVICE_MOUSE_DELTA_X : SE_INPUT_DEVICE_MOUSE_DELTA_Y;
	binding.trigger = SE_INPUT_TRIGGER_AXIS;
	binding.scale = scale;
	binding.modifiers = modifiers;
	binding.invert = invert;
	binding.exact_modifiers = exact_modifiers;
	return se_input_bind(input, context_id, action, &binding);
}

b8 se_input_bind_mouse_scroll(se_input* input, const se_input_context_id context_id, const u16 action, const se_input_axis axis, const f32 scale, const se_input_modifiers modifiers, const b8 invert, const b8 exact_modifiers) {
	se_input_binding_desc binding = SE_INPUT_BINDING_DESC_DEFAULTS;
	binding.device = axis == SE_INPUT_AXIS_X ? SE_INPUT_DEVICE_MOUSE_SCROLL_X : SE_INPUT_DEVICE_MOUSE_SCROLL_Y;
	binding.trigger = SE_INPUT_TRIGGER_AXIS;
	binding.scale = scale;
	binding.modifiers = modifiers;
	binding.invert = invert;
	binding.exact_modifiers = exact_modifiers;
	return se_input_bind(input, context_id, action, &binding);
}

f32 se_input_get_action_value(const se_input* input, const u16 action) {
	if (!se_input_valid_action(input, action)) {
		return 0.0f;
	}
	return input->action_states[action].value;
}

b8 se_input_is_action_down(const se_input* input, const u16 action) {
	if (!se_input_valid_action(input, action)) {
		return false;
	}
	return input->action_states[action].down;
}

b8 se_input_is_action_pressed(const se_input* input, const u16 action) {
	if (!se_input_valid_action(input, action)) {
		return false;
	}
	return input->action_states[action].pressed;
}

b8 se_input_is_action_released(const se_input* input, const u16 action) {
	if (!se_input_valid_action(input, action)) {
		return false;
	}
	return input->action_states[action].released;
}

u16 se_input_get_actions_count(const se_input* input) {
	if (!input) {
		return 0;
	}
	return input->params.actions_count;
}

u16 se_input_get_contexts_count(const se_input* input) {
	if (!input) {
		return 0;
	}
	return input->params.contexts_count;
}
