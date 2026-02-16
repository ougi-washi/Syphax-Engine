// Syphax-Engine - Ougi Washi

#include "se_graphics.h"
#include "se_text.h"
#include "se_window.h"

#include <stdio.h>

// What you will learn: create a window, draw text, and react to one key.
// Try this: change the text string, font size, or background colors below.
int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Hello Text", 1280, 720);
	if (window == S_HANDLE_NULL) {
		printf("hello_text :: window creation failed (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_text_handle* text = se_text_handle_create(0);
	se_font_handle font = se_font_load(text, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 34.0f);
	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);

	b8 warm_palette = true;
	printf("hello_text controls:\n");
	printf("  Esc: quit\n");
	printf("  Space: switch background color\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);

		if (se_window_is_key_pressed(window, SE_KEY_SPACE)) {
			warm_palette = !warm_palette;
		}
		if (warm_palette) {
			se_render_set_background_color(s_vec4(0.10f, 0.11f, 0.14f, 1.0f));
		} else {
			se_render_set_background_color(s_vec4(0.04f, 0.12f, 0.11f, 1.0f));
		}

		se_render_clear();
		se_text_render(text, font, "Ahlan!", &s_vec2(-0.30f, 0.16f), &s_vec2(1.0f, 1.0f), 0.03f);
		se_text_render(text, font, "Press Space to paint the mood.", &s_vec2(-0.36f, 0.04f), &s_vec2(1.0f, 1.0f), 0.024f);
		se_text_render(text, font, "Press Esc to close.", &s_vec2(-0.22f, -0.08f), &s_vec2(1.0f, 1.0f), 0.022f);

		se_window_end_frame(window);
	}

	se_text_handle_destroy(text);
	se_context_destroy(context);
	return 0;
}
