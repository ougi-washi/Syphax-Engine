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
	f32 ortho_height;
	b8 use_orthographic : 1;
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
extern void se_camera_set_location(const se_camera_handle camera, const s_vec3* location);
extern void se_camera_set_target(const se_camera_handle camera, const s_vec3* target);
extern void se_camera_set_perspective(const se_camera_handle camera, const f32 fov_degrees, const f32 near_plane, const f32 far_plane);
extern void se_camera_set_orthographic(const se_camera_handle camera, const f32 ortho_height, const f32 near_plane, const f32 far_plane);
extern void se_camera_set_aspect(const se_camera_handle camera, const f32 width, const f32 height);
extern void se_camera_set_aspect_from_window(const se_camera_handle camera, const se_window_handle window);
extern void se_camera_set_orbit_defaults(const se_camera_handle camera, const se_window_handle window, const s_vec3* pivot, const f32 distance);
extern void se_camera_orbit(const se_camera_handle camera, const s_vec3 *pivot, const f32 yaw_delta, const f32 pitch_delta, const f32 min_pitch, const f32 max_pitch);
extern void se_camera_pan_world(const se_camera_handle camera, const s_vec3 *delta);
extern void se_camera_pan_local(const se_camera_handle camera, const f32 right_delta, const f32 up_delta, const f32 forward_delta);
extern void se_camera_dolly(const se_camera_handle camera, const s_vec3 *pivot, const f32 distance_delta, const f32 min_distance, const f32 max_distance);
extern void se_camera_clamp_target(const se_camera_handle camera, const s_vec3 *min_bounds, const s_vec3 *max_bounds);
extern void se_camera_smooth_target(const se_camera_handle camera, const s_vec3 *target, const f32 smoothing, const f32 dt);
extern void se_camera_smooth_position(const se_camera_handle camera, const s_vec3 *position, const f32 smoothing, const f32 dt);
extern b8 se_camera_world_to_ndc(const se_camera_handle camera, const s_vec3 *world, s_vec3 *out_ndc);
extern b8 se_camera_world_to_screen(const se_camera_handle camera, const s_vec3 *world, const f32 viewport_width, const f32 viewport_height, s_vec2 *out_screen);
extern b8 se_camera_screen_to_ray(const se_camera_handle camera, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, s_vec3 *out_origin, s_vec3 *out_direction);
extern b8 se_camera_screen_to_plane(const se_camera_handle camera, const f32 screen_x, const f32 screen_y, const f32 viewport_width, const f32 viewport_height, const s_vec3 *plane_point, const s_vec3 *plane_normal, s_vec3 *out_world);

typedef enum {
	SE_CAMERA_CONTROLLER_PRESET_UE = 0,
	SE_CAMERA_CONTROLLER_PRESET_BLENDER,
	SE_CAMERA_CONTROLLER_PRESET_TRACKPAD
} se_camera_controller_preset;

typedef struct {
	se_window_handle window;
	se_camera_handle camera;
	f32 movement_speed;
	f32 mouse_x_speed;
	f32 mouse_y_speed;
	f32 min_pitch;
	f32 max_pitch;
	f32 min_focus_distance;
	f32 max_focus_distance;
	b8 enabled : 1;
	b8 invert_y : 1;
	b8 look_toggle : 1;
	b8 lock_cursor_while_active : 1;
	b8 use_raw_mouse_motion : 1;
	se_camera_controller_preset preset;
} se_camera_controller_params;

#define SE_CAMERA_CONTROLLER_PARAMS_DEFAULTS ((se_camera_controller_params){ .window = S_HANDLE_NULL, .camera = S_HANDLE_NULL, .movement_speed = 120.0f, .mouse_x_speed = 0.0012f, .mouse_y_speed = 0.0012f, .min_pitch = -1.55f, .max_pitch = 1.55f, .min_focus_distance = 0.0f, .max_focus_distance = 0.0f, .enabled = true, .invert_y = false, .look_toggle = false, .lock_cursor_while_active = true, .use_raw_mouse_motion = true, .preset = SE_CAMERA_CONTROLLER_PRESET_UE })

typedef struct se_camera_controller se_camera_controller;

extern se_camera_controller* se_camera_controller_create(const se_camera_controller_params* params);
extern void se_camera_controller_destroy(se_camera_controller* controller);
extern void se_camera_controller_tick(se_camera_controller* controller, const f32 dt);

extern void se_camera_controller_set_enabled(se_camera_controller* controller, const b8 enabled);
extern b8 se_camera_controller_is_enabled(const se_camera_controller* controller);
extern void se_camera_controller_set_invert_y(se_camera_controller* controller, const b8 invert_y);
extern b8 se_camera_controller_get_invert_y(const se_camera_controller* controller);
extern void se_camera_controller_set_look_toggle(se_camera_controller* controller, const b8 look_toggle);
extern b8 se_camera_controller_get_look_toggle(const se_camera_controller* controller);
extern void se_camera_controller_set_speeds(se_camera_controller* controller, const f32 movement_speed, const f32 mouse_x_speed, const f32 mouse_y_speed);

extern void se_camera_controller_set_scene_bounds(se_camera_controller* controller, const s_vec3* center, const f32 radius);
extern void se_camera_controller_set_focus_limits(se_camera_controller* controller, const f32 min_distance, const f32 max_distance);
extern void se_camera_controller_focus_bounds(se_camera_controller* controller);

extern b8 se_camera_controller_set_preset(se_camera_controller* controller, const se_camera_controller_preset preset);
extern se_camera_controller_preset se_camera_controller_get_preset(const se_camera_controller* controller);

#endif // SE_CAMERA_H
