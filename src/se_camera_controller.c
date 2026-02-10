// Syphax-Engine - Ougi Washi

#include "se_camera_controller.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SE_CAMERA_CONTROLLER_MIN_RADIUS 1.0f
#define SE_CAMERA_CONTROLLER_MIN_DISTANCE 0.1f
#define SE_CAMERA_CONTROLLER_FAST_MULTIPLIER 3.0f

struct se_camera_controller {
	se_input* input;
	se_window* window;
	se_camera* camera;
	se_input_context_id context_id;
	u16 action_base;
	f32 movement_speed;
	f32 mouse_x_speed;
	f32 mouse_y_speed;
	f32 min_pitch;
	f32 max_pitch;
	f32 min_focus_distance;
	f32 max_focus_distance;
	f32 focus_distance;
	s_vec3 orbit_target;
	s_vec3 scene_center;
	f32 scene_radius;
	f32 yaw;
	f32 pitch;
	se_camera_controller_preset preset;
	b8 enabled : 1;
	b8 invert_y : 1;
	b8 look_toggle : 1;
	b8 fly_toggle_active : 1;
	b8 lock_cursor_while_active : 1;
	b8 use_raw_mouse_motion : 1;
	b8 was_orbit_active : 1;
	b8 cursor_captured : 1;
	b8 use_custom_focus_limits : 1;
};

static f32 se_camera_controller_clampf(const f32 value, const f32 min_value, const f32 max_value) {
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static u16 se_camera_controller_action_id(const se_camera_controller* controller, const se_camera_controller_action action) {
	return (u16)(controller->action_base + (u16)action);
}

static se_input_modifiers se_camera_controller_modifiers(const b8 shift, const b8 ctrl, const b8 alt, const b8 super) {
	se_input_modifiers modifiers = SE_INPUT_MODIFIERS_NONE;
	modifiers.shift = shift;
	modifiers.ctrl = ctrl;
	modifiers.alt = alt;
	modifiers.super = super;
	return modifiers;
}

static s_vec3 se_camera_controller_forward_from_angles(const f32 yaw, const f32 pitch) {
	s_vec3 forward = s_vec3(
		sinf(yaw) * cosf(pitch),
		sinf(pitch),
		cosf(yaw) * cosf(pitch));
	return s_vec3_normalize(&forward);
}

static void se_camera_controller_update_focus_limits_from_radius(se_camera_controller* controller) {
	if (!controller || controller->use_custom_focus_limits) {
		return;
	}
	const f32 radius = controller->scene_radius < SE_CAMERA_CONTROLLER_MIN_RADIUS ? SE_CAMERA_CONTROLLER_MIN_RADIUS : controller->scene_radius;
	controller->min_focus_distance = radius * 0.05f;
	if (controller->min_focus_distance < SE_CAMERA_CONTROLLER_MIN_DISTANCE) {
		controller->min_focus_distance = SE_CAMERA_CONTROLLER_MIN_DISTANCE;
	}
	controller->max_focus_distance = radius * 30.0f;
	if (controller->max_focus_distance < controller->min_focus_distance * 2.0f) {
		controller->max_focus_distance = controller->min_focus_distance * 2.0f;
	}
}

static void se_camera_controller_clamp_focus_distance(se_camera_controller* controller) {
	if (!controller) {
		return;
	}
	controller->focus_distance = se_camera_controller_clampf(controller->focus_distance, controller->min_focus_distance, controller->max_focus_distance);
}

static void se_camera_controller_sync_from_camera(se_camera_controller* controller) {
	if (!controller || !controller->camera) {
		return;
	}

	s_vec3 to_target = s_vec3_sub(&controller->camera->target, &controller->camera->position);
	f32 distance = s_vec3_length(&to_target);
	if (distance <= 0.0001f) {
		to_target = s_vec3(0.0f, 0.0f, -1.0f);
		distance = 1.0f;
	}

	s_vec3 forward = s_vec3_normalize(&to_target);
	controller->yaw = atan2f(forward.x, forward.z);
	controller->pitch = asinf(forward.y);
	controller->focus_distance = distance;
	controller->orbit_target = controller->camera->target;
	controller->scene_center = controller->camera->target;
	controller->scene_radius = distance;
	if (controller->scene_radius < SE_CAMERA_CONTROLLER_MIN_RADIUS) {
		controller->scene_radius = SE_CAMERA_CONTROLLER_MIN_RADIUS;
	}

	se_camera_controller_update_focus_limits_from_radius(controller);
	se_camera_controller_clamp_focus_distance(controller);
}

static void se_camera_controller_set_cursor_capture(se_camera_controller* controller, const b8 capture) {
	if (!controller || !controller->window || !controller->lock_cursor_while_active) {
		return;
	}
	if (capture == controller->cursor_captured) {
		return;
	}

	if (capture) {
		se_window_set_cursor_mode(controller->window, SE_WINDOW_CURSOR_DISABLED);
		if (controller->use_raw_mouse_motion && se_window_is_raw_mouse_motion_supported(controller->window)) {
			se_window_set_raw_mouse_motion(controller->window, true);
		}
		controller->cursor_captured = true;
		return;
	}

	if (controller->use_raw_mouse_motion && se_window_is_raw_mouse_motion_supported(controller->window)) {
		se_window_set_raw_mouse_motion(controller->window, false);
	}
	se_window_set_cursor_mode(controller->window, SE_WINDOW_CURSOR_NORMAL);
	controller->cursor_captured = false;
}

static b8 se_camera_controller_bind_key(se_camera_controller* controller, const se_camera_controller_action action, const se_key key, const se_input_trigger trigger, const f32 scale, const se_input_modifiers modifiers, const b8 exact_modifiers) {
	return se_input_bind_key(controller->input, controller->context_id, se_camera_controller_action_id(controller, action), key, trigger, scale, modifiers, exact_modifiers);
}

static b8 se_camera_controller_bind_mouse_button(se_camera_controller* controller, const se_camera_controller_action action, const se_mouse_button button, const se_input_trigger trigger, const f32 scale, const se_input_modifiers modifiers, const b8 exact_modifiers) {
	return se_input_bind_mouse_button(controller->input, controller->context_id, se_camera_controller_action_id(controller, action), button, trigger, scale, modifiers, exact_modifiers);
}

static b8 se_camera_controller_bind_mouse_delta(se_camera_controller* controller, const se_camera_controller_action action, const se_input_axis axis, const f32 scale, const b8 invert) {
	return se_input_bind_mouse_delta(controller->input, controller->context_id, se_camera_controller_action_id(controller, action), axis, scale, SE_INPUT_MODIFIERS_NONE, invert, false);
}

static b8 se_camera_controller_bind_mouse_scroll(se_camera_controller* controller, const se_camera_controller_action action, const se_input_axis axis, const f32 scale, const b8 invert) {
	return se_input_bind_mouse_scroll(controller->input, controller->context_id, se_camera_controller_action_id(controller, action), axis, scale, SE_INPUT_MODIFIERS_NONE, invert, false);
}

static b8 se_camera_controller_bind_common_actions(se_camera_controller* controller) {
	b8 ok = true;
	ok &= se_camera_controller_bind_mouse_delta(controller, SE_CAMERA_CONTROLLER_ACTION_LOOK_X, SE_INPUT_AXIS_X, 1.0f, false);
	ok &= se_camera_controller_bind_mouse_delta(controller, SE_CAMERA_CONTROLLER_ACTION_LOOK_Y, SE_INPUT_AXIS_Y, 1.0f, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_FORWARD, SE_KEY_W, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_BACKWARD, SE_KEY_S, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_RIGHT, SE_KEY_D, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_LEFT, SE_KEY_A, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_UP, SE_KEY_E, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_DOWN, SE_KEY_Q, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_SPEED, SE_KEY_LEFT_SHIFT, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_SPEED, SE_KEY_RIGHT_SHIFT, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_key(controller, SE_CAMERA_CONTROLLER_ACTION_FOCUS, SE_KEY_F, SE_INPUT_TRIGGER_PRESSED, 1.0f, SE_INPUT_MODIFIERS_NONE, false);
	ok &= se_camera_controller_bind_mouse_scroll(controller, SE_CAMERA_CONTROLLER_ACTION_ZOOM, SE_INPUT_AXIS_Y, 80.0f, true);
	return ok;
}

static b8 se_camera_controller_bind_ue(se_camera_controller* controller) {
	b8 ok = true;
	const se_input_modifiers alt = se_camera_controller_modifiers(false, false, true, false);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_FLY, SE_MOUSE_RIGHT, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, true);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_ORBIT, SE_MOUSE_LEFT, SE_INPUT_TRIGGER_DOWN, 1.0f, alt, false);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_PAN, SE_MOUSE_MIDDLE, SE_INPUT_TRIGGER_DOWN, 1.0f, alt, false);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_DOLLY, SE_MOUSE_RIGHT, SE_INPUT_TRIGGER_DOWN, 1.0f, alt, false);
	return ok;
}

static b8 se_camera_controller_bind_blender(se_camera_controller* controller) {
	b8 ok = true;
	const se_input_modifiers shift = se_camera_controller_modifiers(true, false, false, false);
	const se_input_modifiers ctrl = se_camera_controller_modifiers(false, true, false, false);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_FLY, SE_MOUSE_RIGHT, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, true);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_ORBIT, SE_MOUSE_MIDDLE, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, true);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_PAN, SE_MOUSE_MIDDLE, SE_INPUT_TRIGGER_DOWN, 1.0f, shift, true);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_DOLLY, SE_MOUSE_MIDDLE, SE_INPUT_TRIGGER_DOWN, 1.0f, ctrl, true);
	return ok;
}

static b8 se_camera_controller_bind_trackpad(se_camera_controller* controller) {
	b8 ok = true;
	const se_input_modifiers alt = se_camera_controller_modifiers(false, false, true, false);
	const se_input_modifiers shift_alt = se_camera_controller_modifiers(true, false, true, false);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_FLY, SE_MOUSE_RIGHT, SE_INPUT_TRIGGER_DOWN, 1.0f, SE_INPUT_MODIFIERS_NONE, true);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_ORBIT, SE_MOUSE_LEFT, SE_INPUT_TRIGGER_DOWN, 1.0f, alt, false);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_PAN, SE_MOUSE_LEFT, SE_INPUT_TRIGGER_DOWN, 1.0f, shift_alt, false);
	ok &= se_camera_controller_bind_mouse_button(controller, SE_CAMERA_CONTROLLER_ACTION_DOLLY, SE_MOUSE_RIGHT, SE_INPUT_TRIGGER_DOWN, 1.0f, alt, false);
	return ok;
}

se_camera_controller* se_camera_controller_create(const se_camera_controller_params* params) {
	if (!params || !params->input || !params->window || !params->camera) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	const u32 required_actions = (u32)params->action_base + (u32)SE_CAMERA_CONTROLLER_ACTION_COUNT;
	if (required_actions > se_input_get_actions_count(params->input)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}

	se_camera_controller* controller = calloc(1, sizeof(*controller));
	if (!controller) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	controller->input = params->input;
	controller->window = params->window;
	controller->camera = params->camera;
	controller->action_base = params->action_base;
	controller->movement_speed = params->movement_speed;
	controller->mouse_x_speed = params->mouse_x_speed;
	controller->mouse_y_speed = params->mouse_y_speed;
	controller->min_pitch = params->min_pitch;
	controller->max_pitch = params->max_pitch;
	controller->enabled = params->enabled;
	controller->invert_y = params->invert_y;
	controller->look_toggle = params->look_toggle;
	controller->lock_cursor_while_active = params->lock_cursor_while_active;
	controller->use_raw_mouse_motion = params->use_raw_mouse_motion;
	controller->preset = params->preset;
	if (controller->movement_speed <= 0.0f) controller->movement_speed = 1.0f;
	if (controller->mouse_x_speed <= 0.0f) controller->mouse_x_speed = 0.001f;
	if (controller->mouse_y_speed <= 0.0f) controller->mouse_y_speed = 0.001f;
	controller->context_id = se_input_context_create(controller->input, "camera_controller", params->context_priority);
	if (controller->context_id == SE_INPUT_CONTEXT_INVALID) {
		free(controller);
		return NULL;
	}

	if (params->min_focus_distance > 0.0f && params->max_focus_distance > params->min_focus_distance) {
		controller->min_focus_distance = params->min_focus_distance;
		controller->max_focus_distance = params->max_focus_distance;
		controller->use_custom_focus_limits = true;
	}

	se_camera_controller_sync_from_camera(controller);
	if (!se_camera_controller_set_preset(controller, params->preset)) {
		se_input_context_destroy(controller->input, controller->context_id);
		free(controller);
		return NULL;
	}

	se_input_set_context_active(controller->input, controller->context_id, controller->enabled);
	se_set_last_error(SE_RESULT_OK);
	return controller;
}

void se_camera_controller_destroy(se_camera_controller* controller) {
	if (!controller) {
		return;
	}

	se_camera_controller_set_cursor_capture(controller, false);
	if (controller->input && controller->context_id != SE_INPUT_CONTEXT_INVALID) {
		se_input_context_destroy(controller->input, controller->context_id);
	}
	free(controller);
}

void se_camera_controller_tick(se_camera_controller* controller, const f32 dt) {
	if (!controller || !controller->input || !controller->window || !controller->camera) {
		return;
	}

	if (!controller->enabled) {
		se_camera_controller_set_cursor_capture(controller, false);
		return;
	}

	const u16 action_look_x = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_LOOK_X);
	const u16 action_look_y = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_LOOK_Y);
	const u16 action_move_forward = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_FORWARD);
	const u16 action_move_backward = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_BACKWARD);
	const u16 action_move_right = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_RIGHT);
	const u16 action_move_left = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_LEFT);
	const u16 action_move_up = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_UP);
	const u16 action_move_down = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_MOVE_DOWN);
	const u16 action_speed = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_SPEED);
	const u16 action_orbit = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_ORBIT);
	const u16 action_pan = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_PAN);
	const u16 action_dolly = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_DOLLY);
	const u16 action_fly = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_FLY);
	const u16 action_focus = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_FOCUS);
	const u16 action_zoom = se_camera_controller_action_id(controller, SE_CAMERA_CONTROLLER_ACTION_ZOOM);

	f32 look_x = se_input_get_action_value(controller->input, action_look_x);
	f32 look_y = se_input_get_action_value(controller->input, action_look_y);
	if (controller->invert_y) {
		look_y = -look_y;
	}

	if (controller->look_toggle && se_input_is_action_pressed(controller->input, action_fly)) {
		controller->fly_toggle_active = !controller->fly_toggle_active;
	}
	const b8 fly_active = controller->look_toggle ? controller->fly_toggle_active : se_input_is_action_down(controller->input, action_fly);
	const b8 orbit_down = se_input_is_action_down(controller->input, action_orbit);
	const b8 pan_down = se_input_is_action_down(controller->input, action_pan);
	const b8 dolly_down = se_input_is_action_down(controller->input, action_dolly);
	const b8 orbit_active = orbit_down || pan_down || dolly_down;

	se_camera_controller_set_cursor_capture(controller, fly_active || orbit_active);

	s_vec3 forward = se_camera_controller_forward_from_angles(controller->yaw, controller->pitch);
	if (orbit_active && !controller->was_orbit_active) {
		s_vec3 offset = s_vec3_muls(&forward, controller->focus_distance);
		controller->orbit_target = s_vec3_add(&controller->camera->position, &offset);
	}

	if (se_input_is_action_pressed(controller->input, action_focus)) {
		se_camera_controller_focus_bounds(controller);
	}

	if (fly_active || orbit_down) {
		controller->yaw += look_x * controller->mouse_x_speed;
		controller->pitch -= look_y * controller->mouse_y_speed;
		controller->pitch = se_camera_controller_clampf(controller->pitch, controller->min_pitch, controller->max_pitch);
	}

	forward = se_camera_controller_forward_from_angles(controller->yaw, controller->pitch);
	s_vec3 world_up = s_vec3(0.0f, 1.0f, 0.0f);
	s_vec3 right = s_vec3_cross(&forward, &world_up);
	if (s_vec3_length(&right) < 0.001f) {
		right = s_vec3(1.0f, 0.0f, 0.0f);
	}
	right = s_vec3_normalize(&right);
	s_vec3 view_up = s_vec3_cross(&right, &forward);
	view_up = s_vec3_normalize(&view_up);

	f32 dolly_value = se_input_get_action_value(controller->input, action_zoom);
	if (dolly_down) {
		dolly_value += look_y;
	}
	if (fabsf(dolly_value) > 0.0001f) {
		const f32 dolly_speed = fmaxf(controller->mouse_y_speed, 0.0001f) * 2.0f;
		controller->focus_distance *= expf(dolly_value * dolly_speed);
		se_camera_controller_clamp_focus_distance(controller);
	}

	if (pan_down) {
		const f32 pan_distance_scale = fmaxf(controller->scene_radius * 0.25f, controller->focus_distance * 0.5f);
		s_vec3 pan_right = s_vec3_muls(&right, -look_x * controller->mouse_x_speed * pan_distance_scale);
		s_vec3 pan_up = s_vec3_muls(&view_up, look_y * controller->mouse_y_speed * pan_distance_scale);
		s_vec3 pan = s_vec3_add(&pan_right, &pan_up);
		controller->orbit_target = s_vec3_add(&controller->orbit_target, &pan);
	}

	if (orbit_active) {
		s_vec3 offset = s_vec3_muls(&forward, -controller->focus_distance);
		controller->camera->position = s_vec3_add(&controller->orbit_target, &offset);
		controller->camera->target = controller->orbit_target;
	}

	s_vec3 move = s_vec3(0.0f, 0.0f, 0.0f);
	if (fly_active) {
		const f32 move_forward = se_input_get_action_value(controller->input, action_move_forward) - se_input_get_action_value(controller->input, action_move_backward);
		const f32 move_right = se_input_get_action_value(controller->input, action_move_right) - se_input_get_action_value(controller->input, action_move_left);
		const f32 move_up = se_input_get_action_value(controller->input, action_move_up) - se_input_get_action_value(controller->input, action_move_down);

		s_vec3 move_forward_vec = s_vec3_muls(&forward, move_forward);
		s_vec3 move_right_vec = s_vec3_muls(&right, move_right);
		s_vec3 move_up_vec = s_vec3_muls(&world_up, move_up);
		move = s_vec3_add(&move_forward_vec, &move_right_vec);
		move = s_vec3_add(&move, &move_up_vec);

		if (s_vec3_length(&move) > 0.0f) {
			move = s_vec3_normalize(&move);
			const b8 speed_active = se_input_is_action_down(controller->input, action_speed);
			const f32 speed = speed_active ? (controller->movement_speed * SE_CAMERA_CONTROLLER_FAST_MULTIPLIER) : controller->movement_speed;
			move = s_vec3_muls(&move, speed * dt);
			controller->camera->position = s_vec3_add(&controller->camera->position, &move);
		}
		controller->camera->target = s_vec3_add(&controller->camera->position, &forward);
		s_vec3 focus_offset = s_vec3_muls(&forward, controller->focus_distance);
		controller->orbit_target = s_vec3_add(&controller->camera->position, &focus_offset);
	} else if (!orbit_active) {
		controller->camera->target = s_vec3_add(&controller->camera->position, &forward);
	}

	controller->camera->up = world_up;
	controller->was_orbit_active = orbit_active;
}

void se_camera_controller_set_enabled(se_camera_controller* controller, const b8 enabled) {
	if (!controller) {
		return;
	}
	controller->enabled = enabled;
	if (controller->input && controller->context_id != SE_INPUT_CONTEXT_INVALID) {
		se_input_set_context_active(controller->input, controller->context_id, enabled);
	}
	if (!enabled) {
		se_camera_controller_set_cursor_capture(controller, false);
	}
}

b8 se_camera_controller_is_enabled(const se_camera_controller* controller) {
	if (!controller) {
		return false;
	}
	return controller->enabled;
}

void se_camera_controller_set_invert_y(se_camera_controller* controller, const b8 invert_y) {
	if (!controller) {
		return;
	}
	controller->invert_y = invert_y;
}

b8 se_camera_controller_get_invert_y(const se_camera_controller* controller) {
	if (!controller) {
		return false;
	}
	return controller->invert_y;
}

void se_camera_controller_set_look_toggle(se_camera_controller* controller, const b8 look_toggle) {
	if (!controller) {
		return;
	}
	controller->look_toggle = look_toggle;
	if (!look_toggle) {
		controller->fly_toggle_active = false;
	}
}

b8 se_camera_controller_get_look_toggle(const se_camera_controller* controller) {
	if (!controller) {
		return false;
	}
	return controller->look_toggle;
}

void se_camera_controller_set_speeds(se_camera_controller* controller, const f32 movement_speed, const f32 mouse_x_speed, const f32 mouse_y_speed) {
	if (!controller) {
		return;
	}
	if (movement_speed > 0.0f) {
		controller->movement_speed = movement_speed;
	}
	if (mouse_x_speed > 0.0f) {
		controller->mouse_x_speed = mouse_x_speed;
	}
	if (mouse_y_speed > 0.0f) {
		controller->mouse_y_speed = mouse_y_speed;
	}
}

void se_camera_controller_set_scene_bounds(se_camera_controller* controller, const s_vec3* center, const f32 radius) {
	if (!controller || !center) {
		return;
	}
	controller->scene_center = *center;
	controller->scene_radius = radius;
	if (controller->scene_radius < SE_CAMERA_CONTROLLER_MIN_RADIUS) {
		controller->scene_radius = SE_CAMERA_CONTROLLER_MIN_RADIUS;
	}
	se_camera_controller_update_focus_limits_from_radius(controller);
	se_camera_controller_clamp_focus_distance(controller);
}

void se_camera_controller_set_focus_limits(se_camera_controller* controller, const f32 min_distance, const f32 max_distance) {
	if (!controller || min_distance <= 0.0f || max_distance <= min_distance) {
		return;
	}
	controller->min_focus_distance = min_distance;
	controller->max_focus_distance = max_distance;
	controller->use_custom_focus_limits = true;
	se_camera_controller_clamp_focus_distance(controller);
}

void se_camera_controller_focus_bounds(se_camera_controller* controller) {
	if (!controller || !controller->camera) {
		return;
	}
	controller->orbit_target = controller->scene_center;
	controller->focus_distance = controller->scene_radius * 1.5f;
	se_camera_controller_clamp_focus_distance(controller);
	s_vec3 forward = se_camera_controller_forward_from_angles(controller->yaw, controller->pitch);
	s_vec3 offset = s_vec3_muls(&forward, -controller->focus_distance);
	controller->camera->position = s_vec3_add(&controller->orbit_target, &offset);
	controller->camera->target = controller->orbit_target;
}

b8 se_camera_controller_set_preset(se_camera_controller* controller, const se_camera_controller_preset preset) {
	if (!controller || !controller->input || controller->context_id == SE_INPUT_CONTEXT_INVALID) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	b8 ok = true;
	ok &= se_input_context_clear_bindings(controller->input, controller->context_id);
	ok &= se_camera_controller_bind_common_actions(controller);

	if (preset == SE_CAMERA_CONTROLLER_PRESET_UE) {
		ok &= se_camera_controller_bind_ue(controller);
	} else if (preset == SE_CAMERA_CONTROLLER_PRESET_BLENDER) {
		ok &= se_camera_controller_bind_blender(controller);
	} else if (preset == SE_CAMERA_CONTROLLER_PRESET_TRACKPAD) {
		ok &= se_camera_controller_bind_trackpad(controller);
	} else {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	if (!ok) {
		return false;
	}

	controller->preset = preset;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

se_camera_controller_preset se_camera_controller_get_preset(const se_camera_controller* controller) {
	if (!controller) {
		return SE_CAMERA_CONTROLLER_PRESET_UE;
	}
	return controller->preset;
}

se_input_context_id se_camera_controller_get_input_context(const se_camera_controller* controller) {
	if (!controller) {
		return SE_INPUT_CONTEXT_INVALID;
	}
	return controller->context_id;
}
