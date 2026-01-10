// Syphax-Engine - Ougi Washi

#include "se_ui.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
    se_window* window = se_window_create("Syphax-Engine - UI Example", WIDTH, HEIGHT);
    
    se_render_handle_params params = {0};
    params.framebuffers_count = 8;
    params.render_buffers_count = 8;
    params.shaders_count = 8;
    se_render_handle* render_handle = se_render_handle_create(&params);
    
    se_ui* ui = se_ui_create(render_handle, window, 4, 1, SE_UI_LAYOUT_HORIZONTAL);

    se_object_2d* button_yes = se_ui_add_object(ui, "examples/ui/button.glsl", &se_vec2(0.1, 0.1));
    se_object_2d* button_no = se_ui_add_object(ui, "examples/ui/button.glsl", &se_vec2(0.1, 0.1));

    // TODO: Edit syphax array and make this in a single line
    se_key_combo exit_keys = {0};
    s_array_init(&exit_keys, 1);
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE); 
    se_window_set_exit_keys(window, &exit_keys);

    while (!se_window_should_close(window)) {
        se_window_poll_events();
        se_window_update(window);
        se_render_clear();
        se_ui_render(ui);
        se_ui_render_to_screen(ui);
        se_window_render_screen(window);
    }
    
    se_ui_destroy(ui);
    se_render_handle_cleanup(render_handle);
    se_window_destroy(window);
    return 0;
}
