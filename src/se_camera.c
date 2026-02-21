// Syphax Engine - Ougi Washi

#include "se_camera.h"
#include "se_debug.h"
#include "se_defines.h"
#include "se_window.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SE_CAMERA_EPSILON 0.00001f

static se_camera* se_camera_from_handle(se_context *ctx, const se_camera_handle camera) {
	return s_array_get(&ctx->cameras, camera);
}

static s_vec4 se_camera_mul_mat4_vec4(const s_mat4 *m, const s_vec4 *v) {
	return s_vec4(
		m->m[0][0] * v->x + m->m[1][0] * v->y + m->m[2][0] * v->z + m->m[3][0] * v->w,
		m->m[0][1] * v->x + m->m[1][1] * v->y + m->m[2][1] * v->z + m->m[3][1] * v->w,
		m->m[0][2] * v->x + m->m[1][2] * v->y + m->m[2][2] * v->z + m->m[3][2] * v->w,
		m->m[0][3] * v->x + m->m[1][3] * v->y + m->m[2][3] * v->z + m->m[3][3] * v->w);
}

static se_camera* se_camera_require(const se_camera_handle camera) {
	se_context *ctx = se_current_context();
	if (!ctx || camera == S_HANDLE_NULL) {
		return NULL;
	}
	return se_camera_from_handle(ctx, camera);
}

se_camera_handle se_camera_create(void) {
	se_context *ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->cameras) == 0) {
		s_array_init(&ctx->cameras);
	}
	se_camera_handle camera_handle = s_array_increment(&ctx->cameras);
	se_camera *camera = s_array_get(&ctx->cameras, camera_handle);
	memset(camera, 0, sizeof(*camera));
	camera->position = (s_vec3){0, 0, 5};
	camera->target = (s_vec3){0, 0, 0};
	camera->up = (s_vec3){0, 1, 0};
	camera->right = (s_vec3){1, 0, 0};
	camera->fov = 45.0f;
	camera->near = 0.1f;
	camera->far = 100.0f;
	camera->aspect = 1.78;
	camera->ortho_height = 10.0f;
	camera->use_orthographic = false;
	se_set_last_error(SE_RESULT_OK);
	return camera_handle;
}

void se_camera_destroy(const se_camera_handle camera) {
	se_context *ctx = se_current_context();
	s_assertf(ctx, "se_camera_destroy :: ctx is null");
	s_array_remove(&ctx->cameras, camera);
}

se_camera *se_camera_get(const se_camera_handle camera) {
	se_context *ctx = se_current_context();
	if (!ctx || camera == S_HANDLE_NULL) {
		return NULL;
	}
	return se_camera_from_handle(ctx, camera);
}

s_mat4 se_camera_get_view_matrix(const se_camera_handle camera) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_mat4_identity;
	}
	return s_mat4_look_at(&camera_ptr->position, &camera_ptr->target, &camera_ptr->up);
}

s_mat4 se_camera_get_projection_matrix(const se_camera_handle camera) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_mat4_identity;
	}
	if (camera_ptr->use_orthographic) {
		const f32 half_height = s_max(camera_ptr->ortho_height * 0.5f, SE_CAMERA_EPSILON);
		const f32 half_width = half_height * s_max(camera_ptr->aspect, SE_CAMERA_EPSILON);
		return s_mat4_ortho(-half_width, half_width, -half_height, half_height, camera_ptr->near, camera_ptr->far);
	}
	return s_mat4_perspective(camera_ptr->fov * (PI / 180.0f), camera_ptr->aspect, camera_ptr->near, camera_ptr->far);
}

s_mat4 se_camera_get_view_projection_matrix(const se_camera_handle camera) {
	const s_mat4 view = se_camera_get_view_matrix(camera);
	const s_mat4 projection = se_camera_get_projection_matrix(camera);
	return s_mat4_mul(&projection, &view);
}

s_vec3 se_camera_get_forward_vector(const se_camera_handle camera) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_vec3(0.0f, 0.0f, -1.0f);
	}
	s_vec3 forward = s_vec3_sub(&camera_ptr->target, &camera_ptr->position);
	const f32 length = s_vec3_length(&forward);
	if (length <= SE_CAMERA_EPSILON) {
		return s_vec3(0.0f, 0.0f, -1.0f);
	}
	return s_vec3_divs(&forward, length);
}

s_vec3 se_camera_get_right_vector(const se_camera_handle camera) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_vec3(1.0f, 0.0f, 0.0f);
	}
	const s_vec3 forward = se_camera_get_forward_vector(camera);
	s_vec3 right = s_vec3_cross(&forward, &camera_ptr->up);
	const f32 length = s_vec3_length(&right);
	if (length <= SE_CAMERA_EPSILON) {
		return s_vec3(1.0f, 0.0f, 0.0f);
	}
	return s_vec3_divs(&right, length);
}

s_vec3 se_camera_get_up_vector(const se_camera_handle camera) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_vec3(0.0f, 1.0f, 0.0f);
	}
	const s_vec3 forward = se_camera_get_forward_vector(camera);
	const s_vec3 right = se_camera_get_right_vector(camera);
	s_vec3 up = s_vec3_cross(&right, &forward);
	const f32 length = s_vec3_length(&up);
	if (length <= SE_CAMERA_EPSILON) {
		const f32 up_length = s_vec3_length(&camera_ptr->up);
		if (up_length <= SE_CAMERA_EPSILON) {
			return s_vec3(0.0f, 1.0f, 0.0f);
		}
		return s_vec3_divs(&camera_ptr->up, up_length);
	}
	return s_vec3_divs(&up, length);
}

void se_camera_set_location(const se_camera_handle camera, const s_vec3* location) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !location) {
		return;
	}
	camera_ptr->position = *location;
}

void se_camera_set_target(const se_camera_handle camera, const s_vec3* target) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !target) {
		return;
	}
	camera_ptr->target = *target;
}

void se_camera_set_perspective(const se_camera_handle camera, const f32 fov_degrees, const f32 near_plane, const f32 far_plane) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || near_plane <= 0.0f || far_plane <= near_plane || fov_degrees <= 1.0f || fov_degrees >= 179.0f) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CAMERA,
			"se_camera_set_perspective :: invalid params fov=%.2f near=%.3f far=%.3f",
			fov_degrees,
			near_plane,
			far_plane);
		return;
	}
	camera_ptr->fov = fov_degrees;
	camera_ptr->near = near_plane;
	camera_ptr->far = far_plane;
	camera_ptr->use_orthographic = false;
}

void se_camera_set_orthographic(const se_camera_handle camera, const f32 ortho_height, const f32 near_plane, const f32 far_plane) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || near_plane <= 0.0f || far_plane <= near_plane || ortho_height <= SE_CAMERA_EPSILON) {
		se_debug_log(
			SE_DEBUG_LEVEL_WARN,
			SE_DEBUG_CATEGORY_CAMERA,
			"se_camera_set_orthographic :: invalid params height=%.2f near=%.3f far=%.3f",
			ortho_height,
			near_plane,
			far_plane);
		return;
	}
	camera_ptr->ortho_height = ortho_height;
	camera_ptr->near = near_plane;
	camera_ptr->far = far_plane;
	camera_ptr->use_orthographic = true;
}

void se_camera_set_aspect(const se_camera_handle camera, const f32 width, const f32 height) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || height == 0.0f) {
		return;
	}
	camera_ptr->aspect = width / height;
}

void se_camera_set_aspect_from_window(const se_camera_handle camera, const se_window_handle window) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || window == S_HANDLE_NULL) {
		return;
	}
	const f32 aspect = se_window_get_aspect(window);
	if (aspect <= 0.0f) {
		return;
	}
	camera_ptr->aspect = aspect;
}

void se_camera_set_orbit_defaults(const se_camera_handle camera, const se_window_handle window, const s_vec3* pivot, const f32 distance) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return;
	}
	const s_vec3 target = pivot ? *pivot : s_vec3(0.0f, 0.0f, 0.0f);
	const f32 safe_distance = s_max(distance, 0.1f);
	camera_ptr->position = s_vec3(
		target.x + safe_distance * 0.75f,
		target.y + safe_distance * 0.45f,
		target.z + safe_distance);
	camera_ptr->target = target;
	camera_ptr->up = s_vec3(0.0f, 1.0f, 0.0f);
	se_camera_set_perspective(camera, 52.0f, 0.05f, 200.0f);
	if (window != S_HANDLE_NULL) {
		se_camera_set_aspect_from_window(camera, window);
	}
}

void se_camera_orbit(const se_camera_handle camera, const s_vec3 *pivot, const f32 yaw_delta, const f32 pitch_delta, const f32 min_pitch, const f32 max_pitch) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !pivot) {
		return;
	}

	s_vec3 to_camera = s_vec3_sub(&camera_ptr->position, pivot);
	f32 distance = s_vec3_length(&to_camera);
	if (distance <= SE_CAMERA_EPSILON) {
		distance = 1.0f;
		to_camera = s_vec3(0.0f, 0.0f, 1.0f);
	}
	const f32 yaw = atan2f(to_camera.x, to_camera.z) + yaw_delta;
	const f32 pitch_input = s_max(-1.0f, s_min(1.0f, to_camera.y / distance));
	f32 pitch = asinf(pitch_input) + pitch_delta;
	pitch = s_max(min_pitch, s_min(max_pitch, pitch));

	s_vec3 new_offset = s_vec3(
		sinf(yaw) * cosf(pitch) * distance,
		sinf(pitch) * distance,
		cosf(yaw) * cosf(pitch) * distance);
	camera_ptr->position = s_vec3_add(pivot, &new_offset);
	camera_ptr->target = *pivot;
}

void se_camera_pan_world(const se_camera_handle camera, const s_vec3 *delta) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !delta) {
		return;
	}
	camera_ptr->position = s_vec3_add(&camera_ptr->position, delta);
	camera_ptr->target = s_vec3_add(&camera_ptr->target, delta);
}

void se_camera_pan_local(const se_camera_handle camera, const f32 right_delta, const f32 up_delta, const f32 forward_delta) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return;
	}
	const s_vec3 right = se_camera_get_right_vector(camera);
	const s_vec3 up = se_camera_get_up_vector(camera);
	const s_vec3 forward = se_camera_get_forward_vector(camera);
	s_vec3 delta = s_vec3(0.0f, 0.0f, 0.0f);
	delta = s_vec3_add(&delta, &s_vec3_muls(&right, right_delta));
	delta = s_vec3_add(&delta, &s_vec3_muls(&up, up_delta));
	delta = s_vec3_add(&delta, &s_vec3_muls(&forward, forward_delta));
	se_camera_pan_world(camera, &delta);
}

void se_camera_dolly(const se_camera_handle camera, const s_vec3 *pivot, const f32 distance_delta, const f32 min_distance, const f32 max_distance) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !pivot) {
		return;
	}
	s_vec3 from_pivot = s_vec3_sub(&camera_ptr->position, pivot);
	f32 distance = s_vec3_length(&from_pivot);
	if (distance <= SE_CAMERA_EPSILON) {
		return;
	}
	distance += distance_delta;
	distance = s_max(min_distance, s_min(max_distance, distance));
	const s_vec3 dir = s_vec3_divs(&from_pivot, s_vec3_length(&from_pivot));
	camera_ptr->position = s_vec3_add(pivot, &s_vec3_muls(&dir, distance));
	camera_ptr->target = *pivot;
}

void se_camera_clamp_target(const se_camera_handle camera, const s_vec3 *min_bounds, const s_vec3 *max_bounds) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !min_bounds || !max_bounds) {
		return;
	}
	s_vec3 clamped = camera_ptr->target;
	clamped.x = s_max(min_bounds->x, s_min(max_bounds->x, clamped.x));
	clamped.y = s_max(min_bounds->y, s_min(max_bounds->y, clamped.y));
	clamped.z = s_max(min_bounds->z, s_min(max_bounds->z, clamped.z));
	const s_vec3 delta = s_vec3_sub(&clamped, &camera_ptr->target);
	camera_ptr->target = clamped;
	camera_ptr->position = s_vec3_add(&camera_ptr->position, &delta);
}

void se_camera_smooth_target(const se_camera_handle camera, const s_vec3 *target, const f32 smoothing, const f32 dt) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !target) {
		return;
	}
	const f32 t = 1.0f - expf(-s_max(smoothing, 0.0f) * s_max(dt, 0.0f));
	camera_ptr->target = s_vec3_lerp(&camera_ptr->target, target, t);
}

void se_camera_smooth_position(const se_camera_handle camera, const s_vec3 *position, const f32 smoothing, const f32 dt) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !position) {
		return;
	}
	const f32 t = 1.0f - expf(-s_max(smoothing, 0.0f) * s_max(dt, 0.0f));
	camera_ptr->position = s_vec3_lerp(&camera_ptr->position, position, t);
}

b8 se_camera_world_to_ndc(const se_camera_handle camera, const s_vec3 *world, s_vec3 *out_ndc) {
	if (!world || !out_ndc) {
		return false;
	}

	const s_mat4 view_projection = se_camera_get_view_projection_matrix(camera);
	const s_vec4 world4 = s_vec4(world->x, world->y, world->z, 1.0f);
	const s_vec4 clip = se_camera_mul_mat4_vec4(&view_projection, &world4);
	if (fabsf(clip.w) <= SE_CAMERA_EPSILON) {
		return false;
	}

	const f32 inv_w = 1.0f / clip.w;
	*out_ndc = s_vec3(clip.x * inv_w, clip.y * inv_w, clip.z * inv_w);
	return clip.w > 0.0f;
}

b8 se_camera_world_to_screen(const se_camera_handle camera, const s_vec3 *world, const f32 viewport_width, const f32 viewport_height, s_vec2 *out_screen) {
	if (!out_screen || viewport_width <= 1.0f || viewport_height <= 1.0f) {
		return false;
	}

	s_vec3 ndc = s_vec3(0.0f, 0.0f, 0.0f);
	if (!se_camera_world_to_ndc(camera, world, &ndc)) {
		return false;
	}
	if (ndc.z < -1.0f || ndc.z > 1.0f) {
		return false;
	}

	out_screen->x = (ndc.x * 0.5f + 0.5f) * viewport_width;
	out_screen->y = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewport_height;
	return true;
}

b8 se_camera_screen_to_ray(const se_camera_handle camera, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, s_vec3 *out_origin, s_vec3 *out_direction) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !out_origin || !out_direction || viewport_width <= 1.0f || viewport_height <= 1.0f) {
		return false;
	}

	const f32 ndc_x = (screen_x / viewport_width) * 2.0f - 1.0f;
	const f32 ndc_y = 1.0f - (screen_y / viewport_height) * 2.0f;
	const s_vec3 forward = se_camera_get_forward_vector(camera);
	const s_vec3 right = se_camera_get_right_vector(camera);
	const s_vec3 up = se_camera_get_up_vector(camera);
	const f32 tan_half_fov = tanf(camera_ptr->fov * (PI / 180.0f) * 0.5f);

	s_vec3 direction = forward;
	direction = s_vec3_add(&direction, &s_vec3_muls(&right, ndc_x * tan_half_fov * camera_ptr->aspect));
	direction = s_vec3_add(&direction, &s_vec3_muls(&up, ndc_y * tan_half_fov));

	const f32 direction_length = s_vec3_length(&direction);
	if (direction_length <= SE_CAMERA_EPSILON) {
		return false;
	}

	*out_origin = camera_ptr->position;
	*out_direction = s_vec3_divs(&direction, direction_length);
	return true;
}

b8 se_camera_screen_to_plane(
	const se_camera_handle camera,
	const f32 screen_x,
	const f32 screen_y,
	const f32 viewport_width,
	const f32 viewport_height,
	const s_vec3 *plane_point,
	const s_vec3 *plane_normal,
	s_vec3 *out_world) {
	if (!plane_point || !plane_normal || !out_world) {
		return false;
	}

	s_vec3 origin = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 direction = s_vec3(0.0f, 0.0f, 0.0f);
	if (!se_camera_screen_to_ray(camera, screen_x, screen_y, viewport_width, viewport_height, &origin, &direction)) {
		return false;
	}

	s_vec3 normal = s_vec3_normalize(plane_normal);
	const f32 denominator = s_vec3_dot(&direction, &normal);
	if (fabsf(denominator) <= SE_CAMERA_EPSILON) {
		return false;
	}

	const s_vec3 to_plane = s_vec3_sub(plane_point, &origin);
	const f32 t = s_vec3_dot(&to_plane, &normal) / denominator;
	if (t < 0.0f) {
		return false;
	}

	*out_world = s_vec3_add(&origin, &s_vec3_muls(&direction, t));
	return true;
}

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
