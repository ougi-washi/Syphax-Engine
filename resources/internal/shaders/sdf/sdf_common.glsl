float sdf_op_smooth_union_factor(float a, float b, float amount) {
	float k = max(amount, SDF_MIN_SMOOTH_UNION_AMOUNT);
	return clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
}

float sdf_op_union(float a, float b, float amount) {
	return min(a, b);
}

float sdf_op_smooth_union(float a, float b, float amount) {
	float k = max(amount, SDF_MIN_SMOOTH_UNION_AMOUNT);
	float h = sdf_op_smooth_union_factor(a, b, amount);
	return mix(b, a, h) - k * h * (1.0 - h);
}

float sdf_hash13(vec3 p) {
	return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

vec3 sdf_hash33(vec3 p) {
	return fract(sin(vec3(
		dot(p, vec3(127.1, 311.7, 74.7)),
		dot(p, vec3(269.5, 183.3, 246.1)),
		dot(p, vec3(113.5, 271.9, 124.6)))) * 43758.5453123) * 2.0 - 1.0;
}

float sdf_noise_perlin(vec3 p) {
	vec3 cell = floor(p);
	vec3 local = fract(p);
	vec3 fade = local * local * (3.0 - 2.0 * local);
	float n000 = dot(normalize(sdf_hash33(cell + vec3(0.0, 0.0, 0.0))), local - vec3(0.0, 0.0, 0.0));
	float n100 = dot(normalize(sdf_hash33(cell + vec3(1.0, 0.0, 0.0))), local - vec3(1.0, 0.0, 0.0));
	float n010 = dot(normalize(sdf_hash33(cell + vec3(0.0, 1.0, 0.0))), local - vec3(0.0, 1.0, 0.0));
	float n110 = dot(normalize(sdf_hash33(cell + vec3(1.0, 1.0, 0.0))), local - vec3(1.0, 1.0, 0.0));
	float n001 = dot(normalize(sdf_hash33(cell + vec3(0.0, 0.0, 1.0))), local - vec3(0.0, 0.0, 1.0));
	float n101 = dot(normalize(sdf_hash33(cell + vec3(1.0, 0.0, 1.0))), local - vec3(1.0, 0.0, 1.0));
	float n011 = dot(normalize(sdf_hash33(cell + vec3(0.0, 1.0, 1.0))), local - vec3(0.0, 1.0, 1.0));
	float n111 = dot(normalize(sdf_hash33(cell + vec3(1.0, 1.0, 1.0))), local - vec3(1.0, 1.0, 1.0));
	float nx00 = mix(n000, n100, fade.x);
	float nx10 = mix(n010, n110, fade.x);
	float nx01 = mix(n001, n101, fade.x);
	float nx11 = mix(n011, n111, fade.x);
	float nxy0 = mix(nx00, nx10, fade.y);
	float nxy1 = mix(nx01, nx11, fade.y);
	return mix(nxy0, nxy1, fade.z);
}

float sdf_noise_vornoi(vec3 p) {
	vec3 cell = floor(p);
	vec3 local = fract(p);
	float min_distance = 8.0;
	for (int z = -1; z <= 1; ++z) {
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				vec3 neighbor = vec3(float(x), float(y), float(z));
				vec3 point = neighbor + 0.5 + 0.45 * sdf_hash33(cell + neighbor);
				min_distance = min(min_distance, length(local - point));
			}
		}
	}
	return 0.5 - min_distance;
}

float sdf_noise_texture_triplanar(sampler2D noise_texture, vec3 p) {
	vec3 blend = abs(p);
	blend = max(blend, vec3(0.0001));
	blend /= max(blend.x + blend.y + blend.z, 0.0001);
	float sample_x = texture(noise_texture, p.yz + vec2(0.17, 0.53)).r;
	float sample_y = texture(noise_texture, p.xz + vec2(0.37, 0.11)).r;
	float sample_z = texture(noise_texture, p.xy + vec2(0.71, 0.29)).r;
	return (sample_x * blend.x + sample_y * blend.y + sample_z * blend.z) * 2.0 - 1.0;
}

struct sdf_shading_data {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float roughness;
	float bias;
	float smoothness;
};

struct sdf_shadow_data {
	float softness;
	float bias;
	float samples;
};

struct sdf_surface_data {
	float distance;
	sdf_shading_data shading;
	sdf_shadow_data shadow;
};

sdf_shading_data sdf_make_shading(vec3 ambient, vec3 diffuse, vec3 specular, float roughness, float bias, float smoothness) {
	sdf_shading_data shading;
	shading.ambient = ambient;
	shading.diffuse = diffuse;
	shading.specular = specular;
	shading.roughness = clamp(roughness, 0.0, 1.0);
	shading.bias = clamp(bias, 0.0, 1.0);
	shading.smoothness = max(smoothness, 0.0);
	return shading;
}

sdf_shadow_data sdf_make_shadow(float softness, float bias, int samples) {
	sdf_shadow_data shadow;
	shadow.softness = max(softness, 0.0);
	shadow.bias = max(bias, 0.0);
	shadow.samples = clamp(float(max(samples, 1)), 1.0, float(SDF_MAX_SHADOW_SAMPLES));
	return shadow;
}

sdf_shading_data sdf_mix_shading(sdf_shading_data a, sdf_shading_data b, float t) {
	sdf_shading_data shading;
	shading.ambient = mix(a.ambient, b.ambient, t);
	shading.diffuse = mix(a.diffuse, b.diffuse, t);
	shading.specular = mix(a.specular, b.specular, t);
	shading.roughness = mix(a.roughness, b.roughness, t);
	shading.bias = mix(a.bias, b.bias, t);
	shading.smoothness = mix(a.smoothness, b.smoothness, t);
	return shading;
}

sdf_shadow_data sdf_mix_shadow(sdf_shadow_data a, sdf_shadow_data b, float t) {
	sdf_shadow_data shadow;
	shadow.softness = mix(a.softness, b.softness, t);
	shadow.bias = mix(a.bias, b.bias, t);
	shadow.samples = mix(a.samples, b.samples, t);
	return shadow;
}

float sdf_shading_band(float value, float bias, float smoothness) {
	float center = clamp(bias, 0.0, 1.0);
	float half_width = max(smoothness * 0.5, 0.0001);
	float band_min = clamp(center - half_width, 0.0, 1.0);
	float band_max = clamp(center + half_width, 0.0, 1.0);
	float band_value = clamp(value, 0.0, 1.0);
	if (band_max <= band_min + 0.0001) {
		return band_value >= center ? 1.0 : 0.0;
	}
	return smoothstep(band_min, band_max, band_value);
}

float scene_sdf(vec3 p);
sdf_surface_data scene_surface(vec3 p);

float sdf_shadow_visibility(vec3 ray_origin, vec3 ray_direction, float min_distance, float max_distance, float shadow_softness, int shadow_samples) {
	float visibility = 1.0;
	float safe_shadow_softness = max(shadow_softness, 0.0001);
	int sample_count = clamp(max(shadow_samples, 1), 1, SDF_MAX_SHADOW_SAMPLES);
	float travel = max(min_distance, 0.0);
	float shadow_limit = min(max_distance, SDF_FAR_DISTANCE);
	if (travel >= shadow_limit) {
		return 1.0;
	}
	for (int i = 0; i < SDF_MAX_SHADOW_SAMPLES; ++i) {
		if (i >= sample_count || travel >= shadow_limit) {
			break;
		}
		vec3 sample_position = ray_origin + ray_direction * travel;
		float distance_to_surface = scene_sdf(sample_position);
		if (distance_to_surface < SDF_HIT_EPSILON) {
			return 0.0;
		}
		visibility = min(visibility, safe_shadow_softness * distance_to_surface / max(travel, 0.0001));
		travel += max(distance_to_surface, 0.02);
	}
	return clamp(visibility, 0.0, 1.0);
}

void sdf_apply_directional_light_visibility(
	vec3 normal,
	vec3 view_direction,
	vec3 light_direction,
	vec3 light_color,
	float roughness,
	float shading_bias,
	float shading_smoothness,
	float visibility,
	out vec3 diffuse_light,
	out vec3 specular_light) {
	vec3 dir = normalize(light_direction);
	float diffuse = max(dot(normal, dir), 0.0);
	if (diffuse <= 0.0 || visibility <= 0.0) {
		diffuse_light = vec3(0.0);
		specular_light = vec3(0.0);
		return;
	}
	float diffuse_band = sdf_shading_band(diffuse, shading_bias, shading_smoothness);
	vec3 half_dir = normalize(dir + view_direction);
	float shininess = mix(96.0, 8.0, clamp(roughness, 0.0, 1.0));
	float specular = pow(max(dot(normal, half_dir), 0.0), shininess) * mix(1.0, 0.18, clamp(roughness, 0.0, 1.0));
	float specular_band = sdf_shading_band(specular, shading_bias, shading_smoothness);
	diffuse_light = light_color * diffuse * diffuse_band * visibility * 0.78;
	specular_light = light_color * specular * specular_band * visibility * 0.35;
}

void sdf_apply_point_light_visibility(
	vec3 normal,
	vec3 view_direction,
	vec3 world_position,
	vec3 light_position,
	vec3 light_color,
	float radius,
	float roughness,
	float shading_bias,
	float shading_smoothness,
	float visibility,
	out vec3 diffuse_light,
	out vec3 specular_light) {
	vec3 to_light = light_position - world_position;
	float distance_to_light = length(to_light);
	if (distance_to_light <= 0.0001 || visibility <= 0.0) {
		diffuse_light = vec3(0.0);
		specular_light = vec3(0.0);
		return;
	}
	vec3 dir = to_light / distance_to_light;
	float diffuse = max(dot(normal, dir), 0.0);
	float safe_radius = max(radius, 0.0001);
	float attenuation = clamp(1.0 - distance_to_light / safe_radius, 0.0, 1.0);
	attenuation *= attenuation;
	if (diffuse <= 0.0 || attenuation <= 0.0) {
		diffuse_light = vec3(0.0);
		specular_light = vec3(0.0);
		return;
	}
	float diffuse_band = sdf_shading_band(diffuse, shading_bias, shading_smoothness);
	vec3 half_dir = normalize(dir + view_direction);
	float shininess = mix(96.0, 8.0, clamp(roughness, 0.0, 1.0));
	float specular = pow(max(dot(normal, half_dir), 0.0), shininess) * mix(1.0, 0.18, clamp(roughness, 0.0, 1.0));
	float specular_band = sdf_shading_band(specular, shading_bias, shading_smoothness);
	diffuse_light = light_color * diffuse * diffuse_band * attenuation * visibility * 0.78;
	specular_light = light_color * specular * specular_band * attenuation * visibility * 0.35;
}

float sdf_safe_w(float w) {
	return abs(w) > 0.00001 ? w : (w < 0.0 ? -0.00001 : 0.00001);
}

void sdf_get_view_ray(vec2 uv, out vec3 ray_origin, out vec3 ray_dir) {
	vec2 ndc = vec2(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0);
	vec4 near_clip = vec4(ndc, -1.0, 1.0);
	vec4 far_clip = vec4(ndc, 1.0, 1.0);
	vec4 near_world_h = u_inv_view_projection * near_clip;
	vec4 far_world_h = u_inv_view_projection * far_clip;
	vec3 near_world = near_world_h.xyz / sdf_safe_w(near_world_h.w);
	vec3 far_world = far_world_h.xyz / sdf_safe_w(far_world_h.w);
	ray_origin = near_world;
	ray_dir = normalize(far_world - near_world);
	if (u_use_orthographic == 0) {
		ray_origin = u_camera_position;
		ray_dir = normalize(far_world - u_camera_position);
	}
}

vec3 sdf_sky(vec3 ray_dir) {
	return mix(vec3(0.05, 0.06, 0.08), vec3(0.16, 0.19, 0.24), clamp(ray_dir.y * 0.5 + 0.5, 0.0, 1.0));
}

vec3 sdf_estimate_normal(vec3 p) {
	float e = SDF_HIT_EPSILON;
	float dx = scene_sdf(p + vec3(e, 0.0, 0.0)) - scene_sdf(p - vec3(e, 0.0, 0.0));
	float dy = scene_sdf(p + vec3(0.0, e, 0.0)) - scene_sdf(p - vec3(0.0, e, 0.0));
	float dz = scene_sdf(p + vec3(0.0, 0.0, e)) - scene_sdf(p - vec3(0.0, 0.0, e));
	return normalize(vec3(dx, dy, dz));
}
