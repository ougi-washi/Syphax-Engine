#include "se_sdf.h"

int main(void) {
	se_sdf_handle scene = se_sdf_create(
		.operation = SE_SDF_SMOOTH_UNION,
		.operation_amount = 0.35f);
	se_sdf_handle sphere = se_sdf_create(
		.type = SE_SDF_SPHERE,
		.sphere = {.radius = 1.0f});
	se_sdf_handle ground = se_sdf_create(
		.type = SE_SDF_BOX,
		.box = {.size = {10.0f, 0.01f, 10.0f}});
	se_sdf_add_child(scene, ground);
	se_sdf_add_child(scene, sphere);
	se_sdf_destroy(scene);
	return 0;
}
