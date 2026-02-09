// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
	se_render_handle* render_handle = se_render_handle_create(NULL);
	se_window* window = se_window_create(render_handle, "Syphax-Engine - Window Example", WIDTH, HEIGHT);
	se_render_set_background_color(s_vec4(0.5f, 0.5f, 0.5f, 1.0f));
	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	s_vec4 clear_color = s_vec4(0.5f, 0.5f, 0.5f, 1.0f);

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_window_present_frame(window, &clear_color);
	}
	se_window_destroy(window);
	return 0;
}
