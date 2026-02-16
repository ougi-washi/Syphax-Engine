#include "se_graphics.h"

int main(void) {
	se_render_init();
	se_render_set_background_color(s_vec4(0.05f, 0.08f, 0.12f, 1.0f));
	se_render_clear();
	se_render_set_blending(1);
	se_render_unbind_framebuffer();
	return 0;
}
