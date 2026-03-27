#include "se_camera.h"
#include "se_sdf.h"

int main(void) {
	se_camera_handle camera = S_HANDLE_NULL;
	se_sdf_handle scene = se_sdf_create(
		.operation = SE_SDF_SMOOTH_UNION,
		.operation_amount = 0.35f);
	se_sdf_handle sphere = se_sdf_create(
		.type = SE_SDF_SPHERE,
		.shading = {
			.ambient = {0.09f, 0.03f, 0.05f},
			.diffuse = {0.92f, 0.28f, 0.42f},
			.specular = {0.80f, 0.74f, 0.78f},
			.roughness = 0.25f,
			.shadow_bias = 0.025f,
			.shadow_smooothness = 1.0f,
		},
		.sphere = {.radius = 1.0f});
	se_sdf_handle ground = se_sdf_create(
		.type = SE_SDF_BOX,
		.shading = {
			.diffuse = {0.28f, 0.31f, 0.34f},
			.roughness = 0.92f,
			.shadow_smooothness = 4.0f,
		},
		.box = {.size = {10.0f, 0.01f, 10.0f}});
	se_sdf_point_light_handle point_light = se_sdf_add_point_light(scene,
		.position = {2.5f, 3.0f, -2.0f},
		.color = {1.0f, 0.2f, 0.1f},
		.radius = 9.0f,
	);
	se_sdf_directional_light_handle sun = se_sdf_add_directional_light(scene,
		.direction = {0.45f, 0.85f, 0.35f},
		.color = {0.82f, 0.90f, 1.0f},
	);
	se_sdf_set_position(sphere, &s_vec3(0.0f, 1.0f, 0.0f));
	se_sdf_add_noise(sphere,
		.type = SE_SDF_NOISE_PERLIN,
		.frequency = 1.25f,
	);
	se_sdf_point_light_set_radius(point_light, 9.0f);
	se_sdf_directional_light_set_direction(sun, &s_vec3(0.45f, 0.85f, 0.35f));
	se_sdf_add_child(scene, ground);
	se_sdf_add_child(scene, sphere);
	se_sdf_render(scene, camera);
	se_sdf_destroy(scene);
	return 0;
}
