#version 300 es

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in mat3 in_instance_transform;
layout(location = 5) in mat4 in_instance_buffer;

out vec2 tex_coord;
out vec4 instance_color;
out vec4 instance_border_color;
out vec3 instance_border_info;
out vec4 instance_effect;

void main() {
	vec3 transformed = in_instance_transform * vec3(in_position, 1.0);
	vec2 new_position = transformed.xy;

	gl_Position = vec4(new_position, 0, 1);
	tex_coord = in_tex_coord;
	instance_color = in_instance_buffer[0];
	instance_border_color = in_instance_buffer[1];
	instance_border_info = in_instance_buffer[2].xyz;
	instance_effect = in_instance_buffer[3];
}
