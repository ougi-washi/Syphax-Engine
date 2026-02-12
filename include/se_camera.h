// Syphax-Engine - Ougi Washi

#ifndef SE_CAMERA_H
#define SE_CAMERA_H

#include "se.h"

typedef struct se_camera {
	s_vec3 position;
	s_vec3 target;
	s_vec3 up;
	s_vec3 right;
	f32 fov;
	f32 near;
	f32 far;
	f32 aspect;
} se_camera;
typedef s_array(se_camera, se_cameras);
typedef se_camera *se_camera_ptr;
typedef s_array(se_camera_ptr, se_cameras_ptr);

extern se_camera *se_camera_create(se_context *ctx);
extern void se_camera_destroy(se_context *ctx, se_camera *camera);
extern s_mat4 se_camera_get_view_matrix(const se_camera *camera);
extern s_mat4 se_camera_get_projection_matrix(const se_camera *camera);
extern void se_camera_set_aspect(se_camera *camera, const f32 width, const f32 height);

#endif // SE_CAMERA_H
