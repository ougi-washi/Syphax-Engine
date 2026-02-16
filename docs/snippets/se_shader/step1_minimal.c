#include "se_shader.h"

int main(void) {
	se_shader_handle shader = se_shader_load(
		SE_RESOURCE_EXAMPLE("scene2d/scene2d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene2d/scene2d_frag.glsl")
	);
	se_shader_use(shader, 1, 1);
	return 0;
}
