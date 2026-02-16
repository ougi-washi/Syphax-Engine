#include "se_text.h"

int main(void) {
	se_text_handle* text = se_text_handle_create(4);
	se_font_handle font = se_font_load(text, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);
	return 0;
}
