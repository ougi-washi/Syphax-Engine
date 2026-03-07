// Syphax-Engine - Ougi Washi

#ifndef SE_CAMERA_H
#define SE_CAMERA_H

#include "se.h"

typedef struct se_camera {
	s_vec3 position;
	s_vec3 target;
	s_vec3 up;
	s_vec3 right;
	// Rotation is (pitch, yaw, roll) in radians.
	s_vec3 rotation;
	f32 target_distance;
	f32 fov;
	f32 near;
	f32 far;
	f32 aspect;
	f32 ortho_height;
	s_mat4 cached_view_matrix;
	s_mat4 cached_projection_matrix;
	s_mat4 cached_view_projection_matrix;
	b8 use_orthographic : 1;
	b8 target_mode : 1;
	b8 view_dirty : 1;
	b8 projection_dirty : 1;
	b8 view_projection_dirty : 1;
} se_camera;
typedef s_array(se_camera, se_cameras);
typedef se_camera_handle se_camera_ptr;
typedef s_array(se_camera_handle, se_cameras_ptr);

extern se_camera_handle se_camera_create(void);
extern void se_camera_destroy(const se_camera_handle camera);
extern se_camera *se_camera_get(const se_camera_handle camera);
extern s_mat4 se_camera_get_view_matrix(const se_camera_handle camera);
extern s_mat4 se_camera_get_projection_matrix(const se_camera_handle camera);
extern s_mat4 se_camera_get_view_projection_matrix(const se_camera_handle camera);
extern s_vec3 se_camera_get_forward_vector(const se_camera_handle camera);
extern s_vec3 se_camera_get_right_vector(const se_camera_handle camera);
extern s_vec3 se_camera_get_up_vector(const se_camera_handle camera);
extern void se_camera_set_rotation(const se_camera_handle camera, const s_vec3* rotation);
extern void se_camera_add_rotation(const se_camera_handle camera, const s_vec3* delta_rotation);
extern b8 se_camera_get_rotation(const se_camera_handle camera, s_vec3* out_rotation);
extern void se_camera_set_location(const se_camera_handle camera, const s_vec3* location);
extern void se_camera_add_location(const se_camera_handle camera, const s_vec3* delta_location);
extern b8 se_camera_get_location(const se_camera_handle camera, s_vec3* out_location);
extern void se_camera_set_target(const se_camera_handle camera, const s_vec3* target);
extern b8 se_camera_get_target(const se_camera_handle camera, s_vec3* out_target);
extern void se_camera_set_target_mode(const se_camera_handle camera, const b8 enabled);
extern b8 se_camera_is_target_mode(const se_camera_handle camera);
extern void se_camera_set_perspective(const se_camera_handle camera, const f32 fov_degrees, const f32 near_plane, const f32 far_plane);
extern void se_camera_set_orthographic(const se_camera_handle camera, const f32 ortho_height, const f32 near_plane, const f32 far_plane);
extern void se_camera_set_aspect(const se_camera_handle camera, const f32 width, const f32 height);
extern void se_camera_set_window_aspect(const se_camera_handle camera, const se_window_handle window);
extern b8 se_camera_world_to_ndc(const se_camera_handle camera, const s_vec3 *world, s_vec3 *out_ndc);
extern b8 se_camera_world_to_screen(const se_camera_handle camera, const s_vec3 *world, const f32 viewport_width, const f32 viewport_height, s_vec2 *out_screen);
extern b8 se_camera_screen_to_ray(const se_camera_handle camera, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, s_vec3 *out_origin, s_vec3 *out_direction);
extern b8 se_camera_screen_to_plane(const se_camera_handle camera, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, const s_vec3 *plane_point, const s_vec3 *plane_normal, s_vec3 *out_world);

#endif // SE_CAMERA_H
