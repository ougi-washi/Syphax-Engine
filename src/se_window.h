// Syphax-Engine - Ougi Washi

#ifndef SE_WINDOW_H
#define SE_WINDOW_H

#include "syphax/s_array.h"
#include <GLFW/glfw3.h>

#define SE_MAX_WINDOWS 8
#define SE_MAX_KEY_COMBOS 8

typedef struct {
    f64 current;
    f64 delta;
    f64 last_frame;
} se_time;

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
} se_window;

S_DEFINE_ARRAY(se_window, se_windows, SE_MAX_WINDOWS);
S_DEFINE_ARRAY(i32, key_combo, SE_MAX_KEY_COMBOS);

extern se_window* se_window_create(const char* title, const u32 width, const u32 height);
extern void se_window_update(se_window* window); // frame start: updates time and frame count for the new frame
extern void se_window_render_quad(se_window* window);   // mid-frame: draws using window's quad
extern void se_window_render_screen(se_window* window); // frame end: clear, renders the frame and swaps buffers
extern void se_window_poll_events();
extern b8 se_window_is_key_down(se_window* window, i32 key);
extern b8 se_window_should_close(se_window* window);
extern void se_window_check_exit_keys(se_window* window, key_combo* keys);
extern f64 se_window_get_delta_time(se_window* window);
extern f64 se_window_get_time(se_window* window);
extern void se_window_set_target_fps(se_window* window, const u16 fps);
extern void se_window_destroy(se_window* window);
extern void se_window_destroy_all();

#endif // SE_WINDOW_H
