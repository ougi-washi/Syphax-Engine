// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"
#include "se_text.h"

int main() {
	se_render_handle *render = NULL;
	se_window *window = NULL;
	se_text_handle* text_handle = NULL;
	se_font* font = NULL;
	render = se_render_handle_create(NULL);
	if (!render) {
		return 1;
	}
	window = se_window_create(render, "Syphax Hello", 1280, 720);
	if (!window) {
		se_render_handle_destroy(render);
		return 1;
	}
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.08f, 0.08f, 0.1f, 1.0f));

	text_handle = se_text_handle_create(render, 0);
	if (!text_handle) {
		se_window_destroy(window);
		se_render_handle_destroy(render);
		return 1;
	}
	font = se_font_load(text_handle, "fonts/ithaca.ttf", 32.0f);
	if (!font) {
		se_text_handle_destroy(text_handle);
		se_window_destroy(window);
		se_render_handle_destroy(render);
		return 1;
	}

	se_window_loop(window,
		se_render_clear();
		se_text_render(text_handle, font, "Hello Syphax", &s_vec2(0.0f, 0.0f), &s_vec2(1.0f, 1.0f), 0.03f);
	);

	se_text_handle_destroy(text_handle);
	se_window_destroy(window);
	se_render_handle_destroy(render);
	return 0;
}
