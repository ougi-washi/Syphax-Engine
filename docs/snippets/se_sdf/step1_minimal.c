#include "se_sdf.h"

int main(void) {
	se_sdf_handle sphere = se_sdf_create(
		.type = SE_SDF_SPHERE,
		.sphere = {.radius = 1.0f});
	se_sdf_destroy(sphere);
	return 0;
}
