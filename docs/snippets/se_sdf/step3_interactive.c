#include "se_sdf.h"

int main(void) {
	se_sdf_desc sdf_desc = SE_SDF_DESC_DEFAULTS;
	se_sdf_handle sdf = se_sdf_create(&sdf_desc);
	se_sdf_clear(sdf);
	se_sdf_node_group_desc group = SE_SDF_NODE_GROUP_DESC_DEFAULTS;
	se_sdf_node_handle root = se_sdf_node_create_group(sdf, &group);
	se_sdf_set_root(sdf, root);
	s_mat4 transform = s_mat4_identity;
	se_sdf_node_set_transform(sdf, root, &transform);
	return 0;
}
