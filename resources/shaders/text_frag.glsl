#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

uniform sampler2D u_atlas_texture;

void main()
{
    frag_color = vec4(texture(u_atlas_texture, tex_coord).r);
}

