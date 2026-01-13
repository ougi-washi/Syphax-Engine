// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_text.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
    se_window* window = se_window_create("Syphax-Engine - Text Example", WIDTH, HEIGHT);
    
    se_render_handle_params params = {0};
    params.framebuffers_count = 1;
    params.shaders_count = 2; // 1 for screen, 1 for text
    se_render_handle* render_handle = se_render_handle_create(&params);
    
    se_text_handle* text_handle = se_text_handle_create(render_handle, 1);
    se_font* font = se_font_load(text_handle, "fonts/ithaca.ttf", 32.f);

    // TODO: Edit syphax array and make this in a single line
    se_key_combo exit_keys = {0};
    s_array_init(&exit_keys, 1);
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE); 
    se_window_set_exit_keys(window, &exit_keys);

    while (!se_window_should_close(window)) {
        se_window_poll_events();
        se_window_update(window);
        se_render_clear();
        se_text_render(text_handle, font, "yer7am dinek, ch'hal boubli \nMa nedri welou", &se_vec2(0., 0.), .1, .08f);
        se_window_render_screen(window);
    }

    se_text_handle_cleanup(text_handle);
    se_render_handle_cleanup(render_handle);
    se_window_destroy(window);
    return 0;
}
