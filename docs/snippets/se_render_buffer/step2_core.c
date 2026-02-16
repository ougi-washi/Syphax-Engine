#include "se_render_buffer.h"

int main(void) {
	se_render_buffer_handle buffer = se_render_buffer_create(1280, 720, SE_RESOURCE_EXAMPLE("scene2d/scene2d_post_frag.glsl"));
	se_render_buffer_bind(buffer);
	s_vec2 scale = s_vec2(0.9f, 0.9f);
	s_vec2 position = s_vec2(0.0f, 0.0f);
	se_render_buffer_set_scale(buffer, &scale);
	se_render_buffer_set_position(buffer, &position);
	return 0;
}
