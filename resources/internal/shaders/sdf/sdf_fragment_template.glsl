#version 330 core

in vec2 v_uv;
out vec4 frag_color;

{{SDF_UNIFORMS}}

{{SDF_COMMON_CONSTANTS}}
{{SDF_COMMON}}
{{SDF_DISTANCE_FUNCTIONS}}
{{SDF_SURFACE_FUNCTIONS}}
{{SDF_SCENE_WRAPPERS}}

void main() {
	vec3 ray_origin = vec3(0.0);
	vec3 ray_dir = vec3(0.0, 0.0, 1.0);
	sdf_get_view_ray(v_uv, ray_origin, ray_dir);
	vec3 sky = sdf_sky(ray_dir);
	float travel = 0.0;
	vec3 hit_position = ray_origin;
	bool hit = false;
	for (int i = 0; i < SDF_MAX_TRACE_STEPS; ++i) {
		if (travel > SDF_FAR_DISTANCE) {
			break;
		}
		hit_position = ray_origin + ray_dir * travel;
		float distance_to_surface = scene_sdf(hit_position);
		if (distance_to_surface < SDF_HIT_EPSILON) {
			hit = true;
			break;
		}
		travel += max(distance_to_surface, SDF_HIT_EPSILON);
	}
	if (!hit) {
		frag_color = vec4(sky, 1.0);
		return;
	}
	vec3 normal = sdf_estimate_normal(hit_position);
	sdf_surface_data surface = scene_surface(hit_position);
	sdf_shading_data shading = surface.shading;
	vec3 view_direction = normalize(u_use_orthographic != 0 ? -ray_dir : (u_camera_position - hit_position));
	vec3 diffuse_lighting = vec3(0.0);
	vec3 specular_lighting = vec3(0.0);
	float shadow_visibility = 1.0;
{{SDF_LIGHT_APPLY}}
	vec3 color = shading.ambient + shading.diffuse * diffuse_lighting + shading.specular * specular_lighting;
	float fog = clamp(travel / 48.0, 0.0, 1.0);
	frag_color = vec4(mix(color, sky, fog), 1.0);
}
