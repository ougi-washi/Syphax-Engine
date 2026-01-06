// Syphax-Engine - Ougi Washi

#include "se_scene.h"
#include "se_text.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
    se_window* window = se_window_create("Syphax-Engine - Text Example", WIDTH, HEIGHT);
    
    se_render_handle_params params = {0};
    params.framebuffers_count = 8;
    params.render_buffers_count = 8;
    params.textures_count = 0;
    params.shaders_count = 8;
    params.models_count = 0;
    params.cameras_count = 0;
    se_render_handle* render_handle = se_render_handle_create(&params);
    
    se_scene_handle_params scene_params = {0};
    scene_params.objects_2d_count = 4;
    scene_params.objects_3d_count = 0;
    scene_params.scenes_2d_count = 2;
    scene_params.scenes_3d_count = 0;
    se_scene_handle* scene_handle = se_scene_handle_create(render_handle, &scene_params);
    se_scene_2d* scene_2d = se_scene_2d_create(scene_handle, &se_vec2(WIDTH, HEIGHT), 4);

    se_text_handle* text_handle = se_text_handle_create(render_handle, 1);
    se_font* font = se_font_load(text_handle, "fonts/ithaca.ttf");

    se_scene_2d_set_auto_resize(scene_2d, window, &se_vec2(1., 1.));

    key_combo exit_keys = {0};
    s_array_init(&exit_keys, 1);
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE);
    
    while (!se_window_should_close(window)) {
        se_window_poll_events();
        se_window_check_exit_keys(window, &exit_keys);
        se_window_update(window);
        //se_render_handle_reload_changed_shaders(render_handle);
        se_render_clear();
        se_text_render(text_handle, font, "yer7am dinek, ch'hal boubli", &se_vec2(0., 0.), 32.f);
        //se_scene_2d_render(scene_2d, render_handle);
        se_scene_2d_render_to_screen(scene_2d, render_handle, window);
        se_window_render_screen(window);
    }

    se_text_handle_cleanup(text_handle);
    se_scene_handle_cleanup(scene_handle);
    se_render_handle_cleanup(render_handle);
    se_window_destroy(window);
    return 0;
}
