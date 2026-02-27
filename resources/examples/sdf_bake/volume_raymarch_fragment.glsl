#version 300 es

precision highp float;
precision mediump int;

out vec4 frag_color;

uniform sampler3D u_volume_texture;
uniform mat4 u_model_inverse;
uniform mat4 u_mvp;
uniform vec3 u_volume_size;
uniform vec3 u_voxel_size;
uniform vec3 u_camera_position;
uniform vec3 u_camera_forward;
uniform vec3 u_camera_right;
uniform vec3 u_camera_up;
uniform vec2 u_viewport_size;
uniform float u_camera_tan_half_fov;
uniform float u_camera_aspect;
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
	vec3 local_epsilon = max(texel_uv * 0.35, vec3(0.00025));

	float dx = se_sample_sdf(local_position + vec3(local_epsilon.x, 0.0, 0.0))
		- se_sample_sdf(local_position - vec3(local_epsilon.x, 0.0, 0.0));
	float dy = se_sample_sdf(local_position + vec3(0.0, local_epsilon.y, 0.0))
		- se_sample_sdf(local_position - vec3(0.0, local_epsilon.y, 0.0));
	float dz = se_sample_sdf(local_position + vec3(0.0, 0.0, local_epsilon.z))
		- se_sample_sdf(local_position - vec3(0.0, 0.0, local_epsilon.z));

	vec3 normal = vec3(dx, dy, dz);
	float normal_len = length(normal);
	if (normal_len < 0.00001) {
		return vec3(0.0);
	}
	return normal / normal_len;
}

bool se_near_volume_boundary(vec3 local_position, vec3 texel_uv) {
	vec3 uv = local_position * 0.5 + 0.5;
	vec3 margin = texel_uv * 1.2;
	return any(lessThanEqual(uv, margin)) || any(greaterThanEqual(uv, vec3(1.0) - margin));
}

float se_sample_sdf_local(vec3 local_position, float world_per_local) {
	return se_sample_volume(local_position).a / world_per_local;
}

bool se_segment_sign_crosses(float a, float b) {
	return (a < 0.0 && b > 0.0) || (a > 0.0 && b < 0.0);
}

float se_refine_sign_crossing(
	vec3 ray_origin_local,
	vec3 ray_dir_local,
	float t_a,
	float t_b,
	float world_per_local
) {
	float a = t_a;
	float b = t_b;
	float sdf_a = se_sample_sdf_local(ray_origin_local + ray_dir_local * a, world_per_local);
	for (int i = 0; i < 6; ++i) {
		float mid = 0.5 * (a + b);
		float sdf_mid = se_sample_sdf_local(ray_origin_local + ray_dir_local * mid, world_per_local);
		if (!se_segment_sign_crosses(sdf_a, sdf_mid)) {
			a = mid;
			sdf_a = sdf_mid;
		} else {
			b = mid;
		}
	}
	return 0.5 * (a + b);
}

void main() {
	if (u_viewport_size.x <= 1.0 || u_viewport_size.y <= 1.0) {
		discard;
	}
	vec2 ndc = vec2(
		(gl_FragCoord.x / u_viewport_size.x) * 2.0 - 1.0,
		(gl_FragCoord.y / u_viewport_size.y) * 2.0 - 1.0
	);
	vec3 ray_dir_world = normalize(
		u_camera_forward +
		u_camera_right * (ndc.x * u_camera_tan_half_fov * u_camera_aspect) +
		u_camera_up * (ndc.y * u_camera_tan_half_fov)
	);
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

	float t = max(t_near, 0.0) + u_min_step;
	vec3 hit_local = vec3(0.0);
	vec3 hit_color = vec3(1.0);
	bool hit = false;
	float dominant_world_extent = max(max(u_volume_size.x, u_volume_size.y), u_volume_size.z);
	float world_per_local = max(dominant_world_extent * 0.5, 0.0001);
	float voxel_distance_world = max(length(u_voxel_size), 0.0008);
	float voxel_distance_local = voxel_distance_world / world_per_local;
	float surface_epsilon_local = u_surface_epsilon / world_per_local;
	float hit_threshold = max(surface_epsilon_local * 0.55, voxel_distance_local * 0.06);
	vec3 texel_uv = 1.0 / vec3(textureSize(u_volume_texture, 0));
	vec3 initial_local = ray_origin_local + ray_dir_local * t;
	vec4 initial_sample = se_sample_volume(initial_local);
	float prev_t = t;
	float prev_sdf = initial_sample.a / world_per_local;
	bool prev_near_boundary = se_near_volume_boundary(initial_local, texel_uv);

	if (!prev_near_boundary && abs(prev_sdf) <= hit_threshold) {
		hit_local = initial_local;
		hit_color = initial_sample.rgb;
		hit = true;
	}

	for (int i = 0; i < 1024 && !hit; ++i) {
		if (i >= u_max_steps || prev_t > t_far) {
			break;
		}

		float step_local = max(abs(prev_sdf) * 0.9, u_min_step);
		float sample_t = prev_t + step_local;
		if (sample_t > t_far) {
			break;
		}
		vec3 local_position = ray_origin_local + ray_dir_local * sample_t;
		vec4 sample_value = se_sample_volume(local_position);
		float sdf_local = sample_value.a / world_per_local;
		float abs_sdf = abs(sdf_local);
		bool near_volume_boundary = se_near_volume_boundary(local_position, texel_uv);

		if (!near_volume_boundary && abs_sdf <= hit_threshold) {
			hit_local = local_position;
			hit_color = sample_value.rgb;
			hit = true;
			break;
		}

		if (!prev_near_boundary && !near_volume_boundary && se_segment_sign_crosses(prev_sdf, sdf_local)) {
			float refined_t = se_refine_sign_crossing(ray_origin_local, ray_dir_local, prev_t, sample_t, world_per_local);
			vec3 refined_local = ray_origin_local + ray_dir_local * refined_t;
			if (!se_near_volume_boundary(refined_local, texel_uv)) {
				hit_local = refined_local;
				hit_color = se_sample_volume(hit_local).rgb;
				hit = true;
				break;
			}
		}

		prev_t = sample_t;
		prev_sdf = sdf_local;
		prev_near_boundary = near_volume_boundary;
	}

	if (!hit) {
		discard;
	}

	vec3 normal_local = se_estimate_normal(hit_local);
	if (dot(normal_local, normal_local) < 0.25) {
		normal_local = normalize(ray_origin_local - hit_local);
	}
	vec3 light_dir_local = normalize(vec3(0.45, 0.75, 0.42));
	float diffuse = max(dot(normal_local, light_dir_local), 0.0);
	vec3 lit_color = hit_color * (0.30 + 0.70 * diffuse);

	vec4 hit_clip = u_mvp * vec4(hit_local, 1.0);
	float hit_ndc_z = hit_clip.z / max(hit_clip.w, 0.00001);
	gl_FragDepth = clamp(hit_ndc_z * 0.5 + 0.5, 0.0, 1.0);
	frag_color = vec4(lit_color, 1.0);
}
