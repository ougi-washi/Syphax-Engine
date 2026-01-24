#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

uniform sampler2D u_texture;
uniform sampler2D u_prev;

void main() {
    //vec4 prev_color = texture(u_prev, tex_coord);
    //vec4 new_color = texture(u_texture, tex_coord);
    //frag_color = mix(prev_color, new_color, new_color.a);
    tex_coord.y = 1. - tex_coord.y;
    frag_color = texture(u_prev, tex_coord);
}
