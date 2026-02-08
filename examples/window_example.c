// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
	se_render_handle* render_handle = se_render_handle_create(NULL);
	se_window* window = se_window_create(render_handle, "Syphax-Engine - Window Example", WIDTH, HEIGHT);
	se_render_set_background_color(s_vec4(0.5f, 0.5f, 0.5f, 1.0f));

	// TODO: Edit syphax array and make this in a single line
	se_key_combo exit_keys = {0};
	s_array_init(&exit_keys, 1);
	s_array_add(&exit_keys, GLFW_KEY_ESCAPE); 
	se_window_set_exit_keys(window, &exit_keys);

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_clear();
		se_window_render_screen(window);
	}

	s_array_clear(&exit_keys);
	se_window_destroy(window);
	return 0;
}
