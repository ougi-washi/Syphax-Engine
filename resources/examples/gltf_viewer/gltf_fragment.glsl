#version 330 core

in vec2 v_tex_coord;
in vec3 v_clip_position;
in vec3 v_clip_normal;

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

uniform int u_has_metallic_roughness_texture;
uniform int u_has_occlusion_texture;
uniform int u_has_emissive_texture;
uniform int u_alpha_mode;
uniform float u_alpha_cutoff;

uniform vec3 u_light_directions[3];
uniform vec3 u_light_colors[3];
uniform vec3 u_ambient_sky;
uniform vec3 u_ambient_ground;
uniform float u_ambient_strength;
uniform float u_exposure;

const float SE_PI = 3.14159265359;
const float SE_EPSILON = 1e-5;
const vec3 SE_FALLBACK_LIGHT_DIRECTIONS[3] = vec3[](
	vec3(0.35, 0.82, 0.45),
	vec3(-0.68, 0.40, 0.36),
	vec3(0.22, -0.55, -0.81)
);
const vec3 SE_FALLBACK_LIGHT_COLORS[3] = vec3[](
	vec3(7.20, 6.70, 6.15),
	vec3(2.40, 2.85, 3.30),
	vec3(0.55, 0.60, 0.72)
);
const vec3 SE_FALLBACK_AMBIENT_SKY = vec3(0.21, 0.24, 0.28);
const vec3 SE_FALLBACK_AMBIENT_GROUND = vec3(0.018, 0.015, 0.012);

float se_distribution_ggx(float n_dot_h, float roughness) {
	float roughness2 = roughness * roughness;
	float alpha2 = roughness2 * roughness2;
	float denom = (n_dot_h * n_dot_h) * (alpha2 - 1.0) + 1.0;
	return alpha2 / max(SE_PI * denom * denom, SE_EPSILON);
}

float se_geometry_schlick_ggx(float n_dot_x, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return n_dot_x / max(n_dot_x * (1.0 - k) + k, SE_EPSILON);
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

vec3 se_aces_tonemap(vec3 color) {
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 se_get_light_direction(int index) {
	vec3 light_direction = u_light_directions[index];
	if (dot(light_direction, light_direction) <= SE_EPSILON) {
		light_direction = SE_FALLBACK_LIGHT_DIRECTIONS[index];
	}
	return normalize(light_direction);
}

vec3 se_get_light_color(int index) {
	vec3 light_color = u_light_colors[index];
	if (dot(light_color, light_color) <= SE_EPSILON) {
		light_color = SE_FALLBACK_LIGHT_COLORS[index];
	}
	return max(light_color, vec3(0.0));
}

vec3 se_get_ambient_sky(void) {
	vec3 ambient_sky = u_ambient_sky;
	if (dot(ambient_sky, ambient_sky) <= SE_EPSILON) {
		ambient_sky = SE_FALLBACK_AMBIENT_SKY;
	}
	return max(ambient_sky, vec3(0.0));
}

vec3 se_get_ambient_ground(void) {
	vec3 ambient_ground = u_ambient_ground;
	if (dot(ambient_ground, ambient_ground) <= SE_EPSILON) {
		ambient_ground = SE_FALLBACK_AMBIENT_GROUND;
	}
	return max(ambient_ground, vec3(0.0));
}

float se_get_ambient_strength(void) {
	if (u_ambient_strength <= SE_EPSILON) {
		return 1.0;
	}
	return max(u_ambient_strength, 0.0);
}

float se_get_exposure(void) {
	if (u_exposure <= SE_EPSILON) {
		return 1.0;
	}
	return max(u_exposure, 0.01);
}

void main() {
	vec4 base_sample = texture(u_texture, v_tex_coord);
	vec4 base_color_factor = clamp(u_base_color_factor, 0.0, 1.0);
	vec3 albedo = pow(clamp(base_sample.rgb * base_color_factor.rgb, 0.0, 1.0), vec3(2.2));
	float alpha = clamp(base_sample.a * base_color_factor.a, 0.0, 1.0);

	if (u_alpha_mode == 1 && alpha < u_alpha_cutoff) {
		discard;
	}

	float metallic = clamp(u_metallic_factor, 0.0, 1.0);
	float roughness = clamp(u_roughness_factor, 0.04, 1.0);
	if (u_has_metallic_roughness_texture != 0) {
		vec4 metallic_roughness_sample = texture(u_metallic_roughness_texture, v_tex_coord);
		metallic *= clamp(metallic_roughness_sample.b, 0.0, 1.0);
		roughness *= clamp(metallic_roughness_sample.g, 0.04, 1.0);
	}

	float ao = 1.0;
	if (u_has_occlusion_texture != 0) {
		float occlusion_sample = clamp(texture(u_occlusion_texture, v_tex_coord).r, 0.0, 1.0);
		ao = mix(1.0, occlusion_sample, clamp(u_ao_factor, 0.0, 1.0));
	}

	vec3 n = normalize(v_clip_normal);
	if (dot(n, n) <= SE_EPSILON) {
		n = vec3(0.0, 0.0, 1.0);
	}
	if (!gl_FrontFacing) {
		n *= -1.0;
	}

	vec3 v = normalize(-v_clip_position);
	if (dot(v, v) <= SE_EPSILON) {
		v = vec3(0.0, 0.0, 1.0);
	}

	vec3 f0 = mix(vec3(0.04), albedo, metallic);
	vec3 direct_lighting = vec3(0.0);

	for (int light_index = 0; light_index < 3; light_index++) {
		vec3 l = se_get_light_direction(light_index);
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

		vec3 specular = (d * g * f) / max(4.0 * n_dot_v * n_dot_l, SE_EPSILON);
		vec3 specular_weight = f;
		vec3 diffuse_weight = (vec3(1.0) - specular_weight) * (1.0 - metallic);
		vec3 diffuse = diffuse_weight * albedo / SE_PI;

		direct_lighting += (diffuse + specular) * se_get_light_color(light_index) * n_dot_l;
	}

	float horizon = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);
	vec3 ambient_probe = mix(se_get_ambient_ground(), se_get_ambient_sky(), horizon);
	ambient_probe *= se_get_ambient_strength();

	vec3 ambient_fresnel = se_fresnel_schlick_roughness(max(dot(n, v), 0.0), f0, roughness);
	vec3 ambient_diffuse_weight = (vec3(1.0) - ambient_fresnel) * (1.0 - metallic);
	vec3 ambient_diffuse = ambient_diffuse_weight * albedo * ambient_probe;
	vec3 ambient_specular = ambient_fresnel * ambient_probe * (0.12 + (1.0 - roughness) * 0.55);
	vec3 ambient_lighting = (ambient_diffuse + ambient_specular) * ao;

	vec3 emissive = max(u_emissive_factor, vec3(0.0));
	if (u_has_emissive_texture != 0) {
		emissive *= pow(texture(u_emissive_texture, v_tex_coord).rgb, vec3(2.2));
	}

	vec3 hdr_color = ambient_lighting + direct_lighting + emissive;
	vec3 ldr_color = se_aces_tonemap(hdr_color * se_get_exposure());
	vec3 final_color = pow(ldr_color, vec3(1.0 / 2.2));

	if (u_alpha_mode != 2) {
		alpha = 1.0;
	}

	frag_color = vec4(final_color, alpha);
}
