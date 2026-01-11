#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

uniform vec3 u_color;

void main() {
    frag_color = vec4(u_color, .7);
}
