#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in mat4 in_instance_transform;
layout(location = 6) in mat4 in_instance_buffer;

uniform vec2 u_texture_size;
uniform vec2 u_scale;
uniform vec2 u_position;

out vec2 tex_coord;

void main() {
    vec2 new_position = vec2(vec4(in_position, 1, 1) * in_instance_transform).xy;

    new_position *= u_scale;
    new_position += u_position;

    gl_Position = vec4(new_position, 0, 1);
    tex_coord = in_tex_coord;
}
