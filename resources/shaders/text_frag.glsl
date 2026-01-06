#version 330 core

in vec2 tex_coord;
in vec4 glyph_rect;
in vec4 glyph_uv;

out vec4 frag_color;

uniform sampler2D u_atlas_texture;

void main() {
    vec2 new_tex_coord = tex_coord + glyph_uv.xy;
    if (new_tex_coord.x > glyph_uv.z || new_tex_coord.y > glyph_uv.w) {
        discard;
    }
    float alpha = texture(u_atlas_texture, new_tex_coord).r;
    frag_color = vec4(1., 1., 1., alpha);
}
