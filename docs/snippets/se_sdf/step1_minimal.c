#include "se_sdf.h"

int main(void) {
	se_sdf_scene_desc scene_desc = SE_SDF_SCENE_DESC_DEFAULTS;
	se_sdf_scene_handle scene = se_sdf_scene_create(&scene_desc);
	se_sdf_scene_clear(scene);
	return 0;
}
