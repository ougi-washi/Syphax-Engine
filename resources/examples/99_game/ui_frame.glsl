#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

void main() {
	float bx = smoothstep(0.00, 0.03, tex_coord.x) * smoothstep(0.00, 0.03, 1.0 - tex_coord.x);
	float by = smoothstep(0.00, 0.03, tex_coord.y) * smoothstep(0.00, 0.03, 1.0 - tex_coord.y);
	float center_mask = bx * by;
	float border = 1.0 - center_mask;
	vec3 border_color = vec3(0.74, 0.86, 0.92);
	frag_color = vec4(border_color, border * 0.95);
}
