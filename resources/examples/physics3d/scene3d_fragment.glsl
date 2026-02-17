#version 330 core

out vec4 frag_color;

in vec2 tex_coord;
in vec3 normal;

void main() {
	frag_color = vec4(tex_coord, 0.0, 1.0);
}
