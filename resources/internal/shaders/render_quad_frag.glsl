#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

uniform sampler2D u_texture;

void main() {
    frag_color = texture(u_texture, tex_coord);
}
