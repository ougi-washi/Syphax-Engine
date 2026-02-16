#include "loader/se_loader.h"

int main(void) {
	const se_gltf_asset* asset = 0;
	se_model_handle model = se_gltf_model_load(asset, 0);
	return 0;
}
