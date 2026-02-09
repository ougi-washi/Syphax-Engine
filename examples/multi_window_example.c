// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
	se_render_handle* render_handle = NULL;
	se_window* window_main = NULL;
	se_window* window_1 = NULL;
	se_window* window_2 = NULL;
	render_handle = se_render_handle_create(NULL);
	if (!render_handle) {
		return 1;
	}
	
	window_main = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window Main", WIDTH, HEIGHT);
	window_1 = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window 1", WIDTH, HEIGHT);
	window_2 = se_window_create(render_handle, "Syphax-Engine - Multi Window Example - Window 2", WIDTH, HEIGHT);
	if (!window_main || !window_1 || !window_2) {
		if (window_1) {
			se_window_destroy(window_1);
		}
		if (window_2) {
			se_window_destroy(window_2);
		}
		if (window_main) {
			se_window_destroy(window_main);
		}
		se_render_handle_destroy(render_handle);
		return 1;
	}
	se_window_set_exit_key(window_main, SE_KEY_ESCAPE);

	while (!se_window_should_close(window_main)) {
		se_window_update(window_main);
		se_window_update(window_1);
		se_window_update(window_2);
		se_window_poll_events();

		se_window_set_current_context(window_main);
		se_render_set_background_color(s_vec4(0.5f, 0.1f, 0.1f, 1.0f));
		se_render_clear();
		se_window_present(window_main);

		se_window_set_current_context(window_1);
		se_render_set_background_color(s_vec4(0.1f, 0.5f, 0.1f, 1.0f));
		se_render_clear();
		se_window_present(window_1);
		
		se_window_set_current_context(window_2);
		se_render_set_background_color(s_vec4(0.1f, 0.1f, 0.5f, 1.0f));
		se_render_clear();
		se_window_present(window_2);
	}

	se_window_destroy(window_1);
	se_window_destroy(window_2);
	se_window_destroy(window_main);
	se_render_handle_destroy(render_handle);
	return 0;
}
