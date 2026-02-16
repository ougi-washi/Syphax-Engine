#version 300 es

precision mediump float;
precision mediump int;

in vec2 tex_coord;
in vec4 instance_color;

out vec4 frag_color;

void main() {
	frag_color = instance_color;
}
