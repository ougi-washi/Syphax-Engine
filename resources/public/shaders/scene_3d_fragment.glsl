#version 330 core

in vec2 tex_coord;
in vec3 clip_position;
in vec3 clip_normal;

out vec4 frag_color;

uniform sampler2D u_texture;
uniform vec4 u_base_color_factor;
uniform float u_metallic_factor;
uniform float u_roughness_factor;
uniform float u_ao_factor;
uniform vec3 u_emissive_factor;

uniform sampler2D u_metallic_roughness_texture;
uniform sampler2D u_occlusion_texture;
uniform sampler2D u_emissive_texture;

uniform bool u_has_metallic_roughness_texture;
uniform bool u_has_occlusion_texture;
uniform bool u_has_emissive_texture;

const float PI = 3.14159265359;
const float EPSILON = 1e-5;

const int LIGHT_COUNT = 3;
const vec3 LIGHT_DIRECTIONS[LIGHT_COUNT] = vec3[](
	vec3(0.35, 0.82, 0.45),
	vec3(-0.68, 0.40, 0.36),
	vec3(0.22, -0.55, -0.81)
);
const vec3 LIGHT_COLORS[LIGHT_COUNT] = vec3[](
	vec3(6.5, 6.2, 5.9),
	vec3(2.2, 2.6, 3.0),
	vec3(0.45, 0.50, 0.62)
);

float se_distribution_ggx(float n_dot_h, float roughness) {
	float roughness2 = roughness * roughness;
	float a2 = roughness2 * roughness2;
	float denom = (n_dot_h * n_dot_h) * (a2 - 1.0) + 1.0;
	return a2 / max(PI * denom * denom, EPSILON);
}

float se_geometry_schlick_ggx(float n_dot_x, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return n_dot_x / max(n_dot_x * (1.0 - k) + k, EPSILON);
}

float se_geometry_smith(float n_dot_v, float n_dot_l, float roughness) {
	float ggx_v = se_geometry_schlick_ggx(n_dot_v, roughness);
	float ggx_l = se_geometry_schlick_ggx(n_dot_l, roughness);
	return ggx_v * ggx_l;
}

vec3 se_fresnel_schlick(float cos_theta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 se_fresnel_schlick_roughness(float cos_theta, vec3 f0, float roughness) {
	vec3 max_reflectance = max(vec3(1.0 - roughness), f0);
	return f0 + (max_reflectance - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float se_defaulted_scalar(float value, float fallback) {
	return (abs(value) <= EPSILON) ? fallback : value;
}

vec4 se_defaulted_color(vec4 value, vec4 fallback) {
	return (length(value) <= EPSILON) ? fallback : value;
}

void main() {
	vec4 base_sample = texture(u_texture, tex_coord);
	vec4 base_factor = se_defaulted_color(u_base_color_factor, vec4(1.0));
	vec3 albedo = pow(clamp(base_sample.rgb * base_factor.rgb, 0.0, 1.0), vec3(2.2));
	float alpha = clamp(base_sample.a * base_factor.a, 0.0, 1.0);

	float metallic = clamp(se_defaulted_scalar(u_metallic_factor, 0.04), 0.0, 1.0);
	float roughness = clamp(se_defaulted_scalar(u_roughness_factor, 0.58), 0.04, 1.0);
	float ao = clamp(se_defaulted_scalar(u_ao_factor, 1.0), 0.0, 1.0);

	if (u_has_metallic_roughness_texture) {
		vec4 metallic_roughness_sample = texture(u_metallic_roughness_texture, tex_coord);
		metallic *= clamp(metallic_roughness_sample.b, 0.0, 1.0);
		roughness *= clamp(metallic_roughness_sample.g, 0.04, 1.0);
	}

	if (u_has_occlusion_texture) {
		ao *= clamp(texture(u_occlusion_texture, tex_coord).r, 0.0, 1.0);
	}

	vec3 n = normalize(clip_normal);
	if (dot(n, n) <= EPSILON) {
		n = vec3(0.0, 0.0, 1.0);
	}
	if (!gl_FrontFacing) {
		n *= -1.0;
	}

	vec3 v = normalize(-clip_position);
	if (dot(v, v) <= EPSILON) {
		v = vec3(0.0, 0.0, 1.0);
	}

	vec3 f0 = mix(vec3(0.04), albedo, metallic);
	vec3 radiance = vec3(0.0);

	for (int i = 0; i < LIGHT_COUNT; i++) {
		vec3 l = normalize(LIGHT_DIRECTIONS[i]);
		vec3 h = normalize(v + l);

		float n_dot_l = max(dot(n, l), 0.0);
		float n_dot_v = max(dot(n, v), 0.0);
		float n_dot_h = max(dot(n, h), 0.0);
		float h_dot_v = max(dot(h, v), 0.0);

		if (n_dot_l <= 0.0 || n_dot_v <= 0.0) {
			continue;
		}

		float d = se_distribution_ggx(n_dot_h, roughness);
		float g = se_geometry_smith(n_dot_v, n_dot_l, roughness);
		vec3 f = se_fresnel_schlick(h_dot_v, f0);

		vec3 specular = (d * g * f) / max(4.0 * n_dot_v * n_dot_l, EPSILON);
		vec3 ks = f;
		vec3 kd = (vec3(1.0) - ks) * (1.0 - metallic);
		vec3 diffuse = kd * albedo / PI;

		radiance += (diffuse + specular) * LIGHT_COLORS[i] * n_dot_l;
	}

	float horizon = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);
	vec3 sky_tint = mix(vec3(0.03, 0.03, 0.035), vec3(0.22, 0.27, 0.34), horizon);
	vec3 ground_tint = vec3(0.015, 0.012, 0.010);
	vec3 env_light = mix(ground_tint, sky_tint, horizon);

	vec3 ambient_f = se_fresnel_schlick_roughness(max(dot(n, v), 0.0), f0, roughness);
	vec3 ambient_kd = (vec3(1.0) - ambient_f) * (1.0 - metallic);
	vec3 ambient_diffuse = ambient_kd * env_light * albedo;
	vec3 ambient_specular = ambient_f * env_light * (0.12 + (1.0 - roughness) * 0.55);
	vec3 ambient = (ambient_diffuse + ambient_specular) * ao;

	vec3 emissive_factor = u_emissive_factor;
	if (u_has_emissive_texture && length(emissive_factor) <= EPSILON) {
		emissive_factor = vec3(1.0);
	}
	vec3 emissive = max(emissive_factor, vec3(0.0));
	if (u_has_emissive_texture) {
		emissive *= pow(texture(u_emissive_texture, tex_coord).rgb, vec3(2.2));
	}

	vec3 color = ambient + radiance + emissive;
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	frag_color = vec4(color, alpha);
}
