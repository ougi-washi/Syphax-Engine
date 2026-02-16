#include "se_text.h"

int main(void) {
	se_text_handle* text = se_text_handle_create(4);
	se_font_handle font = se_font_load(text, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);
	s_vec2 pos = s_vec2(0.0f, 0.0f);
	s_vec2 size = s_vec2(1.0f, 1.0f);
	se_text_render(text, font, "Hello", &pos, &size, 0.03f);
	se_text_render(text, font, "Runtime label", &pos, &size, 0.03f);
	return 0;
}
