// Syphax-Engine - Ougi Washi

#ifndef SE_CAMERA_CONTROLLER_H
#define SE_CAMERA_CONTROLLER_H

#include "se_input.h"
#include "se_render.h"

typedef enum {
	SE_CAMERA_CONTROLLER_PRESET_UE = 0,
	SE_CAMERA_CONTROLLER_PRESET_BLENDER,
	SE_CAMERA_CONTROLLER_PRESET_TRACKPAD
} se_camera_controller_preset;

typedef enum {
	SE_CAMERA_CONTROLLER_ACTION_LOOK_X = 0,
	SE_CAMERA_CONTROLLER_ACTION_LOOK_Y,
	SE_CAMERA_CONTROLLER_ACTION_MOVE_FORWARD,
	SE_CAMERA_CONTROLLER_ACTION_MOVE_BACKWARD,
	SE_CAMERA_CONTROLLER_ACTION_MOVE_RIGHT,
	SE_CAMERA_CONTROLLER_ACTION_MOVE_LEFT,
	SE_CAMERA_CONTROLLER_ACTION_MOVE_UP,
	SE_CAMERA_CONTROLLER_ACTION_MOVE_DOWN,
	SE_CAMERA_CONTROLLER_ACTION_SPEED,
	SE_CAMERA_CONTROLLER_ACTION_ORBIT,
	SE_CAMERA_CONTROLLER_ACTION_PAN,
	SE_CAMERA_CONTROLLER_ACTION_DOLLY,
	SE_CAMERA_CONTROLLER_ACTION_FLY,
	SE_CAMERA_CONTROLLER_ACTION_FOCUS,
	SE_CAMERA_CONTROLLER_ACTION_ZOOM,
	SE_CAMERA_CONTROLLER_ACTION_COUNT
} se_camera_controller_action;

typedef struct {
	se_input* input;
	se_window* window;
	se_camera* camera;
	u16 action_base;
	i32 context_priority;
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

#define SE_CAMERA_CONTROLLER_PARAMS_DEFAULTS ((se_camera_controller_params){ .input = NULL, .window = NULL, .camera = NULL, .action_base = 0, .context_priority = 100, .movement_speed = 120.0f, .mouse_x_speed = 0.0012f, .mouse_y_speed = 0.0012f, .min_pitch = -1.55f, .max_pitch = 1.55f, .min_focus_distance = 0.0f, .max_focus_distance = 0.0f, .enabled = true, .invert_y = false, .look_toggle = false, .lock_cursor_while_active = true, .use_raw_mouse_motion = true, .preset = SE_CAMERA_CONTROLLER_PRESET_UE })

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
extern se_input_context_id se_camera_controller_get_input_context(const se_camera_controller* controller);

#endif // SE_CAMERA_CONTROLLER_H
