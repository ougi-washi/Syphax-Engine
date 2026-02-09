// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_text.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main() {
	se_render_handle* render_handle = NULL;
	se_window* window = NULL;
	se_text_handle* text_handle = NULL;
	se_font* font = NULL;
	render_handle = se_render_handle_create(NULL);
	if (!render_handle) {
		return 1;
	}
	
	window = se_window_create(render_handle, "Syphax-Engine - Text Example", WIDTH, HEIGHT);
	if (!window) {
		se_render_handle_destroy(render_handle);
		return 1;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	
	text_handle = se_text_handle_create(render_handle, 0);
	if (!text_handle) {
		se_window_destroy(window);
		se_render_handle_destroy(render_handle);
		return 1;
	}
	font = se_font_load(text_handle, "fonts/ithaca.ttf", 32.f);
	if (!font) {
		se_text_handle_destroy(text_handle);
		se_window_destroy(window);
		se_render_handle_destroy(render_handle);
		return 1;
	}

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_clear();
		se_text_render(text_handle, font, "yer7am dinek, ch'hal boubli \nMa nedri welou", &s_vec2(0., 0.), &s_vec2(1, 1), .03f);
		se_window_render_screen(window);
	}

	se_text_handle_destroy(text_handle);
	se_render_handle_destroy(render_handle);
	se_window_destroy(window);
	return 0;
}
