#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

uniform sampler2D u_atlas_texture;

void main()
{
    fragColor = vec4(texture(u_atlas_texture, texCoord).r) * color;
}

