// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_render.h"
#include "se_text.h"

int main() {
	se_render_handle *render = se_render_handle_create(NULL);
	se_window *window = se_window_create(render, "Syphax Hello", 1280, 720);
	se_text_handle *text_handle = se_text_handle_create(render, 0);
	se_font *font = se_font_load(text_handle, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_render_set_background_color(s_vec4(0.08f, 0.08f, 0.1f, 1.0f));

	se_window_loop(window,
		se_render_clear();
		se_text_render(text_handle, font, "Hello Syphax", &s_vec2(0.0f, 0.0f), &s_vec2(1.0f, 1.0f), 0.03f);
	);

	se_text_handle_destroy(text_handle);
	se_window_destroy(window);
	se_render_handle_destroy(render);
	return 0;
}
