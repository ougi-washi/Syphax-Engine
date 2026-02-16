#include "loader/se_gltf.h"

int main(void) {
	se_gltf_load_params params = {0};
	se_gltf_asset* asset = se_gltf_load(SE_RESOURCE_PUBLIC("models/cube.gltf"), &params);
	se_gltf_write_params write_params = {0};
	se_gltf_write(asset, SE_RESOURCE_EXAMPLE("gltf/export.glb"), &write_params);
	if (!asset) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
	}
	se_gltf_free(asset);
	return 0;
}
