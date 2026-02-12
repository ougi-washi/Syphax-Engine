// Syphax Engine - Ougi Washi

#include "se_camera.h"
#include "se_defines.h"

se_camera *se_camera_create(se_context *ctx) {
	if (!ctx) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	if (s_array_get_capacity(&ctx->cameras) == s_array_get_size(&ctx->cameras)) {
		const sz current_capacity = s_array_get_capacity(&ctx->cameras);
		const sz next_capacity = (current_capacity == 0) ? 2 : (current_capacity + 2);
		s_array_resize(&ctx->cameras, next_capacity);
	}
	se_camera *camera = s_array_increment(&ctx->cameras);
	memset(camera, 0, sizeof(*camera));
	camera->position = (s_vec3){0, 0, 5};
	camera->target = (s_vec3){0, 0, 0};
	camera->up = (s_vec3){0, 1, 0};
	camera->right = (s_vec3){1, 0, 0};
	camera->fov = 45.0f;
	camera->near = 0.1f;
	camera->far = 100.0f;
	camera->aspect = 1.78;
	se_set_last_error(SE_RESULT_OK);
	return camera;
}

void se_camera_destroy(se_context *ctx, se_camera *camera) {
	s_assertf(ctx, "se_camera_destroy :: ctx is null");
	s_assertf(camera, "se_camera_destroy :: camera is null");
	for (sz i = 0; i < s_array_get_size(&ctx->cameras); i++) {
		se_camera *slot = s_array_get(&ctx->cameras, i);
		if (slot == camera) {
			s_array_remove_at(&ctx->cameras, i);
			break;
		}
	}
}

s_mat4 se_camera_get_view_matrix(const se_camera *camera) {
	return s_mat4_look_at(&camera->position, &camera->target, &camera->up);
}

s_mat4 se_camera_get_projection_matrix(const se_camera *camera) {
	return s_mat4_perspective(camera->fov * (PI / 180.0f), camera->aspect, camera->near, camera->far);
}

void se_camera_set_aspect(se_camera *camera, const f32 width, const f32 height) {
	camera->aspect = width / height;
}
