// Syphax-Engine - Ougi Washi

#include "loader/se_gltf.h"

#include <stdio.h>

int main(void) {
	printf("advanced/gltf_roundtrip :: Advanced example (reference)\n");
	char gltf_path[SE_MAX_PATH_LENGTH] = {0};
	se_paths_resolve_resource_path(gltf_path, SE_MAX_PATH_LENGTH, SE_RESOURCE_EXAMPLE("gltf/triangle.gltf"));

	se_gltf_asset *asset = se_gltf_load(gltf_path, NULL);
	if (asset == NULL) {
		fprintf(stderr, "14_gltf_load :: failed to load '%s'\n", gltf_path);
		return 1;
	}

	printf("14_gltf_load :: loaded '%s'\n", gltf_path);
	printf("14_gltf_load :: meshes=%zu materials=%zu textures=%zu images=%zu nodes=%zu scenes=%zu\n",
		s_array_get_size(&asset->meshes),
		s_array_get_size(&asset->materials),
		s_array_get_size(&asset->textures),
		s_array_get_size(&asset->images),
		s_array_get_size(&asset->nodes),
		s_array_get_size(&asset->scenes));

	char glb_path[SE_MAX_PATH_LENGTH] = {0};
	snprintf(glb_path, sizeof(glb_path), "/tmp/syphax_triangle_roundtrip.glb");
	se_gltf_write_params write_params = {0};
	write_params.write_glb = true;
	if (!se_gltf_write(asset, glb_path, &write_params)) {
		fprintf(stderr, "14_gltf_load :: failed to write '%s'\n", glb_path);
		se_gltf_free(asset);
		return 1;
	}

	se_gltf_asset *roundtrip = se_gltf_load(glb_path, NULL);
	if (roundtrip == NULL) {
		fprintf(stderr, "14_gltf_load :: failed to reload '%s'\n", glb_path);
		se_gltf_free(asset);
		return 1;
	}
	if (s_array_get_size(&roundtrip->meshes) != s_array_get_size(&asset->meshes) ||
		s_array_get_size(&roundtrip->nodes) != s_array_get_size(&asset->nodes) ||
		s_array_get_size(&roundtrip->scenes) != s_array_get_size(&asset->scenes)) {
		fprintf(stderr, "14_gltf_load :: roundtrip mismatch for '%s'\n", glb_path);
		se_gltf_free(roundtrip);
		se_gltf_free(asset);
		return 1;
	}
	printf("14_gltf_load :: glb roundtrip ok '%s'\n", glb_path);

	se_gltf_free(roundtrip);
	se_gltf_free(asset);
	return 0;
}
