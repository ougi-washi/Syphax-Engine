#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in mat4 in_instance_mvp;

out vec2 v_tex_coord;
out vec3 v_clip_position;
out vec3 v_clip_normal;

void main() {
	vec4 clip_position = in_instance_mvp * vec4(in_position, 1.0);
	gl_Position = clip_position;

	mat3 normal_matrix = mat3(transpose(inverse(in_instance_mvp)));
	v_clip_normal = normalize(normal_matrix * in_normal);
	v_clip_position = clip_position.xyz;
	v_tex_coord = in_tex_coord;
}
