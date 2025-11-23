// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
    se_window* window_main = se_window_create("Syphax-Engine - Multi Window Example - Window Main", WIDTH, HEIGHT);
    se_window* window_1 = se_window_create("Syphax-Engine - Multi Window Example - Window 1", WIDTH, HEIGHT);
    se_window* window_2 = se_window_create("Syphax-Engine - Multi Window Example - Window 2", WIDTH, HEIGHT);

    key_combo exit_keys = {0};
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE);

    while (!se_window_should_close(window_main)) {
        se_window_poll_events();
        se_window_check_exit_keys(window_main, &exit_keys);

        glfwMakeContextCurrent(window_main->handle);
        se_render_clear();
        se_render_set_background_color(se_vec(4, .5, 0.1, 0.1, 1));
        se_window_render_screen(window_main);

        glfwMakeContextCurrent(window_1->handle);
        se_render_clear();
        se_render_set_background_color(se_vec(4, 0.1, 0.5, 0.1, 1));
        se_window_render_screen(window_1);
        
        glfwMakeContextCurrent(window_2->handle);
        se_render_clear();
        se_render_set_background_color(se_vec(4, 0.1, 0.1, 0.5, 1));
        se_window_render_screen(window_2);
    }

    se_window_destroy(window_1);
    se_window_destroy(window_2);
    se_window_destroy(window_main);
    return 0;
}
