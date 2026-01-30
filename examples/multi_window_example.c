// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
	se_render_handle_params params = {0};
	params.framebuffers_count = 8;
	params.render_buffers_count = 8;
	params.textures_count = 0;
	params.shaders_count = 8;
	params.models_count = 0;
	params.cameras_count = 0;
	se_render_handle* render_handle = se_render_handle_create(&params);
	
	se_window* window_main = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window Main", WIDTH, HEIGHT);
	se_window* window_1 = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window 1", WIDTH, HEIGHT);
	se_window* window_2 = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window 2", WIDTH, HEIGHT);

	// TODO: Edit syphax array and make this in a single line
	se_key_combo exit_keys = {0};
	s_array_init(&exit_keys, 1);
	s_array_add(&exit_keys, GLFW_KEY_ESCAPE); 
	se_window_set_exit_keys(window_main, &exit_keys);

	while (!se_window_should_close(window_main)) {
		se_window_poll_events();

		glfwMakeContextCurrent(window_main->handle);
		se_window_update(window_main);
		se_render_clear();
		se_render_set_background_color(se_vec(4, .5, 0.1, 0.1, 1));
		se_window_render_screen(window_main);

		glfwMakeContextCurrent(window_1->handle);
		se_window_update(window_1);
		se_render_clear();
		se_render_set_background_color(se_vec(4, 0.1, 0.5, 0.1, 1));
		se_window_render_screen(window_1);
		
		glfwMakeContextCurrent(window_2->handle);
		se_window_update(window_2);
		se_render_clear();
		se_render_set_background_color(se_vec(4, 0.1, 0.1, 0.5, 1));
		se_window_render_screen(window_2);
	}

	se_window_destroy(window_1);
	se_window_destroy(window_2);
	se_window_destroy(window_main);
	return 0;
}
