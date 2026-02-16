#include "loader/se_loader.h"

int main(void) {
	const se_gltf_asset* asset = 0;
	se_model_handle model = se_gltf_model_load(asset, 0);
	se_texture_handle texture = se_gltf_texture_load(asset, 0, SE_REPEAT);
	s_vec3 center = s_vec3(0.0f, 0.0f, 0.0f);
	f32 radius = 0.0f;
	se_gltf_scene_compute_bounds(asset, &center, &radius);
	se_model_handle first = se_gltf_model_load_first(SE_RESOURCE_PUBLIC("models/cube.gltf"), 0);
	(void)first;
	se_model_destroy(model);
	se_model_destroy(first);
	se_texture_destroy(texture);
	return 0;
}
