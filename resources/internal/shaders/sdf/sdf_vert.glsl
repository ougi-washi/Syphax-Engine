#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 v_uv;

void main() {
	gl_Position = vec4(in_position, 0.0, 1.0);
	v_uv = vec2(in_uv.x, 1.0 - in_uv.y);
}
