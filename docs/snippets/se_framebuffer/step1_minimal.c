#include "se_framebuffer.h"

int main(void) {
	s_vec2 size = s_vec2(1280.0f, 720.0f);
	se_framebuffer_handle framebuffer = se_framebuffer_create(&size);
	se_framebuffer_bind(framebuffer);
	return 0;
}
