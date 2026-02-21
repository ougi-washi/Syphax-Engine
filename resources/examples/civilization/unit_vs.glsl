#version 300 es

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in mat4 in_instance_mvp;
layout(location = 7) in mat4 in_unit_data;

out vec2 v_tex_coord;

void main() {
	v_tex_coord = in_tex_coord;
	gl_Position = in_instance_mvp * vec4(in_position, 1.0);
}
