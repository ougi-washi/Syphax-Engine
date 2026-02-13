// Syphax Engine - Ougi Washi

#include "se_camera.h"
#include "se_debug.h"
#include "se_defines.h"
#include "se_window.h"

#include <math.h>
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

void se_camera_set_perspective(const se_camera_handle camera, const f32 fov_degrees, const f32 near_plane, const f32 far_plane) {
	se_camera* camera_ptr = se_camera_require(camera);
	if (!camera_ptr || near_plane <= 0.0f || far_plane <= near_plane || fov_degrees <= 1.0f || fov_degrees >= 179.0f) {
		se_debug_log(SE_DEBUG_LEVEL_WARN, SE_DEBUG_CATEGORY_CAMERA, "Invalid perspective params fov=%.2f near=%.3f far=%.3f", fov_degrees, near_plane, far_plane);
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
		se_debug_log(SE_DEBUG_LEVEL_WARN, SE_DEBUG_CATEGORY_CAMERA, "Invalid orthographic params height=%.2f near=%.3f far=%.3f", ortho_height, near_plane, far_plane);
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
