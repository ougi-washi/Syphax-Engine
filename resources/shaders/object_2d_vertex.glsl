#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;

uniform mat3 u_transform;

out vec2 tex_coord;

void main() {
    vec3 transformed = u_transform * vec3(in_position, 1.0);
    vec2 new_position = transformed.xy;

    gl_Position = vec4(new_position, 0, 1);
    tex_coord = in_tex_coord;
}
