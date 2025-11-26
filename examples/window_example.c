// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
    se_window* window = se_window_create("Syphax-Engine - Window Example", WIDTH, HEIGHT);

    key_combo exit_keys = {0};
    s_array_init(&exit_keys, 1);
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE);

    while (!se_window_should_close(window)) {
        se_window_poll_events();
        se_window_check_exit_keys(window, &exit_keys);
        se_render_clear();
        se_render_set_background_color(se_vec(4, 0.5, 0.5, 0.5, 1));
        se_window_render_screen(window);
    }

    s_array_clear(&exit_keys);
    se_window_destroy(window);
    return 0;
}
