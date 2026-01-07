#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

// Per-instance data
layout(location = 2) in mat4 in_glyph;
                                      
out vec2 tex_coord;
out vec4 glyph_rect;
out vec4 glyph_uv;

void main() {
    glyph_rect = in_glyph[0];
    glyph_uv = in_glyph[1];
    vec2 offset = in_glyph[2].xy;
    gl_Position = vec4(in_position + offset - glyph_rect.xy, 0.0, 1.0);
    tex_coord = in_texcoord;
}
