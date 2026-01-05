#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in mat4 in_text_buffer;

out vec4 color;
out vec2 tex_coord;
out vec2 text_position;
out vec2 text_uv;

void main(){
    gl_Position = vec4(in_position, 0, 1);
    tex_coord = in_tex_coord;
    text_position = in_text_buffer[0].xy;
    text_uv = in_text_buffer[0].zw;
}

