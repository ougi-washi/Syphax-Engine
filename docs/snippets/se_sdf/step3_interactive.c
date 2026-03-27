#include "se_sdf.h"

int main(void) {
	se_sdf_handle scene = se_sdf_create(
		.operation = SE_SDF_SMOOTH_UNION,
		.operation_amount = 0.35f);
	se_sdf_handle sphere = se_sdf_create(
		.type = SE_SDF_SPHERE,
		.shading = {
			.diffuse = {0.92f, 0.28f, 0.42f},
			.specular = {0.80f, 0.74f, 0.78f},
			.roughness = 0.25f,
			.shadow_smooothness = 1.0f,
		},
		.sphere = {.radius = 1.0f});
	se_sdf_noise_handle noise = se_sdf_add_noise(sphere,
		.type = SE_SDF_NOISE_PERLIN,
		.frequency = 1.25f,
	);
	se_sdf_point_light_handle point_light = se_sdf_add_point_light(scene,
		.position = {2.5f, 3.0f, -2.0f},
		.color = {1.0f, 0.2f, 0.1f},
		.radius = 9.0f,
	);
	se_sdf_set_position(sphere, &s_vec3(0.0f, 1.0f, 0.0f));
	se_sdf_noise_set_frequency(noise, 1.5f);
	se_sdf_noise_set_offset(noise, &s_vec3(0.5f, 0.0f, 0.0f));
	se_sdf_point_light_set_position(point_light, &s_vec3(3.0f, 4.0f, -2.0f));
	se_sdf_add_child(scene, sphere);
	se_sdf_destroy(scene);
	return 0;
}
