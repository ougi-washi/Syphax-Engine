#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

uniform vec2 u_texture_size;
uniform vec2 u_scale;
uniform vec2 u_position;

out vec2 tex_coord;

void main() {
    vec2 new_position = in_position.xy;

    new_position *= u_scale;
    new_position += u_position;
    
    gl_Position = vec4(new_position, 0, 1);
    tex_coord = in_tex_coord;
}
