#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in mat4 in_instance_mvp;

out vec2 tex_coord;

void main() {
	gl_Position = in_instance_mvp * vec4(in_position, 1.0);
	tex_coord = in_tex_coord;
}
