#version 300 es
precision mediump float;

in vec2 tex_coord;
in vec3 mesh_normal;
in vec4 particle_color;

uniform sampler2D u_texture;
uniform int u_has_texture;
uniform int u_additive;
uniform float u_intensity;

out vec4 out_color;

vec3 light_dir = vec3(1., 1., 0.);

void main() {
	vec4 base = vec4(1.0);
	if (u_has_texture != 0) {
		base = texture(u_texture, tex_coord);
	}
	vec4 color = base * particle_color;
	float intensity = max(u_intensity, 0.0);
	if (intensity > 0.0) {
		color.rgb *= intensity;
	}
	if (color.a <= 0.001) {
		discard;
	}

	float lighting = max(0.2, dot(normalize(light_dir), normalize(mesh_normal)));
	color.rgb *= lighting;
	out_color = color;
}
