// Syphax-render_handle - Ougi Washi

#include "se_render.h"
#include "se_window.h"
#include "se_audio.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
    // audio setup  
    se_audio_input_init();
   
    se_window* window = se_window_create("Syphax-Engine - Audio Example", WIDTH, HEIGHT);
    se_render_handle render_handle = {0};
    se_camera* camera = se_camera_create(&render_handle);

    se_shader* main_shader = se_shader_load(&render_handle, "vert.glsl", "frag_main.glsl");

    // mesh setup
    se_shaders_ptr se_mesh_shaders = {0};
    se_shader* se_shader_0 = se_shader_load(&render_handle, "vert_mesh.glsl", "frag_mesh.glsl");
    s_array_add(&se_mesh_shaders, se_shader_0);

    se_model* model = se_model_load_obj(&render_handle, "cube.obj", &se_mesh_shaders);
    se_render_buffer* model_buf = se_render_buffer_create(&render_handle, WIDTH, HEIGHT, "examples/audio_example/model_buffer_frag.glsl"); // TODO: fix
 
    // TODO: Edit syphax array and make this in a single line
    se_key_combo exit_keys = {0};
    s_array_init(&exit_keys, 1);
    s_array_add(&exit_keys, GLFW_KEY_ESCAPE); 
    se_window_set_exit_keys(window, &exit_keys);
   
    while (!se_window_should_close(window)) {
        // input
        se_window_poll_events();
        se_window_update(window);
      
        se_render_handle_reload_changed_shaders(&render_handle);
       
        se_uniforms* global_uniforms = se_render_handle_get_global_uniforms(&render_handle);
        const se_vec3 amps = se_audio_input_get_amplitudes();
        se_uniform_set_vec3(global_uniforms, "amps", &amps);
        
        // render model
        se_render_buffer_bind(model_buf);
        se_render_clear();
        const se_vec3 rot_angle = {0.006, se_window_get_delta_time(window) * 0.1, .004};
        se_model_rotate(model, &rot_angle);
        se_model_render(&render_handle, model, camera);
        se_render_buffer_unbind(model_buf);

        // render main shader (screen)
        se_shader_use(&render_handle, main_shader, true, true);
        se_uniform_set_texture(global_uniforms, "model_buffer", model_buf->texture);
        
        se_window_render_screen(window);
    }
   
    se_audio_input_cleanup();
    se_render_handle_cleanup(&render_handle);
    se_camera_destroy(&render_handle, camera);
    se_window_destroy(window);
    return 0;
}
