#version 300 es

precision mediump float;
precision mediump int;

in vec3 v_world_position;
out vec4 frag_color;

uniform sampler3D u_volume_texture;
uniform mat4 u_model_inverse;
uniform vec3 u_volume_size;
uniform vec3 u_voxel_size;
uniform vec3 u_camera_position;
uniform float u_surface_epsilon;
uniform float u_min_step;
uniform int u_max_steps;

const vec3 k_box_min = vec3(-1.0);
const vec3 k_box_max = vec3(1.0);

vec4 se_sample_volume(vec3 local_position) {
	vec3 uv = local_position * 0.5 + 0.5;
	return texture(u_volume_texture, clamp(uv, 0.0, 1.0));
}

float se_sample_sdf(vec3 local_position) {
	return se_sample_volume(local_position).a;
}

vec3 se_sign_not_zero(vec3 value) {
	return vec3(
		value.x < 0.0 ? -1.0 : 1.0,
		value.y < 0.0 ? -1.0 : 1.0,
		value.z < 0.0 ? -1.0 : 1.0
	);
}

bool se_intersect_aabb(vec3 ray_origin, vec3 ray_direction, out float out_t_near, out float out_t_far) {
	vec3 ray_safe = se_sign_not_zero(ray_direction) * max(abs(ray_direction), vec3(0.000001));
	vec3 ray_inv = 1.0 / ray_safe;
	vec3 t0 = (k_box_min - ray_origin) * ray_inv;
	vec3 t1 = (k_box_max - ray_origin) * ray_inv;
	vec3 tmin = min(t0, t1);
	vec3 tmax = max(t0, t1);
	out_t_near = max(max(tmin.x, tmin.y), tmin.z);
	out_t_far = min(min(tmax.x, tmax.y), tmax.z);
	return out_t_far >= max(out_t_near, 0.0);
}

vec3 se_estimate_normal(vec3 local_position) {
	vec3 texel_uv = 1.0 / vec3(textureSize(u_volume_texture, 0));
	vec3 local_epsilon = max(texel_uv * 2.0, vec3(0.001));

	float dx = se_sample_sdf(local_position + vec3(local_epsilon.x, 0.0, 0.0))
		- se_sample_sdf(local_position - vec3(local_epsilon.x, 0.0, 0.0));
	float dy = se_sample_sdf(local_position + vec3(0.0, local_epsilon.y, 0.0))
		- se_sample_sdf(local_position - vec3(0.0, local_epsilon.y, 0.0));
	float dz = se_sample_sdf(local_position + vec3(0.0, 0.0, local_epsilon.z))
		- se_sample_sdf(local_position - vec3(0.0, 0.0, local_epsilon.z));

	vec3 normal = vec3(dx, dy, dz);
	float normal_len = length(normal);
	if (normal_len < 0.00001) {
		return vec3(0.0, 0.0, 1.0);
	}
	return normal / normal_len;
}

void main() {
	vec3 ray_dir_world = normalize(v_world_position - u_camera_position);
	vec3 ray_origin_local = (u_model_inverse * vec4(u_camera_position, 1.0)).xyz;
	vec3 ray_dir_local_raw = (u_model_inverse * vec4(ray_dir_world, 0.0)).xyz;
	float ray_dir_local_len = length(ray_dir_local_raw);
	if (ray_dir_local_len < 0.000001) {
		discard;
	}
	vec3 ray_dir_local = ray_dir_local_raw / ray_dir_local_len;

	float t_near = 0.0;
	float t_far = 0.0;
	if (!se_intersect_aabb(ray_origin_local, ray_dir_local, t_near, t_far)) {
		discard;
	}

	float t = max(t_near, 0.0);
	vec3 hit_local = vec3(0.0);
	vec3 hit_color = vec3(1.0);
	bool hit = false;
	float best_abs_sdf = 1e20;
	vec3 best_local = vec3(0.0);
	vec3 best_color = vec3(1.0);
	float best_t = t;
	float dominant_world_extent = max(max(u_volume_size.x, u_volume_size.y), u_volume_size.z);
	float world_per_local = max(dominant_world_extent * 0.5, 0.0001);
	float voxel_distance_world = max(length(u_voxel_size), 0.0008);
	float voxel_distance_local = voxel_distance_world / world_per_local;
	float surface_epsilon_local = u_surface_epsilon / world_per_local;
	float hit_threshold = max(surface_epsilon_local * 2.8, voxel_distance_local * 1.4);

	for (int i = 0; i < 1024; ++i) {
		if (i >= u_max_steps || t > t_far) {
			break;
		}

		vec3 local_position = ray_origin_local + ray_dir_local * t;
		vec4 sample_value = se_sample_volume(local_position);
		float sdf = sample_value.a;
		float abs_sdf = abs(sdf);

		if (abs_sdf < best_abs_sdf) {
			best_abs_sdf = abs_sdf;
			best_local = local_position;
			best_color = sample_value.rgb;
			best_t = t;
		}

		if (abs_sdf <= hit_threshold) {
			hit = true;
			hit_local = local_position;
			hit_color = sample_value.rgb;
			break;
		}

		float step_local = max(abs_sdf, u_min_step);
		t += step_local;
	}

	if (!hit && best_abs_sdf <= hit_threshold * 1.8) {
		hit = true;
		hit_local = best_local;
		hit_color = best_color;
		t = best_t;
	}

	if (!hit) {
		float proximity = exp(-best_abs_sdf * 10.0);
		vec3 fallback = mix(vec3(0.05, 0.08, 0.12), best_color, clamp(proximity, 0.0, 1.0));
		frag_color = vec4(fallback, 1.0);
		return;
	}

	vec3 normal_local = se_estimate_normal(hit_local);
	vec3 light_dir_local = normalize(vec3(0.52, 0.78, 0.34));
	float diffuse = max(dot(normal_local, light_dir_local), 0.0);
	vec3 view_dir_local = normalize(ray_origin_local - hit_local);
	float rim = pow(clamp(1.0 - max(dot(normal_local, view_dir_local), 0.0), 0.0, 1.0), 2.6);

	vec3 lit_color = hit_color * (0.20 + 0.80 * diffuse) + hit_color * (0.10 * rim);
	frag_color = vec4(lit_color, 1.0);
}
