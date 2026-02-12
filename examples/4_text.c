// Syphax-Engine - Ougi Washi

#include "se_window.h"
#include "se_text.h"

#define WIDTH 1920
#define HEIGHT 1080

i32 main(void) {
	se_context *ctx = se_context_create();
	se_window *window = se_window_create(ctx, "Syphax-Engine - Text Example", WIDTH, HEIGHT);
	se_text_handle *text_handle = se_text_handle_create(ctx, 0);
	se_font *font = se_font_load(text_handle, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.f);

	se_window_set_exit_key(window, SE_KEY_ESCAPE);

	while (!se_window_should_close(window)) {
		se_window_tick(window);
		se_render_clear();
		se_text_render(text_handle, font, "yer7am dinek, ch'hal boubli \nMa nedri welou", &s_vec2(0., 0.), &s_vec2(1, 1), .03f);
		se_window_render_screen(window);
	}

	se_text_handle_destroy(text_handle);
	se_window_destroy(window);
	se_context_destroy(ctx);
	return 0;
}
