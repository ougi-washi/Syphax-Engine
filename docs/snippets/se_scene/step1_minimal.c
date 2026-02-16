#include "se_scene.h"

int main(void) {
	s_vec2 size = s_vec2(1280.0f, 720.0f);
	se_scene_2d_handle scene = se_scene_2d_create(&size, 16);
	se_object_2d_handle object = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/scene2d_frag.glsl"), &s_mat3_identity, 0);
	se_scene_2d_add_object(scene, object);
	return 0;
}
