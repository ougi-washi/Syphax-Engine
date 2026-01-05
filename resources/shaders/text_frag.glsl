#version 330 core

in vec2 tex_coord;
in vec2 text_position;
in vec2 text_uv;
out vec4 frag_color;

uniform sampler2D u_atlas_texture;

void main(){
    vec2 atlas_coord = text_uv + (tex_coord * vec2(text_position));
    float alpha = texture(u_atlas_texture, atlas_coord).r;
    frag_color = vec4(alpha);
}
