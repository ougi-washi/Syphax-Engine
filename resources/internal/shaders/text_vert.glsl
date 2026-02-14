#version 300 es

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

// Per-instance data
// in_glyph[0] = {xoff, yoff, width, height}
// in_glyph[1] = {s0, t0, s1, t1}
// in_glyph[2] = {pen_x, pen_y, 0, 0}
layout(location = 2) in mat4 in_glyph;
                                       
out vec2 tex_coord;
out vec4 glyph_uv;

void main() {
    vec4 glyph_rect = in_glyph[0];
    glyph_uv = in_glyph[1];
    vec2 pen_pos = in_glyph[2].xy;
    
    // glyph_rect: {xoff, yoff, width, height}
    vec2 glyph_offset = glyph_rect.xy;
    vec2 glyph_size = glyph_rect.zw;
    
    // Scale unit quad [-1,1] by glyph size, then add offset and pen position
    // in_position is [-1,1], so we divide by 2 to get unit quad [0,1] size
    vec2 vertex_pos = in_position * glyph_size * 0.5;
    
    gl_Position = vec4(pen_pos + glyph_offset + vertex_pos, 0.0, 1.0);
    tex_coord = in_texcoord;
}
