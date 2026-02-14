#version 300 es

precision mediump float;
precision mediump int;

in vec2 tex_coord;
in vec4 glyph_uv;

out vec4 frag_color;

uniform sampler2D u_atlas_texture;

void main() {
    vec2 final_uv = glyph_uv.xy + tex_coord * (glyph_uv.zw - glyph_uv.xy);
    float alpha = texture(u_atlas_texture, final_uv).r;
    if (alpha < 0.01) discard;
    frag_color = vec4(1.0, 1.0, 1.0, alpha);
}
