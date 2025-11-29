// Syphax-Engine - Ougi Washi

#include "se_scene.h"

#define WIDTH 1920
#define HEIGHT 1080

void on_button_yes_pressed(void* window) {
    if (se_window_is_mouse_down(window, 0)) {
        printf("Button YES pressed\n");
    }
}

void on_button_no_pressed(void* window) {
    if (se_window_is_mouse_down(window, 0)) {
        printf("Button NO pressed\n");
    }
}

i32 main() {
    se_window* window = se_window_create("Syphax-Engine - Scene 2D Example", WIDTH, HEIGHT);
    
    se_render_handle_params params = {0};
    params.framebuffers_count = 2;
    params.render_buffers_count = 2;
    params.textures_count = 1;
    params.shaders_count = 8;
    params.models_count = 1;
    params.cameras_count = 1;
    se_render_handle* render_handle = se_render_handle_create(&params);
    
    se_scene_handle_params scene_params = {0};
    scene_params.objects_2d_count = 8;
    scene_params.objects_3d_count = 0;
    scene_params.scenes_2d_count = 4;
    scene_params.scenes_3d_count = 0;
    se_scene_handle* scene_handle = se_scene_handle_create(render_handle, &scene_params);
    
    se_scene_2d* scene_2d = se_scene_2d_create(scene_handle, &se_vec2(WIDTH, HEIGHT), 10);
    
    se_object_2d* borders = se_object_2d_create(scene_handle, "examples/scene_example/borders.glsl", &se_vec2(0, 0), &se_vec2(0.95, 0.95));
    se_object_2d* panel = se_object_2d_create(scene_handle, "examples/scene_example/panel.glsl", &se_vec2(0, 0), &se_vec2(0.5, 0.5));
    se_object_2d* button_yes = se_object_2d_create(scene_handle, "examples/scene_example/button.glsl", &se_vec2(0.15, 0.), &se_vec2(0.1, 0.1));
    se_object_2d* button_no = se_object_2d_create(scene_handle, "examples/scene_example/button.glsl", &se_vec2(-0.15, 0.), &se_vec2(0.1, 0.1));
    se_shader_set_vec3(button_yes->shader, "u_color", &se_vec3(0, 1, 0));
    se_shader_set_vec3(button_no->shader, "u_color", &se_vec3(1, 0, 0));

    se_box_2d button_box_yes = {0};
    se_box_2d button_box_no = {0};
    se_object_2d_get_box_2d(button_yes, &button_box_yes);
    se_object_2d_get_box_2d(button_no, &button_box_no);
     
    se_window_register_input_event(window, &button_box_yes, 0, &on_button_yes_pressed);
    se_window_register_input_event(window, &button_box_no, 0, &on_button_no_pressed);
    
    se_scene_2d_add_object(scene_2d, borders);
    se_scene_2d_add_object(scene_2d, panel);
    se_scene_2d_add_object(scene_2d, button_yes);
    se_scene_2d_add_object(scene_2d, button_no);

    se_scene_2d_render(scene_2d, render_handle, window);
   
    key_combo exit_keys = {0};
    s_array_init(&exit_keys, 1);
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE);

    while (!se_window_should_close(window)) {
        se_window_poll_events();
        se_window_check_exit_keys(window, &exit_keys);
        se_window_update(window);
        se_render_handle_reload_changed_shaders(render_handle);
        se_render_clear();
        se_scene_2d_render_to_screen(scene_2d, render_handle, window);
        se_window_render_screen(window);
    }

    se_scene_handle_cleanup(scene_handle);
    se_render_handle_cleanup(render_handle);
    se_window_destroy(window);
    return 0;
}
