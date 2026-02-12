// Syphax-Engine - Ougi Washi

#include "se_camera_controller.h"

#include <math.h>
#include <stdlib.h>

#define SE_CAMERA_CONTROLLER_MIN_RADIUS 1.0f
#define SE_CAMERA_CONTROLLER_MIN_DISTANCE 0.1f
#define SE_CAMERA_CONTROLLER_FAST_MULTIPLIER 3.0f

typedef struct {
	b8 fly : 1;
	b8 orbit : 1;
	b8 pan : 1;
	b8 dolly : 1;
} se_camera_controller_modes;

struct se_camera_controller {
	se_window_handle window;
	se_camera_handle camera;
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

static se_camera* se_camera_controller_get_camera(se_camera_controller* controller) {
	if (!controller || controller->camera == S_HANDLE_NULL) {
		return NULL;
	}
	se_context *ctx = se_current_context();
	if (!ctx) {
		return NULL;
	}
	return s_array_get(&ctx->cameras, controller->camera);
}

static void se_camera_controller_sync_from_camera(se_camera_controller* controller) {
	se_camera *camera = se_camera_controller_get_camera(controller);
	if (!camera) {
		return;
	}

	s_vec3 to_target = s_vec3_sub(&camera->target, &camera->position);
	f32 distance = s_vec3_length(&to_target);
	if (distance <= 0.0001f) {
		to_target = s_vec3(0.0f, 0.0f, -1.0f);
		distance = 1.0f;
	}

	s_vec3 forward = s_vec3_normalize(&to_target);
	controller->yaw = atan2f(forward.x, forward.z);
	controller->pitch = asinf(forward.y);
	controller->focus_distance = distance;
	controller->orbit_target = camera->target;
	controller->scene_center = camera->target;
	controller->scene_radius = distance;
	if (controller->scene_radius < SE_CAMERA_CONTROLLER_MIN_RADIUS) {
		controller->scene_radius = SE_CAMERA_CONTROLLER_MIN_RADIUS;
	}

	se_camera_controller_update_focus_limits_from_radius(controller);
	se_camera_controller_clamp_focus_distance(controller);
}

static void se_camera_controller_set_cursor_capture(se_camera_controller* controller, const b8 capture) {
	if (!controller || controller->window == S_HANDLE_NULL || !controller->lock_cursor_while_active) {
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

static se_camera_controller_modes se_camera_controller_get_modes(se_camera_controller* controller) {
	se_camera_controller_modes modes = {0};
	if (!controller || controller->window == S_HANDLE_NULL) {
		return modes;
	}

	const b8 shift = se_window_is_key_down(controller->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(controller->window, SE_KEY_RIGHT_SHIFT);
	const b8 ctrl = se_window_is_key_down(controller->window, SE_KEY_LEFT_CONTROL) || se_window_is_key_down(controller->window, SE_KEY_RIGHT_CONTROL);
	const b8 alt = se_window_is_key_down(controller->window, SE_KEY_LEFT_ALT) || se_window_is_key_down(controller->window, SE_KEY_RIGHT_ALT);
	const b8 lmb = se_window_is_mouse_down(controller->window, SE_MOUSE_LEFT);
	const b8 mmb = se_window_is_mouse_down(controller->window, SE_MOUSE_MIDDLE);
	const b8 rmb = se_window_is_mouse_down(controller->window, SE_MOUSE_RIGHT);

	b8 hold_fly = false;
	if (controller->preset == SE_CAMERA_CONTROLLER_PRESET_UE) {
		hold_fly = rmb && !alt;
		modes.orbit = alt && lmb;
		modes.pan = alt && mmb;
		modes.dolly = alt && rmb;
	} else if (controller->preset == SE_CAMERA_CONTROLLER_PRESET_BLENDER) {
		hold_fly = rmb;
		modes.orbit = mmb && !shift && !ctrl;
		modes.pan = mmb && shift;
		modes.dolly = mmb && ctrl;
	} else {
		hold_fly = rmb && !alt;
		modes.orbit = alt && lmb && !shift;
		modes.pan = alt && shift && lmb;
		modes.dolly = alt && rmb;
	}

	if (controller->look_toggle && se_window_is_mouse_pressed(controller->window, SE_MOUSE_RIGHT)) {
		controller->fly_toggle_active = !controller->fly_toggle_active;
	}
	modes.fly = controller->look_toggle ? controller->fly_toggle_active : hold_fly;
	return modes;
}

se_camera_controller* se_camera_controller_create(const se_camera_controller_params* params) {
	if (!params || params->window == S_HANDLE_NULL || params->camera == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	se_camera_controller* controller = calloc(1, sizeof(*controller));
	if (!controller) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	controller->window = params->window;
	controller->camera = params->camera;
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

	if (params->min_focus_distance > 0.0f && params->max_focus_distance > params->min_focus_distance) {
		controller->min_focus_distance = params->min_focus_distance;
		controller->max_focus_distance = params->max_focus_distance;
		controller->use_custom_focus_limits = true;
	}

	se_camera_controller_sync_from_camera(controller);
	if (!se_camera_controller_set_preset(controller, params->preset)) {
		free(controller);
		return NULL;
	}

	se_set_last_error(SE_RESULT_OK);
	return controller;
}

void se_camera_controller_destroy(se_camera_controller* controller) {
	if (!controller) {
		return;
	}
	se_camera_controller_set_cursor_capture(controller, false);
	free(controller);
}

void se_camera_controller_tick(se_camera_controller* controller, const f32 dt) {
	se_camera *camera = se_camera_controller_get_camera(controller);
	if (!controller || controller->window == S_HANDLE_NULL || !camera) {
		return;
	}

	if (!controller->enabled) {
		se_camera_controller_set_cursor_capture(controller, false);
		return;
	}

	const se_camera_controller_modes modes = se_camera_controller_get_modes(controller);
	const b8 orbit_active = modes.orbit || modes.pan || modes.dolly;

	se_camera_controller_set_cursor_capture(controller, modes.fly || orbit_active);

	s_vec2 mouse_delta = s_vec2(0.0f, 0.0f);
	s_vec2 scroll_delta = s_vec2(0.0f, 0.0f);
	se_window_get_mouse_delta(controller->window, &mouse_delta);
	se_window_get_scroll_delta(controller->window, &scroll_delta);

	s_vec3 forward = se_camera_controller_forward_from_angles(controller->yaw, controller->pitch);
	if (orbit_active && !controller->was_orbit_active) {
		s_vec3 offset = s_vec3_muls(&forward, controller->focus_distance);
		controller->orbit_target = s_vec3_add(&camera->position, &offset);
	}

	if (se_window_is_key_pressed(controller->window, SE_KEY_F)) {
		se_camera_controller_focus_bounds(controller);
	}

	if (modes.fly || modes.orbit) {
		controller->yaw += mouse_delta.x * controller->mouse_x_speed;
		const f32 pitch_delta = controller->invert_y ? (mouse_delta.y * controller->mouse_y_speed) : (-mouse_delta.y * controller->mouse_y_speed);
		controller->pitch += pitch_delta;
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

	f32 dolly_value = -scroll_delta.y * 80.0f;
	if (modes.dolly) {
		dolly_value += mouse_delta.y;
	}
	if (fabsf(dolly_value) > 0.0001f) {
		const f32 dolly_speed = fmaxf(controller->mouse_y_speed, 0.0001f) * 2.0f;
		controller->focus_distance *= expf(dolly_value * dolly_speed);
		se_camera_controller_clamp_focus_distance(controller);
	}

	if (modes.pan) {
		const f32 pan_distance_scale = fmaxf(controller->scene_radius * 0.25f, controller->focus_distance * 0.5f);
		s_vec3 pan_right = s_vec3_muls(&right, -mouse_delta.x * controller->mouse_x_speed * pan_distance_scale);
		s_vec3 pan_up = s_vec3_muls(&view_up, mouse_delta.y * controller->mouse_y_speed * pan_distance_scale);
		s_vec3 pan = s_vec3_add(&pan_right, &pan_up);
		controller->orbit_target = s_vec3_add(&controller->orbit_target, &pan);
	}

	if (orbit_active) {
		s_vec3 offset = s_vec3_muls(&forward, -controller->focus_distance);
		camera->position = s_vec3_add(&controller->orbit_target, &offset);
		camera->target = controller->orbit_target;
	}

	if (modes.fly) {
		s_vec3 move = s_vec3(0.0f, 0.0f, 0.0f);
		if (se_window_is_key_down(controller->window, SE_KEY_W)) move = s_vec3_add(&move, &forward);
		if (se_window_is_key_down(controller->window, SE_KEY_S)) {
			s_vec3 backward = s_vec3_muls(&forward, -1.0f);
			move = s_vec3_add(&move, &backward);
		}
		if (se_window_is_key_down(controller->window, SE_KEY_D)) move = s_vec3_add(&move, &right);
		if (se_window_is_key_down(controller->window, SE_KEY_A)) {
			s_vec3 left = s_vec3_muls(&right, -1.0f);
			move = s_vec3_add(&move, &left);
		}
		if (se_window_is_key_down(controller->window, SE_KEY_E)) move = s_vec3_add(&move, &world_up);
		if (se_window_is_key_down(controller->window, SE_KEY_Q)) {
			s_vec3 down = s_vec3_muls(&world_up, -1.0f);
			move = s_vec3_add(&move, &down);
		}

		if (s_vec3_length(&move) > 0.0f) {
			move = s_vec3_normalize(&move);
			const b8 fast = se_window_is_key_down(controller->window, SE_KEY_LEFT_SHIFT) || se_window_is_key_down(controller->window, SE_KEY_RIGHT_SHIFT);
			const f32 speed = fast ? (controller->movement_speed * SE_CAMERA_CONTROLLER_FAST_MULTIPLIER) : controller->movement_speed;
			move = s_vec3_muls(&move, speed * dt);
			camera->position = s_vec3_add(&camera->position, &move);
		}

		camera->target = s_vec3_add(&camera->position, &forward);
		s_vec3 focus_offset = s_vec3_muls(&forward, controller->focus_distance);
		controller->orbit_target = s_vec3_add(&camera->position, &focus_offset);
	} else if (!orbit_active) {
		camera->target = s_vec3_add(&camera->position, &forward);
	}

	camera->up = world_up;
	controller->was_orbit_active = orbit_active;
}

void se_camera_controller_set_enabled(se_camera_controller* controller, const b8 enabled) {
	if (!controller) {
		return;
	}
	controller->enabled = enabled;
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
	se_camera *camera = se_camera_controller_get_camera(controller);
	if (!controller || !camera) {
		return;
	}
	controller->orbit_target = controller->scene_center;
	controller->focus_distance = controller->scene_radius * 1.5f;
	se_camera_controller_clamp_focus_distance(controller);
	s_vec3 forward = se_camera_controller_forward_from_angles(controller->yaw, controller->pitch);
	s_vec3 offset = s_vec3_muls(&forward, -controller->focus_distance);
	camera->position = s_vec3_add(&controller->orbit_target, &offset);
	camera->target = controller->orbit_target;
}

b8 se_camera_controller_set_preset(se_camera_controller* controller, const se_camera_controller_preset preset) {
	if (!controller) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (preset != SE_CAMERA_CONTROLLER_PRESET_UE && preset != SE_CAMERA_CONTROLLER_PRESET_BLENDER && preset != SE_CAMERA_CONTROLLER_PRESET_TRACKPAD) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
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
