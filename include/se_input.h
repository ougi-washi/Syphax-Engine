// Syphax-Engine - Ougi Washi

#ifndef SE_INPUT_H
#define SE_INPUT_H

#include "se_window.h"
#include "se_defines.h"

#include "syphax/s_array.h"

typedef enum {
	SE_INPUT_DOWN = 0,
	SE_INPUT_PRESS,
	SE_INPUT_RELEASE,
	SE_INPUT_AXIS
} se_input_state;

typedef enum {
	SE_INPUT_SOURCE_KEY = 0,
	SE_INPUT_SOURCE_MOUSE_BUTTON,
	SE_INPUT_SOURCE_AXIS
} se_input_source_type;

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
typedef struct se_input_runtime se_input_runtime;
typedef void (*se_input_callback)(void* user_data);
typedef void (*se_input_text_callback)(const c8* utf8_text, void* user_data);

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
	se_input_runtime* runtime;
	se_input_text_callback text_callback;
	void* text_user_data;
	b8 enabled : 1;
} se_input_handle;

typedef struct {
	f32 deadzone;
	f32 sensitivity;
	f32 exponent;
	f32 smoothing;
} se_input_axis_options;

#define SE_INPUT_AXIS_OPTIONS_DEFAULTS ((se_input_axis_options){ .deadzone = 0.0f, .sensitivity = 1.0f, .exponent = 1.0f, .smoothing = 0.0f })

typedef struct {
	i32 source_id;
	se_input_source_type source_type;
	se_input_state state;
	i32 chord_keys[4];
	u8 chord_count;
	se_input_axis_options axis;
} se_input_action_binding;

typedef struct {
	i32 action_id;
	c8 name[SE_MAX_NAME_LENGTH];
	i32 context_id;
	se_input_action_binding binding;
	f32 value;
	f32 previous_value;
	b8 down : 1;
	b8 pressed : 1;
	b8 released : 1;
	b8 valid : 1;
} se_input_action;

typedef struct {
	i32 context_id;
	c8 name[SE_MAX_NAME_LENGTH];
	b8 enabled : 1;
} se_input_context;

typedef struct {
	f64 timestamp;
	i32 action_id;
	f32 value;
	b8 down : 1;
	b8 pressed : 1;
	b8 released : 1;
} se_input_event_record;

typedef struct {
	u32 action_count;
	u32 context_count;
	u32 queue_size;
	u32 queue_capacity;
	u32 recorded_count;
	b8 recording_enabled : 1;
	b8 replay_enabled : 1;
} se_input_diagnostics;

extern se_input_handle* se_input_create(const se_window_handle window, const u16 bindings_capacity);
extern void se_input_destroy(se_input_handle* input_handle);
extern b8 se_input_bind(se_input_handle* input_handle, const i32 id, const se_input_state state, se_input_callback callback, void* user_data);
extern void se_input_unbind_all(se_input_handle* input_handle);
extern void se_input_set_enabled(se_input_handle* input_handle, const b8 enabled);
extern b8 se_input_is_enabled(se_input_handle* input_handle);
extern void se_input_tick(se_input_handle* input_handle);
extern f32 se_input_get_value(se_input_handle* input_handle, const i32 id, const se_input_state state);

extern b8 se_input_action_create(se_input_handle* input_handle, const i32 action_id, const c8* name, const i32 context_id);
extern b8 se_input_action_bind(se_input_handle* input_handle, const i32 action_id, const se_input_action_binding* binding);
extern b8 se_input_action_rebind_source(se_input_handle* input_handle, const i32 action_id, const i32 source_id, const se_input_source_type source_type, const se_input_state state);
extern b8 se_input_action_unbind(se_input_handle* input_handle, const i32 action_id);
extern f32 se_input_action_get_value(se_input_handle* input_handle, const i32 action_id);
extern b8 se_input_action_is_down(se_input_handle* input_handle, const i32 action_id);
extern b8 se_input_action_is_pressed(se_input_handle* input_handle, const i32 action_id);
extern b8 se_input_action_is_released(se_input_handle* input_handle, const i32 action_id);

extern b8 se_input_context_create(se_input_handle* input_handle, const i32 context_id, const c8* name, const b8 enabled);
extern b8 se_input_context_set_enabled(se_input_handle* input_handle, const i32 context_id, const b8 enabled);
extern b8 se_input_context_is_enabled(se_input_handle* input_handle, const i32 context_id);
extern b8 se_input_context_push(se_input_handle* input_handle, const i32 context_id);
extern b8 se_input_context_pop(se_input_handle* input_handle, i32* out_context_id);
extern b8 se_input_context_peek(se_input_handle* input_handle, i32* out_context_id);
extern void se_input_context_clear_stack(se_input_handle* input_handle);

extern void se_input_set_text_callback(se_input_handle* input_handle, se_input_text_callback callback, void* user_data);
extern void se_input_attach_window_text(se_input_handle* input_handle);
extern void se_input_emit_text(se_input_handle* input_handle, const c8* utf8_text);

extern b8 se_input_get_event_queue(se_input_handle* input_handle, const se_input_event_record** out_events, sz* out_count);
extern void se_input_clear_event_queue(se_input_handle* input_handle);

extern void se_input_record_start(se_input_handle* input_handle);
extern void se_input_record_stop(se_input_handle* input_handle);
extern void se_input_record_clear(se_input_handle* input_handle);
extern b8 se_input_replay_start(se_input_handle* input_handle, const b8 loop);
extern void se_input_replay_stop(se_input_handle* input_handle);

extern b8 se_input_save_mappings(se_input_handle* input_handle, const c8* path);
extern b8 se_input_load_mappings(se_input_handle* input_handle, const c8* path);
extern void se_input_dump_diagnostics(se_input_handle* input_handle, c8* out_buffer, const sz out_buffer_size);
extern void se_input_get_diagnostics(se_input_handle* input_handle, se_input_diagnostics* out_diagnostics);

#endif // SE_INPUT_H
