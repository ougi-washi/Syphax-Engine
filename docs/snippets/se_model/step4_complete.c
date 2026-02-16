#include "se_model.h"

int main(void) {
	se_model_handle model = se_model_load_obj_simple(
		SE_RESOURCE_PUBLIC("models/sphere.obj"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl"),
		SE_RESOURCE_EXAMPLE("scene3d/scene3d_fragment.glsl")
	);
	s_vec3 scale = s_vec3(1.2f, 1.2f, 1.2f);
	s_vec3 rotation = s_vec3(0.0f, 0.01f, 0.0f);
	se_model_scale(model, &scale);
	se_model_rotate(model, &rotation);
	sz mesh_count = se_model_get_mesh_count(model);
	(void)mesh_count;
	se_model_render(model, S_HANDLE_NULL);
	se_model_destroy(model);
	return 0;
}
