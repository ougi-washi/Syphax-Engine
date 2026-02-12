// Syphax-Engine - Ougi Washi

#if defined(SE_WINDOW_BACKEND_TERMINAL)

#include "se_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef s_array(se_window_handle, se_windows_registry);
static se_windows_registry windows_registry = {0};

static se_window* se_window_from_handle(se_context* context, const se_window_handle window) {
	return s_array_get(&context->windows, window);
}

se_window_handle se_window_create(const char* title, const u32 width, const u32 height) {
	se_context* context = se_current_context();
	if (!context || !title) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return S_HANDLE_NULL;
	}
	if (s_array_get_capacity(&context->windows) == 0) {
		s_array_init(&context->windows);
	}

	se_window_handle window_handle = s_array_increment(&context->windows);
	se_window* new_window = s_array_get(&context->windows, window_handle);

	memset(new_window, 0, sizeof(se_window));
	new_window->context = context;
	new_window->width = width;
	new_window->height = height;
	new_window->handle = NULL;
	new_window->cursor_mode = SE_WINDOW_CURSOR_NORMAL;
	new_window->raw_mouse_motion_supported = false;
	new_window->raw_mouse_motion_enabled = false;
	new_window->should_close = false;
	new_window->target_fps = 30;
	new_window->time.current = 0.0;

	if (s_array_get_capacity(&windows_registry) == 0) {
		s_array_init(&windows_registry);
	}
	s_array_add(&windows_registry, window_handle);

	printf("[terminal] window: %s (%u x %u)\n", title, width, height);
	se_set_last_error(SE_RESULT_OK);
	return window_handle;
}

void se_window_attach_render(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	s_assertf(window_ptr, "se_window_attach_render :: window is null");
	window_ptr->context = context;
}

void se_window_update(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	se_window_check_exit_keys(window);
	memcpy(window_ptr->keys_prev, window_ptr->keys, sizeof(window_ptr->keys));
	memcpy(window_ptr->mouse_buttons_prev, window_ptr->mouse_buttons, sizeof(window_ptr->mouse_buttons));
	window_ptr->mouse_dx = 0.0;
	window_ptr->mouse_dy = 0.0;
	window_ptr->scroll_dx = 0.0;
	window_ptr->scroll_dy = 0.0;
	window_ptr->time.last_frame = window_ptr->time.current;
	window_ptr->time.current += 1.0 / (f64)window_ptr->target_fps;
	window_ptr->time.delta = window_ptr->time.current - window_ptr->time.last_frame;
	window_ptr->time.frame_start = window_ptr->time.current;
	window_ptr->frame_count++;
}

void se_window_tick(const se_window_handle window) {
	se_window_update(window);
	se_window_poll_events();
}

void se_window_set_current_context(const se_window_handle window) {
	(void)window;
}

void se_window_render_quad(const se_window_handle window) {
	(void)window;
}

void se_window_render_screen(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	printf("[terminal] frame %llu\n", (unsigned long long)window_ptr->frame_count);
}

void se_window_present(const se_window_handle window) {
	se_window_render_screen(window);
}

void se_window_present_frame(const se_window_handle window, const s_vec4* clear_color) {
	(void)clear_color;
	se_window_render_screen(window);
}

void se_window_poll_events() {
}

b8 se_window_is_key_down(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key];
}

b8 se_window_is_key_pressed(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window_ptr->keys[key] && !window_ptr->keys_prev[key];
}

b8 se_window_is_key_released(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return !window_ptr->keys[key] && window_ptr->keys_prev[key];
}

b8 se_window_is_mouse_down(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button];
}

b8 se_window_is_mouse_pressed(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window_ptr->mouse_buttons[button] && !window_ptr->mouse_buttons_prev[button];
}

b8 se_window_is_mouse_released(const se_window_handle window, se_mouse_button button) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return !window_ptr->mouse_buttons[button] && window_ptr->mouse_buttons_prev[button];
}

f32 se_window_get_mouse_position_x(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (f32)window_ptr->mouse_x : 0.0f;
}

f32 se_window_get_mouse_position_y(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (f32)window_ptr->mouse_y : 0.0f;
}

void se_window_get_mouse_position_normalized(const se_window_handle window, s_vec2* out_mouse_position) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_position) {
		return;
	}
	*out_mouse_position = s_vec2((window_ptr->mouse_x / window_ptr->width), (window_ptr->mouse_y / window_ptr->height));
}

void se_window_get_mouse_delta(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_delta) {
		return;
	}
	*out_mouse_delta = s_vec2(window_ptr->mouse_dx, window_ptr->mouse_dy);
}

void se_window_get_mouse_delta_normalized(const se_window_handle window, s_vec2* out_mouse_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_mouse_delta) {
		return;
	}
	*out_mouse_delta = s_vec2((window_ptr->mouse_dx / window_ptr->width), (window_ptr->mouse_dy / window_ptr->height));
}

void se_window_get_scroll_delta(const se_window_handle window, s_vec2* out_scroll_delta) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !out_scroll_delta) {
		return;
	}
	*out_scroll_delta = s_vec2(window_ptr->scroll_dx, window_ptr->scroll_dy);
}

void se_window_set_cursor_mode(const se_window_handle window, const se_window_cursor_mode mode) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->cursor_mode = mode;
}

se_window_cursor_mode se_window_get_cursor_mode(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return SE_WINDOW_CURSOR_NORMAL;
	}
	return window_ptr->cursor_mode;
}

b8 se_window_is_raw_mouse_motion_supported(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return false;
	}
	return window_ptr->raw_mouse_motion_supported;
}

void se_window_set_raw_mouse_motion(const se_window_handle window, const b8 enabled) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	if (!window_ptr->raw_mouse_motion_supported) {
		window_ptr->raw_mouse_motion_enabled = false;
		return;
	}
	window_ptr->raw_mouse_motion_enabled = enabled;
}

b8 se_window_is_raw_mouse_motion_enabled(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return false;
	}
	return window_ptr->raw_mouse_motion_enabled;
}

b8 se_window_should_close(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->should_close : true;
}

void se_window_set_should_close(const se_window_handle window, const b8 should_close) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->should_close = should_close;
}

void se_window_set_exit_keys(const se_window_handle window, se_key_combo* keys) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr || !keys) {
		return;
	}
	window_ptr->exit_keys = keys;
	window_ptr->use_exit_key = false;
}

void se_window_set_exit_key(const se_window_handle window, se_key key) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->exit_key = key;
	window_ptr->use_exit_key = true;
}

void se_window_check_exit_keys(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	if (window_ptr->use_exit_key) {
		if (window_ptr->exit_key != SE_KEY_UNKNOWN && se_window_is_key_down(window, window_ptr->exit_key)) {
			window_ptr->should_close = true;
			return;
		}
	}
	if (!window_ptr->exit_keys) {
		return;
	}
	se_key_combo* keys = window_ptr->exit_keys;
	if (s_array_get_size(keys) == 0) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(keys); ++i) {
		se_key* current_key = s_array_get(keys, s_array_handle(keys, (u32)i));
		if (!se_window_is_key_down(window, *current_key)) {
			return;
		}
	}
	window_ptr->should_close = true;
}

f64 se_window_get_delta_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->time.delta : 0.0;
}

f64 se_window_get_fps(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? (1.0 / window_ptr->time.delta) : 0.0;
}

f64 se_window_get_time(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	return window_ptr ? window_ptr->time.current : 0.0;
}

void se_window_set_target_fps(const se_window_handle window, const u16 fps) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	window_ptr->target_fps = fps;
}

i32 se_window_register_input_event(const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	(void)window;
	(void)box;
	(void)depth;
	(void)on_interact_callback;
	(void)on_stop_interact_callback;
	(void)callback_data;
	return -1;
}

void se_window_update_input_event(const i32 input_event_id, const se_window_handle window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	(void)input_event_id;
	(void)window;
	(void)box;
	(void)depth;
	(void)on_interact_callback;
	(void)on_stop_interact_callback;
	(void)callback_data;
}

void se_window_register_resize_event(const se_window_handle window, se_resize_event_callback callback, void* data) {
	(void)window;
	(void)callback;
	(void)data;
}

void se_window_destroy(const se_window_handle window) {
	se_context* context = se_current_context();
	se_window* window_ptr = se_window_from_handle(context, window);
	if (!window_ptr) {
		return;
	}
	if (window_ptr->quad.vao != 0) {
		se_quad_destroy(&window_ptr->quad);
	}
	for (sz i = 0; i < s_array_get_size(&windows_registry); i++) {
		se_window_handle slot = *s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)i));
		if (slot == window) {
			s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)i));
			break;
		}
	}
	s_array_remove(&context->windows, window);
	if (s_array_get_size(&windows_registry) == 0) {
		s_array_clear(&windows_registry);
	}
}

void se_window_destroy_all(void){
	while (s_array_get_size(&windows_registry) > 0) {
		se_window_handle window_handle = *s_array_get(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
		if (window_handle == S_HANDLE_NULL) {
			s_array_remove(&windows_registry, s_array_handle(&windows_registry, (u32)(s_array_get_size(&windows_registry) - 1)));
			continue;
		}
		se_window_destroy(window_handle);
	}
}

#endif // SE_WINDOW_BACKEND_TERMINAL
