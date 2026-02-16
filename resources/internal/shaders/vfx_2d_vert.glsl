#version 300 es

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in mat3 in_instance_transform;
layout(location = 5) in mat4 in_instance_buffer;

out vec2 tex_coord;
out vec4 particle_color;

void main() {
	vec3 transformed = in_instance_transform * vec3(in_position, 1.0);
	gl_Position = vec4(transformed.xy, 0.0, 1.0);
	tex_coord = in_tex_coord;
	particle_color = in_instance_buffer[0];
}
