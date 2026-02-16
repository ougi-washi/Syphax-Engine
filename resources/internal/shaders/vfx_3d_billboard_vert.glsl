#version 300 es

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in mat4 in_instance_data;

uniform mat4 u_view_proj;
uniform vec3 u_camera_right;
uniform vec3 u_camera_up;

out vec2 tex_coord;
out vec4 particle_color;

void main() {
	vec3 center = in_instance_data[0].xyz;
	float size = in_instance_data[0].w;
	vec4 color = in_instance_data[1];
	float rotation = in_instance_data[2].x;

	float s = sin(rotation);
	float c = cos(rotation);
	vec2 rotated = vec2(
		in_position.x * c - in_position.y * s,
		in_position.x * s + in_position.y * c);

	vec3 world = center + u_camera_right * (rotated.x * size) + u_camera_up * (rotated.y * size);
	gl_Position = u_view_proj * vec4(world, 1.0);
	tex_coord = in_tex_coord;
	particle_color = color;
}
