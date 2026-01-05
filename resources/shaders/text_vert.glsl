#version 330 core

layout(location = 0) in vec2 in_position;  // Quad vertex position (0,0 to 1,1)
layout(location = 1) in vec2 in_texcoord;  // Quad texture coordinates

// Per-instance data
layout(location = 2) in mat4 in_glyph;  // [0] x, y, width, height
                                        // [1] u0, v0, u1, v1
out vec2 tex_coord;

void main() {
    vec4 glyph_rect = in_glyph[0];
    vec4 glyph_uv = in_glyph[1];
    
    vec2 pixel_pos = glyph_rect.xy  * glyph_rect.zw;
    vec2 ndc = (pixel_pos / 1024.) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    tex_coord = mix(glyph_uv.xy, glyph_uv.zw, in_texcoord);
    
    gl_Position = vec4(ndc, 0.0, 1.0);
}
