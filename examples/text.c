// Syphax-Engine - Ougi Washi

#include "se_scene.h"

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
    
    se_object_2d* button = se_object_2d_create(scene_handle, "examples/scene_example/button.glsl", &se_vec2(0.15, 0.), &se_vec2(0.1, 0.1), 16);
    const se_mat4 identity = mat4_identity();
    for (i32 i = 0; i < 16; i++) {
        se_instance_id button_instance_id = se_object_2d_add_instance(button, &identity, &identity);
        const se_mat4 translate = mat4_translate(&se_vec3(-15 + i * 1.5, 0., 0));
        se_object_2d_set_instance_transform(button, button_instance_id, &translate);
    }

    se_shader_set_vec3(button->shader, "u_color", &se_vec3(0, 1, 0));

    se_scene_2d_add_object(scene_2d, button);
    se_scene_2d_set_auto_resize(scene_2d, window, &se_vec2(1., 1.));

    key_combo exit_keys = {0};
    s_array_init(&exit_keys, 1);
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE);

    while (!se_window_should_close(window)) {
        se_window_poll_events();
        se_window_check_exit_keys(window, &exit_keys);
        se_window_update(window);
        se_render_handle_reload_changed_shaders(render_handle);
        se_render_clear();
        se_scene_2d_render(scene_2d, render_handle);
        se_scene_2d_render_to_screen(scene_2d, render_handle, window);
        se_window_render_screen(window);
    }

    se_scene_handle_cleanup(scene_handle);
    se_render_handle_cleanup(render_handle);
    se_window_destroy(window);
    return 0;
}
