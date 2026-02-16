#include "se_shader.h"

int main(void) {
	se_shader_handle shader = se_shader_load(
		SE_RESOURCE_EXAMPLE("scene2d/scene2d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene2d/scene2d_frag.glsl")
	);
	se_shader_use(shader, 1, 1);
	se_shader_set_float(shader, "u_time", 0.2f);
	s_vec4 tint = s_vec4(0.2f, 0.7f, 0.8f, 1.0f);
	se_shader_set_vec4(shader, "u_tint", &tint);
	se_shader_reload_if_changed(shader);
	i32 location = se_shader_get_uniform_location(shader, "u_tint");
	(void)location;
	se_shader_destroy(shader);
	return 0;
}
