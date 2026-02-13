#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in mat4 in_instance_mvp;

out vec3 v_local_position;
out vec3 v_local_normal;
out vec2 v_uv;

void main() {
	gl_Position = in_instance_mvp * vec4(in_position, 1.0);
	v_local_position = in_position;
	v_local_normal = normalize(in_normal);
	v_uv = in_tex_coord;
}
