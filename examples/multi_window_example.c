// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"
#include "se_rhi.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
	se_render_handle* render_handle = se_render_handle_create(NULL);
	
	se_window* window_main = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window Main", WIDTH, HEIGHT);
	se_window* window_1 = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window 1", WIDTH, HEIGHT);
	se_window* window_2 = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window 2", WIDTH, HEIGHT);
	se_window_set_exit_key(window_main, SE_KEY_ESCAPE);

	while (!se_window_should_close(window_main)) {
		se_window_update(window_main);
		se_window_update(window_1);
		se_window_update(window_2);
		se_window_poll_events();

		glfwMakeContextCurrent((GLFWwindow*)window_main->handle);
		se_render_set_background_color(s_vec4(0.5f, 0.1f, 0.1f, 1.0f));
		se_render_clear();
		se_window_present(window_main);

		glfwMakeContextCurrent((GLFWwindow*)window_1->handle);
		se_render_set_background_color(s_vec4(0.1f, 0.5f, 0.1f, 1.0f));
		se_render_clear();
		se_window_present(window_1);
		
		glfwMakeContextCurrent((GLFWwindow*)window_2->handle);
		se_render_set_background_color(s_vec4(0.1f, 0.1f, 0.5f, 1.0f));
		se_render_clear();
		se_window_present(window_2);
	}

	se_window_destroy(window_1);
	se_window_destroy(window_2);
	se_window_destroy(window_main);
	return 0;
}
