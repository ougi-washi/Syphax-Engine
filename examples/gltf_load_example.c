// Syphax-Engine - Ougi Washi

#include "loader/se_gltf.h"

#include <stdio.h>

int main(void) {
	char gltf_path[SE_MAX_PATH_LENGTH] = {0};
	se_paths_resolve_resource_path(gltf_path, SE_MAX_PATH_LENGTH, "examples/gltf/triangle.gltf");

	se_gltf_asset *asset = se_gltf_load(gltf_path, NULL);

	printf("gltf_load_example :: loaded '%s'\n", gltf_path);
	printf("gltf_load_example :: meshes=%zu materials=%zu textures=%zu images=%zu nodes=%zu scenes=%zu\n",
		asset->meshes.size,
		asset->materials.size,
		asset->textures.size,
		asset->images.size,
		asset->nodes.size,
		asset->scenes.size);

	se_gltf_free(asset);
	return 0;
}
