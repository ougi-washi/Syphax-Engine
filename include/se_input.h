// Syphax-Engine - Ougi Washi

#ifndef SE_INPUT_H
#define SE_INPUT_H

#include "se_window.h"

#include "syphax/s_array.h"

typedef enum {
	SE_INPUT_DOWN = 0,
	SE_INPUT_PRESS,
	SE_INPUT_RELEASE,
	SE_INPUT_AXIS
} se_input_state;

typedef enum {
	SE_INPUT_ID_MOUSE_BUTTON_BASE = 1000,
	SE_INPUT_MOUSE_LEFT = 1000,
	SE_INPUT_MOUSE_RIGHT,
	SE_INPUT_MOUSE_MIDDLE,
	SE_INPUT_MOUSE_BUTTON_4,
	SE_INPUT_MOUSE_BUTTON_5,
	SE_INPUT_MOUSE_BUTTON_6,
	SE_INPUT_MOUSE_BUTTON_7,
	SE_INPUT_MOUSE_BUTTON_8,

	SE_INPUT_ID_MOUSE_AXIS_BASE = 2000,
	SE_INPUT_MOUSE_DELTA_X = 2000,
	SE_INPUT_MOUSE_DELTA_Y,
	SE_INPUT_MOUSE_SCROLL_X,
	SE_INPUT_MOUSE_SCROLL_Y
} se_input_id;

typedef struct se_input_handle se_input_handle;
typedef void (*se_input_callback)(void* user_data);

typedef struct se_input_binding {
	i32 id;
	se_input_state state;
	f32 value;
	se_input_callback callback;
	void* user_data;
	b8 is_valid : 1;
} se_input_binding;

typedef s_array(se_input_binding, se_input_bindings);

typedef struct se_input_handle {
	se_context* ctx;
	se_window_handle window;
	se_input_bindings bindings;
	u16 bindings_capacity;
	b8 enabled : 1;
} se_input_handle;

extern se_input_handle* se_input_create(const se_window_handle window, const u16 bindings_capacity);
extern void se_input_destroy(se_input_handle* input_handle);
extern b8 se_input_bind(se_input_handle* input_handle, const i32 id, const se_input_state state, se_input_callback callback, void* user_data);
extern void se_input_unbind_all(se_input_handle* input_handle);
extern void se_input_set_enabled(se_input_handle* input_handle, const b8 enabled);
extern b8 se_input_is_enabled(se_input_handle* input_handle);
extern void se_input_tick(se_input_handle* input_handle);
extern f32 se_input_get_value(se_input_handle* input_handle, const i32 id, const se_input_state state);

#endif // SE_INPUT_H
