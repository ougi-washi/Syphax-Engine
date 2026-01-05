#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

uniform sampler2D u_atlas_texture;

void main() {
    float alpha = texture(u_atlas_texture, tex_coord).r;
    frag_color = vec4(1., 1., 1., alpha);
}
