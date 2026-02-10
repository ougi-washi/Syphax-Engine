#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

void main() {
    float borders_mask = smoothstep(.9, 1, tex_coord.x);
    borders_mask = max(smoothstep(.9, 1, tex_coord.y), borders_mask);
    borders_mask = max(smoothstep(.1, .0, tex_coord.x), borders_mask);
    borders_mask = max(smoothstep(.1, .0, tex_coord.y), borders_mask);
    frag_color = vec4(1., 1., 1., borders_mask);
}
