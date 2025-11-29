// Syphax-Engine - Ougi Washi

#ifndef SE_WINDOW_H
#define SE_WINDOW_H

#include "syphax/s_array.h"
#include "se_math.h"
#include <GLFW/glfw3.h>

typedef struct {
    f64 current;
    f64 delta;
    f64 last_frame;
} se_time;

typedef enum {
    SE_INPUT_EVENT_KEY,
    SE_INPUT_EVENT_MOUSE
} se_input_event_type;

typedef void (*se_input_event_callback)(void* window, void* data); // Data is a ptr for the user to store/pass data. Handle it carefully.
typedef struct {
    i32 id;
    b8 active : 1;
    se_box_2d box;
    i32 depth;
    se_input_event_callback callback;
    void* callback_data;
} se_input_event;

typedef struct {
    GLFWwindow* handle;
    u32 width;
    u32 height;

    b8 keys[1024];
    f64 mouse_x, mouse_y;
    f64 mouse_dx, mouse_dy;
    b8 mouse_buttons[8];

    GLuint quad_vao;
    GLuint quad_vbo;
    GLuint quad_ebo;

    u16 target_fps;
    se_time time;
    u64 frame_count;

    s_array(se_input_event, input_events);
} se_window;

typedef s_array(se_window, se_windows);
typedef s_array(i32, key_combo);

extern se_window* se_window_create(const char* title, const u32 width, const u32 height);
extern void se_window_update(se_window* window); // frame start: updates time and frame count for the new frame
extern void se_window_render_quad(se_window* window);   // mid-frame: draws using window's quad
extern void se_window_render_screen(se_window* window); // frame end: clear, renders the frame and swaps buffers
extern void se_window_poll_events();
extern b8 se_window_is_key_down(se_window* window, i32 key);
extern b8 se_window_is_mouse_down(se_window* window, i32 button);
extern void se_window_get_mouse_position_normalized(se_window* window, se_vec2* out_mouse_position);
extern b8 se_window_should_close(se_window* window);
extern void se_window_check_exit_keys(se_window* window, key_combo* keys);
extern f64 se_window_get_delta_time(se_window* window);
extern f64 se_window_get_time(se_window* window);
extern void se_window_set_target_fps(se_window* window, const u16 fps);
extern i32 se_window_register_input_event(se_window* window, const se_box_2d* box, const i32 depth, se_input_event_callback callback, void* callback_data);
extern void se_window_destroy(se_window* window);
extern void se_window_destroy_all();

#endif // SE_WINDOW_H
