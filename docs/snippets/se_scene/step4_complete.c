#include "se_scene.h"

int main(void) {
	s_vec2 size = s_vec2(1280.0f, 720.0f);
	se_scene_2d_handle scene = se_scene_2d_create(&size, 16);
	se_object_2d_handle object = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/scene2d_frag.glsl"), &s_mat3_identity, 0);
	se_scene_2d_add_object(scene, object);
	s_vec2 position = s_vec2(0.25f, -0.10f);
	se_object_2d_set_position(object, &position);
	se_scene_2d_render_to_screen(scene, S_HANDLE_NULL);
	s_vec2 readback = se_object_2d_get_position(object);
	(void)readback;
	se_scene_2d_render_to_buffer(scene);
	se_object_2d_destroy(object);
	se_scene_2d_destroy(scene);
	return 0;
}
