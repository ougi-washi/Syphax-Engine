// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main(void) {
	se_context *ctx = se_context_create();
	se_window_handle window_main = se_window_create("Syphax-Engine - Multi Window Example - Window Main", WIDTH, HEIGHT);
	se_window_handle window_1 = se_window_create("Syphax-Engine - Multi Window Example - Window 1", WIDTH, HEIGHT);
	se_window_handle window_2 = se_window_create("Syphax-Engine - Multi Window Example - Window 2", WIDTH, HEIGHT);
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
	se_context_destroy(ctx);
	return 0;
}
