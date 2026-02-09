// Syphax-Engine - Ougi Washi

#ifndef SE_WINDOW_H
#define SE_WINDOW_H

#include "se_render.h"


typedef enum {
	SE_KEY_UNKNOWN = -1,
	SE_KEY_SPACE = 32,
	SE_KEY_APOSTROPHE = 39,
	SE_KEY_COMMA = 44,
	SE_KEY_MINUS = 45,
	SE_KEY_PERIOD = 46,
	SE_KEY_SLASH = 47,
	SE_KEY_0 = 48,
	SE_KEY_1 = 49,
	SE_KEY_2 = 50,
	SE_KEY_3 = 51,
	SE_KEY_4 = 52,
	SE_KEY_5 = 53,
	SE_KEY_6 = 54,
	SE_KEY_7 = 55,
	SE_KEY_8 = 56,
	SE_KEY_9 = 57,
	SE_KEY_SEMICOLON = 59,
	SE_KEY_EQUAL = 61,
	SE_KEY_A = 65,
	SE_KEY_B = 66,
	SE_KEY_C = 67,
	SE_KEY_D = 68,
	SE_KEY_E = 69,
	SE_KEY_F = 70,
	SE_KEY_G = 71,
	SE_KEY_H = 72,
	SE_KEY_I = 73,
	SE_KEY_J = 74,
	SE_KEY_K = 75,
	SE_KEY_L = 76,
	SE_KEY_M = 77,
	SE_KEY_N = 78,
	SE_KEY_O = 79,
	SE_KEY_P = 80,
	SE_KEY_Q = 81,
	SE_KEY_R = 82,
	SE_KEY_S = 83,
	SE_KEY_T = 84,
	SE_KEY_U = 85,
	SE_KEY_V = 86,
	SE_KEY_W = 87,
	SE_KEY_X = 88,
	SE_KEY_Y = 89,
	SE_KEY_Z = 90,
	SE_KEY_LEFT_BRACKET = 91,
	SE_KEY_BACKSLASH = 92,
	SE_KEY_RIGHT_BRACKET = 93,
	SE_KEY_GRAVE_ACCENT = 96,
	SE_KEY_WORLD_1 = 161,
	SE_KEY_WORLD_2 = 162,
	SE_KEY_ESCAPE = 256,
	SE_KEY_ENTER = 257,
	SE_KEY_TAB = 258,
	SE_KEY_BACKSPACE = 259,
	SE_KEY_INSERT = 260,
	SE_KEY_DELETE = 261,
	SE_KEY_RIGHT = 262,
	SE_KEY_LEFT = 263,
	SE_KEY_DOWN = 264,
	SE_KEY_UP = 265,
	SE_KEY_PAGE_UP = 266,
	SE_KEY_PAGE_DOWN = 267,
	SE_KEY_HOME = 268,
	SE_KEY_END = 269,
	SE_KEY_CAPS_LOCK = 280,
	SE_KEY_SCROLL_LOCK = 281,
	SE_KEY_NUM_LOCK = 282,
	SE_KEY_PRINT_SCREEN = 283,
	SE_KEY_PAUSE = 284,
	SE_KEY_F1 = 290,
	SE_KEY_F2 = 291,
	SE_KEY_F3 = 292,
	SE_KEY_F4 = 293,
	SE_KEY_F5 = 294,
	SE_KEY_F6 = 295,
	SE_KEY_F7 = 296,
	SE_KEY_F8 = 297,
	SE_KEY_F9 = 298,
	SE_KEY_F10 = 299,
	SE_KEY_F11 = 300,
	SE_KEY_F12 = 301,
	SE_KEY_F13 = 302,
	SE_KEY_F14 = 303,
	SE_KEY_F15 = 304,
	SE_KEY_F16 = 305,
	SE_KEY_F17 = 306,
	SE_KEY_F18 = 307,
	SE_KEY_F19 = 308,
	SE_KEY_F20 = 309,
	SE_KEY_F21 = 310,
	SE_KEY_F22 = 311,
	SE_KEY_F23 = 312,
	SE_KEY_F24 = 313,
	SE_KEY_F25 = 314,
	SE_KEY_KP_0 = 320,
	SE_KEY_KP_1 = 321,
	SE_KEY_KP_2 = 322,
	SE_KEY_KP_3 = 323,
	SE_KEY_KP_4 = 324,
	SE_KEY_KP_5 = 325,
	SE_KEY_KP_6 = 326,
	SE_KEY_KP_7 = 327,
	SE_KEY_KP_8 = 328,
	SE_KEY_KP_9 = 329,
	SE_KEY_KP_DECIMAL = 330,
	SE_KEY_KP_DIVIDE = 331,
	SE_KEY_KP_MULTIPLY = 332,
	SE_KEY_KP_SUBTRACT = 333,
	SE_KEY_KP_ADD = 334,
	SE_KEY_KP_ENTER = 335,
	SE_KEY_KP_EQUAL = 336,
	SE_KEY_LEFT_SHIFT = 340,
	SE_KEY_LEFT_CONTROL = 341,
	SE_KEY_LEFT_ALT = 342,
	SE_KEY_LEFT_SUPER = 343,
	SE_KEY_RIGHT_SHIFT = 344,
	SE_KEY_RIGHT_CONTROL = 345,
	SE_KEY_RIGHT_ALT = 346,
	SE_KEY_RIGHT_SUPER = 347,
	SE_KEY_MENU = 348,
	SE_KEY_MAX = SE_KEY_MENU,
	SE_KEY_COUNT = SE_KEY_MAX + 1
} se_key;

typedef struct {
	f64 current;
	f64 delta;
	f64 last_frame;
	f64 frame_start;
} se_time;

typedef enum {
	SE_INPUT_EVENT_KEY,
	SE_INPUT_EVENT_MOUSE
} se_input_event_type;

typedef enum {
	SE_MOUSE_LEFT = 0,
	SE_MOUSE_RIGHT = 1,
	SE_MOUSE_MIDDLE = 2,
	SE_MOUSE_BUTTON_4 = 3,
	SE_MOUSE_BUTTON_5 = 4,
	SE_MOUSE_BUTTON_6 = 5,
	SE_MOUSE_BUTTON_7 = 6,
	SE_MOUSE_BUTTON_8 = 7,
	SE_MOUSE_BUTTON_COUNT = 8
} se_mouse_button;

typedef void (*se_input_event_callback)(void* window, void* data); // Data is a ptr for the user to store/pass data. Handle it carefully.
typedef struct {
	i32 id;
	b8 active : 1;
	se_box_2d box;
	i32 depth;
	se_input_event_callback on_interact_callback;
	se_input_event_callback on_stop_interact_callback;
	void* callback_data;
	b8 interacted : 1;
} se_input_event;

typedef void(*se_resize_event_callback)(void* window, void* data);
typedef struct {
	se_resize_event_callback callback;
	void* data;
} se_resize_handle;

typedef s_array(se_key, se_key_combo);

typedef struct {
	void* handle;
	se_render_handle* render_handle;
	u32 width;
	u32 height;

	b8 keys[SE_KEY_COUNT];
	b8 keys_prev[SE_KEY_COUNT];
	f64 mouse_x, mouse_y;
	f64 mouse_dx, mouse_dy;
	b8 mouse_buttons[8];
	b8 mouse_buttons_prev[8];

	se_quad quad;
	se_shader* shader;

	u16 target_fps;
	se_time time;
	u64 frame_count;

	se_key_combo* exit_keys;
	se_key exit_key;
	b8 use_exit_key : 1;
	s_array(se_resize_handle, resize_handles);
	s_array(se_input_event, input_events);
} se_window;

typedef s_array(se_window, se_windows);

extern se_window* se_window_create(se_render_handle* render_handle, const char* title, const u32 width, const u32 height);
extern void se_window_attach_render(se_window* window, se_render_handle* render_handle);
extern void se_window_update(se_window* window); // frame start: updates time and frame count for the new frame
extern void se_window_tick(se_window* window); // update + poll events (single-window convenience)
extern void se_window_render_quad(se_window* window);	 // mid-frame: draws using window's quad
extern void se_window_render_screen(se_window* window); // frame end: clear, renders the frame and swaps buffers
extern void se_window_present(se_window* window); // swaps buffers (alias for render_screen)
extern void se_window_present_frame(se_window* window, const s_vec4* clear_color);
extern void se_window_poll_events();
extern b8 se_window_is_key_down(se_window* window, se_key key);
extern b8 se_window_is_key_pressed(se_window* window, se_key key);
extern b8 se_window_is_key_released(se_window* window, se_key key);
extern b8 se_window_is_mouse_down(se_window* window, se_mouse_button button);
extern b8 se_window_is_mouse_pressed(se_window* window, se_mouse_button button);
extern b8 se_window_is_mouse_released(se_window* window, se_mouse_button button);
extern f32 se_window_get_mouse_position_x(se_window* window);
extern f32 se_window_get_mouse_position_y(se_window* window);
extern void se_window_get_mouse_position_normalized(se_window* window, s_vec2* out_mouse_position);
extern void se_window_get_mouse_delta(se_window* window, s_vec2* out_mouse_delta);
extern void se_window_get_mouse_delta_normalized(se_window* window, s_vec2* out_mouse_delta);
extern b8 se_window_should_close(se_window* window);
extern void se_window_set_should_close(se_window* window, const b8 should_close);
extern void se_window_set_exit_keys(se_window* window, se_key_combo* keys);
extern void se_window_set_exit_key(se_window* window, se_key key);
extern void se_window_check_exit_keys(se_window* window);
extern f64 se_window_get_delta_time(se_window* window);
extern f64 se_window_get_fps(se_window* window);
extern f64 se_window_get_time(se_window* window);
extern void se_window_set_target_fps(se_window* window, const u16 fps);
extern i32 se_window_register_input_event(se_window* window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data);
extern void se_window_update_input_event(const i32 input_event_id, se_window* window, const se_box_2d* box, const i32 depth, se_input_event_callback on_interact_callback, se_input_event_callback on_stop_interact_callback, void* callback_data);
extern void se_window_register_resize_event(se_window* window, se_resize_event_callback callback, void* data);
extern void se_window_destroy(se_window* window);
extern void se_window_destroy_all();

#define se_window_loop(_window, ...) do { \
	while (!se_window_should_close((_window))) { \
		se_window_tick((_window)); \
		__VA_ARGS__ \
		se_window_present((_window)); \
	} \
} while (0)

#endif // SE_WINDOW_H
