#version 300 es

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in mat4 in_instance_mvp;

out vec2 tex_coord;
out vec3 clip_position;
out vec3 clip_normal;

void main() {
	vec4 clip = in_instance_mvp * vec4(in_position, 1.0);
	gl_Position = clip;

	mat3 normal_matrix = mat3(transpose(inverse(in_instance_mvp)));
	clip_normal = normalize(normal_matrix * in_normal);
	clip_position = clip.xyz;
	tex_coord = in_tex_coord;
}
