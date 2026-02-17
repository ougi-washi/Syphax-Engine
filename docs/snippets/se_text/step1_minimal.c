#include "se_text.h"

int main(void) {
	se_font_handle font = se_font_load(SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);
	(void)font;
	return 0;
}
