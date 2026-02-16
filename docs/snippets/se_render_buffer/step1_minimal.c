#include "se_render_buffer.h"

int main(void) {
	se_render_buffer_handle buffer = se_render_buffer_create(1280, 720, SE_RESOURCE_EXAMPLE("scene2d/scene2d_post_frag.glsl"));
	se_render_buffer_bind(buffer);
	return 0;
}
