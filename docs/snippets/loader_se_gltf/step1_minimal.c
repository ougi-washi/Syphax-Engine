#include "loader/se_gltf.h"

int main(void) {
	se_gltf_load_params params = {0};
	se_gltf_asset* asset = se_gltf_load(SE_RESOURCE_PUBLIC("models/cube.gltf"), &params);
	return 0;
}
