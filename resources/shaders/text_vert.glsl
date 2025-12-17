#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_tex_coord;
layout (location = 2) in vec4 in_color;

out vec4 color;
out vec2 tex_coord;

uniform mat4 u_view_projection_mat;

void main()
{
    gl_Position = u_view_projection_mat * vec4(in_position, 1.0);
    color = in_color;
    tex_coord = in_tex_coord;
}

