#version 330 core

in vec3 v_local_position;
in vec3 v_local_normal;
in vec2 v_uv;

out vec4 frag_color;

uniform vec3 u_team_color;
uniform float u_ambient_boost;
uniform float u_specular_power;
uniform float u_emissive_strength;
uniform float u_time;

void main() {
	vec3 n = normalize(v_local_normal);
	if (dot(n, n) < 0.0001) {
		n = vec3(0.0, 1.0, 0.0);
	}

	vec3 l0 = normalize(vec3(0.44, 0.88, 0.16));
	vec3 l1 = normalize(vec3(-0.58, 0.42, -0.69));
	vec3 v = normalize(vec3(0.15, 0.70, 0.35) - v_local_position * 0.18);

	float d0 = max(dot(n, l0), 0.0);
	float d1 = max(dot(n, l1), 0.0);

	vec3 h0 = normalize(l0 + v);
	vec3 h1 = normalize(l1 + v);
	float s0 = pow(max(dot(n, h0), 0.0), 24.0) * u_specular_power;
	float s1 = pow(max(dot(n, h1), 0.0), 18.0) * u_specular_power;

	float rim = pow(1.0 - max(dot(n, v), 0.0), 2.2);
	float pulse = 0.55 + 0.45 * sin(u_time * 2.4 + v_uv.x * 8.0 + v_uv.y * 5.0);
	float hemi = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);

	vec3 sky = vec3(0.35, 0.43, 0.56);
	vec3 ground = vec3(0.06, 0.07, 0.08);
	vec3 ambient = mix(ground, sky, hemi) * u_team_color * (0.42 + u_ambient_boost * 0.75);
	vec3 diffuse = u_team_color * (d0 * 0.95 + d1 * 0.42);
	vec3 spec = vec3(s0 + s1);
	vec3 emissive = u_team_color * (rim * pulse * u_emissive_strength);

	vec3 color = ambient + diffuse + spec + emissive;
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	frag_color = vec4(color, 1.0);
}
