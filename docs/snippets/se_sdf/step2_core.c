#include "se_sdf.h"

int main(void) {
	se_sdf_scene_desc scene_desc = SE_SDF_SCENE_DESC_DEFAULTS;
	se_sdf_scene_handle scene = se_sdf_scene_create(&scene_desc);
	se_sdf_scene_clear(scene);
	se_sdf_node_group_desc group = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	se_sdf_node_handle root = se_sdf_node_create_group(scene, &group);
	se_sdf_scene_set_root(scene, root);
	return 0;
}
