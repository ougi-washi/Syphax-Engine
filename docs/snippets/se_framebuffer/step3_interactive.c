#include "se_framebuffer.h"

int main(void) {
	s_vec2 size = s_vec2(1280.0f, 720.0f);
	se_framebuffer_handle framebuffer = se_framebuffer_create(&size);
	se_framebuffer_bind(framebuffer);
	s_vec2 resized = s_vec2(1920.0f, 1080.0f);
	se_framebuffer_set_size(framebuffer, &resized);
	u32 texture_id = 0;
	se_framebuffer_get_texture_id(framebuffer, &texture_id);
	(void)texture_id;
	se_framebuffer_unbind(framebuffer);
	se_framebuffer_cleanup(framebuffer);
	return 0;
}
