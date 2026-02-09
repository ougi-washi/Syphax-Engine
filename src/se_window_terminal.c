// Syphax-Engine - Ougi Washi

#if defined(SE_WINDOW_BACKEND_TERMINAL)

#include "se_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static se_windows windows_container = { 0 };

se_window* se_window_create(se_render_handle* render_handle, const char* title, const u32 width, const u32 height) {
	s_assertf(title, "se_window_create :: title is null");
	if (s_array_get_capacity(&windows_container) == 0) {
		s_array_init(&windows_container, 8);
	}
	se_window* new_window = s_array_increment(&windows_container);
	memset(new_window, 0, sizeof(se_window));
	new_window->render_handle = render_handle;
	new_window->width = width;
	new_window->height = height;
	new_window->handle = NULL;
	new_window->target_fps = 30;
	new_window->time.current = 0.0;
	printf("[terminal] window: %s (%u x %u)\n", title, width, height);
	return new_window;
}

void se_window_attach_render(se_window* window, se_render_handle* render_handle) {
	s_assertf(window, "se_window_attach_render :: window is null");
	window->render_handle = render_handle;
}

void se_window_update(se_window* window) {
	se_window_check_exit_keys(window);
	memcpy(window->keys_prev, window->keys, sizeof(window->keys));
	memcpy(window->mouse_buttons_prev, window->mouse_buttons, sizeof(window->mouse_buttons));
	window->mouse_dx = 0.0;
	window->mouse_dy = 0.0;
	window->time.last_frame = window->time.current;
	window->time.current += 1.0 / (f64)window->target_fps;
	window->time.delta = window->time.current - window->time.last_frame;
	window->time.frame_start = window->time.current;
	window->frame_count++;
}

void se_window_tick(se_window* window) {
	se_window_update(window);
	se_window_poll_events();
}

void se_window_render_quad(se_window* window) {
	(void)window;
}

void se_window_render_screen(se_window* window) {
	printf("[terminal] frame %llu\n", (unsigned long long)window->frame_count);
}

void se_window_present(se_window* window) {
	se_window_render_screen(window);
}

void se_window_present_frame(se_window* window, const s_vec4* clear_color) {
	(void)clear_color;
	se_window_render_screen(window);
}

void se_window_poll_events() {
}

b8 se_window_is_key_down(se_window* window, se_key key) {
	if (!window || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window->keys[key];
}

b8 se_window_is_key_pressed(se_window* window, se_key key) {
	if (!window || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return window->keys[key] && !window->keys_prev[key];
}

b8 se_window_is_key_released(se_window* window, se_key key) {
	if (!window || key < 0 || key >= SE_KEY_COUNT) {
		return false;
	}
	return !window->keys[key] && window->keys_prev[key];
}

b8 se_window_is_mouse_down(se_window* window, se_mouse_button button) {
	if (!window || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window->mouse_buttons[button];
}

b8 se_window_is_mouse_pressed(se_window* window, se_mouse_button button) {
	if (!window || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return window->mouse_buttons[button] && !window->mouse_buttons_prev[button];
}

b8 se_window_is_mouse_released(se_window* window, se_mouse_button button) {
	if (!window || button < 0 || button >= SE_MOUSE_BUTTON_COUNT) {
		return false;
	}
	return !window->mouse_buttons[button] && window->mouse_buttons_prev[button];
}

f32 se_window_get_mouse_position_x(se_window* window) {
	return window ? (f32)window->mouse_x : 0.0f;
}

f32 se_window_get_mouse_position_y(se_window* window) {
	return window ? (f32)window->mouse_y : 0.0f;
}

void se_window_get_mouse_position_normalized(se_window* window, s_vec2* out_mouse_position) {
	if (!window || !out_mouse_position) {
		return;
	}
	*out_mouse_position = s_vec2((window->mouse_x / window->width), (window->mouse_y / window->height));
}

void se_window_get_mouse_delta(se_window* window, s_vec2* out_mouse_delta) {
	if (!window || !out_mouse_delta) {
		return;
	}
	*out_mouse_delta = s_vec2(window->mouse_dx, window->mouse_dy);
}

void se_window_get_mouse_delta_normalized(se_window* window, s_vec2* out_mouse_delta) {
	if (!window || !out_mouse_delta) {
		return;
	}
	*out_mouse_delta = s_vec2((window->mouse_dx / window->width), (window->mouse_dy / window->height));
}

b8 se_window_should_close(se_window* window) {
	return window ? window->should_close : true;
}

void se_window_set_should_close(se_window* window, const b8 should_close) {
	if (!window) {
		return;
	}
	window->should_close = should_close;
}

void se_window_set_exit_keys(se_window* window, se_key_combo* keys) {
	if (!window || !keys) {
		return;
	}
	window->exit_keys = keys;
	window->use_exit_key = false;
}

void se_window_set_exit_key(se_window* window, se_key key) {
	if (!window) {
		return;
	}
	window->exit_key = key;
	window->use_exit_key = true;
}

void se_window_check_exit_keys(se_window* window) {
	if (!window) {
		return;
	}
	if (window->use_exit_key) {
		if (window->exit_key != SE_KEY_UNKNOWN && se_window_is_key_down(window, window->exit_key)) {
			window->should_close = true;
			return;
		}
	}
	if (!window->exit_keys) {
		return;
	}
	se_key_combo* keys = window->exit_keys;
	if (keys->size == 0) {
		return;
	}
	s_foreach(keys, i) {
		se_key* current_key = s_array_get(keys, i);
		if (!se_window_is_key_down(window, *current_key)) {
			return;
		}
	}
	window->should_close = true;
}

f64 se_window_get_delta_time(se_window* window) {
	return window ? window->time.delta : 0.0;
}

f64 se_window_get_fps(se_window* window) {
	return window ? (1.0 / window->time.delta) : 0.0;
}

f64 se_window_get_time(se_window* window) {
	return window ? window->time.current : 0.0;
}

void se_window_set_target_fps(se_window* window, const u16 fps) {
	if (!window) {
		return;
	}
	window->target_fps = fps;
}

i32 se_window_register_input_event(se_window* window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	(void)window;
	(void)box;
	(void)depth;
	(void)on_interact_callback;
	(void)on_stop_interact_callback;
	(void)callback_data;
	return -1;
}

void se_window_update_input_event(const i32 input_event_id, se_window* window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data) {
	(void)input_event_id;
	(void)window;
	(void)box;
	(void)depth;
	(void)on_interact_callback;
	(void)on_stop_interact_callback;
	(void)callback_data;
}

void se_window_register_resize_event(se_window* window, se_resize_event_callback callback, void* data) {
	(void)window;
	(void)callback;
	(void)data;
}

void se_window_destroy(se_window* window) {
	if (!window) {
		return;
	}
	se_quad_destroy(&window->quad);
	if (s_array_get_size(&windows_container) == 0) {
		return;
	}
	s_array_remove(&windows_container, window);
	if (s_array_get_size(&windows_container) == 0) {
		s_array_clear(&windows_container);
	}
}

void se_window_destroy_all(){
	s_foreach_reverse(&windows_container, i) {
		se_window* window = s_array_get(&windows_container, i);
		se_window_destroy(window);
	}
}

#endif // SE_WINDOW_BACKEND_TERMINAL
