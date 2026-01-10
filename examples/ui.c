// Syphax-Engine - Ougi Washi

#include "se_ui.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
    se_window* window = se_window_create("Syphax-Engine - UI Example", WIDTH, HEIGHT);
    
    se_render_handle_params params = {0};
    params.framebuffers_count = 8;
    params.render_buffers_count = 8;
    params.textures_count = 0;
    params.shaders_count = 8;
    params.models_count = 0;
    params.cameras_count = 0;
    se_render_handle* render_handle = se_render_handle_create(&params);
    
    se_ui* ui = se_ui_create(render_handle, 4, 1, SE_UI_LAYOUT_HORIZONTAL);

    se_object_2d* button_yes = se_ui_add_object(ui, "examples/ui/button.glsl", &se_vec2(0., 0.));
    se_object_2d* button_no = se_ui_add_object(ui, "examples/ui/button.glsl", &se_vec2(0., 0.));

    key_combo exit_keys = {0};
    s_array_init(&exit_keys, 1);
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE);
    
    while (!se_window_should_close(window)) {
        se_window_poll_events();
        se_window_check_exit_keys(window, &exit_keys);
        se_window_update(window);
        se_ui_render(ui, render_handle);
        se_ui_render_to_screen(ui, render_handle, window);
        se_window_render_screen(window);
    }
    
    se_ui_destroy(ui);
    se_render_handle_cleanup(render_handle);
    se_window_destroy(window);
    return 0;
}
