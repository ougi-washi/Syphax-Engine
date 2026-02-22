// Syphax Engine - Ougi Washi

#include "se_camera.h"
#include "se_debug.h"
#include "se_defines.h"
#include "se_window.h"

#include <math.h>
#include <string.h>

#define SE_CAMERA_EPSILON 0.00001f
#define SE_CAMERA_MAX_PITCH (PI * 0.5f - 0.001f)

static se_camera* se_camera_from_handle(se_context* ctx, const se_camera_handle camera) {
	return s_array_get(&ctx->cameras, camera);
}

static s_vec4 se_camera_mul_mat4_vec4(const s_mat4* m, const s_vec4* v) {
	return s_vec4(
		m->m[0][0] * v->x + m->m[1][0] * v->y + m->m[2][0] * v->z + m->m[3][0] * v->w,
		m->m[0][1] * v->x + m->m[1][1] * v->y + m->m[2][1] * v->z + m->m[3][1] * v->w,
		m->m[0][2] * v->x + m->m[1][2] * v->y + m->m[2][2] * v->z + m->m[3][2] * v->w,
		m->m[0][3] * v->x + m->m[1][3] * v->y + m->m[2][3] * v->z + m->m[3][3] * v->w);
}

static se_camera* se_camera_require(const se_camera_handle camera) {
	se_context* ctx = se_current_context();
	if (!ctx || camera == S_HANDLE_NULL) {
		return NULL;
	}
	return se_camera_from_handle(ctx, camera);
}

static f32 se_camera_clampf(const f32 value, const f32 min_value, const f32 max_value) {
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

static f32 se_camera_wrap_angle(f32 angle) {
	const f32 two_pi = PI * 2.0f;
	while (angle > PI) {
		angle -= two_pi;
	}
	while (angle < -PI) {
		angle += two_pi;
	}
	return angle;
}

static void se_camera_mark_view_dirty(se_camera* camera_ptr) {
	if (!camera_ptr) {
		return;
	}
	camera_ptr->view_dirty = true;
	camera_ptr->view_projection_dirty = true;
}

static void se_camera_mark_projection_dirty(se_camera* camera_ptr) {
	if (!camera_ptr) {
		return;
	}
	camera_ptr->projection_dirty = true;
	camera_ptr->view_projection_dirty = true;
}

static void se_camera_basis_from_rotation(const s_vec3* rotation, s_vec3* out_forward, s_vec3* out_right, s_vec3* out_up) {
	const f32 pitch = se_camera_clampf(rotation->x, -SE_CAMERA_MAX_PITCH, SE_CAMERA_MAX_PITCH);
	const f32 yaw = rotation->y;
	const f32 roll = rotation->z;

	s_vec3 forward = s_vec3(
		sinf(yaw) * cosf(pitch),
		sinf(pitch),
		-cosf(yaw) * cosf(pitch));
	forward = s_vec3_normalize(&forward);

	const s_vec3 world_up = s_vec3(0.0f, 1.0f, 0.0f);
	s_vec3 right = s_vec3_cross(&forward, &world_up);
	if (s_vec3_length(&right) <= SE_CAMERA_EPSILON) {
		right = s_vec3(1.0f, 0.0f, 0.0f);
	}
	right = s_vec3_normalize(&right);

	s_vec3 up = s_vec3_cross(&right, &forward);
	up = s_vec3_normalize(&up);

	if (fabsf(roll) > SE_CAMERA_EPSILON) {
		const f32 c = cosf(roll);
		const f32 s = sinf(roll);
		const s_vec3 roll_right = s_vec3_add(&s_vec3_muls(&right, c), &s_vec3_muls(&up, s));
		const s_vec3 roll_up = s_vec3_sub(&s_vec3_muls(&up, c), &s_vec3_muls(&right, s));
		right = s_vec3_normalize(&roll_right);
		up = s_vec3_normalize(&roll_up);
	}

	if (out_forward) {
		*out_forward = forward;
	}
	if (out_right) {
		*out_right = right;
	}
	if (out_up) {
		*out_up = up;
	}
}

static b8 se_camera_angles_from_direction(const s_vec3* direction, f32* out_pitch, f32* out_yaw) {
	const f32 length = s_vec3_length(direction);
	if (length <= SE_CAMERA_EPSILON) {
		return false;
	}
	const s_vec3 dir = s_vec3_divs(direction, length);
	if (out_pitch) {
		*out_pitch = asinf(se_camera_clampf(dir.y, -1.0f, 1.0f));
	}
	if (out_yaw) {
		*out_yaw = atan2f(dir.x, -dir.z);
	}
	return true;
}

static s_vec3 se_camera_rotate_around_axis(const s_vec3* vector, const s_vec3* axis, const f32 radians) {
	const f32 axis_len = s_vec3_length(axis);
	if (axis_len <= SE_CAMERA_EPSILON || fabsf(radians) <= SE_CAMERA_EPSILON) {
		return *vector;
	}
	const s_vec3 unit_axis = s_vec3_divs(axis, axis_len);
	const f32 c = cosf(radians);
	const f32 s = sinf(radians);
	const s_vec3 term_a = s_vec3_muls(vector, c);
	const s_vec3 term_b = s_vec3_muls(&s_vec3_cross(&unit_axis, vector), s);
	const s_vec3 term_c = s_vec3_muls(&unit_axis, s_vec3_dot(&unit_axis, vector) * (1.0f - c));
	return s_vec3_add(&s_vec3_add(&term_a, &term_b), &term_c);
}

static s_vec3 se_camera_rotation_from_basis(const s_vec3* forward, const s_vec3* right) {
	s_vec3 out_rotation = s_vec3(0.0f, 0.0f, 0.0f);
	if (!forward || !right) {
		return out_rotation;
	}

	const s_vec3 fwd = s_vec3_normalize(forward);
	out_rotation.x = asinf(se_camera_clampf(fwd.y, -1.0f, 1.0f));
	out_rotation.y = atan2f(fwd.x, -fwd.z);

	const s_vec3 no_roll = s_vec3(out_rotation.x, out_rotation.y, 0.0f);
	s_vec3 base_forward = s_vec3(0.0f, 0.0f, -1.0f);
	s_vec3 base_right = s_vec3(1.0f, 0.0f, 0.0f);
	s_vec3 base_up = s_vec3(0.0f, 1.0f, 0.0f);
	se_camera_basis_from_rotation(&no_roll, &base_forward, &base_right, &base_up);

	const s_vec3 right_norm = s_vec3_normalize(right);
	out_rotation.z = atan2f(s_vec3_dot(&right_norm, &base_up), s_vec3_dot(&right_norm, &base_right));
	out_rotation.x = se_camera_clampf(out_rotation.x, -SE_CAMERA_MAX_PITCH, SE_CAMERA_MAX_PITCH);
	out_rotation.y = se_camera_wrap_angle(out_rotation.y);
	out_rotation.z = se_camera_wrap_angle(out_rotation.z);
	return out_rotation;
}

static void se_camera_sync_pose(se_camera* camera_ptr) {
	if (!camera_ptr) {
		return;
	}

	camera_ptr->rotation.x = se_camera_clampf(camera_ptr->rotation.x, -SE_CAMERA_MAX_PITCH, SE_CAMERA_MAX_PITCH);
	camera_ptr->rotation.y = se_camera_wrap_angle(camera_ptr->rotation.y);
	camera_ptr->rotation.z = se_camera_wrap_angle(camera_ptr->rotation.z);

	if (camera_ptr->target_mode) {
		const s_vec3 to_target = s_vec3_sub(&camera_ptr->target, &camera_ptr->position);
		const f32 distance = s_vec3_length(&to_target);
		if (distance > SE_CAMERA_EPSILON) {
			camera_ptr->target_distance = distance;
			f32 pitch = camera_ptr->rotation.x;
			f32 yaw = camera_ptr->rotation.y;
			if (se_camera_angles_from_direction(&to_target, &pitch, &yaw)) {
				camera_ptr->rotation.x = pitch;
				camera_ptr->rotation.y = yaw;
			}
		} else {
			if (camera_ptr->target_distance <= SE_CAMERA_EPSILON) {
				camera_ptr->target_distance = 1.0f;
			}
			s_vec3 forward = s_vec3(0.0f, 0.0f, -1.0f);
			se_camera_basis_from_rotation(&camera_ptr->rotation, &forward, NULL, NULL);
			camera_ptr->target = s_vec3_add(&camera_ptr->position, &s_vec3_muls(&forward, camera_ptr->target_distance));
		}
	} else {
		if (camera_ptr->target_distance <= SE_CAMERA_EPSILON) {
			const f32 distance = s_vec3_length(&s_vec3_sub(&camera_ptr->target, &camera_ptr->position));
			camera_ptr->target_distance = distance > SE_CAMERA_EPSILON ? distance : 1.0f;
		}
	}

	se_camera_basis_from_rotation(&camera_ptr->rotation, NULL, &camera_ptr->right, &camera_ptr->up);
	se_camera_mark_view_dirty(camera_ptr);
}

static void se_camera_update_view_matrix(se_camera* camera_ptr) {
	if (!camera_ptr || !camera_ptr->view_dirty) {
		return;
	}

	s_vec3 forward = s_vec3(0.0f, 0.0f, -1.0f);
	se_camera_basis_from_rotation(&camera_ptr->rotation, &forward, &camera_ptr->right, &camera_ptr->up);

	if (camera_ptr->target_mode) {
		camera_ptr->cached_view_matrix = s_mat4_look_at(&camera_ptr->position, &camera_ptr->target, &camera_ptr->up);
	} else {
		const s_vec3 look_target = s_vec3_add(&camera_ptr->position, &forward);
		camera_ptr->cached_view_matrix = s_mat4_look_at(&camera_ptr->position, &look_target, &camera_ptr->up);
	}

	camera_ptr->view_dirty = false;
	camera_ptr->view_projection_dirty = true;
}

static void se_camera_update_projection_matrix(se_camera* camera_ptr) {
	if (!camera_ptr || !camera_ptr->projection_dirty) {
		return;
	}

	if (camera_ptr->use_orthographic) {
		const f32 half_height = s_max(camera_ptr->ortho_height * 0.5f, SE_CAMERA_EPSILON);
		const f32 half_width = half_height * s_max(camera_ptr->aspect, SE_CAMERA_EPSILON);
		camera_ptr->cached_projection_matrix = s_mat4_ortho(-half_width, half_width, -half_height, half_height, camera_ptr->near, camera_ptr->far);
	} else {
		camera_ptr->cached_projection_matrix = s_mat4_perspective(
			camera_ptr->fov * (PI / 180.0f),
			s_max(camera_ptr->aspect, SE_CAMERA_EPSILON),
			camera_ptr->near,
			camera_ptr->far);
	}

	camera_ptr->projection_dirty = false;
	camera_ptr->view_projection_dirty = true;
}

static void se_camera_update_view_projection_matrix(se_camera* camera_ptr) {
	if (!camera_ptr) {
		return;
	}
	se_camera_update_view_matrix(camera_ptr);
	se_camera_update_projection_matrix(camera_ptr);
	if (!camera_ptr->view_projection_dirty) {
		return;
	}
	camera_ptr->cached_view_projection_matrix = s_mat4_mul(&camera_ptr->cached_projection_matrix, &camera_ptr->cached_view_matrix);
	camera_ptr->view_projection_dirty = false;
}

se_camera_handle se_camera_create(void) {
	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&ctx->cameras) == 0) {
		s_array_init(&ctx->cameras);
	}

	const se_camera_handle camera_handle = s_array_increment(&ctx->cameras);
	se_camera* camera = s_array_get(&ctx->cameras, camera_handle);
	memset(camera, 0, sizeof(*camera));
	camera->position = s_vec3(0.0f, 0.0f, 5.0f);
	camera->target = s_vec3(0.0f, 0.0f, 0.0f);
	camera->up = s_vec3(0.0f, 1.0f, 0.0f);
	camera->right = s_vec3(1.0f, 0.0f, 0.0f);
	camera->rotation = s_vec3(0.0f, 0.0f, 0.0f);
	camera->target_distance = 5.0f;
	camera->fov = 45.0f;
	camera->near = 0.1f;
	camera->far = 100.0f;
	camera->aspect = 1.78f;
	camera->ortho_height = 10.0f;
	camera->cached_view_matrix = s_mat4_identity;
	camera->cached_projection_matrix = s_mat4_identity;
	camera->cached_view_projection_matrix = s_mat4_identity;
	camera->use_orthographic = false;
	camera->target_mode = true;
	camera->view_dirty = true;
	camera->projection_dirty = true;
	camera->view_projection_dirty = true;
	se_camera_sync_pose(camera);

	se_set_last_error(SE_RESULT_OK);
	return camera_handle;
}

void se_camera_destroy(const se_camera_handle camera) {
	se_context* ctx = se_current_context();
	s_assertf(ctx, "se_camera_destroy :: ctx is null");
	s_array_remove(&ctx->cameras, camera);
}

se_camera* se_camera_get(const se_camera_handle camera) {
	se_context* ctx = se_current_context();
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
	se_camera_update_view_matrix(camera_ptr);
	return camera_ptr->cached_view_matrix;
}

s_mat4 se_camera_get_projection_matrix(const se_camera_handle camera) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_mat4_identity;
	}
	se_camera_update_projection_matrix(camera_ptr);
	return camera_ptr->cached_projection_matrix;
}

s_mat4 se_camera_get_view_projection_matrix(const se_camera_handle camera) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_mat4_identity;
	}
	se_camera_update_view_projection_matrix(camera_ptr);
	return camera_ptr->cached_view_projection_matrix;
}

s_vec3 se_camera_get_forward_vector(const se_camera_handle camera) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_vec3(0.0f, 0.0f, -1.0f);
	}

	if (camera_ptr->target_mode) {
		const s_vec3 to_target = s_vec3_sub(&camera_ptr->target, &camera_ptr->position);
		const f32 length = s_vec3_length(&to_target);
		if (length > SE_CAMERA_EPSILON) {
			return s_vec3_divs(&to_target, length);
		}
	}

	s_vec3 forward = s_vec3(0.0f, 0.0f, -1.0f);
	se_camera_basis_from_rotation(&camera_ptr->rotation, &forward, NULL, NULL);
	return forward;
}

s_vec3 se_camera_get_right_vector(const se_camera_handle camera) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_vec3(1.0f, 0.0f, 0.0f);
	}
	s_vec3 right = s_vec3(1.0f, 0.0f, 0.0f);
	se_camera_basis_from_rotation(&camera_ptr->rotation, NULL, &right, NULL);
	return right;
}

s_vec3 se_camera_get_up_vector(const se_camera_handle camera) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return s_vec3(0.0f, 1.0f, 0.0f);
	}
	s_vec3 up = s_vec3(0.0f, 1.0f, 0.0f);
	se_camera_basis_from_rotation(&camera_ptr->rotation, NULL, NULL, &up);
	return up;
}

void se_camera_set_rotation(const se_camera_handle camera, const s_vec3* rotation) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !rotation) {
		return;
	}
	camera_ptr->rotation = *rotation;
	se_camera_sync_pose(camera_ptr);
}

void se_camera_add_rotation(const se_camera_handle camera, const s_vec3* delta_rotation) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !delta_rotation) {
		return;
	}

	s_vec3 forward = s_vec3(0.0f, 0.0f, -1.0f);
	s_vec3 right = s_vec3(1.0f, 0.0f, 0.0f);
	s_vec3 up = s_vec3(0.0f, 1.0f, 0.0f);
	se_camera_basis_from_rotation(&camera_ptr->rotation, &forward, &right, &up);

	forward = se_camera_rotate_around_axis(&forward, &right, delta_rotation->x);
	up = se_camera_rotate_around_axis(&up, &right, delta_rotation->x);
	forward = s_vec3_normalize(&forward);
	up = s_vec3_normalize(&up);
	right = s_vec3_normalize(&s_vec3_cross(&forward, &up));
	up = s_vec3_normalize(&s_vec3_cross(&right, &forward));

	forward = se_camera_rotate_around_axis(&forward, &up, delta_rotation->y);
	right = se_camera_rotate_around_axis(&right, &up, delta_rotation->y);
	forward = s_vec3_normalize(&forward);
	right = s_vec3_normalize(&right);
	up = s_vec3_normalize(&s_vec3_cross(&right, &forward));

	right = se_camera_rotate_around_axis(&right, &forward, delta_rotation->z);
	up = se_camera_rotate_around_axis(&up, &forward, delta_rotation->z);
	right = s_vec3_normalize(&right);
	up = s_vec3_normalize(&up);
	forward = s_vec3_normalize(&s_vec3_cross(&up, &right));

	camera_ptr->rotation = se_camera_rotation_from_basis(&forward, &right);
	se_camera_sync_pose(camera_ptr);
}

b8 se_camera_get_rotation(const se_camera_handle camera, s_vec3* out_rotation) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !out_rotation) {
		return false;
	}
	*out_rotation = camera_ptr->rotation;
	return true;
}

void se_camera_set_location(const se_camera_handle camera, const s_vec3* location) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !location) {
		return;
	}
	const s_vec3 delta = s_vec3_sub(location, &camera_ptr->position);
	camera_ptr->position = *location;
	if (camera_ptr->target_mode) {
		camera_ptr->target = s_vec3_add(&camera_ptr->target, &delta);
	}
	se_camera_sync_pose(camera_ptr);
}

void se_camera_add_location(const se_camera_handle camera, const s_vec3* delta_location) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !delta_location) {
		return;
	}
	camera_ptr->position = s_vec3_add(&camera_ptr->position, delta_location);
	if (camera_ptr->target_mode) {
		camera_ptr->target = s_vec3_add(&camera_ptr->target, delta_location);
	}
	se_camera_sync_pose(camera_ptr);
}

b8 se_camera_get_location(const se_camera_handle camera, s_vec3* out_location) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !out_location) {
		return false;
	}
	*out_location = camera_ptr->position;
	return true;
}

void se_camera_set_target(const se_camera_handle camera, const s_vec3* target) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !target) {
		return;
	}
	camera_ptr->target = *target;
	const f32 distance = s_vec3_length(&s_vec3_sub(&camera_ptr->target, &camera_ptr->position));
	camera_ptr->target_distance = distance > SE_CAMERA_EPSILON ? distance : 1.0f;
	if (camera_ptr->target_mode) {
		se_camera_sync_pose(camera_ptr);
	}
}

b8 se_camera_get_target(const se_camera_handle camera, s_vec3* out_target) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !out_target) {
		return false;
	}
	*out_target = camera_ptr->target;
	return true;
}

void se_camera_set_target_mode(const se_camera_handle camera, const b8 enabled) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return;
	}
	camera_ptr->target_mode = enabled ? true : false;
	se_camera_sync_pose(camera_ptr);
}

b8 se_camera_is_target_mode(const se_camera_handle camera) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr) {
		return false;
	}
	return camera_ptr->target_mode;
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
	se_camera_mark_projection_dirty(camera_ptr);
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
	se_camera_mark_projection_dirty(camera_ptr);
}

void se_camera_set_aspect(const se_camera_handle camera, const f32 width, const f32 height) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || width <= SE_CAMERA_EPSILON || height <= SE_CAMERA_EPSILON) {
		return;
	}
	camera_ptr->aspect = width / height;
	se_camera_mark_projection_dirty(camera_ptr);
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
	se_camera_mark_projection_dirty(camera_ptr);
}

b8 se_camera_world_to_ndc(const se_camera_handle camera, const s_vec3* world, s_vec3* out_ndc) {
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

b8 se_camera_world_to_screen(const se_camera_handle camera, const s_vec3* world, const f32 viewport_width, const f32 viewport_height, s_vec2* out_screen) {
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

b8 se_camera_screen_to_ray(const se_camera_handle camera, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, s_vec3* out_origin, s_vec3* out_direction) {
	const se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || !out_origin || !out_direction || viewport_width <= 1.0f || viewport_height <= 1.0f) {
		return false;
	}

	const f32 ndc_x = (screen_x / viewport_width) * 2.0f - 1.0f;
	const f32 ndc_y = 1.0f - (screen_y / viewport_height) * 2.0f;

	const s_vec3 forward = se_camera_get_forward_vector(camera);
	const s_vec3 right = se_camera_get_right_vector(camera);
	const s_vec3 up = se_camera_get_up_vector(camera);

	if (camera_ptr->use_orthographic) {
		const f32 half_height = s_max(camera_ptr->ortho_height * 0.5f, SE_CAMERA_EPSILON);
		const f32 half_width = half_height * s_max(camera_ptr->aspect, SE_CAMERA_EPSILON);
		s_vec3 origin = camera_ptr->position;
		origin = s_vec3_add(&origin, &s_vec3_muls(&right, ndc_x * half_width));
		origin = s_vec3_add(&origin, &s_vec3_muls(&up, ndc_y * half_height));
		*out_origin = origin;
		*out_direction = forward;
		return true;
	}

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
	const s_vec3* plane_point,
	const s_vec3* plane_normal,
	s_vec3* out_world) {
	if (!plane_point || !plane_normal || !out_world) {
		return false;
	}

	s_vec3 origin = s_vec3(0.0f, 0.0f, 0.0f);
	s_vec3 direction = s_vec3(0.0f, 0.0f, 0.0f);
	if (!se_camera_screen_to_ray(camera, screen_x, screen_y, viewport_width, viewport_height, &origin, &direction)) {
		return false;
	}

	const s_vec3 normal = s_vec3_normalize(plane_normal);
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
