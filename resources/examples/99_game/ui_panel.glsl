#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

void main() {
	float edge = min(min(tex_coord.x, tex_coord.y), min(1.0 - tex_coord.x, 1.0 - tex_coord.y));
	float fade = smoothstep(0.00, 0.08, edge);
	vec3 top = vec3(0.06, 0.10, 0.13);
	vec3 bottom = vec3(0.03, 0.05, 0.07);
	vec3 color = mix(bottom, top, tex_coord.y);
	frag_color = vec4(color, 0.48 * fade);
}
