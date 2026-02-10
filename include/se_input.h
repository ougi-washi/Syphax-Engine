// Syphax-Engine - Ougi Washi

#ifndef SE_INPUT_H
#define SE_INPUT_H

#include "se_window.h"

#define SE_INPUT_CONTEXT_NAME_LENGTH 32
#define SE_INPUT_CONTEXT_INVALID (-1)

typedef i32 se_input_context_id;

typedef enum {
	SE_INPUT_DEVICE_KEY = 0,
	SE_INPUT_DEVICE_MOUSE_BUTTON,
	SE_INPUT_DEVICE_MOUSE_DELTA_X,
	SE_INPUT_DEVICE_MOUSE_DELTA_Y,
	SE_INPUT_DEVICE_MOUSE_SCROLL_X,
	SE_INPUT_DEVICE_MOUSE_SCROLL_Y
} se_input_device;

typedef enum {
	SE_INPUT_TRIGGER_DOWN = 0,
	SE_INPUT_TRIGGER_PRESSED,
	SE_INPUT_TRIGGER_RELEASED,
	SE_INPUT_TRIGGER_AXIS
} se_input_trigger;

typedef enum {
	SE_INPUT_AXIS_X = 0,
	SE_INPUT_AXIS_Y
} se_input_axis;

typedef struct {
	b8 shift : 1;
	b8 ctrl : 1;
	b8 alt : 1;
	b8 super : 1;
} se_input_modifiers;

#define SE_INPUT_MODIFIERS_NONE ((se_input_modifiers){0})
#define SE_INPUT_MODIFIERS_SHIFT ((se_input_modifiers){ .shift = true })
#define SE_INPUT_MODIFIERS_CTRL ((se_input_modifiers){ .ctrl = true })
#define SE_INPUT_MODIFIERS_ALT ((se_input_modifiers){ .alt = true })
#define SE_INPUT_MODIFIERS_SUPER ((se_input_modifiers){ .super = true })

typedef struct {
	se_input_device device;
	se_input_trigger trigger;
	se_input_modifiers modifiers;
	f32 scale;
	union {
		se_key key;
		se_mouse_button mouse_button;
	};
	b8 invert : 1;
	b8 exact_modifiers : 1;
} se_input_binding_desc;

#define SE_INPUT_BINDING_DESC_DEFAULTS ((se_input_binding_desc){ .device = SE_INPUT_DEVICE_KEY, .trigger = SE_INPUT_TRIGGER_DOWN, .modifiers = SE_INPUT_MODIFIERS_NONE, .scale = 1.0f, .key = SE_KEY_UNKNOWN, .invert = false, .exact_modifiers = false })

typedef struct {
	u16 actions_count;
	u16 contexts_count;
	u16 bindings_per_action;
} se_input_params;

#define SE_INPUT_PARAMS_DEFAULTS ((se_input_params){ .actions_count = 64, .contexts_count = 8, .bindings_per_action = 8 })

typedef struct se_input se_input;

extern se_input* se_input_create(const se_input_params* params);
extern void se_input_destroy(se_input* input);
extern void se_input_tick(se_input* input, se_window* window);

extern se_input_context_id se_input_context_create(se_input* input, const c8* name, const i32 priority);
extern b8 se_input_context_destroy(se_input* input, const se_input_context_id context_id);
extern b8 se_input_set_context_active(se_input* input, const se_input_context_id context_id, const b8 active);
extern b8 se_input_is_context_active(const se_input* input, const se_input_context_id context_id);
extern b8 se_input_set_context_priority(se_input* input, const se_input_context_id context_id, const i32 priority);
extern b8 se_input_context_clear_bindings(se_input* input, const se_input_context_id context_id);
extern b8 se_input_action_clear_bindings(se_input* input, const se_input_context_id context_id, const u16 action);

extern b8 se_input_bind(se_input* input, const se_input_context_id context_id, const u16 action, const se_input_binding_desc* binding);
extern b8 se_input_bind_key(se_input* input, const se_input_context_id context_id, const u16 action, const se_key key, const se_input_trigger trigger, const f32 scale, const se_input_modifiers modifiers, const b8 exact_modifiers);
extern b8 se_input_bind_mouse_button(se_input* input, const se_input_context_id context_id, const u16 action, const se_mouse_button button, const se_input_trigger trigger, const f32 scale, const se_input_modifiers modifiers, const b8 exact_modifiers);
extern b8 se_input_bind_mouse_delta(se_input* input, const se_input_context_id context_id, const u16 action, const se_input_axis axis, const f32 scale, const se_input_modifiers modifiers, const b8 invert, const b8 exact_modifiers);
extern b8 se_input_bind_mouse_scroll(se_input* input, const se_input_context_id context_id, const u16 action, const se_input_axis axis, const f32 scale, const se_input_modifiers modifiers, const b8 invert, const b8 exact_modifiers);

extern f32 se_input_get_action_value(const se_input* input, const u16 action);
extern b8 se_input_is_action_down(const se_input* input, const u16 action);
extern b8 se_input_is_action_pressed(const se_input* input, const u16 action);
extern b8 se_input_is_action_released(const se_input* input, const u16 action);

extern u16 se_input_get_actions_count(const se_input* input);
extern u16 se_input_get_contexts_count(const se_input* input);

#endif // SE_INPUT_H
