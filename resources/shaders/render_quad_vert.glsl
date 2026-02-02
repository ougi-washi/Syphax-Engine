#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;

out vec2 tex_coord;

void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
    tex_coord = vec2(in_tex_coord.x, 1.0 - in_tex_coord.y);
}

