#version 300 es

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in mat4 in_instance_mvp;
layout(location = 7) in vec4 in_instance_color;

out vec2 tex_coord;
out vec4 particle_color;

void main() {
	gl_Position = in_instance_mvp * vec4(in_position, 1.0);
	tex_coord = in_tex_coord;
	particle_color = in_instance_color;
}
