#version 300 es

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

uniform mat4 u_mvp;

void main() {
	gl_Position = u_mvp * vec4(in_position, 1.0);
}
