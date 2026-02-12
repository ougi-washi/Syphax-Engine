// Syphax Engine - Ougi Washi

#include "se_camera.h"
#include "se_defines.h"

static se_camera* se_camera_from_handle(se_context *ctx, const se_camera_handle camera) {
	return s_array_get(&ctx->cameras, camera);
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
	se_context *ctx = se_current_context();
	se_camera* camera_ptr = se_camera_from_handle(ctx, camera);
	return s_mat4_look_at(&camera_ptr->position, &camera_ptr->target, &camera_ptr->up);
}

s_mat4 se_camera_get_projection_matrix(const se_camera_handle camera) {
	se_context *ctx = se_current_context();
	se_camera* camera_ptr = se_camera_from_handle(ctx, camera);
	return s_mat4_perspective(camera_ptr->fov * (PI / 180.0f), camera_ptr->aspect, camera_ptr->near, camera_ptr->far);
}

void se_camera_set_aspect(const se_camera_handle camera, const f32 width, const f32 height) {
	se_context *ctx = se_current_context();
	se_camera* camera_ptr = se_camera_from_handle(ctx, camera);
	camera_ptr->aspect = width / height;
}
